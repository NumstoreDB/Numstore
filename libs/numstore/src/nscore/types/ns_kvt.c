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

#include "nscore/types/ns_kvt.h"

#include "core/ns_arena_alloc.h"
#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_linked_list.h"
#include "core/ns_string.h"
#include "core/ns_testing.h"
#include "core/ns_utils.h"
#include "nscore/types/ns_sarray_t.h"
#include "nscore/types/ns_type_ref.h"
#include "nscore/types/ns_types.h"
#include "nscore/types/ns_union_t.h"

#include <stdio.h>
#include <string.h>

/******************************************************************************
 * SECTION: Key Value Type Builder
 ******************************************************************************/

DEFINE_DBG_ASSERT (struct kvt_list_builder, kvt_list_builder, s, {
  ASSERT (s);
  ASSERT (s->klen <= 10);
  ASSERT (s->tlen <= 10);
})

struct kvt_list_builder
kvlb_create (struct builder *b)
{
  return (struct kvt_list_builder){
      .head = NULL,
      .klen = 0,
      .tlen = 0,
      .b    = b,
  };
}

static bool
kvlb_has_key_been_used (const struct kvt_list_builder *ub, struct string key)
{
  DBG_ASSERT (kvt_list_builder, ub);
  for (struct llnode *it = ub->head; it != NULL; it = it->next) {
    struct kv_llnode *kn = container_of (it, struct kv_llnode, link);
    if (string_equal (kn->key, key)) {
      return true;
    }
  }
  return false;
}

err_t
kvlb_accept_key (struct kvt_list_builder *ub, struct string key, error *e)
{
  DBG_ASSERT (kvt_list_builder, ub);

  if (kvlb_has_key_been_used (ub, key)) {
    return error_causef (e, ERR_INTERP, "duplicate key: %.*s", (int)key.len, key.data);
  }

  // Copy key data to persistent memory
  key.data = arena_alloc_copy (ub->b->persistent, key.data, key.len, e);
  if (key.data == NULL) {
    return error_trace (e);
  }

  // Keys and types fill the same list from the front, so a type accepted
  // first may already have made the node for this key
  struct llnode    *slot = llnode_get_n (ub->head, ub->klen);
  struct kv_llnode *node;

  if (slot != NULL) {
    node = container_of (slot, struct kv_llnode, link);
  } else {
    node = builder_malloc_temp (ub->b, 1, sizeof *node, e);
    if (node == NULL) {
      return error_trace (e);
    }
    llnode_init (&node->link);
    node->value = NULL;

    if (ub->head == NULL) {
      ub->head = &node->link;
    } else {
      list_append (&ub->head, &node->link);
    }
  }

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
  if (slot != NULL) {
    node = container_of (slot, struct kv_llnode, link);
  } else {
    node = builder_malloc_temp (ub->b, 1, sizeof *node, e);
    if (node == NULL) {
      return error_trace (e);
    }
    llnode_init (&node->link);
    node->key = (struct string){0};
    if (ub->head == NULL) {
      ub->head = &node->link;
    } else {
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
  DBG_ASSERT (kvt_list_builder, ub);
  ASSERT (dest);

  if (ub->klen == 0) {
    return error_causef (e, ERR_INTERP, "no keys");
  }
  if (ub->klen != ub->tlen) {
    return error_causef (e, ERR_INTERP, "key/value count mismatch");
  }

  struct string *keys = builder_malloc_persist (ub->b, ub->klen, sizeof *keys, e);
  if (keys == NULL) {
    return error_trace (e);
  }

  struct type **types = (struct type **)builder_malloc_persist (ub->b, ub->tlen, sizeof *types, e);
  if (types == NULL) {
    return error_trace (e);
  }

  size_t i = 0;
  for (struct llnode *it = ub->head; it != NULL; it = it->next) {
    struct kv_llnode *kn = container_of (it, struct kv_llnode, link);
    keys[i]              = kn->key;
    types[i]             = kn->value;
    i++;
  }

  dest->len   = ub->klen;
  dest->keys  = keys;
  dest->types = types;

  return SUCCESS;
}

#ifndef NDEBUG
TEST (kvt_list_builder)
{
  BUILDER_INIT (b, &ctx->alloc);

  // 0. freshly-created builder must be clean
  struct kvt_list_builder kb = kvlb_create (&b);
  test_assert_int_equal (kb.klen, 0);
  test_assert_int_equal (kb.tlen, 0);
  test_fail_if (kb.head != NULL);

  // 1. accept first key "id"
  struct string key_id = strfcstr ("id");
  test_assert_int_equal (kvlb_accept_key (&kb, key_id, &ctx->e), SUCCESS);

  // 2. duplicate key "id" must fail
  test_assert_int_equal (kvlb_accept_key (&kb, key_id, &ctx->e), ERR_INTERP);
  error_reset (&ctx->e);

  // 3. accept a type for that key (u32)
  struct type t_u32 = (struct type){.type = T_PRIM, .p = U32};
  test_assert_int_equal (kvlb_accept_type (&kb, &t_u32, &ctx->e), SUCCESS);

  // 4. accept second key/value pair ("name", i32)
  struct string key_name = strfcstr ("name");
  test_assert_int_equal (kvlb_accept_key (&kb, key_name, &ctx->e), SUCCESS);
  struct type t_i32 = (struct type){.type = T_PRIM, .p = I32};
  test_assert_int_equal (kvlb_accept_type (&kb, &t_i32, &ctx->e), SUCCESS);

  // 5. mismatched key/type counts -> build must fail
  struct string key_extra = strfcstr ("extra");
  test_assert_int_equal (kvlb_accept_key (&kb, key_extra, &ctx->e),
                         SUCCESS); // klen=3, tlen=2
  struct kvt_list list_fail = {0};
  test_assert_int_equal (kvlb_build (&list_fail, &kb, &ctx->e), ERR_INTERP);
  error_reset (&ctx->e);

  // 6. add matching type so counts align
  struct type t_f32 = (struct type){.type = T_PRIM, .p = F32};
  test_assert_int_equal (kvlb_accept_type (&kb, &t_f32, &ctx->e), SUCCESS);

  // 7. successful build
  struct kvt_list list = {0};
  test_assert_int_equal (kvlb_build (&list, &kb, &ctx->e), SUCCESS);
  test_assert_int_equal (list.len, 3);
  // 8. ensure key order preserved (id, name, extra)
  test_assert (string_equal (list.keys[0], key_id));
  test_assert (string_equal (list.keys[1], key_name));
  test_assert (string_equal (list.keys[2], key_extra));

  // 9. ensure type mapping correct
  test_assert_int_equal (list.types[0]->p, t_u32.p);
  test_assert_int_equal (list.types[1]->p, t_i32.p);
  test_assert_int_equal (list.types[2]->p, t_f32.p);

  BUILDER_CLOSE (b);
  arena_alloc_free_all (&ctx->alloc);
}

TEST (kvt_list_builder_edges)
{
  struct type t_u32 = mk_prim (U32);
  struct type t_i8  = mk_prim (I8);

  TEST_CASE ("types accepted before their keys")
  {
    BUILDER_INIT (b, &ctx->alloc);
    struct kvt_list_builder kb = kvlb_create (&b);

    test_assert_int_equal (kvlb_accept_type (&kb, &t_u32, &ctx->e), SUCCESS);
    test_assert_int_equal (kvlb_accept_type (&kb, &t_i8, &ctx->e), SUCCESS);
    test_assert_int_equal (kvlb_accept_key (&kb, strfcstr ("a"), &ctx->e), SUCCESS);
    test_assert_int_equal (kvlb_accept_key (&kb, strfcstr ("b"), &ctx->e), SUCCESS);

    struct kvt_list list;
    test_assert_int_equal (kvlb_build (&list, &kb, &ctx->e), SUCCESS);
    test_assert_int_equal (list.len, 2);
    test_assert (string_equal (list.keys[1], strfcstr ("b")));
    test_assert_ptr_equal (list.types[1], &t_i8);

    BUILDER_CLOSE (b);
  }

  TEST_CASE ("no keys")
  {
    BUILDER_INIT (b, &ctx->alloc);
    struct kvt_list_builder kb = kvlb_create (&b);
    struct kvt_list         list;
    test_err_t_check (kvlb_build (&list, &kb, &ctx->e), ERR_INTERP, &ctx->e);
    BUILDER_CLOSE (b);
  }

  TEST_CASE ("key copy out of memory")
  {
    struct arena_alloc persist;
    type_test_arena_create (&persist, ctx->faulty_mem);
    BUILDER_INIT (b, &persist);
    struct kvt_list_builder kb = kvlb_create (&b);

    faulty_mem_fail_after (ctx->faulty_mem, 0);
    test_err_t_check (kvlb_accept_key (&kb, strfcstr ("a"), &ctx->e), ERR_NOMEM, &ctx->e);
    faulty_mem_reset (ctx->faulty_mem);

    BUILDER_CLOSE (b);
    arena_alloc_free_all (&persist);
  }

  // Each of the two persistent arrays kvlb_build makes can fail
  int   fail_after = 0;
  err_t ret;
  do {
    TEST_CASE ("build. Fail after: %d", fail_after)
    {
      error_reset (&ctx->e);

      struct arena_alloc persist;
      type_test_arena_create (&persist, ctx->faulty_mem);
      BUILDER_INIT (b, &persist);
      struct kvt_list_builder kb = kvlb_create (&b);
      test_assert_int_equal (kvlb_accept_key (&kb, strfcstr ("a"), &ctx->e), SUCCESS);
      test_assert_int_equal (kvlb_accept_type (&kb, &t_u32, &ctx->e), SUCCESS);

      struct kvt_list list;
      faulty_mem_fail_after (ctx->faulty_mem, fail_after);
      ret = kvlb_build (&list, &kb, &ctx->e);
      faulty_mem_reset (ctx->faulty_mem);

      BUILDER_CLOSE (b);
      arena_alloc_free_all (&persist);
      fail_after++;
    }
  }
  while (ret != SUCCESS);

  arena_alloc_free_all (&ctx->alloc);
}
#endif

/******************************************************************************
 * SECTION: Key Value Type List Builder
 ******************************************************************************/

DEFINE_DBG_ASSERT (struct kvt_ref_list_builder, kvt_ref_list_builder, s, {
  ASSERT (s);
  ASSERT (s->klen <= 10);
  ASSERT (s->tlen <= 10);
})

struct kvt_ref_list_builder
kvrlb_create (struct builder *b)
{
  return (struct kvt_ref_list_builder){
      .head = NULL,
      .klen = 0,
      .tlen = 0,
      .b    = b,
  };
}

static bool
kvrlb_has_key_been_used (const struct kvt_ref_list_builder *ub, struct string key)
{
  DBG_ASSERT (kvt_ref_list_builder, ub);
  for (struct llnode *it = ub->head; it != NULL; it = it->next) {
    struct kv_ref_llnode *kn = container_of (it, struct kv_ref_llnode, link);
    if (string_equal (kn->key, key)) {
      return true;
    }
  }
  return false;
}

err_t
kvrlb_accept_key (struct kvt_ref_list_builder *ub, struct string key, error *e)
{
  DBG_ASSERT (kvt_ref_list_builder, ub);

  if (kvrlb_has_key_been_used (ub, key)) {
    return error_causef (e, ERR_INTERP, "duplicate key: %.*s", (int)key.len, key.data);
  }

  // Copy key data to persistent memory
  key.data = arena_alloc_copy (ub->b->persistent, key.data, key.len, e);
  if (key.data == NULL) {
    return error_trace (e);
  }

  // Keys and types fill the same list from the front, so a type accepted
  // first may already have made the node for this key
  struct llnode        *slot = llnode_get_n (ub->head, ub->klen);
  struct kv_ref_llnode *node;
  if (slot != NULL) {
    node = container_of (slot, struct kv_ref_llnode, link);
  } else {
    node = builder_malloc_temp (ub->b, 1, sizeof *node, e);
    if (node == NULL) {
      return error_trace (e);
    }
    llnode_init (&node->link);
    node->value = (struct type_ref){0};

    if (ub->head == NULL) {
      ub->head = &node->link;
    } else {
      list_append (&ub->head, &node->link);
    }
  }

  node->key = key;
  ub->klen++;

  return SUCCESS;
}

err_t
kvrlb_accept_type (struct kvt_ref_list_builder *ub, struct type_ref t, error *e)
{
  DBG_ASSERT (kvt_ref_list_builder, ub);

  struct llnode        *slot = llnode_get_n (ub->head, ub->tlen);
  struct kv_ref_llnode *node;
  if (slot != NULL) {
    node = container_of (slot, struct kv_ref_llnode, link);
  } else {
    node = builder_malloc_temp (ub->b, 1, sizeof *node, e);
    if (node == NULL) {
      return error_trace (e);
    }
    llnode_init (&node->link);
    node->key = (struct string){0};
    if (ub->head == NULL) {
      ub->head = &node->link;
    } else {
      list_append (&ub->head, &node->link);
    }
  }

  node->value = t;
  ub->tlen++;
  return SUCCESS;
}

err_t
kvrlb_build (struct kvt_ref_list *dest, struct kvt_ref_list_builder *ub, error *e)
{
  DBG_ASSERT (kvt_ref_list_builder, ub);
  ASSERT (dest);

  if (ub->klen == 0) {
    error_causef (e, ERR_INTERP, "no keys");
    goto theend;
  }
  if (ub->klen != ub->tlen) {
    error_causef (e, ERR_INTERP, "key/value count mismatch");
    goto theend;
  }

  struct string *keys = builder_malloc_persist (ub->b, ub->klen, sizeof *keys, e);
  if (keys == NULL) {
    goto theend;
  }

  struct type_ref *types = builder_malloc_persist (ub->b, ub->tlen, sizeof *types, e);

  if (types == NULL) {
    goto theend;
  }

  size_t i = 0;
  for (struct llnode *it = ub->head; it != NULL; it = it->next) {
    struct kv_ref_llnode *kn = container_of (it, struct kv_ref_llnode, link);
    keys[i]                  = kn->key;
    types[i]                 = kn->value;
    i++;
  }

  dest->len   = ub->klen;
  dest->keys  = keys;
  dest->types = types;

theend:
  return error_trace (e);
}

#ifndef NDEBUG
TEST (kvt_ref_list_builder)
{
  struct type_ref x = tr_take (strfcstr ("x"), ta_take ());
  struct type_ref y = tr_take (strfcstr ("y"), ta_take ());

  TEST_CASE ("types accepted before their keys")
  {
    BUILDER_INIT (b, &ctx->alloc);
    struct kvt_ref_list_builder kb = kvrlb_create (&b);

    test_assert_int_equal (kvrlb_accept_type (&kb, x, &ctx->e), SUCCESS);
    test_assert_int_equal (kvrlb_accept_type (&kb, y, &ctx->e), SUCCESS);
    test_assert_int_equal (kvrlb_accept_key (&kb, strfcstr ("a"), &ctx->e), SUCCESS);
    test_assert_int_equal (kvrlb_accept_key (&kb, strfcstr ("b"), &ctx->e), SUCCESS);

    struct kvt_ref_list list;
    test_assert_int_equal (kvrlb_build (&list, &kb, &ctx->e), SUCCESS);
    test_assert_int_equal (list.len, 2);
    test_assert (string_equal (list.keys[1], strfcstr ("b")));
    test_assert (type_ref_equal (list.types[1], y));

    BUILDER_CLOSE (b);
  }

  TEST_CASE ("duplicate key")
  {
    BUILDER_INIT (b, &ctx->alloc);
    struct kvt_ref_list_builder kb = kvrlb_create (&b);
    test_assert_int_equal (kvrlb_accept_key (&kb, strfcstr ("a"), &ctx->e), SUCCESS);
    test_err_t_check (kvrlb_accept_key (&kb, strfcstr ("a"), &ctx->e), ERR_INTERP, &ctx->e);
    BUILDER_CLOSE (b);
  }

  TEST_CASE ("no keys")
  {
    BUILDER_INIT (b, &ctx->alloc);
    struct kvt_ref_list_builder kb = kvrlb_create (&b);
    struct kvt_ref_list         list;
    test_err_t_check (kvrlb_build (&list, &kb, &ctx->e), ERR_INTERP, &ctx->e);
    BUILDER_CLOSE (b);
  }

  TEST_CASE ("key/value count mismatch")
  {
    BUILDER_INIT (b, &ctx->alloc);
    struct kvt_ref_list_builder kb = kvrlb_create (&b);
    test_assert_int_equal (kvrlb_accept_key (&kb, strfcstr ("a"), &ctx->e), SUCCESS);
    struct kvt_ref_list list;
    test_err_t_check (kvrlb_build (&list, &kb, &ctx->e), ERR_INTERP, &ctx->e);
    BUILDER_CLOSE (b);
  }

  TEST_CASE ("key copy out of memory")
  {
    struct arena_alloc persist;
    type_test_arena_create (&persist, ctx->faulty_mem);
    BUILDER_INIT (b, &persist);
    struct kvt_ref_list_builder kb = kvrlb_create (&b);

    faulty_mem_fail_after (ctx->faulty_mem, 0);
    test_err_t_check (kvrlb_accept_key (&kb, strfcstr ("a"), &ctx->e), ERR_NOMEM, &ctx->e);
    faulty_mem_reset (ctx->faulty_mem);

    BUILDER_CLOSE (b);
    arena_alloc_free_all (&persist);
  }

  // Each of the two persistent arrays kvrlb_build makes can fail
  int   fail_after = 0;
  err_t ret;
  do {
    TEST_CASE ("build. Fail after: %d", fail_after)
    {
      error_reset (&ctx->e);

      struct arena_alloc persist;
      type_test_arena_create (&persist, ctx->faulty_mem);
      BUILDER_INIT (b, &persist);
      struct kvt_ref_list_builder kb = kvrlb_create (&b);
      test_assert_int_equal (kvrlb_accept_key (&kb, strfcstr ("a"), &ctx->e), SUCCESS);
      test_assert_int_equal (kvrlb_accept_type (&kb, x, &ctx->e), SUCCESS);

      struct kvt_ref_list list;
      faulty_mem_fail_after (ctx->faulty_mem, fail_after);
      ret = kvrlb_build (&list, &kb, &ctx->e);
      faulty_mem_reset (ctx->faulty_mem);

      BUILDER_CLOSE (b);
      arena_alloc_free_all (&persist);
      fail_after++;
    }
  }
  while (ret != SUCCESS);

  arena_alloc_free_all (&ctx->alloc);
}
#endif
