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

////////////////////////////////////////////////////////////
/// Type Ref

struct type_ref {
  enum type_ref_t {
    TR_TAKE,
    TR_STRUCT,
  } type;

  union {
    struct take_tr {
      struct string        vname;
      struct type_accessor ta;
    } tk;

    struct struct_tr {
      u16              len;
      struct string   *keys;
      struct type_ref *types;
    } st;
  };
};
bool type_ref_equal (struct type_ref left, const struct type_ref right);
struct type *
tr_construct (struct type *reftype, struct type_ref *tr, struct chunk_alloc *alloc, error *e);

////////////////////////////////////////////////////////////
/// KVT List

struct kvt_ref_list {
  u16              len;
  struct string   *keys;
  struct type_ref *types;
};

struct kv_ref_llnode {
  struct string   key;
  struct type_ref value;
  struct llnode   link;
};

struct kvt_ref_list_builder {
  struct llnode *head;

  u16 klen;
  u16 tlen;

  struct chunk_alloc *temp;
  struct chunk_alloc *persistent;
};

void kvrlb_create (
    struct kvt_ref_list_builder *dest,
    struct chunk_alloc          *temp,
    struct chunk_alloc          *persistent);

err_t kvrlb_accept_key (struct kvt_ref_list_builder *ub, struct string key, error *e);

err_t kvrlb_accept_type (struct kvt_ref_list_builder *eb, struct type_ref t, error *e);

err_t kvrlb_build (struct kvt_ref_list *dest, struct kvt_ref_list_builder *eb, error *e);
