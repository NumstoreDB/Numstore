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

#include "_numstore.h"
#include "nscore/errors.h"
#include "nscore/nshandle.h"
#include "nscore/var.h"
#include "numstore.h"

#include <c_specx.h>

#ifndef NTEST
TEST (cgd_test_create_delete_rollback_delete)
{
  test_assert_int_equal (nsh_cleanup ("test"), 0);
  nsdb_t *db = nsdb_open ("test");
  test_assert (db != NULL);

  // Create the variable
  test_assert_int_equal (
      nsdb_create (db, "n8Si3C", "union { tok6UW u32, YGhr cf128, LDzpWVm f16 }"),
      0
  );

  // The culprit txn
  test_assert_int_equal (nsdb_begin (db), 0);
  test_assert_int_equal (nsdb_delete (db, "n8Si3C"), 0);
  test_assert_int_equal (nsdb_rollback (db), 0);

  // Do something (seemingly unrelated)
  test_assert_int_equal (nsdb_begin (db), 0);
  test_assert_int_equal (
      nsdb_create (
          db,
          "yJIF",
          "struct { sQf8W7t6 struct { ukc7C4 cf256, CHbmDuiD6 union { aVmHRo cf64, FeVvpnN u64 } } "
          "}"
      ),
      0
  );
  test_assert_int_equal (nsdb_commit (db), 0);

  // This failed - it shouldn't because we roll'ed back our previous delete
  //      CAUSE:
  //          pgr_delete_and_release was setting the page in the fsm log
  //          to the page being released, not the fsm - this came from a
  //          refactor - I used to do that
  //          also it never included the bit in the log
  test_assert (nsdb_delete (db, "n8Si3C") == 0);

  test_assert_int_equal (nsdb_close (db), 0);
}

TEST (cgd_test_create_crash_close_delete)
{
  test_assert_int_equal (nsh_cleanup ("test"), 0);
  nsdb_t *db = nsdb_open ("test");
  test_assert (db != NULL);

  // Create
  test_assert_int_equal (nsdb_create (db, "MkWMJ9a", "[8][9][3][3] i16"), 0);

  // Crash
  test_assert_int_equal (_nsdb_crash (db), 0);
  db = nsdb_open ("test");
  test_assert (db != NULL);

  // Close
  test_assert_int_equal (nsdb_close (db), 0);
  db = nsdb_open ("test");
  test_assert (db != NULL);

  // This Failed
  test_assert (nsdb_delete (db, "MkWMJ9a") != 0);

  test_assert_int_equal (nsdb_close (db), 0);
}
#endif
