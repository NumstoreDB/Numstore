
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

#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "core/os/ns_filesystem.h"
#include "core/os/ns_memory.h"
#include "nscore/algorithms/numstore/ns_numstore_algorithms.h"
#include "nscore/nsdb/ns_nsdb.h"
#include "nscore/types/ns_types.h"
#include "nscore/variables/ns_variables.h"
#include "numstore/ns_numstore_internal.h"
#include "numstore/numstore.h"

#ifdef TESTING
#  include "core/testing/ns_testing.h"
#endif

#ifdef TESTING
TEST (0001_create_delete_rollback_delete)
{
  sb_size res;

  // Clean re open database
  test_assert_int_equal (numstore_cleanup ("test"), 0);
  numstore_t *db = numstore_open ("test");
  test_assert (db != NULL);

  // Create the variable
  res = _numstore_fexecute_simple_with_data (
      db,
      NULL,
      NULL,
      0,
      "create n8Si3C union { tok6UW u32, YGhr cf128, LDzpWVm f16 }"
  );
  test_assert_int_equal (res, 0);

  // The culprit txn: delete the variable, then roll it back
  struct txn *tx = numstore_begin (db);
  test_assert (tx != NULL);

  res = _numstore_fexecute_simple_with_data (db, tx, NULL, 0, "delete n8Si3C");
  test_assert_int_equal (res, 0);

  test_assert_int_equal (numstore_rollback (db, tx), 0);

  // Do something (seemingly unrelated)
  tx = numstore_begin (db);
  test_assert (tx != NULL);

  res = _numstore_fexecute_simple_with_data (
      db,
      tx,
      NULL,
      0,
      "create yJIF "
      "struct { sQf8W7t6 struct { ukc7C4 cf256, CHbmDuiD6 union { aVmHRo "
      "cf64, FeVvpnN u64 } } }"
  );
  test_assert_int_equal (res, 0);

  test_assert_int_equal (numstore_commit (db, tx), 0);

  // Delete the variable again
  //
  // This failed - it shouldn't because we roll'ed back our previous delete
  //      CAUSE:
  //          pgr_delete_and_release was setting the page in the fsm log
  //          to the page being released, not the fsm - this came from a
  //          refactor - I used to do that
  //          also it never included the bit in the log
  res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "delete n8Si3C");
  test_assert_int_equal (res, 0);

  // Close database
  test_assert_int_equal (numstore_close (db), 0);
}
#endif
