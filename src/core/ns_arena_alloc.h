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

/**
 * @file
 * @brief Allocators and memory allocation patterns
 *
 * Alloc.h contains specialized allocators used in numstore
 * such as:
 * - Slab Allocator  - an allocator that allocates fixed size slabs
 * - Malloc Plan     - a two phase malloc - defining the size - then one malloc
 */

#ifndef NS_CHUNK_ALLOC_H
#define NS_CHUNK_ALLOC_H

#include "core/ns_concurrency.h"
#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "core/os/ns_memory.h"

struct arena_alloc_settings
{
  u32          max_alloc_size;
  u32          max_total_size;
  float        target_chunk_mult;
  u32          min_chunk_size;
  u32          max_chunk_size;
  u32          max_chunks;
  struct i_mem mem;
};

struct arena_alloc
{
  latch                       latch;
  struct arena_alloc_settings settings;
  struct chunk               *head;
  u32                         num_chunks;
  u32                         total_allocated;
  u32                         total_used;
};

void arena_alloc_create_default (struct arena_alloc *dest);
void *chunk_malloc (struct arena_alloc *ca, u32 req, u32 size, error *e);
void arena_alloc_free_all (struct arena_alloc *ca);

#endif
