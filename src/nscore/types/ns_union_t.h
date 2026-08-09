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

#ifndef NS_UNION_T_H
#define NS_UNION_T_H

#include "core/ns_alloc.h"
#include "core/ns_error.h"    // error
#include "core/ns_stdtypes.h" // u32 ...etc
#include "nscore/types/ns_kvt.h"
#include "nscore/types/ns_types.h"

struct type *union_t_resolve_key (struct union_t *t, struct string key);

err_t union_t_create (
    struct union_t   *dest,
    struct kvt_list   list,
    struct allocator *dalloc,
    error            *e
);

u32   union_t_get_serial_size (const struct union_t *t);
void  union_t_serialize (struct serializer *dest, const struct union_t *src);
err_t union_t_deserialize (
    struct union_t      *dest,
    struct deserializer *src,
    struct allocator    *a,
    error               *e
);
err_t union_t_validate (const struct union_t *s, error *e);
i32   union_t_snprintf (char *str, u32 size, const struct union_t *st);
u32   union_t_byte_size (const struct union_t *t);
err_t union_t_random (struct union_t *un, struct allocator *alloc, u32 depth, error *e);
bool  union_t_equal (const struct union_t *left, const struct union_t *right);

#endif
