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

#include "nscore/compiler/ns_tokens.h"

#include "core/ns_csx_assert.h"
#include "core/ns_string.h"
#include "core/testing/ns_testing.h"

#include <stddef.h>

bool
token_equal (const struct token *left, const struct token *right)
{
  if (left->type != right->type) {
    return false;
  }

  switch (left->type) {
      // Other
    case TT_STRING:
    case TT_IDENTIFIER: {
      return string_equal (
          (struct string){left->str.len, (char *)left->str.data},
          (struct string){left->str.len, (char *)right->str.data}
      );
    }

      // Tokens that start with a number or +/-
    case TT_INTEGER: return left->integer == right->integer;
    case TT_FLOAT: return left->floating == right->floating;

    default: {
      return true;
    }
  }
}

const char *
tt_tostr (enum token_t t)
{
  switch (t) {
    TT_FOREACH (case_ENUM_RETURN_STRING)
  }

  UNREACHABLE (); // LCOV_EXCL_LINE
  return NULL;
}

#ifdef TESTING
TEST (tt_tostr)
{
#  define TC_TTTOSTR(x)                 \
    const char *x##_str = tt_tostr (x); \
    i_log_info ("%s\n", x##_str);
  TT_FOREACH (TC_TTTOSTR);
}
#endif
