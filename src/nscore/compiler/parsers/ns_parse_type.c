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
#include "core/ns_string.h"
#include "core/testing/ns_testing.h"
#include "nscore/compiler/ns_compiler.h"
#include "nscore/compiler/ns_lexer.h"
#include "nscore/compiler/ns_tokens.h"
#include "nscore/compiler/parsers/ns_parser.h"
#include "nscore/types/ns_kvt.h"
#include "nscore/types/ns_sarray_t.h"
#include "nscore/types/ns_struct_t.h"
#include "nscore/types/ns_types.h"
#include "nscore/types/ns_union_t.h"

#include <string.h>

/******************************************************************************
 * SECTION: Type
 * ----------------------------------------------------------------------------
 * type           ::= struct_type | union_type | sarray_type | primitive_type
 * primitive_type ::= PRIM
 * sarray_type    ::= ( '[' INTEGER ']' )+ type
 * struct_type    ::= 'struct' '{' field ( ',' field )* '}'
 * union_type     ::= 'union'  '{' field ( ',' field )* '}'
 * field          ::= IDENT type
 ******************************************************************************/

struct type_parser
{
  struct parser *base;
  struct type   *dest;
};

static err_t parse_type_inner (struct type_parser *parser, struct type *out, error *e);

static err_t
parse_primitive_type (struct type_parser *parser, struct type *out, error *e)
{
  if (!parser_match (parser->base, TT_PRIM)) {
    return error_causef (
        e,
        ERR_SYNTAX,
        "Expected primitive type at position %u",
        parser->base->pos
    );
  }

  struct token *tok = parser_advance (parser->base);
  out->type         = T_PRIM;
  out->p            = tok->prim;

  return SUCCESS;
}

static err_t
parse_sarray_type (struct type_parser *parser, struct type *out, error *e)
{
  struct sarray_builder builder = sab_create (parser->base->b);

  // Parse all [N] brackets
  while (parser_match (parser->base, TT_LEFT_BRACKET)) {
    WRAP (parser_expect (parser->base, TT_LEFT_BRACKET, e));

    i32 num;
    if (!parser_maybe_parse_integer (parser->base, &num)) {
      return error_causef (
          e,
          ERR_SYNTAX,
          "Expected array size at position "
          "%u",
          parser->base->pos
      );
    }

    WRAP (sab_accept_dim (&builder, num, e));
    WRAP (parser_expect (parser->base, TT_RIGHT_BRACKET, e));
  }

  // Inner most type
  struct type *inner = builder_malloc_persist (parser->base->b, 1, sizeof *inner, e);
  if (inner == NULL) {
    return error_trace (e);
  }
  WRAP (parse_type_inner (parser, inner, e));
  WRAP (sab_accept_type (&builder, inner, e));

  out->type = T_SARRAY;
  return sab_build (&out->sa, &builder, e);
}

static err_t
parse_field (struct kvt_list_builder *builder, struct type_parser *parser, error *e)
{
  // IDENT
  if (!parser_match (parser->base, TT_IDENTIFIER)) {
    return error_causef (e, ERR_SYNTAX, "Expected identifier at position %u", parser->base->pos);
  }

  struct token *tok = parser_advance (parser->base);
  WRAP (kvlb_accept_key (
      builder,
      (struct string){
          .data = (char *)tok->str.data,
          .len  = tok->str.len,
      },
      e
  ));

  // Type
  struct type *inner = builder_malloc_persist (parser->base->b, 1, sizeof *inner, e);
  if (inner == NULL) {
    return error_trace (e);
  }
  WRAP (parse_type_inner (parser, inner, e));
  WRAP (kvlb_accept_type (builder, inner, e));

  return SUCCESS;
}

static err_t
parse_struct_type (struct type_parser *parser, struct type *out, error *e)
{
  // 'struct'
  WRAP (parser_expect (parser->base, TT_STRUCT, e));

  // '{ '
  WRAP (parser_expect (parser->base, TT_LEFT_BRACE, e));

  struct kvt_list_builder builder = kvlb_create (parser->base->b);

  WRAP (parse_field (&builder, parser, e));

  while (parser_match (parser->base, TT_COMMA)) {
    parser_advance (parser->base);
    WRAP (parse_field (&builder, parser, e));
  }

  WRAP (parser_expect (parser->base, TT_RIGHT_BRACE, e));

  // Build kvt list
  struct kvt_list list;
  WRAP (kvlb_build (&list, &builder, e));

  out->type = T_STRUCT;

  return struct_t_create (&out->st, list, NULL, e);
}

static err_t
parse_union_type (struct type_parser *parser, struct type *out, error *e)
{
  // 'union'
  WRAP (parser_expect (parser->base, TT_UNION, e));

  // '{ '
  WRAP (parser_expect (parser->base, TT_LEFT_BRACE, e));

  struct kvt_list_builder builder = kvlb_create (parser->base->b);

  WRAP (parse_field (&builder, parser, e));

  while (parser_match (parser->base, TT_COMMA)) {
    parser_advance (parser->base);
    WRAP (parse_field (&builder, parser, e));
  }

  WRAP (parser_expect (parser->base, TT_RIGHT_BRACE, e));

  // Build kvt list
  struct kvt_list list;
  WRAP (kvlb_build (&list, &builder, e));

  out->type = T_UNION;

  return union_t_create (&out->un, list, NULL, e);
}

static err_t
parse_type_inner (struct type_parser *parser, struct type *out, error *e)
{
  struct token *tok = parser_peek (parser->base);

  switch (tok->type) {
    case TT_STRUCT: {
      return parse_struct_type (parser, out, e);
    }
    case TT_UNION: {
      return parse_union_type (parser, out, e);
    }
    case TT_LEFT_BRACKET: {
      return parse_sarray_type (parser, out, e);
    }
    case TT_PRIM: {
      return parse_primitive_type (parser, out, e);
    }
    default: {
      return error_causef (
          e,
          ERR_SYNTAX,
          "Expected type at position %u, got token "
          "type %s",
          parser->base->pos,
          tt_tostr (tok->type)
      );
    }
  }
}

err_t
parse_type (struct parser *p, struct type *dest, error *e)
{
  struct type_parser parser = {
      .base = p,
      .dest = dest,
  };

  if (unlikely ((parse_type_inner (&parser, parser.dest, e)) < SUCCESS)) {
    goto theend;
  }

theend:
  return error_trace (e);
}

err_t
compile_type (struct type *dest, const char *text, struct allocator *dalloc, error *e)
{
  BUILDER_INIT (b, dalloc);

  struct lexer lex;
  if (lex_tokens (text, &b.temp, strlen (text), &lex, e)) {
    goto theend;
  }

  struct parser parser = parser_init (lex.tokens, &b, lex.ntokens);

  if (parse_type (&parser, dest, e)) {
    goto theend;
  }

theend:
  BUILDER_CLOSE (b);
  return error_trace (e);
}

#ifdef TESTING

static void
test_compile_type_green_path (const char *query, struct type expected)
{
  ALLOC_INIT (alloc);

  struct type actual;
  error       e = error_create ();

  TEST_CASE ("SHOULD PASS: %s", query)
  {
    test_assert_equal (compile_type (&actual, query, &alloc, &e), SUCCESS);
    i_log_type (&expected, &e);
    i_log_type (&actual, &e);
    test_assert (type_equal (&expected, &actual));
  }

  ALLOC_CLOSE (alloc);
}

static void
test_compile_type_red_path (const char *query, err_t code)
{
  ALLOC_INIT (alloc);

  struct type actual;
  error       e = error_create ();

  TEST_CASE ("SHOULD FAIL: %s", query)
  {
    test_err_t_check (compile_type (&actual, query, &alloc, &e), code, &e);
  }

  ALLOC_CLOSE (alloc);
}

TEST (compile_type)
{
  test_compile_type_green_path ("i8", TI8);
  test_compile_type_green_path ("i16", TI16);
  test_compile_type_green_path ("i32", TI32);
  test_compile_type_green_path ("i64", TI64);

  test_compile_type_green_path ("u8", TU8);
  test_compile_type_green_path ("u16", TU16);
  test_compile_type_green_path ("u32", TU32);
  test_compile_type_green_path ("u64", TU64);

  test_compile_type_green_path ("f16", TF16);
  test_compile_type_green_path ("f32", TF32);
  test_compile_type_green_path ("f64", TF64);
  test_compile_type_green_path ("f128", TF128);

  test_compile_type_green_path ("cf32", TCF32);
  test_compile_type_green_path ("cf64", TCF64);
  test_compile_type_green_path ("cf128", TCF128);
  test_compile_type_green_path ("cf256", TCF256);

  test_compile_type_green_path ("ci16", TCI16);
  test_compile_type_green_path ("ci32", TCI32);
  test_compile_type_green_path ("ci64", TCI64);
  test_compile_type_green_path ("ci128", TCI128);

  test_compile_type_green_path ("cu16", TCU16);
  test_compile_type_green_path ("cu32", TCU32);
  test_compile_type_green_path ("cu64", TCU64);
  test_compile_type_green_path ("cu128", TCU128);

  test_compile_type_red_path ("i2", ERR_SYNTAX);

  // SARRAY
  test_compile_type_green_path ("[10]i32", mk_sarray (1, (u32[]){10}, &TI32));
  test_compile_type_green_path ("[5][10]f64", mk_sarray (2, (u32[]){5, 10}, &TF64));
  test_compile_type_green_path ("[2][3][4]u8", mk_sarray (3, (u32[]){2, 3, 4}, &TU8));

  // STRUCT
  test_compile_type_green_path (
      "struct { x i32, y f64 }",
      mk_struct (
          2,
          (struct string[]){
              strfcstr ("x"),
              strfcstr ("y"),
          },
          (struct type *[]){
              &TI32,
              &TF64,
          }
      )
  );

  struct type inner = mk_struct (1, (struct string[]){strfcstr ("b")}, (struct type *[]){&TI32});
  test_compile_type_green_path (
      "struct { a struct { b i32 } }",
      mk_struct (1, (struct string[]){strfcstr ("a")}, (struct type *[]){&inner})
  );

  // UNION
  test_compile_type_green_path (
      "union { x i32, y f64 }",
      mk_union (
          2,
          (struct string[]){
              strfcstr ("x"),
              strfcstr ("y"),
          },
          (struct type *[]){
              &TI32,
              &TF64,
          }
      )
  );

  // COMPLICATED
  struct type inner_sarray = mk_sarray (1, (u32[]){5}, &TF64);
  struct type inner_struct = mk_struct (
      2,
      (struct string[]){strfcstr ("x"), strfcstr ("y")},
      (struct type *[]){&TI32, &inner_sarray}
  );
  test_compile_type_green_path (
      "[10]struct { x i32, y [5]f64 }",
      mk_sarray (1, (u32[]){10}, &inner_struct)
  );
}

#endif
