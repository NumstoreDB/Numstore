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

#include "dpg_table/ns_dirty_page_table.h"
#include "lock_table/ns_lock_table.h"
#include "ns_error.h"
#include "ns_testing.h"
#include "pager/ns_pager.h"
#include "txn_table/ns_txn_table.h"

err_t
aries_ctx_create (struct aries_ctx *dest, struct i_mem mem, error *e)
{
  dest->max_tid = 0;
  slab_alloc_init (&dest->alloc, mem, sizeof (struct txn), 1000);
  arena_alloc_create_default (&dest->backing_alloc);

  dest->txt = txnt_open (mem, e);
  if (dest->txt == NULL) {
    goto failed;
  }

  dest->dpt = dpgt_open (mem, e);
  if (dest->dpt == NULL) {
    goto txt_failed;
  }

  if (dblb_create (&dest->txn_ptrs, &dest->backing_alloc, sizeof (struct txn *), 100, e) < 0) {
    goto dpt_failed;
  }

  return SUCCESS;

dpt_failed:
  dpgt_close (dest->dpt);
txt_failed:
  txnt_close (dest->txt);
failed:
  slab_alloc_destroy (&dest->alloc);

  return error_trace (e);
}

void
aries_ctx_free (struct aries_ctx *ctx)
{
  ASSERT (ctx);
  slab_alloc_destroy (&ctx->alloc);
  txnt_close (ctx->txt);
  dpgt_close (ctx->dpt);
  arena_alloc_free_all (&ctx->backing_alloc);
}

struct txn *
aries_ctx_txn_alloc (struct aries_ctx *ctx, error *e)
{
  struct txn *tx = slab_alloc_alloc (&ctx->alloc, e);
  if (tx == NULL) {
    return NULL;
  }

  if (dblb_append (&ctx->txn_ptrs, (const void *)&tx, 1, e) < 0) {
    slab_alloc_free (&ctx->alloc, tx);
    return NULL;
  }

  return tx;
}

#ifndef NDEBUG
TEST (aries_ctx_create_fail_mem)
{
  int   fail_after = 0;
  err_t ret;

  do {
    TEST_CASE ("Fail after: %d", fail_after)
    {
      error_reset (&ctx->e);

      // Run
      struct aries_ctx actx;
      faulty_mem_fail_after (ctx->faulty_mem, fail_after);
      ret = aries_ctx_create (&actx, ctx->faulty_mem, &ctx->e);
      faulty_mem_reset (ctx->faulty_mem);

      // Teardown - a failed create cleans up after itself
      if (ret == SUCCESS) {
        aries_ctx_free (&actx);
      } else {
        test_assert_int_equal (ctx->e.cause_code, ERR_NOMEM);
        error_reset (&ctx->e);
      }

      fail_after++;
    }
  }
  while (ret != SUCCESS);
}

TEST (aries_ctx_txn_alloc_fail_mem)
{
  // Setup
  struct aries_ctx actx;
  test_assert_int_equal (aries_ctx_create (&actx, ctx->faulty_mem, &ctx->e), SUCCESS);

  int         fail_after = 0;
  struct txn *tx;

  do {
    TEST_CASE ("Fail after: %d", fail_after)
    {
      error_reset (&ctx->e);

      // Run
      faulty_mem_fail_after (ctx->faulty_mem, fail_after);
      tx = aries_ctx_txn_alloc (&actx, &ctx->e);
      faulty_mem_reset (ctx->faulty_mem);

      if (tx == NULL) {
        test_assert_int_equal (ctx->e.cause_code, ERR_NOMEM);
        error_reset (&ctx->e);
      }

      fail_after++;
    }
  }
  while (tx == NULL);

  // Teardown
  aries_ctx_free (&actx);
}
#endif
