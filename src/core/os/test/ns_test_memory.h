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

#ifndef NS_TEST_MEMORY_H
#define NS_TEST_MEMORY_H

#include "core/os/ns_memory.h"

struct test_memory_data
{
  float        malloc_fail_prob;
  float        calloc_fail_prob;
  float        realloc_fail_prob;
  float        ree_fail_prob;

  struct i_mem delegate;
};

struct i_mem test_memory (struct test_memory_data *data);

#endif
