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

#ifndef NS_SARRAY_T_H
#define NS_SARRAY_T_H

#include <stdbool.h>

#include "core/ns_alloc.h"
#include "core/ns_error.h"
#include "core/ns_linked_list.h"
#include "core/ns_stdtypes.h"
#include "nscore/types/ns_types.h"

struct allocator;
struct builder;
struct deserializer;
struct sarray_t;
struct serializer;
struct type;

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
err_t                 sab_accept_dim (struct sarray_builder *eb, i32 dim, error *e);
err_t                 sab_accept_type (struct sarray_builder *eb, struct type *t, error *e);
err_t                 sab_build (struct sarray_t *persistent, struct sarray_builder *eb, error *e);

u32   sarray_t_get_serial_size (const struct sarray_t *t);
err_t sarray_t_deserialize (
    struct sarray_t     *persistent,
    struct deserializer *src,
    struct allocator    *a,
    error               *e
);

void sarray_t_serialize (struct serializer *persistent, const struct sarray_t *src);

err_t sarray_t_validate (const struct sarray_t *t, error *e);
i32   sarray_t_snprintf (char *str, u32 size, const struct sarray_t *p);
u32   sarray_t_byte_size (const struct sarray_t *t);
err_t sarray_t_random (struct sarray_t *sa, struct allocator *temp, u32 depth, error *e);
bool  sarray_t_equal (const struct sarray_t *left, const struct sarray_t *right);

#endif
