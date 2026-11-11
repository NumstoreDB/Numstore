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

#include "ns_tokens.h"

#include "ns_csx_assert.h"
#include "ns_string.h"

#ifndef NDEBUG
#  include "ns_testing.h"
#endif

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
      // TODO: the right side is measured with left's length, so "ab" equals
      // "abc". Compare right->str.len too.
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

#ifndef NDEBUG
TEST (tt_tostr)
{
#  define TC_TTTOSTR(x)                 \
    const char *x##_str = tt_tostr (x); \
    (void)x##_str;                      \
    i_log_info ("%s\n", x##_str);
  TT_FOREACH (TC_TTTOSTR);
}

TEST (token_equal)
{
  TEST_CASE ("different types are never equal")
  {
    struct token left  = tt_integer (1);
    struct token right = tt_float (1.0F);
    test_fail_if (token_equal (&left, &right));
  }

  TEST_CASE ("identifiers compare by text")
  {
    struct token left  = tt_ident ("abc", 3);
    struct token same  = tt_ident ("abc", 3);
    struct token other = tt_ident ("abd", 3);
    test_assert (token_equal (&left, &same));
    test_fail_if (token_equal (&left, &other));
  }

  TEST_CASE ("numbers compare by value")
  {
    struct token one   = tt_integer (1);
    struct token two   = tt_integer (2);
    struct token half  = tt_float (0.5F);
    struct token half2 = tt_float (0.5F);
    test_fail_if (token_equal (&one, &two));
    test_assert (token_equal (&half, &half2));
  }

  TEST_CASE ("other tokens compare by type alone")
  {
    struct token left  = quick_tok (TT_COMMA);
    struct token right = quick_tok (TT_COMMA);
    test_assert (token_equal (&left, &right));
  }
}
#endif
