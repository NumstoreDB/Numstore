
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

TEST (ns_query_api_mismatch)
{
  test_assert_int_equal (ns_cleanup ("test"), 0);
  nsdb_t *db = ns_open ("test");
  test_assert (db != NULL);

  u32 src[4] = {0, 1, 2, 3};
  u32 dest[4];
  test_assert_int_equal (ns_execute (db, NULL, "create foo u32"), 0);
  test_assert_int_equal (ns_write (db, NULL, src, sizeof src, "insert foo 0 4"), 4);

  TEST_CASE ("ns_execute rejects queries that need a buffer")
  {
    test_assert (ns_execute (db, NULL, "read foo") < 0);
    test_assert (ns_strerror (db, false) != NULL);
    test_assert (ns_execute (db, NULL, "insert foo 0 4") < 0);
    test_assert (ns_strerror (db, false) != NULL);
    test_assert (ns_execute (db, NULL, "write foo") < 0);
    test_assert (ns_strerror (db, false) != NULL);
  }

  TEST_CASE ("ns_read rejects queries that don't read")
  {
    test_assert (ns_read (db, NULL, dest, sizeof dest, "insert foo 0 4") < 0);
    test_assert (ns_read (db, NULL, dest, sizeof dest, "write foo") < 0);
    test_assert (ns_read (db, NULL, dest, sizeof dest, "get foo") < 0);
    test_assert (ns_read (db, NULL, dest, sizeof dest, "create bar u32") < 0);
    test_assert (ns_read (db, NULL, dest, sizeof dest, "delete foo") < 0);
    test_assert (ns_strerror (db, false) != NULL);
  }

  TEST_CASE ("ns_write rejects queries that don't write")
  {
    test_assert (ns_write (db, NULL, src, sizeof src, "read foo") < 0);
    test_assert (ns_write (db, NULL, src, sizeof src, "remove foo") < 0);
    test_assert (ns_write (db, NULL, src, sizeof src, "get foo") < 0);
    test_assert (ns_write (db, NULL, src, sizeof src, "delete foo") < 0);
    test_assert (ns_strerror (db, false) != NULL);
  }

  TEST_CASE ("ns_malloc rejects queries that don't read")
  {
    b_size blen = 0;
    test_assert (ns_malloc (db, NULL, &blen, "insert foo 0 4") == NULL);
    test_assert (ns_strerror (db, false) != NULL);
    test_assert (ns_malloc (db, NULL, &blen, "get foo") == NULL);
    test_assert (ns_strerror (db, false) != NULL);
  }

  TEST_CASE ("Queries on a variable that doesn't exist fail")
  {
    test_assert (ns_read (db, NULL, dest, sizeof dest, "read nope") < 0);
    test_assert (ns_write (db, NULL, src, sizeof src, "insert nope 0 4") < 0);
    test_assert (ns_write (db, NULL, src, sizeof src, "write nope") < 0);
    test_assert (ns_execute (db, NULL, "remove nope") < 0);
    test_assert (ns_execute (db, NULL, "delete nope") < 0);
    test_assert (ns_get_var (db, NULL, "get nope") == NULL);
    test_assert (ns_strerror (db, false) != NULL);
  }

  TEST_CASE ("Queries that don't compile fail")
  {
    test_assert (ns_execute (db, NULL, "") < 0);
    test_assert (ns_execute (db, NULL, "foo") < 0);
    test_assert (ns_execute (db, NULL, "create") < 0);
    test_assert (ns_execute (db, NULL, "create bar") < 0);
    test_assert (ns_execute (db, NULL, "create bar u33") < 0);
    test_assert (ns_execute (db, NULL, "create bar struct { }") < 0);
    test_assert (ns_execute (db, NULL, "create bar struct { a u8, a u8 }") < 0);
    test_assert (ns_execute (db, NULL, "create bar [0]u8") < 0);
    test_assert (ns_execute (db, NULL, "create u32 u32") < 0);
    test_assert (ns_read (db, NULL, dest, sizeof dest, "read foo[") < 0);
    test_assert (ns_read (db, NULL, dest, sizeof dest, "read foo[a]") < 0);
    test_assert (ns_write (db, NULL, src, sizeof src, "insert foo") < 0);
    test_assert (ns_write (db, NULL, src, sizeof src, "insert foo 0") < 0);
    test_assert (ns_strerror (db, false) != NULL);

    // The error is reported once
    test_assert (ns_strerror (db, false) == NULL);
  }

  TEST_CASE ("A create with the same type is fine, another type isn't")
  {
    test_assert_int_equal (ns_execute (db, NULL, "create foo u32"), 0);
    test_assert (ns_execute (db, NULL, "create foo u64") < 0);
    test_assert (ns_execute (db, NULL, "create foo [1]u32") < 0);
  }

  TEST_CASE ("None of the failures above changed the variable")
  {
    test_assert_int_equal (ns_read (db, NULL, dest, sizeof dest, "read foo"), 4);
    test_assert_memequal (dest, src, sizeof src);
    test_assert (ns_get_var (db, NULL, "get bar") == NULL);
  }

  test_assert_int_equal (ns_close (db), 0);
}

#endif
