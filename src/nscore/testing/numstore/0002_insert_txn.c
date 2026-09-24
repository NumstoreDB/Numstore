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

TEST (numstore_insert_txn)
{
  TEST_CASE ("Committing an insert makes the data persist")
  {
    sb_size res;
    err_t   err;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    // Generate random data to insert
    u32 *src = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      src[i] = (u32)randu32 ();
    }

    // Insert the data inside a transaction and commit it
    struct txn *tx = numstore_begin (db);
    test_assert (tx != NULL);

    res = _numstore_fexecute_simple_with_data (db, tx, src, 0, "insert foo %d %d", 0, ITERS);
    test_assert_int_equal (res, ITERS);

    test_assert_int_equal (numstore_commit (db, tx), 0);

    // The variable should have the new length
    numstore_var_t *var;
    err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get foo");
    test_assert_int_equal (err, SUCCESS);
    test_assert_int_equal (numstore_var_len (var), ITERS);
    numstore_var_free (var);

    // Read the data back and compare
    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    _numstore_fexecute_simple_with_data (db, NULL, dst, 0, "read foo[:]");
    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (dst[i], src[i]);
    }

    // Cleanup
    i_free (mem, src);
    i_free (mem, dst);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Rolling back an insert leaves the variable length unchanged")
  {
    sb_size res;
    err_t   err;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    // The variable starts out empty
    numstore_var_t *var;
    err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get foo");
    test_assert_int_equal (err, SUCCESS);
    test_assert_int_equal (numstore_var_len (var), 0);
    numstore_var_free (var);

    // Generate random data to insert
    u32 *src = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      src[i] = (u32)randu32 ();
    }

    // Insert the data inside a transaction, then roll it back
    struct txn *tx = numstore_begin (db);
    test_assert (tx != NULL);

    res = _numstore_fexecute_simple_with_data (db, tx, src, 0, "insert foo %d %d", 0, ITERS);
    test_assert_int_equal (res, ITERS);

    test_assert_int_equal (numstore_rollback (db, tx), 0);

    // The variable should be empty again
    err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get foo");
    test_assert_int_equal (err, SUCCESS);
    test_assert_int_equal (numstore_var_len (var), 0);
    numstore_var_free (var);

    // Cleanup
    i_free (mem, src);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Rolling back an insert reverts the data to its earlier contents")
  {
    sb_size res;
    err_t   err;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    // Insert the initial data
    u32 *initial = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      initial[i] = (u32)randu32 ();
    }
    res = _numstore_fexecute_simple_with_data (db, NULL, initial, 0, "insert foo %d %d", 0, ITERS);
    test_assert_int_equal (res, ITERS);

    // Generate extra data to insert in a transaction
    u32 *extra = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      extra[i] = (u32)randu32 ();
    }

    // Append the extra data inside a transaction
    struct txn *tx = numstore_begin (db);
    test_assert (tx != NULL);

    res = _numstore_fexecute_simple_with_data (db, tx, extra, 0, "insert foo %d %d", ITERS, ITERS);
    test_assert_int_equal (res, ITERS);

    // Inside the transaction the variable should have both sets of data
    numstore_var_t *var;
    err = _numstore_fexecute_simple_with_var (&var, db, tx, "get foo");
    test_assert_int_equal (err, SUCCESS);
    test_assert_int_equal (numstore_var_len (var), ITERS * 2);
    numstore_var_free (var);

    // Rollback
    test_assert_int_equal (numstore_rollback (db, tx), 0);

    // Only the initial data should remain
    err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get foo");
    test_assert_int_equal (err, SUCCESS);
    test_assert_int_equal (numstore_var_len (var), ITERS);
    numstore_var_free (var);

    // Read the data back and compare to the initial data
    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    _numstore_fexecute_simple_with_data (db, NULL, dst, 0, "read foo[:]");
    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (dst[i], initial[i]);
    }

    // Cleanup
    i_free (mem, initial);
    i_free (mem, extra);
    i_free (mem, dst);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Each single element insert returns one and grows the variable length by one")
  {
    sb_size res;
    err_t   err;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    for (int i = 0; i < ITERS; ++i) {
      // Append one element
      u32 val = (u32)randu32 ();
      res     = _numstore_fexecute_simple_with_data (db, NULL, &val, 0, "insert foo %d %d", i, 1);
      test_assert_int_equal (res, 1);

      // The length should have grown by one
      numstore_var_t *var;
      err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get foo");
      test_assert_int_equal (err, SUCCESS);
      test_assert_int_equal (numstore_var_len (var), i + 1);
      numstore_var_free (var);
    }

    // Close database
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Inserting at the front repeatedly preserves the intended element order")
  {
    sb_size res;
    err_t   err;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    // Generate the values we expect to end up with
    u32 *vals = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      vals[i] = (u32)randu32 ();
    }

    // Insert them one at a time at the front, last value first
    for (int i = ITERS - 1; i >= 0; --i) {
      res = _numstore_fexecute_simple_with_data (db, NULL, &vals[i], 0, "insert foo %d %d", 0, 1);
      test_assert_int_equal (res, 1);
    }

    // The variable should have the full length
    numstore_var_t *var;
    err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get foo");
    test_assert_int_equal (err, SUCCESS);
    test_assert_int_equal (numstore_var_len (var), ITERS);
    numstore_var_free (var);

    // Read the data back and compare
    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    _numstore_fexecute_simple_with_data (db, NULL, dst, 0, "read foo[:]");
    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (dst[i], vals[i]);
    }

    // Cleanup
    i_free (mem, vals);
    i_free (mem, dst);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Rolling back %d inserts one after another leaves the data stable", ITERS)
  {
    sb_size res;
    err_t   err;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    // Insert the initial data
    u32 *initial = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      initial[i] = (u32)randu32 ();
    }
    res = _numstore_fexecute_simple_with_data (db, NULL, initial, 0, "insert foo %d %d", 0, ITERS);
    test_assert_int_equal (res, ITERS);

    // Append one element in a transaction and roll it back, over and over
    u32 *extra = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      extra[i]       = (u32)randu32 ();

      struct txn *tx = numstore_begin (db);
      test_assert (tx != NULL);

      res = _numstore_fexecute_simple_with_data (
          db,
          tx,
          &extra[i],
          0,
          "insert foo %d %d",
          ITERS,
          1
      );
      test_assert_int_equal (res, 1);

      test_assert_int_equal (numstore_rollback (db, tx), 0);

      // The length should be back to the initial length
      numstore_var_t *var;
      err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get foo");
      test_assert_int_equal (err, SUCCESS);
      test_assert_int_equal (numstore_var_len (var), ITERS);
      numstore_var_free (var);
    }

    // Read the data back and compare to the initial data
    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    _numstore_fexecute_simple_with_data (db, NULL, dst, 0, "read foo[:]");
    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (dst[i], initial[i]);
    }

    // Cleanup
    i_free (mem, initial);
    i_free (mem, extra);
    i_free (mem, dst);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Inserting into many variables keeps each variable independent")
  {
    sb_size res;
    err_t   err;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create ITERS variables, each with a single random element
    u32 *vals = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      vals[i] = (u32)randu32 ();

      res     = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create var_%d u32", i);
      test_assert_int_equal (res, 0);

      res = _numstore_fexecute_simple_with_data (
          db,
          NULL,
          &vals[i],
          0,
          "insert var_%d %d %d",
          i,
          0,
          1
      );
      test_assert_int_equal (res, 1);
    }

    // Each variable should hold only its own element
    for (int i = 0; i < ITERS; ++i) {
      // Check the length
      numstore_var_t *var;
      err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get var_%d", i);
      test_assert_int_equal (err, SUCCESS);
      test_assert_int_equal (numstore_var_len (var), 1);
      numstore_var_free (var);

      // Check the data
      u32 dst = 0;
      _numstore_fexecute_simple_with_data (db, NULL, &dst, 0, "read var_%d[:]", i);
      test_assert_int_equal (dst, vals[i]);
    }

    // Cleanup
    i_free (mem, vals);
    test_assert_int_equal (numstore_close (db), 0);
  }
}

#endif
