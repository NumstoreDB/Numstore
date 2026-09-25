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

#include "numstore/numstore.h"

#ifdef TESTING
#  include "core/testing/ns_testing.h"
#endif

#ifdef TESTING

TEST (ns_create_txn)
{
  TEST_CASE ("Committing a created variable makes it persist across a database reopen")
  {
    int res;

    // Clean re open database
    test_assert_int_equal (ns_cleanup ("test"), 0);
    nsdb_t *db = ns_open ("test");
    test_assert (db != NULL);

    // Create a variable inside a transaction and commit it
    struct txn *tx = ns_begin (db);
    test_assert (tx != NULL);

    res = ns_exec (db, tx, "create foo u32");
    test_assert_int_equal (res, 0);

    test_assert_int_equal (ns_commit (db, tx), 0);

    // Close and re open the database
    test_assert_int_equal (ns_close (db), 0);
    db = ns_open ("test");
    test_assert (db != NULL);

    // The variable should still exist and be empty
    nsdb_var_t *var = ns_get_var (db, NULL, "get foo");
    test_assert (var != NULL);
    test_assert_int_equal (ns_var_len (var), 0);
    ns_var_free (var);

    // Close database
    test_assert_int_equal (ns_close (db), 0);
  }

  TEST_CASE ("Rolling back a created variable leaves it not visible")
  {
    int res;

    // Clean re open database
    test_assert_int_equal (ns_cleanup ("test"), 0);
    nsdb_t *db = ns_open ("test");
    test_assert (db != NULL);

    // Begin transaction
    struct txn *tx = ns_begin (db);
    test_assert (tx != NULL);

    // Create a new variable
    res = ns_exec (db, tx, "create foo u32");
    test_assert_int_equal (res, 0);

    // Rollback
    test_assert_int_equal (ns_rollback (db, tx), 0);

    // Get the variable (should fail)
    nsdb_var_t *var = ns_get_var (db, NULL, "get foo");
    test_assert (var == NULL);

    // Close database
    test_assert_int_equal (ns_close (db), 0);
  }

  TEST_CASE (
      "Creating a variable succeeds after an earlier create of the same name "
      "was rolled back"
  )
  {
    int res;

    // Clean re open database
    test_assert_int_equal (ns_cleanup ("test"), 0);
    nsdb_t *db = ns_open ("test");
    test_assert (db != NULL);

    // Create a variable inside a transaction, then roll it back
    struct txn *tx = ns_begin (db);
    test_assert (tx != NULL);

    res = ns_exec (db, tx, "create foo u32");
    test_assert_int_equal (res, 0);

    test_assert_int_equal (ns_rollback (db, tx), 0);

    // Create a variable with the same name outside of a transaction
    res = ns_exec (db, NULL, "create foo u32");
    test_assert_int_equal (res, 0);

    // Close database
    test_assert_int_equal (ns_close (db), 0);
  }

  TEST_CASE (
      "Creating %d variables in separate committed transactions makes them all "
      "persist",
      10
  )
  {
    int res;

    // Clean re open database
    test_assert_int_equal (ns_cleanup ("test"), 0);
    nsdb_t *db = ns_open ("test");
    test_assert (db != NULL);

    // Create a variable per transaction and commit each
    for (int i = 0; i < 10; ++i) {
      struct txn *tx = ns_begin (db);
      test_assert (tx != NULL);

      res = ns_exec (db, tx, "create var_%d u32", i);
      test_assert_int_equal (res, 0);

      test_assert_int_equal (ns_commit (db, tx), 0);
    }

    // Every variable should exist and be empty
    for (int i = 0; i < 10; ++i) {
      nsdb_var_t *var = ns_get_var (db, NULL, "get var_%d", i);
      test_assert (var != NULL);
      test_assert_int_equal (ns_var_len (var), 0);
      ns_var_free (var);
    }

    // Close database
    test_assert_int_equal (ns_close (db), 0);
  }

  TEST_CASE (
      "Only committed creates persist when %d transactions alternate commit "
      "and rollback",
      10
  )
  {
    int res;

    // Clean re open database
    test_assert_int_equal (ns_cleanup ("test"), 0);
    nsdb_t *db = ns_open ("test");
    test_assert (db != NULL);

    // Create a variable per transaction, committing evens and rolling back odds
    for (int i = 0; i < 10; ++i) {
      struct txn *tx = ns_begin (db);
      test_assert (tx != NULL);

      res = ns_exec (db, tx, "create var_%d u32", i);
      test_assert_int_equal (res, 0);

      if (i % 2 == 0) {
        test_assert_int_equal (ns_commit (db, tx), 0);
      } else {
        test_assert_int_equal (ns_rollback (db, tx), 0);
      }
    }

    // Evens should exist, odds should not
    for (int i = 0; i < 10; ++i) {
      nsdb_var_t *var = ns_get_var (db, NULL, "get var_%d", i);
      if (i % 2 == 0) {
        test_assert (var != NULL);
        test_assert_int_equal (ns_var_len (var), 0);
        ns_var_free (var);
      } else {
        test_assert (var == NULL);
      }
    }

    // Close database
    test_assert_int_equal (ns_close (db), 0);
  }

  TEST_CASE ("A newly created variable is always empty")
  {
    int res;

    // Clean re open database
    test_assert_int_equal (ns_cleanup ("test"), 0);
    nsdb_t *db = ns_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < 10; ++i) {
      // Create a variable
      res = ns_exec (db, NULL, "create var_%d u32", i);
      test_assert_int_equal (res, 0);

      // Check that it is empty
      nsdb_var_t *var = ns_get_var (db, NULL, "get var_%d", i);
      test_assert (var != NULL);
      test_assert_int_equal (ns_var_len (var), 0);
      ns_var_free (var);
    }

    // Close database
    test_assert_int_equal (ns_close (db), 0);
  }

  TEST_CASE ("Creating a variable that already exists fails")
  {
    int res;

    // Clean re open database
    test_assert_int_equal (ns_cleanup ("test"), 0);
    nsdb_t *db = ns_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < 10; ++i) {
      // Create a variable
      res = ns_exec (db, NULL, "create var_%d u32", i);
      test_assert_int_equal (res, 0);

      // Create the same variable again (should fail)
      res = ns_exec (db, NULL, "create var_%d f32", i);
      test_assert (res != 0);
      ns_strerror (db);
    }

    // Close database
    test_assert_int_equal (ns_close (db), 0);
  }

  TEST_CASE (
      "Rolling back a create %d times and then committing it once leaves the "
      "variable empty",
      10
  )
  {
    int res;

    // Clean re open database
    test_assert_int_equal (ns_cleanup ("test"), 0);
    nsdb_t *db = ns_open ("test");
    test_assert (db != NULL);

    // Create the same variable and roll it back, over and over
    for (int i = 0; i < 10; ++i) {
      struct txn *tx = ns_begin (db);
      test_assert (tx != NULL);

      res = ns_exec (db, tx, "create foo u32");
      test_assert_int_equal (res, 0);

      test_assert_int_equal (ns_rollback (db, tx), 0);

      // The variable should not exist
      nsdb_var_t *var = ns_get_var (db, NULL, "get foo");
      test_assert (var == NULL);
      ns_strerror (db);
    }

    // Finally create the variable and commit it
    struct txn *tx = ns_begin (db);
    test_assert (tx != NULL);

    res = ns_exec (db, tx, "create foo u32");
    test_assert_int_equal (res, 0);

    test_assert_int_equal (ns_commit (db, tx), 0);

    // The variable should exist and be empty
    nsdb_var_t *var = ns_get_var (db, NULL, "get foo");
    test_assert (var != NULL);
    test_assert_int_equal (ns_var_len (var), 0);
    ns_var_free (var);

    // Close database
    test_assert_int_equal (ns_close (db), 0);
  }
}

#endif
