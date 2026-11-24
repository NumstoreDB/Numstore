
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
TEST (0008_delete_if_exists_ignored)
{
  test_assert_int_equal (ns_cleanup ("test"), 0);
  nsdb_t *db = ns_open ("test");
  test_assert (db != NULL);

  // Delete a variable that doesn't exist
  //
  // This failed with "Variable: nope doesn't exist"
  //      CAUSE:
  //          nsdb_plan_execute passed false to numstore_delete instead of
  //          the if_exists flag of the query
  test_assert_int_equal (ns_execute (db, NULL, "delete if exists nope"), 0);
  test_assert (ns_strerror (db, false) == NULL);

  // Without the flag it's still an error
  test_assert (ns_execute (db, NULL, "delete nope") != 0);

  // And the flag still deletes a variable that does exist
  test_assert_int_equal (ns_execute (db, NULL, "create foo u32"), 0);
  test_assert_int_equal (ns_execute (db, NULL, "delete if exists foo"), 0);
  test_assert (ns_get_var (db, NULL, "get foo") == NULL);

  test_assert_int_equal (ns_close (db), 0);
}
#endif
