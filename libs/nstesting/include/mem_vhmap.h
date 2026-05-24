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
#include "nscore/variables.h"

// You may consider moving this to numstore - might be useful for caching

struct mem_vhmap
{
  struct htable    *vhasht; // Hash table of variables
  struct slab_alloc alloc;  // Allocator for variable frames
};

// Lifecycle
struct mem_vhmap *mem_vhmap_create (error *e);
void              mem_vhmap_free (struct mem_vhmap *db);
struct mem_vhmap *mem_vhmap_clone (const struct mem_vhmap *src, error *e);
err_t             mem_vhmap_add_var (struct mem_vhmap *db, struct variable *var, error *e);
struct variable  *mem_vhmap_get_var (struct mem_vhmap *db, struct string name);
void              mem_vhmap_remove_var (struct mem_vhmap *db, struct string name);
u32               mem_vhmap_count (struct mem_vhmap *db);
struct variable  *mem_vhmap_random (struct mem_vhmap *db);
