
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
#include "core/ns_stdtypes.h"
#include "core/os/ns_filesystem.h"
#include "core/os/ns_memory.h"
#include "nscore/algorithms/numstore/ns_numstore_algorithms.h"
#include "nscore/nsdb/ns_nsdb.h"
#include "nscore/types/ns_types.h"
#include "nscore/variables/ns_variables.h"
#include "numstore/ns_numstore_internal.h"
#include "numstore/numstore.h"

#ifdef TESTING
#  include "core/testing/ns_testing.h"
#endif

#ifdef TESTING

TEST (numstore_delete_txn)
{
  TEST_CASE ("Rolling back a delete lets the variable be deleted again")
  {
    sb_size res;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    // Delete the variable inside a transaction, then roll it back
    struct txn *tx = numstore_begin (db);
    test_assert (tx != NULL);

    res = _numstore_fexecute_simple_with_data (db, tx, NULL, 0, "delete foo");
    test_assert_int_equal (res, 0);

    test_assert_int_equal (numstore_rollback (db, tx), 0);

    // Delete the variable again, outside of a transaction
    res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "delete foo");
    test_assert_int_equal (res, 0);

    // Close database
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Deleting a variable inside a transaction makes it no longer visible")
  {
    sb_size res;
    err_t   err;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create ITERS variables
    for (int i = 0; i < ITERS; ++i) {
      res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create var_%d u32", i);
      test_assert_int_equal (res, 0);
    }

    // Delete a variable in a transaction, rolling back each time
    for (int i = 0; i < ITERS; ++i) {
      // Begin transaction
      struct txn *tx = numstore_begin (db);
      test_assert (tx != NULL);

      // Delete the variable
      res = _numstore_fexecute_simple_with_data (db, tx, NULL, 0, "delete var");
      test_assert_int_equal (res, ERR_VARIABLE_NE);
      numstore_perror (db, "delete var");

      // Get the variable - should be non existent
      numstore_var_t *var;
      err = _numstore_fexecute_simple_with_var (&var, db, tx, "get var");
      test_assert (err != SUCCESS);
      numstore_perror (db, "delete var");

      // Rollback transaction
      test_assert_int_equal (numstore_rollback (db, tx), 0);
    }

    // Close database
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Rolling back a delete keeps both the variable and its data intact")
  {
    sb_size res;
    err_t   err;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create one variable
    res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    // Insert an array into the variable
    u32 src[ITERS];
    for (int i = 0; i < ITERS; ++i) {
      src[i] = (u32)randu32 ();
    }
    res = _numstore_fexecute_simple_with_data (db, NULL, src, 0, "insert foo %d %d", 0, ITERS);
    test_assert_int_equal (res, ITERS);

    for (int i = 0; i < ITERS; ++i) {
      // Delete the variable inside a transaction, then roll it back
      {
        struct txn *tx = numstore_begin (db);
        test_assert (tx != NULL);

        res = _numstore_fexecute_simple_with_data (db, tx, NULL, 0, "delete foo");
        test_assert_int_equal (res, 0);

        test_assert_int_equal (numstore_rollback (db, tx), 0);
      }

      // The variable should still exist with the same length
      {
        numstore_var_t *var;
        err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get foo");
        test_assert_int_equal (err, SUCCESS);
        test_assert_int_equal (numstore_var_len (var), ITERS);
        numstore_var_free (var);
      }

      // The data should be unchanged
      {
        u32 dst[ITERS];
        res = _numstore_fexecute_simple_with_data (db, NULL, dst, 0, "read foo[:]");
        test_assert_int_equal (res, ITERS);
        for (int j = 0; j < ITERS; ++j) {
          test_assert_int_equal (dst[j], src[j]);
        }
      }
    }

    // Close database
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Deleting a variable that doesn't exist fails with ERR_VARIABLE_NE")
  {
    sb_size res;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i) {
      // Try to delete a non existent variable
      res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "delete var_%d", i);
      test_assert_int_equal (res, ERR_VARIABLE_NE);
      numstore_perror (db, "delete");
    }

    // Close database
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Deleting the same variable twice fails the second time")
  {
    sb_size res;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i) {
      // Create a variable
      res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create var_%d u32", i);
      test_assert_int_equal (res, 0);

      // Delete a non existent variable
      res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "delete var");
      test_assert_int_equal (res, ERR_VARIABLE_NE);
      numstore_perror (db, "delete");

      // Delete the variable we created
      res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "delete var_%d");
      test_assert_int_equal (res, SUCCESS);

      // Delete the variable again - should fail
      res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "delete var_%d");
      test_assert_int_equal (res, ERR_VARIABLE_NE);
      numstore_perror (db, "delete");
    }

    // Close database
    test_assert_int_equal (numstore_close (db), 0);
  }
}
#endif
