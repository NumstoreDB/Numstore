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

#include "ns_arena_alloc.h"
#include "ns_platform.h"
#include "ns_stride.h"
#include "types/ns_query.h"
#include "types/ns_subtype.h"
#include "types/ns_type_ref.h"
#include "types/ns_types.h"

/******************************************************************************
 * SECTION: Compiler
 * ----------------------------------------------------------------------------
 * @brief Compiler of various objects from strings
 *
 * Allocates types on the provided dalloc if provided
 ******************************************************************************/

err_t compile_type (struct type *dest, const char *text, struct arena_alloc *dalloc, error *e);

HEADER_FUNC struct type *
compile_type_alloc (const char *text, struct arena_alloc *dalloc, error *e)
{
  struct type *ret = arena_malloc (dalloc, 1, sizeof *ret, e);
  if (ret != NULL) {
    // TODO: return NULL when compile_type fails. Only tests call this, and
    // they read the failure from [e], so the half-built type comes back.
    (void)compile_type (ret, text, dalloc, e);
  }
  return ret;
}

err_t compile_subtype (
    struct subtype     *dest,
    const char         *text,
    struct arena_alloc *dalloc,
    error              *e
);

err_t compile_multi_user_stride (
    struct multi_user_stride *dest,
    const char               *text,
    struct arena_alloc       *dalloc,
    error                    *e
);

err_t compile_user_stride (struct user_stride *dest, const char *text, error *e);

err_t compile_type_ref (
    struct type_ref    *dest,
    const char         *text,
    struct arena_alloc *dalloc,
    error              *e
);

err_t compile_query (struct query *dest, const char *text, struct arena_alloc *dalloc, error *e);

// Checks what the grammar can't - e.g. a create type that's too wide
err_t validate_query (const struct query *q, error *e);

HEADER_FUNC err_t
compile_and_validate_query (
    struct query       *dest,
    const char         *text,
    struct arena_alloc *dalloc,
    error              *e
)
{
  WRAP (compile_query (dest, text, dalloc, e));
  return validate_query (dest, e);
}

#endif // COMPILER_H
