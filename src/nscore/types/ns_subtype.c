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

#include "nscore/types/ns_subtype.h"

#include "core/ns_alloc.h"
#include "core/ns_error.h"
#include "core/ns_string.h"
#include "core/testing/ns_testing.h"
#include "nscore/compiler/ns_compiler.h"
#include "nscore/types/ns_type_accessor.h"

#include <stdbool.h>

/******************************************************************************
 * SECTION: Sub Type
 ******************************************************************************/

struct subtype
subtype_create (struct string vname, struct type_accessor ta)
{
  return (struct subtype){
      .vname = vname,
      .ta    = ta,
  };
}

bool
subtype_equal (const struct subtype *left, const struct subtype *right)
{
  return string_equal (left->vname, right->vname) && type_accessor_equal (left->ta, right->ta);
}

#ifdef TESTING
TEST (subtype_equal)
{
  ALLOC_INIT (alloc);
  error e = error_create ();

#  define STE_TC(left, right, expected)                           \
    TEST_CASE ("%s == %s == %d", left, right, expected)           \
    {                                                             \
      struct subtype leftst;                                      \
      struct subtype rightst;                                     \
                                                                  \
      compile_subtype (&leftst, left, &alloc, &e);                \
      compile_subtype (&rightst, right, &alloc, &e);              \
                                                                  \
      if (expected) {                                             \
        test_assert (subtype_equal (&leftst, &rightst));          \
      } else {                                                    \
        test_assert (subtype_equal (&leftst, &rightst) == false); \
      }                                                           \
    }

  STE_TC ("foo.bar", "foo.bar", true);
  STE_TC ("foo.bar[0]", "foo.bar[0]", true);
  STE_TC ("foo.bar[1]", "foo.bar[0]", false);
  STE_TC ("foo.bar[0][1]", "foo.bar[0]", false);
  STE_TC ("foo[0][1]", "foo.bar[0]", false);
  STE_TC ("biz.bar[0]", "foo.bar[0]", false);

  ALLOC_CLOSE (alloc);
}
#endif
