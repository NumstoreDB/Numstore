
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
#include "ns_utils.h"
#include "numstore.h"

#include <stdlib.h>
#include <string.h>

#ifndef NDEBUG
#  include "ns_testing.h"
#endif

#ifndef NDEBUG

// foo = [0, 1, ..., 9]
static nsdb_t *
remove_stride_open (void)
{
  if (ns_cleanup ("test") != 0) {
    return NULL;
  }
  nsdb_t *db = ns_open ("test");
  if (db == NULL) {
    return NULL;
  }

  u32 src[10];
  for (u32 i = 0; i < 10; ++i) {
    src[i] = i;
  }
  if (ns_execute (db, NULL, "create foo u32") != 0) {
    return NULL;
  }
  if (ns_write (db, NULL, src, sizeof src, "insert foo 0 10") != 10) {
    return NULL;
  }

  return db;
}

TEST (ns_remove_stride)
{
  const struct
  {
    const char *query;
    sb_size     nremoved;
    u32         removed[10];
    u32         left[10];
  } cases[] = {
      {"remove foo", 10, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, {0}},
      {"remove foo[:]", 10, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, {0}},
      {"remove foo[0:3]", 3, {0, 1, 2}, {3, 4, 5, 6, 7, 8, 9}},
      {"remove foo[7:]", 3, {7, 8, 9}, {0, 1, 2, 3, 4, 5, 6}},
      {"remove foo[3:6]", 3, {3, 4, 5}, {0, 1, 2, 6, 7, 8, 9}},
      {"remove foo[0:6:2]", 3, {0, 2, 4}, {1, 3, 5, 6, 7, 8, 9}},
      {"remove foo[::3]", 4, {0, 3, 6, 9}, {1, 2, 4, 5, 7, 8}},
      {"remove foo[1::4]", 3, {1, 5, 9}, {0, 2, 3, 4, 6, 7, 8}},

      // A bare index is one element
      {"remove foo[2]", 1, {2}, {0, 1, 3, 4, 5, 6, 7, 8, 9}},
      {"remove foo[-1]", 1, {9}, {0, 1, 2, 3, 4, 5, 6, 7, 8}},

      // Negative indices
      {"remove foo[-3:]", 3, {7, 8, 9}, {0, 1, 2, 3, 4, 5, 6}},
      {"remove foo[:-8]", 2, {0, 1}, {2, 3, 4, 5, 6, 7, 8, 9}},

      // Out of range clamps
      {"remove foo[8:99]", 2, {8, 9}, {0, 1, 2, 3, 4, 5, 6, 7}},
      {"remove foo[100:]", 0, {0}, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}},

      // An empty range removes nothing
      {"remove foo[5:5]", 0, {0}, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}},
      {"remove foo[7:3]", 0, {0}, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}},
  };

  for (u32 i = 0; i < arrlen (cases); ++i) {
    TEST_CASE ("%s", cases[i].query)
    {
      nsdb_t *db = remove_stride_open ();
      test_assert (db != NULL);

      u32 dest[11];
      memset (dest, 0xEE, sizeof dest);

      sb_size nremoved = ns_read (db, NULL, dest, 10 * sizeof (u32), "%s", cases[i].query);
      test_assert_int_equal (nremoved, cases[i].nremoved);
      test_assert_memequal (dest, cases[i].removed, (size_t)nremoved * sizeof (u32));
      test_assert_int_equal (dest[nremoved], 0xEEEEEEEE);

      // What is left closed the gaps
      sb_size nleft = 10 - cases[i].nremoved;
      test_assert_int_equal (ns_read (db, NULL, dest, sizeof dest, "read foo"), nleft);
      test_assert_memequal (dest, cases[i].left, (size_t)nleft * sizeof (u32));

      nsdb_var_t *var = ns_get_var (db, NULL, "get foo");
      test_assert (var != NULL);
      test_assert_int_equal (ns_var_len (var), nleft);
      ns_var_free (var);

      test_assert_int_equal (ns_close (db), 0);
    }
  }

  TEST_CASE ("ns_execute removes without handing the elements back")
  {
    nsdb_t *db = remove_stride_open ();
    test_assert (db != NULL);

    // Returns the number of elements removed
    test_assert_int_equal (ns_execute (db, NULL, "remove foo[0:6:2]"), 3);

    u32       dest[10];
    const u32 left[7] = {1, 3, 5, 6, 7, 8, 9};
    test_assert_int_equal (ns_read (db, NULL, dest, sizeof dest, "read foo"), 7);
    test_assert_memequal (dest, left, sizeof left);

    test_assert_int_equal (ns_close (db), 0);
  }

  TEST_CASE ("A remove stops at the end of the destination")
  {
    nsdb_t *db = remove_stride_open ();
    test_assert (db != NULL);

    u32 dest[3] = {0xEEEEEEEE, 0xEEEEEEEE, 0xEEEEEEEE};
    test_assert_int_equal (ns_read (db, NULL, dest, 2 * sizeof (u32), "remove foo"), 2);
    test_assert_int_equal (dest[0], 0);
    test_assert_int_equal (dest[1], 1);
    test_assert_int_equal (dest[2], 0xEEEEEEEE);

    // Only what fit was removed
    nsdb_var_t *var = ns_get_var (db, NULL, "get foo");
    test_assert (var != NULL);
    test_assert_int_equal (ns_var_len (var), 8);
    ns_var_free (var);

    test_assert_int_equal (ns_close (db), 0);
  }

  TEST_CASE ("ns_malloc removes and allocates the result")
  {
    nsdb_t *db = remove_stride_open ();
    test_assert (db != NULL);

    b_size blen = 0;
    u32   *data = ns_malloc (db, NULL, &blen, "remove foo[::3]");
    test_assert (data != NULL);
    test_assert_int_equal (blen, 4 * sizeof (u32));
    test_assert_int_equal (data[0], 0);
    test_assert_int_equal (data[3], 9);
    free (data);

    nsdb_var_t *var = ns_get_var (db, NULL, "get foo");
    test_assert (var != NULL);
    test_assert_int_equal (ns_var_len (var), 6);
    ns_var_free (var);

    test_assert_int_equal (ns_close (db), 0);
  }

  TEST_CASE ("A rolled back remove puts the elements back")
  {
    nsdb_t *db = remove_stride_open ();
    test_assert (db != NULL);

    nsdb_txn_t *tx = ns_begin (db);
    test_assert (tx != NULL);

    u32 dest[10];
    test_assert_int_equal (ns_read (db, tx, dest, sizeof dest, "remove foo[1::2]"), 5);
    test_assert_int_equal (ns_read (db, tx, dest, sizeof dest, "read foo"), 5);
    test_assert_int_equal (ns_rollback (db, tx), 0);

    test_assert_int_equal (ns_read (db, NULL, dest, sizeof dest, "read foo"), 10);
    for (u32 k = 0; k < 10; ++k) {
      test_assert_int_equal (dest[k], k);
    }

    test_assert_int_equal (ns_close (db), 0);
  }
}

#endif
