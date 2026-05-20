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

#include "c_specx.h"
#include "nscore/errors.h"
#include "nscore/type_accessor.h"
#include "nscore/types.h"

bool user_stride_equal (const struct user_stride *left, const struct user_stride *right) {
  return left->start == right->start && left->step == right->step && left->stop == right->stop
         && left->present == right->present;
}

DEFINE_DBG_ASSERT (struct range_builder, range_builder, s, { ASSERT (s); })

void rb_create (
    struct range_builder *dest,
    struct chunk_alloc   *temp,
    struct chunk_alloc   *persistent) {
  *dest = (struct range_builder){
      .head       = NULL,
      .len        = 0,
      .temp       = temp,
      .persistent = persistent,
  };
  DBG_ASSERT (range_builder, dest);
}

err_t rb_accept_stride (struct range_builder *rb, struct user_stride stride, error *e) {
  DBG_ASSERT (range_builder, rb);

  struct rb_llnode *node = chunk_malloc (rb->temp, 1, sizeof *node, e);
  if (!node) { return error_trace (e); }

  llnode_init (&node->link);
  node->stride = stride;

  if (!rb->head) {
    rb->head = &node->link;
  } else {
    list_append (&rb->head, &node->link);
  }

  rb->len++;
  return SUCCESS;
}

err_t rb_build (struct range_ta *dest, struct range_builder *rb, error *e) {
  DBG_ASSERT (range_builder, rb);

  if (rb->len == 0) { return error_causef (e, ERR_INTERP, "range: no dimensions"); }

  struct user_stride *dims = chunk_malloc (rb->persistent, rb->len, sizeof *dims, e);
  if (!dims) { return error_trace (e); }

  u32 i = 0;
  for (struct llnode *it = rb->head; it; it = it->next) {
    struct rb_llnode *rn = container_of (it, struct rb_llnode, link);
    dims[i]              = rn->stride;
    i++;
  }

  dest->dim_accessors = dims;
  dest->dlen          = rb->len;

  return SUCCESS;
}

DEFINE_DBG_ASSERT (struct type_accessor_builder, type_accessor_builder, s, { ASSERT (s); })

static struct type_accessor *tab_alloc (struct type_accessor_builder *builder, error *e) {
  if (builder->head == NULL) { return &builder->ret; }

  struct type_accessor *ta = chunk_malloc (builder->persistent, 1, sizeof *ta, e);
  return ta;
}

static void tab_link (struct type_accessor_builder *builder, struct type_accessor *ta) {
  if (!builder->head) {
    builder->head = ta;
  } else {
    if (builder->tail->type == TA_SELECT) {
      builder->tail->select.sub_ta = ta;
    } else if (builder->tail->type == TA_RANGE) {
      builder->tail->range.sub_ta = ta;
    }
  }
  builder->tail = ta;
}

static err_t tab_flush_range (struct type_accessor_builder *builder, error *e) {
  if (!builder->in_range) { return SUCCESS; }

  struct type_accessor *ta = tab_alloc (builder, e);
  if (!ta) { return error_trace (e); }

  ta->type         = TA_RANGE;
  ta->range.sub_ta = NULL;

  WRAP (rb_build (&ta->range, &builder->rb, e));

  tab_link (builder, ta);
  builder->in_range = false;

  return SUCCESS;
}

static void tab_ensure_range (struct type_accessor_builder *builder) {
  if (!builder->in_range) {
    rb_create (&builder->rb, builder->temp, builder->persistent);
    builder->in_range = true;
  }
}

void tab_create (
    struct type_accessor_builder *dest,
    struct chunk_alloc           *temp,
    struct chunk_alloc           *persistent) {
  *dest = (struct type_accessor_builder){
      .head       = NULL,
      .tail       = NULL,
      .temp       = temp,
      .persistent = persistent,
      .in_range   = false,
  };

  DBG_ASSERT (type_accessor_builder, dest);
}

err_t tab_accept_select (struct type_accessor_builder *builder, struct string key, error *e) {
  DBG_ASSERT (type_accessor_builder, builder);

  WRAP (tab_flush_range (builder, e));

  struct type_accessor *ta = tab_alloc (builder, e);
  if (!ta) { return error_trace (e); }

  key.data = chunk_alloc_move_mem (builder->persistent, key.data, key.len, e);
  if (!key.data) { return error_trace (e); }

  ta->type          = TA_SELECT;
  ta->select.key    = key;
  ta->select.sub_ta = NULL;

  tab_link (builder, ta);

  return SUCCESS;
}

err_t tab_accept_stride (
    struct type_accessor_builder *builder,
    struct user_stride            stride,
    error                        *e) {
  DBG_ASSERT (type_accessor_builder, builder);
  tab_ensure_range (builder);
  return rb_accept_stride (&builder->rb, stride, e);
}

err_t tab_accept_take (struct type_accessor_builder *builder, error *e) {
  DBG_ASSERT (type_accessor_builder, builder);

  WRAP (tab_flush_range (builder, e));

  struct type_accessor *ta = tab_alloc (builder, e);
  if (!ta) { return error_trace (e); }

  ta->type = TA_TAKE;

  tab_link (builder, ta);

  return SUCCESS;
}

err_t tab_build (struct type_accessor *dest, struct type_accessor_builder *builder, error *e) {
  DBG_ASSERT (type_accessor_builder, builder);

  WRAP (tab_accept_take (builder, e));

  *dest = builder->ret;
  return SUCCESS;
}

#ifndef NTEST
TEST (type_accessor_builder) {
  error e = error_create ();

  struct chunk_alloc arena;
  chunk_alloc_create_default (&arena);

  // 0. freshly-created builder must be clean
  struct type_accessor_builder builder;
  tab_create (&builder, &arena, &arena);
  test_fail_if (builder.head != NULL);
  test_fail_if (builder.tail != NULL);

  struct type_accessor acc;
  tab_build (&acc, &builder, &e);
  test_assert_int_equal (acc.type, TA_TAKE);

  tab_create (&builder, &arena, &arena);

  // 2. accept a select accessor
  struct string key1 = strfcstr ("field1");
  test_assert_int_equal (tab_accept_select (&builder, key1, &e), SUCCESS);
  // 3. accept a stride + single (enters range mode)
  test_assert_int_equal (tab_accept_stride (&builder, ustride012 (0, 10, 2), &e), SUCCESS);
  test_assert (builder.in_range);
  test_assert_int_equal (tab_accept_stride (&builder, ustride_single (5), &e), SUCCESS);
  test_assert_int_equal (builder.rb.len, 2);

  // 4. accept another select accessor (should flush the range)
  struct string key2 = strfcstr ("field2");
  test_assert_int_equal (tab_accept_select (&builder, key2, &e), SUCCESS);
  test_fail_if (builder.in_range);

  // 5. successful build
  test_assert_int_equal (tab_build (&acc, &builder, &e), SUCCESS);

  // 6. verify chain: SELECT(field1) → RANGE([0:10:2, 5]) →
  // SELECT(field2) → TAKE
  test_assert_int_equal (acc.type, TA_SELECT);
  test_assert_int_equal (string_equal (acc.select.key, key1), true);
  struct type_accessor *range_acc = acc.select.sub_ta;
  test_assert_int_equal (range_acc->type, TA_RANGE);
  test_assert_int_equal (range_acc->range.dlen, 2);

  test_assert_int_equal (range_acc->range.dim_accessors[0].start, 0);
  test_assert_int_equal (range_acc->range.dim_accessors[0].stop, 2);
  test_assert_int_equal (range_acc->range.dim_accessors[0].step, 10);

  test_assert_int_equal (range_acc->range.dim_accessors[1].start, 5);
  test_assert_int_equal (range_acc->range.dim_accessors[1].stop, 0);
  test_assert_int_equal (range_acc->range.dim_accessors[1].step, 0);

  struct type_accessor *select_acc = range_acc->range.sub_ta;
  test_assert_int_equal (select_acc->type, TA_SELECT);
  test_assert_int_equal (string_equal (select_acc->select.key, key2), true);
  test_assert_int_equal (select_acc->select.sub_ta->type, TA_TAKE);

  chunk_alloc_free_all (&arena);
}
#endif
