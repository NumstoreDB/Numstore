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

#include "core/ns_error.h"
#include "core/ns_numerics.h"
#include "core/ns_stdtypes.h"
#include "numstore/numstore.h"

#ifdef TESTING
#  include "core/testing/ns_testing.h"
#endif

#ifdef TESTING

#  define ITERS 10

TEST (ns_delete_txn)
{
  TEST_CASE ("Rolling back a delete lets the variable be deleted again")
  {
    int res;

    // Clean re open database
    test_assert_int_equal (ns_cleanup ("test"), 0);
    nsdb_t *db = ns_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = ns_exec (db, NULL, "create foo u32");
    test_assert_int_equal (res, 0);

    // Delete the variable inside a transaction, then roll it back
    struct txn *tx = ns_begin (db);
    test_assert (tx != NULL);

    res = ns_exec (db, tx, "delete foo");
    test_assert_int_equal (res, 0);

    test_assert_int_equal (ns_rollback (db, tx), 0);

    // Delete the variable again, outside of a transaction
    res = ns_exec (db, NULL, "delete foo");
    test_assert_int_equal (res, 0);

    // Close database
    test_assert_int_equal (ns_close (db), 0);
  }

  TEST_CASE ("Deleting a variable inside a transaction makes it no longer visible")
  {
    int res;

    // Clean re open database
    test_assert_int_equal (ns_cleanup ("test"), 0);
    nsdb_t *db = ns_open ("test");
    test_assert (db != NULL);

    // Create ITERS variables
    for (int i = 0; i < ITERS; ++i) {
      res = ns_exec (db, NULL, "create var_%d u32", i);
      test_assert_int_equal (res, 0);
    }

    // Delete a variable in a transaction, rolling back each time
    for (int i = 0; i < ITERS; ++i) {
      // Begin transaction
      struct txn *tx = ns_begin (db);
      test_assert (tx != NULL);

      // Delete the variable
      res = ns_exec (db, tx, "delete var_%d", i);
      test_assert_int_equal (res, 0);

      // Inside the transaction the variable should be gone
      nsdb_var_t *var = ns_get_var (db, tx, "get var_%d", i);
      test_assert (var == NULL);

      // Rollback transaction
      test_assert_int_equal (ns_rollback (db, tx), 0);

      // After the rollback the variable should be back
      var = ns_get_var (db, NULL, "get var_%d", i);
      test_assert (var != NULL);
      ns_var_free (var);
    }

    // Close database
    test_assert_int_equal (ns_close (db), 0);
  }

  TEST_CASE ("Rolling back a delete keeps both the variable and its data intact")
  {
    sb_size res;

    // Clean re open database
    test_assert_int_equal (ns_cleanup ("test"), 0);
    nsdb_t *db = ns_open ("test");
    test_assert (db != NULL);

    // Create one variable
    res = ns_exec (db, NULL, "create foo u32");
    test_assert_int_equal (res, 0);

    // Insert an array into the variable
    u32 src[ITERS];
    rand_bytes (src, sizeof (src));
    res = ns_write (db, NULL, src, sizeof (src), "insert foo %d %d", 0, ITERS);
    test_assert_int_equal (res, ITERS);

    for (int i = 0; i < ITERS; ++i) {
      // Delete the variable inside a transaction, then roll it back
      {
        struct txn *tx = ns_begin (db);
        test_assert (tx != NULL);

        res = ns_exec (db, tx, "delete foo");
        test_assert_int_equal (res, 0);

        test_assert_int_equal (ns_rollback (db, tx), 0);
      }

      // The variable should still exist with the same length
      {
        nsdb_var_t *var = ns_get_var (db, NULL, "get foo");
        test_assert (var != NULL);
        test_assert_int_equal (ns_var_len (var), ITERS);
        ns_var_free (var);
      }

      // The data should be unchanged
      {
        u32 dst[ITERS];
        res = ns_read (db, NULL, dst, sizeof (dst), "read foo[:]");
        test_assert_int_equal (res, ITERS);
        test_assert_memequal (src, dst, sizeof (src));
      }
    }

    // Close database
    test_assert_int_equal (ns_close (db), 0);
  }

  TEST_CASE ("Deleting a variable that doesn't exist fails with ERR_VARIABLE_NE")
  {
    int res;

    // Clean re open database
    test_assert_int_equal (ns_cleanup ("test"), 0);
    nsdb_t *db = ns_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i) {
      // Try to delete a non existent variable
      res = ns_exec (db, NULL, "delete var_%d", i);
      test_assert_int_equal (res, ERR_VARIABLE_NE);
    }

    // Close database
    test_assert_int_equal (ns_close (db), 0);
  }

  TEST_CASE ("Deleting the same variable twice fails the second time")
  {
    int res;

    // Clean re open database
    test_assert_int_equal (ns_cleanup ("test"), 0);
    nsdb_t *db = ns_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i) {
      // Create a variable
      res = ns_exec (db, NULL, "create var_%d u32", i);
      test_assert_int_equal (res, 0);

      // Delete a non existent variable
      res = ns_exec (db, NULL, "delete var");
      test_assert_int_equal (res, ERR_VARIABLE_NE);

      // Delete the variable we created
      res = ns_exec (db, NULL, "delete var_%d", i);
      test_assert_int_equal (res, SUCCESS);

      // Delete the variable again - should fail
      res = ns_exec (db, NULL, "delete var_%d", i);
      test_assert_int_equal (res, ERR_VARIABLE_NE);
    }

    // Close database
    test_assert_int_equal (ns_close (db), 0);
  }
}

#endif
