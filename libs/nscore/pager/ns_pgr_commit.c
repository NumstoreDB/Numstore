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

#include "ns_crash.h"
#include "ns_error.h"
#include "pager/ns_page_fixture.h"
#include "pager/ns_pager.h"
#include "txn_table/ns_txn_table.h"

err_t
pgr_commit (struct pager *p, struct txn *tx, error *e)
{
  DBG_ASSERT (pager, p);

  if (tx->data.state != TX_RUNNING) {
    return error_causef (e, ERR_DUPLICATE_COMMIT, "txn already committed");
  }

  // COMMIT
  lsn l = wal_append_commit_log (p->ww, tx->tid, tx->data.last_lsn);

  // FLUSH
  wal_flush_all (p->ww);

  // END
  wal_append_end_log (p->ww, tx->tid, l);

  // Remove the transaction from the txn table
  txnt_remove_txn_expect (p->tnxt, tx);

  // Unlock all locks from the txn (2PL shrinking phase)
  lockt_unlock_tx (p->lt, tx);
  tx->data.state = TX_DONE;
  return error_trace (e);
}

#ifndef NDEBUG
#  include "lock_table/ns_lock_table.h"
#  include "ns_testing.h"

TEST (pgr_commit_happy_path)
{
  struct pgr_fixture f;
  test_assert_int_equal (pgr_fixture_create (&f, &ctx->e), SUCCESS);

  struct txn tx;
  test_assert_int_equal (pgr_begin_txn (&tx, f.p, &ctx->e), SUCCESS);
  test_assert_int_equal (tx.data.state, TX_RUNNING);

  test_assert_int_equal (pgr_commit (f.p, &tx, &ctx->e), SUCCESS);
  test_assert_int_equal (tx.data.state, TX_DONE);

  test_assert_int_equal (pgr_fixture_teardown (&f, &ctx->e), SUCCESS);
}

TEST (pgr_commit_txn_not_running)
{
  struct pgr_fixture f;
  test_assert_int_equal (pgr_fixture_create (&f, &ctx->e), SUCCESS);

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

#  define test_commit_refused_in_state(f, ctx, _state)                                        \
    do {                                                                                      \
      /* Setup - Begin transaction */                                                         \
      struct txn tx;                                                                          \
      test_assert_int_equal (pgr_begin_txn (&tx, (f).p, &(ctx)->e), SUCCESS);                 \
                                                                                              \
      /* Change transaction state */                                                          \
      tx.data.state = (_state);                                                               \
                                                                                              \
      /* Try to commit (shouldn't work) */                                                    \
      test_err_t_check (pgr_commit ((f).p, &tx, &(ctx)->e), ERR_DUPLICATE_COMMIT, &(ctx)->e); \
      test_assert_int_equal (tx.data.state, (_state));                                        \
                                                                                              \
      /* Put it back so the fixture can tear down cleanly */                                  \
      tx.data.state = TX_RUNNING;                                                             \
      test_assert_int_equal (pgr_commit ((f).p, &tx, &(ctx)->e), SUCCESS);                    \
    }                                                                                         \
    while (0)

  TEST_CASE ("Not running is refused")
  {
    test_commit_refused_in_state (f, ctx, TX_CANDIDATE_FOR_UNDO);
    test_commit_refused_in_state (f, ctx, TX_COMMITTED);
    test_commit_refused_in_state (f, ctx, TX_DONE);
  }

  test_assert_int_equal (pgr_fixture_teardown (&f, &ctx->e), SUCCESS);
}

TEST (pgr_commit_fail_no_wal)
{
  struct pgr_fixture f;

  int                fail_after = 0;
  err_t              ret;

  do {
    TEST_CASE ("Failure no wal capacity. Fail after: %d", fail_after)
    {
      pgr_fixture_create_with (&f, ctx->faulty_os, ctx->faulty_mem, &ctx->e);

      // An empty buffer so the commit record fits without a flush
      wal_flush_all (f.p->ww);
      test_assert_int_equal (pgr_begin_txn (&f.tx, f.p, &ctx->e), SUCCESS);

      // Do operation under error
      faulty_os_fail_after (ctx->faulty_os, fail_after);
      capture_intentional_crash (&ret, &ctx->e)
      {
        ret = pgr_commit (f.p, &f.tx, &ctx->e);
      }
      faulty_os_reset (ctx->faulty_os);

      pgr_fixture_teardown_with (&f, &ctx->e);

      fail_after++;
    }
  }
  while (ret != SUCCESS);
}

// Same as above, but with a full WAL buffer so the commit record has to flush
TEST (pgr_commit_fail_full_wal)
{
  struct pgr_fixture f;

  int                fail_after = 0;
  err_t              ret;

  do {
    TEST_CASE ("Failure with a full WAL. Fail after: %d", fail_after)
    {
      pgr_fixture_create_with (&f, ctx->faulty_os, ctx->faulty_mem, &ctx->e);

      wal_flush_all (f.p->ww);
      test_assert_int_equal (pgr_begin_txn (&f.tx, f.p, &ctx->e), SUCCESS);

      // Force the commit record to flush
      wal_ensure_flush_next (f.p->ww);

      // Do operation under error
      faulty_os_fail_after (ctx->faulty_os, fail_after);
      capture_intentional_crash (&ret, &ctx->e)
      {
        ret = pgr_commit (f.p, &f.tx, &ctx->e);
      }
      faulty_os_reset (ctx->faulty_os);

      pgr_fixture_teardown_with (&f, &ctx->e);

      fail_after++;
    }
  }
  while (ret != SUCCESS);
}
#endif
