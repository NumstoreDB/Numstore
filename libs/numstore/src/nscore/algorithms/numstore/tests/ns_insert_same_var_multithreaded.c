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

#include "core/ns_arena_alloc.h"
#include "core/ns_error.h"
#include "core/ns_numerics.h"
#include "core/ns_stream.h"
#include "core/ns_stride.h"
#include "core/ns_testing.h"
#include "nscore/algorithms/numstore/ns_numstore_algorithms.h"
#include "nscore/page/ns_page_fixture.h"
#include "nscore/pager/ns_pager.h"
#include "nscore/types/ns_types.h"

#include <stdatomic.h>

#ifndef NDEBUG

#  define NITERS           10
#  define NTHREADS         10
#  define SIZE_EACH_INSERT 10000

struct insert_mt_ctx
{
  struct pgr_fixture *p;
  _Atomic int        *ready;
  _Atomic int        *go;
  _Atomic int        *idx;
  u8                 *data[NTHREADS];
};

static void *
insert_mt_func (void *_ctx)
{
  struct insert_mt_ctx *ctx = _ctx;
  int                   i   = atomic_fetch_add (ctx->idx, 1);
  error                 e   = error_create ();
  struct txn            tx;

  ALLOC_INIT (alloc);
  istream_create_from (stream, ctx->data[i], SIZE_EACH_INSERT);

  atomic_fetch_sub (ctx->ready, 1);
  while (atomic_load (ctx->go) == 0) {
  }

  pgr_begin_txn (&tx, ctx->p->p, &e);
  numstore_insert_from_name (
      ctx->p->p,
      &tx,
      strfcstr ("foo"),
      0,
      SIZE_EACH_INSERT,
      &alloc,
      NULL,
      &stream,
      &e
  );
  pgr_commit (ctx->p->p, &tx, &e);

  ALLOC_CLOSE (alloc);
  return NULL;
}

TEST (numstore_insert_multithreaded)
{
  for (int k = 0; k < NITERS; ++k) {
    struct pgr_fixture f;
    pgr_fixture_create (&f, &ctx->e);
    numstore_init_pager (f.p, &ctx->e);

    pgr_begin_txn (&f.tx, f.p, &ctx->e);
    numstore_create (f.p, &f.tx, strfcstr ("foo"), TU8, &ctx->alloc, NULL, &ctx->e);
    pgr_commit (f.p, &f.tx, &ctx->e);

    _Atomic int          ready = NTHREADS, go = 0, idx = 0;
    struct insert_mt_ctx targs = {.p = &f, .ready = &ready, .go = &go, .idx = &idx};

    for (int i = 0; i < NTHREADS; ++i) {
      targs.data[i] = f.mem.table->malloc (f.mem.self, 1, SIZE_EACH_INSERT, &ctx->e);
      rand_bytes (targs.data[i], SIZE_EACH_INSERT);
    }

    i_thread workers[NTHREADS];
    for (int i = 0; i < NTHREADS; ++i) {
      f.os.table->thread_create (f.os.self, &workers[i], insert_mt_func, &targs, &ctx->e);
    }

    while (atomic_load (&ready) > 0) {
    }
    atomic_store (&go, 1);

    for (int i = 0; i < NTHREADS; ++i) {
      f.os.table->thread_join (f.os.self, workers[i].self);
    }

    // Fresh txn for the read (f.tx was already committed)
    struct txn rtx;
    b_size     dlen;
    void      *_a;
    pgr_begin_txn (&rtx, f.p, &ctx->e);
    numstore_read_malloc_from_name (
        f.p,
        &rtx,
        strfcstr ("foo"),
        ustride0 (0),
        &ctx->alloc,
        NULL,
        &_a,
        &dlen,
        f.mem,
        &ctx->e
    );
    pgr_commit (f.p, &rtx, &ctx->e);
    u8 *actual = _a;

    test_assert_int_equal (dlen, NTHREADS * SIZE_EACH_INSERT);

    // Any serial order is valid; each chunk must appear intact exactly once
    bool used[NTHREADS] = {0};
    for (int slot = 0; slot < NTHREADS; ++slot) {
      int match = -1;
      for (int i = 0; i < NTHREADS; ++i) {
        if (!used[i]
            && memcmp (actual + slot * SIZE_EACH_INSERT, targs.data[i], SIZE_EACH_INSERT) == 0) {
          match = i;
          break;
        }
      }
      test_assert (match >= 0);
      used[match] = true;
    }

    f.mem.table->free (f.mem.self, actual);
    for (int i = 0; i < NTHREADS; ++i) {
      f.mem.table->free (f.mem.self, targs.data[i]);
    }
    arena_alloc_free_all (&ctx->alloc);
    pgr_fixture_teardown (&f, &ctx->e);
  }
}

#endif
