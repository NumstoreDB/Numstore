
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
  // v450 failed with ERR_CORRUPT "Root node must have at least 1 element"
  // and v500 with ERR_VARIABLE_NE "Variable: v500 doesn't exist". It got
  // worse from there: commit another transaction, reopen, and around 30
  // more variables were gone
  //      CAUSE:
  //          pgr_flush only writes a page that is in the dirty page table,
  //          and only pgr_release ever added one. pgr_rollback undoes with
  //          pgr_get_writable + pgr_unfix, so the page it just changed was
  //          only in the table if the forward update was still unflushed.
  //
  //          Once the database is bigger than the buffer pool, pages the
  //          transaction changed get evicted - flushed and taken out of the
  //          table - before the rollback. The rollback then undoes them in
  //          memory, the next eviction (or close) drops them without a
  //          write, and the page on disk is the one from before the undo
  //      FIX:
  //          pgr_rollback adds the page to the dirty page table with the
  //          lsn of its clr
  //
  // The same cause broke every large rollback, not only deletes. On a
  // variable of 6000000 u32 (24MB) these all came back wrong or corrupt:
  //      write foo[::3]        then rollback
  //      remove foo[::2]       then rollback
  //      insert foo 1234 3000000, rollback, close, reopen
  // They take too long to be a unit test each, this one stands in for them
  //
  // NOT CHECKED - restart (crash recovery) probably has a cousin of this:
  //    pgr_restart_redo and pgr_restart_undo change pages the same way
  //    (pgr_get_writable + pgr_unfix), and during restart the dirty pages
  //    are tracked in ctx->dpt, which is only merged into p->dpt when
  //    restart is done. pgr_flush looks at p->dpt. So a restart that touches
  //    more pages than the buffer pool should evict redone / undone pages
  //    without writing them
  //
  //    I couldn't get a clean reproduction: a transaction big enough to
  //    need it runs into the ns_crash deadlock of regression 0015 first
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
