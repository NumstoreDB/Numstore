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

#include "core/os/ns_time.h"
#include "core/testing/ns_testing.h"
#include "nscore/page/ns_page_fixture.h"
#include "nscore/pager/ns_pager.h"

static err_t
pgr_refresh_wal (struct pager *p, error *e)
{
  DBG_ASSERT (pager, p);
  wal_delete_and_reopen (p->ww, e);
  return error_trace (e);
}

static err_t
pgr_deletion_blocking_checkpoint (struct pager *p, error *e)
{
  ASSERT (p->ww);

  // i_log_debug ("Starting Checkpoint - locking the database\n");

  // This is what makes the checkpoint blocking
  // it will wait for all open transactions to complete
  lockt_lock (p->lt, lock_db (), LM_X, NULL, e);
  // i_log_debug ("Checkpoint - lock acquired\n");

  // Flush all pages - so the database is consistent
  if (pgr_flush_all_pages (p, e) < 0) {
    goto theend;
  }

  // Flush the WAL
  if (wal_flush_all (p->ww, e)) {
    goto theend;
  }

  // Get the end_lsn
  lsn end_lsn = wal_start_lsn (p->ww) + wal_size (p->ww);
  // i_log_info ("CHECKPOINT: Next start of the lsn = %" PRlsn "\n", end_lsn);

  // Write the next min lsn slot
  if (pgr_write_next_lsn (p, end_lsn, e)) {
    goto theend;
  }

  // Delete the WAL and replace it with a fresh one
  if (pgr_refresh_wal (p, e) < 0) {
    goto theend;
  }

  ASSERT (wal_isnew (p->ww));

  // Write the next start lsn for the WAL
  if (wal_write_start_lsn (p->ww, end_lsn, e)) {
    goto theend;
  }

theend:

  // Unlock the database lock
  // i_log_debug ("Checkpoint Done - unlocking the database\n");
  lockt_unlock (p->lt, lock_db (), LM_X, e);
  return SUCCESS;
}

static void
pgr_do_checkpoint (void *ctx)
{
  struct pager *p = ctx;
  error         e = error_create ();
  if (pgr_deletion_blocking_checkpoint (p, &e)) {
    error_log_consume (&e);
  }
}

err_t
pgr_launch_checkpoint_thread (struct pager *p, u64 msec, error *e)
{
  return periodic_task_start (&p->checkpoint_task, msec, pgr_do_checkpoint, p, e);
}

#ifdef TESTING
struct args
{
  struct pager *p;
  _Atomic u32   done;
};

static void *
producer_thread (void *_args)
{
  struct args  *args = _args;
  error         e    = error_create ();
  page_h        a    = page_h_create ();
  struct ns_txn tx;

  while (!args->done) {
    pgr_begin_txn (&tx, args->p, &e);

    pgr_new (&a, args->p, &tx, PG_DATA_LIST, &e);
    dl_make_valid (page_h_w (&a));
    pgr_release (args->p, &a, PG_DATA_LIST, &e);

    pgr_commit (args->p, &tx, &e);
  }

  return NULL;
}

TEST (pgr_checkpoint)
{
  TEST_CASE ("smoke test")
  {
    struct pgr_fixture f;
    pgr_fixture_create (&f);
    pgr_launch_checkpoint_thread (f.p, 100, &f.e);

    struct args args = {
        .done = 0,
        .p    = f.p,
    };

    i_thread producer;
    i_thread_create (default_threading (), &producer, producer_thread, &args, &f.e);
    i_sleep_ms (1000);

    args.done = 1;

    i_thread_join (default_threading (), &producer, &f.e);

    pgr_fixture_teardown (&f);
  }
}

#endif
