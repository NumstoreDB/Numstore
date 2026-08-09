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

#ifndef NS_KVT_H
#define NS_KVT_H

#include "core/ns_alloc.h"
#include "core/ns_error.h"
#include "core/ns_linked_list.h"
#include "core/ns_stdtypes.h"

/******************************************************************************
 * SECTION: Key Value Type List
 * ----------------------------------------------------------------------------
 * @brief A list of string key type value
 ******************************************************************************/

struct kvt_list
{
  u16            len;
  struct string *keys;
  struct type  **types;
};

struct kv_llnode
{
  struct string key;
  struct type  *value;
  struct llnode link;
};

struct kvt_list_builder
{
  struct llnode  *head;
  u16             klen;
  u16             tlen;
  struct builder *b;
};

struct kvt_list_builder kvlb_create (struct builder *b);
err_t                   kvlb_accept_key (struct kvt_list_builder *ub, struct string key, error *e);
err_t                   kvlb_accept_type (struct kvt_list_builder *eb, struct type *t, error *e);
err_t                   kvlb_build (struct kvt_list *dest, struct kvt_list_builder *eb, error *e);

#endif
