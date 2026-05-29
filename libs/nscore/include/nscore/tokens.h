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

#include <c_specx.h>

#include "nscore/types.h"

enum token_t
{
  TT_PLUS = 1,
  TT_MINUS,
  TT_SLASH,
  TT_STAR,

  TT_BANG,
  TT_BANG_EQUAL,
  TT_EQUAL_EQUAL,
  TT_GREATER,
  TT_GREATER_EQUAL,
  TT_LESS,
  TT_LESS_EQUAL,

  TT_NOT,
  TT_CARET,
  TT_PERCENT,
  TT_PIPE,
  TT_PIPE_PIPE,
  TT_AMPERSAND,
  TT_AMPERSAND_AMPERSAND,

  TT_SEMICOLON,
  TT_COLON,
  TT_LEFT_BRACKET,
  TT_RIGHT_BRACKET,
  TT_LEFT_BRACE,
  TT_RIGHT_BRACE,
  TT_LEFT_PAREN,
  TT_RIGHT_PAREN,
  TT_COMMA,
  TT_DOT,

  TT_STRING,
  TT_IDENTIFIER,

  TT_INTEGER,
  TT_FLOAT,

  TT_CREATE,
  TT_DELETE,
  TT_INSERT,
  TT_APPEND,
  TT_READ,
  TT_WRITE,
  TT_REMOVE,
  TT_TAKE,

  TT_STRUCT,
  TT_UNION,
  TT_PRIM,

  TT_FILE,
  TT_QUERY,
  TT_OFST,
  TT_LEN,

  TT_TRUE,
  TT_FALSE,

  TT_AS,

  TT_EOF,
};

struct token
{
  enum token_t type;

  union {
    struct
    {
      const char *data;
      u32         len;
    } str;
    i32         integer;
    f32         floating;
    enum prim_t prim;
  };

  const char *text_start;
  u32         text_len;
};

#define case_OPCODE \
  TT_CREATE:        \
  case TT_DELETE:   \
  case TT_INSERT

HEADER_FUNC bool
tt_is_opcode (enum token_t ttype)
{
  switch (ttype)
  {
    case case_OPCODE:
    {
      return true;
    }
    default:
    {
      return false;
    }
  }
}

// Shorthands
#define quick_tok(_type) \
  (struct token)         \
  {                      \
    .type = _type        \
  }

#define tt_integer(val)                \
  (struct token)                       \
  {                                    \
    .type = TT_INTEGER, .integer = val \
  }

#define tt_float(val)                 \
  (struct token)                      \
  {                                   \
    .type = TT_FLOAT, .floating = val \
  }

#define tt_ident(_data, _len) \
  (struct token)              \
  {                           \
    .type = TT_IDENTIFIER,    \
    .str  = {                 \
        .data = _data,        \
        .len  = _len,         \
    },                        \
  }

#define tt_string(_data, _len) \
  (struct token)               \
  {                            \
    .type = TT_STRING,         \
    .str  = {                  \
        .data = _data,         \
        .len  = _len,          \
    },                         \
  }

#define tt_prim(val)             \
  (struct token)                 \
  {                              \
    .type = TT_PRIM, .prim = val \
  }

#define tt_opcode(op, _s)  \
  (struct token)           \
  {                        \
    .type = op, .stmt = _s \
  }

#define tt_err(_e)            \
  (struct token)              \
  {                           \
    .type = TT_ERROR, .e = _e \
  }

#define MAX_TOK_T_LEN 16

bool token_equal (const struct token *left, const struct token *right);

const char *tt_tostr (enum token_t t);
