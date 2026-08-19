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

#include "core/os/ns_memory.h"

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "core/ns_bounds.h"
#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "core/testing/ns_testing.h"

/******************************************************************************
 * SECTION: Memory
 ******************************************************************************/

static void *
def_malloc (void *v, const u32 nelem, const u32 size, error *e)
{
  (void)v;

  ASSERT (nelem > 0);
  ASSERT (size > 0);

  u32 bytes;
  if (!safe_mul_u32 (&bytes, nelem, size))
  {
    error_causef (e, ERR_NOMEM, "malloc %d*%d: overflow", nelem, size);
    return NULL;
  }

  errno     = 0;
  void *ret = malloc ((size_t)bytes);
  if (ret == NULL)
  {
    if (errno == ENOMEM)
    {
      error_causef (e, ERR_NOMEM, "malloc %d*%d: %s", nelem, size, strerror (errno));
    }
    else
    {
      error_causef (e, ERR_NOMEM, "malloc: %s", strerror (errno));
    }
  }
  return ret;
}

static void *
def_calloc (void *v, const u32 nelem, const u32 size, error *e)
{
  (void)v;

  ASSERT (nelem > 0);
  ASSERT (size > 0);

  u32 bytes = 0;
  if (!safe_mul_u32 (&bytes, nelem, size))
  {
    error_causef (e, ERR_NOMEM, "malloc %d*%d: overflow", nelem, size);
    return NULL;
  }

  ASSERT (bytes > 0);

  errno     = 0;
  void *ret = calloc ((size_t)nelem, (size_t)size);
  if (ret == NULL)
  {
    if (errno == ENOMEM)
    {
      error_causef (e, ERR_NOMEM, "calloc %d*%d: %s", nelem, size, strerror (errno));
    }
    else
    {
      error_causef (e, ERR_NOMEM, "calloc: %s", strerror (errno));
    }
  }
  return ret;
}

static void *
def_realloc (void *v, void *ptr, const u32 nelem, const u32 size, error *e)
{
  (void)v;

  ASSERT (nelem > 0);
  ASSERT (size > 0);

  u32 bytes = 0;
  {
    bool ok = safe_mul_u32 (&bytes, nelem, size);
    ASSERT (ok);
    if (!ok)
    {
      error_causef (e, ERR_NOMEM, "realloc %u*%u: overflow", nelem, size);
      return NULL;
    }
  }

  errno     = 0;
  void *ret = realloc (ptr, (size_t)bytes);
  if (ret == NULL)
  {
    error_causef (e, ERR_NOMEM, "realloc %u bytes: %s", bytes, strerror (errno));
    return NULL;
  }
  return ret;
}

#ifdef TESTING
TEST (i_realloc_basic)
{
  error e = error_create ();

  u32 *a = i_realloc (mem, NULL, 10, sizeof *a, &e); // behaves like malloc
  for (u32 i = 0; i < 10; i++)
  {
    a[i] = i;
  }

  u32 *b = i_realloc (mem, a, 20, sizeof *b, &e);
  for (u32 i = 0; i < 10; i++)
  {
    test_assert (b[i] == i);
  }

  i_free (mem, b);
}
#endif

static void
def_free (void *v, void *ptr)
{
  (void)v;
  ASSERT (ptr);
  free (ptr);
}

static const struct i_mem_table default_mem_table = {
    .malloc  = def_malloc,
    .calloc  = def_calloc,
    .realloc = def_realloc,
    .free    = def_free,
};

struct i_mem
default_mem (void)
{
  return (struct i_mem){
      .table = &default_mem_table,
      .data  = NULL,
  };
}
