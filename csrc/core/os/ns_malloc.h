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

#ifndef NS_MALLOC_H
#define NS_MALLOC_H

#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_stdtypes.h"

////////////////////////////
/// Global allocator interface
///
/// Call these explicitly through the handle that owns them, e.g.
///
///   void *p = mem.table->malloc (mem.self, 1, sizeof *p, e);
///   mem.table->free (mem.self, p);
///
/// free is never handed a NULL pointer - use mem_cfree for a maybe-NULL one.

struct i_mem_vtable
{
  void *(*malloc) (void *self, const u32 nelem, const u32 size, error *e);
  void *(*calloc) (void *self, const u32 nelem, const u32 size, error *e);
  void *(*realloc) (void *self, void *ptr, const u32 nelem, const u32 size, error *e);
  void (*free) (void *self, void *v);
};

struct i_mem
{
  const struct i_mem_vtable *table;
  void                      *self;
};

DEFINE_DBG_ASSERT (struct i_mem, i_mem, m, {
  ASSERT (m);
  ASSERT (m->table);
})

/// The system default allocator - malloc/calloc/realloc/free
struct i_mem default_mem (void);

// Faulty memory
err_t faulty_mem_create (struct i_mem delegate, float fail_percent, struct i_mem *dest, error *e);
void faulty_mem_set_fail (struct i_mem mem, float fail_percent);
void faulty_mem_fail_after (struct i_mem mem, int count);
void faulty_mem_reset (struct i_mem mem);
u32 faulty_num_mallocs (struct i_mem mem);
void faulty_mem_free (struct i_mem mem);

// Free if not null
#define mem_cfree(mem, ptr)                  \
  do {                                       \
    if ((ptr)) {                             \
      (mem).table->free ((mem).self, (ptr)); \
    }                                        \
  }                                          \
  while (0)

#endif
