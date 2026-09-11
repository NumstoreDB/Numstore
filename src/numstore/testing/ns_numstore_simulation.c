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
#include "core/ns_numerics.h"
#include "core/os/ns_filesystem.h"
#include "core/os/ns_memory.h"
#include "core/os/ns_time.h"
#include "nscore/nsdb/ns_nsdb.h"
#include "nscore/types/ns_types.h"
#include "numstore/numstore.h"
#include "numstore/testing/ns_actual_db_stepper.h"
#include "numstore/testing/ns_operation_generator.h"
#include "numstore/testing/ns_reference_db_stepper.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

struct ns_simulation
{
  struct ns_ref     *ref;
  struct ns_db      *db;

  // Which actions are turned on
  u8                 enabled[NSS_AT_LEN];

  const char        *dbname;
  int                max_insert_len;
  float              sample_space_prob;

  // Run identity
  u64                seed;
  const char        *commit_hash;
  u64                sequence_id;

  // Run metrics
  u64                start;       // Epoch for everything below
  u64                step_number; // Which step is the test in
  u64                clock;       // Absolute timestamp of the last observation

  i_timer            timer;
  struct arena_alloc alloc;

  struct i_mem       reliable_mem;
  struct i_mem       test_mem;
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
  WRAP (ns_db_create_and_maybe_switch (meta->db, op->op_create.vname, op->op_create.typestr, e));
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
      default_filesystem (),
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

      .reliable_mem      = params.reliable_mem,
      .test_mem          = params.test_mem,
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
  ns_db_close (meta->db);
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

  switch (op->type) {
    case NSS_BEGIN_TXN: WRAP (nss_begin_txn (meta, e)); break;
    case NSS_COMMIT_TXN: WRAP (nss_commit_txn (meta, e)); break;
    case NSS_ROLLBACK_TXN: WRAP (nss_rollback_txn (meta, e)); break;
    case NSS_CRASH_AND_REOPEN: WRAP (nss_crash_and_reopen (meta, e)); break;
    case NSS_CLOSE_AND_REOPEN: WRAP (nss_close_and_reopen (meta, e)); break;
    case NSS_CREATE_AND_SWAP_IF_EMPTY: WRAP (nss_create (meta, op, e)); break;
    case NSS_SWITCH: WRAP (nss_switch (meta, op, e)); break;
    case NSS_DELETE_CURRENT_VARIABLE_AND_SWITCH: WRAP (nss_delete (meta, op, e)); break;
    case NSS_INSERT: WRAP (nss_insert (meta, op, e)); break;
    case NSS_REMOVE: WRAP (nss_remove (meta, op, e)); break;
    case NSS_READ: WRAP (nss_read (meta, op, e)); break;
    case NSS_WRITE: WRAP (nss_write (meta, op, e)); break;
    case NSS_NONE_AVAILABLE: break;
    default: UNREACHABLE (); return -1;
  }

  // Increment step
  meta->step_number++;

  // Choose a set of randomized actions
  if (op->type == NSS_NONE_AVAILABLE || randf () <= meta->sample_space_prob) {
    opg_spin_enabled (meta->enabled);
  }

  opg_free (op);

  return error_trace (e);
}
