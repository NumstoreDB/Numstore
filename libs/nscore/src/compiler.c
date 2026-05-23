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

#include "nscore/compiler.h"

#include "nscore/errors.h"
#include "nscore/lexer.h"
#include "nscore/parsers/multi_user_stride.h"
#include "nscore/parsers/parser.h"
#include "nscore/parsers/subtype.h"
#include "nscore/parsers/type.h"
#include "nscore/parsers/type_ref.h"
#include "nscore/parsers/user_stride.h"

err_t
compile_type (struct type *dest, const char *text, struct chunk_alloc *dalloc, error *e)
{
  struct lexer lex;
  WRAP (lex_tokens (text, strlen (text), &lex, e));

  struct parser parser = parser_init (lex.tokens, lex.ntokens);

  err_t ret = parse_type (&parser, dest, dalloc, e);
  lex_free (&lex);
  return ret;
}

err_t
compile_subtype (struct subtype *dest, const char *text, struct chunk_alloc *dalloc, error *e)
{
  struct lexer lex;
  WRAP (lex_tokens (text, strlen (text), &lex, e));

  struct parser parser = parser_init (lex.tokens, lex.ntokens);

  err_t ret = parse_subtype (&parser, dest, dalloc, e);
  lex_free (&lex);
  return ret;
}

err_t
compile_multi_user_stride (
    struct multi_user_stride *dest,
    const char               *text,
    struct chunk_alloc       *dalloc,
    error                    *e
)
{
  struct lexer lex;
  WRAP (lex_tokens (text, strlen (text), &lex, e));

  struct parser parser = parser_init (lex.tokens, lex.ntokens);

  err_t ret = parse_multi_user_stride (&parser, dest, dalloc, e);
  lex_free (&lex);
  return ret;
}

err_t
compile_user_stride (struct user_stride *dest, const char *text, error *e)
{
  struct lexer lex;
  WRAP (lex_tokens (text, strlen (text), &lex, e));

  struct parser parser = parser_init (lex.tokens, lex.ntokens);

  err_t ret = parse_user_stride (&parser, dest, e);
  lex_free (&lex);
  return ret;
}

err_t
compile_type_ref (struct type_ref *dest, const char *text, struct chunk_alloc *dalloc, error *e)
{
  struct lexer lex;
  WRAP (lex_tokens (text, strlen (text), &lex, e));

  struct parser parser = parser_init (lex.tokens, lex.ntokens);

  err_t ret = parse_type_ref (&parser, dest, dalloc, e);

  lex_free (&lex);

  return ret;
}

#ifndef NTEST
TEST (compile_user_stride_basic)
{
  error              err    = error_create ();
  struct user_stride stride = {0};
  TEST_CASE ("empty stride []")
  {
    test_assert_int_equal (compile_user_stride (&stride, "[]", &err), ERR_SYNTAX);
    err.cause_code = SUCCESS;
  }

  TEST_CASE ("single integer [5]")
  {
    stride = (struct user_stride){0};
    test_assert_int_equal (compile_user_stride (&stride, "[5]", &err), SUCCESS);
    test_assert_int_equal (stride.start, 5);
    test_assert_int_equal (stride.present & START_PRESENT, START_PRESENT);
    test_assert_int_equal (stride.present & STOP_PRESENT, 0);
    test_assert_int_equal (stride.present & STEP_PRESENT, 0);
  }

  TEST_CASE ("step cannot be zero")
  {
    stride = (struct user_stride){0};
    test_assert_int_equal (compile_user_stride (&stride, "[::0]", &err), ERR_SYNTAX);
    err.cause_code = SUCCESS;
  }
}

TEST (compile_type_primitives)
{
  error              err = error_create ();
  struct chunk_alloc arena;
  chunk_alloc_create_default (&arena);
  struct type t;
  TEST_CASE ("i8")
  {
    test_assert_int_equal (compile_type (&t, "i8", &arena, &err), SUCCESS);
    test_assert_int_equal (t.type, T_PRIM);
    test_assert_int_equal (t.p, I8);
  }
  TEST_CASE ("i16")
  {
    test_assert_int_equal (compile_type (&t, "i16", &arena, &err), SUCCESS);
    test_assert_int_equal (t.type, T_PRIM);
    test_assert_int_equal (t.p, I16);
  }
  TEST_CASE ("i32")
  {
    test_assert_int_equal (compile_type (&t, "i32", &arena, &err), SUCCESS);
    test_assert_int_equal (t.type, T_PRIM);
    test_assert_int_equal (t.p, I32);
  }
  TEST_CASE ("i64")
  {
    test_assert_int_equal (compile_type (&t, "i64", &arena, &err), SUCCESS);
    test_assert_int_equal (t.type, T_PRIM);
    test_assert_int_equal (t.p, I64);
  }
  TEST_CASE ("u8")
  {
    test_assert_int_equal (compile_type (&t, "u8", &arena, &err), SUCCESS);
    test_assert_int_equal (t.type, T_PRIM);
    test_assert_int_equal (t.p, U8);
  }
  TEST_CASE ("u16")
  {
    test_assert_int_equal (compile_type (&t, "u16", &arena, &err), SUCCESS);
    test_assert_int_equal (t.type, T_PRIM);
    test_assert_int_equal (t.p, U16);
  }
  TEST_CASE ("u32")
  {
    test_assert_int_equal (compile_type (&t, "u32", &arena, &err), SUCCESS);
    test_assert_int_equal (t.type, T_PRIM);
    test_assert_int_equal (t.p, U32);
  }
  TEST_CASE ("u64")
  {
    test_assert_int_equal (compile_type (&t, "u64", &arena, &err), SUCCESS);
    test_assert_int_equal (t.type, T_PRIM);
    test_assert_int_equal (t.p, U64);
  }
  TEST_CASE ("f32")
  {
    test_assert_int_equal (compile_type (&t, "f32", &arena, &err), SUCCESS);
    test_assert_int_equal (t.type, T_PRIM);
    test_assert_int_equal (t.p, F32);
  }
  TEST_CASE ("f64")
  {
    test_assert_int_equal (compile_type (&t, "f64", &arena, &err), SUCCESS);
    test_assert_int_equal (t.type, T_PRIM);
    test_assert_int_equal (t.p, F64);
  }
  chunk_alloc_free_all (&arena);
}

TEST (compile_type_sarray)
{
  error              err = error_create ();
  struct chunk_alloc arena;
  chunk_alloc_create_default (&arena);
  struct type t;
  TEST_CASE ("[10]i32")
  {
    test_assert_int_equal (compile_type (&t, "[10]i32", &arena, &err), SUCCESS);
    test_assert_int_equal (t.type, T_SARRAY);
    test_assert_int_equal (t.sa.rank, 1);
    test_assert_int_equal (t.sa.dims[0], 10);
    test_assert_int_equal (t.sa.t->type, T_PRIM);
    test_assert_int_equal (t.sa.t->p, I32);
  }
  TEST_CASE ("[5][10]f64")
  {
    test_assert_int_equal (compile_type (&t, "[5][10]f64", &arena, &err), SUCCESS);
    test_assert_int_equal (t.type, T_SARRAY);
    test_assert_int_equal (t.sa.rank, 2);
    test_assert_int_equal (t.sa.dims[0], 5);
    test_assert_int_equal (t.sa.dims[1], 10);
    test_assert_int_equal (t.sa.t->type, T_PRIM);
    test_assert_int_equal (t.sa.t->p, F64);
  }
  TEST_CASE ("[2][3][4]u8")
  {
    test_assert_int_equal (compile_type (&t, "[2][3][4]u8", &arena, &err), SUCCESS);
    test_assert_int_equal (t.type, T_SARRAY);
    test_assert_int_equal (t.sa.rank, 3);
    test_assert_int_equal (t.sa.dims[0], 2);
    test_assert_int_equal (t.sa.dims[1], 3);
    test_assert_int_equal (t.sa.dims[2], 4);
  }
  chunk_alloc_free_all (&arena);
}

TEST (compile_type_struct)
{
  error              err = error_create ();
  struct chunk_alloc arena;
  chunk_alloc_create_default (&arena);
  struct type t;
  TEST_CASE ("struct { x i32 y f64 }")
  {
    test_assert_int_equal (compile_type (&t, "struct { x i32, y f64 }", &arena, &err), SUCCESS);
    test_assert_int_equal (t.type, T_STRUCT);
    test_assert_int_equal (t.st.len, 2);
    test_assert (string_equal (t.st.keys[0], strfcstr ("x")));
    test_assert (string_equal (t.st.keys[1], strfcstr ("y")));
    test_assert_int_equal (t.st.types[0]->type, T_PRIM);
    test_assert_int_equal (t.st.types[0]->p, I32);
    test_assert_int_equal (t.st.types[1]->type, T_PRIM);
    test_assert_int_equal (t.st.types[1]->p, F64);
  }
  TEST_CASE ("nested struct { a struct { b i32 } }")
  {
    test_assert_int_equal (
        compile_type (&t, "struct { a struct { b i32 } }", &arena, &err),
        SUCCESS
    );
    test_assert_int_equal (t.type, T_STRUCT);
    test_assert_int_equal (t.st.len, 1);
    test_assert (string_equal (t.st.keys[0], strfcstr ("a")));
    test_assert_int_equal (t.st.types[0]->type, T_STRUCT);
    test_assert_int_equal (t.st.types[0]->st.len, 1);
  }
  chunk_alloc_free_all (&arena);
}

TEST (compile_type_union)
{
  error              err = error_create ();
  struct chunk_alloc arena;
  chunk_alloc_create_default (&arena);
  struct type t;
  TEST_CASE ("union { a i32, b f64 }")
  {
    test_assert_int_equal (compile_type (&t, "union { a i32, b f64 }", &arena, &err), SUCCESS);
    test_assert_int_equal (t.type, T_UNION);
    test_assert_int_equal (t.un.len, 2);
    test_assert (string_equal (t.un.keys[0], strfcstr ("a")));
    test_assert (string_equal (t.un.keys[1], strfcstr ("b")));
    test_assert_int_equal (t.un.types[0]->type, T_PRIM);
    test_assert_int_equal (t.un.types[0]->p, I32);
    test_assert_int_equal (t.un.types[1]->type, T_PRIM);
    test_assert_int_equal (t.un.types[1]->p, F64);
  }
  chunk_alloc_free_all (&arena);
}

TEST (compile_type_complex)
{
  error              err = error_create ();
  struct chunk_alloc arena;
  chunk_alloc_create_default (&arena);
  struct type t;
  TEST_CASE ("[10]struct { x i32, y [5]f64 }")
  {
    test_assert_int_equal (
        compile_type (&t, "[10]struct { x i32, y [5]f64 }", &arena, &err),
        SUCCESS
    );
    test_assert_int_equal (t.type, T_SARRAY);
    test_assert_int_equal (t.sa.rank, 1);
    test_assert_int_equal (t.sa.dims[0], 10);
    test_assert_int_equal (t.sa.t->type, T_STRUCT);
    test_assert_int_equal (t.sa.t->st.len, 2);
  }
  chunk_alloc_free_all (&arena);
}

#endif
