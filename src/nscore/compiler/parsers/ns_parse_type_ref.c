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
#include "core/ns_string.h"
#include "core/testing/ns_testing.h"
#include "nscore/compiler/ns_compiler.h"
#include "nscore/compiler/ns_lexer.h"
#include "nscore/compiler/parsers/ns_parser.h"

/******************************************************************************
 * SECTION: Type Ref
 * ----------------------------------------------------------------------------
 * type_ref        ::= struct_type_ref | take_type_ref
 * struct_type_ref ::= 'struct' '{' field_ref ( ',' field_ref )* '}'
 * field_ref       ::= IDENT type_ref
 * take_type_ref   ::= subtype
 ******************************************************************************/

struct type_ref_parser
{
  struct parser   *base;
  struct type_ref *dest;
};

static err_t parse_type_ref_inner (struct type_ref_parser *parser, struct type_ref *out, error *e);

static err_t
parse_take_type_ref (struct type_ref_parser *parser, struct type_ref *out, error *e)
{
  struct subtype st;
  WRAP (parse_subtype (parser->base, &st, e));

  out->type     = TR_TAKE;
  out->tk.vname = st.vname;
  out->tk.ta    = st.ta;

  return SUCCESS;
}

static err_t
parse_field_ref (struct kvt_ref_list_builder *builder, struct type_ref_parser *parser, error *e)
{
  // IDENT
  if (!parser_match (parser->base, TT_IDENTIFIER))
  {
    return error_causef (e, ERR_SYNTAX, "Expected identifier at position %u", parser->base->pos);
  }

  struct token *tok = parser_advance (parser->base);
  WRAP (kvrlb_accept_key (
      builder,
      (struct string){
          .data = (char *)tok->str.data,
          .len  = tok->str.len,
      },
      e
  ));

  // Type ref
  struct type_ref inner;
  WRAP (parse_type_ref_inner (parser, &inner, e));
  WRAP (kvrlb_accept_type (builder, inner, e));

  return SUCCESS;
}

static err_t
parse_struct_type_ref (struct type_ref_parser *parser, struct type_ref *out, error *e)
{
  // 'struct'
  WRAP (parser_expect (parser->base, TT_STRUCT, e));

  // '{ '
  WRAP (parser_expect (parser->base, TT_LEFT_BRACE, e));

  struct kvt_ref_list_builder builder = kvrlb_create (parser->base->b);

  WRAP (parse_field_ref (&builder, parser, e));

  while (parser_match (parser->base, TT_COMMA))
  {
    parser_advance (parser->base);
    WRAP (parse_field_ref (&builder, parser, e));
  }

  WRAP (parser_expect (parser->base, TT_RIGHT_BRACE, e));

  // Build kvt_ref list
  struct kvt_ref_list list;
  WRAP (kvrlb_build (&list, &builder, e));

  out->type = TR_STRUCT;
  out->st   = (struct struct_tr){
      .len   = list.len,
      .keys  = list.keys,
      .types = list.types,
  };

  return SUCCESS;
}

static err_t
parse_type_ref_inner (struct type_ref_parser *parser, struct type_ref *out, error *e)
{
  struct token *tok = parser_peek (parser->base);

  switch (tok->type)
  {
    case TT_STRUCT:
    {
      return parse_struct_type_ref (parser, out, e);
    }
    case TT_IDENTIFIER:
    {
      return parse_take_type_ref (parser, out, e);
    }
    default:
    {
      return error_causef (
          e,
          ERR_SYNTAX,
          "Expected type_ref (struct or identifier) "
          "at "
          "position %u, got token type %s",
          parser->base->pos,
          tt_tostr (tok->type)
      );
    }
  }
}

err_t
parse_type_ref (struct parser *p, struct type_ref *dest, error *e)
{
  struct type_ref_parser parser = {
      .base = p,
      .dest = dest,
  };

  if (unlikely ((parse_type_ref_inner (&parser, parser.dest, e)) < SUCCESS))
  {
    goto theend;
  }

theend:
  return error_trace (e);
}

err_t
compile_type_ref (struct type_ref *dest, const char *text, struct allocator *dalloc, error *e)
{
  BUILDER_INIT (b, dalloc);

  struct lexer lex;
  if (lex_tokens (text, &b.temp, strlen (text), &lex, e) < 0)
  {
    goto theend;
  }

  struct parser parser = parser_init (lex.tokens, &b, lex.ntokens);

  if (parse_type_ref (&parser, dest, e))
  {
    goto theend;
  }

theend:
  BUILDER_CLOSE (b);
  return error_trace (e);
}

#ifdef TESTING

static void
test_compile_type_ref_green_path (const char *query, struct type_ref expected)
{
  ALLOC_INIT (alloc);

  struct type_ref actual;
  error           e = error_create ();

  TEST_CASE ("SHOULD PASS: %s", query)
  {
    test_assert_equal (compile_type_ref (&actual, query, &alloc, &e), SUCCESS);
    test_assert (type_ref_equal (expected, actual));
  }

  ALLOC_CLOSE (alloc);
}

static void
test_compile_type_ref_red_path (const char *query, err_t code)
{
  ALLOC_INIT (alloc);

  struct type_ref actual;
  error           e = error_create ();

  TEST_CASE ("SHOULD FAIL: %s", query)
  {
    test_err_t_check (compile_type_ref (&actual, query, &alloc, &e), code, &e);
  }

  ALLOC_CLOSE (alloc);
}

TEST (compile_type_ref)
{
  test_compile_type_ref_green_path ("myvar", tr_take (strfcstr ("myvar"), ta_take ()));

  test_compile_type_ref_green_path (
      "myvar[9]",
      tr_take (
          strfcstr ("myvar"),
          ta_range ((struct user_stride[]){ustride_single (9)}, 1, &ta_take ())
      )
  );

  test_compile_type_ref_green_path (
      "myvar.field",
      tr_take (strfcstr ("myvar"), ta_select (strfcstr ("field"), &ta_take ()))
  );

  struct type_accessor subrange =
      ta_range ((struct user_stride[]){ustride_single (0)}, 1, &ta_take ());
  test_compile_type_ref_green_path (
      "myvar.a[0]",
      tr_take (strfcstr ("myvar"), ta_select (strfcstr ("a"), &subrange))
  );

  test_compile_type_ref_green_path (
      "struct { a myvar }",
      tr_struct (
          1,
          (struct string[]){strfcstr ("a")},
          (struct type_ref[]){tr_take (strfcstr ("myvar"), ta_take ())}
      )
  );

  test_compile_type_ref_green_path (
      "struct { a x, b y }",
      tr_struct (
          2,
          (struct string[]){strfcstr ("a"), strfcstr ("b")},
          (struct type_ref[]){
              tr_take (strfcstr ("x"), ta_take ()),
              tr_take (strfcstr ("y"), ta_take ()),
          }
      )
  );

  test_compile_type_ref_green_path (
      "struct { a x, b y, c z }",
      tr_struct (
          3,
          (struct string[]){strfcstr ("a"), strfcstr ("b"), strfcstr ("c")},
          (struct type_ref[]){
              tr_take (strfcstr ("x"), ta_take ()),
              tr_take (strfcstr ("y"), ta_take ()),
              tr_take (strfcstr ("z"), ta_take ()),
          }
      )
  );

  test_compile_type_ref_green_path (
      "struct { a struct { b x } }",
      tr_struct (
          1,
          (struct string[]){strfcstr ("a")},
          (struct type_ref[]){
              tr_struct (
                  1,
                  (struct string[]){strfcstr ("b")},
                  (struct type_ref[]){tr_take (strfcstr ("x"), ta_take ())}
              ),
          }
      )
  );

  test_compile_type_ref_green_path (
      "struct { a myvar[0] }",
      tr_struct (
          1,
          (struct string[]){strfcstr ("a")},
          (struct type_ref[]){tr_take (
              strfcstr ("myvar"),
              ta_range ((struct user_stride[]){ustride_single (0)}, 1, &ta_take ())
          )}
      )
  );

  test_compile_type_ref_red_path ("", ERR_SYNTAX);
  test_compile_type_ref_red_path ("42", ERR_SYNTAX);
  test_compile_type_ref_red_path ("[0]", ERR_SYNTAX);
  test_compile_type_ref_red_path ("struct a x }", ERR_SYNTAX);
  test_compile_type_ref_red_path ("struct {}", ERR_SYNTAX);
  test_compile_type_ref_red_path ("struct { a }", ERR_SYNTAX);
  test_compile_type_ref_red_path ("struct { a x", ERR_SYNTAX);
  test_compile_type_ref_red_path ("struct { a x, }", ERR_SYNTAX);
  test_compile_type_ref_red_path ("struct { a x,, b y }", ERR_SYNTAX);
  test_compile_type_ref_red_path ("struct { a x, b }", ERR_SYNTAX);
  test_compile_type_ref_red_path ("struct { a x, struct y }", ERR_SYNTAX);
}

#endif
