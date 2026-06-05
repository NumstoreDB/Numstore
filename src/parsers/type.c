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

#include "nscore/parsers/type.h"

struct type_parser
{
  struct parser      *base;
  struct type        *dest;
  struct chunk_alloc  temp;
  struct chunk_alloc *persistent;
};

static err_t
parse_type_inner (struct type_parser *parser, struct type *out, error *e);

// primitive_type ::= PRIM
static err_t
parse_primitive_type (struct type_parser *parser, struct type *out, error *e)
{
  if (!parser_match (parser->base, TT_PRIM))
  {
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

// sarray_type ::= '[' INTEGER ']'+ type
static err_t
parse_sarray_type (struct type_parser *parser, struct type *out, error *e)
{
  err_t err;

  struct sarray_builder builder;
  sab_create (&builder, &parser->temp, parser->persistent);

  // Parse all [N] brackets
  while (parser_match (parser->base, TT_LEFT_BRACKET))
  {
    WRAP (parser_expect (parser->base, TT_LEFT_BRACKET, e));

    if (!parser_match (parser->base, TT_INTEGER))
    {
      return error_causef (
          e,
          ERR_SYNTAX,
          "Expected array size at position "
          "%u",
          parser->base->pos
      );
    }

    struct token *tok = parser_advance (parser->base);

    WRAP (sab_accept_dim (&builder, tok->integer, e));
    WRAP (parser_expect (parser->base, TT_RIGHT_BRACKET, e));
  }

  // Inner most type
  struct type *inner = chunk_malloc (parser->persistent, 1, sizeof *inner, e);
  if (inner == NULL)
  {
    return error_trace (e);
  }
  WRAP (parse_type_inner (parser, inner, e));
  WRAP (sab_accept_type (&builder, inner, e));

  out->type = T_SARRAY;
  return sab_build (&out->sa, &builder, e);
}

// field           ::= IDENTIFIER type
static err_t
parse_field (
    struct kvt_list_builder *builder,
    struct type_parser      *parser,
    error                   *e
)
{
  // IDENT
  if (!parser_match (parser->base, TT_IDENTIFIER))
  {
    return error_causef (
        e,
        ERR_SYNTAX,
        "Expected identifier at position %u",
        parser->base->pos
    );
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
  struct type *inner = chunk_malloc (parser->persistent, 1, sizeof *inner, e);
  if (inner == NULL)
  {
    return error_trace (e);
  }
  WRAP (parse_type_inner (parser, inner, e));
  WRAP (kvlb_accept_type (builder, inner, e));

  return SUCCESS;
}

// struct_type     ::= 'struct' '{' IDENT type (',' IDENT type)* '}'
static err_t
parse_struct_type (struct type_parser *parser, struct type *out, error *e)
{
  err_t err;

  // 'struct'
  WRAP (parser_expect (parser->base, TT_STRUCT, e));

  // '{ '
  WRAP (parser_expect (parser->base, TT_LEFT_BRACE, e));

  struct kvt_list_builder builder;
  kvlb_create (&builder, &parser->temp, parser->persistent);

  WRAP (parse_field (&builder, parser, e));

  while (parser_match (parser->base, TT_COMMA))
  {
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

// union_type     ::= 'union' '{' IDENT type (',' IDENT type)* '}'
static err_t
parse_union_type (struct type_parser *parser, struct type *out, error *e)
{
  err_t err;

  // 'union'
  WRAP (parser_expect (parser->base, TT_UNION, e));

  // '{ '
  WRAP (parser_expect (parser->base, TT_LEFT_BRACE, e));

  struct kvt_list_builder builder;
  kvlb_create (&builder, &parser->temp, parser->persistent);

  WRAP (parse_field (&builder, parser, e));

  while (parser_match (parser->base, TT_COMMA))
  {
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

// type ::= struct_type | union_type | sarray_type | primitive_type
static err_t
parse_type_inner (struct type_parser *parser, struct type *out, error *e)
{
  struct token *tok = parser_peek (parser->base);

  switch (tok->type)
  {
    case TT_STRUCT:
    {
      return parse_struct_type (parser, out, e);
    }
    case TT_UNION:
    {
      return parse_union_type (parser, out, e);
    }
    case TT_LEFT_BRACKET:
    {
      return parse_sarray_type (parser, out, e);
    }
    case TT_PRIM:
    {
      return parse_primitive_type (parser, out, e);
    }
    default:
    {
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
parse_type (
    struct parser      *p,
    struct type        *dest,
    struct chunk_alloc *dalloc,
    error              *e
)
{
  struct type_parser parser = {
      .base       = p,
      .dest       = dest,
      .persistent = dalloc,
  };

  chunk_alloc_create_default (&parser.temp);

  if (unlikely ((parse_type_inner (&parser, parser.dest, e)) < SUCCESS))
  {
    goto theend;
  }

theend:
  chunk_alloc_free_all (&parser.temp);
  return error_trace (e);
}
