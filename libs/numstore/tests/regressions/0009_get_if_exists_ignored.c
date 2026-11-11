
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
TEST (0009_get_if_exists_ignored)
{
  test_assert_int_equal (ns_cleanup ("test"), 0);
  nsdb_t *db = ns_open ("test");
  test_assert (db != NULL);

  // Get a variable that doesn't exist
  //
  // FIXED
  //      CAUSE:
  //          nsdb_plan_get_var passed false to numstore_get instead of the
  //          if_exists flag of the query, so the miss was an error
  //      FIX:
  //          nsdb_plan_get_var turns its query into a get (query_to_get),
  //          passes that flag and returns NULL with a clean error on a
  //          miss. NULL from a get is only a failure when ns_strerror
  //          isn't NULL - pynumstore returns None and numstore-rs returns
  //          Ok(None) for it
  nsdb_var_t *var = ns_get_var (db, NULL, "get if exists nope");
  test_assert (var == NULL);
  test_assert (ns_strerror (db, false) == NULL);

  // delete has the flag too
  var = ns_get_var (db, NULL, "delete if exists nope");
  test_assert (var == NULL);
  test_assert (ns_strerror (db, false) == NULL);

  // Without the flag a miss is still an error
  var = ns_get_var (db, NULL, "get nope");
  test_assert (var == NULL);
  test_assert (ns_strerror (db, false) != NULL);

  // And a variable that exists comes back either way
  test_assert_int_equal (ns_execute (db, NULL, "create foo u32"), 0);
  var = ns_get_var (db, NULL, "get if exists foo");
  test_assert (var != NULL);
  ns_var_free (var);

  test_assert_int_equal (ns_close (db), 0);
}
#endif
