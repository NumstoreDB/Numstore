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

#include "core/ns_alloc.h"

#include <stdatomic.h>
#include <string.h>

#include "core/ns_chunk_alloc.h"
#include "core/ns_csx_assert.h"
#include "core/ns_error.h"

void
create_default_allocator (struct allocator *alloc)
{
  alloc->type = AT_CHUNK_ALLOCATOR;
  chunk_alloc_create_default (&alloc->calloc);
}

void *
allocate (struct allocator *alloc, u32 nelem, u32 size, error *e)
{
  ASSERT (alloc);
  switch (alloc->type)
  {
    case AT_CHUNK_ALLOCATOR:
    {
      return chunk_malloc (&alloc->calloc, nelem, size, e);
    }
    default:
    {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
}

void *
allocator_copy (struct allocator *alloc, const void *ptr, u32 size, error *e)
{
  void *dest = allocate (alloc, size, 1, e);

  if (dest == NULL)
  {
    return NULL;
  }

  memcpy (dest, ptr, size);

  return dest;
}

void
allocator_free (struct allocator *alloc)
{
  switch (alloc->type)
  {
    case AT_CHUNK_ALLOCATOR:
    {
      chunk_alloc_free_all (&alloc->calloc);
      return;
    }
    default:
    {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
  UNREACHABLE ();
}

void
builder_init (struct builder *b, struct allocator *alloc)
{
  b->persistent = alloc;
  create_default_allocator (&b->temp);
}

void *
builder_malloc_temp (struct builder *b, u32 nelem, u32 size, error *e)
{
  return allocate (&b->temp, nelem, size, e);
}

void *
builder_malloc_persist (struct builder *b, u32 nelem, u32 size, error *e)
{
  return allocate (b->persistent, nelem, size, e);
}

void
builder_free (struct builder *b)
{
  allocator_free (&b->temp);
}

struct allocator g_malloc = {
    .type = AT_MALLOC,
};
