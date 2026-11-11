
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
// foo = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
static nsdb_t *
empty_range_open (void)
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

TEST (0006_empty_range_reads_to_end)
{
  // An empty range ([5:5], [7:3]) resolves to 0 elements
  //
  // These all acted on every element from start to the end of the variable
  //      CAUSE:
  //          the rope takes nelem == 0 as "no limit" and numstore_read /
  //          numstore_write / numstore_remove passed the 0 straight through

  TEST_CASE ("read foo[5:5] reads nothing")
  {
    nsdb_t *db = empty_range_open ();
    test_assert (db != NULL);

    u32 dest[10];
    test_assert_int_equal (ns_read (db, NULL, dest, sizeof dest, "read foo[5:5]"), 0);
    test_assert_int_equal (ns_read (db, NULL, dest, sizeof dest, "read foo[7:3]"), 0);

    test_assert_int_equal (ns_close (db), 0);
  }

  TEST_CASE ("write foo[5:5] writes nothing")
  {
    nsdb_t *db = empty_range_open ();
    test_assert (db != NULL);

    u32 src[5] = {50, 51, 52, 53, 54};
    test_assert_int_equal (ns_write (db, NULL, src, sizeof src, "write foo[5:5]"), 0);

    u32 dest[10];
    test_assert_int_equal (ns_read (db, NULL, dest, sizeof dest, "read foo"), 10);
    for (u32 i = 0; i < 10; ++i) {
      test_assert_int_equal (dest[i], i);
    }

    test_assert_int_equal (ns_close (db), 0);
  }

  TEST_CASE ("remove foo[7:3] removes nothing")
  {
    nsdb_t *db = empty_range_open ();
    test_assert (db != NULL);

    u32 dest[10];
    test_assert_int_equal (ns_read (db, NULL, dest, sizeof dest, "remove foo[7:3]"), 0);
    test_assert_int_equal (ns_execute (db, NULL, "remove foo[2:2]"), 0);

    nsdb_var_t *var = ns_get_var (db, NULL, "get foo");
    test_assert (var != NULL);
    test_assert_int_equal (ns_var_len (var), 10);
    ns_var_free (var);

    test_assert_int_equal (ns_close (db), 0);
  }
}
#endif
