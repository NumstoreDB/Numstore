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

#pragma once

#include "c_specx.h"
#include "nscore/type_accessor.h"
#include "nscore/types.h"

struct subtype
{
  struct string        vname;
  struct type_accessor ta;
};

err_t subtype_create (struct subtype *dest, struct string vname, struct type_accessor ta, error *e);

bool subtype_equal (const struct subtype *left, const struct subtype *right);

struct type *subtype_get_type (
    struct type          *stype,
    struct type_accessor *ta,
    struct chunk_alloc   *alloc,
    error                *e
);
