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

#include "alloc.h"
#include "collections.h"
#include "compiler.h"
#include "error.h"
#include "query.h"
#include "testing.h"
#include "types.h"

/******************************************************************************
 * SECTION: Sub Type
 * ----------------------------------------------------------------------------
 * subtype ::= IDENT ( multi_user_stride | '.' IDENT )*
 ******************************************************************************/

struct sub_type_parser
{
  struct parser  *base;
  struct subtype *dest;
};

static err_t
parse_sub_type_inner (struct sub_type_parser *parser, error *e)
{
  if (!parser_match (parser->base, TT_IDENTIFIER))
  {
    return error_causef (
        e,
        ERR_SYNTAX,
        "Expected variable name at position %u",
        parser->base->pos
    );
  }

  // VNAME
  struct token *tok  = parser_advance (parser->base);
  struct string name = {.data = tok->str.data, .len = tok->str.len};

  // Type accessors
  struct type_accessor_builder tab = tab_create (parser->base->b);
  while (true)
  {
    // Stride
    if (parser_match (parser->base, TT_LEFT_BRACKET))
    {
      struct multi_user_stride stride;
      WRAP (parse_multi_user_stride (parser->base, &stride, e));
      for (u32 i = 0; i < stride.len; ++i)
      {
        WRAP (tab_accept_stride (&tab, stride.strides[i], e));
      }
    }

    // Dot
    else if (parser_match (parser->base, TT_DOT))
    {
      parser_advance (parser->base);
      if (!parser_match (parser->base, TT_IDENTIFIER))
      {
        return error_causef (
            e,
            ERR_SYNTAX,
            "Expected "
            "identifier at "
            "position %u",
            parser->base->pos
        );
      }

      tok                  = parser_advance (parser->base);
      struct string select = {
          .data = tok->str.data,
          .len  = tok->str.len,
      };
      WRAP (tab_accept_select (&tab, select, e));
    }

    // Done
    else
    {
      break;
    }
  }

  struct type_accessor ta;
  WRAP (tab_build (&ta, &tab, e));

  *parser->dest = subtype_create (name, ta);

  return SUCCESS;
}

static err_t
parse_subtype (struct parser *p, struct subtype *dest, error *e)
{
  struct sub_type_parser parser = {
      .base = p,
      .dest = dest,
  };

  err_t rc = parse_sub_type_inner (&parser, e);

  return rc;
}

err_t
compile_subtype (
    struct subtype   *dest,
    const char       *text,
    struct allocator *dalloc,
    error            *e
)
{
  BUILDER_INIT (b, dalloc);

  struct lexer lex;
  if (lex_tokens (text, &b.temp, strlen (text), &lex, e))
  {
    goto theend;
  }

  struct parser parser = parser_init (lex.tokens, &b, lex.ntokens);

  if (parse_subtype (&parser, dest, e))
  {
    goto theend;
  }

theend:
  BUILDER_CLOSE (b);
  return error_trace (e);
}

#ifdef TESTING
static void
test_compile_subtype_green_path (const char *query, struct subtype expected)
{
  ALLOC_INIT (alloc);

  struct subtype actual;
  error          e = error_create ();

  TEST_CASE ("SHOULD PASS: %s", query)
  {
    test_assert_equal (compile_subtype (&actual, query, &alloc, &e), SUCCESS);
    test_assert (subtype_equal (&expected, &actual));
  }

  ALLOC_CLOSE (alloc);
}

static void
test_compile_subtype_red_path (const char *query, err_t code)
{
  ALLOC_INIT (alloc);

  struct subtype actual;
  error          e = error_create ();

  TEST_CASE ("SHOULD FAIL: %s", query)
  {
    test_err_t_check (compile_subtype (&actual, query, &alloc, &e), code, &e);
  }

  ALLOC_CLOSE (alloc);
}

TEST (compile_subtype)
{
  test_compile_subtype_green_path (
      "myvar",
      subtype_create (strfcstr ("myvar"), ta_take ())
  );

  test_compile_subtype_green_path (
      "myvar[9]",
      subtype_create (
          strfcstr ("myvar"),
          ta_range ((struct user_stride[]){ustride_single (9)}, 1, &ta_take ())
      )
  );

  test_compile_subtype_green_path (
      "myvar.field",
      subtype_create (
          strfcstr ("myvar"),
          ta_select (strfcstr ("field"), &ta_take ())
      )
  );

  struct type_accessor subrange =
      ta_range ((struct user_stride[]){ustride_single (0)}, 1, &ta_take ());

  test_compile_subtype_green_path (
      "myvar.a[0]",
      subtype_create (strfcstr ("myvar"), ta_select (strfcstr ("a"), &subrange))
  );

  test_compile_subtype_red_path ("", ERR_SYNTAX);
  test_compile_subtype_red_path ("42", ERR_SYNTAX);
  test_compile_subtype_red_path ("[0]", ERR_SYNTAX);
  test_compile_subtype_red_path ("struct a x }", ERR_SYNTAX);
  test_compile_subtype_red_path ("foo.", ERR_SYNTAX);
  test_compile_subtype_red_path (".bar", ERR_SYNTAX);
}
#endif
