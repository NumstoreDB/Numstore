
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
static b_size
len_of_foo (nsdb_t *db)
{
  nsdb_var_t *var = ns_get_var (db, NULL, "get foo");
  if (var == NULL) {
    return (b_size)-1;
  }
  const b_size ret = ns_var_len (var);
  ns_var_free (var);
  return ret;
}

TEST (0019_insert_short_source_buffer)
{
  test_assert_int_equal (ns_cleanup ("test"), 0);
  nsdb_t *db = ns_open ("test");
  test_assert (db != NULL);

  const u32 src[4] = {1, 2, 3, 4};
  u32       dest[16];
  test_assert_int_equal (ns_execute (db, NULL, "create foo u32"), 0);

  // An insert whose source buffer holds fewer elements than the query asks
  //
  // "insert foo 0 10" with a 4 element buffer inserted the 4 elements and
  // returned 4, but the variable's length went up by 10. From then on
  // ns_var_len said 10 and a read returned 4
  //      CAUSE:
  //          numstore_insert added the length from the query to the
  //          variable (len * tsize) instead of what ns_insert took from
  //          the source. With an element size that doesn't divide the
  //          buffer (a 6 byte buffer for u32) it inserted half an element
  //          and hit ASSERT (ret % tsize == 0)
  //      FIX:
  //          numstore_insert grows the variable by what was inserted, so a
  //          short buffer is a short insert, the same as a short write.
  //          nsdb_plan_write rejects a buffer that isn't a whole number of
  //          elements before anything changes

  TEST_CASE ("The variable grows by what was inserted")
  {
    test_assert_int_equal (ns_write (db, NULL, src, sizeof src, "insert foo 0 10"), 4);
    test_assert_type_equal (len_of_foo (db), (b_size)4, b_size, PRb_size);
    test_assert_int_equal (ns_read (db, NULL, dest, sizeof dest, "read foo"), 4);
    test_assert_memequal (dest, src, sizeof src);
  }

  TEST_CASE ("A source that isn't a whole number of elements is an error")
  {
    // 6 bytes is one u32 and a half
    test_assert (ns_write (db, NULL, src, 6, "insert foo 4 2") < 0);
    test_assert (ns_strerror (db, false) != NULL);
    test_assert (ns_write (db, NULL, src, 6, "write foo[0:2]") < 0);
    test_assert (ns_strerror (db, false) != NULL);
    test_assert (ns_write (db, NULL, src, 3, "insert foo 0 1") < 0);
    test_assert (ns_strerror (db, false) != NULL);

    // And nothing changed
    test_assert_type_equal (len_of_foo (db), (b_size)4, b_size, PRb_size);
    test_assert_int_equal (ns_read (db, NULL, dest, sizeof dest, "read foo"), 4);
    test_assert_memequal (dest, src, sizeof src);
  }

  // "insert foo 0 -1" - the length is unsigned inside, so -1 was a huge
  // length and the insert took the whole buffer
  TEST_CASE ("A negative length is a syntax error")
  {
    test_assert (ns_write (db, NULL, src, sizeof src, "insert foo 0 -1") < 0);
    test_assert (ns_strerror (db, false) != NULL);
    test_assert_type_equal (len_of_foo (db), (b_size)4, b_size, PRb_size);
  }

  TEST_CASE ("The length is still right after a reopen")
  {
    test_assert_int_equal (ns_close (db), 0);
    db = ns_open ("test");
    test_assert (db != NULL);

    test_assert_type_equal (len_of_foo (db), (b_size)4, b_size, PRb_size);
    test_assert_int_equal (ns_read (db, NULL, dest, sizeof dest, "read foo"), 4);
    test_assert_memequal (dest, src, sizeof src);
  }

  test_assert_int_equal (ns_close (db), 0);
}
#endif
