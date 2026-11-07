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

#include "core/ns_ext_array.h"

#include "core/ns_bounds.h"
#include "core/ns_csx_assert.h"
#include "core/ns_data_validator.h"
#include "core/ns_data_writer.h"
#include "core/ns_error.h"
#include "core/ns_logging.h"
#include "core/ns_stride.h"
#include "core/ns_testing.h"
#include "core/ns_utils.h"
#include "core/os/ns_malloc.h"

#include <string.h>

DEFINE_DBG_ASSERT (struct ext_array, ext_array, r, {
  ASSERT (r);
  ASSERT (r->mem.table);
  ASSERT (r->len <= r->cap);
  ASSERT (r->cap == 0 || r->data != NULL);
})

struct ext_array
ext_array_create (struct i_mem mem)
{
  return (struct ext_array){
      .mem  = mem,
      .data = NULL,
      .len  = 0,
      .cap  = 0,
  };
}

void
ext_array_free (struct ext_array *r)
{
  DBG_ASSERT (ext_array, r);

  if (r->data != NULL) {
    r->mem.table->free (r->mem.self, r->data);
  }
  r->data = NULL;
  r->len  = 0;
  r->cap  = 0;
}

static err_t
ext_array_reserve (struct ext_array *r, const u32 cap, error *e)
{
  DBG_ASSERT (ext_array, r);

  if (cap > r->cap) {
    u32 newcap;
    if (!safe_mul_u32 (&newcap, cap, 2)) {
      return error_causef (e, ERR_ARITH, "capacity %" PRIu32 " overflows when doubled", cap);
    }
    u8 *data = r->mem.table->realloc (r->mem.self, r->data, newcap, 1, e);
    if (data == NULL) {
      return error_trace (e);
    }
    r->data = data;
    r->cap  = newcap;
  }
  return SUCCESS;
}

i64
ext_array_insert (struct ext_array *r, const u32 ofst, const void *src, const u32 slen, error *e)
{
  DBG_ASSERT (ext_array, r);
  ASSERT (ofst <= r->len);

  u32 newlen;
  if (!safe_add_u32 (&newlen, r->len, slen)) {
    return error_causef (e, ERR_ARITH, "insert of %" PRIu32 " bytes overflows length", slen);
  }

  if (ext_array_reserve (r, newlen, e) < 0) {
    return error_trace (e);
  }

  const u32 tlen = r->len - ofst;
  if (tlen > 0) {
    u8 *tail = r->mem.table->malloc (r->mem.self, tlen, 1, e);
    if (tail == NULL) {
      return error_trace (e);
    }
    memcpy (tail, r->data + ofst, tlen);
    memcpy (r->data + ofst, src, slen);
    memcpy (r->data + ofst + slen, tail, tlen);
    r->mem.table->free (r->mem.self, tail);
  } else {
    memcpy (r->data + ofst, src, slen);
  }

  r->len = newlen;
  return slen;
}

u64
ext_array_read (const struct ext_array *r, const struct stride str, const u32 size, void *_dest)
{
  DBG_ASSERT (ext_array, r);

  u8 *dest       = _dest;
  u64 total_read = 0;
  u64 bidx       = str.start * size;

  while (total_read < str.nelems) {
    if (bidx + size > r->len) {
      return total_read;
    }
    memcpy (dest, r->data + bidx, size);
    dest += size;
    bidx += str.stride * size;
    total_read++;
  }

  return total_read;
}

u64
ext_array_write (
    const struct ext_array *r,
    const struct stride     str,
    const u32               size,
    const void             *_src
)
{
  DBG_ASSERT (ext_array, r);

  const u8 *src           = _src;
  u64       total_written = 0;
  u64       bidx          = str.start * size;

  while (total_written < str.nelems) {
    if (bidx + size > r->len) {
      return total_written;
    }
    memcpy (r->data + bidx, src, size);
    src += size;
    bidx += str.stride * size;
    total_written++;
  }

  return total_written;
}

u64
ext_array_remove (struct ext_array *r, const struct stride str, const u32 size, void *_dest)
{
  DBG_ASSERT (ext_array, r);

  u8 *dest          = _dest;
  u64 total_removed = 0;
  u32 wpos          = 0;
  u32 rpos          = 0;
  u64 next_remove   = str.start;

  while (rpos * size < r->len) {
    if (total_removed < str.nelems && rpos == next_remove) {
      if (dest != NULL) {
        memcpy (dest, r->data + ((size_t)rpos * size), size);
        dest += size;
      }
      total_removed++;
      next_remove += str.stride;
    } else {
      if (wpos != rpos) {
        memmove (r->data + ((size_t)wpos * size), r->data + ((size_t)rpos * size), size);
      }
      wpos++;
    }
    rpos++;
  }

  r->len = wpos * size;
  return total_removed;
}

u64
ext_array_get_len (const struct ext_array *r)
{
  DBG_ASSERT (ext_array, r);
  return r->len;
}

static i64
ext_array_insert_func (void *ctx, const u32 ofst, const void *src, const u32 slen, error *e)
{
  struct ext_array *arr = ctx;
  return ext_array_insert (arr, ofst, src, slen, e);
}

static i64
ext_array_read_func (void *ctx, const struct stride str, const u32 size, void *dest, error *e)
{
  (void)e;
  struct ext_array *arr = ctx;
  // Counts are bounded by the u32 length, so they fit in i64
  return (i64)ext_array_read (arr, str, size, dest);
}

static i64
ext_array_write_func (void *ctx, const struct stride str, const u32 size, const void *src, error *e)
{
  (void)e;
  struct ext_array *arr = ctx;
  return (i64)ext_array_write (arr, str, size, src);
}

static i64
ext_array_remove_func (void *ctx, const struct stride str, const u32 size, void *dest, error *e)
{
  (void)e;
  struct ext_array *arr = ctx;
  return (i64)ext_array_remove (arr, str, size, dest);
}

static i64
ext_array_getlen_func (void *ctx, error *e)
{
  (void)e;
  struct ext_array *arr = ctx;
  return (i64)ext_array_get_len (arr);
}

static const struct data_writer_functions ext_array_funcs = {
    .insert = ext_array_insert_func,
    .read   = ext_array_read_func,
    .write  = ext_array_write_func,
    .remove = ext_array_remove_func,
    .getlen = ext_array_getlen_func,
};

void
ext_array_data_writer (struct data_writer *dest, struct ext_array *arr)
{
  ASSERT (dest);
  DBG_ASSERT (ext_array, arr);

  dest->functions = ext_array_funcs;
  dest->ctx       = arr;
}

#ifndef NDEBUG
TEST (ext_array_insert_read)
{
  TEST_CASE ("basic sequential")
  {
    struct ext_array a     = ext_array_create (ctx->mem);

    const u32        src[] = {1, 2, 3, 4, 5};
    ext_array_insert (&a, 0, src, sizeof (src), &ctx->e);

    u32       dest[5] = {0};
    const u64 n       = ext_array_read (
        &a,
        (struct stride){
            .start  = 0,
            .stride = 1,
            .nelems = 5,
        },
        sizeof (u32),
        dest
    );

    test_assert_type_equal (n, (u64)5, u64, PRIu64);
    test_assert_memequal (src, dest, sizeof (src));
    ext_array_free (&a);
  }

  TEST_CASE ("insert at end (append)")
  {
    struct ext_array a        = ext_array_create (ctx->mem);

    const u32        first[]  = {1, 2, 3};
    const u32        second[] = {4, 5, 6};
    ext_array_insert (&a, 0, first, sizeof (first), &ctx->e);
    ext_array_insert (&a, sizeof (first), second, sizeof (second), &ctx->e);

    u32 dest[6] = {0};
    ext_array_read (
        &a,
        (struct stride){
            .start  = 0,
            .stride = 1,
            .nelems = 6,
        },
        sizeof (u32),
        dest
    );

    const u32 expected[] = {1, 2, 3, 4, 5, 6};
    test_assert_memequal (expected, dest, sizeof (expected));
    ext_array_free (&a);
  }

  TEST_CASE ("insert in middle")
  {
    struct ext_array a          = ext_array_create (ctx->mem);

    const u32        initial[]  = {1, 3, 4};
    const u32        inserted[] = {2};
    ext_array_insert (&a, 0, initial, sizeof (initial), &ctx->e);
    ext_array_insert (&a, sizeof (u32), inserted, sizeof (inserted), &ctx->e);

    u32 dest[4] = {0};
    ext_array_read (
        &a,
        (struct stride){
            .start  = 0,
            .stride = 1,
            .nelems = 4,
        },
        sizeof (u32),
        dest
    );

    const u32 expected[] = {1, 2, 3, 4};
    test_assert_memequal (expected, dest, sizeof (expected));
    ext_array_free (&a);
  }

  TEST_CASE ("read strided")
  {
    struct ext_array a     = ext_array_create (ctx->mem);

    const u32        src[] = {1, 2, 3, 4, 5, 6};
    ext_array_insert (&a, 0, src, sizeof (src), &ctx->e);

    u32       dest[3] = {0};
    const u64 n       = ext_array_read (
        &a,
        (struct stride){
            .start  = 0,
            .stride = 2,
            .nelems = 3,
        },
        sizeof (u32),
        dest
    );

    test_assert_type_equal (n, (u64)3, u64, PRIu64);
    const u32 expected[] = {1, 3, 5};
    test_assert_memequal (expected, dest, sizeof (expected));
    ext_array_free (&a);
  }

  TEST_CASE ("read past end")
  {
    struct ext_array a     = ext_array_create (ctx->mem);

    const u32        src[] = {10, 20, 30};
    ext_array_insert (&a, 0, src, sizeof (src), &ctx->e);

    u32       dest[10] = {0};
    const u64 n        = ext_array_read (
        &a,
        (struct stride){
            .start  = 1,
            .stride = 1,
            .nelems = 10,
        },
        sizeof (u32),
        dest
    );

    test_assert_type_equal (n, (u64)2, u64, PRIu64);
    const u32 expected[] = {20, 30};
    test_assert_memequal (expected, dest, 2);
    ext_array_free (&a);
  }
}

TEST (ext_array_write)
{
  TEST_CASE ("overwrite single middle")
  {
    struct ext_array a     = ext_array_create (ctx->mem);

    const u32        src[] = {1, 2, 3, 4, 5};
    ext_array_insert (&a, 0, src, sizeof (src), &ctx->e);

    const u32 patch[] = {99};
    const u64 n       = ext_array_write (
        &a,
        (struct stride){
            .start  = 2,
            .stride = 1,
            .nelems = 1,
        },
        sizeof (u32),
        patch
    );

    test_assert_type_equal (n, (u64)1, u64, PRIu64);

    u32 dest[5] = {0};
    ext_array_read (&a, (struct stride){.start = 0, .stride = 1, .nelems = 5}, sizeof (u32), dest);

    const u32 expected[] = {1, 2, 99, 4, 5};
    test_assert_memequal (expected, dest, sizeof (expected));
    ext_array_free (&a);
  }

  TEST_CASE ("overwrite strided")
  {
    struct ext_array a     = ext_array_create (ctx->mem);

    const u32        src[] = {1, 2, 3, 4, 5, 6};
    ext_array_insert (&a, 0, src, sizeof (src), &ctx->e);

    const u32 patch[] = {0, 0, 0};
    const u64 n       = ext_array_write (
        &a,
        (struct stride){
            .start  = 0,
            .stride = 2,
            .nelems = 3,
        },
        sizeof (u32),
        patch
    );

    test_assert_type_equal (n, (u64)3, u64, PRIu64);

    u32 dest[6] = {0};
    ext_array_read (
        &a,
        (struct stride){
            .start  = 0,
            .stride = 1,
            .nelems = 6,
        },
        sizeof (u32),
        dest
    );

    const u32 expected[] = {0, 2, 0, 4, 0, 6};
    test_assert_memequal (expected, dest, sizeof (expected));
    ext_array_free (&a);
  }

  TEST_CASE ("write past end returns short count")
  {
    struct ext_array a     = ext_array_create (ctx->mem);

    const u32        src[] = {1, 2, 3};
    ext_array_insert (&a, 0, src, sizeof (src), &ctx->e);

    const u32 patch[] = {9, 9, 9};
    const u64 n       = ext_array_write (
        &a,
        (struct stride){
            .start  = 2,
            .stride = 1,
            .nelems = 3,
        },
        sizeof (u32),
        patch
    );

    test_assert_type_equal (n, (u64)1, u64, PRIu64);
    ext_array_free (&a);
  }
}

TEST (ext_array_remove)
{
  TEST_CASE ("remove from middle")
  {
    struct ext_array a     = ext_array_create (ctx->mem);

    const u32        src[] = {1, 2, 3, 4, 5};
    ext_array_insert (&a, 0, src, sizeof (src), &ctx->e);

    u32       removed = 0;
    const u64 n       = ext_array_remove (
        &a,
        (struct stride){
            .start  = 2,
            .stride = 1,
            .nelems = 1,
        },
        sizeof (u32),
        &removed
    );

    test_assert_type_equal (n, (u64)1, u64, PRIu64);
    u32 expected_removed = 3;
    test_assert_memequal (&expected_removed, &removed, 1);

    u32 dest[4] = {0};
    ext_array_read (
        &a,
        (struct stride){
            .start  = 0,
            .stride = 1,
            .nelems = 4,
        },
        sizeof (u32),
        dest
    );

    const u32 expected[] = {1, 2, 4, 5};
    test_assert_memequal (expected, dest, sizeof (expected));
    ext_array_free (&a);
  }

  TEST_CASE ("remove first")
  {
    struct ext_array a     = ext_array_create (ctx->mem);

    const u32        src[] = {10, 20, 30};
    ext_array_insert (&a, 0, src, sizeof (src), &ctx->e);

    u32 removed = 0;
    ext_array_remove (
        &a,
        (struct stride){
            .start  = 0,
            .stride = 1,
            .nelems = 1,
        },
        sizeof (u32),
        &removed
    );

    u32 dest[2] = {0};
    ext_array_read (
        &a,
        (struct stride){
            .start  = 0,
            .stride = 1,
            .nelems = 2,
        },
        sizeof (u32),
        dest
    );

    const u32 expected[] = {20, 30};
    test_assert_memequal (expected, dest, sizeof (expected));
    ext_array_free (&a);
  }

  TEST_CASE ("remove last")
  {
    struct ext_array a     = ext_array_create (ctx->mem);

    const u32        src[] = {10, 20, 30};
    ext_array_insert (&a, 0, src, sizeof (src), &ctx->e);

    u32 removed = 0;
    ext_array_remove (
        &a,
        (struct stride){
            .start  = 2,
            .stride = 1,
            .nelems = 1,
        },
        sizeof (u32),
        &removed
    );

    u32 dest[2] = {0};
    ext_array_read (
        &a,
        (struct stride){
            .start  = 0,
            .stride = 1,
            .nelems = 2,
        },
        sizeof (u32),
        dest
    );

    const u32 expected[] = {10, 20};
    test_assert_memequal (expected, dest, sizeof (expected));
    ext_array_free (&a);
  }

  TEST_CASE ("remove strided")
  {
    struct ext_array a     = ext_array_create (ctx->mem);

    const u32        src[] = {1, 2, 3, 4, 5, 6};
    ext_array_insert (&a, 0, src, sizeof (src), &ctx->e);

    u32       removed[3] = {0};
    const u64 n          = ext_array_remove (
        &a,
        (struct stride){
            .start  = 0,
            .stride = 2,
            .nelems = 3,
        },
        sizeof (u32),
        removed
    );

    test_assert_type_equal (n, (u64)3, u64, PRIu64);
    const u32 expected_removed[] = {1, 3, 5};
    test_assert_memequal (expected_removed, removed, sizeof (expected_removed));

    u32 dest[3] = {0};
    ext_array_read (
        &a,
        (struct stride){
            .start  = 0,
            .stride = 1,
            .nelems = 3,
        },
        sizeof (u32),
        dest
    );

    const u32 expected[] = {2, 4, 6};
    test_assert_memequal (expected, dest, sizeof (expected));
    ext_array_free (&a);
  }
}

TEST (ext_array_random)
{
  /**
  struct ext_array ext_arr_1 = ext_array_create ();
  struct ext_array ext_arr_2 = ext_array_create ();

  struct data_writer ref;
  struct data_writer sut;

  ext_array_data_writer (&ref, &ext_arr_1);
  ext_array_data_writer (&sut, &ext_arr_2);

  struct dvalidtr d = {
  .sut = sut,
  .ref = ref,
  };

  dvalidtr_random_test (&d, 1, 1000, 10000, &e);

  ext_array_free (&ext_arr_1);

  ext_array_free (&ext_arr_2);
  */

  // Block sizes to test
  const u32 niters[] = {100, 100, 100, 100, 100, 100, 1000, 1000, 1000, 1000};

  for (u32 i = 0; i < arrlen (niters); ++i) {
    i_log_info ("Block random test: %" PRIu32 "\n", i);

    struct ext_array   ext_arr_1 = ext_array_create (ctx->mem);
    struct ext_array   ext_arr_2 = ext_array_create (ctx->mem);

    struct data_writer ref;
    struct data_writer sut;

    ext_array_data_writer (&ref, &ext_arr_1);
    ext_array_data_writer (&sut, &ext_arr_2);

    struct dvalidtr d = {
        .mem     = ctx->mem,
        .sut     = sut,
        .ref     = ref,
        .isvalid = NULL,
    };

    dvalidtr_random_test (&d, 1, niters[i], 1000, &ctx->e);

    ext_array_free (&ext_arr_1);
    ext_array_free (&ext_arr_2);
  }
}

#endif

err_t
ext_array_clone (struct ext_array *dest, struct ext_array *r, error *e)
{
  DBG_ASSERT (ext_array, r);

  *dest         = ext_array_create (r->mem);
  const u32 len = r->len;

  if (len > 0) {
    if (ext_array_reserve (dest, len, e) < 0) {
      return error_trace (e);
    }

    i64 inserted = ext_array_insert (dest, 0, r->data, len, e);
    (void)inserted;
    ASSERT (inserted == (i64)len);
  }

  return SUCCESS;
}

#ifndef NDEBUG
TEST (ext_array_capacity_doubles_on_growth)
{
  struct ext_array a    = ext_array_create (ctx->mem);

  // Seed with one byte to force the first allocation (cap → 2).
  const u8         seed = 0x01;
  ext_array_insert (&a, 0, &seed, 1, &ctx->e);
  test_assert_int_equal (ctx->e.cause_code, SUCCESS);
  test_assert (a.cap >= 2U);

  // Fill to capacity.
  const u32 orig_cap = a.cap;
  for (u32 i = a.len; i < orig_cap; ++i) {
    u8 b = (u8)(i & 0xFF);
    ext_array_insert (&a, a.len, &b, 1, &ctx->e);
    test_assert_int_equal (ctx->e.cause_code, SUCCESS);
  }
  test_assert_type_equal (a.len, orig_cap, u32, PRIu32);

  // One more byte must trigger realloc; new cap must be >= 2×old.
  const u8 extra = 0xFF;
  ext_array_insert (&a, a.len, &extra, 1, &ctx->e);
  test_assert_int_equal (ctx->e.cause_code, SUCCESS);
  test_assert (a.cap >= orig_cap * 2U);

  ext_array_free (&a);
}

// Removing every element via stride must leave an empty array.
TEST (ext_array_remove_all_produces_empty)
{
  struct ext_array a      = ext_array_create (ctx->mem);

  const u8         src[5] = {10, 20, 30, 40, 50};
  ext_array_insert (&a, 0, src, sizeof src, &ctx->e);
  test_assert_int_equal (ctx->e.cause_code, SUCCESS);

  u8 out[5] = {0};
  const u64
      n = ext_array_remove (&a, (struct stride){.start = 0, .stride = 1, .nelems = 5}, 1, out);

  test_assert_type_equal (n, (u64)5, u64, PRIu64);
  test_assert_type_equal (ext_array_get_len (&a), (u64)0, u64, PRIu64);
  test_assert_memequal (src, out, 5);

  ext_array_free (&a);
}

TEST (ext_array_insert_fail_nomem)
{
  const u8 head[4] = {1, 2, 3, 4};
  const u8 mid[3]  = {9, 9, 9};
  i64      ret;
  int      fail_after = 0;

  do {
    TEST_CASE ("Fail after: %d", fail_after)
    {
      error_reset (&ctx->e);
      struct ext_array a = ext_array_create (ctx->faulty_mem);
      test_assert_type_equal (ext_array_insert (&a, 0, head, 4, &ctx->e), (i64)4, i64, PRId64);

      // Insert in the middle so both the grow and the tail copy allocate
      faulty_mem_fail_after (ctx->faulty_mem, fail_after);
      ret = ext_array_insert (&a, 2, mid, 3, &ctx->e);
      faulty_mem_reset (ctx->faulty_mem);

      if (ret < 0) {
        // A failed insert leaves the contents alone
        test_assert_type_equal (ext_array_get_len (&a), (u64)4, u64, PRIu64);
        test_assert_memequal (a.data, head, 4);
      } else {
        const u8 expected[7] = {1, 2, 9, 9, 9, 3, 4};
        test_assert_type_equal (ext_array_get_len (&a), (u64)7, u64, PRIu64);
        test_assert_memequal (a.data, expected, 7);
      }

      ext_array_free (&a);
      fail_after++;
    }
  }
  while (ret < 0);
}

TEST (ext_array_clone_fail_nomem)
{
  const u8         src[5] = {1, 2, 3, 4, 5};
  err_t            ret;
  int              fail_after = 0;

  struct ext_array a          = ext_array_create (ctx->faulty_mem);
  ext_array_insert (&a, 0, src, 5, &ctx->e);
  test_assert_int_equal (ctx->e.cause_code, SUCCESS);

  do {
    TEST_CASE ("Fail after: %d", fail_after)
    {
      error_reset (&ctx->e);
      struct ext_array b;

      faulty_mem_fail_after (ctx->faulty_mem, fail_after);
      ret = ext_array_clone (&b, &a, &ctx->e);
      faulty_mem_reset (ctx->faulty_mem);

      if (ret == SUCCESS) {
        test_assert_type_equal (ext_array_get_len (&b), (u64)5, u64, PRIu64);
        test_assert_memequal (b.data, src, 5);
      }

      ext_array_free (&b);
      fail_after++;
    }
  }
  while (ret != SUCCESS);

  ext_array_free (&a);
}
#endif
