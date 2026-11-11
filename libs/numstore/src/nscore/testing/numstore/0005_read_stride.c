
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

#include "core/ns_stdtypes.h"
#include "core/ns_utils.h"
#include "numstore.h"

#include <stdlib.h>
#include <string.h>

#ifndef NDEBUG
#  include "core/ns_testing.h"
#endif

#ifndef NDEBUG

// The python slice [start:stop:step] of an array of [len] elements
// Returns the number of elements and sets [first]
static i64
read_stride_expect (i64 len, bool hs, i64 start, bool he, i64 stop, i64 step, i64 *first)
{
  if (!hs) {
    start = 0;
  } else {
    if (start < 0) {
      start += len;
    }
    start = start < 0 ? 0 : (start > len ? len : start);
  }

  if (!he) {
    stop = len;
  } else {
    if (stop < 0) {
      stop += len;
    }
    stop = stop < 0 ? 0 : (stop > len ? len : stop);
  }

  *first = start;
  return stop <= start ? 0 : (stop - start + step - 1) / step;
}

TEST (ns_read_stride)
{
  enum
  {
    LEN = 20,
  };

  // foo = [0, 1, ..., 19]
  test_assert_int_equal (ns_cleanup ("test"), 0);
  nsdb_t *db = ns_open ("test");
  test_assert (db != NULL);

  u32 src[LEN];
  for (u32 i = 0; i < LEN; ++i) {
    src[i] = i;
  }
  test_assert_int_equal (ns_execute (db, NULL, "create foo u32"), 0);
  test_assert_int_equal (ns_write (db, NULL, src, sizeof src, "insert foo 0 %d", LEN), LEN);

  const struct
  {
    const char *range;
    bool        hs;
    i64         start;
    bool        he;
    i64         stop;
    i64         step;
  } cases[] = {
      // Whole array
      {"", false, 0, false, 0, 1},
      {"[:]", false, 0, false, 0, 1},
      {"[::]", false, 0, false, 0, 1},
      {"[0:]", true, 0, false, 0, 1},
      {"[0:20]", true, 0, true, 20, 1},

      // Start and stop
      {"[5:]", true, 5, false, 0, 1},
      {"[:5]", false, 0, true, 5, 1},
      {"[2:9]", true, 2, true, 9, 1},
      {"[19:]", true, 19, false, 0, 1},
      {"[0:1]", true, 0, true, 1, 1},

      // Steps
      {"[::2]", false, 0, false, 0, 2},
      {"[0:20:3]", true, 0, true, 20, 3},
      {"[1:19:7]", true, 1, true, 19, 7},
      {"[::19]", false, 0, false, 0, 19},
      {"[::20]", false, 0, false, 0, 20},
      {"[::100]", false, 0, false, 0, 100},
      {"[3:4:100]", true, 3, true, 4, 100},

      // Negative indices count from the end
      {"[-3:]", true, -3, false, 0, 1},
      {"[:-3]", false, 0, true, -3, 1},
      {"[-5:-1]", true, -5, true, -1, 1},
      {"[0:-10:3]", true, 0, true, -10, 3},

      // Out of range clamps
      {"[0:99]", true, 0, true, 99, 1},
      {"[-99:]", true, -99, false, 0, 1},
      {"[-99:99:4]", true, -99, true, 99, 4},

      // Empty ranges
      {"[20:]", true, 20, false, 0, 1},
      {"[25:]", true, 25, false, 0, 1},
      {"[5:5]", true, 5, true, 5, 1},
      {"[7:3]", true, 7, true, 3, 1},
      {"[:0]", false, 0, true, 0, 1},
      {"[:-20]", false, 0, true, -20, 1},
  };

  for (u32 i = 0; i < arrlen (cases); ++i) {
    TEST_CASE ("read foo%s", cases[i].range)
    {
      i64 first;
      i64 nelems = read_stride_expect (
          LEN,
          cases[i].hs,
          cases[i].start,
          cases[i].he,
          cases[i].stop,
          cases[i].step,
          &first
      );

      u32 dest[LEN + 1];
      memset (dest, 0xEE, sizeof dest);

      sb_size nread = ns_read (db, NULL, dest, LEN * sizeof (u32), "read foo%s", cases[i].range);
      test_assert_int_equal (nread, nelems);
      for (i64 k = 0; k < nelems; ++k) {
        test_assert_int_equal (dest[k], first + k * cases[i].step);
      }

      // Nothing past the last element read was touched
      test_assert_int_equal (dest[nelems], 0xEEEEEEEE);
    }
  }

  TEST_CASE ("A bare index is one element")
  {
    u32 dest[2] = {0xEEEEEEEE, 0xEEEEEEEE};

    test_assert_int_equal (ns_read (db, NULL, dest, sizeof dest, "read foo[3]"), 1);
    test_assert_int_equal (dest[0], 3);
    test_assert_int_equal (dest[1], 0xEEEEEEEE);

    test_assert_int_equal (ns_read (db, NULL, dest, sizeof dest, "read foo[-1]"), 1);
    test_assert_int_equal (dest[0], 19);

    test_assert_int_equal (ns_read (db, NULL, dest, sizeof dest, "read foo[20]"), 0);
  }

  TEST_CASE ("A read stops at the end of the destination")
  {
    u32 dest[5];
    memset (dest, 0xEE, sizeof dest);

    // Room for 4 of the 20 elements
    test_assert_int_equal (ns_read (db, NULL, dest, 4 * sizeof (u32), "read foo"), 4);
    test_assert_int_equal (dest[3], 3);
    test_assert_int_equal (dest[4], 0xEEEEEEEE);

    // Same with a stride
    test_assert_int_equal (ns_read (db, NULL, dest, 4 * sizeof (u32), "read foo[1::3]"), 4);
    test_assert_int_equal (dest[0], 1);
    test_assert_int_equal (dest[3], 10);
    test_assert_int_equal (dest[4], 0xEEEEEEEE);
  }

  TEST_CASE ("A destination that isn't a whole number of elements is an error")
  {
    u32 dest[2];
    test_assert (ns_read (db, NULL, dest, 6, "read foo") < 0);
    test_assert (ns_strerror (db) != NULL);
  }

  TEST_CASE ("A step that isn't positive is an error")
  {
    u32 dest[LEN];
    test_assert (ns_read (db, NULL, dest, sizeof dest, "read foo[::0]") < 0);
    test_assert (ns_read (db, NULL, dest, sizeof dest, "read foo[::-1]") < 0);
    test_assert (ns_read (db, NULL, dest, sizeof dest, "read foo[10:0:-2]") < 0);
  }

  TEST_CASE ("ns_malloc returns the same elements as ns_read")
  {
    b_size blen = 0;
    u32   *data = ns_malloc (db, NULL, &blen, "read foo[1:19:7]");
    test_assert (data != NULL);

    // ns_malloc reports bytes, ns_read reports elements
    test_assert_int_equal (blen, 3 * sizeof (u32));
    test_assert_int_equal (data[0], 1);
    test_assert_int_equal (data[1], 8);
    test_assert_int_equal (data[2], 15);
    free (data);

    // An empty range has nothing to allocate
    blen = 77;
    data = ns_malloc (db, NULL, &blen, "read foo[5:5]");
    test_assert (data == NULL);
    test_assert_int_equal (blen, 0);
    test_assert (ns_strerror (db) == NULL);
  }

  test_assert_int_equal (ns_close (db), 0);
}

#endif
