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

#pragma once

#include "nscore/errors.h"
#include "nscore/tokens.h"

#include <c_specx.h>

struct parser
{
  struct token *src;
  u32           src_len;
  u32           pos;
};

DEFINE_DBG_ASSERT (struct parser, parser, p, {
  ASSERT (p->src);
  ASSERT (p->src_len > 0);
  ASSERT (p->pos <= p->src_len);
})

HEADER_FUNC struct parser
parser_init (struct token *src, u32 src_len)
{
  struct parser ret = {
      .src     = src,
      .src_len = src_len,
      .pos     = 0,
  };

  DBG_ASSERT (parser, &ret);

  return ret;
}

HEADER_FUNC u32
parser_remain (struct parser *p)
{
  return p->src_len - p->pos;
}

HEADER_FUNC struct token *
parser_peek (struct parser *p)
{
  DBG_ASSERT (parser, p);
  ASSERT (p->pos < p->src_len);

  return (p->pos < p->src_len) ? &p->src[p->pos] : NULL;
}

HEADER_FUNC struct token *
parser_peek_n (struct parser *p, u32 n)
{
  DBG_ASSERT (parser, p);
  ASSERT (p->pos + n < p->src_len);

  u32 target_pos = p->pos + n;
  return (target_pos < p->src_len) ? &p->src[target_pos] : NULL;
}

HEADER_FUNC bool
parser_match (struct parser *p, enum token_t type)
{
  DBG_ASSERT (parser, p);

  struct token *tok = parser_peek (p);

  return tok->type == type;
}

HEADER_FUNC struct token *
parser_advance (struct parser *p)
{
  DBG_ASSERT (parser, p);

  struct token *tok = &p->src[p->pos];
  p->pos++;

  return tok;
}

// Expect a specific token type, consume it, and advance
HEADER_FUNC err_t
parser_expect (struct parser *p, enum token_t type, error *e)
{
  struct token *tok = parser_peek (p);

  if (tok->type != type)
  {
    return error_causef (
        e,
        ERR_SYNTAX,
        "Expected token type %s at position %u, got %s",
        tt_tostr (type),
        p->pos,
        tt_tostr (tok->type)
    );
  }

  p->pos++;
  return SUCCESS;
}

HEADER_FUNC bool
parser_at_end (struct parser *p)
{
  DBG_ASSERT (parser, p);
  return p->pos == p->src_len;
}

HEADER_FUNC err_t
parser_check_end (struct parser *p, error *e)
{
  if (!parser_at_end (p))
  {
    return error_causef (
        e,
        ERR_SYNTAX,
        "Unexpected tokens after "
        "expression at position %u",
        p->pos
    );
  }

  return SUCCESS;
}
