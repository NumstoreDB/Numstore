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

// Several threads, one database handle, one variable for all of them
//
// Writers: every transaction inserts one chunk of SH_CHUNK elements at the
// front of foo and then commits or rolls back. An element says which thread
// wrote it, which of that thread's transactions, and where in the chunk it
// sits - so a chunk can be checked on its own.
//
// Readers: every transaction reads all of foo. Whatever moment that is, foo
// must be a whole number of whole chunks. Half a chunk, a chunk with a hole
// in it or a chunk of a rolled back transaction means one transaction saw
// into another.
//
// LOCKING: this doesn't care how transactions are locked, only that they
// are isolated. With the global database lock they run one after the other.
// With hierarchical locks they have to queue up on foo to pass.
//
// The threads call the API with their own transaction. Calls with a NULL
// transaction from several threads are regression 0022.

enum
{
  SH_NWRITERS = 3,
  SH_NREADERS = 2,
  SH_NTXNS    = 30,
  SH_CHUNK    = 1500,

  SH_CAP      = SH_NWRITERS * SH_NTXNS * SH_CHUNK,
};

struct sh_worker
{
  nsdb_t      *db;
  u32          id;
  u32         *buf;
  u32          ncommitted;
  u32          nfailed;
  _Atomic u32 *writers_left;
  _Atomic u32 *go; // Start signal, so the threads really overlap
};

// thread (4 bits) | transaction (12 bits) | index in the chunk (16 bits)
static u32
sh_elem (const u32 id, const u32 t, const u32 i)
{
  return (id << 28) | (t << 16) | i;
}

// foo is [len] elements: whole chunks, each one intact?
static bool
sh_chunks_intact (const u32 *data, const u32 len)
{
  if (len % SH_CHUNK != 0) {
    return false;
  }
  for (u32 c = 0; c < len / SH_CHUNK; ++c) {
    const u32 *chunk = &data[c * SH_CHUNK];
    const u32  head  = chunk[0];
    if ((head & 0xFFFF) != 0) {
      return false;
    }
    for (u32 i = 0; i < SH_CHUNK; ++i) {
      if (chunk[i] != (head | i)) {
        return false;
      }
    }
  }
  return true;
}

static u32
sh_rand (u64 *state, const u32 n)
{
  *state ^= *state << 13;
  *state ^= *state >> 7;
  *state ^= *state << 17;
  return (u32)(*state % n);
}

static void *
sh_writer_run (void *arg)
{
  struct sh_worker *w  = arg;
  u64               rs = 0x9E3779B97F4A7C15ULL * (w->id + 1);

  // Everybody starts at once
  while (atomic_load (w->go) == 0) {
  }

  for (u32 t = 0; t < SH_NTXNS; ++t) {
    for (u32 i = 0; i < SH_CHUNK; ++i) {
      w->buf[i] = sh_elem (w->id, t, i);
    }

    nsdb_txn_t *tx = ns_begin (w->db);
    if (tx == NULL) {
      w->nfailed++;
      break;
    }

    if (ns_write (w->db, tx, w->buf, SH_CHUNK * sizeof (u32), "insert foo 0 %d", SH_CHUNK)
        != SH_CHUNK) {
      w->nfailed++;
    }

    // One in three transactions rolls back
    if (sh_rand (&rs, 3) == 0) {
      if (ns_rollback (w->db, tx) != 0) {
        w->nfailed++;
      }
    } else if (ns_commit (w->db, tx) != 0) {
      w->nfailed++;
    } else {
      w->ncommitted++;
    }
  }

  atomic_fetch_sub (w->writers_left, 1);
  return NULL;
}

static void *
sh_reader_run (void *arg)
{
  struct sh_worker *w = arg;

  // Everybody starts at once
  while (atomic_load (w->go) == 0) {
  }

  // Keep reading for as long as somebody writes
  while (atomic_load (w->writers_left) > 0) {
    nsdb_txn_t *tx = ns_begin (w->db);
    if (tx == NULL) {
      w->nfailed++;
      break;
    }

    // An empty foo reads 0 elements
    const sb_size len = ns_read (w->db, tx, w->buf, SH_CAP * sizeof (u32), "read foo");
    if (len < 0 || !sh_chunks_intact (w->buf, (u32)len)) {
      w->nfailed++;
    }

    if (ns_commit (w->db, tx) != 0) {
      w->nfailed++;
    }
  }

  return NULL;
}

static void
sh_run (struct test_ctx *ctx, const bool crash)
{
  test_assert_int_equal (ns_cleanup ("test"), 0);
  nsdb_t *db = ns_open ("test");
  test_assert (db != NULL);
  test_assert_int_equal (ns_execute (db, NULL, "create foo u32"), 0);

  // Setup
  _Atomic u32      writers_left = SH_NWRITERS;
  _Atomic u32      go           = 0;
  struct sh_worker workers[SH_NWRITERS + SH_NREADERS];
  for (u32 i = 0; i < SH_NWRITERS + SH_NREADERS; ++i) {
    const bool writer = i < SH_NWRITERS;
    workers[i]        = (struct sh_worker){
        .db           = db,
        .id           = i,
        .writers_left = &writers_left,
        .go           = &go,
        .buf = ctx->mem.table
                   ->malloc (ctx->mem.self, writer ? SH_CHUNK : SH_CAP, sizeof (u32), NULL),
    };
    test_assert (workers[i].buf != NULL);
  }

  // Run
  i_thread threads[SH_NWRITERS + SH_NREADERS];
  for (u32 i = 0; i < SH_NWRITERS + SH_NREADERS; ++i) {
    test_assert_int_equal (
        ctx->os.table->thread_create (
            ctx->os.self,
            &threads[i],
            i < SH_NWRITERS ? sh_writer_run : sh_reader_run,
            &workers[i],
            &ctx->e
        ),
        SUCCESS
    );
  }
  atomic_store (&go, 1);
  for (u32 i = 0; i < SH_NWRITERS + SH_NREADERS; ++i) {
    ctx->os.table->thread_join (ctx->os.self, threads[i].self);
  }

  if (crash) {
    test_assert_int_equal (ns_crash (db), 0);
  } else {
    test_assert_int_equal (ns_close (db), 0);
  }
  db = ns_open ("test");
  test_assert (db != NULL);

  // Check - foo is the committed chunks and nothing else
  u32 ncommitted = 0;
  for (u32 i = 0; i < SH_NWRITERS + SH_NREADERS; ++i) {
    test_assert_int_equal (workers[i].nfailed, 0);
    ncommitted += workers[i].ncommitted;
  }

  u32 *back = ctx->mem.table->malloc (ctx->mem.self, SH_CAP, sizeof (u32), NULL);
  test_assert (back != NULL);

  sb_size len = 0;
  if (ncommitted > 0) {
    len = ns_read (db, NULL, back, SH_CAP * sizeof (u32), "read foo");
  }
  test_assert_int_equal (len, (int)(ncommitted * SH_CHUNK));
  test_assert (sh_chunks_intact (back, (u32)len));

  // Every writer finds as many of its chunks as it committed
  u32 seen[SH_NWRITERS] = {0};
  for (u32 c = 0; c < ncommitted; ++c) {
    const u32 id = back[c * SH_CHUNK] >> 28;
    test_assert (id < SH_NWRITERS);
    seen[id]++;
  }
  for (u32 i = 0; i < SH_NWRITERS; ++i) {
    test_assert_int_equal (seen[i], workers[i].ncommitted);
  }

  // A writer's own chunks are in the order it committed them - each insert
  // goes to the front, so its transaction numbers go down
  u32 last[SH_NWRITERS];
  for (u32 i = 0; i < SH_NWRITERS; ++i) {
    last[i] = SH_NTXNS;
  }
  for (u32 c = 0; c < ncommitted; ++c) {
    const u32 head = back[c * SH_CHUNK];
    const u32 id   = head >> 28;
    const u32 t    = (head >> 16) & 0xFFF;
    test_assert (t < last[id]);
    last[id] = t;
  }

  ctx->mem.table->free (ctx->mem.self, back);
  for (u32 i = 0; i < SH_NWRITERS + SH_NREADERS; ++i) {
    ctx->mem.table->free (ctx->mem.self, workers[i].buf);
  }
  test_assert_int_equal (ns_close (db), 0);
}

TEST (ns_concurrent_shared_variable)
{
  TEST_CASE ("Writers and readers on one variable never see half a transaction")
  {
    sh_run (ctx, false);
  }

  TEST_CASE ("Same, through a crash and recovery")
  {
    sh_run (ctx, true);
  }
}
#endif
