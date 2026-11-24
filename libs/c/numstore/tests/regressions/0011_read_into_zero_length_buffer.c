
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

#ifndef NDEBUG
#  include "ns_testing.h"
#endif

#ifndef NDEBUG
TEST (0011_read_into_zero_length_buffer)
{
  test_assert_int_equal (ns_cleanup ("test"), 0);
  nsdb_t *db = ns_open ("test");
  test_assert (db != NULL);

  u32 src[4] = {0, 1, 2, 3};
  u32 dest[4];
  test_assert_int_equal (ns_execute (db, NULL, "create foo u32"), 0);
  test_assert_int_equal (ns_write (db, NULL, src, sizeof src, "insert foo 0 4"), 4);

  // Read / write with an empty buffer
  //
  // These crashed on an assert instead of returning an error
  //      CAUSE:
  //          nsdb_read / nsdb_write asserted on [dest] and [dlen], which
  //          come straight from the caller. nsdb_plan_read / nsdb_plan_write
  //          already validate them
  test_assert (ns_read (db, NULL, dest, 0, "read foo") < 0);
  test_assert (ns_strerror (db, false) != NULL);
  test_assert (ns_write (db, NULL, src, 0, "insert foo 0 4") < 0);
  test_assert (ns_strerror (db, false) != NULL);

  // A remove doesn't need a destination - the plan api already allowed this
  test_assert_int_equal (ns_read (db, NULL, NULL, 0, "remove foo[0:2]"), 2);

  // Nothing above changed more than the remove did
  test_assert_int_equal (ns_read (db, NULL, dest, sizeof dest, "read foo"), 2);
  test_assert_int_equal (dest[0], 2);
  test_assert_int_equal (dest[1], 3);

  test_assert_int_equal (ns_close (db), 0);
}
#endif
