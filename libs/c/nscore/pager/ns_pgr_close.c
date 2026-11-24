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

#include "dpg_table/ns_dirty_page_table.h"
#include "lock_table/ns_lock_table.h"
#include "ns_error.h"
#include "ns_file_pager.h"
#include "os/ns_malloc.h"
#include "pager/ns_page_fixture.h"
#include "pager/ns_pager.h"
#include "txn_table/ns_txn_table.h"
#include "wal/ns_wal.h"

#ifndef NDEBUG
#  include "ns_testing.h"
#endif

#include <limits.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

err_t
pgr_close (struct pager *p, error *e)
{
  DBG_ASSERT (pager, p);
  CHECK_UNHANDLED_ERROR (e);

  i_log_debug ("Closing Database - waiting for checkpoint task to complete\n");

  // Close carries on through every failure below and frees the pager anyway.
  // Each failure is recorded in [e] and reported by the return at the end.
  // TODO: that is wrong for the lock and for eviction - see the disabled
  // pgr_close_fail_os and pgr_close_fail_os_after_lock tests below.
  (void)periodic_task_stop (&p->checkpoint_task, e);

  i_log_debug ("Checkpoint task complete\n");

  i_log_lockt (LOG_INFO, p->lt);
  (void)lockt_lock (p->lt, lock_db (), LM_X, NULL, e);

  // Similar to a checkpoint
  {
    // Evict all pages - so the database is consistent
    i_log_debug ("Evicting pages\n");
    (void)pgr_evict_all_pages (p, e);

    i_log_debug ("Flushing WAL\n");
    wal_flush_all (p->ww);

    lsn end_lsn = wal_start_lsn (p->ww) + wal_size (p->ww);
    i_log_info ("Writing next WAL next start_lsn = %" PRlsn " to the database\n", end_lsn);

    (void)pgr_write_next_lsn (p, end_lsn, e);

    (void)wal_close_and_delete (p->ww, e);
    i_log_debug ("WAL is deleted\n");
  }

  (void)fpgr_close (p->fp, e);
  i_log_lockt (LOG_INFO, p->lt);
  lockt_unlock (p->lt, lock_db (), LM_X);
  lockt_free (p->lt, p->mem);

  txnt_close (p->tnxt);
  dpgt_close (p->dpt);

  p->mem.table->free (p->mem.self, p);

  return error_trace (e);
}

#ifndef NDEBUG
TEST (pgr_close_success)
{
  test_assert_int_equal (pgr_delete_single_file ("testdb", ctx->os, &ctx->e), SUCCESS);

  struct pager *p = pgr_open ("testdb", ctx->mem, ctx->os, &ctx->e);
  test_assert (p != NULL);
  test_assert_int_equal (pgr_close (p, &ctx->e), SUCCESS);
  test_assert_int_equal (pgr_delete_single_file ("testdb", ctx->os, &ctx->e), SUCCESS);
}
#endif

#ifndef NDEBUG
#  include "ns_crash.h"
#  include "ns_page_data_list.h"

// TODO: pgr_close ignores the result of
// lockt_lock (p->lt, lock_db (), LM_X, NULL, e). Creating the lock frame
// makes an OS call (the gr_lock mutex), so with Fail after 0 the lock is
// never taken, close carries on anyway, and lockt_unlock asserts
// ASSERT (found) in lockt_unlock_once.
//
// Fix: check the lockt_lock result (and decide what a close that can't lock
// should do). pgr_close_fail_os_after_lock sweeps the rest of close.
TEST_DISABLED (pgr_close_fail_os)
{
  err_t ret;
  int   fail_after = 0;

  do {
    TEST_CASE ("Fail after: %d", fail_after)
    {
      struct pgr_fixture f;
      test_err_t_check (
          pgr_fixture_create_with (&f, ctx->faulty_os, ctx->mem, &ctx->e),
          SUCCESS,
          &ctx->e
      );

      // Leave a dirty page and unflushed log for close to write out
      page_h h = page_h_create ();
      pgr_begin_txn (&f.tx, f.p, &ctx->e);
      pgr_new (&h, f.p, &f.tx, PG_DATA_LIST, &ctx->e);
      dl_make_valid (page_h_w (&h));
      const pgno pg = page_h_pgno (&h);
      pgr_release (f.p, &h, PG_DATA_LIST, &ctx->e);
      pgr_commit (f.p, &f.tx, &ctx->e);
      test_assert_int_equal (ctx->e.cause_code, SUCCESS);

      faulty_os_fail_after (ctx->faulty_os, fail_after);
      capture_intentional_crash (&ret, &ctx->e)
      {
        ret = pgr_close (f.p, &ctx->e);
      }
      faulty_os_reset (ctx->faulty_os);

      // pgr_close frees the pager whether or not it reported an error
      if (ret == ERR_CRASHED) {
        error_reset (&ctx->e);
        pgr_crash (f.p, &ctx->e);
      }
      error_reset (&ctx->e);
      arena_alloc_free_all (&f.alloc);

      // Whatever happened to close, the committed page has to survive a reopen
      struct pager *p = pgr_open ("testdb", ctx->mem, ctx->os, &ctx->e);
      test_assert (p != NULL);
      test_err_t_check (pgr_get (&h, PG_DATA_LIST, pg, p, &ctx->e), SUCCESS, &ctx->e);
      test_err_t_check (pgr_release (p, &h, PG_DATA_LIST, &ctx->e), SUCCESS, &ctx->e);
      test_err_t_check (pgr_close (p, &ctx->e), SUCCESS, &ctx->e);

      fail_after++;
    }
  }
  while (ret != SUCCESS);

  test_err_t_check (pgr_delete_single_file ("testdb", ctx->os, &ctx->e), SUCCESS, &ctx->e);
}

// Same sweep, skipping the lock frame's mutex_create (see pgr_close_fail_os).
//
// TODO: loses committed data. When a page write fails during
// pgr_evict_all_pages (Fail after 4: fpgr_write -> pwrite), pgr_close keeps
// going: it writes the next lsn to the header and wal_close_and_delete
// removes the WAL. The page never reached the file and its log is gone, so
// on reopen the committed page reads back as an UNKNOWN page.
//
// Fix: once eviction or the WAL flush fails, close must not advance the
// next lsn or delete the WAL - leave it for recovery, like a crash.
TEST_DISABLED (pgr_close_fail_os_after_lock)
{
  err_t ret;
  int   fail_after = 1;

  do {
    TEST_CASE ("Fail after: %d", fail_after)
    {
      struct pgr_fixture f;
      test_err_t_check (
          pgr_fixture_create_with (&f, ctx->faulty_os, ctx->mem, &ctx->e),
          SUCCESS,
          &ctx->e
      );

      // Leave a dirty page and unflushed log for close to write out
      page_h h = page_h_create ();
      pgr_begin_txn (&f.tx, f.p, &ctx->e);
      pgr_new (&h, f.p, &f.tx, PG_DATA_LIST, &ctx->e);
      dl_make_valid (page_h_w (&h));
      const pgno pg = page_h_pgno (&h);
      pgr_release (f.p, &h, PG_DATA_LIST, &ctx->e);
      pgr_commit (f.p, &f.tx, &ctx->e);
      test_assert_int_equal (ctx->e.cause_code, SUCCESS);

      faulty_os_fail_after (ctx->faulty_os, fail_after);
      capture_intentional_crash (&ret, &ctx->e)
      {
        ret = pgr_close (f.p, &ctx->e);
      }
      faulty_os_reset (ctx->faulty_os);

      // pgr_close frees the pager whether or not it reported an error
      if (ret == ERR_CRASHED) {
        error_reset (&ctx->e);
        pgr_crash (f.p, &ctx->e);
      }
      error_reset (&ctx->e);
      arena_alloc_free_all (&f.alloc);

      // Whatever happened to close, the committed page has to survive a reopen
      struct pager *p = pgr_open ("testdb", ctx->mem, ctx->os, &ctx->e);
      test_assert (p != NULL);
      test_err_t_check (pgr_get (&h, PG_DATA_LIST, pg, p, &ctx->e), SUCCESS, &ctx->e);
      test_err_t_check (pgr_release (p, &h, PG_DATA_LIST, &ctx->e), SUCCESS, &ctx->e);
      test_err_t_check (pgr_close (p, &ctx->e), SUCCESS, &ctx->e);

      fail_after++;
    }
  }
  while (ret != SUCCESS);

  test_err_t_check (pgr_delete_single_file ("testdb", ctx->os, &ctx->e), SUCCESS, &ctx->e);
}
#endif
