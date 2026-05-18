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

#include "c_specx.h"
#include "numstore/node_updates.h"
#include "numstore/page_h.h"
#include "numstore/pager.h"
#include "numstore/pages/page.h"
#include "numstore/rope.h"

/*
 * R+Tree Inner-Node Rebalancing
 *
 * After an insert, write, or remove operation changes the byte count of one
 * or more leaf pages, the inner-node key array at every level of the tree
 * must be updated to reflect the new sizes.  If a leaf-level operation also
 * adds or removes data-list pages, the corresponding inner-node entries must
 * be inserted or deleted, which may cause inner nodes to overflow (>
 * IN_MAX_KEYS entries) or underflow (< IN_MIN_KEYS entries).  Overflow
 * requires splitting a node; underflow requires merging with a sibling or
 * borrowing entries from one.
 *
 * This is coordinated through a node_updates (nupd) object.  The bottom of
 * the stack populates a nupd that describes what changed at the leaf level:
 * which pages were added/removed and what their new byte counts are.  The
 * rebalancer walks up the inner-node stack and, at each level, applies the
 * nupd to the corresponding inner-node page.  This produces a new nupd
 * describing what changed at that level, which is then applied to the level
 * above, and so on until the root is reached.
 *
 * NOTATION USED IN INLINE DIAGRAMS
 * ---------------------------------
 *   + : an existing (valid) inner-node entry
 *   o : an entry that has been "observed" (logically consumed by nupd; the
 *       page it references has been accounted for but not yet written back)
 *   _ : a logically empty slot (has room for a new entry)
 *   - : a slot that is physically present but logically empty (will be
 *       overwritten before the node is released)
 *
 * RIGHT vs LEFT EXECUTION
 * -----------------------
 * When the nupd has changes to the right of the pivot (new pages inserted
 * after the current position), rb_execute_right() walks forward through the
 * sibling chain, consuming observed entries and emitting compacted ones.
 * rb_execute_left() does the symmetric thing for changes to the left.
 *
 * MOVE-UP TRANSITION
 * ------------------
 * ns_rebalance_move_up_stack() pops one inner-node level off the seek stack,
 * applies the accumulated nupd to the popped node via
 * ns_rebalance_apply_to_pivot(), then swaps input/output nupd buffers for
 * the next level.  If the popped level becomes the new root (isroot is set),
 * all remaining stack levels above it are deleted and the root pgno is
 * updated.
 */

// Key:
// +: An existing inner node page / key
// o: An observed inner node page / key (effectively deleted)
// _: An empty spot for inner node page / key
// -: A "logically empty" spot but the node might say it's occupied

/*
 * Delete an inner-node page and every page in its sibling chain in both
 * directions.
 *
 * Called when a rebalance determines that an entire level of inner nodes has
 * been collapsed into a single new root below it.  All the now-obsolete
 * sibling pages must be freed so their slots return to the FSM.
 */
static err_t in_delete_chain (page_h *cur, struct txn *tx, struct pager *p, error *e) {
  page_h next_next = page_h_create ();
  page_h next      = page_h_create ();
  page_h prev      = page_h_create ();
  page_h prev_prev = page_h_create ();

  pgno npg = in_get_next (page_h_ro (cur));
  if (npg != PGNO_NULL) {
    if (pgr_get_writable (&next, tx, PG_INNER_NODE, npg, p, e)) { goto failed; }
  }

  pgno ppg = in_get_prev (page_h_ro (cur));
  if (ppg != PGNO_NULL) {
    if (pgr_get_writable (&prev, tx, PG_INNER_NODE, ppg, p, e)) { goto failed; }
  }

  if (pgr_delete_and_release (p, tx, cur, e)) { goto failed; }

  while (next.mode != PHM_NONE) {
    npg = in_get_next (page_h_ro (&next));
    if (npg != PGNO_NULL) {
      if (pgr_get_writable (&next_next, tx, PG_INNER_NODE, npg, p, e)) { goto failed; }
    }
    if (pgr_delete_and_release (p, tx, &next, e)) { goto failed; }
    page_h_xfer_ownership_ptr (&next, &next_next);
  }

  while (prev.mode != PHM_NONE) {
    ppg = in_get_prev (page_h_ro (&prev));
    if (ppg != PGNO_NULL) {
      if (pgr_get_writable (&prev_prev, tx, PG_INNER_NODE, ppg, p, e)) { goto failed; }
    }
    if (pgr_delete_and_release (p, tx, &prev, e)) { goto failed; }
    page_h_xfer_ownership_ptr (&prev, &prev_prev);
  }

  return SUCCESS;

failed:
  pgr_cancel_if_exists (p, cur);
  pgr_cancel_if_exists (p, &prev);
  pgr_cancel_if_exists (p, &next);
  pgr_cancel_if_exists (p, &prev_prev);
  pgr_cancel_if_exists (p, &next_next);

  return error_trace (e);
}

static err_t rb_right_to_left (struct ns_rebalance_params *pms, error *e) {
  struct root_update   root;
  struct three_in_pair tip_out;
  page_h               prev = page_h_create ();
  page_h               next = page_h_xfer_ownership (&pms->limit);

  if (nupd_done_left (pms->input)) {
    const struct ns_balance_and_release_params bparams = {
        .p  = pms->p,
        .tx = pms->tx,

        .output = &tip_out,
        .root   = &root,

        .prev = &prev,
        .cur  = &pms->cur,
        .next = &next,
    };
    if (ns_balance_and_release (bparams, e)) { goto failed; }
    if (nupd_append_tip_right (pms->output, tip_out, e)) { goto failed; }
    pms->layer_root = root;
    return SUCCESS;
  }

  // We go left, then right - so you never need to go right again
  UNREACHABLE ();

failed:
  pgr_cancel_if_exists (pms->p, &prev);
  pgr_cancel_if_exists (pms->p, &next);
  return error_trace (e);
}

static err_t rb_left_to_right (struct ns_rebalance_params *pms, error *e) {
  struct root_update   root;
  struct three_in_pair tip_out;
  page_h               prev = page_h_xfer_ownership (&pms->limit);
  page_h               next = page_h_create ();

  // Fully done
  if (nupd_done_right (pms->input)) {
    const struct ns_balance_and_release_params bparams = {
        .p  = pms->p,
        .tx = pms->tx,

        .output = &tip_out,
        .root   = &root,

        .prev = &prev,
        .cur  = &pms->cur,
        .next = &next,
    };
    if (ns_balance_and_release (bparams, e)) { goto failed; }

    if (nupd_append_tip_left (pms->output, tip_out, e)) { goto failed; }
    pms->layer_root = root;

    return SUCCESS;
  }

  // If cur == pivot, we don't need to rebalance - we can just start left
  if (page_h_pgno (&pms->cur) != nupd_pivot_pg (pms->output)) {
    // Rebalance
    const struct ns_balance_and_release_params bparams = {
        .p  = pms->p,
        .tx = pms->tx,

        .output = &tip_out,
        .root   = &root,

        .prev = &prev,
        .cur  = &pms->cur,
        .next = &next,
    };
    if (ns_balance_and_release (bparams, e)) { goto failed; }
    if (nupd_append_tip_left (pms->output, tip_out, e)) { goto failed; }

    // Fetch pivot
    const pgno pivot = nupd_pivot_pg (pms->output);
    if (pgr_get_writable (&pms->cur, pms->tx, PG_INNER_NODE, pivot, pms->p, e)) { goto failed; }
  } else {
    if (pgr_release_if_exists (pms->p, &prev, PG_INNER_NODE, e)) { goto failed; }
  }

  ASSERT (prev.mode == PHM_NONE);
  ASSERT (pms->cur.mode == PHM_X);
  ASSERT (next.mode == PHM_NONE);

  pms->lidx = in_get_len (page_h_ro (&pms->cur));
  in_set_len (page_h_w (&pms->cur), IN_MAX_KEYS);

  const pgno npg = in_get_next (page_h_ro (&pms->cur));
  if (npg != PGNO_NULL && pms->limit.mode == PHM_NONE) {
    if (pgr_get_writable (&pms->limit, pms->tx, PG_INNER_NODE, npg, pms->p, e)) { goto failed; }
  }

  return SUCCESS;

failed:
  pgr_cancel_if_exists (pms->p, &prev);
  pgr_cancel_if_exists (pms->p, &next);
  return error_trace (e);
}

static err_t rb_execute_right (struct ns_rebalance_params *pms, error *e) {
  page_h next      = page_h_create ();
  page_h next_next = page_h_create ();

  while (true) {
    // [+++++++++++_______]
    // ^
    // lidx
    // [a, b, c] p [d, e, f, g, h, i]
    // ^     ^        ^
    // rcons  rlen     robs
    if (nupd_done_observing_right (pms->input)) {
      pms->lidx += nupd_append_maximally_right (pms->input, &pms->cur, pms->lidx);

      // [++++++++++++++++++]
      // ^
      // lidx
      // [a, b, c] p [d, e, f, g, h, i]
      // ^  ^      ^
      // rlen rcons  robs
      // rcons didn't reach robs. That can
      // only happen if we filled up current
      // node
      if (!nupd_done_right (pms->input)) {
        ASSERT (pms->lidx == IN_MAX_KEYS);

        if (nupd_commit_1st_right (
                pms->output,
                page_h_pgno (&pms->cur),
                in_get_size (page_h_ro (&pms->cur)),
                e)) {
          goto failed;
        }

        // cur -> limit
        // cur -> new -> limit
        // new -> limit
        if (pgr_new (&next, pms->p, pms->tx, PG_INNER_NODE, e)) { goto failed; }
        in_link (page_h_w (&pms->cur), page_h_w (&next));
        in_link (page_h_w (&next), page_h_w_or_null (&pms->limit));
        if (pgr_release (pms->p, &pms->cur, PG_INNER_NODE, e)) { goto failed; }
        page_h_xfer_ownership_ptr (&pms->cur, &next);

        in_set_len (page_h_w (&pms->cur), IN_MAX_KEYS);

        pms->lidx = 0;

        continue;
      } else {
        // [++++++++----------]
        // ^
        // lidx
        // [++++++++__________]
        in_set_len (page_h_w (&pms->cur), pms->lidx);
        return rb_right_to_left (pms, e);
      }
    }

    // [+++++++++++_______]
    // ^
    // lidx
    // [a, b, c] p [d, e, f, g, h, i]
    // ^     ^        ^
    // rcons  robs     rlen
    else {
      if (nupd_observe_all_right (pms->input, &pms->limit, e)) { goto failed; }
      pms->lidx += nupd_append_maximally_right (pms->input, &pms->cur, pms->lidx);

      // [++++++++++++______]
      // ^
      // lidx
      // [a, b, c] p [d, e, f, g, h, i]
      // ^        ^  ^
      // rcons    robs rlen
      if (!nupd_done_right (pms->input) && pms->lidx > IN_MAX_KEYS / 2) {
        // Shift right (limit
        // is effectively
        // "empty" because it
        // was observed so we
        // can use it as a slot
        // for next) cur ->
        // NULL cur
        // -> limit limit limit
        // -> next
        if (pms->limit.mode == PHM_NONE) {
          if (pgr_new (&pms->limit, pms->p, pms->tx, PG_INNER_NODE, e)) { goto failed; }
          in_link (page_h_w (&pms->cur), page_h_w (&pms->limit));
        }

        // cur -> limit
        // limit
        // limit -> next

        // [++++++++----------]
        // ^
        // lidx
        // [++++++++__________]
        // ^
        // lidx
        in_set_len (page_h_w (&pms->cur), pms->lidx);

        if (nupd_commit_1st_right (
                pms->output,
                page_h_pgno (&pms->cur),
                in_get_size (page_h_ro (&pms->cur)),
                e)) {
          goto failed;
        }

        if (pgr_release (pms->p, &pms->cur, PG_INNER_NODE, e)) { goto failed; }

        // cur = limit
        page_h_xfer_ownership_ptr (&pms->cur, &pms->limit);

        // Open cur for writes
        in_set_len (page_h_w (&pms->cur), IN_MAX_KEYS);
        pms->lidx = 0;

        // Shift right
        const pgno npg = in_get_next (page_h_ro (&pms->cur));
        if (npg != PGNO_NULL && pms->limit.mode == PHM_NONE) {
          if (pgr_get_writable (&pms->limit, pms->tx, PG_INNER_NODE, npg, pms->p, e)) {
            goto failed;
          }
        }
      }

      // [++++++____________]
      // ^
      // lidx
      // [a, b, c] p [d, e, f, g, h, i]
      // ^        ^  ^
      // rcons    robs rlen
      // OR
      // Node could be done:
      // TODO - (18) Maybe optimize this out
      // - right now there's an extra page
      // load [a, b, c] p [d, e, f, g, h, i]
      // ^  ^
      // rlen robs
      // rcons
      else {
        ASSERT (nupd_done_consuming_right (pms->input));
        ASSERT (nupd_done_right (pms->input) || pms->limit.mode != PHM_NONE);

        if (pms->limit.mode != PHM_NONE) {
          const pgno npg = page_h_pgno (&pms->limit);

          const pgno nnpg = in_get_next (page_h_ro (&pms->limit));
          if (nnpg != PGNO_NULL) {
            if (pgr_get_writable (&next_next, pms->tx, PG_INNER_NODE, nnpg, pms->p, e)) {
              goto failed;
            }
          }
          if (pgr_delete_and_release (pms->p, pms->tx, &pms->limit, e)) { goto failed; }
          in_link (page_h_w (&pms->cur), page_h_w_or_null (&next_next));
          page_h_xfer_ownership_ptr (&pms->limit, &next_next);

          if (nupd_append_2nd_right (pms->output, pgh_unravel (&pms->cur), npg, 0, e)) {
            goto failed;
          }
        }
      }
    }
  }

failed:
  pgr_cancel_if_exists (pms->p, &next);
  pgr_cancel_if_exists (pms->p, &next_next);
  return error_trace (e);
}

static err_t rb_execute_left (struct ns_rebalance_params *pms, error *e) {
  page_h prev      = page_h_create ();
  page_h prev_prev = page_h_create ();

  while (true) {
    // [_______+++++++++++]
    // ^
    // lidx
    // [a, b, c, d, e, f] p [g, h, i]
    // ^        ^     ^
    // lobs     llen   lcons
    if (nupd_done_observing_left (pms->input)) {
      pms->lidx -= nupd_append_maximally_left (pms->input, &pms->cur, pms->lidx);

      // [++++++++++++++++++]
      // ^
      // lidx
      // [a, b, c, d, e, f] p [g, h, i]
      // ^     ^  ^
      // lobs lcons llen
      // lcons didn't reach lobs. That can
      // only happen if we filled up current
      // node
      if (!nupd_done_left (pms->input)) {
        ASSERT (pms->lidx == 0);
        if (nupd_commit_1st_left (
                pms->output,
                page_h_pgno (&pms->cur),
                in_get_size (page_h_ro (&pms->cur)),
                e)) {
          goto failed;
        }

        // limit <- cur
        // limit <- new <- cur
        // limit <- new
        if (pgr_new (&prev, pms->p, pms->tx, PG_INNER_NODE, e)) { goto failed; }
        in_link (page_h_w_or_null (&pms->limit), page_h_w (&prev));
        in_link (page_h_w (&prev), page_h_w (&pms->cur));
        if (pgr_release (pms->p, &pms->cur, PG_INNER_NODE, e)) { goto failed; }
        pms->cur = page_h_xfer_ownership (&prev);

        in_set_len (page_h_w (&pms->cur), IN_MAX_KEYS);
        pms->lidx = IN_MAX_KEYS;

        continue;
      } else {
        // [----------++++++++]
        // ^
        // lidx
        // [++++++++__________]
        in_cut_left (page_h_w (&pms->cur), pms->lidx);
        pms->lidx = in_get_len (page_h_ro (&pms->cur));
        return rb_left_to_right (pms, e);
      }
    }

    // [_______+++++++++++]
    // ^
    // lidx
    // [a, b, c, d, e, f] p [g, h, i]
    // ^        ^     ^
    // llen     lobs  lcons
    else {
      if (nupd_observe_all_left (pms->input, &pms->limit, e)) { goto failed; }
      pms->lidx -= nupd_append_maximally_left (pms->input, &pms->cur, pms->lidx);

      // [_______+++++++++++]
      // ^
      // lidx
      // [a, b, c, d, e, f] p [g, h, i]
      // ^  ^        ^
      // llen lobs   lcons
      if (!nupd_done_left (pms->input) && (IN_MAX_KEYS - pms->lidx) > IN_MAX_KEYS / 2) {
        // Shift left (limit is
        // effectively "empty"
        // because it was
        // observed so we can
        // use it as a slot for
        // next) NULL <- cur
        // limit -> cur
        // limit
        // prev <- limit
        if (pms->limit.mode == PHM_NONE) {
          if (pgr_new (&pms->limit, pms->p, pms->tx, PG_INNER_NODE, e)) { goto failed; }
          in_link (page_h_w (&pms->limit), page_h_w (&pms->cur));
        }

        // limit <- cur
        // limit
        // prev <- limit

        // [----------++++++++]
        // ^
        // lidx
        // [++++++++__________]
        in_cut_left (page_h_w (&pms->cur), pms->lidx);
        if (nupd_commit_1st_left (
                pms->output,
                page_h_pgno (&pms->cur),
                in_get_size (page_h_ro (&pms->cur)),
                e)) {
          goto failed;
        }

        if (pgr_release (pms->p, &pms->cur, PG_INNER_NODE, e)) { goto failed; }

        // cur = limit
        page_h_xfer_ownership_ptr (&pms->cur, &pms->limit);

        // Open cur for writes
        in_set_len (page_h_w (&pms->cur), IN_MAX_KEYS);
        pms->lidx = IN_MAX_KEYS;

        // Shift left
        const pgno ppg = in_get_prev (page_h_ro (&pms->cur));
        if (ppg != PGNO_NULL && pms->limit.mode == PHM_NONE) {
          if (pgr_get_writable (&pms->limit, pms->tx, PG_INNER_NODE, ppg, pms->p, e)) {
            goto failed;
          }
        }
      }

      // [___________+++++++]
      // ^
      // lidx
      // [a, b, c, d, e, f] p [g, h, i]
      // ^  ^        ^
      // llen lobs   lcons
      // OR
      // Node could be done:
      // TODO - (18) Maybe optimize this out
      // - right now there's an extra page
      // load [a, b, c, d, e, f] p [g, h, i]
      // ^  ^
      // lobs llen
      // lcons
      else {
        ASSERT (nupd_done_consuming_left (pms->input));
        ASSERT (nupd_done_left (pms->input) || pms->limit.mode != PHM_NONE);

        if (pms->limit.mode != PHM_NONE) {
          const pgno ppg = page_h_pgno (&pms->limit);

          const pgno pppg = in_get_prev (page_h_ro (&pms->limit));
          if (pppg != PGNO_NULL) {
            if (pgr_get_writable (&prev_prev, pms->tx, PG_INNER_NODE, pppg, pms->p, e)) {
              goto failed;
            }
          }
          if (pgr_delete_and_release (pms->p, pms->tx, &pms->limit, e)) { goto failed; }
          in_link (page_h_w_or_null (&prev_prev), page_h_w (&pms->cur));
          page_h_xfer_ownership_ptr (&pms->limit, &prev_prev);

          if (nupd_append_2nd_left (pms->output, pgh_unravel (&pms->cur), ppg, 0, e)) {
            goto failed;
          }
        }
      }
    }
  }

failed:
  pgr_cancel_if_exists (pms->p, &prev);
  pgr_cancel_if_exists (pms->p, &prev_prev);
  return error_trace (e);
}

static err_t ns_pop_stack (struct ns_rebalance_params *pms, error *e) {
  struct seek_v *ref = &pms->pstack[--(pms->sp)];

  struct seek_v v = {
      .pg   = page_h_xfer_ownership (&ref->pg),
      .lidx = ref->lidx,
  };

  if (pms->cur.mode != PHM_NONE) {
    if (pgr_release (pms->p, &pms->cur, PG_INNER_NODE | PG_DATA_LIST, e)) { goto failed; }
  }

  pms->cur  = page_h_xfer_ownership (&v.pg);
  pms->lidx = v.lidx;

  return SUCCESS;

failed:
  pgr_cancel_if_exists (pms->p, &v.pg);
  return error_trace (e);
}

static err_t ns_rebalance_move_up_stack (struct ns_rebalance_params *pms, error *e);

static err_t ns_rebalance_apply_to_pivot (struct ns_rebalance_params *pms, error *e) {
  page_h prev = page_h_create ();
  page_h next = page_h_create ();

  // Output is empty
  /**
  ASSERT (pms->output->lcons == 0);
  ASSERT (pms->output->llen == 0);
  ASSERT (pms->output->lobs == 0);
  ASSERT (pms->output->rcons == 0);
  ASSERT (pms->output->rlen == 0);
  ASSERT (pms->output->rcons == 0);
  ASSERT (pms->output->pivot.pg == page_h_pgno (&pms->cur));
  ASSERT (pms->output->pivot.key == in_get_size (page_h_ro
  (&pms->cur)));

  // Input is not consumed
  ASSERT (pms->input->lcons == 0);
  ASSERT (pms->input->lobs == 0);
  ASSERT (pms->input->rcons == 0);
  ASSERT (pms->input->robs == 0);
  if (in_get_len (page_h_ro (&pms->cur)) > 0)
  {
  ASSERT (pms->input->pivot.pg == in_get_leaf (page_h_ro (&pms->cur),
  pms->lidx));
  }
  */

  if (nupd_observe_pivot (pms->input, &pms->cur, pms->lidx, e)) { goto failed; }
  in_set_len (page_h_w (&pms->cur), IN_MAX_KEYS);

  // ----------> Append right
  // [++++++++++++++------]
  // Shift Right
  // [------++++++++++++++]
  // <--- Append Left
  // [--++++++++++++++++++]
  // ^
  // lidx
  // Continue in left mode
  pms->lidx = IN_MAX_KEYS - nupd_append_maximally_right_then_left (pms->input, &pms->cur);

  if (nupd_done_left (pms->input)) {
    // [++++++++++++++++++__]
    // ^
    // lidx
    in_cut_left (page_h_w (&pms->cur), pms->lidx);
    pms->lidx = IN_MAX_KEYS - pms->lidx;

    // DONE EARLY
    if (nupd_done_right (pms->input)) {
      struct three_in_pair tip_out;

      const struct ns_balance_and_release_params bparams = {
          .p  = pms->p,
          .tx = pms->tx,

          .output = &tip_out,
          .root   = &pms->layer_root,

          .prev = &prev,
          .cur  = &pms->cur,
          .next = &next,
      };
      if (ns_balance_and_release (bparams, e)) { goto failed; }
      if (nupd_append_tip_right (pms->output, tip_out, e)) { goto failed; }

      return ns_rebalance_move_up_stack (pms, e);
    }

    // Open up for right updates
    // [++++++++++++++++++--]
    // ^
    // lidx
    in_set_len (page_h_w (&pms->cur), IN_MAX_KEYS);

    // Right mode in read mode
    const pgno next_pg = in_get_next (page_h_ro (&pms->cur));
    if (next_pg != PGNO_NULL) {
      if (pgr_get_writable (&pms->limit, pms->tx, PG_INNER_NODE, next_pg, pms->p, e)) {
        goto failed;
      }
    }

    return SUCCESS;
  }

  // Left mode
  const pgno prev_pg = in_get_prev (page_h_ro (&pms->cur));
  if (prev_pg != PGNO_NULL) {
    if (pgr_get (&pms->limit, PG_INNER_NODE, in_get_prev (page_h_ro (&pms->cur)), pms->p, e)) {
      goto failed;
    }
  }

  return SUCCESS;

failed:
  pgr_cancel_if_exists (pms->p, &prev);
  pgr_cancel_if_exists (pms->p, &next);
  return error_trace (e);
}

static err_t ns_rebalance_move_up_stack (struct ns_rebalance_params *pms, error *e) {
  if (pms->layer_root.isroot) {
    // Delete all the next layers above
    while (pms->sp != 0) {
      if (ns_pop_stack (pms, e)) { goto failed; }
      if (in_delete_chain (&pms->cur, pms->tx, pms->p, e)) { goto failed; }
    }

    pms->lidx = 0;
    pms->root = pms->layer_root.root;

    return SUCCESS;
  } else {
    if (pms->sp == 0) {
      if (pgr_new (&pms->cur, pms->p, pms->tx, PG_INNER_NODE, e)) { goto failed; }

      pms->root = page_h_pgno (&pms->cur);
      pms->lidx = 0;
    } else {
      if (ns_pop_stack (pms, e)) { goto failed; }
    }

    // Swap node updates
    struct node_updates *input  = pms->input;
    struct node_updates *output = pms->output;
    pms->output                 = input;
    pms->input                  = output;

    if (pms->output == NULL) {
      pms->output = nupd_init (page_h_pgno (&pms->cur), in_get_size (page_h_ro (&pms->cur)), e);
      if (pms->output == NULL) { goto failed; }
    } else {
      nupd_reset (pms->output, page_h_pgno (&pms->cur), in_get_size (page_h_ro (&pms->cur)));
    }

    return ns_rebalance_apply_to_pivot (pms, e);
  }

failed:
  return error_trace (e);
}

/*
 * Propagate size changes and structural updates up the inner-node stack.
 *
 * Outer loop: pop one level at a time from the seek stack.  For each level,
 * call ns_rebalance_move_up_stack() which loads the inner-node page from the
 * stack and applies the current input nupd via ns_rebalance_apply_to_pivot().
 *
 * After apply_to_pivot, the current input nupd may still have unconsumed left
 * or right updates (entries that need to move to sibling nodes).  These are
 * handled by rb_execute_left() and rb_execute_right() respectively, which
 * walk the sibling chain and pack or unpack entries until the nupd is fully
 * consumed.  Each execution function produces an output nupd describing what
 * changed at this level, which becomes the input nupd for the level above.
 *
 * When the layer_root is set (the current level is the tree root), the loop
 * exits and any obsolete levels above are deleted by the move_up function.
 */
err_t ns_rebalance (struct ns_rebalance_params *pms, error *e) {
  pms->cur   = page_h_create ();
  pms->limit = page_h_create ();
  pms->lidx  = 0;

  while (true) {
    if (ns_rebalance_move_up_stack (pms, e)) { goto failed; }

    // Pop up the stack once
    if (pms->layer_root.isroot) { return error_trace (e); }

    bool done = true;

    // Execute left
    if (!nupd_done_left (pms->input)) {
      done = false;
      if (rb_execute_left (pms, e)) { goto failed; }
    }

    // Execute right
    if (!nupd_done_right (pms->input)) {
      done = false;
      if (rb_execute_right (pms, e)) { goto failed; }
    }

    if (done) { return SUCCESS; }
  }

failed:
  pgr_cancel_if_exists (pms->p, &pms->cur);
  pgr_cancel_if_exists (pms->p, &pms->limit);
  return error_trace (e);
}
