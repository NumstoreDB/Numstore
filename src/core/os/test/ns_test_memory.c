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

#include "core/os/test/ns_test_memory.h"

#include "core/os/ns_memory.h"

#define OOM_WITH_PROB(data, label, expr)                   \
  do {                                                     \
    if (rand () < data->label) {                           \
      error_causef (e, ERR_IO, #expr "Simulated failure"); \
      return NULL;                                         \
    } else {                                               \
      return expr;                                         \
    }                                                      \
  }                                                        \
  while (0)

static void *
test_malloc (void *v, const u32 nelem, const u32 size, error *e)
{
  struct test_memory_data *data = v;
  OOM_WITH_PROB (data, malloc_fail_prob, i_malloc (data->delegate, nelem, size, e));
}

static void *
test_calloc (void *v, const u32 nelem, const u32 size, error *e)
{
  struct test_memory_data *data = v;
  OOM_WITH_PROB (data, calloc_fail_prob, i_calloc (data->delegate, nelem, size, e));
}

static void *
test_realloc (void *v, void *ptr, const u32 nelem, const u32 size, error *e)
{
  struct test_memory_data *data = v;
  OOM_WITH_PROB (data, realloc_fail_prob, i_realloc (data->delegate, ptr, nelem, size, e));
}

static void
test_free (void *v, void *ptr)
{
  (void)v;
  free (ptr);
}

static const struct i_mem_table test_mem_table = {
    .malloc  = test_malloc,
    .calloc  = test_calloc,
    .realloc = test_realloc,
    .free    = test_free,
};

struct i_mem
test_memory (struct test_memory_data *data)
{
  return (struct i_mem){
      .table = &test_mem_table,
      .data  = data,
  };
}
