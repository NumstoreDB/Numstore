
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

#include "core/ns_stdtypes.h"
#include "numstore.h"

#include <time.h>

#ifndef NDEBUG
#  include "core/ns_testing.h"
#endif

#ifndef NDEBUG
// DISABLED - this test never returns. ns_crash deadlocks, and a test that
// hangs takes the whole unit test run down with it. Enable it to work on
// the bug
//
// WHAT HAPPENS:
//    A transaction that stays open across one checkpoint period (5 seconds,
//    nsdb_open) makes ns_crash hang forever:
//
//      main thread          ns_crash -> pgr_crash -> periodic_task_stop
//                           waits for the checkpoint thread to finish
//      checkpoint thread    pgr_deletion_blocking_checkpoint
//                           waits for lock_db () in LM_X, which it can't
//                           get while the transaction is open
//      the transaction      is never going to finish - we're crashing
//
// WHAT IS KNOWN:
//    - The transaction doesn't have to do anything special. One create is
//      enough, it only has to be open when the checkpoint timer fires
//    - The transaction itself is fine. After the 5 seconds it can still
//      create, insert and commit, and the commit lets the checkpoint run
//    - ns_close with an open transaction blocks too, but numstore.h says so.
//      ns_crash is the one that is supposed to work in any state
//    - It is why crash recovery of a big transaction looked like it hung:
//      a 20MB write takes longer than 5 seconds, so the checkpoint thread
//      was already waiting when the test called ns_crash
//    - The simulation can't hit it, its transactions are much shorter than
//      5 seconds
//
// WHY THE OBVIOUS FIX IS WRONG:
//    pgr_crash stops the checkpoint task and then releases the locks of
//    every transaction. Swapping those two lets the checkpoint thread take
//    the lock and run a whole checkpoint in the middle of the crash - which
//    flushes the uncommitted pages and then deletes the wal that holds
//    their undo. That is corruption, the deadlock is better
//
// WHAT A FIX NEEDS:
//    A way to cancel the checkpoint while it waits for the lock.
//    periodic_task_stop would have to wake the lock wait (gr_lock /
//    lockt_lock have no cancel today) or the checkpoint would have to try
//    the lock with a timeout and check for a stop request in between
TEST_DISABLED (0015_crash_with_txn_open_across_checkpoint)
{
  test_assert_int_equal (ns_cleanup ("test"), 0);
  nsdb_t *db = ns_open ("test");
  test_assert (db != NULL);

  txn_t *tx = ns_begin (db);
  test_assert (tx != NULL);
  test_assert_int_equal (ns_execute (db, tx, "create foo u32"), 0);

  // Stay in the transaction until the checkpoint timer fires (5 seconds)
  time_t start = time (NULL);
  while (time (NULL) - start < 7) {
  }

  // Never returns
  test_assert_int_equal (ns_crash (db), 0);

  // The transaction never committed
  db = ns_open ("test");
  test_assert (db != NULL);
  test_assert (ns_get_var (db, NULL, "get foo") == NULL);

  test_assert_int_equal (ns_close (db), 0);
}
#endif
