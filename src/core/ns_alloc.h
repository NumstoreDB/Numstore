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

#ifndef NS_ALLOC_H
#define NS_ALLOC_H

#include "core/ns_chunk_alloc.h"
#include "core/ns_error.h"
#include "core/ns_stdtypes.h"

/******************************************************************************
 * SECTION: Generic Allocator
 * ----------------------------------------------------------------------------
 * @brief Just a container for any generic allocator
 ******************************************************************************/

struct allocator
{
  enum
  {
    AT_CHUNK_ALLOCATOR,
    AT_MALLOC,
  } type;

  union {
    struct chunk_alloc calloc;
  };
};

void create_default_allocator (struct allocator *alloc);
void *allocate (struct allocator *alloc, u32 nelem, u32 size, error *e);
void *allocator_copy (struct allocator *alloc, const void *ptr, u32 size, error *e);
void allocator_free (struct allocator *alloc);

#define ALLOC_INIT(name) \
  struct allocator name; \
  create_default_allocator (&name)

#define ALLOC_CLOSE(name) allocator_free (&name)
#define ALLOC_RESET(name) \
  allocator_free (&name); \
  create_default_allocator (&name)

/******************************************************************************
 * SECTION: Builder Pattern
 * ----------------------------------------------------------------------------
 * @brief Contains two allocators - a persistent allocator and a temp
 ******************************************************************************/

struct builder
{
  struct allocator *persistent;
  struct allocator  temp;
};

void builder_init (struct builder *b, struct allocator *alloc);
void *builder_malloc_temp (struct builder *b, u32 nelem, u32 size, error *e);
void *builder_malloc_persist (struct builder *b, u32 nelem, u32 size, error *e);
void builder_free (struct builder *b);

#define BUILDER_INIT(name, alloc) \
  struct builder name;            \
  builder_init (&name, alloc)

#define BUILDER_CLOSE(name) builder_free (&name)

#endif // NS_ALLOC_H
