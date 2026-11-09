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

#ifndef NS_TXN_H
#define NS_TXN_H

#include "core/ns_csx_assert.h"
#include "core/ns_error.h" // error
#include "core/ns_htable.h"
#include "core/ns_latch.h"
#include "core/ns_slab_alloc.h"
#include "core/ns_stdtypes.h"
#include "nscore/lock_table/ns_lock_table.h"

#include <stdbool.h>

enum tx_state
{
  TX_RUNNING,
  TX_CANDIDATE_FOR_UNDO,
  TX_COMMITTED,
  TX_DONE,
};

struct txn_data
{
  enum tx_state state;
  lsn           min_lsn;
  lsn           last_lsn;
  lsn           undo_next_lsn;
};

struct txn_lock
{
  struct lt_lock   lock;
  enum lock_mode   mode;
  struct txn_lock *next;
};

struct txn
{
  txid              tid;
  struct txn_data   data;
  struct hnode      node;
  struct txn_lock  *locks;
  struct slab_alloc lock_alloc;
  latch             l;
};

DEFINE_DBG_ASSERT (struct txn, ns_txn, t, {
  ASSERT (t);

  /**
   * min_lsn <= undo_next_lsn <= last_lsn
   *
   * or undo_next_lsn = 0
   */

  ASSERT (t->data.last_lsn >= t->data.min_lsn);
  ASSERT (t->data.last_lsn >= t->data.undo_next_lsn);

  if (t->data.undo_next_lsn != 0) {
    ASSERT (t->data.undo_next_lsn >= t->data.min_lsn);
  }
})

void txn_init (struct txn *dest, txid tid, struct txn_data data, struct i_mem mem);
void txn_key_init (struct txn *dest, txid tid);
void txn_update_data (struct txn *t, struct txn_data data);
void txn_update (struct txn *t, enum tx_state state, lsn last, lsn undo_next);
void txn_update_state (struct txn *t, enum tx_state new_state);
bool txn_data_equal_unsafe (const struct txn_data *left, const struct txn_data *right);

typedef void (*lock_func) (struct lt_lock lock, enum lock_mode mode, void *ctx);
err_t txn_newlock (struct txn *t, struct lt_lock lock, enum lock_mode mode, error *e);
struct txn_lock *txn_get_lock_unsafe (const struct txn *t, const struct lt_lock lock);
void txn_close (struct txn *t);
void txn_foreach_lock (struct txn *t, lock_func func, void *ctx);

HEADER_FUNC bool
txn_haslock_unsafe (const struct txn *t, const struct lt_lock lock)
{
  return txn_get_lock_unsafe (t, lock) != NULL;
}

HEADER_FUNC bool
txn_haslock (struct txn *t, const struct lt_lock lock)
{
  latch_lock (&t->l);
  bool ret = txn_get_lock_unsafe (t, lock) != NULL;
  latch_unlock (&t->l);
  return ret;
}

#endif
