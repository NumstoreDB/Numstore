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

#ifndef NS_SUBTYPE_H
#define NS_SUBTYPE_H

#include "core/ns_alloc.h"
#include "core/ns_error.h" // error
#include "core/ns_string.h"
#include "nscore/types/ns_type_accessor.h"

#include <stdbool.h>

struct allocator;
struct type;

/******************************************************************************
 * SECTION: Sub Type
 * ----------------------------------------------------------------------------
 * @brief A sub type accesses sub elements of a type
 ******************************************************************************/

struct subtype
{
  struct string        vname;
  struct type_accessor ta;
};

struct subtype subtype_create (struct string vname, struct type_accessor ta);
bool subtype_equal (const struct subtype *left, const struct subtype *right);
struct type *subtype_get_type (
    struct type          *stype,
    struct type_accessor *ta,
    struct allocator     *alloc,
    error                *e
);

#endif
