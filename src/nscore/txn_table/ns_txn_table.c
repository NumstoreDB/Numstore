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

#include "nscore/txn_table/ns_txn_table.h"

#include "core/ns_csx_assert.h"
#include "core/ns_dbl_buffer.h"
#include "core/ns_utils.h"
#include "core/os/ns_memory.h"
#include "core/os/ns_threading.h"
#include "core/os/ns_time.h"
#include "core/testing/ns_testing.h"
#include "nscore/lock_table/ns_lock_table.h"

#include <stddef.h>

/******************************************************************************
 * SECTION: Transaction
 ******************************************************************************/

void
txn_init (struct ns_txn *dest, const txid tid, const struct ns_txn_data data, struct i_mem mem)
{
  dest->data  = data;
  dest->tid   = tid;
  dest->locks = NULL;
  hnode_init (&dest->node, tid);
  latch_init (&dest->l);
  slab_alloc_init (&dest->lock_alloc, mem, sizeof (struct ns_txn_lock), 512);
}

void
txn_key_init (struct ns_txn *dest, const txid tid)
{
  dest->tid = tid;
  hnode_init (&dest->node, tid);
  latch_init (&dest->l);
}

void
txn_update_data (struct ns_txn *t, const struct ns_txn_data data)
{
  latch_lock (&t->l);
  t->data = data;
  latch_unlock (&t->l);
}

void
txn_update (struct ns_txn *t, enum tx_state state, const lsn last, const lsn undo_next)
{
  (void)state; // TODO - I think this needs to be used - fix it
  latch_lock (&t->l);
  t->data = (struct ns_txn_data){
      .state         = TX_CANDIDATE_FOR_UNDO,
      .last_lsn      = last,
      .undo_next_lsn = undo_next,
  };
  latch_unlock (&t->l);
}

void
txn_update_state (struct ns_txn *t, const enum tx_state new_state)
{
  latch_lock (&t->l);
  t->data.state = new_state;
  latch_unlock (&t->l);
}

bool
txn_data_equal_unsafe (const struct ns_txn_data *left, const struct ns_txn_data *right)
{
  bool equal = true;

  equal      = ((equal && left->last_lsn == right->last_lsn) != 0);
  equal      = ((equal && left->undo_next_lsn == right->undo_next_lsn) != 0);
  equal      = ((equal && left->state == right->state) != 0);

  return equal;
}

static bool
txn_haslock_unsafe (const struct ns_txn *t, const struct lt_lock lock)
{
  bool                      ret  = false;

  const struct ns_txn_lock *curr = t->locks;
  while (curr != NULL) {
    if (lt_lock_equal (curr->lock, lock)) {
      ret = true;
      goto theend;
    }
    curr = curr->next;
  }

theend:
  return ret;
}

err_t
txn_newlock (struct ns_txn *t, const struct lt_lock lock, const enum lock_mode mode, error *e)
{
  latch_lock (&t->l);

  if (txn_haslock_unsafe (t, lock)) {
    latch_unlock (&t->l);
    return SUCCESS;
  }

  struct ns_txn_lock *next = slab_alloc_alloc (&t->lock_alloc, e);
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

bool
txn_haslock (struct ns_txn *t, const struct lt_lock lock)
{
  latch_lock (&t->l);

  bool                      ret  = false;

  const struct ns_txn_lock *curr = t->locks;
  while (curr != NULL) {
    if (lt_lock_equal (curr->lock, lock)) {
      ret = true;
      goto theend;
    }
    curr = curr->next;
  }

theend:
  latch_unlock (&t->l);
  return ret;
}

void
txn_close (struct ns_txn *t)
{
  latch_lock (&t->l);

  struct ns_txn_lock *curr = t->locks;
  while (curr != NULL) {
    struct ns_txn_lock *next = curr->next;
    slab_alloc_free (&t->lock_alloc, curr);
    curr = next;
  }

  slab_alloc_destroy (&t->lock_alloc);

  latch_unlock (&t->l);
}

void
txn_foreach_lock (struct ns_txn *t, const lock_func func, void *ctx)
{
  latch_lock (&t->l);

  const struct ns_txn_lock *curr = t->locks;
  while (curr != NULL) {
    func (curr->lock, curr->mode, ctx);
    curr = curr->next;
  }

  latch_unlock (&t->l);
}

#ifdef TESTING

static void *
txn_newlock_test (void *_tx)
{
  (void)_tx; // Unused
#  define MAYBE_ADD_LOCK(type, r)                                            \
    lock = r;                                                                \
    if (txn_newlock (tx, lock, LM_X, &e)) {                                  \
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
  error e = error_create ();

  TEST_CASE ("txn_newlock single threaded")
  {
    struct ns_txn tx;
    txn_init (
        &tx,
        10,
        (struct ns_txn_data){
            .state         = TX_RUNNING,
            .last_lsn      = 10,
            .undo_next_lsn = 5,
        },
        mem
    );

    for (u32 i = 0; i < 1000; ++i) {
      txn_newlock_test (&tx);
    }

    txn_close (&tx);
  }

  TEST_CASE ("txn_newlock multi threaded")
  {
    struct ns_txn tx;
    txn_init (
        &tx,
        10,
        (struct ns_txn_data){
            .state         = TX_RUNNING,
            .last_lsn      = 10,
            .undo_next_lsn = 5,
        },
        mem
    );

    i_thread threads[100];

    for (u32 i = 0; i < 100; ++i) {
      i_thread_create (default_threading (), &threads[i], txn_newlock_test, &tx, &e);
    }

    for (u32 i = 0; i < 100; ++i) {
      i_thread_join (default_threading (), &threads[i], &e);
    }

    txn_close (&tx);
  }
}
#endif

/******************************************************************************
 * SECTION: Transaction Table
 ******************************************************************************/

DEFINE_DBG_ASSERT (struct ns_txn_table, txn_table, t, { ASSERT (t); })

#define TXN_SERIAL_UNIT (sizeof (txid) + sizeof (lsn) + sizeof (lsn))

struct ns_txn_table
{
  struct i_mem   mem;
  latch          l;
  struct htable *t;

  bool           isfrozen;
};

struct ns_txn_table *
txnt_open (struct i_mem mem, error *e)
{
  struct ns_txn_table *dest = i_malloc (mem, 1, sizeof *dest, e);
  if (dest == NULL) {
    goto failed;
  }

  dest->mem = mem;
  dest->t   = htable_create (512, mem, e);
  if (dest->t == NULL) {
    goto dest_failed;
  }

  latch_init (&dest->l);

  return dest;

dest_failed:
  i_free (mem, dest);
failed:
  return NULL;
}

void
txnt_close (struct ns_txn_table *t)
{
  DBG_ASSERT (txn_table, t);
  struct i_mem mem = t->mem;
  latch_lock (&t->l);
  htable_free (t->t);
  latch_unlock (&t->l);
  i_free (mem, t);
}

#ifdef TESTING
TEST (txnt_open)
{
  TEST_CASE ("basic")
  {
    error                e = error_create ();
    struct ns_txn_table *t = txnt_open (mem, &e);
    txnt_close (t);
  }

  TEST_CASE ("open multiple")
  {
    error e = error_create ();
    for (int i = 0; i < 4; ++i) {
      struct ns_txn_table *t = txnt_open (mem, &e);
      txnt_close (t);
    }
  }
}
#endif

struct ns_txn_merge_ctx
{
  struct ns_txn_table *dest;
  error               *e;
  struct dbl_buffer   *txn_dest;
  struct slab_alloc   *alloc;
  struct i_mem         mem;
};

static void
merge_txn (struct ns_txn *tx, void *vctx)
{
  const struct ns_txn_merge_ctx *ctx = vctx;
  ASSERT (ctx->txn_dest == NULL || ctx->txn_dest->size == sizeof (struct ns_txn));

  // Fail fast on an error
  if (ctx->e->cause_code) {
    return;
  }

  latch_lock (&tx->l);

  // Skip duplicate transactions
  if (txn_exists (ctx->dest, tx->tid)) {
    goto theend;
  }

  // Handle in case we copy transaction over
  struct ns_txn *target_txn = tx;

  // If provided, allocate a new transaction to copy over
  if (ctx->txn_dest && ctx->alloc) {
    target_txn = slab_alloc_alloc (ctx->alloc, ctx->e);
    if (target_txn == NULL) {
      goto theend;
    }
    if (dblb_append (ctx->txn_dest, &target_txn, 1, ctx->e)) {
      goto theend;
    }
    txn_init (target_txn, tx->tid, tx->data, ctx->mem);
  }

  // Insert into the table
  txnt_insert_txn (ctx->dest, target_txn);

theend:
  latch_unlock (&tx->l);
}

err_t
txnt_merge_into (
    struct ns_txn_table *dest,
    struct ns_txn_table *src,
    struct dbl_buffer   *txn_dest,
    struct slab_alloc   *alloc,
    struct i_mem         mem,
    error               *e
)
{
  struct ns_txn_merge_ctx ctx = {
      .dest     = dest,
      .e        = e,
      .txn_dest = txn_dest,
      .alloc    = alloc,
      .mem      = mem,
  };

  latch_lock (&src->l);
  txnt_foreach (src, merge_txn, &ctx);
  latch_unlock (&src->l);

  return ctx.e->cause_code;
}

#ifdef TESTING
TEST (txnt_merge_into)
{
  TEST_CASE ("Empty to Empty")
  {
    error                e      = error_create ();
    struct ns_txn_table *src    = txnt_open (mem, &e);
    struct ns_txn_table *dest   = txnt_open (mem, &e);
    const err_t          result = txnt_merge_into (dest, src, NULL, NULL, mem, &e);
    test_assert (result == SUCCESS);

    txnt_close (dest);
    txnt_close (src);
  }

  TEST_CASE ("Data")
  {
    error                e    = error_create ();
    struct ns_txn_table *dest = txnt_open (mem, &e);
    struct ns_txn_table *src  = txnt_open (mem, &e);
    // Add to dest
    struct ns_txn        dest_txns[5];
    for (int i = 0; i < 5; i++) {
      txn_init (
          &dest_txns[i],
          i + 1,
          (struct ns_txn_data){
              .last_lsn      = (i + 1) * 10,
              .undo_next_lsn = (i + 1) * 10 - 1,
              .state         = TX_RUNNING,
          },
          mem
      );
      txnt_insert_txn (dest, &dest_txns[i]);
    }

    // Add to src (different tids)
    struct ns_txn src_txns[5];
    for (int i = 0; i < 5; i++) {
      txn_init (
          &src_txns[i],
          i + 6,
          (struct ns_txn_data){
              .last_lsn      = (i + 6) * 10,
              .undo_next_lsn = (i + 6) * 10 - 1,
              .state         = TX_CANDIDATE_FOR_UNDO,
          },
          mem
      );
      txnt_insert_txn (src, &src_txns[i]);
    }

    const err_t result = txnt_merge_into (dest, src, NULL, NULL, mem, &e);
    test_assert (result == SUCCESS);

    // Verify all exist in dest
    for (txid tid = 1; tid <= 10; tid++) {
      test_assert (txn_exists (dest, tid));
    }

    txnt_close (dest);
    txnt_close (src);
  }

  TEST_CASE ("no duplicate insert")
  {
    error                e    = error_create ();

    struct ns_txn_table *dest = txnt_open (mem, &e);
    struct ns_txn_table *src  = txnt_open (mem, &e);
    // Add same tid to both with different values
    struct ns_txn        dest_tx;
    txn_init (
        &dest_tx,
        42,
        (struct ns_txn_data){
            .last_lsn      = 100,
            .undo_next_lsn = 90,
            .state         = TX_RUNNING,
        },
        mem
    );
    txnt_insert_txn (dest, &dest_tx);

    struct ns_txn src_tx;
    txn_init (
        &src_tx,
        42,
        (struct ns_txn_data){
            .last_lsn      = 200,
            .undo_next_lsn = 190,
            .state         = TX_COMMITTED,
        },
        mem
    );
    txnt_insert_txn (src, &src_tx);

    txnt_merge_into (dest, src, NULL, NULL, mem, &e);

    // Dest should keep its original value
    struct ns_txn *retrieved;
    bool           found = txnt_get (&retrieved, dest, 42);
    test_assert (found);
    test_assert_int_equal (retrieved->data.last_lsn, 100);
    test_assert_int_equal (retrieved->data.state, TX_RUNNING);

    txnt_close (dest);
    txnt_close (src);
  }
}
#endif

static void
find_max_undo (struct ns_txn *tx, void *vctx)
{
  slsn *max = vctx;

  latch_lock (&tx->l);

  if ((tx->data.state == TX_CANDIDATE_FOR_UNDO) && ((slsn)tx->data.undo_next_lsn > *max)) {
    *max = tx->data.undo_next_lsn;
  }

  latch_unlock (&tx->l);
}

slsn
txnt_max_u_undo_lsn (struct ns_txn_table *t)
{
  slsn max = -1;

  latch_lock (&t->l);
  txnt_foreach (t, find_max_undo, &max);
  latch_unlock (&t->l);

  return max;
}

#ifdef TESTING
TEST (txnt_max_u_undo_lsn)
{
  TEST_CASE ("empty")
  {
    error                e   = error_create ();
    struct ns_txn_table *t   = txnt_open (mem, &e);
    slsn                 max = txnt_max_u_undo_lsn (t);
    test_assert (max == -1);

    txnt_close (t);
  }

  TEST_CASE ("candidates")
  {
    error                e = error_create ();
    struct ns_txn_table *t = txnt_open (mem, &e);
    struct ns_txn        tx1, tx2, tx3, tx4;

    // Running - should be ignored
    txn_init (
        &tx1,
        1,
        (struct ns_txn_data){.last_lsn = 100, .undo_next_lsn = 100, .state = TX_RUNNING},
        mem
    );

    // Candidates
    txn_init (
        &tx2,
        2,
        (struct ns_txn_data){.last_lsn = 50, .undo_next_lsn = 40, .state = TX_CANDIDATE_FOR_UNDO},
        mem
    );
    txn_init (
        &tx3,
        3,
        (struct ns_txn_data){.last_lsn = 80, .undo_next_lsn = 75, .state = TX_CANDIDATE_FOR_UNDO},
        mem
    );
    txn_init (
        &tx4,
        4,
        (struct ns_txn_data){.last_lsn = 60, .undo_next_lsn = 55, .state = TX_CANDIDATE_FOR_UNDO},
        mem
    );

    txnt_insert_txn (t, &tx1);
    txnt_insert_txn (t, &tx2);
    txnt_insert_txn (t, &tx3);
    txnt_insert_txn (t, &tx4);

    slsn max = txnt_max_u_undo_lsn (t);
    test_assert (max == 75);

    txnt_close (t);
  }

  TEST_CASE ("only running txns")
  {
    error                e = error_create ();
    struct ns_txn_table *t = txnt_open (mem, &e);
    struct ns_txn        tx1, tx2, tx3;
    txn_init (
        &tx1,
        1,
        (struct ns_txn_data){.last_lsn = 100, .undo_next_lsn = 90, .state = TX_RUNNING},
        mem
    );
    txn_init (
        &tx2,
        2,
        (struct ns_txn_data){.last_lsn = 200, .undo_next_lsn = 190, .state = TX_RUNNING},
        mem
    );
    txn_init (
        &tx3,
        3,
        (struct ns_txn_data){.last_lsn = 300, .undo_next_lsn = 290, .state = TX_COMMITTED},
        mem
    );

    txnt_insert_txn (t, &tx1);
    txnt_insert_txn (t, &tx2);
    txnt_insert_txn (t, &tx3);

    slsn max = txnt_max_u_undo_lsn (t);
    test_assert (max == -1);

    txnt_close (t);
  }
}
#endif

static void
find_min_lsn (struct ns_txn *tx, void *vctx)
{
  slsn *min = vctx;

  latch_lock (&tx->l);

  if (*min == -1 || (slsn)tx->data.min_lsn < *min) {
    *min = tx->data.min_lsn;
  }

  latch_unlock (&tx->l);
}

slsn
txnt_min_lsn (struct ns_txn_table *t)
{
  slsn min = -1; // Actually a big number b/c unsigned

  latch_lock (&t->l);
  txnt_foreach (t, find_min_lsn, &min);
  latch_unlock (&t->l);

  return min;
}

#ifdef TESTING
TEST (txnt_min_lsn)
{
  TEST_CASE ("empty")
  {
    error                e   = error_create ();
    struct ns_txn_table *t   = txnt_open (mem, &e);
    const slsn           min = txnt_min_lsn (t);
    test_assert (min == -1);

    txnt_close (t);
  }

  TEST_CASE ("candidates")
  {
    error                e = error_create ();
    struct ns_txn_table *t = txnt_open (mem, &e);
    struct ns_txn        tx1, tx2, tx3, tx4;

    txn_init (&tx1, 1, (struct ns_txn_data){.min_lsn = 100}, mem);
    txn_init (&tx2, 2, (struct ns_txn_data){.min_lsn = 40}, mem);
    txn_init (&tx3, 3, (struct ns_txn_data){.min_lsn = 75}, mem);
    txn_init (&tx4, 4, (struct ns_txn_data){.min_lsn = 55}, mem);

    txnt_insert_txn (t, &tx1);
    txnt_insert_txn (t, &tx2);
    txnt_insert_txn (t, &tx3);
    txnt_insert_txn (t, &tx4);

    const slsn min = txnt_min_lsn (t);
    test_assert (min == 40);

    txnt_close (t);
  }
}
#endif

struct ns_txn_foreach_ctx
{
  void (*action) (struct ns_txn *, void *ctx);
  void *ctx;
};

static void
hnode_foreach (struct hnode *node, void *ctx)
{
  const struct ns_txn_foreach_ctx *_ctx = ctx;
  _ctx->action (container_of (node, struct ns_txn, node), _ctx->ctx);
}

void
txnt_foreach (const struct ns_txn_table *t, void (*action) (struct ns_txn *, void *ctx), void *ctx)
{
  struct ns_txn_foreach_ctx _ctx = {
      .action = action,
      .ctx    = ctx,
  };
  htable_foreach (t->t, hnode_foreach, &_ctx);
}

u32
txnt_get_size (const struct ns_txn_table *dest)
{
  return htable_size (dest->t);
}

static bool
txn_equals_for_exists (const struct hnode *left, const struct hnode *right)
{
  // Might have passed the exact same ref as exists in the htable
  if (left == right) {
    return true;
  }

  // Otherwise, passed a key with just relevant information

  struct ns_txn *_left  = container_of (left, struct ns_txn, node);
  struct ns_txn *_right = container_of (right, struct ns_txn, node);

  latch_lock (&_left->l);
  latch_lock (&_right->l);

  bool ret = _left->tid == _right->tid;

  latch_unlock (&_right->l);
  latch_unlock (&_left->l);

  return ret;
}

bool
txn_exists (const struct ns_txn_table *t, const txid tid)
{
  struct ns_txn tx;
  txn_key_init (&tx, tid);

  struct hnode **ret = htable_lookup (t->t, &tx.node, txn_equals_for_exists);

  return ret != NULL;
}

#ifdef TESTING
TEST (txnt_exists)
{
  TEST_CASE ("txnt_exists")
  {
    error                e = error_create ();
    struct ns_txn_table *t = txnt_open (mem, &e);
    test_assert (!txn_exists (t, 1100));

    struct ns_txn tx;
    txn_init (
        &tx,
        1100,
        (struct ns_txn_data){
            .last_lsn      = 100,
            .undo_next_lsn = 90,
            .state         = TX_RUNNING,
        },
        mem
    );

    txnt_insert_txn (t, &tx);
    test_assert (txn_exists (t, 1100));

    txnt_close (t);
  }
}
#endif

void
txnt_insert_txn (struct ns_txn_table *t, struct ns_txn *tx)
{
  DBG_ASSERT (txn_table, t);
  ASSERT (!txn_exists (t, tx->tid));

  latch_lock (&t->l);
  htable_insert (t->t, &tx->node);
  latch_unlock (&t->l);
}

void
txnt_insert_txn_if_not_exists (struct ns_txn_table *t, struct ns_txn *tx)
{
  DBG_ASSERT (txn_table, t);

  latch_lock (&t->l);

  if (txn_exists (t, tx->tid)) {
    goto theend;
  }

  htable_insert (t->t, &tx->node);

theend:
  latch_unlock (&t->l);
}

#ifdef TESTING
TEST (txnt_insert)
{
  TEST_CASE ("new")
  {
    error                e = error_create ();
    struct ns_txn_table *t = txnt_open (mem, &e);
    struct ns_txn        tx;
    txn_init (
        &tx,
        900,
        (struct ns_txn_data){
            .last_lsn      = 100,
            .undo_next_lsn = 90,
            .state         = TX_RUNNING,
        },
        mem
    );

    txnt_insert_txn_if_not_exists (t, &tx);

    struct ns_txn *retrieved;
    bool           found = txnt_get (&retrieved, t, 900);
    test_assert (found);
    test_assert (retrieved->tid == 900);

    txnt_close (t);
  }

  TEST_CASE ("if not exists but exists")
  {
    error                e = error_create ();
    struct ns_txn_table *t = txnt_open (mem, &e);
    struct ns_txn        tx1;
    txn_init (
        &tx1,
        1000,
        (struct ns_txn_data){
            .last_lsn      = 100,
            .undo_next_lsn = 90,
            .state         = TX_RUNNING,
        },
        mem
    );

    txnt_insert_txn (t, &tx1);

    struct ns_txn tx2;
    txn_init (
        &tx2,
        1000,
        (struct ns_txn_data){
            .last_lsn      = 200,
            .undo_next_lsn = 180,
            .state         = TX_COMMITTED,
        },
        mem
    );

    txnt_insert_txn_if_not_exists (t, &tx2);

    struct ns_txn *retrieved;
    bool           found = txnt_get (&retrieved, t, 1000);
    test_assert (found);
    test_assert (retrieved->data.last_lsn == 100);
    test_assert (retrieved->data.state == TX_RUNNING);

    txnt_close (t);
  }

  TEST_CASE ("different states")
  {
    error                e = error_create ();
    struct ns_txn_table *t = txnt_open (mem, &e);
    struct ns_txn        tx1, tx2, tx3;

    txn_init (
        &tx1,
        1,
        (struct ns_txn_data){.last_lsn = 10, .undo_next_lsn = 9, .state = TX_RUNNING},
        mem
    );
    txn_init (
        &tx2,
        2,
        (struct ns_txn_data){.last_lsn = 20, .undo_next_lsn = 19, .state = TX_CANDIDATE_FOR_UNDO},
        mem
    );
    txn_init (
        &tx3,
        3,
        (struct ns_txn_data){.last_lsn = 30, .undo_next_lsn = 29, .state = TX_COMMITTED},
        mem
    );

    txnt_insert_txn (t, &tx1);
    txnt_insert_txn (t, &tx2);
    txnt_insert_txn (t, &tx3);

    struct ns_txn *retrieved;

    test_assert (txnt_get (&retrieved, t, 1));
    test_assert (retrieved->data.state == TX_RUNNING);

    test_assert (txnt_get (&retrieved, t, 2));
    test_assert (retrieved->data.state == TX_CANDIDATE_FOR_UNDO);

    test_assert (txnt_get (&retrieved, t, 3));
    test_assert (retrieved->data.state == TX_COMMITTED);

    txnt_close (t);
  }
}
#endif

bool
txnt_get (struct ns_txn **dest, struct ns_txn_table *t, const txid tid)
{
  DBG_ASSERT (txn_table, t);

  struct ns_txn key;
  txn_key_init (&key, tid);

  latch_lock (&t->l);

  struct hnode **node = htable_lookup (t->t, &key.node, txn_equals_for_exists);
  if (node) {
    *dest = container_of (*node, struct ns_txn, node);
  }

  latch_unlock (&t->l);

  return node != NULL;
}

void
txnt_get_expect (struct ns_txn **dest, struct ns_txn_table *t, const txid tid)
{
  DBG_ASSERT (txn_table, t);

  struct ns_txn key;
  txn_key_init (&key, tid);

  latch_lock (&t->l);

  struct hnode **node = htable_lookup (t->t, &key.node, txn_equals_for_exists);
  ASSERT (node);
  *dest = container_of (*node, struct ns_txn, node);

  latch_unlock (&t->l);
}

#ifdef TESTING
TEST (txnt_get)
{
  TEST_CASE ("nonexistent returns false")
  {
    error                e = error_create ();
    struct ns_txn_table *t = txnt_open (mem, &e);
    struct ns_txn       *retrieved;
    bool                 found = txnt_get (&retrieved, t, 9999);
    test_assert (!found);

    txnt_close (t);
  }

  TEST_CASE ("and get tx running")
  {
    error                e = error_create ();
    struct ns_txn_table *t = txnt_open (mem, &e);
    struct ns_txn        tx;
    txn_init (
        &tx,
        100,
        (struct ns_txn_data){
            .last_lsn      = 50,
            .undo_next_lsn = 40,
            .state         = TX_RUNNING,
        },
        mem
    );

    txnt_insert_txn (t, &tx);

    struct ns_txn *retrieved;
    bool           found = txnt_get (&retrieved, t, 100);
    test_assert (found);
    test_assert (txn_data_equal_unsafe (&retrieved->data, &tx.data));

    txnt_close (t);
  }

  TEST_CASE ("and get tx candidate for undo")
  {
    error                e = error_create ();
    struct ns_txn_table *t = txnt_open (mem, &e);
    struct ns_txn        tx;
    txn_init (
        &tx,
        200,
        (struct ns_txn_data){
            .last_lsn      = 100,
            .undo_next_lsn = 90,
            .state         = TX_CANDIDATE_FOR_UNDO,
        },
        mem
    );

    txnt_insert_txn (t, &tx);

    struct ns_txn *retrieved;
    bool           found = txnt_get (&retrieved, t, 200);
    test_assert (found);
    test_assert (retrieved->tid == 200);
    test_assert (retrieved->data.last_lsn == 100);
    test_assert (retrieved->data.undo_next_lsn == 90);
    test_assert (retrieved->data.state == TX_CANDIDATE_FOR_UNDO);

    txnt_close (t);
  }

  TEST_CASE ("update last lsn")
  {
    error                e = error_create ();
    struct ns_txn_table *t = txnt_open (mem, &e);
    struct ns_txn        tx;
    txn_init (
        &tx,
        600,
        (struct ns_txn_data){
            .last_lsn      = 100,
            .undo_next_lsn = 90,
            .state         = TX_RUNNING,
        },
        mem
    );

    txnt_insert_txn (t, &tx);

    // Fetch, update, verify
    struct ns_txn *retrieved;
    bool           found = txnt_get (&retrieved, t, 600);
    test_assert (found);

    struct ns_txn_data new_data = retrieved->data;
    new_data.last_lsn           = 200;
    txn_update_data (retrieved, new_data);

    // Verify update
    found = txnt_get (&retrieved, t, 600);
    test_assert (found);
    test_assert (retrieved->data.last_lsn == 200);

    txnt_close (t);
  }

  TEST_CASE ("txnt state transitions all types")
  {
    error                e = error_create ();
    struct ns_txn_table *t = txnt_open (mem, &e);
    struct ns_txn        tx;
    txn_init (
        &tx,
        2000,
        (struct ns_txn_data){
            .last_lsn      = 100,
            .undo_next_lsn = 99,
            .state         = TX_RUNNING,
        },
        mem
    );

    txnt_insert_txn (t, &tx);

    struct ns_txn  key;
    struct ns_txn *retrieved = &key;

    test_assert (txnt_get (&retrieved, t, 2000));
    test_assert (retrieved->data.state == TX_RUNNING);

    // Transition to CANDIDATE_FOR_UNDO
    struct ns_txn_data new_data = retrieved->data;
    new_data.state              = TX_CANDIDATE_FOR_UNDO;
    txn_update_data (retrieved, new_data);

    test_assert (txnt_get (&retrieved, t, 2000));
    test_assert (retrieved->data.state == TX_CANDIDATE_FOR_UNDO);

    // Transition to COMMITTED
    new_data       = retrieved->data;
    new_data.state = TX_COMMITTED;
    txn_update_data (retrieved, new_data);

    test_assert (txnt_get (&retrieved, t, 2000));
    test_assert (retrieved->data.state == TX_COMMITTED);

    txnt_close (t);
  }
}
#endif

void
txnt_remove_txn (bool *exists, struct ns_txn_table *t, const struct ns_txn *tx)
{
  DBG_ASSERT (txn_table, t);

  latch_lock (&t->l);

  struct hnode **node = htable_lookup (t->t, &tx->node, txn_equals_for_exists);

  if (node == NULL) {
    *exists = false;
    goto theend;
  }

  *exists = true;

  htable_delete (t->t, node);

theend:
  latch_unlock (&t->l);
}

void
txnt_remove_txn_expect (struct ns_txn_table *t, const struct ns_txn *tx)
{
  DBG_ASSERT (txn_table, t);

  latch_lock (&t->l);

  struct hnode **node = htable_lookup (t->t, &tx->node, txn_equals_for_exists);

  ASSERT (node != NULL);

  htable_delete (t->t, node);

  latch_unlock (&t->l);
}

#ifdef TESTING
TEST (txnt_remove)
{
  TEST_CASE ("txnt_remove_existing_txn")
  {
    error                e = error_create ();
    struct ns_txn_table *t = txnt_open (mem, &e);
    struct ns_txn        tx;
    txn_init (
        &tx,
        400,
        (struct ns_txn_data){
            .last_lsn      = 100,
            .undo_next_lsn = 90,
            .state         = TX_RUNNING,
        },
        mem
    );

    txnt_insert_txn (t, &tx);

    bool removed;
    txnt_remove_txn (&removed, t, &tx);
    test_assert (removed);

    struct ns_txn *retrieved;
    bool           found = txnt_get (&retrieved, t, 400);
    test_assert (!found);

    txnt_close (t);
  }

  TEST_CASE ("txnt_remove_nonexistent_txn")
  {
    error                e = error_create ();
    struct ns_txn_table *t = txnt_open (mem, &e);
    struct ns_txn        tx;
    txn_key_init (&tx, 500);

    bool removed;
    txnt_remove_txn (&removed, t, &tx);
    test_assert (!removed);

    txnt_close (t);
  }

  TEST_CASE ("txnt_double_remove_same_transaction")
  {
    error                e = error_create ();
    struct ns_txn_table *t = txnt_open (mem, &e);
    struct ns_txn        tx;
    txn_init (
        &tx,
        100,
        (struct ns_txn_data){
            .last_lsn      = 50,
            .undo_next_lsn = 49,
            .state         = TX_RUNNING,
        },
        mem
    );

    txnt_insert_txn (t, &tx);

    bool removed;
    txnt_remove_txn (&removed, t, &tx);
    test_assert (removed);

    txnt_remove_txn (&removed, t, &tx);
    test_assert (!removed);

    txnt_close (t);
  }

  TEST_CASE ("txnt_operations_after_remove")
  {
    error                e = error_create ();
    struct ns_txn_table *t = txnt_open (mem, &e);
    struct ns_txn        tx;
    txn_init (
        &tx,
        200,
        (struct ns_txn_data){
            .last_lsn      = 50,
            .undo_next_lsn = 49,
            .state         = TX_RUNNING,
        },
        mem
    );

    txnt_insert_txn (t, &tx);

    bool removed;
    txnt_remove_txn (&removed, t, &tx);
    test_assert (removed);

    // Get should fail
    struct ns_txn *retrieved;
    bool           found = txnt_get (&retrieved, t, 200);
    test_assert (!found);

    txnt_close (t);
  }
}
#endif

struct ns_txnt_eq_ctx
{
  struct ns_txn_table *other;
  bool                 ret;
};

static void
txnt_eq_foreach (struct hnode *node, void *_ctx)
{
  struct ns_txnt_eq_ctx *ctx = _ctx;
  if ((int)ctx->ret == false) {
    return;
  }

  struct ns_txn *tx = container_of (node, struct ns_txn, node);
  struct ns_txn  candidate;

  latch_lock (&tx->l);
  {
    txn_key_init (&candidate, tx->tid);

    struct hnode **other_node = htable_lookup (
        ctx->other->t,
        &candidate.node,
        txn_equals_for_exists
    );

    if (other_node == NULL) {
      ctx->ret = false;
      latch_unlock (&tx->l);
      return;
    }

    struct ns_txn *other_tx = container_of (*other_node, struct ns_txn, node);

    latch_lock (&other_tx->l);
    {
      bool equal = true;

      equal      = ((equal && tx->data.last_lsn == other_tx->data.last_lsn) != 0);
      equal      = ((equal && tx->data.undo_next_lsn == other_tx->data.undo_next_lsn) != 0);

      ctx->ret   = equal;
    }
    latch_unlock (&other_tx->l);
  }
  latch_unlock (&tx->l);
}

bool
txnt_equal_ignore_state (struct ns_txn_table *left, struct ns_txn_table *right)
{
  latch_lock (&left->l);
  latch_lock (&right->l);

  if (htable_size (left->t) != htable_size (right->t)) {
    latch_unlock (&right->l);
    latch_unlock (&left->l);
    return false;
  }

  struct ns_txnt_eq_ctx ctx = {
      .other = right,
      .ret   = true,
  };
  htable_foreach (left->t, txnt_eq_foreach, &ctx);

  latch_unlock (&right->l);
  latch_unlock (&left->l);

  return ctx.ret;
}

#ifdef TESTING
TEST (txnt_equal_ignore_state)
{
  TEST_CASE ("txnt_equal_ignore_state_empty_tables")
  {
    error                e  = error_create ();
    struct ns_txn_table *t1 = txnt_open (mem, &e);
    struct ns_txn_table *t2 = txnt_open (mem, &e);
    test_assert (txnt_equal_ignore_state (t1, t2));

    txnt_close (t1);
    txnt_close (t2);
  }

  TEST_CASE ("txnt_equal_ignore_state_same_content")
  {
    error                e  = error_create ();
    struct ns_txn_table *t1 = txnt_open (mem, &e);
    struct ns_txn_table *t2 = txnt_open (mem, &e);
    struct ns_txn        t1_txns[5], t2_txns[5];
    for (int i = 0; i < 5; i++) {
      txn_init (
          &t1_txns[i],
          i + 1,
          (struct ns_txn_data){
              .last_lsn      = (i + 1) * 10,
              .undo_next_lsn = (i + 1) * 10 - 1,
              .state         = TX_RUNNING,
          },
          mem
      );
      txn_init (
          &t2_txns[i],
          i + 1,
          (struct ns_txn_data){
              .last_lsn      = (i + 1) * 10,
              .undo_next_lsn = (i + 1) * 10 - 1,
              .state         = TX_RUNNING,
          },
          mem
      );
      txnt_insert_txn (t1, &t1_txns[i]);
      txnt_insert_txn (t2, &t2_txns[i]);
    }

    test_assert (txnt_equal_ignore_state (t1, t2));

    txnt_close (t1);
    txnt_close (t2);
  }

  TEST_CASE ("txnt_not_equal_different_content")
  {
    error                e  = error_create ();
    struct ns_txn_table *t1 = txnt_open (mem, &e);
    struct ns_txn_table *t2 = txnt_open (mem, &e);
    struct ns_txn        tx1, tx2;
    txn_init (
        &tx1,
        1,
        (struct ns_txn_data){.last_lsn = 10, .undo_next_lsn = 9, .state = TX_RUNNING},
        mem
    );
    txn_init (
        &tx2,
        1,
        (struct ns_txn_data){.last_lsn = 20, .undo_next_lsn = 19, .state = TX_RUNNING},
        mem
    );

    txnt_insert_txn (t1, &tx1);
    txnt_insert_txn (t2, &tx2);

    test_assert (!txnt_equal_ignore_state (t1, t2));

    txnt_close (t1);
    txnt_close (t2);
  }
}
#endif

void
txnt_crash (struct ns_txn_table *t)
{
  DBG_ASSERT (txn_table, t);
  struct i_mem mem = t->mem;
  htable_free (t->t);
  i_free (mem, t);
}

#ifdef TESTING
#  include <stdatomic.h>

struct ns_txnt_thread_ctx
{
  struct ns_txn_table *table;
  struct ns_txn       *txn_bank; // Pre-allocated transactions
  _Atomic int          counter;
  txid                 start_tid;
  int                  count;
  struct i_mem         mem;
};

static void *
txnt_insert_thread (void *arg)
{
  struct ns_txnt_thread_ctx *ctx = arg;

  for (int i = 0; i < ctx->count; i++) {
    txn_init (
        &ctx->txn_bank[i],
        ctx->start_tid + i,
        (struct ns_txn_data){
            .last_lsn      = ctx->start_tid + i,
            .undo_next_lsn = ctx->start_tid + i - 1,
            .state         = TX_RUNNING,
        },
        ctx->mem
    );

    txnt_insert_txn (ctx->table, &ctx->txn_bank[i]);

    atomic_fetch_add (&ctx->counter, 1);
  }

  return NULL;
}

static void *
txnt_reader_thread (void *arg)
{
  struct ns_txnt_thread_ctx *ctx = arg;

  for (int i = 0; i < ctx->count; i++) {
    struct ns_txn *retrieved;
    if (txnt_get (&retrieved, ctx->table, ctx->start_tid + i)) {
      atomic_fetch_add (&ctx->counter, 1);
    }
  }

  return NULL;
}

static void *
txnt_updater_thread (void *arg)
{
  struct ns_txnt_thread_ctx *ctx = arg;

  for (int i = 0; i < ctx->count; i++) {
    struct ns_txn *retrieved;
    if (txnt_get (&retrieved, ctx->table, ctx->start_tid + i)) {
      struct ns_txn_data new_data = retrieved->data;
      new_data.last_lsn           = ctx->start_tid + i + 1000;
      txn_update_data (retrieved, new_data);
      atomic_fetch_add (&ctx->counter, 1);
    }
  }

  return NULL;
}

static void *
txnt_state_transition_thread (void *arg)
{
  struct ns_txnt_thread_ctx *ctx = arg;

  for (int i = 0; i < ctx->count; i++) {
    struct ns_txn *retrieved;
    if (txnt_get (&retrieved, ctx->table, ctx->start_tid + i)) {
      // TX_RUNNING -> TX_CANDIDATE_FOR_UNDO
      struct ns_txn_data new_data = retrieved->data;
      new_data.state              = TX_CANDIDATE_FOR_UNDO;
      txn_update_data (retrieved, new_data);

      atomic_fetch_add (&ctx->counter, 1);
      i_sleep_us (100);

      // -> TX_COMMITTED
      new_data.state = TX_COMMITTED;
      txn_update_data (retrieved, new_data);
    }
  }

  return NULL;
}

TEST (txnt_concurrent)
{
  TEST_CASE ("txnt_concurrent_inserts")
  {
    error                     e = error_create ();
    struct ns_txn_table      *t = txnt_open (mem, &e);
    struct ns_txn             txn_bank1[100], txn_bank2[100], txn_bank3[100];

    struct ns_txnt_thread_ctx ctx1 = {
        .table     = t,
        .txn_bank  = txn_bank1,
        .start_tid = 0,
        .count     = 100,
        .counter   = 0,
        .mem       = mem,
    };
    struct ns_txnt_thread_ctx ctx2 = {
        .table     = t,
        .txn_bank  = txn_bank2,
        .start_tid = 100,
        .count     = 100,
        .counter   = 0,
        .mem       = mem,
    };
    struct ns_txnt_thread_ctx ctx3 = {
        .table     = t,
        .txn_bank  = txn_bank3,
        .start_tid = 200,
        .count     = 100,
        .counter   = 0,
        .mem       = mem,
    };

    i_thread t1, t2, t3;
    test_assert_equal (
        i_thread_create (default_threading (), &t1, txnt_insert_thread, &ctx1, &e),
        SUCCESS
    );
    test_assert_equal (
        i_thread_create (default_threading (), &t2, txnt_insert_thread, &ctx2, &e),
        SUCCESS
    );
    test_assert_equal (
        i_thread_create (default_threading (), &t3, txnt_insert_thread, &ctx3, &e),
        SUCCESS
    );

    i_thread_join (default_threading (), &t1, &e);
    i_thread_join (default_threading (), &t2, &e);
    i_thread_join (default_threading (), &t3, &e);

    int total_inserts = ctx1.counter + ctx2.counter + ctx3.counter;
    test_assert_equal (total_inserts, 300);

    for (txid tid = 0; tid < 300; tid++) {
      test_assert (txn_exists (t, tid));
    }

    txnt_close (t);
  }

  TEST_CASE ("txnt_concurrent_readers")
  {
    error                e = error_create ();
    struct ns_txn_table *t = txnt_open (mem, &e);
    // Pre-populate
    struct ns_txn        txns[200];
    for (int i = 0; i < 200; i++) {
      txn_init (
          &txns[i],
          i,
          (struct ns_txn_data){
              .last_lsn      = i,
              .undo_next_lsn = i - 1,
              .state         = TX_RUNNING,
          },
          mem
      );
      txnt_insert_txn (t, &txns[i]);
    }

    struct ns_txnt_thread_ctx ctx1 = {
        .table     = t,
        .start_tid = 0,
        .count     = 100,
        .counter   = 0,
        .mem       = mem,
    };
    struct ns_txnt_thread_ctx ctx2 = {
        .table     = t,
        .start_tid = 50,
        .count     = 100,
        .counter   = 0,
        .mem       = mem,
    };
    struct ns_txnt_thread_ctx ctx3 = {
        .table     = t,
        .start_tid = 100,
        .count     = 100,
        .counter   = 0,
        .mem       = mem,
    };

    i_thread t1, t2, t3;
    test_assert_equal (
        i_thread_create (default_threading (), &t1, txnt_reader_thread, &ctx1, &e),
        SUCCESS
    );
    test_assert_equal (
        i_thread_create (default_threading (), &t2, txnt_reader_thread, &ctx2, &e),
        SUCCESS
    );
    test_assert_equal (
        i_thread_create (default_threading (), &t3, txnt_reader_thread, &ctx3, &e),
        SUCCESS
    );

    i_thread_join (default_threading (), &t1, &e);
    i_thread_join (default_threading (), &t2, &e);
    i_thread_join (default_threading (), &t3, &e);

    int total_reads = ctx1.counter + ctx2.counter + ctx3.counter;
    test_assert_equal (total_reads, 300);

    txnt_close (t);
  }

  TEST_CASE ("txnt_update_undo_next")
  {
    error                e = error_create ();
    struct ns_txn_table *t = txnt_open (mem, &e);
    struct ns_txn        tx;
    txn_init (
        &tx,
        700,
        (struct ns_txn_data){
            .last_lsn      = 100,
            .undo_next_lsn = 80,
            .state         = TX_RUNNING,
        },
        mem
    );

    txnt_insert_txn (t, &tx);

    struct ns_txn *retrieved;
    bool           found = txnt_get (&retrieved, t, 700);
    test_assert (found);

    struct ns_txn_data new_data = retrieved->data;
    new_data.undo_next_lsn      = 150;
    txn_update_data (retrieved, new_data);

    found = txnt_get (&retrieved, t, 700);
    test_assert (found);
    test_assert (retrieved->data.undo_next_lsn == 150);

    txnt_close (t);
  }

  TEST_CASE ("txnt_update_state")
  {
    error                e = error_create ();
    struct ns_txn_table *t = txnt_open (mem, &e);
    struct ns_txn        tx;
    txn_init (
        &tx,
        800,
        (struct ns_txn_data){
            .last_lsn      = 100,
            .undo_next_lsn = 90,
            .state         = TX_RUNNING,
        },
        mem
    );

    txnt_insert_txn (t, &tx);

    struct ns_txn *retrieved;
    bool           found = txnt_get (&retrieved, t, 800);
    test_assert (found);

    struct ns_txn_data new_data = retrieved->data;
    new_data.state              = TX_COMMITTED;
    txn_update_data (retrieved, new_data);

    found = txnt_get (&retrieved, t, 800);
    test_assert (found);
    test_assert (retrieved->data.state == TX_COMMITTED);

    txnt_close (t);
  }

  TEST_CASE ("txnt_concurrent_updates")
  {
    error                e = error_create ();
    struct ns_txn_table *t = txnt_open (mem, &e);
    // Pre-populate
    struct ns_txn        txns[300];
    for (int i = 0; i < 300; i++) {
      txn_init (
          &txns[i],
          i,
          (struct ns_txn_data){
              .last_lsn      = i,
              .undo_next_lsn = i - 1,
              .state         = TX_RUNNING,
          },
          mem
      );
      txnt_insert_txn (t, &txns[i]);
    }

    struct ns_txnt_thread_ctx ctx1 = {
        .table     = t,
        .start_tid = 0,
        .count     = 100,
        .counter   = 0,
        .mem       = mem,
    };
    struct ns_txnt_thread_ctx ctx2 = {
        .table     = t,
        .start_tid = 100,
        .count     = 100,
        .counter   = 0,
        .mem       = mem,
    };
    struct ns_txnt_thread_ctx ctx3 = {
        .table     = t,
        .start_tid = 200,
        .count     = 100,
        .counter   = 0,
        .mem       = mem,
    };

    i_thread t1, t2, t3;
    test_assert_equal (
        i_thread_create (default_threading (), &t1, txnt_updater_thread, &ctx1, &e),
        SUCCESS
    );
    test_assert_equal (
        i_thread_create (default_threading (), &t2, txnt_updater_thread, &ctx2, &e),
        SUCCESS
    );
    test_assert_equal (
        i_thread_create (default_threading (), &t3, txnt_updater_thread, &ctx3, &e),
        SUCCESS
    );

    i_thread_join (default_threading (), &t1, &e);
    i_thread_join (default_threading (), &t2, &e);
    i_thread_join (default_threading (), &t3, &e);

    int total_updates = ctx1.counter + ctx2.counter + ctx3.counter;
    test_assert_equal (total_updates, 300);

    // Verify updates
    for (txid tid = 0; tid < 300; tid++) {
      struct ns_txn *retrieved;
      test_assert (txnt_get (&retrieved, t, tid));
      test_assert_equal (retrieved->data.last_lsn, tid + 1000);
    }

    txnt_close (t);
  }

  TEST_CASE ("txnt_concurrent_state_transitions")
  {
    error                e = error_create ();
    struct ns_txn_table *t = txnt_open (mem, &e);
    // Pre-populate with running transactions
    struct ns_txn        txns[150];
    for (int i = 0; i < 150; i++) {
      txn_init (
          &txns[i],
          i,
          (struct ns_txn_data){
              .last_lsn      = i,
              .undo_next_lsn = i - 1,
              .state         = TX_RUNNING,
          },
          mem
      );
      txnt_insert_txn (t, &txns[i]);
    }

    struct ns_txnt_thread_ctx ctx1 = {
        .table     = t,
        .start_tid = 0,
        .count     = 50,
        .counter   = 0,
        .mem       = mem,
    };
    struct ns_txnt_thread_ctx ctx2 = {
        .table     = t,
        .start_tid = 50,
        .count     = 50,
        .counter   = 0,
        .mem       = mem,
    };
    struct ns_txnt_thread_ctx ctx3 = {
        .table     = t,
        .start_tid = 100,
        .count     = 50,
        .counter   = 0,
        .mem       = mem,
    };

    i_thread t1, t2, t3;
    test_assert_equal (
        i_thread_create (default_threading (), &t1, txnt_state_transition_thread, &ctx1, &e),
        SUCCESS
    );
    test_assert_equal (
        i_thread_create (default_threading (), &t2, txnt_state_transition_thread, &ctx2, &e),
        SUCCESS
    );
    test_assert_equal (
        i_thread_create (default_threading (), &t3, txnt_state_transition_thread, &ctx3, &e),
        SUCCESS
    );

    i_thread_join (default_threading (), &t1, &e);
    i_thread_join (default_threading (), &t2, &e);
    i_thread_join (default_threading (), &t3, &e);

    int total_transitions = ctx1.counter + ctx2.counter + ctx3.counter;
    test_assert_equal (total_transitions, 150);

    // Verify all are committed
    for (txid tid = 0; tid < 150; tid++) {
      struct ns_txn *retrieved;
      test_assert (txnt_get (&retrieved, t, tid));
      test_assert_equal (retrieved->data.state, TX_COMMITTED);
    }

    txnt_close (t);
  }
}

#endif
