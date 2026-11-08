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

#include "core/ns_dbl_buffer.h"

#include "core/ns_arena_alloc.h"
#include "core/ns_bounds.h"
#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_testing.h"
#include "core/ns_utils.h"
#include "core/os/ns_malloc.h"

#include <string.h>

DEFINE_DBG_ASSERT (struct dbl_buffer, dbl_buffer, d, {
  ASSERT (d);
  ASSERT (d->data);
  ASSERT (d->nelem <= d->nelem_cap);
})

err_t
dblb_create (
    struct dbl_buffer  *dest,
    struct arena_alloc *alloc,
    const u32           size,
    const u32           initial_cap,
    error              *e
)
{
  ASSERT (dest);
  ASSERT (alloc);
  ASSERT (initial_cap > 0);
  ASSERT (size > 0);

  void *data = arena_malloc (alloc, initial_cap, size, e);
  if (data == NULL) {
    return error_trace (e);
  }

  *dest = (struct dbl_buffer){
      .data      = data,
      .size      = size,
      .nelem_cap = initial_cap,
      .nelem     = 0,
      .alloc     = alloc,
  };

  DBG_ASSERT (dbl_buffer, dest);

  return SUCCESS;
}

#ifndef NDEBUG
TEST (dblb_create)
{
  TEST_CASE ("basic")
  {
    struct dbl_buffer db;

    // Create buffer with size=4, initial_cap=2
    const err_t       err = dblb_create (&db, &ctx->alloc, 4, 2, &ctx->e);
    test_assert_int_equal (err, SUCCESS);
    test_assert_type_equal (db.size, 4, u32, PRIu32);
    test_assert_type_equal (db.nelem, 0, u32, PRIu32);
    test_assert_type_equal (db.nelem_cap, 2, u32, PRIu32);
    test_assert (db.data != NULL);

    arena_alloc_free_all (&ctx->alloc);
  }

  TEST_CASE ("malloc fails")
  {
    arena_alloc_free_all (&ctx->faulty_alloc);
    faulty_mem_reset (ctx->faulty_mem);

    struct dbl_buffer  buffer;

    /**
     * Testing that the data isn't overrided on fail
     */
    int                i     = 1;
    struct arena_alloc dummy = {0};
    buffer.size              = 10;
    buffer.nelem             = 11;
    buffer.nelem_cap         = 12;
    buffer.data              = &i;
    buffer.alloc             = &dummy;

    // fail the main malloc
    faulty_mem_fail_after (ctx->faulty_mem, 0);
    test_err_t_check (
        dblb_create (&buffer, &ctx->faulty_alloc, 10, 100, &ctx->e),
        ERR_NOMEM,
        &ctx->e
    );

    test_assert_type_equal (buffer.size, 10, u32, PRIu32);
    test_assert_type_equal (buffer.nelem, 11, u32, PRIu32);
    test_assert_type_equal (buffer.nelem_cap, 12, u32, PRIu32);
    test_assert_ptr_equal (buffer.data, &i);
    test_assert_ptr_equal (buffer.alloc, &dummy);
  }
}
#endif

err_t
dblb_append (struct dbl_buffer *d, const void *data, const u32 nelem, error *e)
{
  DBG_ASSERT (dbl_buffer, d);
  ASSERT (data);

  void *head = dblb_append_alloc (d, nelem, e);
  if (head == NULL) {
    return error_trace (e);
  }

  memcpy (head, data, (size_t)d->size * nelem);

  return SUCCESS;
}

#ifndef NDEBUG
TEST (dblb_append)
{
  TEST_CASE ("basic")
  {
    arena_alloc_free_all (&ctx->faulty_alloc);
    faulty_mem_reset (ctx->faulty_mem);

    struct dbl_buffer db;

    // Create the buffer
    dblb_create (&db, &ctx->faulty_alloc, 4, 2, &ctx->e);
    test_assert_int_equal (faulty_num_mallocs (ctx->faulty_mem), 1);

    // No extra realloc
    u32 data1[] = {1};
    dblb_append (&db, data1, arrlen (data1), &ctx->e);
    test_assert_int_equal (faulty_num_mallocs (ctx->faulty_mem), 1);
    test_assert_type_equal (db.nelem, 1, u32, PRIu32);
    test_assert_int_equal (1, ((u32 *)db.data)[0]);

    // Executes a realloc
    u32 data2[] = {2, 3, 4, 5};
    dblb_append (&db, data2, arrlen (data2), &ctx->e);
    test_assert_int_equal (faulty_num_mallocs (ctx->faulty_mem), 2);
    test_assert_type_equal (db.nelem, 5, u32, PRIu32);
    test_assert_int_equal (1, ((u32 *)db.data)[0]);
    test_assert_int_equal (2, ((u32 *)db.data)[1]);
    test_assert_int_equal (3, ((u32 *)db.data)[2]);
    test_assert_int_equal (4, ((u32 *)db.data)[3]);
    test_assert_int_equal (5, ((u32 *)db.data)[4]);
  }

  TEST_CASE ("malloc fails")
  {
    arena_alloc_free_all (&ctx->faulty_alloc);
    faulty_mem_reset (ctx->faulty_mem);

    struct dbl_buffer db;

    // Create the buffer
    dblb_create (&db, &ctx->faulty_alloc, 4, 2, &ctx->e);
    test_assert_int_equal (faulty_num_mallocs (ctx->faulty_mem), 1);

    // No extra realloc
    u32 data1[] = {1};
    dblb_append (&db, data1, arrlen (data1), &ctx->e);
    test_assert_int_equal (faulty_num_mallocs (ctx->faulty_mem), 1);
    test_assert_type_equal (db.nelem, 1, u32, PRIu32);
    test_assert_int_equal (1, ((u32 *)db.data)[0]);

    // Executes a realloc
    u32 data2[] = {2, 3, 4, 5};
    faulty_mem_fail_after (ctx->faulty_mem, 0);
    test_err_t_check (dblb_append (&db, data2, arrlen (data2), &ctx->e), ERR_NOMEM, &ctx->e);
    test_assert_type_equal (db.nelem, 1, u32, PRIu32);
    test_assert_int_equal (1, ((u32 *)db.data)[0]);
  }
}
#endif

err_t
dblb_ensure_space (struct dbl_buffer *d, const u32 nelem, error *e)
{
  DBG_ASSERT (dbl_buffer, d);

  if (nelem >= d->nelem_cap) {
    u32 old_bytes;
    u32 newcap;
    if (!safe_mul_u32 (&old_bytes, d->size, d->nelem_cap) || !safe_mul_u32 (&newcap, 2, nelem)) {
      return error_causef (e, ERR_ARITH, "growing to %" PRIu32 " elements overflows", nelem);
    }

    // TODO: this asks for old_bytes * newcap bytes, not newcap * size, so the
    // buffer grows quadratically. It should be arena_malloc (d->alloc, newcap,
    // d->size, e).
    void *newdata = arena_malloc (d->alloc, old_bytes, newcap, e);
    if (newdata == NULL) {
      return error_trace (e);
    }
    memcpy (newdata, d->data, (size_t)d->size * d->nelem);
    d->data      = newdata;
    d->nelem_cap = newcap;
  }
  // TODO: on success this returns whatever [e] already held rather than
  // SUCCESS
  return error_trace (e);
}

void *
dblb_append_alloc (struct dbl_buffer *d, const u32 nelem, error *e)
{
  DBG_ASSERT (dbl_buffer, d);

  u32 newnelem_cap;
  if (!safe_add_u32 (&newnelem_cap, d->nelem, nelem)) {
    error_causef (e, ERR_ARITH, "appending %" PRIu32 " elements overflows", nelem);
    return NULL;
  }

  if (dblb_ensure_space (d, newnelem_cap, e) < 0) {
    return NULL;
  }

  void *ret = (u8 *)d->data + ((size_t)d->nelem * d->size);
  d->nelem += nelem;

  return ret;
}

void
dblb_reset (struct dbl_buffer *d)
{
  DBG_ASSERT (dbl_buffer, d);

  d->nelem_cap = 0;
  d->nelem     = 0;
}

#ifndef NDEBUG

TEST (dblb_append_single)
{
  struct dbl_buffer db;

  dblb_create (&db, &ctx->alloc, sizeof (u32), 4, &ctx->e);

  // Append single element
  u32         val = 0x12345678;
  const err_t err = dblb_append (&db, &val, 1, &ctx->e);
  test_assert_int_equal (err, SUCCESS);
  test_assert_type_equal (db.nelem, 1, u32, PRIu32);
  test_assert_type_equal (db.nelem_cap, 4, u32, PRIu32);

  // Verify data
  const u32 *data = (u32 *)db.data;
  test_assert_int_equal (data[0], 0x12345678);

  arena_alloc_free_all (&ctx->alloc);
}

TEST (dblb_append_multiple)
{
  struct dbl_buffer db;

  dblb_create (&db, &ctx->alloc, sizeof (u32), 2, &ctx->e);

  // Append multiple elements
  u32         vals[] = {0x11, 0x22, 0x33};
  const err_t err    = dblb_append (&db, vals, 3, &ctx->e);
  test_assert_int_equal (err, SUCCESS);
  test_assert_type_equal (db.nelem, 3, u32, PRIu32);

  // Verify data
  const u32 *data = (u32 *)db.data;
  test_assert_int_equal (data[0], 0x11);
  test_assert_int_equal (data[1], 0x22);
  test_assert_int_equal (data[2], 0x33);

  arena_alloc_free_all (&ctx->alloc);
}

TEST (dblb_append_triggers_realloc)
{
  struct dbl_buffer db;

  // Start with capacity of 2
  dblb_create (&db, &ctx->alloc, sizeof (u32), 2, &ctx->e);

  u32 val1 = 0xAA;
  dblb_append (&db, &val1, 1, &ctx->e);
  test_assert_type_equal (db.nelem, 1, u32, PRIu32);
  test_assert_type_equal (db.nelem_cap, 2, u32, PRIu32);

  u32 val2 = 0xBB;
  dblb_append (&db, &val2, 1, &ctx->e);
  test_assert_type_equal (db.nelem, 2, u32, PRIu32);
  test_assert_type_equal (db.nelem_cap, 4, u32, PRIu32);

  u32 val3 = 0xCC;
  dblb_append (&db, &val3, 1, &ctx->e);
  test_assert_type_equal (db.nelem, 3, u32, PRIu32);
  test_assert_type_equal (db.nelem_cap, 4, u32, PRIu32);

  u32 val4 = 0xDD;
  dblb_append (&db, &val4, 1, &ctx->e);
  test_assert_type_equal (db.nelem, 4, u32, PRIu32);
  test_assert_type_equal (db.nelem_cap, 8, u32, PRIu32);

  const u32 *data = (u32 *)db.data;
  test_assert_int_equal (data[0], 0xAA);
  test_assert_int_equal (data[1], 0xBB);
  test_assert_int_equal (data[2], 0xCC);
  test_assert_int_equal (data[3], 0xDD);

  arena_alloc_free_all (&ctx->alloc);
}

TEST (dblb_append_alloc_basic)
{
  struct dbl_buffer db;

  dblb_create (&db, &ctx->alloc, sizeof (u32), 4, &ctx->e);

  // Allocate space for 2 elements
  void *ptr = dblb_append_alloc (&db, 2, &ctx->e);
  test_assert (ptr != NULL);
  test_assert_type_equal (db.nelem, 2, u32, PRIu32);

  // Write directly to allocated space
  u32 *data       = (u32 *)ptr;
  data[0]         = 0x1111;
  data[1]         = 0x2222;

  // Verify
  const u32 *base = (u32 *)db.data;
  test_assert_int_equal (base[0], 0x1111);
  test_assert_int_equal (base[1], 0x2222);

  arena_alloc_free_all (&ctx->alloc);
}

TEST (dblb_append_alloc_sequential)
{
  struct dbl_buffer db;

  dblb_create (&db, &ctx->alloc, sizeof (u32), 8, &ctx->e);

  // First allocation
  u32 *ptr1 = (u32 *)dblb_append_alloc (&db, 2, &ctx->e);
  test_assert (ptr1 != NULL);
  ptr1[0]   = 0xAA;
  ptr1[1]   = 0xBB;

  // Second allocation - should be right after first
  u32 *ptr2 = (u32 *)dblb_append_alloc (&db, 2, &ctx->e);
  test_assert (ptr2 != NULL);
  test_assert_type_equal ((i64)((u8 *)ptr2 - (u8 *)ptr1), (i64)(2 * sizeof (u32)), i64, PRId64);
  ptr2[0]         = 0xCC;
  ptr2[1]         = 0xDD;

  // Verify all data
  const u32 *base = (u32 *)db.data;
  test_assert_int_equal (base[0], 0xAA);
  test_assert_int_equal (base[1], 0xBB);
  test_assert_int_equal (base[2], 0xCC);
  test_assert_int_equal (base[3], 0xDD);
  test_assert_type_equal (db.nelem, 4, u32, PRIu32);

  arena_alloc_free_all (&ctx->alloc);
}

TEST (dblb_append_alloc_triggers_realloc)
{
  struct dbl_buffer db;

  dblb_create (&db, &ctx->alloc, sizeof (u32), 2, &ctx->e);

  // Fill to capacity
  dblb_append_alloc (&db, 2, &ctx->e);
  test_assert_type_equal (db.nelem_cap, 4, u32, PRIu32);

  // This should trigger realloc
  const void *ptr = dblb_append_alloc (&db, 1, &ctx->e);
  test_assert (ptr != NULL);
  test_assert_type_equal (db.nelem, 3, u32, PRIu32);
  test_assert_type_equal (db.nelem_cap, 4, u32, PRIu32);

  arena_alloc_free_all (&ctx->alloc);
}

TEST (dblb_append_alloc_fail_nomem)
{
  struct dbl_buffer db;
  void             *ret;
  int               fail_after = 0;

  do {
    TEST_CASE ("Fail after: %d", fail_after)
    {
      // Setup - two elements in a buffer of capacity 4, so appending three grows it
      error_reset (&ctx->e);
      arena_alloc_free_all (&ctx->faulty_alloc);
      faulty_mem_reset (ctx->faulty_mem);

      test_fail_if (dblb_create (&db, &ctx->faulty_alloc, sizeof (u32), 2, &ctx->e) < 0);
      u32 *head = dblb_append_alloc (&db, 2, &ctx->e);
      test_assert (head != NULL);
      head[0] = 1;
      head[1] = 2;

      // Run
      faulty_mem_fail_after (ctx->faulty_mem, fail_after);
      ret = dblb_append_alloc (&db, 3, &ctx->e);
      faulty_mem_reset (ctx->faulty_mem);

      // A failed append leaves the contents alone; a successful one keeps them
      const u32 expected_nelem = ret == NULL ? 2 : 5;
      test_assert_int_equal (db.nelem, expected_nelem);
      test_assert_int_equal (((u32 *)db.data)[0], 1);
      test_assert_int_equal (((u32 *)db.data)[1], 2);

      fail_after++;
    }
  }
  while (ret == NULL);

  // Teardown
  arena_alloc_free_all (&ctx->faulty_alloc);
}

TEST (dblb_different_element_sizes)
{
  struct dbl_buffer db;

  // Test with u8
  dblb_create (&db, &ctx->alloc, sizeof (u8), 4, &ctx->e);
  u8 byte = 0x42;
  dblb_append (&db, &byte, 1, &ctx->e);
  test_assert_int_equal (((u8 *)db.data)[0], 0x42);

  // Test with u64
  dblb_create (&db, &ctx->alloc, sizeof (u64), 4, &ctx->e);
  u64 large = 0x123456789ABCDEF0;
  dblb_append (&db, &large, 1, &ctx->e);
  test_assert_equal (((u64 *)db.data)[0], 0x123456789ABCDEF0);
  arena_alloc_free_all (&ctx->alloc);
}

TEST (dblb_struct_elements)
{
  struct dbl_buffer db;

  struct test_struct
  {
    u32 id;
    u32 value;
  };

  dblb_create (&db, &ctx->alloc, sizeof (struct test_struct), 2, &ctx->e);

  struct test_struct s1 = {.id = 1, .value = 100};
  struct test_struct s2 = {.id = 2, .value = 200};

  dblb_append (&db, &s1, 1, &ctx->e);
  dblb_append (&db, &s2, 1, &ctx->e);

  const struct test_struct *data = (struct test_struct *)db.data;
  test_assert_int_equal (data[0].id, 1);
  test_assert_int_equal (data[0].value, 100);
  test_assert_int_equal (data[1].id, 2);
  test_assert_int_equal (data[1].value, 200);

  arena_alloc_free_all (&ctx->alloc);
}

TEST (dblb_free_resets)
{
  struct dbl_buffer db;

  dblb_create (&db, &ctx->alloc, sizeof (u32), 4, &ctx->e);
  u32 val = 0x1234;
  dblb_append (&db, &val, 1, &ctx->e);

  dblb_reset (&db);

  // Verify fields are reset
  test_assert_type_equal (db.nelem, 0, u32, PRIu32);
  arena_alloc_free_all (&ctx->alloc);
}

TEST (dblb_large_append)
{
  struct dbl_buffer db;

  dblb_create (&db, &ctx->alloc, sizeof (u32), 2, &ctx->e);

  // Append many elements at once
  u32 vals[100];
  for (u32 i = 0; i < 100; i++) {
    vals[i] = i;
  }

  dblb_append (&db, vals, 100, &ctx->e);
  test_assert_type_equal (db.nelem, 100, u32, PRIu32);

  // Verify data
  const u32 *data = (u32 *)db.data;
  for (u32 i = 0; i < 100; i++) {
    test_assert_int_equal (data[i], i);
  }

  arena_alloc_free_all (&ctx->alloc);
}
#endif
