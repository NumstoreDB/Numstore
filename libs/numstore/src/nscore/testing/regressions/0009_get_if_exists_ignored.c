
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
TEST (0009_get_if_exists_ignored)
{
  test_assert_int_equal (ns_cleanup ("test"), 0);
  nsdb_t *db = ns_open ("test");
  test_assert (db != NULL);

  // Get a variable that doesn't exist
  //
  // NOT FIXED - this still fails
  //      CAUSE:
  //          nsdb_plan_get_var passes false to numstore_get instead of the
  //          if_exists flag of the query, so the miss is an error
  //
  //          Passing the flag isn't enough: numstore_get then hands back a
  //          zeroed variable, and ns_get_var has no way to say "no variable,
  //          no error" other than NULL - which every caller (pynumstore,
  //          numstore-rs) reads as a failure today
  nsdb_var_t *var = ns_get_var (db, NULL, "get if exists nope");
  test_assert (var == NULL);
  test_assert (ns_strerror (db) == NULL);

  test_assert_int_equal (ns_close (db), 0);
}
#endif
