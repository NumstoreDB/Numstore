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

#ifndef NS_TYPE_ACCESSOR_H
#define NS_TYPE_ACCESSOR_H

#include "core/ns_alloc.h"
#include "core/ns_byte_accessor.h"
#include "core/ns_error.h"
#include "core/ns_linked_list.h"
#include "core/ns_platform.h"
#include "core/ns_stdtypes.h"
#include "core/ns_stride.h"
#include "core/ns_string.h"

#include <stdbool.h>

struct allocator;
struct builder;
struct type;

/******************************************************************************
 * SECTION: Type Accessor
 * ----------------------------------------------------------------------------
 * @brief How to access an individual type
 ******************************************************************************/

struct type_accessor
{
  enum ta_type type;

  union {
    struct select_ta
    {
      struct string         key;
      struct type_accessor *sub_ta;
    } select;

    struct range_ta
    {
      struct user_stride   *dim_accessors;
      u32                   dlen;
      struct type_accessor *sub_ta;
    } range;
  };
};

bool type_accessor_equal (const struct type_accessor left, const struct type_accessor right);
struct type *ta_subtype (
    struct type          *reftype,
    struct type_accessor *ta,
    struct allocator     *alloc,
    error                *e
);
struct byte_accessor *type_to_byte_accessor (
    struct type_accessor *src,
    struct type          *reftype,
    struct allocator     *dalloc,
    error                *e
);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Simple Stack Constructors
 *----------------------------------------------------------------------------*/

#define ta_take() ((struct type_accessor){.type = TA_TAKE})

#define ta_select(_key, _sub_ta) \
  ((struct type_accessor){       \
      .type   = TA_SELECT,       \
      .select = {                \
          .key    = (_key),      \
          .sub_ta = (_sub_ta),   \
      },                         \
  })

HEADER_FUNC struct type_accessor
ta_range (struct user_stride *dim_accessors, u16 dlen, struct type_accessor *sub_ta)
{
  return (struct type_accessor){
      .type  = TA_RANGE,
      .range = (struct range_ta){
          .dim_accessors = dim_accessors,
          .dlen          = dlen,
          .sub_ta        = sub_ta,
      },
  };
}

////////////////////////////////////////////////////////////
/// BUILDER

struct rb_llnode
{
  struct user_stride stride;
  struct llnode      link;
};

struct range_builder
{
  struct llnode  *head;
  u32             len;
  struct builder *b;
};

struct range_builder rb_create (struct builder *b);
err_t rb_accept_stride (struct range_builder *rb, struct user_stride stride, error *e);
err_t rb_build (struct range_ta *dest, struct range_builder *rb, error *e);

struct type_accessor_builder
{
  struct type_accessor  ret;
  struct type_accessor *head;
  struct type_accessor *tail;
  struct builder       *b;
  struct range_builder  rb;
  bool                  in_range;
};

struct type_accessor_builder tab_create (struct builder *b);
err_t tab_accept_select (struct type_accessor_builder *builder, struct string key, error *e);
err_t tab_accept_stride (
    struct type_accessor_builder *builder,
    struct user_stride            stride,
    error                        *e
);
err_t tab_accept_take (struct type_accessor_builder *builder, error *e);
err_t tab_build (struct type_accessor *dest, struct type_accessor_builder *builder, error *e);

#endif
