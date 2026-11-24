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

#include "txn_table/ns_txn.h"

#include "lock_table/ns_lt_lock.h"
#include "ns_csx_assert.h"
#include "ns_gr_lock.h"
#include "ns_latch.h"
#include "ns_testing.h"
#include "os/ns_malloc.h"
#include "os/ns_os.h"

#include <stddef.h>

void
txn_init (struct txn *dest, const txid tid, const struct txn_data data, struct i_mem mem)
{
  dest->data  = data;
  dest->tid   = tid;
  dest->locks = NULL;
  hnode_init (&dest->node, (u32)tid); // Hash key, truncation is fine
  latch_init (&dest->l);
  slab_alloc_init (&dest->lock_alloc, mem, sizeof (struct txn_lock), 512);
}

void
txn_key_init (struct txn *dest, const txid tid)
{
  dest->tid = tid;
  hnode_init (&dest->node, (u32)tid); // Hash key, truncation is fine
  latch_init (&dest->l);
}

void
txn_update_data (struct txn *t, const struct txn_data data)
{
  latch_lock (&t->l);
  t->data = data;
  latch_unlock (&t->l);
}

void
txn_update (struct txn *t, enum tx_state state, const lsn last, const lsn undo_next)
{
  // TODO: state is ignored and the txn always becomes TX_CANDIDATE_FOR_UNDO, so
  // a caller passing another state is silently overridden. Use it or drop it.
  (void)state;
  latch_lock (&t->l);
  t->data = (struct txn_data){
      .state         = TX_CANDIDATE_FOR_UNDO,
      .last_lsn      = last,
      .undo_next_lsn = undo_next,
  };
  latch_unlock (&t->l);
}

void
txn_update_state (struct txn *t, const enum tx_state new_state)
{
  latch_lock (&t->l);
  t->data.state = new_state;
  latch_unlock (&t->l);
}

bool
txn_data_equal_unsafe (const struct txn_data *left, const struct txn_data *right)
{
  bool equal = true;

  equal      = ((equal && left->last_lsn == right->last_lsn) != 0);
  equal      = ((equal && left->undo_next_lsn == right->undo_next_lsn) != 0);
  equal      = ((equal && left->state == right->state) != 0);

  return equal;
}

err_t
txn_newlock (struct txn *t, const struct lt_lock lock, const enum lock_mode mode, error *e)
{
  latch_lock (&t->l);
  DBG_ASSERT (ns_txn, t);

  struct txn_lock *next = slab_alloc_alloc (&t->lock_alloc, e);
  if (next == NULL) {
    latch_unlock (&t->l);
    return error_trace (e);
  }

  next->lock = lock;
  next->mode = mode;

  next->next = t->locks;
  t->locks   = next;
  latch_unlock (&t->l);

  return SUCCESS;
}

struct txn_lock *
txn_get_lock_unsafe (const struct txn *t, const struct lt_lock lock)
{
  DBG_ASSERT (ns_txn, t);

  struct txn_lock *ret  = NULL;

  struct txn_lock *curr = t->locks;

  while (curr != NULL) {
    if (lt_lock_equal (curr->lock, lock)) {
      ret = curr;
      goto theend;
    }
    curr = curr->next;
  }

theend:
  return ret;
}

void
txn_close (struct txn *t)
{
  latch_lock (&t->l);
  DBG_ASSERT (ns_txn, t);

  struct txn_lock *curr = t->locks;
  while (curr != NULL) {
    struct txn_lock *next = curr->next;
    slab_alloc_free (&t->lock_alloc, curr);
    curr = next;
  }

  slab_alloc_destroy (&t->lock_alloc);

  latch_unlock (&t->l);
}

void
txn_foreach_lock (struct txn *t, const lock_func func, void *ctx)
{
  latch_lock (&t->l);
  DBG_ASSERT (ns_txn, t);

  const struct txn_lock *curr = t->locks;
  while (curr != NULL) {
    func (curr->lock, curr->mode, ctx);
    curr = curr->next;
  }

  latch_unlock (&t->l);
}

#ifndef NDEBUG

static void *
txn_newlock_test (void *_tx)
{
  (void)_tx; // Unused
#  define MAYBE_ADD_LOCK(type, r)                                            \
    lock = r;                                                                \
    if (txn_newlock (tx, lock, LM_X, &e) < 0) {                              \
      goto failed;                                                           \
    }                                                                        \
    if (!txn_haslock (tx, lock)) {                                           \
      error_causef (&e, ERR_INVALID_ARGUMENT, "Transaction must have lock"); \
      goto failed;                                                           \
    }

  return NULL;
}

TEST (txn_basic)
{
  TEST_CASE ("txn_newlock single threaded")
  {
    struct txn tx;
    txn_init (
        &tx,
        10,
        (struct txn_data){
            .state         = TX_RUNNING,
            .last_lsn      = 10,
            .undo_next_lsn = 5,
        },
        ctx->mem
    );

    for (u32 i = 0; i < 1000; ++i) {
      txn_newlock_test (&tx);
    }

    txn_close (&tx);
  }

  TEST_CASE ("txn_newlock multi threaded")
  {
    struct txn tx;
    txn_init (
        &tx,
        10,
        (struct txn_data){
            .state         = TX_RUNNING,
            .last_lsn      = 10,
            .undo_next_lsn = 5,
        },
        ctx->mem
    );

    i_thread threads[100];
    _Static_assert (MAX_THREADS >= 100, "this test needs 100 live thread slots");

    u32 started = 0;
    for (u32 i = 0; i < 100; ++i) {
      if (ctx->os.table
              ->thread_create (ctx->os.self, &threads[i], txn_newlock_test, &tx, &ctx->e)) {
        break;
      }
      started++;
    }

    for (u32 i = 0; i < started; ++i) {
      ctx->os.table->thread_join (ctx->os.self, threads[i].self);
    }

    test_assert_int_equal ((int)started, 100);

    txn_close (&tx);
  }
}

TEST (txn_haslock)
{
  TEST_CASE ("An empty transaction holds nothing")
  {
    struct txn tx;
    txn_init (
        &tx,
        10,
        (struct txn_data){
            .state         = TX_RUNNING,
            .last_lsn      = 10,
            .undo_next_lsn = 5,
        },
        ctx->mem
    );

    // Walks the whole (empty) list and falls out of the loop
    test_assert (!txn_haslock (&tx, lock_db ()));

    txn_close (&tx);
  }

  TEST_CASE ("A taken lock is reported held")
  {
    struct txn tx;
    txn_init (
        &tx,
        10,
        (struct txn_data){
            .state         = TX_RUNNING,
            .last_lsn      = 10,
            .undo_next_lsn = 5,
        },
        ctx->mem
    );

    test_assert_int_equal (txn_newlock (&tx, lock_db (), LM_X, &ctx->e), SUCCESS);
    test_assert (txn_haslock (&tx, lock_db ()));

    // Re-taking it is a no-op, and it is still held afterwards
    test_assert_int_equal (txn_newlock (&tx, lock_db (), LM_X, &ctx->e), SUCCESS);
    test_assert (txn_haslock (&tx, lock_db ()));

    txn_close (&tx);
  }

  TEST_CASE ("Closing gives the locks back")
  {
    struct txn tx;
    txn_init (
        &tx,
        10,
        (struct txn_data){
            .state         = TX_RUNNING,
            .last_lsn      = 10,
            .undo_next_lsn = 5,
        },
        ctx->mem
    );

    test_assert_int_equal (txn_newlock (&tx, lock_db (), LM_X, &ctx->e), SUCCESS);
    test_assert (txn_haslock (&tx, lock_db ()));
    txn_close (&tx);

    // A reinitialised txn starts over, it does not inherit the old list
    txn_init (
        &tx,
        11,
        (struct txn_data){
            .state         = TX_RUNNING,
            .last_lsn      = 11,
            .undo_next_lsn = 6,
        },
        ctx->mem
    );
    test_assert (!txn_haslock (&tx, lock_db ()));

    txn_close (&tx);
  }
}
#endif
