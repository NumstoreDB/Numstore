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
#include "core/ns_platform.h"
#include "core/ns_stdtypes.h"
#include "core/ns_stride.h"
#include "core/testing/ns_testing.h"
#include "nscore/compiler/ns_compiler.h"
#include "nscore/compiler/ns_lexer.h"
#include "nscore/compiler/ns_tokens.h"
#include "nscore/compiler/parsers/ns_parser.h"

#include <string.h>

/******************************************************************************
 * SECTION: Multi User Stride
 * ----------------------------------------------------------------------------
 * multi_user_stride ::= '[' ( entry ( ',' entry )* )? ']'
 *
 * entry             ::= INTEGER ( ':' INTEGER? ( ':' INTEGER? )? )? |
 *                       ':' INTEGER? ( ':' INTEGER? )?
 ******************************************************************************/

struct multi_user_stride_parser
{
  struct parser     *base;
  struct mus_builder builder;
};

// Parse optional ':' NUMBER (step)
static err_t
parse_mus_step (struct multi_user_stride_parser *parser, struct user_stride *s, error *e)
{
  if (!parser_match (parser->base, TT_COLON)) {
    return SUCCESS;
  }

  s->present |= COLON_PRESENT;
  parser_advance (parser->base);

  i32 num;
  if (parser_maybe_parse_integer (parser->base, &num)) {
    s->step = num;
    s->present |= STEP_PRESENT;
  }

  return SUCCESS;
}

static err_t
parse_mus_stop (struct multi_user_stride_parser *parser, struct user_stride *s, error *e)
{
  i32 num;
  if (parser_maybe_parse_integer (parser->base, &num)) {
    s->stop = num;
    s->present |= STOP_PRESENT;
  }

  return parse_mus_step (parser, s, e);
}

static err_t
parse_entry (struct multi_user_stride_parser *parser, error *e)
{
  struct user_stride s = {0};

  // Optional start integer
  i32                num;
  if (parser_maybe_parse_integer (parser->base, &num)) {
    s.start = num;
    s.present |= START_PRESENT;

    // Bare number with no colon â†’ single index
    if (!parser_match (parser->base, TT_COLON)) {
      return musb_accept_key (&parser->builder, s, e);
    }

    s.present |= COLON_PRESENT;
    parser_advance (parser->base);
    WRAP (parse_mus_stop (parser, &s, e));
    return musb_accept_key (&parser->builder, s, e);
  }

  // No leading number â€” must be ':'
  if (parser_match (parser->base, TT_COLON)) {
    s.present |= COLON_PRESENT;
    parser_advance (parser->base);
    WRAP (parse_mus_stop (parser, &s, e));
    return musb_accept_key (&parser->builder, s, e);
  }

  return error_causef (e, ERR_SYNTAX, "Expected number or ':' at position %u", parser->base->pos);
}

static err_t
parse_multi_user_stride_inner (struct multi_user_stride_parser *parser, error *e)
{
  // Check for empty: []
  if (parser_match (parser->base, TT_RIGHT_BRACKET)) {
    return SUCCESS;
  }

  WRAP (parse_entry (parser, e));

  while (parser_match (parser->base, TT_COMMA)) {
    parser_advance (parser->base);
    WRAP (parse_entry (parser, e));
  }

  return SUCCESS;
}

err_t
parse_multi_user_stride (struct parser *parser, struct multi_user_stride *dest, error *e)
{
  struct multi_user_stride_parser p = {
      .base    = parser,
      .builder = musb_create (parser->b),
  };

  if (unlikely ((parser_expect (p.base, TT_LEFT_BRACKET, e)) < SUCCESS)) {
    goto theend;
  }
  if (unlikely ((parse_multi_user_stride_inner (&p, e)) < SUCCESS)) {
    goto theend;
  }
  if (unlikely ((parser_expect (p.base, TT_RIGHT_BRACKET, e)) < SUCCESS)) {
    goto theend;
  }
  if (unlikely ((musb_build (dest, &p.builder, e)) < SUCCESS)) {
    goto theend;
  }

theend:
  return error_trace (e);
}

err_t
compile_multi_user_stride (
    struct multi_user_stride *dest,
    const char               *text,
    struct allocator         *dalloc,
    error                    *e
)
{
  BUILDER_INIT (b, dalloc);

  struct lexer lex;
  if (lex_tokens (text, &b.temp, strlen (text), &lex, e) < 0) {
    goto theend;
  }

  struct parser parser = parser_init (lex.tokens, &b, lex.ntokens);

  if (parse_multi_user_stride (&parser, dest, e)) {
    goto theend;
  }

theend:
  BUILDER_CLOSE (b);
  return error_trace (e);
}

#ifdef TESTING
TEST (compile_multi_user_stride)
{
  ALLOC_INIT (alloc);

  error                    e      = error_create ();
  struct multi_user_stride stride = {0};

  // -------------------------------------------------------------------------
  // Empty array
  // -------------------------------------------------------------------------
  TEST_CASE ("[ ]")
  {
    compile_multi_user_stride (&stride, "[]", &alloc, &e);
    test_assert_int_equal (stride.len, 0);
    test_assert_equal (stride.strides, NULL);
    compile_multi_user_stride (&stride, "[ ]", &alloc, &e);
    test_assert_int_equal (stride.len, 0);
    test_assert_equal (stride.strides, NULL);
    compile_multi_user_stride (&stride, " [ ]", &alloc, &e);
    test_assert_int_equal (stride.len, 0);
    test_assert_equal (stride.strides, NULL);
    compile_multi_user_stride (&stride, " [] ", &alloc, &e);
    test_assert_int_equal (stride.len, 0);
    test_assert_equal (stride.strides, NULL);
  }

  // -------------------------------------------------------------------------
  // Single bare index
  // -------------------------------------------------------------------------
  TEST_CASE ("[ 0 ]")
  {
    compile_multi_user_stride (&stride, "[0]", &alloc, &e);
    test_assert_int_equal (stride.len, 1);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (stride.strides[0].present, START_PRESENT);
    test_assert_int_equal (stride.strides[0].start, 0);
  }

  // -------------------------------------------------------------------------
  // Two bare indices
  // -------------------------------------------------------------------------
  TEST_CASE ("[ 0, 0 ]")
  {
    compile_multi_user_stride (&stride, "[0, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    // first
    test_assert_int_equal (stride.strides[0].present, START_PRESENT);
    test_assert_int_equal (stride.strides[0].start, 0);
    // second
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // -------------------------------------------------------------------------
  // start + colon, no stop, no step  â†’  "0:"
  // -------------------------------------------------------------------------
  TEST_CASE ("[ 0: ]")
  {
    compile_multi_user_stride (&stride, "[0:]", &alloc, &e);
    test_assert_int_equal (stride.len, 1);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (stride.strides[0].present, START_PRESENT | COLON_PRESENT);
    test_assert_int_equal (stride.strides[0].start, 0);
  }

  // -------------------------------------------------------------------------
  // "0:" followed by a bare index
  // -------------------------------------------------------------------------
  TEST_CASE ("[ 0:, 0 ]")
  {
    compile_multi_user_stride (&stride, "[0:, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (stride.strides[0].present, START_PRESENT | COLON_PRESENT);
    test_assert_int_equal (stride.strides[0].start, 0);
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // -------------------------------------------------------------------------
  // Colon only  â†’  ":"
  // -------------------------------------------------------------------------
  TEST_CASE ("[ :, 0 ]")
  {
    compile_multi_user_stride (&stride, "[:, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (stride.strides[0].present, COLON_PRESENT);
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // -------------------------------------------------------------------------
  // Colon + stop  â†’  ":0"
  // -------------------------------------------------------------------------
  TEST_CASE ("[ :0, 0 ]")
  {
    compile_multi_user_stride (&stride, "[:0, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (stride.strides[0].present, COLON_PRESENT | STOP_PRESENT);
    test_assert_int_equal (stride.strides[0].stop, 0);
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // -------------------------------------------------------------------------
  // Double colon, no numbers  â†’  "::"
  // parse_entry sets COLON_PRESENT, parse_step also matches ':' and sets it
  // again (no-op), no integers â†’ only COLON_PRESENT in present
  // -------------------------------------------------------------------------
  TEST_CASE ("[ ::, 0 ]")
  {
    compile_multi_user_stride (&stride, "[::, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (stride.strides[0].present, COLON_PRESENT);
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // -------------------------------------------------------------------------
  // Double colon + step  â†’  "::0"
  // COLON_PRESENT (from first ':') | STEP_PRESENT (from "::0")
  // Note: no START_PRESENT, no STOP_PRESENT
  // -------------------------------------------------------------------------
  TEST_CASE ("[ ::0, 0 ]")
  {
    compile_multi_user_stride (&stride, "[::0, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (stride.strides[0].present, COLON_PRESENT | STEP_PRESENT);
    test_assert_int_equal (stride.strides[0].step, 0);
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // -------------------------------------------------------------------------
  // Colon + stop + colon, no step  â†’  ":0:"
  // -------------------------------------------------------------------------
  TEST_CASE ("[ :0:, 0 ]")
  {
    compile_multi_user_stride (&stride, "[:0:, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (stride.strides[0].present, COLON_PRESENT | STOP_PRESENT);
    test_assert_int_equal (stride.strides[0].stop, 0);
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // -------------------------------------------------------------------------
  // Colon + stop + colon + step  â†’  ":0:0"
  // -------------------------------------------------------------------------
  TEST_CASE ("[ :0:0, 0 ]")
  {
    compile_multi_user_stride (&stride, "[:0:0, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (stride.strides[0].present, COLON_PRESENT | STOP_PRESENT | STEP_PRESENT);
    test_assert_int_equal (stride.strides[0].stop, 0);
    test_assert_int_equal (stride.strides[0].step, 0);
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // -------------------------------------------------------------------------
  // Start + double colon, no stop/step  â†’  "0::"
  // -------------------------------------------------------------------------
  TEST_CASE ("[ 0::, 0 ]")
  {
    compile_multi_user_stride (&stride, "[0::, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (stride.strides[0].present, START_PRESENT | COLON_PRESENT);
    test_assert_int_equal (stride.strides[0].start, 0);
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // -------------------------------------------------------------------------
  // Start + double colon + step  â†’  "0::0"
  // -------------------------------------------------------------------------
  TEST_CASE ("[ 0::0, 0 ]")
  {
    compile_multi_user_stride (&stride, "[0::0, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (stride.strides[0].present, START_PRESENT | COLON_PRESENT | STEP_PRESENT);
    test_assert_int_equal (stride.strides[0].start, 0);
    test_assert_int_equal (stride.strides[0].step, 0);
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // -------------------------------------------------------------------------
  // Full slice  â†’  "0:0:0"
  // -------------------------------------------------------------------------
  TEST_CASE ("[ 0:0:0, 0 ]")
  {
    compile_multi_user_stride (&stride, "[0:0:0, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (
        stride.strides[0].present,
        START_PRESENT | COLON_PRESENT | STOP_PRESENT | STEP_PRESENT
    );
    test_assert_int_equal (stride.strides[0].start, 0);
    test_assert_int_equal (stride.strides[0].stop, 0);
    test_assert_int_equal (stride.strides[0].step, 0);
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // =========================================================================
  // Error conditions
  // =========================================================================

  // Missing opening bracket
  TEST_CASE ("error: no leading '['")
  {
    err_t err = compile_multi_user_stride (&stride, "0, 1]", &alloc, &e);
    test_assert (err < SUCCESS);
    test_assert_int_equal (e.cause_code, ERR_SYNTAX);
    e.cause_code = 0;
    e.cmlen      = 0;
  }

  // Missing closing bracket
  TEST_CASE ("error: no trailing ']'")
  {
    err_t err = compile_multi_user_stride (&stride, "[0, 1", &alloc, &e);
    test_assert (err < SUCCESS);
    test_assert_int_equal (e.cause_code, ERR_SYNTAX);
    e.cause_code = 0;
    e.cmlen      = 0;
  }

  // Empty input
  TEST_CASE ("error: empty string")
  {
    err_t err = compile_multi_user_stride (&stride, "", &alloc, &e);
    test_assert (err < SUCCESS);
    test_assert_int_equal (e.cause_code, ERR_SYNTAX);
    e.cause_code = 0;
    e.cmlen      = 0;
  }

  // Trailing comma with no entry after it  â†’  parse_entry gets ']', not a
  // number or ':', so it returns ERR_SYNTAX
  TEST_CASE ("error: trailing comma '[0,]'")
  {
    err_t err = compile_multi_user_stride (&stride, "[0,]", &alloc, &e);
    test_assert (err < SUCCESS);
    test_assert_int_equal (e.cause_code, ERR_SYNTAX);
    e.cause_code = 0;
    e.cmlen      = 0;
  }

  // Leading comma parse_entry gets ',' which is neither number nor ':'
  TEST_CASE ("error: leading comma '[,0]'")
  {
    err_t err = compile_multi_user_stride (&stride, "[,0]", &alloc, &e);
    test_assert (err < SUCCESS);
    test_assert_int_equal (e.cause_code, ERR_SYNTAX);
    e.cause_code = 0;
    e.cmlen      = 0;
  }

  // Bare comma between two commas
  TEST_CASE ("error: double comma '[0,,1]'")
  {
    err_t err = compile_multi_user_stride (&stride, "[0,,1]", &alloc, &e);
    test_assert (err < SUCCESS);
    test_assert_int_equal (e.cause_code, ERR_SYNTAX);
    e.cause_code = 0;
    e.cmlen      = 0;
  }

  // Garbage token (not number, colon, comma, or bracket)
  TEST_CASE ("error: garbage token '[abc]'")
  {
    err_t err = compile_multi_user_stride (&stride, "[abc]", &alloc, &e);
    test_assert (err < SUCCESS);
    test_assert_int_equal (e.cause_code, ERR_SYNTAX);
    e.cause_code = 0;
    e.cmlen      = 0;
  }

  // Completely wrong structure
  TEST_CASE ("error: only a number, no brackets")
  {
    err_t err = compile_multi_user_stride (&stride, "42", &alloc, &e);
    test_assert (err < SUCCESS);
    test_assert_int_equal (e.cause_code, ERR_SYNTAX);
    e.cause_code = 0;
    e.cmlen      = 0;
  }

  ALLOC_CLOSE (alloc);
}
#endif
