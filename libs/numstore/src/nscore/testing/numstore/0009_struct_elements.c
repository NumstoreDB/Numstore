
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
#include "numstore.h"

#include <stdlib.h>
#include <string.h>

#ifndef NDEBUG
#  include "core/ns_testing.h"
#endif

#ifndef NDEBUG

// struct { a u8, b [2]u16, c f64 } - packed, 13 bytes
enum
{
  STRUCT_ELEM_SIZE = 13,
  STRUCT_ELEM_LEN  = 2000,
};

TEST (ns_struct_elements)
{
  static u8 src[STRUCT_ELEM_LEN * STRUCT_ELEM_SIZE];
  static u8 dest[STRUCT_ELEM_LEN * STRUCT_ELEM_SIZE];
  for (u32 i = 0; i < sizeof src; ++i) {
    src[i] = (u8)(i * 31 + 7);
  }

  test_assert_int_equal (ns_cleanup ("test"), 0);
  nsdb_t *db = ns_open ("test");
  test_assert (db != NULL);

  test_assert_int_equal (ns_execute (db, NULL, "create p struct { a u8, b [2]u16, c f64 }"), 0);

  TEST_CASE ("Lengths are in elements, not bytes")
  {
    test_assert_int_equal (
        ns_write (db, NULL, src, sizeof src, "insert p 0 %d", STRUCT_ELEM_LEN),
        STRUCT_ELEM_LEN
    );

    nsdb_var_t *var = ns_get_var (db, NULL, "get p");
    test_assert (var != NULL);
    test_assert_int_equal (ns_var_len (var), STRUCT_ELEM_LEN);
    ns_var_free (var);
  }

  TEST_CASE ("Elements come back whole")
  {
    test_assert_int_equal (ns_read (db, NULL, dest, sizeof dest, "read p"), STRUCT_ELEM_LEN);
    test_assert_memequal (dest, src, sizeof src);

    // One element from the middle
    test_assert_int_equal (ns_read (db, NULL, dest, STRUCT_ELEM_SIZE, "read p[1234]"), 1);
    test_assert_memequal (dest, &src[1234 * STRUCT_ELEM_SIZE], STRUCT_ELEM_SIZE);
  }

  TEST_CASE ("A stride steps over whole elements")
  {
    // Elements 3, 503, 1003, 1503
    test_assert_int_equal (ns_read (db, NULL, dest, sizeof dest, "read p[3::500]"), 4);
    for (u32 k = 0; k < 4; ++k) {
      test_assert_memequal (
          &dest[k * STRUCT_ELEM_SIZE],
          &src[(3 + k * 500) * STRUCT_ELEM_SIZE],
          STRUCT_ELEM_SIZE
      );
    }
  }

  TEST_CASE ("A destination with room for part of an element is an error")
  {
    test_assert (ns_read (db, NULL, dest, STRUCT_ELEM_SIZE + 1, "read p") < 0);
  }

  TEST_CASE ("Write and remove whole elements")
  {
    u8 elem[STRUCT_ELEM_SIZE];
    memset (elem, 0xAB, sizeof elem);

    test_assert_int_equal (ns_write (db, NULL, elem, sizeof elem, "write p[10]"), 1);
    test_assert_int_equal (ns_read (db, NULL, dest, sizeof dest, "remove p[9:12]"), 3);
    test_assert_memequal (&dest[0], &src[9 * STRUCT_ELEM_SIZE], STRUCT_ELEM_SIZE);
    test_assert_memequal (&dest[STRUCT_ELEM_SIZE], elem, STRUCT_ELEM_SIZE);
    test_assert_memequal (
        &dest[2 * STRUCT_ELEM_SIZE],
        &src[11 * STRUCT_ELEM_SIZE],
        STRUCT_ELEM_SIZE
    );

    // Element 9 is now what was element 12
    test_assert_int_equal (ns_read (db, NULL, dest, STRUCT_ELEM_SIZE, "read p[9]"), 1);
    test_assert_memequal (dest, &src[12 * STRUCT_ELEM_SIZE], STRUCT_ELEM_SIZE);
  }

  test_assert_int_equal (ns_close (db), 0);
}

#endif
