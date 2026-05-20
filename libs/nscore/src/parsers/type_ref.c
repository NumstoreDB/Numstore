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

#include "nscore/parsers/type_ref.h"

#include "nscore/parsers/subtype.h"

struct type_ref_parser {
  struct parser      *base;
  struct type_ref    *dest;
  struct chunk_alloc  temp;
  struct chunk_alloc *persistent;
};

static err_t parse_type_ref_inner (struct type_ref_parser *parser, struct type_ref *out, error *e);

// take_type_ref ::= subtype
static err_t parse_take_type_ref (struct type_ref_parser *parser, struct type_ref *out, error *e) {
  struct subtype st;
  WRAP (parse_subtype (parser->base, &st, parser->persistent, e));

  out->type     = TR_TAKE;
  out->tk.vname = st.vname;
  out->tk.ta    = st.ta;

  return SUCCESS;
}

// field_ref       ::= IDENTIFIER type_ref
static err_t
parse_field_ref (struct kvt_ref_list_builder *builder, struct type_ref_parser *parser, error *e) {
  // IDENT
  if (!parser_match (parser->base, TT_IDENTIFIER)) {
    return error_causef (e, ERR_SYNTAX, "Expected identifier at position %u", parser->base->pos);
  }

  struct token *tok = parser_advance (parser->base);
  WRAP (kvrlb_accept_key (
      builder,
      (struct string){
          .data = (char *)tok->str.data,
          .len  = tok->str.len,
      },
      e));

  // Type ref
  struct type_ref inner;
  WRAP (parse_type_ref_inner (parser, &inner, e));
  WRAP (kvrlb_accept_type (builder, inner, e));

  return SUCCESS;
}

// struct_type_ref ::= 'struct' '{' IDENT type_ref (',' IDENT type_ref)* '}'
static err_t
parse_struct_type_ref (struct type_ref_parser *parser, struct type_ref *out, error *e) {
  err_t err;

  // 'struct'
  WRAP (parser_expect (parser->base, TT_STRUCT, e));

  // '{ '
  WRAP (parser_expect (parser->base, TT_LEFT_BRACE, e));

  struct kvt_ref_list_builder builder;
  kvrlb_create (&builder, &parser->temp, parser->persistent);

  WRAP (parse_field_ref (&builder, parser, e));

  while (parser_match (parser->base, TT_COMMA)) {
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

// type_ref ::= struct_type_ref | take_type_ref
static err_t parse_type_ref_inner (struct type_ref_parser *parser, struct type_ref *out, error *e) {
  struct token *tok = parser_peek (parser->base);

  switch (tok->type) {
    case TT_STRUCT: {
      return parse_struct_type_ref (parser, out, e);
    }
    case TT_IDENTIFIER: {
      return parse_take_type_ref (parser, out, e);
    }
    default: {
      return error_causef (
          e,
          ERR_SYNTAX,
          "Expected type_ref (struct or identifier) "
          "at "
          "position %u, got token type %s",
          parser->base->pos,
          tt_tostr (tok->type));
    }
  }
}

err_t parse_type_ref (
    struct parser      *p,
    struct type_ref    *dest,
    struct chunk_alloc *dalloc,
    error              *e) {
  struct type_ref_parser parser = {
      .base       = p,
      .dest       = dest,
      .persistent = dalloc,
  };

  chunk_alloc_create_default (&parser.temp);

  if (unlikely ((parse_type_ref_inner (&parser, parser.dest, e)) < SUCCESS)) { goto theend; }

theend:
  chunk_alloc_free_all (&parser.temp);
  return error_trace (e);
}
