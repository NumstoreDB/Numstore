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

#include "ns_bounds.h"
#include "ns_error.h"
#include "ns_testing.h"
#include "numstore/ns_numstore_algorithms.h"
#include "pager/ns_page_fixture.h"
#include "rope/ns_rope_algorithms.h"
#include "var/ns_var_algorithms.h"

err_t
numstore_remove_malloc (
    struct pager      *p,
    struct txn        *tx,
    struct variable   *var,
    struct user_stride ustr,
    void             **dest,
    b_size            *dlen,
    struct i_mem       mem,
    error             *e
)
{
  DBG_ASSERT (pager, p);
  DBG_ASSERT (ns_txn, tx);
  DBG_ASSERT (variable, var);
  DBG_ASSERT (user_stride, &ustr);
  ASSERT (dest);
  ASSERT (dlen);
  DBG_ASSERT (i_mem, &mem);
  DBG_ASSERT (clean_error, e);

  // Resolve sizes
  t_size tsize = type_byte_size (var->dtype);
  ASSERT (var->nbytes % tsize == 0); // Variable is valid (from assert)
  b_size        len = var->nbytes / tsize;

  // Resolve length based on the stride
  struct stride stride;
  if (stride_resolve (&stride, ustr, len, e) < 0) {
    return error_trace (e);
  }

  // The step started as a positive i64, so it fits in sb_size
  ASSERT (stride.stride <= (u64)I64_MAX);

  if (stride.nelems == 0) {
    *dlen = 0;
    *dest = NULL;
    return SUCCESS;
  }

  // The allocator and the buffer stream both take u32 sizes
  u32 nbytes;
  if (stride.nelems > U32_MAX || !safe_mul_u32 (&nbytes, (u32)stride.nelems, tsize)) {
    return error_causef (
        e,
        ERR_ARITH,
        "%" PRIu64 " elements of %" PRt_size " bytes overflow one buffer",
        stride.nelems,
        tsize
    );
  }

  void *buffer = mem.table->malloc (mem.self, (u32)stride.nelems, tsize, e);
  if (buffer == NULL) {
    return error_trace (e);
  }

  // Create output stream
  ostream_create_from (stream, buffer, nbytes);

  // REMOVE
  struct ns_remove_params rparams = {
      .p      = p,
      .dest   = &stream,
      .tx     = tx,
      .root   = var->rpt_root,
      .size   = tsize,
      .bofst  = tsize * stride.start,
      .stride = (sb_size)stride.stride,
      .nelem  = stride.nelems,
  };
  sb_size ret = ns_remove (&rparams, e);
  if (ret < 0) {
    mem.table->free (mem.self, buffer);
    return error_trace (e);
  }

  // Update new sizes and root. ns_remove returns the number of elements it
  // took out, so the variable shrinks by that much - `len` is the length we
  // started with, not the amount removed.
  ASSERT ((b_size)ret * tsize <= var->nbytes);
  b_size newsize = var->nbytes - ((b_size)ret * tsize);
  pgno   newroot = rparams.root;
  if (ns_var_update_by_var_root (p, tx, var->var_root, newroot, newsize, e) < 0) {
    // TODO: buffer leaks on this path. Free it like the ns_remove failure
    // above does.
    return error_trace (e);
  }

  // Update the user supplied variable
  var->nbytes   = newsize;
  var->rpt_root = rparams.root;

  // Set return values
  *dlen         = (b_size)ret * tsize;
  *dest         = buffer;

  return SUCCESS;
}

#ifndef NDEBUG
TEST (numstore_remove_malloc_fail_mem)
{
  int   fail_after = 0;
  err_t ret;

  do {
    TEST_CASE ("Fail after: %d", fail_after)
    {
      error_reset (&ctx->e);

      // Setup - a variable with 10 elements, on a healthy allocator
      struct pgr_fixture f;
      pgr_fixture_create (&f, &ctx->e);
      numstore_init_pager (f.p, &ctx->e);
      pgr_begin_txn (&f.tx, f.p, &ctx->e);

      struct variable var;
      numstore_create (f.p, &f.tx, strfcstr ("var"), TU32, &ctx->alloc, &var, &ctx->e);
      u32 data[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      istream_create_from_array (src, data);
      numstore_insert_from_name (
          f.p,
          &f.tx,
          strfcstr ("var"),
          0,
          10,
          &ctx->alloc,
          &var,
          &src,
          &ctx->e
      );

      // Run - only the output buffer comes from the faulty allocator
      void  *buf  = NULL;
      b_size dlen = 0;
      faulty_mem_fail_after (ctx->faulty_mem, fail_after);
      ret = numstore_remove_malloc (
          f.p,
          &f.tx,
          &var,
          USER_STRIDE_ALL,
          &buf,
          &dlen,
          ctx->faulty_mem,
          &ctx->e
      );
      faulty_mem_reset (ctx->faulty_mem);

      if (ret == SUCCESS) {
        test_assert_type_equal (dlen, (b_size)(10 * sizeof (u32)), b_size, PRb_size);
        ctx->faulty_mem.table->free (ctx->faulty_mem.self, buf);
      } else {
        // A failed allocation leaves the variable alone
        test_assert_int_equal (ctx->e.cause_code, ERR_NOMEM);
        test_assert_type_equal (var.nbytes, (b_size)(10 * sizeof (u32)), b_size, PRb_size);
        error_reset (&ctx->e);
      }

      // Teardown
      pgr_commit (f.p, &f.tx, &ctx->e);
      pgr_fixture_teardown (&f, &ctx->e);
      arena_alloc_free_all (&ctx->alloc);

      fail_after++;
    }
  }
  while (ret != SUCCESS);
}
#endif
