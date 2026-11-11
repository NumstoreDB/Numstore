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

#include "ns_page_h.h"
#include "ns_stdtypes.h"
#include "pager/ns_page_fixture.h"
#include "pager/ns_pager.h"

#ifndef NDEBUG
#  include "ns_testing.h"
#endif

/**
 * This is almost a 1:1 COPY of 0001_pager_is...
 * Feel free to make this better - it just
 * begins a txn within each individual thread
 * instead of on the parent thread.
 */

#ifndef NDEBUG

struct thread_ctx
{
  struct wal   *w;
  struct pager *p;
  _Atomic int  *ready;
  _Atomic int  *go;
  pgno          a;
  pgno          b;
  pgno          c;
  pgno          d;
  bool          success;
  err_t         ret;
};

static void *
simple_pager_ops (void *_ctx)
{
  struct thread_ctx *ctx = _ctx;
  struct pager      *p   = ctx->p;
  struct txn         tx;

  error              e = error_create ();

  page_h             a = page_h_create ();
  page_h             b = page_h_create ();
  page_h             c = page_h_create ();
  page_h             d = page_h_create ();

  // Create some random data
  decl_rand_buffer (abytes, u8, DL_DATA_SIZE);
  decl_rand_buffer (bbytes, u8, DL_DATA_SIZE);
  decl_rand_buffer (cbytes, u8, DL_DATA_SIZE);
  decl_rand_buffer (dbytes, u8, DL_DATA_SIZE);

  // Wait for the parent thread to signal
  atomic_fetch_sub (ctx->ready, 1);

  // We begin transaction after ready and go to
  // assert that transctions aren't globally
  // blocking - e.g. multiple transactions can
  // happen at the same time
  pgr_begin_txn (&tx, p, &e);

  while (atomic_load (ctx->go) == 0) {
    spin_pause ();
  }

  // Create 4 new pages
  pgr_new (&a, p, &tx, PG_DATA_LIST, &e);
  pgr_new (&b, p, &tx, PG_DATA_LIST, &e);
  pgr_new (&c, p, &tx, PG_DATA_LIST, &e);
  pgr_new (&d, p, &tx, PG_DATA_LIST, &e);

  dl_memset (page_h_w (&a), abytes, DL_DATA_SIZE);
  dl_memset (page_h_w (&b), bbytes, DL_DATA_SIZE);
  dl_memset (page_h_w (&c), cbytes, DL_DATA_SIZE);
  dl_memset (page_h_w (&d), dbytes, DL_DATA_SIZE);

  // Get their page numbers
  pgno ap = page_h_pgno (&a);
  pgno bp = page_h_pgno (&b);
  pgno cp = page_h_pgno (&c);
  pgno dp = page_h_pgno (&d);

  i_log_info ("%" PRpgno " %" PRpgno " %" PRpgno " %" PRpgno "\n", ap, bp, cp, dp);

  // Release all of them
  pgr_release (p, &a, PG_DATA_LIST, &e);
  pgr_release (p, &b, PG_DATA_LIST, &e);
  pgr_release (p, &c, PG_DATA_LIST, &e);
  pgr_release (p, &d, PG_DATA_LIST, &e);

  // Get them
  pgr_get (&a, PG_DATA_LIST, ap, p, &e);
  pgr_get (&b, PG_DATA_LIST, bp, p, &e);
  pgr_get (&c, PG_DATA_LIST, cp, p, &e);
  pgr_get (&d, PG_DATA_LIST, dp, p, &e);

  pgr_commit (p, &tx, &e);

  // Check data
  bool avalid  = memcmp (dl_get_data (page_h_ro (&a)), abytes, DL_DATA_SIZE) == 0;
  bool bvalid  = memcmp (dl_get_data (page_h_ro (&b)), bbytes, DL_DATA_SIZE) == 0;
  bool cvalid  = memcmp (dl_get_data (page_h_ro (&c)), cbytes, DL_DATA_SIZE) == 0;
  bool dvalid  = memcmp (dl_get_data (page_h_ro (&d)), dbytes, DL_DATA_SIZE) == 0;
  ctx->success = (bool)(avalid && bvalid && cvalid && dvalid);

  // Release them all
  pgr_release (p, &a, PG_DATA_LIST, &e);
  pgr_release (p, &b, PG_DATA_LIST, &e);
  pgr_release (p, &c, PG_DATA_LIST, &e);
  pgr_release (p, &d, PG_DATA_LIST, &e);

  ctx->ret = e.cause_code;

  ctx->a   = ap;
  ctx->b   = bp;
  ctx->c   = cp;
  ctx->d   = dp;

  return NULL;
}

// Robin hood hash table for buffer pool
#  define KTYPE  pgno
#  define VTYPE  bool
#  define SUFFIX pg
#  include "ns_robin_hood_ht.h"
#  undef KTYPE
#  undef VTYPE
#  undef SUFFIX

TEST (pgr_multi_txns_concurrency)
{
  struct pgr_fixture pf;
  pgr_fixture_create (&pf, &ctx->e);

  struct thread_ctx tctx[50];
  i_thread          threads[arrlen (tctx)];
  _Atomic int       ready = arrlen (tctx);
  _Atomic int       go    = 0;
  for (u32 i = 0; i < arrlen (tctx); ++i) {
    tctx[i] = (struct thread_ctx){.p = pf.p, .ready = &ready, .go = &go, .success = false};
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

  // Validate results
  {
    hash_table_pg unique_set;
    hentry_pg     _hdata[4 * arrlen (threads)];
    ht_init_pg (&unique_set, _hdata, arrlen (_hdata));

    // Check results
    for (u32 i = 0; i < arrlen (threads); ++i) {
      test_assert (tctx[i].success);

      test_assert_int_equal (tctx[i].ret, SUCCESS);
      hdata_pg data = {
          .key   = tctx[i].a,
          .value = false,
      };
      hti_res res = ht_insert_pg (&unique_set, data);
      test_assert_int_equal (res, HTIR_SUCCESS);

      data = (hdata_pg){
          .key   = tctx[i].b,
          .value = false,
      };
      res = ht_insert_pg (&unique_set, data);
      test_assert_int_equal (res, HTIR_SUCCESS);

      data = (hdata_pg){
          .key   = tctx[i].c,
          .value = false,
      };
      res = ht_insert_pg (&unique_set, data);
      test_assert_int_equal (res, HTIR_SUCCESS);

      data = (hdata_pg){
          .key   = tctx[i].d,
          .value = false,
      };
      res = ht_insert_pg (&unique_set, data);
      test_assert_int_equal (res, HTIR_SUCCESS);
    }
  }

  pgr_fixture_teardown (&pf, &ctx->e);
}
#endif
