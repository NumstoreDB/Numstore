
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
TEST (0014_type_size_overflow)
{
  test_assert_int_equal (ns_cleanup ("test"), 0);
  nsdb_t *db = ns_open ("test");
  test_assert (db != NULL);

  // Create a type that is wider than a t_size can say
  //
  // The create passed. The first insert into the variable then crashed a
  // debug build on
  //     ASSERT (ret % tsize == 0)     ns_numstore_insert.c (numstore_insert)
  // and [65536][65536]u8 was worse, its size was 0
  //      CAUSE:
  //          type_byte_size multiplies the dimensions in a u32 and wraps on
  //          purpose - the c code it replaced did the same.
  //          [65536][65536]u8 came out 0 bytes wide and
  //          [65535][65535][65535]u64 some other wrong number. Nothing
  //          checked the size when the type was compiled, so the bad type
  //          got stored
  //      FIX:
  //          type_validate (nslang/types/ns_types.c) now computes the size
  //          with overflow checks and fails with ERR_ARITH, and
  //          every query is validated right after it's compiled
  //          (compile_and_validate_query -> validate_query), so the bad
  //          type never gets as far as numstore_create. Before, a release
  //          build never validated a created type
  //
  // Tabled, still open:
  //    - A database that already has a variable like this now fails
  //      DBG_ASSERT (valid_type) wherever the type is used
  //    - Is U32_MAX the right limit. An element that big can't be read
  //      into a buffer anyway, a much smaller limit might be more honest
  test_assert (ns_execute (db, NULL, "create foo [65536][65536]u8") < 0);
  test_assert (ns_strerror (db, false) != NULL);
  test_assert (ns_execute (db, NULL, "create foo [65535][65535][65535]u64") < 0);
  test_assert (
      ns_execute (db, NULL, "create foo struct { a [65535][65535]u8, b [65535][65535]u8 }") < 0
  );
  test_assert (ns_get_var (db, NULL, "get foo") == NULL);

  // The widest type that fits is fine
  test_assert_int_equal (ns_execute (db, NULL, "create foo [65535][65537]u8"), 0);

  test_assert_int_equal (ns_close (db), 0);
}
#endif
