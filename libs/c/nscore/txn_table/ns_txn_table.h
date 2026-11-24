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

#ifndef TXN_TABLE_H
#define TXN_TABLE_H

#include "ns_dbl_buffer.h"
#include "ns_error.h"
#include "ns_slab_alloc.h"
#include "ns_stdtypes.h"
#include "txn_table/ns_txn.h"

#include <stdbool.h>

struct txn_table
{
  struct i_mem   mem;
  latch          l;
  struct htable *t;

  bool           isfrozen;
};

DEFINE_DBG_ASSERT (struct txn_table, txn_table, t, { ASSERT (t); })

///////////////////////
///// Lifecycle
struct txn_table *txnt_open (struct i_mem mem, error *e);
void txnt_close (struct txn_table *t);
void txnt_crash (struct txn_table *t);

///////////////////////
///// Getters
slsn txnt_max_u_undo_lsn (struct txn_table *t);
u32 txnt_get_size (const struct txn_table *dest);
slsn txnt_min_lsn (struct txn_table *t);

///////////////////////
///// Utilties
typedef void (*txnt_action) (struct txn *t, void *ctx);
void txnt_foreach (const struct txn_table *t, txnt_action action, void *ctx);
bool txnt_equal_ignore_state (struct txn_table *left, struct txn_table *right);
err_t txnt_merge_into (
    struct txn_table  *dest,
    struct txn_table  *src,
    struct dbl_buffer *txn_dest,
    struct slab_alloc *alloc,
    struct i_mem       mem,
    error             *e
);

///////////////////////
///// Main API
bool txn_exists (const struct txn_table *t, txid tid);
void txnt_insert_txn (struct txn_table *t, struct txn *tx);
void txnt_insert_txn_if_not_exists (struct txn_table *t, struct txn *tx);
bool txnt_get (struct txn **dest, struct txn_table *t, txid tid);
void txnt_get_expect (struct txn **dest, struct txn_table *t, txid tid);
void txnt_remove_txn (bool *exists, struct txn_table *t, const struct txn *tx);
void txnt_remove_txn_expect (struct txn_table *t, const struct txn *unsafe_tx);

#endif // TXN_TABLE_H
