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

TEST (0002_create_crash_close_delete)
{
  sb_size res;

  // Clean re open database
  test_assert_int_equal (numstore_cleanup ("test"), 0);
  numstore_t *db = numstore_open ("test");
  test_assert (db != NULL);

  // Create the variable
  res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create MkWMJ9a [8][9][3][3] i16");
  test_assert_int_equal (res, 0);

  // Crash, then re open
  test_assert_int_equal (numstore_crash (db), 0);
  db = numstore_open ("test");
  test_assert (db != NULL);

  // Cleanly close, then re open
  test_assert_int_equal (numstore_close (db), 0);
  db = numstore_open ("test");
  test_assert (db != NULL);

  // Delete the variable
  //
  // This Failed - it shouldnt
  //        CAUSE:
  //          The first log of fsm is a fsm update log. But the fsm page starts
  //          uninitialized, therefore it needs one upfront physical log first
  //          before it can be used - log a physical update log then continue on
  //          with fsm specific logs
  res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "delete MkWMJ9a");
  test_assert_int_equal (res, 0);

  // Close database
  test_assert_int_equal (numstore_close (db), 0);
}

#endif
