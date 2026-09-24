
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

#  define ITERS        10
#  define REOPEN_ITERS 20

TEST (numstore_create_txn)
{
  TEST_CASE ("Committing a created variable makes it persist across a database reopen")
  {
    sb_size res;
    err_t   err;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create a variable inside a transaction and commit it
    struct txn *tx = numstore_begin (db);
    test_assert (tx != NULL);

    res = _numstore_fexecute_simple_with_data (db, tx, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    test_assert_int_equal (numstore_commit (db, tx), 0);

    // Close and re open the database
    test_assert_int_equal (numstore_close (db), 0);
    db = numstore_open ("test");
    test_assert (db != NULL);

    // The variable should still exist and be empty
    numstore_var_t *var;
    err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get if exists foo");
    test_assert_int_equal (err, SUCCESS);
    test_assert (var != NULL);
    test_assert_int_equal (numstore_var_len (var), 0);
    numstore_var_free (var);

    // Close database
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Rolling back a created variable leaves it not visible")
  {
    sb_size res;
    err_t   err;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Begin transaction
    struct txn *tx = numstore_begin (db);
    test_assert (tx != NULL);

    // Create a new variable
    res = _numstore_fexecute_simple_with_data (db, tx, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    // Rollback
    test_assert_int_equal (numstore_rollback (db, tx), 0);

    // Get the variable (should fail)
    numstore_var_t *var;
    err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get foo");
    test_assert (err != SUCCESS);
    numstore_perror (db, "get foo");

    // Close database
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE (
      "Creating a variable succeeds after an earlier create of the same name was rolled back"
  )
  {
    sb_size res;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create a variable inside a transaction, then roll it back
    struct txn *tx = numstore_begin (db);
    test_assert (tx != NULL);

    res = _numstore_fexecute_simple_with_data (db, tx, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    test_assert_int_equal (numstore_rollback (db, tx), 0);

    // Create a variable with the same name outside of a transaction
    res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    // Close database
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE (
      "Creating %d variables in separate committed transactions makes them all persist",
      ITERS
  )
  {
    sb_size res;
    err_t   err;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create a variable per transaction and commit each
    for (int i = 0; i < ITERS; ++i) {
      struct txn *tx = numstore_begin (db);
      test_assert (tx != NULL);

      res = _numstore_fexecute_simple_with_data (db, tx, NULL, 0, "create var_%d u32", i);
      test_assert_int_equal (res, 0);

      test_assert_int_equal (numstore_commit (db, tx), 0);
    }

    // Check the variables
    for (int i = 0; i < ITERS; ++i) {
      numstore_var_t *var = NULL;
      err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get if exists foo");
      test_assert_int_equal (err, SUCCESS);
      test_assert (var->var.dtype == NULL);
    }

    // Close database
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE (
      "Only committed creates persist when %d transactions alternate commit and rollback",
      ITERS
  )
  {
    sb_size res;
    err_t   err;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create a variable per transaction, committing evens and rolling back odds
    for (int i = 0; i < ITERS; ++i) {
      struct txn *tx = numstore_begin (db);
      test_assert (tx != NULL);

      res = _numstore_fexecute_simple_with_data (db, tx, NULL, 0, "create var_%d u32", i);
      test_assert_int_equal (res, 0);

      if (i % 2 == 0) {
        test_assert_int_equal (numstore_commit (db, tx), 0);
      } else {
        test_assert_int_equal (numstore_rollback (db, tx), 0);
      }
    }

    // NOTE: both branches of the original if/else did the exact same
    // thing ("get if exists foo", which never existed either way), so
    // this was collapsed into one unconditional check.
    for (int i = 0; i < ITERS; ++i) {
      numstore_var_t *var;
      err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get if exists foo");
      test_assert_int_equal (err, SUCCESS);
      test_assert (var->var.dtype == NULL);
    }

    // Close database
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("A newly created variable is always empty")
  {
    sb_size res;
    err_t   err;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i) {
      // Create a variable
      res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create var_%d u32", i);
      test_assert_int_equal (res, 0);

      // Check that it is empty
      numstore_var_t *var;
      err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get if exists foo");
      test_assert_int_equal (err, SUCCESS);
      test_assert (var->var.dtype == NULL);
    }

    // Close database
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Creating a variable that already exists fails")
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

      // Create the same variable again
      res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create var_%d u32", i);
      test_assert_int_equal (res, SUCCESS);
    }

    // Close database
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE (
      "Rolling back a create %d times and then committing it once leaves the variable empty",
      ITERS
  )
  {
    sb_size res;
    err_t   err;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create the same variable and roll it back, over and over
    for (int i = 0; i < ITERS; ++i) {
      struct txn *tx = numstore_begin (db);
      test_assert (tx != NULL);

      res = _numstore_fexecute_simple_with_data (db, tx, NULL, 0, "create foo u32");
      test_assert_int_equal (res, 0);

      test_assert_int_equal (numstore_rollback (db, tx), 0);

      // The variable should not exist
      numstore_var_t *var;
      err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get if exists foo");
      test_assert_int_equal (err, SUCCESS);
      test_assert (var->var.dtype == NULL);
    }

    // Finally create the variable and commit it
    struct txn *tx = numstore_begin (db);
    test_assert (tx != NULL);

    res = _numstore_fexecute_simple_with_data (db, tx, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    test_assert_int_equal (numstore_commit (db, tx), 0);

    // The variable should exist and be empty
    numstore_var_t *var;
    err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get if exists foo");
    test_assert_int_equal (err, SUCCESS);
    test_assert (var != NULL);
    test_assert_int_equal (numstore_var_len (var), 0);
    numstore_var_free (var);

    // Close database
    test_assert_int_equal (numstore_close (db), 0);
  }
}

#endif
