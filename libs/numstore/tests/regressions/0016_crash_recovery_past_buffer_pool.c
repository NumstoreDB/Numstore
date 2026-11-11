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

#ifndef NDEBUG
#  include "ns_testing.h"
#endif

#ifndef NDEBUG
TEST (0016_crash_recovery_past_buffer_pool)
{
  // About twice the buffer pool
  enum
  {
    NELEMS = 8500000,
  };

  _Static_assert (
      (u64)NELEMS * sizeof (u32) >= 2 * (u64)MEMORY_PAGE_LEN * NS_PAGE_SIZE,
      "0016: the data must be at least twice the size of the buffer pool"
  );

  u32 *data = ctx->mem.table->malloc (ctx->mem.self, NELEMS, sizeof *data, NULL);
  test_assert (data != NULL);
  for (u32 i = 0; i < NELEMS; ++i) {
    data[i] = i;
  }

  test_assert_int_equal (ns_cleanup ("test"), 0);
  nsdb_t *db = ns_open ("test");
  test_assert (db != NULL);

  // One committed insert, then crash before a checkpoint gets to it
  //
  // After the reopen foo was gone ("Variable: foo doesn't exist"), or the
  // open itself failed
  //      CAUSE:
  //          Same family as 0013. Restart tracks dirty pages in ctx->dpt
  //          (the table analysis builds) and only merges it into p->dpt
  //          when restart is done. pgr_flush looks at p->dpt. Redo touches
  //          more pages than the buffer pool holds here, so pages it had
  //          just redone were evicted without a write and the redo was lost
  //      FIX:
  //          pgr_restart_redo and pgr_restart_undo add the page they
  //          change to p->dpt (ARIES 4.4, same note as pgr_rollback).
  //          ctx->dpt still decides what redo looks at.
  //          dpgt_merge_into updates a page that's already in dest instead
  //          of adding it twice
  test_assert_int_equal (ns_execute (db, NULL, "create foo u32"), 0);
  test_assert_int_equal (
      ns_write (db, NULL, data, NELEMS * sizeof *data, "insert foo 0 %d", NELEMS),
      NELEMS
  );
  test_assert_int_equal (ns_crash (db), 0);

  // Recovery has to bring all of it back
  db = ns_open ("test");
  test_assert (db != NULL);

  u32 *back = ctx->mem.table->calloc (ctx->mem.self, NELEMS, sizeof *back, NULL);
  test_assert (back != NULL);
  test_assert_int_equal (ns_read (db, NULL, back, NELEMS * sizeof *back, "read foo"), NELEMS);

  u32 nbad = 0;
  for (u32 i = 0; i < NELEMS; ++i) {
    if (back[i] != data[i]) {
      nbad++;
    }
  }
  test_assert_int_equal (nbad, 0);

  // The undo side: overwrite all of it, and crash with the transaction
  // still open. Restart undo then changes more pages than the buffer pool
  // holds, and each of them has to reach the file as well
  //
  // (This takes longer than one checkpoint period, so it needs the fix of
  // regression 0015 - ns_crash used to hang here)
  for (u32 i = 0; i < NELEMS; ++i) {
    back[i] = ~data[i];
  }

  nsdb_txn_t *tx = ns_begin (db);
  test_assert (tx != NULL);
  test_assert_int_equal (ns_write (db, tx, back, NELEMS * sizeof *back, "write foo"), NELEMS);
  test_assert_int_equal (ns_execute (db, tx, "remove foo[:%d]", NELEMS / 2), NELEMS / 2);
  test_assert_int_equal (ns_crash (db), 0);

  // Recovery has to take all of it out again
  db = ns_open ("test");
  test_assert (db != NULL);
  test_assert_int_equal (ns_read (db, NULL, back, NELEMS * sizeof *back, "read foo"), NELEMS);

  nbad = 0;
  for (u32 i = 0; i < NELEMS; ++i) {
    if (back[i] != data[i]) {
      nbad++;
    }
  }
  test_assert_int_equal (nbad, 0);

  // And the file is good without the wal: close, reopen, read again
  test_assert_int_equal (ns_close (db), 0);
  db = ns_open ("test");
  test_assert (db != NULL);
  test_assert_int_equal (ns_read (db, NULL, back, NELEMS * sizeof *back, "read foo"), NELEMS);

  nbad = 0;
  for (u32 i = 0; i < NELEMS; ++i) {
    if (back[i] != data[i]) {
      nbad++;
    }
  }
  test_assert_int_equal (nbad, 0);

  test_assert_int_equal (ns_close (db), 0);

  ctx->mem.table->free (ctx->mem.self, back);
  ctx->mem.table->free (ctx->mem.self, data);
}
#endif
