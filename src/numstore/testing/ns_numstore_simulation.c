/// Copyright 2026 Theo Lincke
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.

#include "numstore/testing/ns_numstore_simulation.h"

#include "core/ns_arena_alloc.h"
#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_logging.h"
#include "core/ns_numerics.h"
#include "core/os/ns_filesystem.h"
#include "core/os/ns_memory.h"
#include "core/os/ns_time.h"
#include "nscore/types/ns_types.h"
#include "numstore/numstore.h"
#include "numstore/testing/ns_actual_db_stepper.h"
#include "numstore/testing/ns_operation_generator.h"
#include "numstore/testing/ns_reference_db_stepper.h"

#ifdef TESTING
#  include "core/testing/ns_testing.h"
#endif

struct ns_simulation
{
  struct ns_ref       *ref;
  struct ns_db        *db;

  // Which actions are turned on
  u8                   enabled[NSS_AT_LEN];

  const char          *dbname;
  int                  max_insert_len;
  float                sample_space_prob;

  // Run identity
  u64                  seed;
  const char          *commit_hash;
  u64                  sequence_id;

  // Run metrics
  u64                  start;       // Epoch for everything below
  u64                  step_number; // Which step is the test in
  u64                  clock;       // Absolute timestamp of the last observation

  // Throughput: every byte inserted, removed, read or written over the run
  u64                  total_bytes_moved;

  i_timer              timer;
  struct arena_alloc   alloc;

  // The file system used by the system under test
  // (can be faulty)
  struct i_file_system test_filesystem;

  // Memory used by the test (can be faulty)
  struct i_mem         test_mem;

  // Memory used for things that aren't being tested
  struct i_mem         reliable_mem;
};

////////// ACTIONS

static err_t
nss_begin_txn (struct ns_simulation *meta, error *e)
{
  WRAP (ns_ref_begin_txn (meta->ref, e));
  WRAP (ns_db_begin_txn (meta->db, e));
  return SUCCESS;
}

static err_t
nss_commit_txn (struct ns_simulation *meta, error *e)
{
  WRAP (ns_ref_commit_txn (meta->ref, e));
  WRAP (ns_db_commit_txn (meta->db, e));
  return SUCCESS;
}

static err_t
nss_rollback_txn (struct ns_simulation *meta, error *e)
{
  ns_ref_rollback_txn (meta->ref);
  WRAP (ns_db_rollback_txn (meta->db, e));
  return SUCCESS;
}

static err_t
nss_crash_and_reopen (struct ns_simulation *meta, error *e)
{
  ns_ref_crash_and_reopen (meta->ref);
  WRAP (ns_db_crash_and_reopen (meta->db, e));
  return SUCCESS;
}

static err_t
nss_close_and_reopen (struct ns_simulation *meta, error *e)
{
  ns_ref_close_and_reopen (meta->ref);
  WRAP (ns_db_close_and_reopen (meta->db, e));
  return SUCCESS;
}

static err_t
nss_create (struct ns_simulation *meta, struct operation *op, error *e)
{
  WRAP (ns_ref_create_and_maybe_switch (meta->ref, op->op_create.vname, op->op_create.t, e));
  WRAP (ns_db_create_and_maybe_switch (meta->db, op->op_create.vname, *op->op_create.t, e));
  return SUCCESS;
}

static err_t
nss_switch (struct ns_simulation *meta, struct operation *op, error *e)
{
  ns_ref_switch (meta->ref, op->op_switch.vname);
  WRAP (ns_db_switch (meta->db, op->op_switch.vname, e));
  return SUCCESS;
}

static err_t
nss_delete (struct ns_simulation *meta, struct operation *op, error *e)
{
  ns_ref_delete_cur_and_switch (meta->ref, op->op_delete.next);
  WRAP (ns_db_delete_cur_and_switch (meta->db, op->op_delete.next, e));
  return SUCCESS;
}

static err_t
nss_insert (struct ns_simulation *meta, struct operation *op, error *e)
{
  WRAP (ns_ref_insert (meta->ref, op->op_insert.data, op->op_insert.ofst, op->op_insert.nelems, e));
  WRAP (ns_db_insert (meta->db, op->op_insert.data, op->op_insert.ofst, op->op_insert.nelems, e));
  return SUCCESS;
}

static err_t
nss_remove (struct ns_simulation *meta, struct operation *op, error *e)
{
  ns_ref_remove (
      meta->ref,
      op->op_remove.ref_dest,
      (struct stride){
          .start  = op->op_remove.start,
          .stride = op->op_remove.stride,
          .nelems = op->op_remove.nelems,
      }
  );
  WRAP (ns_db_remove (
      meta->db,
      op->op_remove.db_dest,
      (struct stride){
          .start  = op->op_remove.start,
          .stride = op->op_remove.stride,
          .nelems = op->op_remove.nelems,
      },
      e
  ));
  return SUCCESS;
}

static err_t
nss_read (struct ns_simulation *meta, struct operation *op, error *e)
{
  ns_ref_read (
      meta->ref,
      op->op_read.ref_dest,
      (struct stride){
          .start  = op->op_read.start,
          .stride = op->op_read.stride,
          .nelems = op->op_read.nelems,
      }
  );
  WRAP (ns_db_read (
      meta->db,
      op->op_read.db_dest,
      (struct stride){
          .start  = op->op_read.start,
          .stride = op->op_read.stride,
          .nelems = op->op_read.nelems,
      },
      e
  ));
  return SUCCESS;
}

static err_t
nss_write (struct ns_simulation *meta, struct operation *op, error *e)
{
  ns_ref_write (
      meta->ref,
      op->op_write.data,
      (struct stride){
          .start  = op->op_write.start,
          .stride = op->op_write.stride,
          .nelems = op->op_write.nelems,
      }
  );
  WRAP (ns_db_write (
      meta->db,
      op->op_write.data,
      (struct stride){
          .start  = op->op_write.start,
          .stride = op->op_write.stride,
          .nelems = op->op_write.nelems,
      },
      e
  ));
  return SUCCESS;
}

////////// LOGGING

static void
format_quoted_str (char *buf, size_t bufsize, const char *str)
{
  size_t maxlen    = bufsize - 6; /* was -5: off by one, ate the closing quote */
  size_t len       = strlen (str);
  bool   truncated = len > maxlen;
  snprintf (
      buf,
      bufsize,
      "\"%.*s%s\"",
      (int)(truncated ? maxlen : len),
      str,
      truncated ? "..." : ""
  );
}

static void
nss_log_operation (struct ns_simulation *meta, struct operation *op, bool completed)
{
  const char *completed_str = completed ? "true" : "false";
  const char *in_txn_str    = meta->ref->in_txn ? "true" : "false";
  const bool  has_var       = ns_ref_nvars (meta->ref) > 0;

  print_json_start ();

  switch (op->type) {
    case NSS_BEGIN_TXN: print_entry ("action", "\"begin_txn\""); break;
    case NSS_COMMIT_TXN: print_entry ("action", "\"commit_txn\""); break;
    case NSS_ROLLBACK_TXN: print_entry ("action", "\"rollback_txn\""); break;
    case NSS_CRASH_AND_REOPEN: print_entry ("action", "\"crash_and_reopen\""); break;
    case NSS_CLOSE_AND_REOPEN: print_entry ("action", "\"close_and_reopen\""); break;
    case NSS_CREATE_AND_SWAP_IF_EMPTY: print_entry ("action", "\"create\""); break;
    case NSS_SWITCH: print_entry ("action", "\"switch\""); break;
    case NSS_DELETE_CURRENT_VARIABLE_AND_SWITCH: print_entry ("action", "\"delete\""); break;
    case NSS_INSERT: print_entry ("action", "\"insert\""); break;
    case NSS_REMOVE: print_entry ("action", "\"remove\""); break;
    case NSS_READ: print_entry ("action", "\"read\""); break;
    case NSS_WRITE: print_entry ("action", "\"write\""); break;
    case NSS_NONE_AVAILABLE: print_entry ("action", "\"none_available\""); break;
    case NSS_AT_LEN: UNREACHABLE (); break;
  }

  print_entry ("completed", completed_str);
  print_entry ("in_txn", in_txn_str);

  // Step number
  {
    char buf[32];
    snprintf (buf, sizeof (buf), "%" PRIu64, meta->step_number);
    print_entry ("step", buf);
  }

  // Seed
  {
    char buf[32];
    snprintf (buf, sizeof (buf), "%" PRIu64, meta->seed);
    print_entry ("seed", buf);
  }

  // Commit Hash
  {
    char buf[256];
    format_quoted_str (buf, sizeof (buf), meta->commit_hash);
    print_entry ("commit_hash", buf);
  }

  // Sequence Id
  {
    char buf[32];
    snprintf (buf, sizeof (buf), "%" PRIu64, meta->sequence_id);
    print_entry ("sequence_id", buf);
  }

  // Database Name
  {
    char buf[256];
    format_quoted_str (buf, sizeof (buf), meta->dbname);
    print_entry ("dbname", buf);
  }

  // Max Insert Length
  {
    char buf[32];
    snprintf (buf, sizeof (buf), "%d", meta->max_insert_len);
    print_entry ("max_insert_len", buf);
  }

  // Sample Space Probability
  {
    char buf[32];
    snprintf (buf, sizeof (buf), "%g", (double)meta->sample_space_prob);
    print_entry ("sample_space_prob", buf);
  }

  // Operation Duration Ms
  {
    char buf[32];
    snprintf (buf, sizeof (buf), "%.6f", (double)meta->db->prev_op_duration_ns / 1e6);
    print_entry ("op_duration_ms", buf);
  }

  // Total Working Ms
  {
    char buf[32];
    snprintf (buf, sizeof (buf), "%.6f", (double)meta->db->total_working_ns / 1e6);
    print_entry ("total_working_ms", buf);
  }

  // Database Size
  {
    char buf[32];
    snprintf (buf, sizeof (buf), "%.9f", (double)meta->db->db_size_bytes / 1e9);
    print_entry ("db_size_gb", buf);
  }

  // Elapsed time
  {
    char buf[32];
    snprintf (
        buf,
        sizeof (buf),
        "%.6f",
        (double)(i_timer_now_ns (&meta->timer) - meta->start) / 1e6
    );
    print_entry ("elapsed_ms", buf);
  }

  // number of variables
  {
    char buf[32];
    snprintf (buf, sizeof (buf), "%" PRIu32, ns_ref_nvars (meta->ref));
    print_entry ("nvars", buf);
  }

  // Tracked gb
  {
    char buf[32];
    snprintf (buf, sizeof (buf), "%.9f", (double)ns_ref_tracked_bytes (meta->ref) / 1e9);
    print_entry ("tracked_gb", buf);
  }

  if (has_var) {
    // Current variable name
    {
      char buf[256];
      format_quoted_str (buf, sizeof (buf), ns_ref_cur_name (meta->ref));
      print_entry ("cur_var", buf);
    }

    // Current variable length
    {
      char buf[32];
      snprintf (buf, sizeof (buf), "%" PRb_size, ns_ref_cur_len (meta->ref));
      print_entry ("cur_var_len", buf);
    }

    // Current variable tsize
    {
      char buf[32];
      snprintf (buf, sizeof (buf), "%" PRt_size, ns_ref_cur_tsize (meta->ref));
      print_entry ("cur_var_tsize", buf);
    }
  }

  switch (op->type) {
    case NSS_BEGIN_TXN:
    case NSS_COMMIT_TXN:
    case NSS_ROLLBACK_TXN:
    case NSS_CRASH_AND_REOPEN:
    case NSS_CLOSE_AND_REOPEN:
    case NSS_NONE_AVAILABLE: {
      break;
    }

    case NSS_CREATE_AND_SWAP_IF_EMPTY: {
      // Variable name
      {
        char buf[256];
        format_quoted_str (buf, sizeof (buf), op->op_create.vname);
        print_entry ("vname", buf);
      }

      // Variable type
      {
        char buf[256];
        format_quoted_str (buf, sizeof (buf), op->op_create.typestr);
        print_entry ("type", buf);
      }
      break;
    }

    case NSS_SWITCH: {
      // Variable name
      {
        char buf[256];
        format_quoted_str (buf, sizeof (buf), op->op_switch.vname);
        print_entry ("vname", buf);
      }
      break;
    }

    case NSS_DELETE_CURRENT_VARIABLE_AND_SWITCH: {
      // Delete current variable
      {
        char        buf[256];
        const char *val = "null";
        if (op->op_delete.next) {
          format_quoted_str (buf, sizeof (buf), op->op_delete.next);
          val = buf;
        }
        print_entry ("next", val);
      }
      break;
    }

    case NSS_INSERT:
    case NSS_REMOVE:
    case NSS_READ:
    case NSS_WRITE: {
      u64 bytes_moved = 0;
      if (has_var) {
        t_size tsize = ns_ref_cur_tsize (meta->ref);
        switch (op->type) {
          case NSS_INSERT: bytes_moved = op->op_insert.nelems * tsize; break;
          case NSS_REMOVE: bytes_moved = op->op_remove.nelems * tsize; break;
          case NSS_READ: bytes_moved = op->op_read.nelems * tsize; break;
          case NSS_WRITE: bytes_moved = op->op_write.nelems * tsize; break;
          default: break;
        }
      }
      meta->total_bytes_moved += bytes_moved;

      const double op_ms    = (double)meta->db->prev_op_duration_ns / 1e6;
      const double total_ms = (double)meta->db->total_working_ns / 1e6;

      if (op->type == NSS_INSERT) {
        // Offset
        {
          char buf[32];
          snprintf (buf, sizeof (buf), "%" PRb_size, op->op_insert.ofst);
          print_entry ("ofst", buf);
        }
        // Number of elements
        {
          char buf[32];
          snprintf (buf, sizeof (buf), "%" PRb_size, op->op_insert.nelems);
          print_entry ("nelems", buf);
        }
      } else if (op->type == NSS_REMOVE) {
        // Start
        {
          char buf[32];
          snprintf (buf, sizeof (buf), "%" PRb_size, op->op_remove.start);
          print_entry ("start", buf);
        }
        // stride
        {
          char buf[32];
          snprintf (buf, sizeof (buf), "%" PRb_size, op->op_remove.stride);
          print_entry ("stride", buf);
        }
        // nelems
        {
          char buf[32];
          snprintf (buf, sizeof (buf), "%" PRb_size, op->op_remove.nelems);
          print_entry ("nelems", buf);
        }
      } else if (op->type == NSS_READ) {
        // start
        {
          char buf[32];
          snprintf (buf, sizeof (buf), "%" PRb_size, op->op_read.start);
          print_entry ("start", buf);
        }

        // stride
        {
          char buf[32];
          snprintf (buf, sizeof (buf), "%" PRb_size, op->op_read.stride);
          print_entry ("stride", buf);
        }

        // nelems
        {
          char buf[32];
          snprintf (buf, sizeof (buf), "%" PRb_size, op->op_read.nelems);
          print_entry ("nelems", buf);
        }
      } else if (op->type == NSS_WRITE) { /* NSS_WRITE */

        // start
        {
          char buf[32];
          snprintf (buf, sizeof (buf), "%" PRb_size, op->op_write.start);
          print_entry ("start", buf);
        }

        // stride
        {
          char buf[32];
          snprintf (buf, sizeof (buf), "%" PRb_size, op->op_write.stride);
          print_entry ("stride", buf);
        }

        // nelems
        {
          char buf[32];
          snprintf (buf, sizeof (buf), "%" PRb_size, op->op_write.nelems);
          print_entry ("nelems", buf);
        }
      } else {
        UNREACHABLE ();
      }

      // bytes moved
      {
        char buf[32];
        snprintf (buf, sizeof (buf), "%" PRIu64, bytes_moved);
        print_entry ("bytes_moved", buf);
      }

      // bytes per ms
      {
        char buf[32];
        if (op_ms > 0) {
          snprintf (buf, sizeof (buf), "%.3f", (double)bytes_moved / op_ms);
        } else {
          snprintf (buf, sizeof (buf), "null");
        }
        print_entry ("bytes_per_ms", buf);
      }

      // total bytes moved
      {
        char buf[32];
        snprintf (buf, sizeof (buf), "%" PRIu64, meta->total_bytes_moved);
        print_entry ("total_bytes_moved", buf);
      }

      // average bytes moved per ms
      {
        char buf[32];
        if (total_ms > 0) {
          snprintf (buf, sizeof (buf), "%.3f", (double)meta->total_bytes_moved / total_ms);
        } else {
          snprintf (buf, sizeof (buf), "null");
        }
        print_entry ("avg_bytes_per_ms", buf);
      }
      break;
    }

    case NSS_AT_LEN: UNREACHABLE (); break;
  }

  print_last_entry ("placeholder", "null");

  print_json_end ();
}

/******************************************************************************
 * SECTION: Main Api
 ******************************************************************************/

DEFINE_DBG_ASSERT (struct ns_simulation_params, ns_simulation_params, p, {
  ASSERT (p);
  ASSERT (p->commit_hash);
  ASSERT (p->dbname);
  ASSERT (p->max_insert_len > 0);
  ASSERT (p->sample_space_prob <= 1);
  ASSERT (p->sample_space_prob >= 0);
})

struct ns_simulation *
ns_simul_open (struct ns_simulation_params params, error *e)
{
  DBG_ASSERT (ns_simulation_params, &params);

  // Clean up the database before starting
  if (numstore_cleanup (params.dbname) < 0) {
    return NULL;
  }

  struct ns_simulation *ret = i_malloc (params.reliable_mem, 1, sizeof *ret, e);
  if (ret == NULL) {
    return NULL;
  }

  struct ns_ref *ref = ns_ref_new (default_mem (), e);
  if (ref == NULL) {
    i_free (params.reliable_mem, ret);
    return NULL;
  }

  struct ns_db *db = ns_db_new (
      params.reliable_mem,
      params.test_mem,
      params.test_filesystem,
      params.dbname,
      e
  );
  if (db == NULL) {
    ns_ref_free (ref);
    i_free (params.reliable_mem, ret);
    return NULL;
  }

  *ret = (struct ns_simulation){
      .ref               = ref,
      .db                = db,

      .dbname            = params.dbname,
      .max_insert_len    = params.max_insert_len,
      .sample_space_prob = params.sample_space_prob,

      .seed              = params.seed,
      .commit_hash       = params.commit_hash,
      .sequence_id       = params.sequence_id,

      .start             = 0,
      .step_number       = 0,
      .clock             = 0,
      .total_bytes_moved = 0,

      .reliable_mem      = params.reliable_mem,
      .test_mem          = params.test_mem,
      .test_filesystem   = params.test_filesystem,
  };

  memcpy (ret->enabled, params.enabled, sizeof (params.enabled));

  ret->start = i_timer_now_ns (&ret->timer);
  ret->clock = ret->start;

  return ret;
}

err_t
ns_simul_close (struct ns_simulation *meta, error *e)
{
  ns_ref_free (meta->ref);
  ns_db_close (meta->db, e);
  i_free (meta->reliable_mem, meta);
  return error_trace (e);
}

err_t
ns_simul_step (struct ns_simulation *meta, error *e)
{
  struct rand_op_params params = {
      .ref        = meta->ref,
      .enabled    = meta->enabled,
      .max_nelems = meta->max_insert_len,
      .mem        = meta->reliable_mem,
  };

  struct operation *op = opg_random (params, e);
  if (op == NULL) {
    return error_trace (e);
  }

  err_t result = SUCCESS;
  switch (op->type) {
    case NSS_BEGIN_TXN: result = nss_begin_txn (meta, e); break;
    case NSS_COMMIT_TXN: result = nss_commit_txn (meta, e); break;
    case NSS_ROLLBACK_TXN: result = nss_rollback_txn (meta, e); break;
    case NSS_CRASH_AND_REOPEN: result = nss_crash_and_reopen (meta, e); break;
    case NSS_CLOSE_AND_REOPEN: result = nss_close_and_reopen (meta, e); break;
    case NSS_CREATE_AND_SWAP_IF_EMPTY: result = nss_create (meta, op, e); break;
    case NSS_SWITCH: result = nss_switch (meta, op, e); break;
    case NSS_DELETE_CURRENT_VARIABLE_AND_SWITCH: result = nss_delete (meta, op, e); break;
    case NSS_INSERT: result = nss_insert (meta, op, e); break;
    case NSS_REMOVE: result = nss_remove (meta, op, e); break;
    case NSS_READ: result = nss_read (meta, op, e); break;
    case NSS_WRITE: result = nss_write (meta, op, e); break;
    case NSS_NONE_AVAILABLE: break;
    default: UNREACHABLE (); return -1;
  }

  nss_log_operation (meta, op, result >= SUCCESS);

  // Increment step
  meta->step_number++;

  // Choose a set of randomized actions
  if (op->type == NSS_NONE_AVAILABLE || randf () <= meta->sample_space_prob) {
    opg_spin_enabled (meta->enabled);
  }

  opg_free (op);

  if (result < SUCCESS) {
    return error_trace (e);
  }

  return SUCCESS;
}

#ifdef TESTING
TEST (ns_simul)
{
  error e = error_create ();

  TEST_CASE ("Smoke test")
  {
    struct ns_simulation_params params = {
        .seed              = 1234,
        .commit_hash       = "abcd",
        // .enabled
        .sequence_id       = 10,
        .dbname            = "foo",
        .max_insert_len    = 1000,
        .sample_space_prob = 1,
        .test_filesystem   = fs,
        .test_mem          = mem,
        .reliable_mem      = mem,
    };
    struct ns_simulation *simul = ns_simul_open (params, &e);

    ns_simul_step (simul, &e);
    ns_simul_step (simul, &e);
    ns_simul_step (simul, &e);

    ns_simul_close (simul, &e);
  }
}
#endif
