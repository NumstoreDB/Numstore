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

#include "core/ns_alloc.h"
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
  struct ns_ref   *ref;
  struct ns_db    *db;

  // Which actions are turned on
  u8               enabled[NSS_AT_LEN];

  const char      *dbname;
  int              max_insert_len;
  float            sample_space_prob;

  // Run identity
  u64              seed;
  const char      *commit_hash;
  u64              sequence_id;

  // Run metrics
  u64              start;       // Epoch for everything below
  u64              step_number; // Which step is the test in
  u64              clock;       // Absolute timestamp of the last observation

  i_timer          timer;
  struct allocator alloc;

  struct i_mem     reliable_mem;
  struct i_mem     test_mem;

  error            e;
};

////////// ACTIONS

static int
nss_begin_txn (struct ns_simulation *meta)
{
  if (ns_ref_begin_txn (meta->ref, &meta->e)) {
    return -1;
  }
  ns_db_begin_txn (meta->db);
  return 0;
}

static int
nss_commit_txn (struct ns_simulation *meta)
{
  if (ns_ref_commit_txn (meta->ref, &meta->e)) {
    return -1;
  }
  ns_db_commit_txn (meta->db);
  return 0;
}

static int
nss_rollback_txn (struct ns_simulation *meta)
{
  ns_ref_rollback_txn (meta->ref);
  ns_db_rollback_txn (meta->db);
  return 0;
}

static void
nss_crash_and_reopen (struct ns_simulation *meta)
{
  ns_ref_crash_and_reopen (meta->ref);
  ns_db_crash_and_reopen (meta->db);
}

static void
nss_close_and_reopen (struct ns_simulation *meta)
{
  ns_ref_close_and_reopen (meta->ref);
  ns_db_close_and_reopen (meta->db);
}

static int
nss_create (struct ns_simulation *meta, struct operation *op)
{
  if (ns_ref_create (meta->ref, op->op_create.vname, op->op_create.t, &meta->e)) {
    return -1;
  }
  ns_db_create (meta->db, op->op_create.vname, op->op_create.typestr);
  return 0;
}

static void
nss_switch (struct ns_simulation *meta, struct operation *op)
{
  ns_ref_switch (meta->ref, op->op_switch.vname);
  ns_db_switch (meta->db, op->op_switch.vname);
}

static void
nss_delete (struct ns_simulation *meta, struct operation *op)
{
  ns_ref_delete (meta->ref, op->op_delete.next);
  ns_db_delete (meta->db, op->op_delete.next);
}

static int
nss_insert (struct ns_simulation *meta, struct operation *op)
{
  if (ns_ref_insert (
          meta->ref,
          op->op_insert.data,
          op->op_insert.ofst,
          op->op_insert.nelems,
          &meta->e
      )) {
    return -1;
  }
  ns_db_insert (meta->db, op->op_insert.data, op->op_insert.ofst, op->op_insert.nelems);
  return 0;
}

static void
nss_remove (struct ns_simulation *meta, struct operation *op)
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
  ns_db_remove (
      meta->db,
      op->op_remove.db_dest,
      (struct stride){
          .start  = op->op_remove.start,
          .stride = op->op_remove.stride,
          .nelems = op->op_remove.nelems,
      }
  );
}

static void
nss_read (struct ns_simulation *meta, struct operation *op)
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
  ns_db_read (
      meta->db,
      op->op_read.db_dest,
      (struct stride){
          .start  = op->op_read.start,
          .stride = op->op_read.stride,
          .nelems = op->op_read.nelems,
      }
  );
}

static void
nss_write (struct ns_simulation *meta, struct operation *op)
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
  ns_db_write (
      meta->db,
      op->op_write.data,
      (struct stride){
          .start  = op->op_write.start,
          .stride = op->op_write.stride,
          .nelems = op->op_write.nelems,
      }
  );
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
  struct operation *op = opg_random ((struct rand_op_params){}, e);
  if (op == NULL) {
    return error_trace (e);
  }

  switch (op->type) {
    case NSS_BEGIN_TXN: nss_begin_txn (meta); break;
    case NSS_COMMIT_TXN: nss_commit_txn (meta); break;
    case NSS_ROLLBACK_TXN: nss_rollback_txn (meta); break;
    case NSS_CRASH_AND_REOPEN: nss_crash_and_reopen (meta); break;
    case NSS_CLOSE_AND_REOPEN: nss_close_and_reopen (meta); break;
    case NSS_CREATE: nss_create (meta, op); break;
    case NSS_SWITCH: nss_switch (meta, op); break;
    case NSS_DELETE: nss_delete (meta, op); break;
    case NSS_INSERT: nss_insert (meta, op); break;
    case NSS_REMOVE: nss_remove (meta, op); break;
    case NSS_READ: nss_read (meta, op); break;
    case NSS_WRITE: nss_write (meta, op); break;
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
