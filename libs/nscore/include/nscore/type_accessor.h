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

#include <c_specx.h>

#include "nscore/types.h"

////////////////////////////////////////////////////////////
/// TYPE ACCESSOR

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

bool type_accessor_equal (
    const struct type_accessor left,
    const struct type_accessor right
);

struct type *ta_subtype (
    struct type          *reftype,
    struct type_accessor *ta,
    struct chunk_alloc   *alloc,
    error                *e
);

bool user_stride_equal (
    const struct user_stride *left,
    const struct user_stride *right
);

struct byte_accessor *type_to_byte_accessor (
    struct type_accessor *src,
    struct type          *reftype,
    struct chunk_alloc   *dalloc,
    error                *e
);

////////////////////////////////////////////////////////////
/// BUILDER

struct rb_llnode
{
  struct user_stride stride;
  struct llnode      link;
};

struct range_builder
{
  struct llnode      *head;
  u32                 len;
  struct chunk_alloc *temp;
  struct chunk_alloc *persistent;
};

void rb_create (
    struct range_builder *dest,
    struct chunk_alloc   *temp,
    struct chunk_alloc   *persistent
);

err_t rb_accept_stride (
    struct range_builder *rb,
    struct user_stride    stride,
    error                *e
);

err_t rb_build (struct range_ta *dest, struct range_builder *rb, error *e);

struct type_accessor_builder
{
  struct type_accessor  ret;
  struct type_accessor *head;
  struct type_accessor *tail;
  struct chunk_alloc   *temp;
  struct chunk_alloc   *persistent;
  struct range_builder  rb;
  bool                  in_range;
};

void tab_create (
    struct type_accessor_builder *dest,
    struct chunk_alloc           *temp,
    struct chunk_alloc           *persistent
);

err_t tab_accept_select (
    struct type_accessor_builder *builder,
    struct string                 key,
    error                        *e
);

err_t tab_accept_stride (
    struct type_accessor_builder *builder,
    struct user_stride            stride,
    error                        *e
);

err_t tab_accept_take (struct type_accessor_builder *builder, error *e);

err_t tab_build (
    struct type_accessor         *dest,
    struct type_accessor_builder *builder,
    error                        *e
);
