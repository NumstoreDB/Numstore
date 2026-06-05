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

#ifndef C_SPECX_MEMORY_H
#define C_SPECX_MEMORY_H

#include <c_specx/error.h>
#include <c_specx/platform.h>
#include <c_specx/stdtypes.h>

////////////////////////////////////////////////////////////
// MEMORY

typedef struct i_vmem i_vmem;

struct i_vmem
{
  void *(*i_malloc) (i_vmem *v, u32 nelem, u32 size, error *e);
  void *(*i_calloc) (i_vmem *v, u32 nelem, u32 size, error *e);
  void *(*i_realloc_right) (
      i_vmem *v,
      void   *ptr,
      u32     old_nelem,
      u32     nelem,
      u32     size,
      error  *e
  );
  void *(*i_realloc_left) (
      i_vmem *v,
      void   *ptr,
      u32     old_nelem,
      u32     nelem,
      u32     size,
      error  *e
  );
  void *(*i_crealloc_right) (
      i_vmem *v,
      void   *ptr,
      u32     old_nelem,
      u32     nelem,
      u32     size,
      error  *e
  );
  void *(*i_crealloc_left) (
      i_vmem *v,
      void   *ptr,
      u32     old_nelem,
      u32     nelem,
      u32     size,
      error  *e
  );
  void (*i_free) (i_vmem *v, void *ptr);
};

extern struct i_vmem default_vmem;

HEADER_FUNC void *
i_malloc (u32 nelem, u32 size, error *e)
{
  return default_vmem.i_malloc (&default_vmem, nelem, size, e);
}
HEADER_FUNC void *
i_calloc (u32 nelem, u32 size, error *e)
{
  return default_vmem.i_calloc (&default_vmem, nelem, size, e);
}
HEADER_FUNC void *
i_realloc_right (void *ptr, u32 old_nelem, u32 nelem, u32 size, error *e)
{
  return default_vmem
      .i_realloc_right (&default_vmem, ptr, old_nelem, nelem, size, e);
}
HEADER_FUNC void *
i_realloc_left (void *ptr, u32 old_nelem, u32 nelem, u32 size, error *e)
{
  return default_vmem
      .i_realloc_left (&default_vmem, ptr, old_nelem, nelem, size, e);
}
HEADER_FUNC void *
i_crealloc_right (void *ptr, u32 old_nelem, u32 nelem, u32 size, error *e)
{
  return default_vmem
      .i_crealloc_right (&default_vmem, ptr, old_nelem, nelem, size, e);
}
HEADER_FUNC void *
i_crealloc_left (void *ptr, u32 old_nelem, u32 nelem, u32 size, error *e)
{
  return default_vmem
      .i_crealloc_left (&default_vmem, ptr, old_nelem, nelem, size, e);
}
HEADER_FUNC void
i_free (void *ptr)
{
  default_vmem.i_free (&default_vmem, ptr);
}
#define i_cfree(ptr) \
  do                 \
  {                  \
    if (ptr)         \
    {                \
      i_free (ptr);  \
    }                \
  }                  \
  while (0)

#endif // C_SPECX_MEMORY_H
