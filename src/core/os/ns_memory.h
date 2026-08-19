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

#ifndef NS_MEMORY_H
#define NS_MEMORY_H

#include "core/ns_error.h"
#include "core/ns_stdtypes.h"

/**
 * An interface for global memory allocation
 */
struct i_mem_table
{
  void *(*malloc) (void *v, u32 nelem, u32 size, error *e);
  void *(*calloc) (void *v, u32 nelem, u32 size, error *e);
  void *(*realloc) (void *v, void *ptr, u32 nelem, u32 size, error *e);
  void (*free) (void *v, void *ptr);
};

struct i_mem
{
  const struct i_mem_table *table;
  void                     *data;
};

// Default global allocator
struct i_mem default_mem (void);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Default Methods
 *----------------------------------------------------------------------------*/

#define i_malloc(mem, nelem, size, e)       (mem).table->malloc ((mem).data, nelem, size, e)
#define i_calloc(mem, nelem, size, e)       (mem).table->calloc ((mem).data, nelem, size, e)
#define i_realloc(mem, ptr, nelem, size, e) (mem).table->realloc ((mem).data, ptr, nelem, size, e)
#define i_free(mem, ptr)                    (mem).table->free ((mem).data, ptr)
#define i_cfree(mem, ptr) \
  do                      \
  {                       \
    if (ptr)              \
    {                     \
      i_free (mem, ptr);  \
    }                     \
  }                       \
  while (0)

#endif
