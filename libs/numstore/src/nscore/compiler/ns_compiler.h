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

#include "core/ns_arena_alloc.h"
#include "core/ns_error.h"
#include "core/ns_platform.h"
#include "core/ns_stride.h"
#include "nscore/types/ns_query.h"
#include "nscore/types/ns_subtype.h"
#include "nscore/types/ns_type_ref.h"
#include "nscore/types/ns_types.h"

#include <nslang.h>
#include <stdarg.h>
#include <stdio.h>

/******************************************************************************
 * SECTION: Compiler
 * ----------------------------------------------------------------------------
 * @brief Compiler of various objects from strings
 *
 * Allocates types on the provided dalloc
 *
 * The lexer and the parsers are in the nslang rust crate (libs/nslang)
 ******************************************************************************/

HEADER_FUNC err_t
compile_type (struct type *dest, const char *text, struct arena_alloc *dalloc, error *e)
{
  struct nslang_error ne;
  NSLANG_ARENA (a, dalloc, e);

  if (nslang_compile_type (dest, text, &a, &ne) < 0) {
    return nslang_raise (e, &ne);
  }
  return SUCCESS;
}

HEADER_FUNC struct type *
compile_type_alloc (const char *text, struct arena_alloc *dalloc, error *e)
{
  NSLANG_ARENA (a, dalloc, e);

  // nslang needs an aligned type - arena_malloc doesn't align
  struct type *ret = nslang_arena_malloc (a.ctx, sizeof *ret, _Alignof (struct type));
  if (ret != NULL) {
    // TODO: return NULL when compile_type fails. Only tests call this, and
    // they read the failure from [e], so the half-built type comes back.
    (void)compile_type (ret, text, dalloc, e);
  }
  return ret;
}

HEADER_FUNC err_t
compile_subtype (struct subtype *dest, const char *text, struct arena_alloc *dalloc, error *e)
{
  struct nslang_error ne;
  NSLANG_ARENA (a, dalloc, e);

  if (nslang_compile_subtype (dest, text, &a, &ne) < 0) {
    return nslang_raise (e, &ne);
  }
  return SUCCESS;
}

HEADER_FUNC err_t
compile_multi_user_stride (
    struct multi_user_stride *dest,
    const char               *text,
    struct arena_alloc       *dalloc,
    error                    *e
)
{
  struct nslang_error ne;
  NSLANG_ARENA (a, dalloc, e);

  if (nslang_compile_multi_user_stride (dest, text, &a, &ne) < 0) {
    return nslang_raise (e, &ne);
  }
  return SUCCESS;
}

HEADER_FUNC err_t
compile_user_stride (struct user_stride *dest, const char *text, error *e)
{
  struct nslang_error ne;

  if (nslang_compile_user_stride (dest, text, &ne) < 0) {
    return nslang_raise (e, &ne);
  }
  return SUCCESS;
}

HEADER_FUNC err_t
compile_type_ref (struct type_ref *dest, const char *text, struct arena_alloc *dalloc, error *e)
{
  struct nslang_error ne;
  NSLANG_ARENA (a, dalloc, e);

  if (nslang_compile_type_ref (dest, text, &a, &ne) < 0) {
    return nslang_raise (e, &ne);
  }
  return SUCCESS;
}

HEADER_FUNC err_t
compile_query (struct query *dest, const char *text, struct arena_alloc *dalloc, error *e)
{
  struct nslang_error ne;
  NSLANG_ARENA (a, dalloc, e);

  if (nslang_compile_query (dest, text, &a, &ne) < 0) {
    return nslang_raise (e, &ne);
  }
  return SUCCESS;
}

HEADER_FUNC sb_size
ns_query_fcompile (
    struct arena_alloc *alloc, // Where to allocate query
    const char         *fmt,   // Format string
    va_list             ap,    // Var args
    struct query       *q,     // destination
    error              *e
)
{
  va_list ap2;
  va_copy (ap2, ap);

  // Compute the length the formatted query needs, without writing anything.
  i32 qlen = vsnprintf (NULL, 0, fmt, ap);
  if (qlen < 0) {
    va_end (ap2);
    return error_causef (e, ERR_INVALID_ARGUMENT, "Invalid printf argument");
  }

  char *buf = arena_malloc (alloc, (u32)qlen + 1, 1, e);
  if (buf == NULL) {
    va_end (ap2);
    return error_trace (e);
  }

  // Actually write the formatted query into buf.
  qlen = vsnprintf (buf, (size_t)qlen + 1, fmt, ap2);
  ASSERT (qlen >= 0);
  va_end (ap2);

  if (compile_query (q, buf, alloc, e) < 0) {
    return error_trace (e);
  }

  return SUCCESS;
}

#endif // COMPILER_H
