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
/**
 * @file
 * @brief Rope algorithms
 *
 * Various numstore rope algorithms
 */

#ifndef ROPE_H
#define ROPE_H

#include "node_updates.h" // node_updates
#include "numstore.h"     // pgno
#include "pager.h"        // page_h
#include "pager.h"        // pager

/******************************************************************************
 * SECTION: Validation
 * ----------------------------------------------------------------------------
 * @brief Validating a rope tree to ensure the database is not corrupt
 *
 * This is a WIP
 ******************************************************************************/

/**
 * @fn err_t ns_rptree_valid(struct pager *p, pgno rpt_root, b_size nbytes,
 * error *e)
 * @brief Validates the structural integrity of an R+Tree's repeat-index tree.
 */
err_t ns_rptree_valid (struct pager *p, pgno rpt_root, b_size nbytes, error *e);

/******************************************************************************
 * SECTION: Core R+Tree Algorithms
 * ----------------------------------------------------------------------------
 * @brief Algorithms for updating a rope+tree - primarily insert read write and
 * delete
 *
 * These are the core algorithms - other algorithms often build on top
 * of these
 ******************************************************************************/

/**
 * @struct ns_insert_params
 * @brief Parameters for ns_insert().
 *
 * Inserts [nelem] elements of [size] bytes each from [src] into the R+Tree
 * rooted at [root], starting at byte offset [bofst].  When nelem == 0, bytes
 * are consumed from [src] until it is exhausted (unlimited insert).
 *
 * [root] is updated in place if the tree root changes (e.g. because the root
 * was split into a new level).
 *
 * @var ns_insert_params::p
 * @brief The database
 *
 * @var ns_insert_params::src
 * @brief Source stream to read inserted bytes from
 *
 * @var ns_insert_params::tx
 * @brief Transaction to attach this mutation to
 *
 * @var ns_insert_params::root
 * @brief Root page of the target tree (updated in place if root changes)
 *
 * @var ns_insert_params::bofst
 * @brief Byte offset at which to begin inserting
 *
 * @var ns_insert_params::bytes
 * @brief Number of bytes to insert (0 = consume src until exhausted)
 *
 */
struct ns_insert_params
{
  struct pager  *p;
  struct stream *src;
  struct txn    *tx;
  pgno           root;
  b_size         bofst;
  b_size         bytes;
};

/**
 * @struct ns_write_params
 * @brief Parameters for ns_write() — in-place overwrite of elements in an
 * R+Tree
 *
 * @var ns_write_params::p
 * @brief The database
 *
 * @var ns_write_params::src
 * @brief Source stream to read replacement bytes from
 *
 * @var ns_write_params::tx
 * @brief Transaction to attach this mutation to
 *
 * @var ns_write_params::root
 * @brief Root page of the target tree
 *
 * @var ns_write_params::size
 * @brief Size of each element in bytes
 *
 * @var ns_write_params::bofst
 * @brief Byte offset at which to begin writing
 *
 * @var ns_write_params::stride
 * @brief Bytes to advance between successive element writes (1 = contiguous)
 *
 * @var ns_write_params::nelem
 * @brief Number of elements to write
 *
 */
struct ns_write_params
{
  struct pager  *p;
  struct stream *src;
  struct txn    *tx;
  pgno           root;
  t_size         size;
  b_size         bofst;
  sb_size        stride;
  b_size         nelem;
};

/**
 * @struct ns_read_params
 * @brief Parameters for ns_read() — element retrieval from an R+Tree into a
 * stream
 *
 * @var ns_read_params::p
 * @brief The database
 *
 * @var ns_read_params::dest
 * @brief Destination stream to push read bytes into
 *
 * @var ns_read_params::tx
 * @brief Transaction to attach this read to
 *
 * @var ns_read_params::root
 * @brief Root page of the source tree
 *
 * @var ns_read_params::size
 * @brief Size of each element in bytes
 *
 * @var ns_read_params::bofst
 * @brief Byte offset at which to begin reading
 *
 * @var ns_read_params::stride
 * @brief Bytes to advance between successive element reads (1 = contiguous)
 *
 * @var ns_read_params::nelem
 * @brief Number of elements to read
 *
 */
struct ns_read_params
{
  struct pager  *p;
  struct stream *dest;
  struct txn    *tx;
  pgno           root;
  t_size         size;
  b_size         bofst;
  sb_size        stride;
  b_size         nelem;
};

/**
 * @struct ns_remove_params
 * @brief Parameters for ns_remove() — element deletion and optional capture
 * from an R+Tree
 *
 * @var ns_remove_params::p
 * @brief The database
 *
 * @var ns_remove_params::dest
 * @brief Optional stream to capture removed bytes before deletion (NULL to
 *
 * discard)
 * @var ns_remove_params::tx
 * @brief Transaction to attach this mutation to
 *
 * @var ns_remove_params::root
 * @brief Root page of the target tree (updated in place if root changes)
 *
 * @var ns_remove_params::size
 * @brief Size of each element in bytes
 *
 * @var ns_remove_params::bofst
 * @brief Byte offset at which to begin removing
 *
 * @var ns_remove_params::stride
 * @brief Bytes to advance between successive element removals (1 = contiguous)
 *
 * @var ns_remove_params::nelem
 * @brief Number of elements to remove
 *
 */
struct ns_remove_params
{
  struct pager  *p;
  struct stream *dest;
  struct txn    *tx;
  pgno           root;
  b_size         size;
  b_size         bofst;
  sb_size        stride;
  b_size         nelem;
};

/**
 * @fn sb_size ns_insert(struct ns_insert_params *params, error *e)
 * @brief Inserts elements into an R+Tree; returns bytes inserted or a negative
 * error code
 */
sb_size ns_insert (struct ns_insert_params *params, error *e);

/**
 * @fn sb_size ns_write(struct ns_write_params params, error *e)
 * @brief Overwrites elements in an R+Tree; returns bytes written or a negative
 * error code
 */
sb_size ns_write (struct ns_write_params params, error *e);

/**
 * @fn sb_size ns_read(struct ns_read_params params, error *e)
 * @brief Reads elements from an R+Tree; returns bytes read or a negative error
 * code
 */
sb_size ns_read (struct ns_read_params params, error *e);

/**
 * @fn sb_size ns_remove(struct ns_remove_params *params, error *e)
 * @brief Removes elements from an R+Tree; returns bytes removed or a negative
 * error code
 */
sb_size ns_remove (struct ns_remove_params *params, error *e);

/******************************************************************************
 * SECTION: Seek
 * ----------------------------------------------------------------------------
 * @brief Primary seek algorithm for locating a byte in the rope + tree
 ******************************************************************************/

/**
 * @struct seek_v
 * @brief A single entry in the seek stack, identifying a page and a local index
 * within it
 *
 * @var seek_v::pg
 * @brief Handle to the page (held in read mode)
 *
 * @var seek_v::lidx
 * @brief Local byte index within the page
 *
 */
struct seek_v
{
  page_h pg;
  p_size lidx;
};

/**
 * @struct ns_seek_params
 * @brief Parameters for ns_seek().
 *
 * Traverses the R+Tree from root to the data-list page containing [bofst].
 * On return, [pg] holds the data-list page in read mode and [lidx] is the
 * local byte index within that page where [bofst] lands.
 *
 * If [save_stack] is true, each inner node visited during the descent is
 * saved into [pstack] (in read mode) rather than released.  This gives
 * ns_rebalance() a pre-loaded path to the root without a second traversal.
 * The caller is responsible for releasing all pages in pstack[0...sp-1] on
 * the success path (or cancelling them on the error path).
 *
 * The stack depth is bounded at 20 levels; an R+Tree with IN_MAX_KEYS
 * children per node can index far more data than any practical storage device
 * before depth 20, so this bound is effectively unreachable.
 *
 * @var ns_seek_params::p
 * @brief The database
 *
 * @var ns_seek_params::tx
 * @brief Transaction to attach this traversal to
 *
 * @var ns_seek_params::root
 * @brief Root page of the tree to seek into
 *
 * @var ns_seek_params::bofst
 * @brief Byte offset to locate
 *
 * @var ns_seek_params::save_stack
 * @brief If true, inner nodes are retained in pstack rather than released
 *
 * @var ns_seek_params::pstack
 * @brief Outputs: Inner nodes visited during descent (valid if save_stack is
 *
 * true)
 * @var ns_seek_params::sp
 * @brief Outputs: Number of valid entries in pstack
 *
 * @var ns_seek_params::pg
 * @brief Outputs: Resulting data-list page, held in read mode (PHM_S)
 *
 * @var ns_seek_params::lidx
 * @brief Outputs: Byte offset within pg where bofst lands
 *
 */
struct ns_seek_params
{
  struct pager *p;
  struct txn   *tx;
  pgno          root;
  b_size        bofst;
  bool          save_stack;
  struct seek_v pstack[20];
  u32           sp;
  page_h        pg;
  p_size        lidx;
};

/**
 * @fn err_t ns_seek(struct ns_seek_params *a, error *e)
 * @brief Traverses the R+Tree to the data-list page containing bofst
 */
err_t ns_seek (struct ns_seek_params *a, error *e);

/******************************************************************************
 * SECTION: Rebalance
 * ----------------------------------------------------------------------------
 * @brief Rebalance algorithm is used for insert and remove
 *
 * Makes heavy use of node updates
 ******************************************************************************/

/**
 * @struct root_update
 * @brief Carries an updated root page number and a flag indicating whether it
 * is the tree root
 *
 * @var root_update::root
 * @brief The (possibly new) root page number
 *
 * @var root_update::isroot
 * @brief True if this page is now the tree root
 *
 */
struct root_update
{
  pgno root;
  bool isroot;
};

/**
 * @struct ns_balance_and_release_params
 * @brief Parameters for ns_balance_and_release()
 *
 * @var ns_balance_and_release_params::p
 * @brief The database
 *
 * @var ns_balance_and_release_params::tx
 * @brief Transaction to attach mutations to
 *
 * @var ns_balance_and_release_params::output
 * @brief Receives the resulting (prev, cur, next) in_pairs for nupd accounting
 *
 * @var ns_balance_and_release_params::root
 * @brief Updated with the new root if a merge reduces tree height
 *
 * @var ns_balance_and_release_params::prev
 * @brief Left sibling page (may be absent but must be non-NULL)
 *
 * @var ns_balance_and_release_params::cur
 * @brief The leaf page to balance (must be loaded)
 *
 * @var ns_balance_and_release_params::next
 * @brief Right sibling page (may be absent but must be non-NULL)
 *
 */
struct ns_balance_and_release_params
{
  struct pager         *p;
  struct txn           *tx;
  struct three_in_pair *output;
  struct root_update   *root;
  page_h               *prev;
  page_h               *cur;
  page_h               *next;
};

/**
 * @fn err_t ns_balance_and_release(struct ns_balance_and_release_params params,
 * error *e)
 * @brief Balance the leaf page cur against its siblings and release all three.
 *
 * Called after every data-level mutation (insert, remove) to enforce the
 * invariant that every non-root data-list page holds at least maxlen/2 bytes.
 *
 * next, prev don't necessarily need to be loaded. This algorithm will
 * minimize the number of page loads - they can be absent but non null and
 * the algorithm will check for existing pages first
 *
 * On success, prev, cur, and next are all released.  The caller obtains the
 * resulting (prev, cur, next) in_pairs via params.output for nupd accounting.
 */
err_t
ns_balance_and_release (struct ns_balance_and_release_params params, error *e);

/**
 * @struct ns_rebalance_params
 * @brief Parameters for the main bottom-up rebalance pass after an insert or
 * remove
 *
 * @var ns_rebalance_params::p
 * @brief The database
 *
 * @var ns_rebalance_params::tx
 * @brief Transaction to attach mutations to
 *
 * @var ns_rebalance_params::root
 * @brief Root page of the tree being rebalanced
 *
 * @var ns_rebalance_params::pstack
 * @brief Stack of inner-node pages saved during the preceding seek
 *
 * @var ns_rebalance_params::sp
 * @brief Number of valid entries in pstack
 *
 * @var ns_rebalance_params::input
 * @brief Update set fed into this rebalance layer (swaps with output each
 * iteration)
 *
 * @var ns_rebalance_params::output
 * @brief Update set produced by this rebalance layer
 *
 * @var ns_rebalance_params::layer_root
 * @brief Carries the root update if this layer collapses to a new root
 *
 * @var ns_rebalance_params::cur
 * @brief Stateful working variables: Current inner-node page being processed
 *
 * @var ns_rebalance_params::limit
 * @brief Stateful working variables: Sentinel page marking the end of the
 * current layer
 *
 * @var ns_rebalance_params::lidx
 * @brief Stateful working variables: Local index within cur being updated
 */
struct ns_rebalance_params
{
  struct pager        *p;
  struct txn          *tx;
  pgno                 root;
  struct seek_v       *pstack;
  u32                  sp;
  struct node_updates *input;
  struct node_updates *output;
  struct root_update   layer_root;
  page_h               cur;
  page_h               limit;
  p_size               lidx;
};

/**
 * @fn err_t ns_rebalance(struct ns_rebalance_params *params, error *e)
 * @brief Propagates structural updates bottom-up through the inner nodes of the
 * R+Tree
 */
err_t ns_rebalance (struct ns_rebalance_params *params, error *e);
#endif // ROPE_H
