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
#include "numstore/page_fixture.h"
#include "numstore/pager.h"
#include "numstore/pages/data_list.h"
#include "numstore/txn.h"

#ifndef NTEST

struct thread_ctx {
  struct wal   *w;
  struct pager *p;
  i_semaphore  *begin;
  pgno          a;
  pgno          b;
  pgno          c;
  pgno          d;
  bool          success;
  err_t         ret;
};

static void *simple_pager_ops (void *_ctx) {
  struct thread_ctx *ctx = _ctx;
  struct pager      *p   = ctx->p;
  struct txn         tx;
  error              e = error_create ();

  page_h a = page_h_create ();
  page_h b = page_h_create ();
  page_h c = page_h_create ();
  page_h d = page_h_create ();

  // Create some random data
  decl_rand_buffer (abytes, u8, DL_DATA_SIZE);
  decl_rand_buffer (bbytes, u8, DL_DATA_SIZE);
  decl_rand_buffer (cbytes, u8, DL_DATA_SIZE);
  decl_rand_buffer (dbytes, u8, DL_DATA_SIZE);

  i_semaphore_wait (ctx->begin);

  pgr_begin_txn (&tx, p, &e);

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

  // Check data
  ctx->success = memcmp (dl_get_data (page_h_ro (&a)), abytes, DL_DATA_SIZE) == 0;
  ctx->success = ctx->success && memcmp (dl_get_data (page_h_ro (&b)), bbytes, DL_DATA_SIZE) == 0;
  ctx->success = ctx->success && memcmp (dl_get_data (page_h_ro (&c)), cbytes, DL_DATA_SIZE) == 0;
  ctx->success = ctx->success && memcmp (dl_get_data (page_h_ro (&d)), dbytes, DL_DATA_SIZE) == 0;

  // Release them all
  pgr_release (p, &a, PG_DATA_LIST, &e);
  pgr_release (p, &b, PG_DATA_LIST, &e);
  pgr_release (p, &c, PG_DATA_LIST, &e);
  pgr_release (p, &d, PG_DATA_LIST, &e);

  ctx->ret = e.cause_code;

  ctx->a = ap;
  ctx->b = bp;
  ctx->c = cp;
  ctx->d = dp;

  return NULL;
}

// Robin hood hash table for buffer pool
#  define KTYPE  pgno
#  define VTYPE  bool
#  define SUFFIX pg
#  include "c_specx.h"
#  undef KTYPE
#  undef VTYPE
#  undef SUFFIX

TEST (pager_mt) {
  struct pgr_fixture pf;
  pgr_fixture_create (&pf);

  i_semaphore begin;

  struct thread_ctx ctx[] = {
      {.p = pf.p, .begin = &begin, .success = false},
      {.p = pf.p, .begin = &begin, .success = false},
      {.p = pf.p, .begin = &begin, .success = false},
      {.p = pf.p, .begin = &begin, .success = false},
      {.p = pf.p, .begin = &begin, .success = false},
      {.p = pf.p, .begin = &begin, .success = false},
      {.p = pf.p, .begin = &begin, .success = false},
      {.p = pf.p, .begin = &begin, .success = false},
      {.p = pf.p, .begin = &begin, .success = false},
      {.p = pf.p, .begin = &begin, .success = false},
      {.p = pf.p, .begin = &begin, .success = false},
      {.p = pf.p, .begin = &begin, .success = false},
      {.p = pf.p, .begin = &begin, .success = false},
      {.p = pf.p, .begin = &begin, .success = false},
      {.p = pf.p, .begin = &begin, .success = false},
  };

  i_thread threads[arrlen (ctx)];

  i_semaphore_create (&begin, arrlen (threads), &pf.e);

  // Create all threads
  for (u32 i = 0; i < arrlen (threads); ++i) {
    i_thread_create (&threads[i], simple_pager_ops, &ctx[i], &pf.e);
  }

  // Post semaphore for the number of threads there are
  for (u32 i = 0; i < arrlen (threads); ++i) { i_semaphore_post (&begin); }

  // Join them all
  for (u32 i = 0; i < arrlen (threads); ++i) { i_thread_join (&threads[i], &pf.e); }

  hash_table_pg unique_set;
  hentry_pg     _hdata[4 * arrlen (threads)];
  ht_init_pg (&unique_set, _hdata, arrlen (_hdata));

  // Check results
  for (u32 i = 0; i < arrlen (threads); ++i) {
    test_assert (ctx[i].success);

    test_assert_int_equal (ctx[i].ret, SUCCESS);
    hdata_pg data = {
        .key   = ctx[i].a,
        .value = 0,
    };
    hti_res res = ht_insert_pg (&unique_set, data);
    test_assert_int_equal (res, HTIR_SUCCESS);

    data = (hdata_pg){
        .key   = ctx[i].b,
        .value = 0,
    };
    res = ht_insert_pg (&unique_set, data);
    test_assert_int_equal (res, HTIR_SUCCESS);

    data = (hdata_pg){
        .key   = ctx[i].c,
        .value = 0,
    };
    res = ht_insert_pg (&unique_set, data);
    test_assert_int_equal (res, HTIR_SUCCESS);

    data = (hdata_pg){
        .key   = ctx[i].d,
        .value = 0,
    };
    res = ht_insert_pg (&unique_set, data);
    test_assert_int_equal (res, HTIR_SUCCESS);
  }

  pgr_fixture_teardown (&pf);
}
#endif
