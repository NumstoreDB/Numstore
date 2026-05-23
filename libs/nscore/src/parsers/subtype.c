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

#include "nscore/parsers/subtype.h"

#include "nscore/parsers/multi_user_stride.h"

struct sub_type_parser
{
  struct parser      *base;
  struct subtype     *dest;
  struct chunk_alloc  temp;
  struct chunk_alloc *persistent;
};

// sub_type   ::= IDENT stride* ('.' IDENT stride*)*
static err_t
parse_sub_type_inner (struct sub_type_parser *parser, error *e)
{
  if (!parser_match (parser->base, TT_IDENTIFIER))
  {
    return error_causef (e, ERR_SYNTAX, "Expected variable name at position %u", parser->base->pos);
  }

  // VNAME
  struct token *tok   = parser_advance (parser->base);
  struct string vname = {.data = tok->str.data, .len = tok->str.len};

  // Type accessors
  struct type_accessor_builder tab;
  tab_create (&tab, &parser->temp, parser->persistent);
  while (true)
  {
    // Stride
    if (parser_match (parser->base, TT_LEFT_BRACKET))
    {
      struct multi_user_stride stride;
      WRAP (parse_multi_user_stride (parser->base, &stride, parser->persistent, e));
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

  return subtype_create (parser->dest, vname, ta, e);
}

err_t
parse_subtype (struct parser *p, struct subtype *dest, struct chunk_alloc *dalloc, error *e)
{
  struct sub_type_parser parser = {
      .base       = p,
      .dest       = dest,
      .persistent = dalloc,
  };

  chunk_alloc_create_default (&parser.temp);

  err_t rc = parse_sub_type_inner (&parser, e);

  chunk_alloc_free_all (&parser.temp);

  return rc;
}
