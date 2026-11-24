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

#include "ns_stdtypes.h"
#include "numstore.h"
#include "os/ns_os.h"

#include <stdatomic.h>
#include <string.h>

#ifndef NDEBUG
#  include "ns_testing.h"
#endif

#ifndef NDEBUG

// Several threads, one database handle, one variable per thread
//
// Every thread runs its own transactions against its own variable: a few
// inserts and removes, then a commit or a rollback. It keeps a copy of what
// its variable should hold. At the end the main thread compares.
//
// LOCKING: this doesn't care how transactions are locked. With the global
// database lock the transactions run one after the other. With hierarchical
// locks they overlap, and they still must not disturb each other - no thread
// touches another thread's variable.
//
// The threads call the API with their own transaction. Calls with a NULL
// transaction from several threads are regression 0022.

enum
{
  SV_NTHREADS   = 4,
  SV_NTXNS      = 25,
  SV_MAX_INSERT = 2000,
  SV_MAX_OPS    = 3,

  // The most a variable can ever hold
  SV_CAP        = SV_NTXNS * SV_MAX_OPS * SV_MAX_INSERT,
};

struct sv_worker
{
  nsdb_t      *db;
  u32          id;
  _Atomic u32 *go; // Start signal, so the threads really overlap

  // What the variable holds after the last commit, and how the thread did
  u32         *model;
  u32          len;
  u32         *saved;
  u32         *buf;
  u32          nfailed;
};

static u32
sv_rand (u64 *state, const u32 n)
{
  *state ^= *state << 13;
  *state ^= *state >> 7;
  *state ^= *state << 17;
  return (u32)(*state % n);
}

static void *
sv_worker_run (void *arg)
{
  struct sv_worker *w    = arg;
  u64               rs   = 0x9E3779B97F4A7C15ULL * (w->id + 1);
  u32               next = 1;

  // Everybody starts at once
  while (atomic_load (w->go) == 0) {
  }

  for (u32 t = 0; t < SV_NTXNS; ++t) {
    nsdb_txn_t *tx = ns_begin (w->db);
    if (tx == NULL) {
      w->nfailed++;
      return NULL;
    }

    // Remember the committed state for a rollback
    const u32 saved_len = w->len;
    memcpy (w->saved, w->model, saved_len * sizeof (u32));

    const u32 nops = 1 + sv_rand (&rs, SV_MAX_OPS);
    for (u32 op = 0; op < nops; ++op) {
      // Insert somewhere. The top 4 bits say which thread wrote the element
      const u32 n    = 1 + sv_rand (&rs, SV_MAX_INSERT);
      const u32 ofst = sv_rand (&rs, w->len + 1);
      for (u32 i = 0; i < n; ++i) {
        w->buf[i] = (w->id << 28) | next++;
      }

      if (ns_write (w->db, tx, w->buf, n * sizeof (u32), "insert v%u %u %u", w->id, ofst, n)
          != (sb_size)n) {
        w->nfailed++;
      }
      memmove (&w->model[ofst + n], &w->model[ofst], (w->len - ofst) * sizeof (u32));
      memcpy (&w->model[ofst], w->buf, n * sizeof (u32));
      w->len += n;

      // Sometimes take a piece out again
      if (w->len > 100 && sv_rand (&rs, 3) == 0) {
        const u32 rn = 1 + sv_rand (&rs, w->len / 2);
        const u32 ro = sv_rand (&rs, w->len - rn);

        if (ns_execute (w->db, tx, "remove v%u[%u:%u]", w->id, ro, ro + rn) != (int)rn) {
          w->nfailed++;
        }
        memmove (&w->model[ro], &w->model[ro + rn], (w->len - ro - rn) * sizeof (u32));
        w->len -= rn;
      }
    }

    // One in three transactions rolls back
    if (sv_rand (&rs, 3) == 0) {
      if (ns_rollback (w->db, tx) != 0) {
        w->nfailed++;
      }
      memcpy (w->model, w->saved, saved_len * sizeof (u32));
      w->len = saved_len;
    } else if (ns_commit (w->db, tx) != 0) {
      w->nfailed++;
    }
  }

  return NULL;
}

static void
sv_run (struct test_ctx *ctx, const bool crash)
{
  test_assert_int_equal (ns_cleanup ("test"), 0);
  nsdb_t *db = ns_open ("test");
  test_assert (db != NULL);

  // Setup - one variable and one model per thread
  _Atomic u32      go = 0;
  struct sv_worker workers[SV_NTHREADS];
  for (u32 i = 0; i < SV_NTHREADS; ++i) {
    test_assert_int_equal (ns_execute (db, NULL, "create v%u u32", i), 0);

    workers[i] = (struct sv_worker){
        .db    = db,
        .id    = i,
        .go    = &go,
        .model = ctx->mem.table->malloc (ctx->mem.self, SV_CAP, sizeof (u32), NULL),
        .saved = ctx->mem.table->malloc (ctx->mem.self, SV_CAP, sizeof (u32), NULL),
        .buf   = ctx->mem.table->malloc (ctx->mem.self, SV_MAX_INSERT, sizeof (u32), NULL),
    };
    test_assert (workers[i].model != NULL);
    test_assert (workers[i].saved != NULL);
    test_assert (workers[i].buf != NULL);
  }

  // Run
  i_thread threads[SV_NTHREADS];
  for (u32 i = 0; i < SV_NTHREADS; ++i) {
    test_assert_int_equal (
        ctx->os.table
            ->thread_create (ctx->os.self, &threads[i], sv_worker_run, &workers[i], &ctx->e),
        SUCCESS
    );
  }
  atomic_store (&go, 1);
  for (u32 i = 0; i < SV_NTHREADS; ++i) {
    ctx->os.table->thread_join (ctx->os.self, threads[i].self);
  }

  // What the threads committed has to survive either way of closing
  if (crash) {
    test_assert_int_equal (ns_crash (db), 0);
  } else {
    test_assert_int_equal (ns_close (db), 0);
  }
  db = ns_open ("test");
  test_assert (db != NULL);

  // Check - every variable is exactly what its thread committed
  u32 *back = ctx->mem.table->malloc (ctx->mem.self, SV_CAP, sizeof (u32), NULL);
  test_assert (back != NULL);

  for (u32 i = 0; i < SV_NTHREADS; ++i) {
    test_assert_int_equal (workers[i].nfailed, 0);

    if (workers[i].len > 0) {
      test_assert_int_equal (
          ns_read (db, NULL, back, SV_CAP * sizeof (u32), "read v%u", i),
          (int)workers[i].len
      );
      test_assert_memequal (back, workers[i].model, workers[i].len * sizeof (u32));
    } else {
      nsdb_var_t *var = ns_get_var (db, NULL, "get v%u", i);
      test_assert (var != NULL);
      test_assert_int_equal (ns_var_len (var), 0);
      ns_var_free (var);
    }

    ctx->mem.table->free (ctx->mem.self, workers[i].model);
    ctx->mem.table->free (ctx->mem.self, workers[i].saved);
    ctx->mem.table->free (ctx->mem.self, workers[i].buf);
  }

  ctx->mem.table->free (ctx->mem.self, back);
  test_assert_int_equal (ns_close (db), 0);
}

TEST (ns_concurrent_separate_variables)
{
  TEST_CASE ("Each thread's variable holds what that thread committed")
  {
    sv_run (ctx, false);
  }

  TEST_CASE ("Same, through a crash and recovery")
  {
    sv_run (ctx, true);
  }
}
#endif
