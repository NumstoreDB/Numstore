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

#ifndef COMPILER_H
#define COMPILER_H

#include "collections.h" // dbl_buffer / multi_user_stride
#include "csx_assert.h"  // DEFINE_DBG_ASSERT
#include "platform.h"    // HEADER_FUNC
#include "stdtypes.h"    // u32 ...etc
#include "types.h"       // type

/******************************************************************************
 * SECTION: Tokens
 * ----------------------------------------------------------------------------
 * @brief Representation of tokens in compiled structs
 ******************************************************************************/

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
TT_CREATE:          \
case TT_DELETE:     \
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

/******************************************************************************
 * SECTION: Lexer
 * ----------------------------------------------------------------------------
 * @brief A lexer that converts strings into tokens
 *
 * Uses a double buffer under the hood so there are inner allocations
 ******************************************************************************/

struct lexer
{
  const char *src;
  u32         src_len;
  u32         start;
  u32         current;

  struct token *tokens;

  u32               ntokens;
  struct dbl_buffer _tokens;
};

err_t lex_tokens (const char *src, u32 src_len, struct lexer *lex, error *e);
void  lex_free (struct lexer *lex);

/******************************************************************************
 * SECTION: Parser
 * ----------------------------------------------------------------------------
 * @brief A parser utility
 *
 * No actual type is parsed, this tool just advances tokens and
 * offers useful utilities for parsing
 ******************************************************************************/

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

/******************************************************************************
 * SECTION: Parser Implementations
 * ----------------------------------------------------------------------------
 * @brief Implementations of parsers
 ******************************************************************************/

// stride       ::= '[' entry_list ']'
// entry_list   ::= entry
// | entry ',' entry_list
// entry        ::= slice_range
// | NUMBER
// slice_range  ::= NUMBER? ':' NUMBER?
// | NUMBER? ':' NUMBER? ':' NUMBER?

err_t parse_multi_user_stride (
    struct parser            *parser,
    struct multi_user_stride *dest,
    struct chunk_alloc       *dalloc,
    error                    *e
);

// subtype    ::= IDENT stride* ('.' IDENT stride*)*

err_t parse_subtype (
    struct parser      *p,
    struct subtype     *dest,
    struct chunk_alloc *dalloc,
    error              *e
);

// type            ::= struct_type
// | union_type
// | sarray_type
// | primitive_type
// struct_type     ::= 'struct' '{' field (',' field)* '}'
// union_type      ::= 'union' '{' field (',' field)* '}'
// sarray_type     ::= '[' INTEGER ']'+ type
// primitive_type  ::= PRIM
// field           ::= IDENTIFIER type

err_t parse_type (
    struct parser      *p,
    struct type        *dest,
    struct chunk_alloc *dalloc,
    error              *e
);

// type_ref        ::= struct_type_ref
// | take_type_ref
// struct_type_ref ::= 'struct' '{' field_ref (',' field_ref)* '}'
// take_type_ref   ::= subtype
// field_ref       ::= IDENTIFIER type_ref

err_t parse_type_ref (
    struct parser      *p,
    struct type_ref    *dest,
    struct chunk_alloc *dalloc,
    error              *e
);

// entry        ::= slice_range
// | NUMBER
// slice_range  ::= NUMBER? ':' NUMBER?
// | NUMBER? ':' NUMBER? ':' NUMBER?

err_t
parse_user_stride (struct parser *parser, struct user_stride *dest, error *e);

/******************************************************************************
 * SECTION: Compiler
 * ----------------------------------------------------------------------------
 * @brief Compiler of various objects from strings
 *
 * Allocates types on the provided dalloc if provided
 ******************************************************************************/

err_t compile_type (
    struct type        *dest,
    const char         *text,
    struct chunk_alloc *dalloc,
    error              *e
);

err_t compile_subtype (
    struct subtype     *dest,
    const char         *text,
    struct chunk_alloc *dalloc,
    error              *e
);

err_t compile_multi_user_stride (
    struct multi_user_stride *dest,
    const char               *text,
    struct chunk_alloc       *dalloc,
    error                    *e
);

err_t
compile_user_stride (struct user_stride *dest, const char *text, error *e);

err_t compile_type_ref (
    struct type_ref    *dest,
    const char         *text,
    struct chunk_alloc *dalloc,
    error              *e
);
#endif // COMPILER_H
