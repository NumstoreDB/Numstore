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
 * SECTION: SArray Builder
 * ----------------------------------------------------------------------------
 * @brief A builder for a strict array
 ******************************************************************************/

struct dim_llnode
{
  u32           dim;
  struct llnode link;
};

struct sarray_builder
{
  struct llnode  *head;
  struct type    *type;
  struct builder *b;
};

struct sarray_builder sab_create (struct builder *b);
err_t sab_accept_dim (struct sarray_builder *eb, i32 dim, error *e);
err_t sab_accept_type (struct sarray_builder *eb, struct type *t, error *e);
err_t sab_build (
    struct sarray_t       *persistent,
    struct sarray_builder *eb,
    error                 *e
);

#endif
