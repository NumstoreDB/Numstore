
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
TEST (0018_smfile_stride_past_end)
{
  // 18 bytes: 0 1 2 ... 17
  u8 src[18];
  for (u32 i = 0; i < sizeof src; ++i) {
    src[i] = (u8)i;
  }

  test_assert_int_equal (ns_cleanup ("test"), 0);
  nsdb_t *smf = ns_smfile_open ("test");
  test_assert (smf != NULL);
  test_assert_int_equal (ns_smfile_insert (smf, NULL, src, 0, sizeof src), (int)sizeof src);

  // A strided read / remove that asks for more than the file holds
  //
  // 4 byte elements, every 2nd one, start at bytes 0, 8 and 16. The one at
  // 16 is cut off by the end of the file at 18, so only 2 are whole. Asking
  // for 3 or more failed with ERR_CORRUPT instead of returning those 2
  //      CAUSE:
  //          var_resolve_nelem clamped the count to (nbytes - bofst) / size,
  //          as if the elements were next to each other. With a stride the
  //          rope ran off the end of the file part way through and reported
  //          the odd byte count as corruption
  //      FIX:
  //          var_resolve_nelem takes the stride and only counts elements
  //          that fit as a whole

  TEST_CASE ("Read stops at the last whole element")
  {
    u8 dest[40];
    test_assert_int_equal (ns_smfile_read (smf, NULL, dest, 4, 0, 2, 5), 2);

    const u8 expected[8] = {0, 1, 2, 3, 8, 9, 10, 11};
    test_assert_memequal (dest, expected, sizeof expected);
  }

  TEST_CASE ("A strided write past the end is still an invalid argument")
  {
    test_assert (ns_smfile_write (smf, NULL, src, 4, 0, 2, 5) < 0);
    test_assert (ns_strerror (smf, false) != NULL);

    // And nothing changed
    u8 dest[18];
    test_assert_int_equal (ns_smfile_read (smf, NULL, dest, 1, 0, 1, sizeof dest), 18);
    test_assert_memequal (dest, src, sizeof src);
  }

  TEST_CASE ("Remove stops at the last whole element")
  {
    u8 dest[40];
    test_assert_int_equal (ns_smfile_remove (smf, NULL, dest, 4, 0, 2, 5), 2);

    const u8 removed[8] = {0, 1, 2, 3, 8, 9, 10, 11};
    test_assert_memequal (dest, removed, sizeof removed);

    test_assert_int_equal (ns_smfile_size (smf, NULL), 10);
    test_assert_int_equal (ns_smfile_read (smf, NULL, dest, 1, 0, 1, sizeof dest), 10);

    const u8 left[10] = {4, 5, 6, 7, 12, 13, 14, 15, 16, 17};
    test_assert_memequal (dest, left, sizeof left);
  }

  test_assert_int_equal (ns_close (smf), 0);
}
#endif
