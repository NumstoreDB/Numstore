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

#include "numstore.h"

#ifndef NDEBUG
#  include "ns_testing.h"
#endif

#ifndef NDEBUG

#  define ARIES_TEST_DB "testdb"

/// Wipe [ARIES_TEST_DB], open it, and create the single u8 variable "var"
/// that every test below treats as its "file".
static nsdb_t *
aries_open_fresh (void)
{
  if (ns_cleanup (ARIES_TEST_DB) < 0) {
    return NULL;
  }

  nsdb_t *db = ns_open (ARIES_TEST_DB);
  if (db == NULL) {
    return NULL;
  }

  if (ns_execute (db, NULL, "create var u8") < 0) {
    ns_close (db);
    return NULL;
  }

  return db;
}

/// Re open the existing [ARIES_TEST_DB] (runs recovery if it was crashed)
static nsdb_t *
aries_reopen (void)
{
  return ns_open (ARIES_TEST_DB);
}

/// Insert [len] bytes of [data] at byte offset [ofst] of "var"
static sb_size
aries_insert (nsdb_t *db, nsdb_txn_t *tx, const char *data, u32 ofst, u32 len)
{
  return ns_write (db, tx, data, len, "insert var %d %d", (int)ofst, (int)len);
}

/// Read [len] bytes starting at byte offset [ofst] of "var" into [dest]
static sb_size
aries_read (nsdb_t *db, nsdb_txn_t *tx, char *dest, u32 ofst, u32 len)
{
  return ns_read (db, tx, dest, len, "read var[%d:%d]", (int)ofst, (int)(ofst + len));
}

/// Read the whole variable back and compare it to the string literal [expected]
#  define ASSERT_CONTENTS(db, expected)                           \
    do {                                                          \
      char actual[sizeof (expected) - 1];                         \
      test_assert_int_equal (                                     \
          aries_read ((db), NULL, actual, 0, sizeof (actual)),    \
          (int)sizeof (actual)                                    \
      );                                                          \
      test_assert_memequal ((expected), actual, sizeof (actual)); \
    }                                                             \
    while (0)

/// Insert [str] (a string literal) at [ofst] and expect every byte to land
#  define INSERT_STR(db, tx, str, ofst)                             \
    test_assert_int_equal (                                         \
        aries_insert ((db), (tx), (str), (ofst), sizeof (str) - 1), \
        (int)(sizeof (str) - 1)                                     \
    )

TEST (aries_crash)
{
  TEST_CASE ("sample5_bug")
  {
    nsdb_t *db = aries_open_fresh ();
    test_assert (db != NULL);

    nsdb_txn_t *tx = ns_begin (db);
    test_assert (tx != NULL);
    INSERT_STR (db, tx, "AAAAAAAAAA", 0);
    test_assert_int_equal (ns_commit (db, tx), 0);
    test_assert_int_equal (ns_close (db), 0);

    db = aries_reopen ();
    test_assert (db != NULL);
    tx = ns_begin (db);
    test_assert (tx != NULL);
    INSERT_STR (db, tx, "BB", 3);
    test_assert_int_equal (ns_commit (db, tx), 0);
    test_assert_int_equal (ns_close (db), 0);

    db = aries_reopen ();
    test_assert (db != NULL);
    tx = ns_begin (db);
    test_assert (tx != NULL);
    INSERT_STR (db, tx, "CC", 7);
    test_assert_int_equal (ns_commit (db, tx), 0);
    test_assert_int_equal (ns_crash (db), 0);

    db = aries_reopen ();
    test_assert (db != NULL);
    ASSERT_CONTENTS (db, "AAABBAACCAAAAA");
    test_assert_int_equal (ns_close (db), 0);
  }

  // 1. Uncommitted work after a crash must be discarded.
  TEST_CASE ("crash_before_commit_discards_uncommitted")
  {
    nsdb_t *db = aries_open_fresh ();
    test_assert (db != NULL);

    nsdb_txn_t *tx = ns_begin (db);
    test_assert (tx != NULL);
    INSERT_STR (db, tx, "AAAAAAAAAA", 0);
    test_assert_int_equal (ns_commit (db, tx), 0);
    test_assert_int_equal (ns_close (db), 0);

    db = aries_reopen ();
    test_assert (db != NULL);
    tx = ns_begin (db);
    test_assert (tx != NULL);
    INSERT_STR (db, tx, "ZZ", 3);
    // deliberately no commit
    test_assert_int_equal (ns_crash (db), 0);

    db = aries_reopen ();
    test_assert (db != NULL);
    ASSERT_CONTENTS (db, "AAAAAAAAAA");
    test_assert_int_equal (ns_close (db), 0);
  }

  // 2. Committed txn followed by uncommitted txn in the same session.
  //    Recovery must keep the first, throw away the second.
  TEST_CASE ("crash_keeps_committed_drops_followon_uncommitted")
  {
    nsdb_t *db = aries_open_fresh ();
    test_assert (db != NULL);

    nsdb_txn_t *tx = ns_begin (db);
    test_assert (tx != NULL);
    INSERT_STR (db, tx, "HELLO", 0);
    test_assert_int_equal (ns_commit (db, tx), 0);

    tx = ns_begin (db);
    test_assert (tx != NULL);
    INSERT_STR (db, tx, "XX", 1);
    test_assert_int_equal (ns_crash (db), 0);

    db = aries_reopen ();
    test_assert (db != NULL);
    ASSERT_CONTENTS (db, "HELLO");
    test_assert_int_equal (ns_close (db), 0);
  }

  // 3. Multiple crash/reopen cycles. Each session commits, then crashes;
  //    every committed write must survive across all of them.
  TEST_CASE ("repeated_crash_recover_cycles_preserve_all_commits")
  {
    nsdb_t *db = aries_open_fresh ();
    test_assert (db != NULL);
    nsdb_txn_t *tx = ns_begin (db);
    test_assert (tx != NULL);
    INSERT_STR (db, tx, "ONE", 0);
    test_assert_int_equal (ns_commit (db, tx), 0);
    test_assert_int_equal (ns_crash (db), 0);

    db = aries_reopen ();
    test_assert (db != NULL);
    tx = ns_begin (db);
    test_assert (tx != NULL);
    INSERT_STR (db, tx, "TWO", 3);
    test_assert_int_equal (ns_commit (db, tx), 0);
    test_assert_int_equal (ns_crash (db), 0);

    db = aries_reopen ();
    test_assert (db != NULL);
    tx = ns_begin (db);
    test_assert (tx != NULL);
    INSERT_STR (db, tx, "THREE", 6);
    test_assert_int_equal (ns_commit (db, tx), 0);
    test_assert_int_equal (ns_crash (db), 0);

    db = aries_reopen ();
    test_assert (db != NULL);
    ASSERT_CONTENTS (db, "ONETWOTHREE");
    test_assert_int_equal (ns_close (db), 0);
  }

  // 4. Crash with no commits at all in the new session.
  //    File must look exactly as it did at the previous clean close.
  TEST_CASE ("crash_with_no_new_commits_is_a_noop")
  {
    nsdb_t *db = aries_open_fresh ();
    test_assert (db != NULL);

    nsdb_txn_t *tx = ns_begin (db);
    test_assert (tx != NULL);
    INSERT_STR (db, tx, "STABLE", 0);
    test_assert_int_equal (ns_commit (db, tx), 0);
    test_assert_int_equal (ns_close (db), 0);

    db = aries_reopen ();
    test_assert (db != NULL);
    // open, do nothing, crash
    test_assert_int_equal (ns_crash (db), 0);

    db = aries_reopen ();
    test_assert (db != NULL);
    ASSERT_CONTENTS (db, "STABLE");
    test_assert_int_equal (ns_close (db), 0);
  }

  // 5. Many tiny committed transactions, then crash.
  //    Stresses WAL replay across a long redo chain.
  TEST_CASE ("many_small_commits_then_crash")
  {
    nsdb_t *db = aries_open_fresh ();
    test_assert (db != NULL);

    for (int i = 0; i < 26; i++) {
      char        c  = (char)('A' + i);
      nsdb_txn_t *tx = ns_begin (db);
      test_assert (tx != NULL);
      test_assert_int_equal (aries_insert (db, tx, &c, i, 1), 1);
      test_assert_int_equal (ns_commit (db, tx), 0);
    }
    test_assert_int_equal (ns_crash (db), 0);

    db = aries_reopen ();
    test_assert (db != NULL);
    ASSERT_CONTENTS (db, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    test_assert_int_equal (ns_close (db), 0);
  }

  // 6. Several inserts inside a single transaction, then commit, then crash.
  //    Either ALL of them survive or NONE of them - atomicity check.
  TEST_CASE ("multi_insert_single_txn_is_atomic_through_crash")
  {
    nsdb_t *db = aries_open_fresh ();
    test_assert (db != NULL);

    nsdb_txn_t *tx = ns_begin (db);
    test_assert (tx != NULL);
    INSERT_STR (db, tx, "AAAA", 0);
    INSERT_STR (db, tx, "BB", 2);
    INSERT_STR (db, tx, "CC", 0);
    test_assert_int_equal (ns_commit (db, tx), 0);
    test_assert_int_equal (ns_crash (db), 0);

    // AAAA -> AABBAA -> CCAABBAA
    db = aries_reopen ();
    test_assert (db != NULL);
    ASSERT_CONTENTS (db, "CCAABBAA");
    test_assert_int_equal (ns_close (db), 0);
  }

  // 7. Append at the exact end-of-file boundary, then crash.
  //    Catches off-by-ones where offset == length is treated as out-of-range.
  TEST_CASE ("append_at_end_offset_then_crash")
  {
    nsdb_t *db = aries_open_fresh ();
    test_assert (db != NULL);

    nsdb_txn_t *tx = ns_begin (db);
    test_assert (tx != NULL);
    INSERT_STR (db, tx, "ABCDE", 0);
    test_assert_int_equal (ns_commit (db, tx), 0);
    test_assert_int_equal (ns_close (db), 0);

    db = aries_reopen ();
    test_assert (db != NULL);
    tx = ns_begin (db);
    test_assert (tx != NULL);
    INSERT_STR (db, tx, "FGH", 5); // offset == current length
    test_assert_int_equal (ns_commit (db, tx), 0);
    test_assert_int_equal (ns_crash (db), 0);

    db = aries_reopen ();
    test_assert (db != NULL);
    ASSERT_CONTENTS (db, "ABCDEFGH");
    test_assert_int_equal (ns_close (db), 0);
  }

  // 8. Insert at offset 0 into a non-empty file shifts everything right.
  //    Recovery must preserve the shift.
  TEST_CASE ("insert_at_zero_shifts_existing_through_crash")
  {
    nsdb_t *db = aries_open_fresh ();
    test_assert (db != NULL);

    nsdb_txn_t *tx = ns_begin (db);
    test_assert (tx != NULL);
    INSERT_STR (db, tx, "WORLD", 0);
    test_assert_int_equal (ns_commit (db, tx), 0);
    test_assert_int_equal (ns_close (db), 0);

    db = aries_reopen ();
    test_assert (db != NULL);
    tx = ns_begin (db);
    test_assert (tx != NULL);
    INSERT_STR (db, tx, "HELLO ", 0);
    test_assert_int_equal (ns_commit (db, tx), 0);
    test_assert_int_equal (ns_crash (db), 0);

    db = aries_reopen ();
    test_assert (db != NULL);
    ASSERT_CONTENTS (db, "HELLO WORLD");
    test_assert_int_equal (ns_close (db), 0);
  }

  // 9. Single insert larger than one page - exercises multi-page redo.
  //    Adjust BIG_SIZE to comfortably exceed your page size.
  TEST_CASE ("large_page_spanning_insert_then_crash")
  {
    enum
    {
      BIG_SIZE = 16384
    };

    char *big = ctx->mem.table->malloc (ctx->mem.self, BIG_SIZE, 1, NULL);
    test_assert (big != NULL);
    for (int i = 0; i < BIG_SIZE; i++) {
      big[i] = (char)('A' + (i % 26));
    }

    nsdb_t *db = aries_open_fresh ();
    test_assert (db != NULL);
    nsdb_txn_t *tx = ns_begin (db);
    test_assert (tx != NULL);
    test_assert_int_equal (aries_insert (db, tx, big, 0, BIG_SIZE), BIG_SIZE);
    test_assert_int_equal (ns_commit (db, tx), 0);
    test_assert_int_equal (ns_crash (db), 0);

    db = aries_reopen ();
    test_assert (db != NULL);
    char *actual = ctx->mem.table->malloc (ctx->mem.self, BIG_SIZE, 1, NULL);
    test_assert (actual != NULL);
    test_assert_int_equal (aries_read (db, NULL, actual, 0, BIG_SIZE), BIG_SIZE);
    test_assert_memequal (big, actual, BIG_SIZE);
    test_assert_int_equal (ns_close (db), 0);

    ctx->mem.table->free (ctx->mem.self, big);
    ctx->mem.table->free (ctx->mem.self, actual);
  }

  // 10. Reads at the boundary of recovered content.
  //     Read from the tail end of the file after recovery, where partial
  //     reads / overruns are most likely to misbehave.
  TEST_CASE ("tail_read_after_recovery")
  {
    nsdb_t *db = aries_open_fresh ();
    test_assert (db != NULL);

    nsdb_txn_t *tx = ns_begin (db);
    test_assert (tx != NULL);
    INSERT_STR (db, tx, "0123456789", 0);
    test_assert_int_equal (ns_commit (db, tx), 0);
    test_assert_int_equal (ns_crash (db), 0);

    db = aries_reopen ();
    test_assert (db != NULL);
    // read the last 4 bytes
    char actual[4];
    test_assert_int_equal (aries_read (db, NULL, actual, 6, sizeof (actual)), 4);
    test_assert_memequal ("6789", actual, sizeof (actual));
    test_assert_int_equal (ns_close (db), 0);
  }
}

#endif
