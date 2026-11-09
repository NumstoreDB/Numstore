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

#include "core/ns_stdtypes.h"
#include "nscore/page/ns_page.h"
#include "nscore/page/ns_page_data_list.h"
#include "nscore/page/ns_page_fixture.h"
#include "nscore/page/ns_page_h.h"
#include "nscore/pager/ns_pager.h"

#include <stdatomic.h>

#ifndef NDEBUG
#  include "core/ns_testing.h"
#endif

#ifndef NDEBUG

#  define NITERS   700
#  define NTHREADS 10

struct thread_ctx
{
  _Atomic int  *iter;
  struct pager *p;
  struct txn   *tx;
  _Atomic int  *ready;
  _Atomic int  *go;
  bool          success;
  err_t         ret;
};

static void *
simple_pager_ops (void *_ctx)
{
  struct thread_ctx *ctx = _ctx;
  struct pager      *p   = ctx->p;
  error              e   = error_create ();
  page_h             a   = page_h_create ();

  // Create some random data
  decl_rand_buffer (abytes, u8, DL_DATA_SIZE);

  // Wait for the parent thread to signal
  atomic_fetch_sub (ctx->ready, 1);
  while (atomic_load (ctx->go) == 0) {
    spin_pause ();
  }

  for (int i = 0; i < NITERS; ++i) {
    // Create a new page
    pgr_new (&a, p, ctx->tx, PG_DATA_LIST, &e);

    int iter = atomic_fetch_add (ctx->iter, 1);
    if (iter % 50 == 0) {
      i_log_info ("Thread iter: %d / %d\n", iter, NITERS * NTHREADS);
    }

    pgno apg = page_h_pgno (&a);
    dl_memset (page_h_w (&a), abytes, DL_DATA_SIZE);
    pgr_release (p, &a, PG_DATA_LIST, &e);

    // Get the same page and check it's the same
    pgr_get (&a, PG_DATA_LIST, apg, p, &e);
    ctx->success = memcmp (dl_get_data (page_h_ro (&a)), abytes, DL_DATA_SIZE) == 0;
    pgr_release (p, &a, PG_DATA_LIST, &e);
  }

  ctx->ret = e.cause_code;

  return NULL;
}

TEST (0004_pgr_new_get)
{
  struct pgr_fixture pf;
  pgr_fixture_create (&pf, &ctx->e);

  pgr_begin_txn (&pf.tx, pf.p, &ctx->e);

  struct thread_ctx tctx[10];
  i_thread          threads[arrlen (tctx)];
  _Atomic int       iter  = 0;
  _Atomic int       ready = arrlen (tctx);
  _Atomic int       go    = 0;

  for (int i = 0; i < arrlen (tctx); ++i) {
    tctx[i] = (struct thread_ctx){
        .iter    = &iter,
        .p       = pf.p,
        .tx      = &pf.tx,
        .ready   = &ready,
        .go      = &go,
        .success = false
    };
  }

  // Create all threads
  for (u32 i = 0; i < arrlen (threads); ++i) {
    ctx->os.table->thread_create (ctx->os.self, &threads[i], simple_pager_ops, &tctx[i], &ctx->e);
  }

  // Wait for children to be ready
  while (atomic_load (&ready) > 0) {
    spin_pause ();
  }

  // Launch threads
  atomic_store (&go, 1);

  // Join them all
  for (u32 i = 0; i < arrlen (threads); ++i) {
    ctx->os.table->thread_join (ctx->os.self, threads[i].self);
  }

  pgr_commit (pf.p, &pf.tx, &ctx->e);

  // Validate results
  for (u32 i = 0; i < arrlen (threads); ++i) {
    test_assert (tctx[i].success);
    test_assert_int_equal (tctx[i].ret, SUCCESS);
  }

  pgr_fixture_teardown (&pf, &ctx->e);
}
#endif
