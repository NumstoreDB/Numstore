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

TEST (ns_insert_txn)
{
  TEST_CASE ("Committing an insert makes the data persist")
  {
    sb_size res;

    // Clean re open database
    test_assert_int_equal (ns_cleanup ("test"), 0);
    nsdb_t *db = ns_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = ns_exec (db, NULL, "create foo u32");
    test_assert_int_equal (res, 0);

    // Generate random data to insert
    u32 *src = i_malloc (mem, 10 * sizeof (u32), 1, NULL);
    rand_bytes (src, 10 * sizeof (u32));

    // Insert the data inside a transaction and commit it
    struct txn *tx = ns_begin (db);
    test_assert (tx != NULL);

    res = ns_write (db, tx, src, 10 * sizeof (u32), "insert foo %d %d", 0, 10);
    test_assert_int_equal (res, 10);

    test_assert_int_equal (ns_commit (db, tx), 0);

    // The variable should have the new length
    nsdb_var_t *var = ns_get_var (db, NULL, "get foo");
    test_assert (var != NULL);
    test_assert_int_equal (ns_var_len (var), 10);
    ns_var_free (var);

    // Read the data back and compare
    u32 *dst = i_malloc (mem, 10 * sizeof (u32), 1, NULL);
    res      = ns_read (db, NULL, dst, 10 * sizeof (u32), "read foo[:]");
    test_assert_int_equal (res, 10);
    test_assert_memequal (src, dst, 10 * sizeof (u32));

    // Cleanup
    i_free (mem, src);
    i_free (mem, dst);
    test_assert_int_equal (ns_close (db), 0);
  }

  TEST_CASE ("Rolling back an insert leaves the variable length unchanged")
  {
    sb_size res;

    // Clean re open database
    test_assert_int_equal (ns_cleanup ("test"), 0);
    nsdb_t *db = ns_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = ns_exec (db, NULL, "create foo u32");
    test_assert_int_equal (res, 0);

    // The variable starts out empty
    nsdb_var_t *var = ns_get_var (db, NULL, "get foo");
    test_assert (var != NULL);
    test_assert_int_equal (ns_var_len (var), 0);
    ns_var_free (var);

    // Generate random data to insert
    u32 *src = i_malloc (mem, 10 * sizeof (u32), 1, NULL);
    rand_bytes (src, 10 * sizeof (u32));

    // Insert the data inside a transaction, then roll it back
    struct txn *tx = ns_begin (db);
    test_assert (tx != NULL);

    res = ns_write (db, tx, src, 10 * sizeof (u32), "insert foo %d %d", 0, 10);
    test_assert_int_equal (res, 10);

    test_assert_int_equal (ns_rollback (db, tx), 0);

    // The variable should be empty again
    var = ns_get_var (db, NULL, "get foo");
    test_assert (var != NULL);
    test_assert_int_equal (ns_var_len (var), 0);
    ns_var_free (var);

    // Cleanup
    i_free (mem, src);
    test_assert_int_equal (ns_close (db), 0);
  }

  TEST_CASE ("Rolling back an insert reverts the data to its earlier contents")
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

    // Generate extra data to insert in a transaction
    u32 *extra = i_malloc (mem, 10 * sizeof (u32), 1, NULL);
    rand_bytes (extra, 10 * sizeof (u32));

    // Append the extra data inside a transaction
    struct txn *tx = ns_begin (db);
    test_assert (tx != NULL);

    res = ns_write (db, tx, extra, 10 * sizeof (u32), "insert foo %d %d", 10, 10);
    test_assert_int_equal (res, 10);

    // Inside the transaction the variable should have both sets of data
    nsdb_var_t *var = ns_get_var (db, tx, "get foo");
    test_assert (var != NULL);
    test_assert_int_equal (ns_var_len (var), 10 * 2);
    ns_var_free (var);

    // Rollback
    test_assert_int_equal (ns_rollback (db, tx), 0);

    // Only the initial data should remain
    var = ns_get_var (db, NULL, "get foo");
    test_assert (var != NULL);
    test_assert_int_equal (ns_var_len (var), 10);
    ns_var_free (var);

    // Read the data back and compare to the initial data
    u32 *dst = i_malloc (mem, 10 * sizeof (u32), 1, NULL);
    res      = ns_read (db, NULL, dst, 10 * sizeof (u32), "read foo[:]");
    test_assert_int_equal (res, 10);
    test_assert_memequal (initial, dst, 10 * sizeof (u32));

    // Cleanup
    i_free (mem, initial);
    i_free (mem, extra);
    i_free (mem, dst);
    test_assert_int_equal (ns_close (db), 0);
  }

  TEST_CASE (
      "Each single element insert returns one and grows the variable length by "
      "one"
  )
  {
    sb_size res;

    // Clean re open database
    test_assert_int_equal (ns_cleanup ("test"), 0);
    nsdb_t *db = ns_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = ns_exec (db, NULL, "create foo u32");
    test_assert_int_equal (res, 0);

    for (int i = 0; i < 10; ++i) {
      // Append one element
      u32 val;
      rand_bytes (&val, sizeof (val));
      res = ns_write (db, NULL, &val, sizeof (val), "insert foo %d %d", i, 1);
      test_assert_int_equal (res, 1);

      // The length should have grown by one
      nsdb_var_t *var = ns_get_var (db, NULL, "get foo");
      test_assert (var != NULL);
      test_assert_int_equal (ns_var_len (var), i + 1);
      ns_var_free (var);
    }

    // Close database
    test_assert_int_equal (ns_close (db), 0);
  }

  TEST_CASE ("Inserting at the front repeatedly preserves the intended element order")
  {
    sb_size res;

    // Clean re open database
    test_assert_int_equal (ns_cleanup ("test"), 0);
    nsdb_t *db = ns_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = ns_exec (db, NULL, "create foo u32");
    test_assert_int_equal (res, 0);

    // Generate the values we expect to end up with
    u32 *vals = i_malloc (mem, 10 * sizeof (u32), 1, NULL);
    rand_bytes (vals, 10 * sizeof (u32));

    // Insert them one at a time at the front, last value first
    for (int i = 10 - 1; i >= 0; --i) {
      res = ns_write (db, NULL, &vals[i], sizeof (u32), "insert foo %d %d", 0, 1);
      test_assert_int_equal (res, 1);
    }

    // The variable should have the full length
    nsdb_var_t *var = ns_get_var (db, NULL, "get foo");
    test_assert (var != NULL);
    test_assert_int_equal (ns_var_len (var), 10);
    ns_var_free (var);

    // Read the data back and compare
    u32 *dst = i_malloc (mem, 10 * sizeof (u32), 1, NULL);
    res      = ns_read (db, NULL, dst, 10 * sizeof (u32), "read foo[:]");
    test_assert_int_equal (res, 10);
    test_assert_memequal (vals, dst, 10 * sizeof (u32));

    // Cleanup
    i_free (mem, vals);
    i_free (mem, dst);
    test_assert_int_equal (ns_close (db), 0);
  }

  TEST_CASE ("Rolling back %d inserts one after another leaves the data stable", 10)
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

    // Append one element in a transaction and roll it back, over and over
    u32 *extra = i_malloc (mem, 10 * sizeof (u32), 1, NULL);
    rand_bytes (extra, 10 * sizeof (u32));

    for (int i = 0; i < 10; ++i) {
      struct txn *tx = ns_begin (db);
      test_assert (tx != NULL);

      res = ns_write (db, tx, &extra[i], sizeof (u32), "insert foo %d %d", 10, 1);
      test_assert_int_equal (res, 1);

      test_assert_int_equal (ns_rollback (db, tx), 0);

      // The length should be back to the initial length
      nsdb_var_t *var = ns_get_var (db, NULL, "get foo");
      test_assert (var != NULL);
      test_assert_int_equal (ns_var_len (var), 10);
      ns_var_free (var);
    }

    // Read the data back and compare to the initial data
    u32 *dst = i_malloc (mem, 10 * sizeof (u32), 1, NULL);
    res      = ns_read (db, NULL, dst, 10 * sizeof (u32), "read foo[:]");
    test_assert_int_equal (res, 10);
    test_assert_memequal (initial, dst, 10 * sizeof (u32));

    // Cleanup
    i_free (mem, initial);
    i_free (mem, extra);
    i_free (mem, dst);
    test_assert_int_equal (ns_close (db), 0);
  }

  TEST_CASE ("Inserting into many variables keeps each variable independent")
  {
    sb_size res;

    // Clean re open database
    test_assert_int_equal (ns_cleanup ("test"), 0);
    nsdb_t *db = ns_open ("test");
    test_assert (db != NULL);

    // Generate one random element per variable
    u32 *vals = i_malloc (mem, 10 * sizeof (u32), 1, NULL);
    rand_bytes (vals, 10 * sizeof (u32));

    // Create 10 variables, each with a single element
    for (int i = 0; i < 10; ++i) {
      res = ns_exec (db, NULL, "create var_%d u32", i);
      test_assert_int_equal (res, 0);

      res = ns_write (db, NULL, &vals[i], sizeof (u32), "insert var_%d %d %d", i, 0, 1);
      test_assert_int_equal (res, 1);
    }

    // Each variable should hold only its own element
    for (int i = 0; i < 10; ++i) {
      // Check the length
      nsdb_var_t *var = ns_get_var (db, NULL, "get var_%d", i);
      test_assert (var != NULL);
      test_assert_int_equal (ns_var_len (var), 1);
      ns_var_free (var);

      // Check the data
      u32 dst = 0;
      res     = ns_read (db, NULL, &dst, sizeof (dst), "read var_%d[:]", i);
      test_assert_int_equal (res, 1);
      test_assert_int_equal (dst, vals[i]);
    }

    // Cleanup
    i_free (mem, vals);
    test_assert_int_equal (ns_close (db), 0);
  }
}

#endif
