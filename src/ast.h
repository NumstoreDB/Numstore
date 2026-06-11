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

#ifndef QUERY_H
#define QUERY_H

#include "compile_config.h"
#include "compiler.h"

/******************************************************************************
 * SECTION: Literals
 * ----------------------------------------------------------------------------
 *
 * @brief A Literal is a user entered value
 ******************************************************************************/

struct object
{
  struct string  *keys;
  struct literal *literals;
  u32             len;
};

struct array
{
  struct literal *literals;
  u32             len;
};

i32   object_t_snprintf (char *str, u32 size, const struct object *st);
bool  object_equal (const struct object *left, const struct object *right);
err_t object_plus (
    struct object       *dest,
    const struct object *right,
    struct lalloc       *alloc,
    error               *e
);

bool  array_equal (const struct array *left, const struct array *right);
err_t array_plus (
    struct array       *dest,
    const struct array *right,
    struct lalloc      *alloc,
    error              *e
);

enum literal_t
{
  /* Composite */
  LT_OBJECT,
  LT_ARRAY,

  /* Simple */
  LT_STRING,
  LT_INTEGER,
  LT_DECIMAL,
  LT_COMPLEX,
  LT_BOOL,
};

const char *literal_t_tostr (enum literal_t);
bool literal_equal (const struct literal *left, const struct literal *right);

struct literal
{
  enum literal_t type;

  union {
    bool          bl;
    struct object obj;
    struct array  arr;
    struct string str;
    i32           integer;
    f32           decimal;
    cf128         cplx;
  };
};

/////////////////////////
// Object / Array builders

struct object_llnode
{
  struct string  key;
  struct literal v;
  struct llnode  link;
};

struct array_llnode
{
  struct literal v;
  struct llnode  link;
};

struct object_builder
{
  struct llnode *head;

  u16 klen;
  u16 tlen;

  struct lalloc *work;
  struct lalloc *dest;
};

struct array_builder
{
  struct llnode *head;

  struct lalloc *work;
  struct lalloc *dest;
};

struct object_builder objb_create (struct lalloc *work, struct lalloc *dest);
err_t                 objb_accept_string (
    struct object_builder *o,
    const struct string    key,
    error                 *e
);
err_t
objb_accept_literal (struct object_builder *o, struct literal v, error *e);
err_t objb_build (struct object *dest, struct object_builder *b, error *e);

struct array_builder arb_create (struct lalloc *work, struct lalloc *dest);
err_t arb_accept_literal (struct array_builder *o, struct literal v, error *e);
err_t arb_build (struct array *dest, struct array_builder *b, error *e);

/////////////////////////
// Simple constructors for the other types

HEADER_FUNC struct literal
literal_string_create (struct string str)
{
  return (struct literal){
      .str  = str,
      .type = LT_STRING,
  };
}

HEADER_FUNC struct literal
literal_integer_create (i64 integer)
{
  return (struct literal){
      .type    = LT_INTEGER,
      .integer = integer,
  };
}

HEADER_FUNC struct literal
literal_decimal_create (f128 decimal)
{
  return (struct literal){
      .type    = LT_DECIMAL,
      .decimal = decimal,
  };
}

HEADER_FUNC struct literal
literal_complex_create (cf128 cplx)
{
  struct literal ret = {
      .type = LT_COMPLEX,
  };
  ret.cplx[0] = cplx[0];
  ret.cplx[1] = cplx[1];

  return ret;
}

HEADER_FUNC struct literal
literal_bool_create (bool bl)
{
  return (struct literal){
      .type = LT_BOOL,
      .bl   = bl,
  };
}

void i_log_literal (struct literal *v);

/////////////////////////
// Expression reductions

// dest = dest + right
err_t literal_plus_literal (
    struct literal       *dest,
    const struct literal *right,
    struct lalloc        *alloc,
    error                *e
);

// dest = dest - right
err_t literal_minus_literal (
    struct literal       *dest,
    const struct literal *right,
    error                *e
);

// dest = dest * right
err_t literal_star_literal (
    struct literal       *dest,
    const struct literal *right,
    error                *e
);

// dest = dest / right
err_t literal_slash_literal (
    struct literal       *dest,
    const struct literal *right,
    error                *e
);

// dest = dest == right
err_t literal_equal_equal_literal (
    struct literal       *dest,
    const struct literal *right,
    error                *e
);

// dest = dest != right
err_t literal_bang_equal_literal (
    struct literal       *dest,
    const struct literal *right,
    error                *e
);

// dest = dest > right
err_t literal_greater_literal (
    struct literal       *dest,
    const struct literal *right,
    error                *e
);

// dest = dest >= right
err_t literal_greater_equal_literal (
    struct literal       *dest,
    const struct literal *right,
    error                *e
);

// dest = dest < right
err_t literal_less_literal (
    struct literal       *dest,
    const struct literal *right,
    error                *e
);

// dest = dest <= right
err_t literal_less_equal_literal (
    struct literal       *dest,
    const struct literal *right,
    error                *e
);

// dest = dest ^ right
err_t literal_caret_literal (
    struct literal       *dest,
    const struct literal *right,
    error                *e
);

// dest = dest % right
err_t literal_mod_literal (
    struct literal       *dest,
    const struct literal *right,
    error                *e
);

// dest = dest | right
err_t literal_pipe_literal (
    struct literal       *dest,
    const struct literal *right,
    error                *e
);

// dest = dest || right
void
literal_pipe_pipe_literal (struct literal *dest, const struct literal *right);

// dest = dest & right
err_t literal_ampersand_literal (
    struct literal       *dest,
    const struct literal *right,
    error                *e
);

// dest = dest && right
void literal_ampersand_ampersand_literal (
    struct literal       *dest,
    const struct literal *right
);

// dest = ~dest
err_t literal_not (struct literal *dest, error *e);

// dest = -dest
err_t literal_minus (struct literal *dest, error *e);

// dest = !dest
void literal_bang (struct literal *dest);

/******************************************************************************
 * SECTION: Expressions
 * ----------------------------------------------------------------------------
 *
 * @brief A fixed sized memory arena
 *
 * A Linear allocator takes a buffer and dishes out memory
 * from this fixed sized buffer. It is not dynamic.
 * It does not do any mallocs under the hood.
 ******************************************************************************/

struct expr;

/*-----------------------------------------------------------------------------
 * SUBSECTION: Unary
 * @brief Single operator operations
 *
 * !EXPR
 * -EXPR
 *----------------------------------------------------------------------------*/

struct unary
{
  enum token_t op;
  struct expr *e;
};

DEFINE_DBG_ASSERT (struct unary, unary, u, {
  ASSERT (u);
  ASSERT (u->op == TT_MINUS || u->op == TT_BANG);
})

/*-----------------------------------------------------------------------------
 * SUBSECTION: Binary
 * @brief Two variables, one operator
 *
 * EXPR + EXPR
 * EXPR == EXPR
 * etc.
 *----------------------------------------------------------------------------*/

struct binary
{
  struct expr *left;
  enum token_t op;
  struct expr *right;
};

DEFINE_DBG_ASSERT (struct binary, binary, b, {
  switch (b->op)
  {
    case TT_EQUAL_EQUAL:
    case TT_BANG_EQUAL:
    case TT_LESS:
    case TT_LESS_EQUAL:
    case TT_GREATER:
    case TT_GREATER_EQUAL:
    case TT_PLUS:
    case TT_MINUS:
    case TT_STAR:
    case TT_SLASH: break;
    default: ASSERT (false);
  }
})

/*-----------------------------------------------------------------------------
 * SUBSECTION: Expression
 * @brief A value, unary, binary or grouping
 *----------------------------------------------------------------------------*/

struct expr
{
  enum expr_t
  {
    ET_VALUE,
    ET_UNARY,
    ET_BINARY,
    ET_GROUPING,
  } type;

  union {
    struct literal l;
    struct unary   u;
    struct binary  b;
    struct expr   *g;
    char          *v;
  };
};

HEADER_FUNC struct expr
create_literal_expr (struct literal l)
{
  return (struct expr){.type = ET_VALUE, .l = l};
}

HEADER_FUNC struct expr
create_grouping_expr (struct expr *e)
{
  return (struct expr){.type = ET_GROUPING, .g = e};
}

HEADER_FUNC struct expr
create_unary_expr (struct expr *e, enum token_t op)
{
  struct unary ret = {
      .op = op,
      .e  = e,
  };

  DBG_ASSERT (unary, &ret);

  return (struct expr){
      .type = ET_UNARY,
      .u    = ret,
  };
}

HEADER_FUNC struct expr
create_binary_expr (struct expr *left, enum token_t op, struct expr *right)
{
  struct binary ret = {
      .left  = left,
      .op    = op,
      .right = right,
  };

  DBG_ASSERT (binary, &ret);

  return (struct expr){
      .type = ET_BINARY,
      .b    = ret,
  };
}

err_t expr_evaluate (
    struct literal *dest,
    struct expr    *exp,
    struct lalloc  *work,
    error          *e
);

/******************************************************************************
 * SECTION: Query
 * ----------------------------------------------------------------------------
 *
 * @brief A Literal is a user entered value
 ******************************************************************************/

struct query_source
{
  struct literal
};

struct query
{
  enum
  {
    QT_INSERT,
    QT_READ,
    QT_WRITE,
    QT_REMOVE,
    QT_CREATE,
    QT_DELETE,
    QT_GET,
    QT_EXIT,
    QT_HELP,
  } type;

  union {
    struct
    {
      const char *vname;
      b_size      offst;
    } insert;
  };
};

#endif
