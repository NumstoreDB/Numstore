
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
TEST (0021_query_trailing_text)
{
  test_assert_int_equal (ns_cleanup ("test"), 0);
  nsdb_t *db = ns_open ("test");
  test_assert (db != NULL);

  const u32 src[4] = {1, 2, 3, 4};
  u32       dest[4];
  test_assert_int_equal (ns_execute (db, NULL, "create foo u32"), 0);
  test_assert_int_equal (ns_execute (db, NULL, "create bar u32"), 0);
  test_assert_int_equal (ns_write (db, NULL, src, sizeof src, "insert foo 0 4"), 4);

  // Text after a complete query
  //
  // "delete foo bar" deleted foo and said nothing about bar. So did
  // "delete foo.a" and "delete foo if exists"
  //      CAUSE:
  //          compile_query stopped at the end of the first query it could
  //          parse and never looked at the tokens left over
  //      FIX:
  //          compile_query expects the end of the input after the query

  TEST_CASE ("A query with text after it is a syntax error")
  {
    test_assert (ns_execute (db, NULL, "delete foo bar") < 0);
    test_assert (ns_strerror (db, false) != NULL);
    test_assert (ns_execute (db, NULL, "delete foo.a") < 0);
    test_assert (ns_execute (db, NULL, "delete foo if exists") < 0);
    test_assert (ns_execute (db, NULL, "create baz u32 u32") < 0);
    test_assert (ns_execute (db, NULL, "remove foo[0:1] bar") < 0);
    test_assert (ns_read (db, NULL, dest, sizeof dest, "read foo[0:2] [2:4]") < 0);
    test_assert (ns_write (db, NULL, src, sizeof src, "insert foo 0 4 4") < 0);
    test_assert (ns_write (db, NULL, src, sizeof src, "write foo[0:4] delete bar") < 0);

    nsdb_var_t *var = ns_get_var (db, NULL, "get foo bar");
    test_assert (var == NULL);
    test_assert (ns_strerror (db, false) != NULL);
  }

  TEST_CASE ("And none of them did anything")
  {
    test_assert_int_equal (ns_read (db, NULL, dest, sizeof dest, "read foo"), 4);
    test_assert_memequal (dest, src, sizeof src);

    nsdb_var_t *var = ns_get_var (db, NULL, "get bar");
    test_assert (var != NULL);
    ns_var_free (var);

    test_assert (ns_get_var (db, NULL, "get baz") == NULL);
    test_assert (ns_strerror (db, false) != NULL);
  }

  // A query that ends where the lexer wants to look one character ahead
  //
  // These hit an assert in the lexer (and read one byte past the query in
  // a release build)
  //      CAUSE:
  //          peek_next and match looked at the next character without
  //          checking that there is one. A number followed by '.' and
  //          a '!', '<', '>', '|', '&' or '=' at the very end got there
  //      FIX:
  //          Both treat the end of the input as "no match"

  TEST_CASE ("A query that ends in the middle of a token")
  {
    const char *queries[] = {
        "read foo[0:1.",
        "delete foo 1.",
        "delete foo !",
        "delete foo <",
        "delete foo >",
        "delete foo |",
        "delete foo &",
        "delete foo =",
    };

    for (u32 i = 0; i < sizeof queries / sizeof *queries; ++i) {
      test_assert (ns_execute (db, NULL, "%s", queries[i]) < 0);
      test_assert (ns_strerror (db, false) != NULL);
    }

    // foo is still there
    test_assert_int_equal (ns_read (db, NULL, dest, sizeof dest, "read foo"), 4);
  }

  TEST_CASE ("Whitespace after a query is fine")
  {
    test_assert_int_equal (ns_read (db, NULL, dest, sizeof dest, "read foo[0:2] \t\n"), 2);
    test_assert_int_equal (ns_execute (db, NULL, "delete bar  "), 0);
  }

  test_assert_int_equal (ns_close (db), 0);
}
#endif
