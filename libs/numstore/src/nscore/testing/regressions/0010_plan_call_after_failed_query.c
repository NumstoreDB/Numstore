
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

#ifndef NDEBUG
#  include "core/ns_testing.h"
#endif

#ifndef NDEBUG
TEST (0010_plan_call_after_failed_query)
{
  test_assert_int_equal (ns_cleanup ("test"), 0);
  nsdb_t *db = ns_open ("test");
  test_assert (db != NULL);

  u32 src[4] = {0, 1, 2, 3};
  u32 dest[4];
  test_assert_int_equal (ns_execute (db, NULL, "create foo u32"), 0);
  test_assert_int_equal (ns_write (db, NULL, src, sizeof src, "insert foo 0 4"), 4);

  nsdb_plan_t *plan = ns_plan_fcreate (db, "read foo");
  test_assert (plan != NULL);

  // Something unrelated fails and nobody reads the error
  test_assert (ns_execute (db, NULL, "this is not a query") != 0);

  // Run the plan
  //
  // This crashed on the clean error assert (and in a release build the
  // plan ran with the stale error of the query above)
  //      CAUSE:
  //          ns_execute / ns_read / ... reset the database error before they
  //          run, the ns_plan_* functions didn't
  test_assert_int_equal (ns_plan_read (plan, NULL, dest, sizeof dest), 4);
  test_assert (ns_plan_strerror (plan) == NULL);

  // A plan that fails doesn't poison the next call either
  nsdb_plan_t *bad = ns_plan_fcreate (db, "read nope");
  test_assert (bad != NULL);
  test_assert (ns_plan_read (bad, NULL, dest, sizeof dest) < 0);
  test_assert_int_equal (ns_plan_read (plan, NULL, dest, sizeof dest), 4);

  nsdb_var_t *var = ns_plan_get_var (plan, NULL);
  test_assert (var != NULL);
  test_assert_int_equal (ns_var_len (var), 4);
  ns_var_free (var);

  ns_plan_free (bad);
  ns_plan_free (plan);
  test_assert_int_equal (ns_close (db), 0);
}
#endif
