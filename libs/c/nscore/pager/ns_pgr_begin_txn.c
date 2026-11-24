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

#include "lock_table/ns_lock_table.h"
#include "ns_crash.h"
#include "ns_error.h"
#include "ns_stdtypes.h"
#include "ns_testing.h"
#include "os/ns_os.h"
#include "pager/ns_page_fixture.h"
#include "pager/ns_pager.h"
#include "wal/ns_wal.h"

err_t
pgr_begin_txn (struct txn *tx, struct pager *p, error *e)
{
  DBG_ASSERT (pager, p);
  slsn l   = 0;

  txid tid = atomic_fetch_add (&p->next_tid, 1);

  txn_init (
      tx,
      tid,
      (struct txn_data){
          .state         = TX_RUNNING,
          .min_lsn       = 0,
          .last_lsn      = 0,
          .undo_next_lsn = 0,
      },
      p->mem
  );

#ifndef EXPERIMENTAL_HIERARCHIAL_TXNS
  if (lockt_lock (p->lt, lock_db (), LM_X, tx, e) < 0) {
    // Failing here is fine - just couldn't acquire a lock
    return error_trace (e);
  }
#endif

  l = (slsn)wal_append_begin_log (p->ww, tid);

  if (l < 0) {
    lockt_unlock (p->lt, lock_db (), LM_X);
    // WAL append failed - we just wasted a transaction id - not a big deal
    return error_trace (e);
  }

  txn_update_data (
      tx,
      (struct txn_data){
          .state         = TX_RUNNING,
          .min_lsn       = (lsn)l,
          .last_lsn      = (lsn)l,
          .undo_next_lsn = 0,
      }
  );

  txnt_insert_txn (p->tnxt, tx);

  return SUCCESS;
}

#ifndef NDEBUG
TEST (pgr_begin_txn_fail_no_wal)
{
  struct pgr_fixture f;
  pgr_fixture_create_with (&f, ctx->faulty_os, ctx->faulty_mem, &ctx->e);

  int   fail_after = 0;
  err_t ret;

  do {
    TEST_CASE ("Fail after: %d", fail_after)
    {
      error_reset (&ctx->e);

      // An empty buffer so the begin record fits without a flush
      wal_flush_all (f.p->ww);

      // Do operation under error
      faulty_os_fail_after (ctx->faulty_os, fail_after);
      ret = pgr_begin_txn (&f.tx, f.p, &ctx->e);
      faulty_os_reset (ctx->faulty_os);

      fail_after++;
    }
  }
  while (ret != SUCCESS);

  // Done - commit and clean up
  pgr_commit (f.p, &f.tx, &ctx->e);
  pgr_fixture_teardown_with (&f, &ctx->e);
}

// Same as above, but with a full WAL buffer so the begin record has to flush
TEST (pgr_begin_txn_fail_full_wal)
{
  struct pgr_fixture f;

  int                fail_after = 0;
  err_t              ret;

  do {
    TEST_CASE ("Fail after: %d", fail_after)
    {
      pgr_fixture_create_with (&f, ctx->faulty_os, ctx->faulty_mem, &ctx->e);

      // Force the begin record to flush
      wal_ensure_flush_next (f.p->ww);

      // Do operation under error
      faulty_os_fail_after (ctx->faulty_os, fail_after);
      capture_intentional_crash (&ret, &ctx->e)
      {
        ret = pgr_begin_txn (&f.tx, f.p, &ctx->e);
      }
      faulty_os_reset (ctx->faulty_os);

      // Commit on success to release the database lock
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
