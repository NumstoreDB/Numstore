
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
TEST (0012_remove_empties_leftmost_page)
{
  enum
  {
    LEN = 3000,
  };

  static u32 src[LEN];
  static u32 dest[LEN];
  for (u32 i = 0; i < LEN; ++i) {
    src[i] = i;
  }

  test_assert_int_equal (ns_cleanup ("test"), 0);
  nsdb_t *db = ns_open ("test");
  test_assert (db != NULL);

  // A variable that spans a few data pages
  test_assert_int_equal (ns_execute (db, NULL, "create foo u32"), 0);
  test_assert_int_equal (ns_write (db, NULL, src, sizeof src, "insert foo 0 %d", LEN), LEN);

  // Leave the left most page nearly empty
  test_assert_int_equal (ns_read (db, NULL, dest, sizeof dest, "remove foo[0:1233]"), 1233);

  // Remove a range that empties the left most page
  //
  // This crashed on an assert in debug builds (release builds were fine)
  //      CAUSE:
  //          ns_maybe_delete_cur bridges prev and next around the page it
  //          deletes. The left most page has no prev, so it links (NULL,
  //          next) - which dlgt_link handles, but it asserted left != NULL
  test_assert_int_equal (ns_read (db, NULL, dest, sizeof dest, "remove foo[2:821]"), 819);

  // foo = [1233, 1234, 2054, 2055, ..., 2999]
  test_assert_int_equal (ns_read (db, NULL, dest, sizeof dest, "read foo"), 948);
  test_assert_int_equal (dest[0], 1233);
  test_assert_int_equal (dest[1], 1234);
  for (u32 i = 2; i < 948; ++i) {
    test_assert_int_equal (dest[i], 2052 + i);
  }

  test_assert_int_equal (ns_close (db), 0);
}
#endif
