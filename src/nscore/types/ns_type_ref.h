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

#ifndef NS_TYPE_REF_H
#define NS_TYPE_REF_H

#include "core/ns_alloc.h"
#include "core/ns_error.h" // error
#include "core/ns_linked_list.h"
#include "core/ns_platform.h"
#include "core/ns_stdtypes.h" // u32 ...etc
#include "core/ns_string.h"
#include "nscore/types/ns_type_accessor.h"

#include <stdbool.h>

struct allocator;
struct builder;
struct type;

/******************************************************************************
 * SECTION: Type Reference
 * ----------------------------------------------------------------------------
 * @brief A reference to a specific type
 ******************************************************************************/

struct type_ref
{
  enum type_ref_t
  {
    TR_TAKE,
    TR_STRUCT,
  } type;

  union {
    struct take_tr
    {
      struct string        vname;
      struct type_accessor ta;
    } tk;

    struct struct_tr
    {
      u16              len;
      struct string   *keys;
      struct type_ref *types;
    } st;
  };
};

bool type_ref_equal (struct type_ref left, const struct type_ref right);
struct type *tr_construct (
    struct type      *reftype,
    struct type_ref  *tr,
    struct allocator *alloc,
    error            *e
);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Simple Stack Constructors
 *----------------------------------------------------------------------------*/

HEADER_FUNC struct type_ref
tr_take (struct string name, struct type_accessor ta)
{
  return (struct type_ref){
      .type = TR_TAKE,
      .tk   = {
          .vname = name,
          .ta    = ta,
      },
  };
}

HEADER_FUNC struct type_ref
tr_struct (u16 len, struct string *keys, struct type_ref *types)
{
  return (struct type_ref){
      .type = TR_STRUCT,
      .st   = {
          .len   = (len),
          .keys  = keys,
          .types = types,
      },
  };
}

/******************************************************************************
 * SECTION: Key Value Type Reference List
 * ----------------------------------------------------------------------------
 * @brief A reference of key and values
 ******************************************************************************/

struct kvt_ref_list
{
  u16              len;
  struct string   *keys;
  struct type_ref *types;
};

struct kv_ref_llnode
{
  struct string   key;
  struct type_ref value;
  struct llnode   link;
};

struct kvt_ref_list_builder
{
  struct llnode  *head;

  u16             klen;
  u16             tlen;

  struct builder *b;
};

struct kvt_ref_list_builder kvrlb_create (struct builder *b);
err_t kvrlb_accept_key (struct kvt_ref_list_builder *ub, struct string key, error *e);
err_t kvrlb_accept_type (struct kvt_ref_list_builder *eb, struct type_ref t, error *e);
err_t kvrlb_build (struct kvt_ref_list *dest, struct kvt_ref_list_builder *eb, error *e);

#endif
