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

#ifndef NDEBUG
#  include "ns_testing.h"
#endif

#ifndef NDEBUG

// What the global database lock promises
//
// pgr_begin_txn takes lock_db () in LM_X, so one transaction is open at a
// time and ns_begin waits for the one before it to end.
//
// LOCKING: both tests here are only true with that global lock, so they are
// disabled when EXPERIMENTAL_HIERARCHIAL_TXNS is on:
//
//  - ns_concurrent_begin_waits_for_open_txn checks the waiting itself.
//    With hierarchical locks two transactions are meant to be open at once.
//
//  - ns_concurrent_opposite_lock_order takes two variables in opposite
//    orders from two threads. With one lock for everything that can't
//    deadlock. With a lock per variable it is the textbook deadlock, and
//    there is no deadlock detection or lock timeout (lockt_lock waits
//    forever), so the test would hang instead of fail. It needs a rewrite
//    for hierarchical locks once one of the two transactions is supposed
//    to get an error.
//
// The tests that hold for any locking are 0011 and 0012.

struct gl_waiter
{
  nsdb_t     *db;
  _Atomic u32 started; // The thread is about to call ns_begin
  _Atomic u32 entered; // ns_begin returned
  u32         nfailed;
};

static void *
gl_waiter_run (void *arg)
{
  struct gl_waiter *w = arg;

  atomic_store (&w->started, 1);
  nsdb_txn_t *tx = ns_begin (w->db);
  atomic_store (&w->entered, 1);

  if (tx == NULL) {
    w->nfailed++;
    return NULL;
  }

  // The first transaction is over, so its insert is visible
  u32 dest[4];
  if (ns_read (w->db, tx, dest, sizeof dest, "read foo") != 1 || dest[0] != 7) {
    w->nfailed++;
  }
  if (ns_commit (w->db, tx) != 0) {
    w->nfailed++;
  }

  return NULL;
}

#  ifndef EXPERIMENTAL_HIERARCHIAL_TXNS
TEST (ns_concurrent_begin_waits_for_open_txn)
#  else
TEST_DISABLED (ns_concurrent_begin_waits_for_open_txn)
#  endif
{
  test_assert_int_equal (ns_cleanup ("test"), 0);
  nsdb_t *db = ns_open ("test");
  test_assert (db != NULL);
  test_assert_int_equal (ns_execute (db, NULL, "create foo u32"), 0);

  // Setup - a transaction of ours is open, with an insert in it
  const u32   seven = 7;
  nsdb_txn_t *tx    = ns_begin (db);
  test_assert (tx != NULL);
  test_assert_int_equal (ns_write (db, tx, &seven, sizeof seven, "insert foo 0 1"), 1);

  // Run - another thread begins a transaction
  struct gl_waiter w = {.db = db};
  i_thread         thread;
  test_assert_int_equal (
      ctx->os.table->thread_create (ctx->os.self, &thread, gl_waiter_run, &w, &ctx->e),
      SUCCESS
  );

  // It gets as far as ns_begin and stays there. Half a second is a long
  // time for an ns_begin that isn't waiting
  while (atomic_load (&w.started) == 0) {
  }
  i_sleep_ms (500);
  test_assert_int_equal (atomic_load (&w.entered), 0);

  // Our commit lets it in
  test_assert_int_equal (ns_commit (db, tx), 0);
  ctx->os.table->thread_join (ctx->os.self, thread.self);

  test_assert_int_equal (atomic_load (&w.entered), 1);
  test_assert_int_equal (w.nfailed, 0);

  test_assert_int_equal (ns_close (db), 0);
}

enum
{
  GL_ROUNDS = 40,
};

struct gl_orderer
{
  nsdb_t      *db;
  const char  *first;
  const char  *second;
  _Atomic u32 *go; // Start signal, so the threads really overlap
  u32          nfailed;
};

static void *
gl_orderer_run (void *arg)
{
  struct gl_orderer *w = arg;
  const u32          v = 1;

  // Everybody starts at once
  while (atomic_load (w->go) == 0) {
  }

  for (u32 i = 0; i < GL_ROUNDS; ++i) {
    nsdb_txn_t *tx = ns_begin (w->db);
    if (tx == NULL) {
      w->nfailed++;
      return NULL;
    }

    // The other thread takes the same two variables the other way round
    if (ns_write (w->db, tx, &v, sizeof v, "insert %s 0 1", w->first) != 1) {
      w->nfailed++;
    }
    if (ns_write (w->db, tx, &v, sizeof v, "insert %s 0 1", w->second) != 1) {
      w->nfailed++;
    }

    if (ns_commit (w->db, tx) != 0) {
      w->nfailed++;
    }
  }

  return NULL;
}

#  ifndef EXPERIMENTAL_HIERARCHIAL_TXNS
TEST (ns_concurrent_opposite_lock_order)
#  else
TEST_DISABLED (ns_concurrent_opposite_lock_order)
#  endif
{
  test_assert_int_equal (ns_cleanup ("test"), 0);
  nsdb_t *db = ns_open ("test");
  test_assert (db != NULL);
  test_assert_int_equal (ns_execute (db, NULL, "create a u32"), 0);
  test_assert_int_equal (ns_execute (db, NULL, "create b u32"), 0);

  // Run - a then b in one thread, b then a in the other
  _Atomic u32       go         = 0;
  struct gl_orderer workers[2] = {
      {.db = db, .first = "a", .second = "b", .go = &go},
      {.db = db, .first = "b", .second = "a", .go = &go},
  };
  i_thread threads[2];
  for (u32 i = 0; i < 2; ++i) {
    test_assert_int_equal (
        ctx->os.table
            ->thread_create (ctx->os.self, &threads[i], gl_orderer_run, &workers[i], &ctx->e),
        SUCCESS
    );
  }
  atomic_store (&go, 1);
  for (u32 i = 0; i < 2; ++i) {
    ctx->os.table->thread_join (ctx->os.self, threads[i].self);
  }

  // Check - nobody got stuck, and every insert is there
  test_assert_int_equal (workers[0].nfailed, 0);
  test_assert_int_equal (workers[1].nfailed, 0);

  nsdb_var_t *a = ns_get_var (db, NULL, "get a");
  nsdb_var_t *b = ns_get_var (db, NULL, "get b");
  test_assert (a != NULL);
  test_assert (b != NULL);
  test_assert_int_equal (ns_var_len (a), 2 * GL_ROUNDS);
  test_assert_int_equal (ns_var_len (b), 2 * GL_ROUNDS);
  ns_var_free (a);
  ns_var_free (b);

  test_assert_int_equal (ns_close (db), 0);
}
#endif
