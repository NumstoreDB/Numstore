
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

#include <string.h>

#ifndef NDEBUG
#  include "ns_testing.h"
#endif

#ifndef NDEBUG

// foo = [0, 1, ..., 9]
static nsdb_t *
write_stride_open (void)
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

TEST (ns_write_stride)
{
  const u32 src[5] = {100, 101, 102, 103, 104};

  const struct
  {
    const char *query;
    b_size      nsrc; // Elements handed to ns_write
    sb_size     nwritten;
    u32         expected[10];
  } cases[] = {
      {"write foo", 5, 5, {100, 101, 102, 103, 104, 5, 6, 7, 8, 9}},
      {"write foo[5:]", 5, 5, {0, 1, 2, 3, 4, 100, 101, 102, 103, 104}},
      {"write foo[2:4]", 5, 2, {0, 1, 100, 101, 4, 5, 6, 7, 8, 9}},
      {"write foo[0:6:2]", 3, 3, {100, 1, 101, 3, 102, 5, 6, 7, 8, 9}},
      {"write foo[::3]", 5, 4, {100, 1, 2, 101, 4, 5, 102, 7, 8, 103}},
      {"write foo[1::4]", 5, 3, {0, 100, 2, 3, 4, 101, 6, 7, 8, 102}},

      // A bare index is one element
      {"write foo[3]", 5, 1, {0, 1, 2, 100, 4, 5, 6, 7, 8, 9}},
      {"write foo[-1]", 5, 1, {0, 1, 2, 3, 4, 5, 6, 7, 8, 100}},

      // Negative indices
      {"write foo[-3:]", 5, 3, {0, 1, 2, 3, 4, 5, 6, 100, 101, 102}},
      {"write foo[:-8]", 5, 2, {100, 101, 2, 3, 4, 5, 6, 7, 8, 9}},

      // A write never grows the variable
      {"write foo[8:]", 5, 2, {0, 1, 2, 3, 4, 5, 6, 7, 100, 101}},
      {"write foo[8:99]", 5, 2, {0, 1, 2, 3, 4, 5, 6, 7, 100, 101}},
      {"write foo[10:]", 5, 0, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}},
      {"write foo[20:]", 5, 0, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}},

      // An empty range writes nothing
      {"write foo[5:5]", 5, 0, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}},
      {"write foo[7:3]", 5, 0, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}},

      // A write stops at the end of the source
      {"write foo", 2, 2, {100, 101, 2, 3, 4, 5, 6, 7, 8, 9}},
      {"write foo[::2]", 2, 2, {100, 1, 101, 3, 4, 5, 6, 7, 8, 9}},
  };

  for (u32 i = 0; i < arrlen (cases); ++i) {
    TEST_CASE ("%s with %d elements", cases[i].query, (int)cases[i].nsrc)
    {
      nsdb_t *db = write_stride_open ();
      test_assert (db != NULL);

      sb_size
          nwritten = ns_write (db, NULL, src, cases[i].nsrc * sizeof (u32), "%s", cases[i].query);
      test_assert_int_equal (nwritten, cases[i].nwritten);

      u32 dest[11];
      test_assert_int_equal (ns_read (db, NULL, dest, sizeof dest, "read foo"), 10);
      test_assert_memequal (dest, cases[i].expected, sizeof cases[i].expected);

      test_assert_int_equal (ns_close (db), 0);
    }
  }

  TEST_CASE ("A rolled back write restores the old elements")
  {
    nsdb_t *db = write_stride_open ();
    test_assert (db != NULL);

    nsdb_txn_t *tx = ns_begin (db);
    test_assert (tx != NULL);
    test_assert_int_equal (ns_write (db, tx, src, sizeof src, "write foo[::2]"), 5);

    // The transaction sees its own write
    u32 dest[10];
    test_assert_int_equal (ns_read (db, tx, dest, sizeof dest, "read foo[::2]"), 5);
    test_assert_memequal (dest, src, sizeof src);

    test_assert_int_equal (ns_rollback (db, tx), 0);

    test_assert_int_equal (ns_read (db, NULL, dest, sizeof dest, "read foo"), 10);
    for (u32 k = 0; k < 10; ++k) {
      test_assert_int_equal (dest[k], k);
    }

    test_assert_int_equal (ns_close (db), 0);
  }
}

#endif
