
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

#include <string.h>

#ifndef NDEBUG
#  include "core/ns_testing.h"
#endif

#ifndef NDEBUG
TEST (0007_open_with_error_not_terminated)
{
  // The error string of a failed open wasn't null terminated
  //      CAUSE:
  //          ns_open_with_error copied MIN (cmlen, elen) bytes of the message
  //          and never wrote the terminator

  TEST_CASE ("ns_open_with_error terminates the message")
  {
    char erstr[MAX_ERRSTR_LEN];
    memset (erstr, 'x', sizeof erstr);

    test_assert (ns_open_with_error ("/no/such/dir/db", erstr, sizeof erstr) == NULL);
    test_assert (memchr (erstr, '\0', sizeof erstr) != NULL);
    test_assert (erstr[0] != '\0');
  }

  TEST_CASE ("ns_open_with_error truncates to a short buffer")
  {
    char erstr[4];
    memset (erstr, 'x', sizeof erstr);

    test_assert (ns_open_with_error ("/no/such/dir/db", erstr, sizeof erstr) == NULL);
    test_assert_int_equal (erstr[3], '\0');
  }

  TEST_CASE ("ns_smfile_open_with_error terminates the message")
  {
    char erstr[MAX_ERRSTR_LEN];
    memset (erstr, 'x', sizeof erstr);

    test_assert (ns_smfile_open_with_error ("/no/such/dir/db", erstr, sizeof erstr) == NULL);
    test_assert (memchr (erstr, '\0', sizeof erstr) != NULL);
  }
}
#endif
