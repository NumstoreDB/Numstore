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

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

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
nss_log_operation (struct ns_simulation *meta, struct operation *op, bool completed)
{
  char step_buf[32];
  char seed_buf[32];
  char commit_hash_buf[256];
  char sequence_id_buf[32];
  char dbname_buf[256];
  char max_insert_len_buf[32];
  char sample_space_prob_buf[32];
  char op_duration_ms_buf[32];
  char total_working_ms_buf[32];
  char db_size_gb_buf[32];
  char elapsed_ms_buf[32];
  char ref_nvars_buf[32];
  char ref_tracked_gb_buf[32];
  char cur_var_buf[256];
  char cur_var_len_buf[32];
  char cur_var_tsize_buf[32];
  char bytes_moved_buf[32];
  char bytes_per_ms_buf[32];
  char total_bytes_moved_buf[32];
  char avg_bytes_per_ms_buf[32];

  // Throughput accounting: how many bytes this operation actually moved.
  // Only the data operations move bytes - the rest move zero.
  u64  bytes_moved = 0;
  if (ns_ref_nvars (meta->ref) > 0) {
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

  snprintf (bytes_moved_buf, sizeof (bytes_moved_buf), "%" PRIu64, bytes_moved);
  snprintf (
      total_bytes_moved_buf,
      sizeof (total_bytes_moved_buf),
      "%" PRIu64,
      meta->total_bytes_moved
  );

  if (op_ms > 0) {
    snprintf (bytes_per_ms_buf, sizeof (bytes_per_ms_buf), "%.3f", (double)bytes_moved / op_ms);
  } else {
    snprintf (bytes_per_ms_buf, sizeof (bytes_per_ms_buf), "null");
  }

  if (total_ms > 0) {
    snprintf (
        avg_bytes_per_ms_buf,
        sizeof (avg_bytes_per_ms_buf),
        "%.3f",
        (double)meta->total_bytes_moved / total_ms
    );
  } else {
    snprintf (avg_bytes_per_ms_buf, sizeof (avg_bytes_per_ms_buf), "null");
  }

  snprintf (step_buf, sizeof (step_buf), "%" PRIu64, meta->step_number);
  snprintf (seed_buf, sizeof (seed_buf), "%" PRIu64, meta->seed);
  snprintf (commit_hash_buf, sizeof (commit_hash_buf), "\"%s\"", meta->commit_hash);
  snprintf (sequence_id_buf, sizeof (sequence_id_buf), "%" PRIu64, meta->sequence_id);
  snprintf (dbname_buf, sizeof (dbname_buf), "\"%s\"", meta->dbname);
  snprintf (max_insert_len_buf, sizeof (max_insert_len_buf), "%d", meta->max_insert_len);
  snprintf (
      sample_space_prob_buf,
      sizeof (sample_space_prob_buf),
      "%g",
      (double)meta->sample_space_prob
  );
  snprintf (
      op_duration_ms_buf,
      sizeof (op_duration_ms_buf),
      "%.6f",
      (double)meta->db->prev_op_duration_ns / 1e6
  );
  snprintf (
      total_working_ms_buf,
      sizeof (total_working_ms_buf),
      "%.6f",
      (double)meta->db->total_working_ns / 1e6
  );
  snprintf (db_size_gb_buf, sizeof (db_size_gb_buf), "%.9f", (double)meta->db->db_size_bytes / 1e9);
  snprintf (
      elapsed_ms_buf,
      sizeof (elapsed_ms_buf),
      "%.6f",
      (double)(i_timer_now_ns (&meta->timer) - meta->start) / 1e6
  );
  snprintf (ref_nvars_buf, sizeof (ref_nvars_buf), "%" PRIu32, ns_ref_nvars (meta->ref));
  snprintf (
      ref_tracked_gb_buf,
      sizeof (ref_tracked_gb_buf),
      "%.9f",
      (double)ns_ref_tracked_bytes (meta->ref) / 1e9
  );

  if (ns_ref_nvars (meta->ref) > 0) {
    snprintf (cur_var_buf, sizeof (cur_var_buf), "\"%s\"", ns_ref_cur_name (meta->ref));
    snprintf (cur_var_len_buf, sizeof (cur_var_len_buf), "%" PRb_size, ns_ref_cur_len (meta->ref));
    snprintf (
        cur_var_tsize_buf,
        sizeof (cur_var_tsize_buf),
        "%" PRt_size,
        ns_ref_cur_tsize (meta->ref)
    );
  } else {
    snprintf (cur_var_buf, sizeof (cur_var_buf), "null");
    snprintf (cur_var_len_buf, sizeof (cur_var_len_buf), "null");
    snprintf (cur_var_tsize_buf, sizeof (cur_var_tsize_buf), "null");
  }

  const char *completed_str = completed ? "true" : "false";
  const char *in_txn_str    = meta->ref->in_txn ? "true" : "false";

  switch (op->type) {
    case NSS_BEGIN_TXN:
      print_json (
          "action",
          "\"begin_txn\"",
          "completed",
          completed_str,
          "step",
          step_buf,
          "seed",
          seed_buf,
          "commit_hash",
          commit_hash_buf,
          "sequence_id",
          sequence_id_buf,
          "dbname",
          dbname_buf,
          "max_insert_len",
          max_insert_len_buf,
          "sample_space_prob",
          sample_space_prob_buf,
          "in_txn",
          in_txn_str,
          "op_duration_ms",
          op_duration_ms_buf,
          "total_working_ms",
          total_working_ms_buf,
          "db_size_gb",
          db_size_gb_buf,
          "elapsed_ms",
          elapsed_ms_buf,
          "ref_nvars",
          ref_nvars_buf,
          "ref_tracked_gb",
          ref_tracked_gb_buf,
          "cur_var",
          cur_var_buf,
          "cur_var_len",
          cur_var_len_buf,
          "cur_var_tsize",
          cur_var_tsize_buf,
          NULL
      );
      break;

    case NSS_COMMIT_TXN:
      print_json (
          "action",
          "\"commit_txn\"",
          "completed",
          completed_str,
          "step",
          step_buf,
          "seed",
          seed_buf,
          "commit_hash",
          commit_hash_buf,
          "sequence_id",
          sequence_id_buf,
          "dbname",
          dbname_buf,
          "max_insert_len",
          max_insert_len_buf,
          "sample_space_prob",
          sample_space_prob_buf,
          "in_txn",
          in_txn_str,
          "op_duration_ms",
          op_duration_ms_buf,
          "total_working_ms",
          total_working_ms_buf,
          "db_size_gb",
          db_size_gb_buf,
          "elapsed_ms",
          elapsed_ms_buf,
          "ref_nvars",
          ref_nvars_buf,
          "ref_tracked_gb",
          ref_tracked_gb_buf,
          "cur_var",
          cur_var_buf,
          "cur_var_len",
          cur_var_len_buf,
          "cur_var_tsize",
          cur_var_tsize_buf,
          NULL
      );
      break;

    case NSS_ROLLBACK_TXN:
      print_json (
          "action",
          "\"rollback_txn\"",
          "completed",
          completed_str,
          "step",
          step_buf,
          "seed",
          seed_buf,
          "commit_hash",
          commit_hash_buf,
          "sequence_id",
          sequence_id_buf,
          "dbname",
          dbname_buf,
          "max_insert_len",
          max_insert_len_buf,
          "sample_space_prob",
          sample_space_prob_buf,
          "in_txn",
          in_txn_str,
          "op_duration_ms",
          op_duration_ms_buf,
          "total_working_ms",
          total_working_ms_buf,
          "db_size_gb",
          db_size_gb_buf,
          "elapsed_ms",
          elapsed_ms_buf,
          "ref_nvars",
          ref_nvars_buf,
          "ref_tracked_gb",
          ref_tracked_gb_buf,
          "cur_var",
          cur_var_buf,
          "cur_var_len",
          cur_var_len_buf,
          "cur_var_tsize",
          cur_var_tsize_buf,
          NULL
      );
      break;

    case NSS_CRASH_AND_REOPEN:
      print_json (
          "action",
          "\"crash_and_reopen\"",
          "completed",
          completed_str,
          "step",
          step_buf,
          "seed",
          seed_buf,
          "commit_hash",
          commit_hash_buf,
          "sequence_id",
          sequence_id_buf,
          "dbname",
          dbname_buf,
          "max_insert_len",
          max_insert_len_buf,
          "sample_space_prob",
          sample_space_prob_buf,
          "in_txn",
          in_txn_str,
          "op_duration_ms",
          op_duration_ms_buf,
          "total_working_ms",
          total_working_ms_buf,
          "db_size_gb",
          db_size_gb_buf,
          "elapsed_ms",
          elapsed_ms_buf,
          "ref_nvars",
          ref_nvars_buf,
          "ref_tracked_gb",
          ref_tracked_gb_buf,
          "cur_var",
          cur_var_buf,
          "cur_var_len",
          cur_var_len_buf,
          "cur_var_tsize",
          cur_var_tsize_buf,
          NULL
      );
      break;

    case NSS_CLOSE_AND_REOPEN:
      print_json (
          "action",
          "\"close_and_reopen\"",
          "completed",
          completed_str,
          "step",
          step_buf,
          "seed",
          seed_buf,
          "commit_hash",
          commit_hash_buf,
          "sequence_id",
          sequence_id_buf,
          "dbname",
          dbname_buf,
          "max_insert_len",
          max_insert_len_buf,
          "sample_space_prob",
          sample_space_prob_buf,
          "in_txn",
          in_txn_str,
          "op_duration_ms",
          op_duration_ms_buf,
          "total_working_ms",
          total_working_ms_buf,
          "db_size_gb",
          db_size_gb_buf,
          "elapsed_ms",
          elapsed_ms_buf,
          "ref_nvars",
          ref_nvars_buf,
          "ref_tracked_gb",
          ref_tracked_gb_buf,
          "cur_var",
          cur_var_buf,
          "cur_var_len",
          cur_var_len_buf,
          "cur_var_tsize",
          cur_var_tsize_buf,
          NULL
      );
      break;

    case NSS_CREATE_AND_SWAP_IF_EMPTY: {
      char vname_buf[256];
      char type_buf[256];
      snprintf (vname_buf, sizeof (vname_buf), "\"%s\"", op->op_create.vname);
      snprintf (type_buf, sizeof (type_buf), "\"%s\"", op->op_create.typestr);
      print_json (
          "action",
          "\"create\"",
          "completed",
          completed_str,
          "step",
          step_buf,
          "seed",
          seed_buf,
          "commit_hash",
          commit_hash_buf,
          "sequence_id",
          sequence_id_buf,
          "dbname",
          dbname_buf,
          "max_insert_len",
          max_insert_len_buf,
          "sample_space_prob",
          sample_space_prob_buf,
          "in_txn",
          in_txn_str,
          "op_duration_ms",
          op_duration_ms_buf,
          "total_working_ms",
          total_working_ms_buf,
          "db_size_gb",
          db_size_gb_buf,
          "elapsed_ms",
          elapsed_ms_buf,
          "ref_nvars",
          ref_nvars_buf,
          "ref_tracked_gb",
          ref_tracked_gb_buf,
          "cur_var",
          cur_var_buf,
          "cur_var_len",
          cur_var_len_buf,
          "cur_var_tsize",
          cur_var_tsize_buf,
          "vname",
          vname_buf,
          "type",
          type_buf,
          NULL
      );
      break;
    }

    case NSS_SWITCH: {
      char vname_buf[256];
      snprintf (vname_buf, sizeof (vname_buf), "\"%s\"", op->op_switch.vname);
      print_json (
          "action",
          "\"switch\"",
          "completed",
          completed_str,
          "step",
          step_buf,
          "seed",
          seed_buf,
          "commit_hash",
          commit_hash_buf,
          "sequence_id",
          sequence_id_buf,
          "dbname",
          dbname_buf,
          "max_insert_len",
          max_insert_len_buf,
          "sample_space_prob",
          sample_space_prob_buf,
          "in_txn",
          in_txn_str,
          "op_duration_ms",
          op_duration_ms_buf,
          "total_working_ms",
          total_working_ms_buf,
          "db_size_gb",
          db_size_gb_buf,
          "elapsed_ms",
          elapsed_ms_buf,
          "ref_nvars",
          ref_nvars_buf,
          "ref_tracked_gb",
          ref_tracked_gb_buf,
          "cur_var",
          cur_var_buf,
          "cur_var_len",
          cur_var_len_buf,
          "cur_var_tsize",
          cur_var_tsize_buf,
          "vname",
          vname_buf,
          NULL
      );
      break;
    }

    case NSS_DELETE_CURRENT_VARIABLE_AND_SWITCH: {
      char        next_buf[256];
      const char *next_val;
      if (op->op_delete.next) {
        snprintf (next_buf, sizeof (next_buf), "\"%s\"", op->op_delete.next);
        next_val = next_buf;
      } else {
        next_val = "null";
      }
      print_json (
          "action",
          "\"delete\"",
          "completed",
          completed_str,
          "step",
          step_buf,
          "seed",
          seed_buf,
          "commit_hash",
          commit_hash_buf,
          "sequence_id",
          sequence_id_buf,
          "dbname",
          dbname_buf,
          "max_insert_len",
          max_insert_len_buf,
          "sample_space_prob",
          sample_space_prob_buf,
          "in_txn",
          in_txn_str,
          "op_duration_ms",
          op_duration_ms_buf,
          "total_working_ms",
          total_working_ms_buf,
          "db_size_gb",
          db_size_gb_buf,
          "elapsed_ms",
          elapsed_ms_buf,
          "ref_nvars",
          ref_nvars_buf,
          "ref_tracked_gb",
          ref_tracked_gb_buf,
          "cur_var",
          cur_var_buf,
          "cur_var_len",
          cur_var_len_buf,
          "cur_var_tsize",
          cur_var_tsize_buf,
          "next",
          next_val,
          NULL
      );
      break;
    }

    case NSS_INSERT: {
      char ofst_buf[32];
      char nelems_buf[32];
      snprintf (ofst_buf, sizeof (ofst_buf), "%" PRb_size, op->op_insert.ofst);
      snprintf (nelems_buf, sizeof (nelems_buf), "%" PRb_size, op->op_insert.nelems);
      print_json (
          "action",
          "\"insert\"",
          "completed",
          completed_str,
          "step",
          step_buf,
          "seed",
          seed_buf,
          "commit_hash",
          commit_hash_buf,
          "sequence_id",
          sequence_id_buf,
          "dbname",
          dbname_buf,
          "max_insert_len",
          max_insert_len_buf,
          "sample_space_prob",
          sample_space_prob_buf,
          "in_txn",
          in_txn_str,
          "op_duration_ms",
          op_duration_ms_buf,
          "total_working_ms",
          total_working_ms_buf,
          "db_size_gb",
          db_size_gb_buf,
          "elapsed_ms",
          elapsed_ms_buf,
          "ref_nvars",
          ref_nvars_buf,
          "ref_tracked_gb",
          ref_tracked_gb_buf,
          "cur_var",
          cur_var_buf,
          "cur_var_len",
          cur_var_len_buf,
          "cur_var_tsize",
          cur_var_tsize_buf,
          "ofst",
          ofst_buf,
          "nelems",
          nelems_buf,
          "bytes_moved",
          bytes_moved_buf,
          "bytes_per_ms",
          bytes_per_ms_buf,
          "total_bytes_moved",
          total_bytes_moved_buf,
          "avg_bytes_per_ms",
          avg_bytes_per_ms_buf,
          NULL
      );
      break;
    }

    case NSS_REMOVE: {
      char start_buf[32];
      char stride_buf[32];
      char nelems_buf[32];
      snprintf (start_buf, sizeof (start_buf), "%" PRb_size, op->op_remove.start);
      snprintf (stride_buf, sizeof (stride_buf), "%" PRb_size, op->op_remove.stride);
      snprintf (nelems_buf, sizeof (nelems_buf), "%" PRb_size, op->op_remove.nelems);
      print_json (
          "action",
          "\"remove\"",
          "completed",
          completed_str,
          "step",
          step_buf,
          "seed",
          seed_buf,
          "commit_hash",
          commit_hash_buf,
          "sequence_id",
          sequence_id_buf,
          "dbname",
          dbname_buf,
          "max_insert_len",
          max_insert_len_buf,
          "sample_space_prob",
          sample_space_prob_buf,
          "in_txn",
          in_txn_str,
          "op_duration_ms",
          op_duration_ms_buf,
          "total_working_ms",
          total_working_ms_buf,
          "db_size_gb",
          db_size_gb_buf,
          "elapsed_ms",
          elapsed_ms_buf,
          "ref_nvars",
          ref_nvars_buf,
          "ref_tracked_gb",
          ref_tracked_gb_buf,
          "cur_var",
          cur_var_buf,
          "cur_var_len",
          cur_var_len_buf,
          "cur_var_tsize",
          cur_var_tsize_buf,
          "start",
          start_buf,
          "stride",
          stride_buf,
          "nelems",
          nelems_buf,
          "bytes_moved",
          bytes_moved_buf,
          "bytes_per_ms",
          bytes_per_ms_buf,
          "total_bytes_moved",
          total_bytes_moved_buf,
          "avg_bytes_per_ms",
          avg_bytes_per_ms_buf,
          NULL
      );
      break;
    }

    case NSS_READ: {
      char start_buf[32];
      char stride_buf[32];
      char nelems_buf[32];
      snprintf (start_buf, sizeof (start_buf), "%" PRb_size, op->op_read.start);
      snprintf (stride_buf, sizeof (stride_buf), "%" PRb_size, op->op_read.stride);
      snprintf (nelems_buf, sizeof (nelems_buf), "%" PRb_size, op->op_read.nelems);
      print_json (
          "action",
          "\"read\"",
          "completed",
          completed_str,
          "step",
          step_buf,
          "seed",
          seed_buf,
          "commit_hash",
          commit_hash_buf,
          "sequence_id",
          sequence_id_buf,
          "dbname",
          dbname_buf,
          "max_insert_len",
          max_insert_len_buf,
          "sample_space_prob",
          sample_space_prob_buf,
          "in_txn",
          in_txn_str,
          "op_duration_ms",
          op_duration_ms_buf,
          "total_working_ms",
          total_working_ms_buf,
          "db_size_gb",
          db_size_gb_buf,
          "elapsed_ms",
          elapsed_ms_buf,
          "ref_nvars",
          ref_nvars_buf,
          "ref_tracked_gb",
          ref_tracked_gb_buf,
          "cur_var",
          cur_var_buf,
          "cur_var_len",
          cur_var_len_buf,
          "cur_var_tsize",
          cur_var_tsize_buf,
          "start",
          start_buf,
          "stride",
          stride_buf,
          "nelems",
          nelems_buf,
          "bytes_moved",
          bytes_moved_buf,
          "bytes_per_ms",
          bytes_per_ms_buf,
          "total_bytes_moved",
          total_bytes_moved_buf,
          "avg_bytes_per_ms",
          avg_bytes_per_ms_buf,
          NULL
      );
      break;
    }

    case NSS_WRITE: {
      char start_buf[32];
      char stride_buf[32];
      char nelems_buf[32];
      snprintf (start_buf, sizeof (start_buf), "%" PRb_size, op->op_write.start);
      snprintf (stride_buf, sizeof (stride_buf), "%" PRb_size, op->op_write.stride);
      snprintf (nelems_buf, sizeof (nelems_buf), "%" PRb_size, op->op_write.nelems);
      print_json (
          "action",
          "\"write\"",
          "completed",
          completed_str,
          "step",
          step_buf,
          "seed",
          seed_buf,
          "commit_hash",
          commit_hash_buf,
          "sequence_id",
          sequence_id_buf,
          "dbname",
          dbname_buf,
          "max_insert_len",
          max_insert_len_buf,
          "sample_space_prob",
          sample_space_prob_buf,
          "in_txn",
          in_txn_str,
          "op_duration_ms",
          op_duration_ms_buf,
          "total_working_ms",
          total_working_ms_buf,
          "db_size_gb",
          db_size_gb_buf,
          "elapsed_ms",
          elapsed_ms_buf,
          "ref_nvars",
          ref_nvars_buf,
          "ref_tracked_gb",
          ref_tracked_gb_buf,
          "cur_var",
          cur_var_buf,
          "cur_var_len",
          cur_var_len_buf,
          "cur_var_tsize",
          cur_var_tsize_buf,
          "start",
          start_buf,
          "stride",
          stride_buf,
          "nelems",
          nelems_buf,
          "bytes_moved",
          bytes_moved_buf,
          "bytes_per_ms",
          bytes_per_ms_buf,
          "total_bytes_moved",
          total_bytes_moved_buf,
          "avg_bytes_per_ms",
          avg_bytes_per_ms_buf,
          NULL
      );
      break;
    }

    case NSS_NONE_AVAILABLE:
      print_json (
          "action",
          "\"none_available\"",
          "completed",
          completed_str,
          "step",
          step_buf,
          "seed",
          seed_buf,
          "commit_hash",
          commit_hash_buf,
          "sequence_id",
          sequence_id_buf,
          "dbname",
          dbname_buf,
          "max_insert_len",
          max_insert_len_buf,
          "sample_space_prob",
          sample_space_prob_buf,
          "in_txn",
          in_txn_str,
          "op_duration_ms",
          op_duration_ms_buf,
          "total_working_ms",
          total_working_ms_buf,
          "db_size_gb",
          db_size_gb_buf,
          "elapsed_ms",
          elapsed_ms_buf,
          "ref_nvars",
          ref_nvars_buf,
          "ref_tracked_gb",
          ref_tracked_gb_buf,
          "cur_var",
          cur_var_buf,
          "cur_var_len",
          cur_var_len_buf,
          "cur_var_tsize",
          cur_var_tsize_buf,
          NULL
      );
      break;

    case NSS_AT_LEN: UNREACHABLE (); break;
  }
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
  if (nsdb_cleanup (params.dbname) < 0) {
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
