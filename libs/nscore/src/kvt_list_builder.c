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

#include "nscore/errors.h"
#include "nscore/types.h"

#include <c_specx.h>

DEFINE_DBG_ASSERT (struct kvt_list_builder, kvt_list_builder, s, {
  ASSERT (s);
  ASSERT (s->klen <= 10);
  ASSERT (s->tlen <= 10);
})

void
kvlb_create (
    struct kvt_list_builder *dest,
    struct chunk_alloc      *temp,
    struct chunk_alloc      *persistent
)
{
  *dest = (struct kvt_list_builder){
      .head       = NULL,
      .klen       = 0,
      .tlen       = 0,
      .temp       = temp,
      .persistent = persistent,
  };
  DBG_ASSERT (kvt_list_builder, dest);
}

static bool
kvlb_has_key_been_used (const struct kvt_list_builder *ub, struct string key)
{
  for (struct llnode *it = ub->head; it; it = it->next)
  {
    struct kv_llnode *kn = container_of (it, struct kv_llnode, link);
    if (string_equal (kn->key, key)) { return true; }
  }
  return false;
}

err_t
kvlb_accept_key (struct kvt_list_builder *ub, struct string key, error *e)
{
  DBG_ASSERT (kvt_list_builder, ub);

  // Check for duplicate keys
  if (kvlb_has_key_been_used (ub, key))
  {
    return error_causef (e, ERR_INTERP, "duplicate key: %.*s", key.len, key.data);
  }

  // Copy key data to persistent memory
  key.data = chunk_alloc_move_mem (ub->persistent, key.data, key.len, e);
  if (key.data == NULL) { return error_trace (e); }

  // Find where to insert this new key in the linked list
  struct llnode    *slot = llnode_get_n (ub->head, ub->klen);
  struct kv_llnode *node;

  if (slot) { node = container_of (slot, struct kv_llnode, link); }
  else
  {
    // Allocate new node onto temp
    node = chunk_malloc (ub->temp, 1, sizeof *node, e);
    if (!node) { return error_trace (e); }
    llnode_init (&node->link);
    node->value = NULL;

    // Set the head if it doesn't exist
    if (!ub->head) { ub->head = &node->link; }

    // Otherwise, append to the list
    else
    {
      list_append (&ub->head, &node->link);
    }
  }

  // Create the node
  node->key = key;
  ub->klen++;

  return SUCCESS;
}

err_t
kvlb_accept_type (struct kvt_list_builder *ub, struct type *t, error *e)
{
  DBG_ASSERT (kvt_list_builder, ub);

  struct llnode    *slot = llnode_get_n (ub->head, ub->tlen);
  struct kv_llnode *node;
  if (slot) { node = container_of (slot, struct kv_llnode, link); }
  else
  {
    node = chunk_malloc (ub->temp, 1, sizeof *node, e);
    if (!node) { return error_trace (e); }
    llnode_init (&node->link);
    node->key = (struct string){0};
    if (!ub->head) { ub->head = &node->link; }
    else
    {
      list_append (&ub->head, &node->link);
    }
  }

  node->value = t;
  ub->tlen++;
  return SUCCESS;
}

err_t
kvlb_build (struct kvt_list *dest, struct kvt_list_builder *ub, error *e)
{
  ASSERT (dest);

  if (ub->klen == 0) { return error_causef (e, ERR_INTERP, "no keys"); }
  if (ub->klen != ub->tlen) { return error_causef (e, ERR_INTERP, "key/value count mismatch"); }

  struct string *keys = chunk_malloc (ub->persistent, ub->klen, sizeof *keys, e);
  if (!keys) { return error_trace (e); }

  struct type **types = chunk_malloc (ub->persistent, ub->tlen, sizeof (struct type *), e);
  if (!types) { return error_trace (e); }

  size_t i = 0;
  for (struct llnode *it = ub->head; it; it = it->next)
  {
    struct kv_llnode *kn = container_of (it, struct kv_llnode, link);
    keys[i]              = kn->key;
    types[i]             = kn->value;
    i++;
  }

  dest->keys  = keys;
  dest->types = types;
  dest->len   = ub->klen;

  return SUCCESS;
}

#ifndef NTEST
TEST (kvt_list_builder)
{
  error err = error_create ();

  struct chunk_alloc persistent;
  chunk_alloc_create_default (&persistent);

  // 0. freshly-created builder must be clean
  struct kvt_list_builder kb;
  kvlb_create (&kb, &persistent, &persistent);
  test_assert_int_equal (kb.klen, 0);
  test_assert_int_equal (kb.tlen, 0);
  test_fail_if (kb.head != NULL);

  // 1. accept first key "id"
  struct string key_id = strfcstr ("id");
  test_assert_int_equal (kvlb_accept_key (&kb, key_id, &err), SUCCESS);

  // 2. duplicate key "id" must fail
  test_assert_int_equal (kvlb_accept_key (&kb, key_id, &err), ERR_INTERP);
  err.cause_code = SUCCESS;

  // 3. accept a type for that key (u32)
  struct type t_u32 = (struct type){.type = T_PRIM, .p = U32};
  test_assert_int_equal (kvlb_accept_type (&kb, &t_u32, &err), SUCCESS);

  // 4. accept second key/value pair ("name", i32)
  struct string key_name = strfcstr ("name");
  test_assert_int_equal (kvlb_accept_key (&kb, key_name, &err), SUCCESS);
  struct type t_i32 = (struct type){.type = T_PRIM, .p = I32};
  test_assert_int_equal (kvlb_accept_type (&kb, &t_i32, &err), SUCCESS);

  // 5. mismatched key/type counts -> build must fail
  struct string key_extra = strfcstr ("extra");
  test_assert_int_equal (kvlb_accept_key (&kb, key_extra, &err),
                         SUCCESS); // klen=3, tlen=2
  struct kvt_list list_fail = {0};
  test_assert_int_equal (kvlb_build (&list_fail, &kb, &err), ERR_INTERP);
  err.cause_code = SUCCESS;

  // 6. add matching type so counts align
  struct type t_f32 = (struct type){.type = T_PRIM, .p = F32};
  test_assert_int_equal (kvlb_accept_type (&kb, &t_f32, &err), SUCCESS);

  // 7. successful build
  struct kvt_list list = {0};
  test_assert_int_equal (kvlb_build (&list, &kb, &err), SUCCESS);
  test_assert_int_equal (list.len, 3);
  // 8. ensure key order preserved (id, name, extra)
  test_assert_int_equal (string_equal (list.keys[0], key_id), true);
  test_assert_int_equal (string_equal (list.keys[1], key_name), true);
  test_assert_int_equal (string_equal (list.keys[2], key_extra), true);

  // 9. ensure type mapping correct
  test_assert_int_equal (list.types[0]->p, t_u32.p);
  test_assert_int_equal (list.types[1]->p, t_i32.p);
  test_assert_int_equal (list.types[2]->p, t_f32.p);

  chunk_alloc_free_all (&persistent);
}
#endif
