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

#include "core/ns_numerics.h"
#include "numstore/numstore.h"

#ifdef TESTING
#  include "core/testing/ns_testing.h"
#endif

#ifdef TESTING

TEST (ns_write_txn)
{
  TEST_CASE ("Committing a write makes the new data persist")
  {
    sb_size res;

    // Clean re open database
    test_assert_int_equal (ns_cleanup ("test"), 0);
    nsdb_t *db = ns_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = ns_exec (db, NULL, "create foo u32");
    test_assert_int_equal (res, 0);

    // Generate the initial data and the data we will write over it
    u32 *initial = i_malloc (mem, 10 * sizeof (u32), 1, NULL);
    u32 *patch   = i_malloc (mem, 10 * sizeof (u32), 1, NULL);
    rand_bytes (initial, 10 * sizeof (u32));
    rand_bytes (patch, 10 * sizeof (u32));

    // Insert the initial data
    res = ns_write (db, NULL, initial, 10 * sizeof (u32), "insert foo %d %d", 0, 10);
    test_assert_int_equal (res, 10);

    // Overwrite everything inside a transaction and commit it
    struct txn *tx = ns_begin (db);
    test_assert (tx != NULL);

    res = ns_write (db, tx, patch, 10 * sizeof (u32), "write foo[0:%d:1]", 10);
    test_assert_int_equal (res, 10);

    test_assert_int_equal (ns_commit (db, tx), 0);

    // Read the data back and compare to the patch
    u32 *dst = i_malloc (mem, 10 * sizeof (u32), 1, NULL);
    res      = ns_read (db, NULL, dst, 10 * sizeof (u32), "read foo[:]");
    test_assert_int_equal (res, 10);
    test_assert_memequal (patch, dst, 10 * sizeof (u32));

    // Cleanup
    i_free (mem, initial);
    i_free (mem, patch);
    i_free (mem, dst);
    test_assert_int_equal (ns_close (db), 0);
  }

  TEST_CASE ("Rolling back a write reverts the data to its earlier contents")
  {
    sb_size res;

    // Clean re open database
    test_assert_int_equal (ns_cleanup ("test"), 0);
    nsdb_t *db = ns_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = ns_exec (db, NULL, "create foo u32");
    test_assert_int_equal (res, 0);

    // Generate the initial data and the data we will write over it
    u32 *initial = i_malloc (mem, 10 * sizeof (u32), 1, NULL);
    u32 *patch   = i_malloc (mem, 10 * sizeof (u32), 1, NULL);
    rand_bytes (initial, 10 * sizeof (u32));
    rand_bytes (patch, 10 * sizeof (u32));

    // Insert the initial data
    res = ns_write (db, NULL, initial, 10 * sizeof (u32), "insert foo %d %d", 0, 10);
    test_assert_int_equal (res, 10);

    // Overwrite everything inside a transaction, then roll it back
    struct txn *tx = ns_begin (db);
    test_assert (tx != NULL);

    res = ns_write (db, tx, patch, 10 * sizeof (u32), "write foo[0:%d:1]", 10);
    test_assert_int_equal (res, 10);

    test_assert_int_equal (ns_rollback (db, tx), 0);

    // Read the data back and compare to the initial data
    u32 *dst = i_malloc (mem, 10 * sizeof (u32), 1, NULL);
    res      = ns_read (db, NULL, dst, 10 * sizeof (u32), "read foo[:]");
    test_assert_int_equal (res, 10);
    test_assert_memequal (initial, dst, 10 * sizeof (u32));

    // Cleanup
    i_free (mem, initial);
    i_free (mem, patch);
    i_free (mem, dst);
    test_assert_int_equal (ns_close (db), 0);
  }

  TEST_CASE ("Writing to a variable does not change its length")
  {
    sb_size res;

    // Clean re open database
    test_assert_int_equal (ns_cleanup ("test"), 0);
    nsdb_t *db = ns_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = ns_exec (db, NULL, "create foo u32");
    test_assert_int_equal (res, 0);

    // Insert the initial data
    u32 *data = i_malloc (mem, 10 * sizeof (u32), 1, NULL);
    rand_bytes (data, 10 * sizeof (u32));
    res = ns_write (db, NULL, data, 10 * sizeof (u32), "insert foo %d %d", 0, 10);
    test_assert_int_equal (res, 10);
    i_free (mem, data);

    // Overwrite one element at a time
    for (int i = 0; i < 10; ++i) {
      u32 val;
      rand_bytes (&val, sizeof (val));
      res = ns_write (db, NULL, &val, sizeof (val), "write foo[%d:%d:1]", i, i + 1);
      test_assert_int_equal (res, 1);

      // The length should not have changed
      nsdb_var_t *var = ns_get_var (db, NULL, "get foo");
      test_assert (var != NULL);
      test_assert_int_equal (ns_var_len (var), 10);
      ns_var_free (var);
    }

    // Close database
    test_assert_int_equal (ns_close (db), 0);
  }

  TEST_CASE ("Writing a single element leaves all other elements unchanged")
  {
    sb_size res;

    // Clean re open database
    test_assert_int_equal (ns_cleanup ("test"), 0);
    nsdb_t *db = ns_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = ns_exec (db, NULL, "create foo u32");
    test_assert_int_equal (res, 0);

    // Insert the initial data - shadow tracks what the variable should contain
    u32 *shadow = i_malloc (mem, 10 * sizeof (u32), 1, NULL);
    rand_bytes (shadow, 10 * sizeof (u32));
    res = ns_write (db, NULL, shadow, 10 * sizeof (u32), "insert foo %d %d", 0, 10);
    test_assert_int_equal (res, 10);

    // Overwrite a random element, then compare everything against the shadow
    u32 *dst = i_malloc (mem, 10 * sizeof (u32), 1, NULL);
    for (int i = 0; i < 10; ++i) {
      int idx = randu32 () % 10;
      u32 val;
      rand_bytes (&val, sizeof (val));
      shadow[idx] = val;

      // Write the element
      res         = ns_write (db, NULL, &val, sizeof (val), "write foo[%d:%d:1]", idx, idx + 1);
      test_assert_int_equal (res, 1);

      // Read everything back and compare
      res = ns_read (db, NULL, dst, 10 * sizeof (u32), "read foo[:]");
      test_assert_int_equal (res, 10);
      test_assert_memequal (shadow, dst, 10 * sizeof (u32));
    }

    // Cleanup
    i_free (mem, shadow);
    i_free (mem, dst);
    test_assert_int_equal (ns_close (db), 0);
  }

  TEST_CASE ("Rolling back %d random single element writes leaves the data stable", 10)
  {
    sb_size res;

    // Clean re open database
    test_assert_int_equal (ns_cleanup ("test"), 0);
    nsdb_t *db = ns_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = ns_exec (db, NULL, "create foo u32");
    test_assert_int_equal (res, 0);

    // Insert the initial data
    u32 *initial = i_malloc (mem, 10 * sizeof (u32), 1, NULL);
    rand_bytes (initial, 10 * sizeof (u32));
    res = ns_write (db, NULL, initial, 10 * sizeof (u32), "insert foo %d %d", 0, 10);
    test_assert_int_equal (res, 10);

    // Overwrite a random element in a transaction and roll it back, over and
    // over
    u32 *dst = i_malloc (mem, 10 * sizeof (u32), 1, NULL);
    for (int i = 0; i < 10; ++i) {
      int idx = randu32 () % 10;
      u32 val;
      rand_bytes (&val, sizeof (val));

      // Write and rollback
      struct txn *tx = ns_begin (db);
      test_assert (tx != NULL);

      res = ns_write (db, tx, &val, sizeof (val), "write foo[%d:%d:1]", idx, idx + 1);
      test_assert_int_equal (res, 1);

      test_assert_int_equal (ns_rollback (db, tx), 0);

      // Everything should still match the initial data
      res = ns_read (db, NULL, dst, 10 * sizeof (u32), "read foo[:]");
      test_assert_int_equal (res, 10);
      test_assert_memequal (initial, dst, 10 * sizeof (u32));
    }

    // Cleanup
    i_free (mem, initial);
    i_free (mem, dst);
    test_assert_int_equal (ns_close (db), 0);
  }

  TEST_CASE ("Committed writes persist across %d database reopens", 20)
  {
    sb_size res;

    // Clean re open database
    test_assert_int_equal (ns_cleanup ("test"), 0);
    nsdb_t *db = ns_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = ns_exec (db, NULL, "create foo u32");
    test_assert_int_equal (res, 0);

    // Insert the initial data - data tracks what the variable should contain
    u32 *data = i_malloc (mem, 10 * sizeof (u32), 1, NULL);
    rand_bytes (data, 10 * sizeof (u32));
    res = ns_write (db, NULL, data, 10 * sizeof (u32), "insert foo %d %d", 0, 10);
    test_assert_int_equal (res, 10);

    u32 *dst = i_malloc (mem, 10 * sizeof (u32), 1, NULL);
    for (int i = 0; i < 20; ++i) {
      int idx = randu32 () % 10;
      u32 val;
      rand_bytes (&val, sizeof (val));
      data[idx]      = val;

      // Write a random element in a transaction and commit it
      struct txn *tx = ns_begin (db);
      test_assert (tx != NULL);

      res = ns_write (db, tx, &val, sizeof (val), "write foo[%d:%d:1]", idx, idx + 1);
      test_assert_int_equal (res, 1);

      test_assert_int_equal (ns_commit (db, tx), 0);

      // Close and re open the database
      test_assert_int_equal (ns_close (db), 0);
      db = ns_open ("test");
      test_assert (db != NULL);

      // Everything should still match after the reopen
      res = ns_read (db, NULL, dst, 10 * sizeof (u32), "read foo[:]");
      test_assert_int_equal (res, 10);
      test_assert_memequal (data, dst, 10 * sizeof (u32));
    }

    // Cleanup
    i_free (mem, data);
    i_free (mem, dst);
    test_assert_int_equal (ns_close (db), 0);
  }
}

#endif
