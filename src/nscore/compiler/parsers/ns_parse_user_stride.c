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

#include "core/ns_alloc.h"
#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "core/ns_stride.h"
#include "core/testing/ns_testing.h"
#include "nscore/compiler/ns_compiler.h"
#include "nscore/compiler/ns_lexer.h"
#include "nscore/compiler/ns_tokens.h"
#include "nscore/compiler/parsers/ns_parser.h"

#include <string.h>

/******************************************************************************
 * SECTION: User Stride
 * ----------------------------------------------------------------------------
 * user_stride ::= '[' ( INTEGER ( ':' INTEGER? ( ':' INTEGER? )? )?
 *                     | ':' INTEGER? ( ':' INTEGER? )?
 *                     ) ']'
 ******************************************************************************/

// Parse optional ':' NUMBER (step)
static void
parse_us_step (struct parser *base, struct user_stride *s)
{
  if (!parser_match (base, TT_COLON)) {
    return;
  }

  s->present |= COLON_PRESENT;
  parser_advance (base);

  i32 num;
  if (parser_maybe_parse_integer (base, &num)) {
    s->step = num;
    s->present |= STEP_PRESENT;
  }
}

// Parse optional NUMBER (stop), then optional ':' NUMBER (step)
static void
parse_us_stop (struct parser *base, struct user_stride *s)
{
  i32 num;
  if (parser_maybe_parse_integer (base, &num)) {
    s->stop = num;
    s->present |= STOP_PRESENT;
  }

  parse_us_step (base, s);
}

err_t
parse_user_stride (struct parser *parser, struct user_stride *dest, error *e)
{
  struct user_stride s = {0};

  WRAP (parser_expect (parser, TT_LEFT_BRACKET, e));

  int num;
  if (parser_maybe_parse_integer (parser, &num)) {
    // Leading integer: start
    s.start = num;
    s.present |= START_PRESENT;

    if (parser_match (parser, TT_COLON)) {
      // start ':' ...
      s.present |= COLON_PRESENT;
      parser_advance (parser);
      parse_us_stop (parser, &s);
    }
    // else: bare integer â€” single index, nothing more to parse
  } else if (parser_match (parser, TT_COLON)) {
    // No leading integer: ':' ...
    s.present |= COLON_PRESENT;
    parser_advance (parser);
    parse_us_stop (parser, &s);
  } else {
    return error_causef (e, ERR_SYNTAX, "Expected number or ':' at position %u", parser->pos);
  }

  *dest = s;
  return parser_expect (parser, TT_RIGHT_BRACKET, e);
}

err_t
compile_user_stride (struct user_stride *dest, const char *text, error *e)
{
  ALLOC_INIT (alloc);

  struct lexer lex;
  if (lex_tokens (text, &alloc, strlen (text), &lex, e) < 0) {
    goto theend;
  }

  struct parser parser = parser_init (lex.tokens, NULL, lex.ntokens);

  if (parse_user_stride (&parser, dest, e)) {
    goto theend;
  }

theend:
  ALLOC_CLOSE (alloc);
  return error_trace (e);
}

#ifdef TESTING
static void
test_compile_user_stride_green_path (const char *query, struct user_stride expected)
{
  struct user_stride actual;
  error              e = error_create ();

  TEST_CASE ("SHOULD PASS: %s", query)
  {
    test_assert_equal (compile_user_stride (&actual, query, &e), SUCCESS);
    test_assert (user_stride_equal (&actual, &expected));
  }
}

static void
test_compile_user_stride_red_path (const char *query, err_t code)
{
  struct user_stride actual;
  error              e = error_create ();

  TEST_CASE ("SHOULD FAIL: %s", query)
  {
    test_err_t_check (compile_user_stride (&actual, query, &e), code, &e);
  }
}

TEST (compile_user_stride)
{
  test_compile_user_stride_red_path ("[]", ERR_SYNTAX);
  test_compile_user_stride_red_path ("[ ]", ERR_SYNTAX);
  test_compile_user_stride_red_path (" [ ]", ERR_SYNTAX);

  test_compile_user_stride_green_path ("[5]", ustride_single (5));

  test_compile_user_stride_green_path ("[:]", ustride ());
  test_compile_user_stride_green_path ("[:6]", ustride1 (6));
  test_compile_user_stride_green_path ("[5:]", ustride0 (5));
  test_compile_user_stride_green_path ("[5:6]", ustride01 (5, 6));

  test_compile_user_stride_green_path ("[::]", ustride ());
  test_compile_user_stride_green_path ("[::7]", ustride2 (7));
  test_compile_user_stride_green_path ("[:6:]", ustride1 (6));
  test_compile_user_stride_green_path ("[:6:7]", ustride12 (6, 7));

  test_compile_user_stride_green_path ("[5::]", ustride0 (5));
  test_compile_user_stride_green_path ("[5::7]", ustride02 (5, 7));
  test_compile_user_stride_green_path ("[5:6:]", ustride01 (5, 6));
  test_compile_user_stride_green_path ("[5:6:7]", ustride012 (5, 6, 7));
}
#endif
