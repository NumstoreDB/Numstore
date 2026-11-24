
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

#include "ns_stdtypes.h"
#include "numstore.h"

#include <time.h>

#ifndef NDEBUG
#  include "ns_testing.h"
#endif

#ifndef NDEBUG
// FIXED
//
// WHAT HAPPENED:
//    A transaction that stays open across one checkpoint period (5 seconds,
//    nsdb_open) made ns_crash hang forever:
//
//      main thread          ns_crash -> pgr_crash -> periodic_task_stop
//                           waits for the checkpoint thread to finish
//      checkpoint thread    pgr_deletion_blocking_checkpoint
//                           waits for lock_db () in LM_X, which it can't
//                           get while the transaction is open
//      the transaction      is never going to finish - we're crashing
//
//    The transaction doesn't have to do anything special. One create is
//    enough, it only has to be open when the checkpoint timer fires. A big
//    write does it too: 20MB takes longer than 5 seconds
//
// WHY SWAPPING THE TWO STEPS ALONE IS WRONG:
//    pgr_crash stopped the checkpoint task and then released the locks of
//    every transaction. Releasing first lets the checkpoint thread take the
//    lock and run a whole checkpoint in the middle of the crash - which
//    flushes the uncommitted pages and then deletes the wal that holds
//    their undo. That is corruption, the deadlock was better
//
// FIX:
//    pgr_crash sets PGR_CRASHING, then releases the locks, then stops the
//    task. The checkpoint looks at the flag once it has the lock and gives
//    the lock straight back without doing anything
TEST (0015_crash_with_txn_open_across_checkpoint)
{
  test_assert_int_equal (ns_cleanup ("test"), 0);
  nsdb_t *db = ns_open ("test");
  test_assert (db != NULL);

  nsdb_txn_t *tx = ns_begin (db);
  test_assert (tx != NULL);
  test_assert_int_equal (ns_execute (db, tx, "create foo u32"), 0);

  // Stay in the transaction until the checkpoint timer fires (5 seconds)
  time_t start = time (NULL);
  while (time (NULL) - start < 7) {
  }

  // Used to never return
  test_assert_int_equal (ns_crash (db), 0);

  // The transaction never committed
  db = ns_open ("test");
  test_assert (db != NULL);
  test_assert (ns_get_var (db, NULL, "get foo") == NULL);

  test_assert_int_equal (ns_close (db), 0);
}
#endif
