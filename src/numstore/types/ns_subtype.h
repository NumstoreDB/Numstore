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

#ifndef TYPES_H
#define TYPES_H

#include "alloc.h"
#include "collections.h" // llnode
#include "error.h"       // error
#include "numstore.h"    // pgno ...etc
#include "serial.h"      // string
#include "stdtypes.h"    // u32 ...etc

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
