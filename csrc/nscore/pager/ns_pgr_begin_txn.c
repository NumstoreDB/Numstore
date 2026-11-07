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

#include "core/ns_crash.h"
#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "core/ns_testing.h"
#include "core/os/ns_os.h"
#include "nscore/lock_table/ns_lock_table.h"
#include "nscore/page/ns_page_fixture.h"
#include "nscore/pager/ns_pager.h"
#include "nscore/wal/ns_wal.h"

err_t
pgr_begin_txn (struct txn *tx, struct pager *p, error *e)
{
  DBG_ASSERT (pager, p);
  slsn l   = 0;

  // Get the next transaction id
  txid tid = atomic_fetch_add (&p->next_tid, 1);

  // Initialize transaction object with empty data
  txn_init (
      tx,
      tid,
      (struct txn_data){
          .min_lsn       = 0,
          .last_lsn      = 0,
          .undo_next_lsn = 0,
          .state         = TX_RUNNING,
      },
      p->mem
  );

  // Lock the database before doing any changes to the log or page table
  if (lockt_lock (p->lt, lock_db (), LM_X, tx, e)) {
    // Failing here is fine - just couldn't acquire a lock
    return error_trace (e);
  }

  // Append a begin log record
  l = wal_append_begin_log (p->ww, tid);

  if (l < 0) {
    lockt_unlock (p->lt, lock_db (), LM_X);
    // WAL append failed - we just wasted a transaction id - not a big deal
    return error_trace (e);
  }

  // Update transaction meta data
  txn_update_data (
      tx,
      (struct txn_data){
          .min_lsn       = l,
          .last_lsn      = l,
          .undo_next_lsn = 0,
          .state         = TX_RUNNING,
      }
  );

  // Insert txn into the txn table
  txnt_insert_txn (p->tnxt, tx);

  return SUCCESS;
}

#ifndef NDEBUG
TEST (pgr_begin_txn_fail_no_wal)
{
  // Set up with faulty os
  struct pgr_fixture f;
  pgr_fixture_create_with (&f, ctx->faulty_os, ctx->faulty_mem, &ctx->e);

  struct txn tx;
  int        fail_after = 0;
  err_t      ret;

  do {
    TEST_CASE ("Fail after: %d", fail_after)
    {
      error_reset (&ctx->e);

      // Ensure the WAL is flushed (This does assume WAL buffer
      // size is > sizeof(BEGIN))
      // That's pretty much a given
      wal_flush_all (f.p->ww);

      // Do operation under error
      faulty_os_fail_after (ctx->faulty_os, fail_after);
      ret = pgr_begin_txn (&tx, f.p, &ctx->e);
      faulty_os_reset (ctx->faulty_os);

      fail_after++;
    }
  }
  while (ret != SUCCESS);

  // Done - commit and clean up
  pgr_commit (f.p, &tx, &ctx->e);
  pgr_fixture_teardown_with (&f, &ctx->e);
}

//  Same as above, but fills up wal before
//  begin txn - to ensure a wal is triggered
TEST (pgr_begin_txn_fail_full_wal)
{
  // Set up with faulty os
  struct pgr_fixture f;

  int                fail_after = 0;
  err_t              ret;

  do {
    TEST_CASE ("Fail after: %d", fail_after)
    {
      pgr_fixture_create_with (&f, ctx->faulty_os, ctx->faulty_mem, &ctx->e);

      // Ensure the WAL will trigger on next begin for begin record
      // (remember that the WAL doesn't need to flush on a begin txn)
      wal_ensure_flush_next (f.p->ww);

      // Do operation under error
      faulty_os_fail_after (ctx->faulty_os, fail_after);
      capture_intentional_crash (&ret, &ctx->e)
      {
        ret = pgr_begin_txn (&f.tx, f.p, &ctx->e);
      }
      faulty_os_reset (ctx->faulty_os);

      // Commit if success to avoid locking
      if (ret == SUCCESS) {
        pgr_commit (f.p, &f.tx, &ctx->e);
      }

      // The crash hit after the txn recorded its db lock but before it joined
      // the txn table, so pgr_crash never sees it - free its lock records here
      if (ret == ERR_CRASHED) {
        txn_close (&f.tx);
      }

      // Teardown (handles crash case)
      pgr_fixture_teardown_with (&f, &ctx->e);

      fail_after++;
    }
  }
  while (ret != SUCCESS);
}
#endif
