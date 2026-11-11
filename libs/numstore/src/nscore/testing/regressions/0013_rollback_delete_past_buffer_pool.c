
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

#include <inttypes.h>

#ifndef NDEBUG
#  include "core/ns_testing.h"
#endif

#ifndef NDEBUG
TEST (0013_rollback_delete_past_buffer_pool)
{
  // One variable is two pages (the variable page and one data page), so
  // this database is bigger than the buffer pool (MEMORY_PAGE_LEN = 4096)
  enum
  {
    NVARS = 2500,
  };

  test_assert_int_equal (ns_cleanup ("test"), 0);
  nsdb_t *db = ns_open ("test");
  test_assert (db != NULL);

  // v0 = [1000], v1 = [1001], ...
  txn_t *tx = ns_begin (db);
  test_assert (tx != NULL);
  for (u32 i = 0; i < NVARS; ++i) {
    u32 val = 1000 + i;
    test_assert_int_equal (ns_execute (db, tx, "create v%" PRIu32 " u32", i), 0);
    test_assert_int_equal (ns_write (db, tx, &val, sizeof val, "insert v%" PRIu32 " 0 1", i), 1);
  }
  test_assert_int_equal (ns_commit (db, tx), 0);

  // Delete 40 variables that aren't in the buffer pool anymore, then
  // change our mind
  tx = ns_begin (db);
  test_assert (tx != NULL);
  for (u32 i = 100; i < 1100; i += 25) {
    test_assert_int_equal (ns_execute (db, tx, "delete v%" PRIu32, i), 0);
  }
  test_assert_int_equal (ns_rollback (db, tx), 0);

  // Every variable is back
  //
  // NOT FIXED - this test still fails. The work is tabled here
  //
  // WHAT FAILS:
  //    v450 fails with ERR_CORRUPT "Root node must have at least 1 element"
  //    v500 fails with ERR_VARIABLE_NE "Variable: v500 doesn't exist"
  //    The other 2498 variables read back fine
  //
  // WHAT IS KNOWN:
  //    - It needs a database bigger than the buffer pool. With 2048
  //      variables (4096 pages) or less the same test passes, with 2060 it
  //      fails
  //    - It needs enough deletes in the transaction. Rolling back 1, 5, 20
  //      or 60 deletes of the first variables passes. Rolling back 40
  //      deletes spread over the file (this test) fails
  //    - The variables that break are ones the transaction deleted, never
  //      one it didn't touch
  //    - It is deterministic. Same variables every run
  //    - The size of a variable doesn't matter. 300 variables of 20000
  //      elements fail the same way as 2500 variables of 1 element
  //    - Without the transaction it is fine: deleting 1000 of 3000
  //      variables in auto transactions never loses the other 2000
  //    - It gets worse after this point. Commit another transaction that
  //      deletes and creates variables, close and reopen, and around 30
  //      more variables are gone
  //    - A release build fails the same way, so it isn't an assert
  //
  // WHAT I THINK IS HAPPENING (not proven):
  //    The delete changes pages that then get evicted before the rollback
  //    (the pool is full, so it has to evict). The rollback then undoes the
  //    delete on a page image that isn't the one the delete produced - so
  //    the hash chain link of v500 and the data root of v450 don't come
  //    back. Regressions 0001 and 0004 were in the same area (delete +
  //    rollback, and state diverging after churn)
  //
  // WHERE TO LOOK NEXT:
  //    - pgr_delete_and_release and its undo in ns_pgr_rollback.c
  //    - ns_pgr_evict.c: what happens to a page that a live transaction
  //      deleted. Is it flushed with its page lsn before it leaves the pool
  //    - Shrink MEMORY_PAGE_LEN to something like 16 in a debug build. This
  //      test should then fail with a handful of variables, and the
  //      simulation should start hitting it too (today the simulation never
  //      fills 4096 pages, which is why it never found this)
  //
  // The test is slow (about 9 seconds), nearly all of it in the setup.
  // Fewer, bigger variables don't make it faster
  for (u32 i = 0; i < NVARS; ++i) {
    u32 val = 0;
    test_assert_int_equal (ns_read (db, NULL, &val, sizeof val, "read v%" PRIu32, i), 1);
    test_assert_int_equal (val, 1000 + i);
  }

  test_assert_int_equal (ns_close (db), 0);
}
#endif
