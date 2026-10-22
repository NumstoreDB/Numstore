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

#include "core/ns_concurrency.h"
#include "core/ns_csx_assert.h"
#include "core/ns_dbl_buffer.h"
#include "core/ns_error.h" // error
#include "core/ns_htable.h"
#include "core/ns_slab_alloc.h"
#include "core/ns_stdtypes.h"
#include "nscore/lock_table/ns_lock_table.h"

#include <stdbool.h>

struct dbl_buffer;

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

/*-----------------------------------------------------------------------------
 * SUBSECTION: Lifecycle Initialization
 *----------------------------------------------------------------------------*/

/**
 * @fn void txn_init(struct txn *dest, txid tid, struct txn_data data, struct
 * i_mem mem)
 * @brief Allocates and maps tracking fields to complete initialization loops.
 */
void txn_init (struct txn *dest, txid tid, struct txn_data data, struct i_mem mem);

/**
 * @fn void txn_key_init(struct txn *dest, txid tid)
 * @brief Allocates tracking context maps matching explicit keys alone.
 */
void txn_key_init (struct txn *dest, txid tid);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Atomic Updates
 *----------------------------------------------------------------------------*/

/**
 * @fn void txn_update_data(struct txn *t, struct txn_data data)
 * @brief Swaps the complete underlying internal telemetry block in-place.
 */
void txn_update_data (struct txn *t, struct txn_data data);

/**
 * @fn void txn_update(struct txn *t, enum tx_state state, lsn last, lsn
 * undo_next)
 * @brief Comprehensive mutation field overlay editing active properties.
 */
void txn_update (struct txn *t, enum tx_state state, lsn last, lsn undo_next);

/**
 * @fn void txn_update_state(struct txn *t, enum tx_state new_state)
 * @brief Single field updates altering the tracked ARIES lifecycle path.
 */
void txn_update_state (struct txn *t, enum tx_state new_state);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Equality Evaluation
 *----------------------------------------------------------------------------*/

/**
 * @fn bool txn_data_equal_unsafe(const struct txn_data *left, const struct
 * txn_data *right)
 * @brief Direct memory validation check evaluating differences without
 * acquiring latches.
 */
bool txn_data_equal_unsafe (const struct txn_data *left, const struct txn_data *right);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Lock Management
 *----------------------------------------------------------------------------*/

/**
 * @typedef lock_func
 * @brief Iteration callback tracking items allocated out of internal
 * transaction tables.
 */
typedef void (*lock_func) (struct lt_lock lock, enum lock_mode mode, void *ctx);

/**
 * @fn err_t txn_newlock(struct txn *t, struct lt_lock lock, enum lock_mode
 * mode, error *e)
 * @brief Attaches a newly tracked lock resource allocation context container.
 */
err_t txn_newlock (struct txn *t, struct lt_lock lock, enum lock_mode mode, error *e);

/**
 * @fn bool txn_haslock(struct txn *t, struct lt_lock lock)
 * @brief Validation lookup checking whether target identifiers are actively
 * retained.
 */
bool txn_haslock (struct txn *t, struct lt_lock lock);

/**
 * @fn void txn_close(struct txn *t)
 * @brief Deallocates memory spaces and releases all tracking properties.
 */
void txn_close (struct txn *t);

/**
 * @fn void txn_foreach_lock(struct txn *t, lock_func func, void *ctx)
 * @brief Loops over the complete active list of managed resource descriptors.
 */
void txn_foreach_lock (struct txn *t, lock_func func, void *ctx);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Utilities
 *----------------------------------------------------------------------------*/

/******************************************************************************
 * SECTION: Active Transaction Table (ATT) Type Definition
 * ----------------------------------------------------------------------------
 * @brief The ATT is an intrusive hash table keyed by txid.  It tracks every
 * transaction that is currently in flight or that was active at the time of
 * the last crash.  The pager holds the authoritative ATT; a second, ephemeral
 * ATT is constructed inside aries_ctx during recovery and discarded when
 * recovery completes.
 *
 * The ATT has two primary uses:
 * 1. Undo navigation during pgr_rollback() and the ARIES undo phase:
 * txnt_max_u_undo_lsn() returns the highest undo_next_lsn among all
 * TX_CANDIDATE_FOR_UNDO entries, driving the undo loop.
 * 2. WAL truncation: txnt_min_lsn() returns the lowest min_lsn in the
 * table; WAL records before this LSN are not needed by any active
 * transaction and can be reclaimed at checkpoint time.
 *
 * Fuzzy checkpoint serialization requires a consistent snapshot of the ATT
 * while new log records may still be arriving.  txnt_freeze_active_txns_for_
 * serialization() locks the table and individual transaction latches until
 * each transaction has been serialized into the checkpoint END record.
 ******************************************************************************/

struct txn_table;

/*-----------------------------------------------------------------------------
 * SUBSECTION: Lifecycle
 *----------------------------------------------------------------------------*/

/**
 * @fn struct txn_table *txnt_open(struct i_mem mem, error *e)
 * @brief Instantiates and prepares the active transaction table context.
 */
struct txn_table *txnt_open (struct i_mem mem, error *e);

/**
 * @fn void txnt_close(struct txn_table *t)
 * @brief Tears down the transaction table context and releases held handles.
 */
void txnt_close (struct txn_table *t);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Locking / Unlocking and Serializing
 * ----------------------------------------------------------------------------
 * @brief For fuzzy checkpoints, manually locking is needed for the
 * transaction table while serializing the transaction.
 *
 * The basic pattern is:
 *
 * lock(table.txns);
 *
 * for each txn in table:
 * do something with txn
 * unlock(txn);
 *
 * Nothing else to do - all txns are unlocked.
 *
 * Under the hood, for now this just locks the entire txn table until the
 * number of unlocks is equal to the number of txns, but in the future
 * this can be optimized.
 *----------------------------------------------------------------------------*/

/**
 * @fn slsn txnt_max_u_undo_lsn(struct txn_table *t)
 * @brief Returns the maximum undo lsn for any entries in the candidate for
 * undo state.
 *
 * This record is used in the undo phase of recovery.
 *
 * We start from the maximum because during undo we want to read backwards
 * linearly to avoid random access file movement. This method is called over
 * and over.
 *
 * (Later optimization - keep a list of these internally so that I don't
 * re search every time I run this - this method is run in a loop)
 */
slsn txnt_max_u_undo_lsn (struct txn_table *t);

/**
 * @fn slsn txnt_min_lsn(struct txn_table *t)
 * @brief Returns the transaction low water mark.
 *
 * The lowest lsn in the transaction table is the lowest lsn we need to
 * remain active. Anything under that lsn can be deleted.
 *
 * If the transaction table is empty, this method returns -1.
 */
slsn txnt_min_lsn (struct txn_table *t);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Iteration & State Queries
 *----------------------------------------------------------------------------*/

/**
 * @fn void txnt_foreach(const struct txn_table *t, void (*action)(struct txn *,
 * void *ctx), void *ctx)
 * @brief Executes a function on each open transaction. This does not lock
 * internal transactions.
 */
void txnt_foreach (const struct txn_table *t, void (*action) (struct txn *, void *ctx), void *ctx);

/**
 * @fn u32 txnt_get_size(const struct txn_table *dest)
 * @brief Returns the number of transactions active.
 */
u32 txnt_get_size (const struct txn_table *dest);

/**
 * @fn bool txn_exists(const struct txn_table *t, txid tid)
 * @brief Fast path exists check.
 */
bool txn_exists (const struct txn_table *t, txid tid);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Table Merging
 *----------------------------------------------------------------------------*/

/**
 * @fn err_t txnt_merge_into(struct txn_table *dest, struct txn_table *src,
 * struct dbl_buffer *txn_dest, struct slab_alloc *alloc, struct i_mem mem,
 * error *e)
 * @brief Merges txn table [src] into [dest]
 *
 * Duplicate strategy:
 * - On duplicate key, it skips the key. Therefore:
 * 1. If dest already has tid in src, dest stays the source of truth
 *
 * If txn_dest or alloc are null, it doesn't copy transactions over - they
 * stay managed by whatever mechanism managed [src]'s memory.
 */
err_t txnt_merge_into (
    struct txn_table  *dest,
    struct txn_table  *src,
    struct dbl_buffer *txn_dest,
    struct slab_alloc *alloc,
    struct i_mem       mem,
    error             *e
);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Table Mutations (Insert, Get, Remove)
 *----------------------------------------------------------------------------*/

/**
 * @fn void txnt_insert_txn(struct txn_table *t, struct txn *tx)
 * @brief Unconditionally maps an active transaction instance into the table
 * layout.
 */
void txnt_insert_txn (struct txn_table *t, struct txn *tx);

/**
 * @fn void txnt_insert_txn_if_not_exists(struct txn_table *t, struct txn *tx)
 * @brief Checks for preexisting identifiers before executing safe table
 * insertions.
 */
void txnt_insert_txn_if_not_exists (struct txn_table *t, struct txn *tx);

/**
 * @fn bool txnt_get(struct txn **dest, struct txn_table *t, txid tid)
 * @brief Searches the hash map key structure and extracts matching references
 * if present.
 */
bool txnt_get (struct txn **dest, struct txn_table *t, txid tid);

/**
 * @fn void txnt_get_expect(struct txn **dest, struct txn_table *t, txid tid)
 * @brief Asserts that the requested transaction id is actively tracked in the
 * table maps.
 */
void txnt_get_expect (struct txn **dest, struct txn_table *t, txid tid);

/**
 * @fn void txnt_remove_txn(bool *exists, struct txn_table *t, const struct txn
 * *tx)
 * @brief Evicts target tracking details and passes safety check metrics back.
 */
void txnt_remove_txn (bool *exists, struct txn_table *t, const struct txn *tx);

/**
 * @fn void txnt_remove_txn_expect(struct txn_table *t, const struct txn
 * *unsafe_tx)
 * @brief Evicts target tracking details, asserting that the record was found.
 */
void txnt_remove_txn_expect (struct txn_table *t, const struct txn *unsafe_tx);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Test & Validation Helpers
 *----------------------------------------------------------------------------*/

/**
 * @fn bool txnt_equal_ignore_state(struct txn_table *left, struct txn_table
 * *right)
 * @brief Checks structural data parity across two tables ignoring active
 * lifecycle markers.
 */
bool txnt_equal_ignore_state (struct txn_table *left, struct txn_table *right);

/**
 * @fn void txnt_crash(struct txn_table *t)
 * @brief Simulates an ungraceful toolchain crash boundary discarding ephemeral
 * structures.
 */
void txnt_crash (struct txn_table *t);

#endif // TXN_TABLE_H
