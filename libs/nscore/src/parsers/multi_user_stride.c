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

#include "nscore/parsers/multi_user_stride.h"

#include "c_specx.h"

struct multi_user_stride_parser {
  struct parser      *base;
  struct mus_builder  builder;
  struct chunk_alloc  temp;
  struct chunk_alloc *persistent;
};

// Parse optional ':' NUMBER (step)
static err_t parse_step (struct multi_user_stride_parser *parser, struct user_stride *s, error *e) {
  if (!parser_match (parser->base, TT_COLON)) return SUCCESS;

  s->present |= COLON_PRESENT;
  parser_advance (parser->base);

  if (parser_match (parser->base, TT_INTEGER)) {
    struct token *tok = parser_advance (parser->base);
    s->step           = (sb_size)tok->integer;
    s->present |= STEP_PRESENT;
  }

  return SUCCESS;
}

// Parse optional NUMBER ':' NUMBER (stop + step)
static err_t parse_stop (struct multi_user_stride_parser *parser, struct user_stride *s, error *e) {
  if (parser_match (parser->base, TT_INTEGER)) {
    struct token *tok = parser_advance (parser->base);
    s->stop           = (sb_size)tok->integer;
    s->present |= STOP_PRESENT;
  }

  return parse_step (parser, s, e);
}

// entry ::= NUMBER | NUMBER? ':' NUMBER? | NUMBER? ':' NUMBER? ':' NUMBER?
static err_t parse_entry (struct multi_user_stride_parser *parser, error *e) {
  struct user_stride s = {0};

  // Optional start integer
  if (parser_match (parser->base, TT_INTEGER)) {
    struct token *tok = parser_advance (parser->base);
    s.start           = (sb_size)tok->integer;
    s.present |= START_PRESENT;

    // Bare number with no colon → single index
    if (!parser_match (parser->base, TT_COLON)) { return musb_accept_key (&parser->builder, s, e); }

    s.present |= COLON_PRESENT;
    parser_advance (parser->base);
    WRAP (parse_stop (parser, &s, e));
    return musb_accept_key (&parser->builder, s, e);
  }

  // No leading number — must be ':'
  if (parser_match (parser->base, TT_COLON)) {
    s.present |= COLON_PRESENT;
    parser_advance (parser->base);
    WRAP (parse_stop (parser, &s, e));
    return musb_accept_key (&parser->builder, s, e);
  }

  return error_causef (e, ERR_SYNTAX, "Expected number or ':' at position %u", parser->base->pos);
}

// stride ::= '[' entry ( ',' entry )* ']'
static err_t parse_multi_user_stride_inner (struct multi_user_stride_parser *parser, error *e) {
  // Check for empty: []
  if (parser_match (parser->base, TT_RIGHT_BRACKET)) { return SUCCESS; }

  WRAP (parse_entry (parser, e));

  while (parser_match (parser->base, TT_COMMA)) {
    parser_advance (parser->base);
    WRAP (parse_entry (parser, e));
  }

  return SUCCESS;
}

err_t parse_multi_user_stride (
    struct parser            *parser,
    struct multi_user_stride *dest,
    struct chunk_alloc       *alloc,
    error                    *e) {
  struct multi_user_stride_parser p = {
      .base       = parser,
      .persistent = alloc,
  };

  chunk_alloc_create_default (&p.temp);
  musb_create (&p.builder, &p.temp, alloc);

  if (unlikely ((parser_expect (p.base, TT_LEFT_BRACKET, e)) < SUCCESS)) { goto theend; }
  if (unlikely ((parse_multi_user_stride_inner (&p, e)) < SUCCESS)) { goto theend; }
  if (unlikely ((parser_expect (p.base, TT_RIGHT_BRACKET, e)) < SUCCESS)) { goto theend; }
  if (unlikely ((musb_build (dest, &p.builder, e)) < SUCCESS)) { goto theend; }

theend:
  chunk_alloc_free_all (&p.temp);
  return error_trace (e);
}
