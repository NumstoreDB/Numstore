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

#include "core/os/ns_malloc.h"

#include "core/ns_bounds.h"
#include "core/ns_csx_assert.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

static void *
default_malloc (void *self, const u32 nelem, const u32 size, error *e)
{
  ASSERT (self == NULL);
  ASSERT (nelem > 0);
  ASSERT (size > 0);

  u32 bytes;
  if (!safe_mul_u32 (&bytes, nelem, size)) {
    error_causef (e, ERR_NOMEM, "malloc %u*%u: overflow", nelem, size);
    return NULL;
  }

  errno     = 0;
  void *ret = malloc ((size_t)bytes);
  if (ret == NULL) {
    if (errno == ENOMEM) {
      error_causef (e, ERR_NOMEM, "malloc %u*%u: %s", nelem, size, strerror (errno));
    } else {
      error_causef (e, ERR_NOMEM, "malloc %u*%u: out of memory", nelem, size);
    }
  }
  return ret;
}

static void *
default_calloc (void *self, const u32 nelem, const u32 size, error *e)
{
  ASSERT (self == NULL);
  ASSERT (nelem > 0);
  ASSERT (size > 0);

  u32 bytes = 0;
  if (!safe_mul_u32 (&bytes, nelem, size)) {
    error_causef (e, ERR_NOMEM, "calloc %u*%u: overflow", nelem, size);
    return NULL;
  }

  ASSERT (bytes > 0);

  errno     = 0;
  void *ret = calloc ((size_t)nelem, (size_t)size);
  if (ret == NULL) {
    if (errno == ENOMEM) {
      error_causef (e, ERR_NOMEM, "calloc %u*%u: %s", nelem, size, strerror (errno));
    } else {
      error_causef (e, ERR_NOMEM, "calloc %u*%u: out of memory", nelem, size);
    }
  }
  return ret;
}

static void *
default_realloc (void *self, void *ptr, const u32 nelem, const u32 size, error *e)
{
  ASSERT (self == NULL);
  ASSERT (nelem > 0);
  ASSERT (size > 0);

  u32 bytes = 0;
  if (!safe_mul_u32 (&bytes, nelem, size)) {
    error_causef (e, ERR_NOMEM, "realloc %u*%u: overflow", nelem, size);
    return NULL;
  }

  errno     = 0;
  void *ret = realloc (ptr, (size_t)bytes);
  if (ret == NULL) {
    error_causef (e, ERR_NOMEM, "realloc %u bytes: %s", bytes, strerror (errno));
    return NULL;
  }
  return ret;
}

static void
default_free (void *self, void *v)
{
  ASSERT (self == NULL);
  ASSERT (v);
  free (v);
}

static const struct i_mem_vtable default_mem_vtable = {
    .malloc  = default_malloc,
    .calloc  = default_calloc,
    .realloc = default_realloc,
    .free    = default_free,
};

struct i_mem
default_mem (void)
{
  return (struct i_mem){
      .self  = NULL,
      .table = &default_mem_vtable,
  };
}
