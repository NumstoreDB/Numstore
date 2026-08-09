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

#ifndef NS_STRUCT_T_H
#define NS_STRUCT_T_H

#include "core/ns_alloc.h"
#include "core/ns_error.h"    // error
#include "core/ns_stdtypes.h" // u32 ...etc
#include "nscore/types/ns_kvt.h"
#include "nscore/types/ns_types.h"
#include "numstore.h" // pgno ...etc

err_t struct_t_create (
    struct struct_t  *dest,
    struct kvt_list   list,
    struct allocator *dalloc,
    error            *e
);
bool         struct_t_equal (const struct struct_t *left, const struct struct_t *right);
struct type *struct_t_resolve_key (t_size *offset, struct struct_t *t, struct string key);

u32   struct_t_get_serial_size (const struct struct_t *t);
void  struct_t_serialize (struct serializer *dest, const struct struct_t *src);
err_t struct_t_deserialize (
    struct struct_t     *dest,
    struct deserializer *src,
    struct allocator    *a,
    error               *e
);
err_t struct_t_validate (const struct struct_t *s, error *e);
i32   struct_t_snprintf (char *str, u32 size, const struct struct_t *st);
u32   struct_t_byte_size (const struct struct_t *t);
err_t struct_t_random (struct struct_t *st, struct allocator *alloc, u32 depth, error *e);

#endif
