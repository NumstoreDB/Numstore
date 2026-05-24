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

#include "nscore/subtype.h"
#include "nscore/type_ref.h"
#include "nscore/types.h"

#include <c_specx.h>

err_t compile_type (struct type *dest, const char *text, struct chunk_alloc *dalloc, error *e);

err_t
compile_subtype (struct subtype *dest, const char *text, struct chunk_alloc *dalloc, error *e);

err_t compile_multi_user_stride (
    struct multi_user_stride *dest,
    const char               *text,
    struct chunk_alloc       *dalloc,
    error                    *e
);

err_t compile_user_stride (struct user_stride *dest, const char *text, error *e);

err_t
compile_type_ref (struct type_ref *dest, const char *text, struct chunk_alloc *dalloc, error *e);
