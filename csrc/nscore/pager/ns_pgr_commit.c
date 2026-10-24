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

#include "core/ns_error.h"
#include "nscore/page/ns_page_fixture.h"
#include "nscore/pager/ns_pager.h"

err_t
pgr_commit (struct pager *p, struct txn *tx, error *e)
{
  DBG_ASSERT (pager, p);

  if (tx->data.state != TX_RUNNING) {
    error_causef (e, ERR_DUPLICATE_COMMIT, "txn already committed");
    goto theend;
  }

  // COMMIT
  slsn l = wal_append_commit_log (p->ww, tx->tid, tx->data.last_lsn, e);
  if (l < 0) {
    goto theend;
  }

  // FLUSH
  if (wal_flush_all (p->ww, e)) {
    // We have a commit log appended to the WAL.
    // It may or may not be written to disk.
    // If it is written to disk, then good, next recovery,
    // we batch it in the list of commits to append an end
    // to

    goto theend;
  }

  // END
  l = wal_append_end_log (p->ww, tx->tid, l, e);
  if (l < 0) {
    // Failing to append an end log isn't a big deal,
    // we'll append it during the next pgr_recovery

    goto theend;
  }

  // Remove the transaction from the txn table
  txnt_remove_txn_expect (p->tnxt, tx);

  // Unlock all locks from the txn (2PL shrinking phase)
  lockt_unlock_tx (p->lt, tx);

  tx->data.state = TX_DONE;

theend:
  return error_trace (e);
}

#ifndef NDEBUG
#  include "core/ns_testing.h"

/******************************************************************************
 * SECTION: pgr_commit (tests)
 ******************************************************************************/

TEST (pgr_commit_happy_path)
{
  struct pgr_fixture f;
  test_fail_if (pgr_fixture_create (&f, &ctx->e));

  struct txn tx;
  test_assert_int_equal (pgr_begin_txn (&tx, f.p, &ctx->e), SUCCESS);
  test_assert_int_equal (tx.data.state, TX_RUNNING);

  test_assert_int_equal (pgr_commit (f.p, &tx, &ctx->e), SUCCESS);
  test_assert_int_equal (tx.data.state, TX_DONE);

  test_fail_if (pgr_fixture_teardown (&f, &ctx->e));
}

// The guard at the top of pgr_commit: anything but TX_RUNNING is a duplicate
// commit, and the transaction must be left exactly as it was found
TEST (pgr_commit_txn_not_running)
{
  struct pgr_fixture f;
  test_fail_if (pgr_fixture_create (&f, &ctx->e));

  TEST_CASE ("Committing a second time is ERR_DUPLICATE_COMMIT")
  {
    struct txn tx;
    test_assert_int_equal (pgr_begin_txn (&tx, f.p, &ctx->e), SUCCESS);
    test_assert_int_equal (pgr_commit (f.p, &tx, &ctx->e), SUCCESS);
    test_assert_int_equal (tx.data.state, TX_DONE);

    test_err_t_check (pgr_commit (f.p, &tx, &ctx->e), ERR_DUPLICATE_COMMIT, &ctx->e);

    // Still done, not re-committed
    test_assert_int_equal (tx.data.state, TX_DONE);
  }

  TEST_CASE ("A third attempt is refused the same way")
  {
    struct txn tx;
    test_assert_int_equal (pgr_begin_txn (&tx, f.p, &ctx->e), SUCCESS);
    test_assert_int_equal (pgr_commit (f.p, &tx, &ctx->e), SUCCESS);

    test_err_t_check (pgr_commit (f.p, &tx, &ctx->e), ERR_DUPLICATE_COMMIT, &ctx->e);
    test_err_t_check (pgr_commit (f.p, &tx, &ctx->e), ERR_DUPLICATE_COMMIT, &ctx->e);
  }

  // Every state that is not TX_RUNNING must be refused, not just TX_DONE
  const enum tx_state not_running[] = {
      TX_CANDIDATE_FOR_UNDO,
      TX_COMMITTED,
      TX_DONE,
  };

  for (u32 i = 0; i < arrlen (not_running); ++i) {
    TEST_CASE ("state %d is refused", (int)not_running[i])
    {
      struct txn tx;
      test_assert_int_equal (pgr_begin_txn (&tx, f.p, &ctx->e), SUCCESS);

      // Drive the state directly: these are states the commit guard has to
      // reject, and reaching them through real work would need a crash or a
      // recovery pass
      tx.data.state = not_running[i];

      test_err_t_check (pgr_commit (f.p, &tx, &ctx->e), ERR_DUPLICATE_COMMIT, &ctx->e);
      test_assert_int_equal (tx.data.state, not_running[i]);

      // Put it back so the fixture can tear down cleanly
      tx.data.state = TX_RUNNING;
      test_assert_int_equal (pgr_commit (f.p, &tx, &ctx->e), SUCCESS);
    }
  }

  test_fail_if (pgr_fixture_teardown (&f, &ctx->e));
}
#endif
