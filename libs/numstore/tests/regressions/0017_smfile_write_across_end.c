
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
// A 6 byte smart file holding 1 2 3 4 5 6
static nsdb_t *
smfile_with_six_bytes (void)
{
  const u8 six[6] = {1, 2, 3, 4, 5, 6};

  if (ns_cleanup ("test") != 0) {
    return NULL;
  }
  nsdb_t *smf = ns_smfile_open ("test");
  if (smf == NULL) {
    return NULL;
  }
  if (ns_smfile_insert (smf, NULL, six, 0, sizeof six) != (sb_size)sizeof six) {
    ns_close (smf);
    return NULL;
  }
  return smf;
}

TEST (0017_smfile_write_across_end)
{
  const u8 src[8] = {11, 12, 13, 14, 15, 16, 17, 18};
  u8       dest[16];

  // A write that crosses the end of the file
  //
  // ns_smfile_write overwrites what is inside the file and appends the rest.
  // That only worked when the end of the file fell on an element boundary
  // of the write. A file of 6 bytes written with 4 byte elements has 2
  // bytes left over, and those were handled wrong in two ways
  //      CAUSE:
  //          smartfiles_write split the write by whole elements: the ones
  //          that fit ((nbytes - bofst) / size) were overwritten, the rest
  //          were appended at nbytes. The bytes of the element that
  //          straddles the end were never overwritten, so the appended data
  //          landed too far out. And when no whole element fit, the
  //          overwrite ran with nelem == 0, which ns_rope_write reads as
  //          "no limit": it wrote up to the end of the file and failed on
  //          the half element
  //      FIX:
  //          A stride of 1 is a byte range, so smartfiles_write splits it
  //          by bytes instead of elements

  TEST_CASE ("The straddling element isn't skipped")
  {
    nsdb_t *smf = smfile_with_six_bytes ();
    test_assert (smf != NULL);

    // Before the fix: 11 12 13 14 5 6 15 16 17 18
    test_assert_int_equal (ns_smfile_write (smf, NULL, src, 4, 0, 1, 2), 2);
    test_assert_int_equal (ns_smfile_size (smf, NULL), 8);
    test_assert_int_equal (ns_smfile_read (smf, NULL, dest, 1, 0, 1, sizeof dest), 8);
    test_assert_memequal (dest, src, sizeof src);

    test_assert_int_equal (ns_close (smf), 0);
  }

  TEST_CASE ("A write with no whole element inside the file")
  {
    nsdb_t *smf = smfile_with_six_bytes ();
    test_assert (smf != NULL);

    // Before the fix: an error, "wrote 2 bytes, not a multiple of element
    // size 4"
    test_assert_int_equal (ns_smfile_write (smf, NULL, src, 4, 4, 1, 2), 2);
    test_assert_int_equal (ns_smfile_size (smf, NULL), 12);
    test_assert_int_equal (ns_smfile_read (smf, NULL, dest, 1, 0, 1, sizeof dest), 12);

    const u8 expected[12] = {1, 2, 3, 4, 11, 12, 13, 14, 15, 16, 17, 18};
    test_assert_memequal (dest, expected, sizeof expected);

    test_assert_int_equal (ns_close (smf), 0);
  }

  TEST_CASE ("A write inside the file is unchanged")
  {
    nsdb_t *smf = smfile_with_six_bytes ();
    test_assert (smf != NULL);

    test_assert_int_equal (ns_smfile_write (smf, NULL, src, 2, 1, 1, 2), 2);
    test_assert_int_equal (ns_smfile_size (smf, NULL), 6);
    test_assert_int_equal (ns_smfile_read (smf, NULL, dest, 1, 0, 1, sizeof dest), 6);

    const u8 expected[6] = {1, 11, 12, 13, 14, 6};
    test_assert_memequal (dest, expected, sizeof expected);

    test_assert_int_equal (ns_close (smf), 0);
  }
}
#endif
