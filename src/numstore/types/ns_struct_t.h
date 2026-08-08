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

#ifndef TYPES_H
#define TYPES_H

#include "alloc.h"
#include "collections.h" // llnode
#include "error.h"       // error
#include "numstore.h"    // pgno ...etc
#include "serial.h"      // string
#include "stdtypes.h"    // u32 ...etc

err_t struct_t_create (
    struct struct_t  *dest,
    struct kvt_list   list,
    struct allocator *dalloc,
    error            *e
);
bool struct_t_equal (const struct struct_t *left, const struct struct_t *right);
struct type *struct_t_resolve_key (
    t_size          *offset,
    struct struct_t *t,
    struct string    key
);

#endif
