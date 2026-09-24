
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

TEST (numstore_write_txn)
{
  TEST_CASE ("Committing a write makes the new data persist")
  {
    sb_size res;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    // Generate the initial data and the data we will write over it
    u32 *initial = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    u32 *patch   = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      initial[i] = (u32)randu32 ();
      patch[i]   = (u32)randu32 ();
    }

    // Insert the initial data
    res = _numstore_fexecute_simple_with_data (db, NULL, initial, 0, "insert foo %d %d", 0, ITERS);
    test_assert_int_equal (res, ITERS);

    // Overwrite everything inside a transaction and commit it
    struct txn *tx = numstore_begin (db);
    test_assert (tx != NULL);

    _numstore_fexecute_simple_with_data (db, tx, patch, 0, "write foo[0:%d:1]", ITERS);

    test_assert_int_equal (numstore_commit (db, tx), 0);

    // Read the data back and compare to the patch
    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    _numstore_fexecute_simple_with_data (db, NULL, dst, 0, "read foo[:]");
    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (dst[i], patch[i]);
    }

    // Cleanup
    i_free (mem, initial);
    i_free (mem, patch);
    i_free (mem, dst);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Rolling back a write reverts the data to its earlier contents")
  {
    sb_size res;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    // Generate the initial data and the data we will write over it
    u32 *initial = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    u32 *patch   = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      initial[i] = (u32)randu32 ();
      patch[i]   = (u32)randu32 ();
    }

    // Insert the initial data
    res = _numstore_fexecute_simple_with_data (db, NULL, initial, 0, "insert foo %d %d", 0, ITERS);
    test_assert_int_equal (res, ITERS);

    // Overwrite everything inside a transaction, then roll it back
    struct txn *tx = numstore_begin (db);
    test_assert (tx != NULL);

    _numstore_fexecute_simple_with_data (db, tx, patch, 0, "write foo[0:%d:1]", ITERS);

    test_assert_int_equal (numstore_rollback (db, tx), 0);

    // Read the data back and compare to the initial data
    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    _numstore_fexecute_simple_with_data (db, NULL, dst, 0, "read foo[:]");
    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (dst[i], initial[i]);
    }

    // Cleanup
    i_free (mem, initial);
    i_free (mem, patch);
    i_free (mem, dst);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Writing to a variable does not change its length")
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
    u32 *data = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      data[i] = (u32)randu32 ();
    }
    res = _numstore_fexecute_simple_with_data (db, NULL, data, 0, "insert foo %d %d", 0, ITERS);
    test_assert_int_equal (res, ITERS);
    i_free (mem, data);

    // Overwrite one element at a time
    for (int i = 0; i < ITERS; ++i) {
      u32 val = (u32)randu32 ();
      _numstore_fexecute_simple_with_data (db, NULL, &val, 0, "write foo[%d:%d:1]", i, i + 1);

      // The length should not have changed
      numstore_var_t *var;
      err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get foo");
      test_assert_int_equal (err, SUCCESS);
      test_assert_int_equal (numstore_var_len (var), ITERS);
      numstore_var_free (var);
    }

    // Close database
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Writing a single element leaves all other elements unchanged")
  {
    sb_size res;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    // Insert the initial data - shadow tracks what the variable should contain
    u32 *shadow = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      shadow[i] = (u32)randu32 ();
    }
    res = _numstore_fexecute_simple_with_data (db, NULL, shadow, 0, "insert foo %d %d", 0, ITERS);
    test_assert_int_equal (res, ITERS);

    // Overwrite a random element, then compare everything against the shadow
    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      int idx     = randu32 () % ITERS;
      u32 val     = (u32)randu32 ();
      shadow[idx] = val;

      // Write the element
      _numstore_fexecute_simple_with_data (db, NULL, &val, 0, "write foo[%d:%d:1]", idx, idx + 1);

      // Read everything back and compare
      _numstore_fexecute_simple_with_data (db, NULL, dst, 0, "read foo[:]");
      for (int j = 0; j < ITERS; ++j) {
        test_assert_int_equal (dst[j], shadow[j]);
      }
    }

    // Cleanup
    i_free (mem, shadow);
    i_free (mem, dst);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Rolling back %d random single element writes leaves the data stable", ITERS)
  {
    sb_size res;

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

    // Overwrite a random element in a transaction and roll it back, over and over
    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      int         idx = randu32 () % ITERS;
      u32         val = (u32)randu32 ();

      // Write and rollback
      struct txn *tx  = numstore_begin (db);
      test_assert (tx != NULL);

      _numstore_fexecute_simple_with_data (db, tx, &val, 0, "write foo[%d:%d:1]", idx, idx + 1);

      test_assert_int_equal (numstore_rollback (db, tx), 0);

      // Everything should still match the initial data
      _numstore_fexecute_simple_with_data (db, NULL, dst, 0, "read foo[:]");
      for (int j = 0; j < ITERS; ++j) {
        test_assert_int_equal (dst[j], initial[j]);
      }
    }

    // Cleanup
    i_free (mem, initial);
    i_free (mem, dst);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Committed writes persist across %d database reopens", REOPEN_ITERS)
  {
    sb_size res;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    // Insert zeroed data - data tracks what the variable should contain
    u32 *data = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      data[i] = 0;
    }
    res = _numstore_fexecute_simple_with_data (db, NULL, data, 0, "insert foo %d %d", 0, ITERS);
    test_assert_int_equal (res, ITERS);

    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < REOPEN_ITERS; ++i) {
      u32 val        = (u32)randu32 ();
      int idx        = randu32 () % ITERS;
      data[idx]      = val;

      // Write a random element in a transaction and commit it
      struct txn *tx = numstore_begin (db);
      test_assert (tx != NULL);

      _numstore_fexecute_simple_with_data (db, tx, &val, 0, "write foo[%d:%d:1]", idx, idx + 1);

      test_assert_int_equal (numstore_commit (db, tx), 0);

      // Close and re open the database
      test_assert_int_equal (numstore_close (db), 0);
      db = numstore_open ("test");
      test_assert (db != NULL);

      // Everything should still match after the reopen
      _numstore_fexecute_simple_with_data (db, NULL, dst, 0, "read foo[:]");
      for (int j = 0; j < ITERS; ++j) {
        test_assert_int_equal (dst[j], data[j]);
      }
    }

    // Cleanup
    i_free (mem, data);
    i_free (mem, dst);
    test_assert_int_equal (numstore_close (db), 0);
  }
}

#endif
