
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
#include "numstore/numstore.h"

#ifdef TESTING
#  include "core/testing/ns_testing.h"
#endif

#ifdef TESTING
TEST (0001_create_delete_rollback_delete)
{
  sb_size res;

  // Clean re open database
  test_assert_int_equal (ns_cleanup ("test"), 0);
  nsdb_t *db = ns_open ("test");
  test_assert (db != NULL);

  // Create the variable
  res = ns_exec (db, NULL, "create n8Si3C union { tok6UW u32, YGhr cf128, LDzpWVm f16 }");
  test_assert_int_equal (res, 0);

  // The culprit txn: delete the variable, then roll it back
  struct txn *tx = ns_begin (db);
  test_assert (tx != NULL);

  res = ns_exec (db, tx, "delete n8Si3C");
  test_assert_int_equal (res, 0);

  test_assert_int_equal (ns_rollback (db, tx), 0);

  // Do something (seemingly unrelated)
  tx = ns_begin (db);
  test_assert (tx != NULL);

  res = ns_exec (
      db,
      tx,
      "create yJIF "
      "struct { sQf8W7t6 struct { ukc7C4 cf256, CHbmDuiD6 union { aVmHRo "
      "cf64, FeVvpnN u64 } } }"
  );
  test_assert_int_equal (res, 0);

  test_assert_int_equal (ns_commit (db, tx), 0);

  // Delete the variable again
  //
  // This failed - it shouldn't because we roll'ed back our previous delete
  //      CAUSE:
  //          pgr_delete_and_release was setting the page in the fsm log
  //          to the page being released, not the fsm - this came from a
  //          refactor - I used to do that
  //          also it never included the bit in the log
  res = ns_exec (db, NULL, "delete n8Si3C");
  test_assert_int_equal (res, 0);

  // Close database
  test_assert_int_equal (ns_close (db), 0);
}
#endif
