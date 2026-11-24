
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
#include "numstore.h"

#include <string.h>

#ifndef NDEBUG
#  include "ns_testing.h"
#endif

#ifndef NDEBUG
TEST (0020_read_into_partial_element_buffer)
{
  test_assert_int_equal (ns_cleanup ("test"), 0);
  nsdb_t *db = ns_open ("test");
  test_assert (db != NULL);

  const u32 src[4] = {1, 2, 3, 4};
  u8        dest[16];
  test_assert_int_equal (ns_execute (db, NULL, "create foo u32"), 0);
  test_assert_int_equal (ns_write (db, NULL, src, sizeof src, "insert foo 0 4"), 4);

  // Read / remove into a buffer that isn't a whole number of elements
  //
  // A 6 byte buffer holds one u32 and a half. "read foo[0:2]" filled all 6
  // bytes and then failed with ERR_CORRUPT ("read 6 bytes, not a multiple
  // of element size 4"). A remove failed the same way, after it had already
  // taken 6 bytes out of the variable
  //      CAUSE:
  //          nsdb_plan_read gave the algorithms the caller's buffer as it
  //          was. The rope stops when the destination is full, which was
  //          in the middle of an element. The error was only noticed after
  //          the bytes had moved
  //      FIX:
  //          nsdb_plan_read rejects the buffer before anything happens, the
  //          same as nsdb_plan_write does for its source (regression 0019).
  //          It's still an error - but ERR_INVALID_ARGUMENT, with the
  //          buffer and the variable untouched

  TEST_CASE ("Read fails before it writes to the buffer")
  {
    memset (dest, 0xee, sizeof dest);
    test_assert (ns_read (db, NULL, dest, 6, "read foo[0:2]") < 0);
    test_assert (ns_strerror (db, false) != NULL);
    test_assert_int_equal (dest[0], 0xee);

    test_assert (ns_read (db, NULL, dest, 3, "read foo[0:2]") < 0);
    test_assert (ns_strerror (db, false) != NULL);
    test_assert_int_equal (dest[0], 0xee);
  }

  TEST_CASE ("Remove fails before it removes anything")
  {
    // A transaction of the caller's isn't rolled back for it, so the
    // remove itself must not have started
    nsdb_txn_t *tx = ns_begin (db);
    test_assert (tx != NULL);

    memset (dest, 0xee, sizeof dest);
    test_assert (ns_read (db, tx, dest, 6, "remove foo[0:2]") < 0);
    test_assert (ns_strerror (db, false) != NULL);
    test_assert_int_equal (dest[0], 0xee);

    u32 left[4];
    test_assert_int_equal (ns_read (db, tx, left, sizeof left, "read foo"), 4);
    test_assert_memequal (left, src, sizeof src);

    test_assert_int_equal (ns_commit (db, tx), 0);
  }

  TEST_CASE ("A buffer of whole elements is still a short read")
  {
    memset (dest, 0xee, sizeof dest);
    test_assert_int_equal (ns_read (db, NULL, dest, 8, "read foo"), 2);
    test_assert_memequal (dest, src, 8);
    test_assert_int_equal (dest[8], 0xee);
  }

  test_assert_int_equal (ns_close (db), 0);
}
#endif
