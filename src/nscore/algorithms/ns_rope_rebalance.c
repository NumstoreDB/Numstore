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

#include "error.h"
#include "node_updates.h"
#include "numerics.h"
#include "numstore.h"
#include "os.h"
#include "page.h"
#include "page_fixture.h"
#include "page_h.h"
#include "pager.h"
#include "rope_algorithms.h"
#include "serial.h"
#include "testing.h"

/******************************************************************************
 * SECTION: ns_rebalance
 * ----------------------------------------------------------------------------
 * @brief Main rebalance algorithm for ropes
 ******************************************************************************/

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
static err_t
in_delete_chain (page_h *cur, struct txn *tx, struct pager *p, error *e)
{
  page_h next_next = page_h_create ();
  page_h next      = page_h_create ();
  page_h prev      = page_h_create ();
  page_h prev_prev = page_h_create ();

  pgno npg = in_get_next (page_h_ro (cur));
  if (npg != PGNO_NULL)
  {
    if (pgr_get_writable (&next, tx, PG_INNER_NODE, npg, p, e))
    {
      goto failed;
    }
  }

  pgno ppg = in_get_prev (page_h_ro (cur));
  if (ppg != PGNO_NULL)
  {
    if (pgr_get_writable (&prev, tx, PG_INNER_NODE, ppg, p, e))
    {
      goto failed;
    }
  }

  if (pgr_delete_and_release (p, tx, cur, e))
  {
    goto failed;
  }

  while (next.mode != PHM_NONE)
  {
    npg = in_get_next (page_h_ro (&next));
    if (npg != PGNO_NULL)
    {
      if (pgr_get_writable (&next_next, tx, PG_INNER_NODE, npg, p, e))
      {
        goto failed;
      }
    }
    if (pgr_delete_and_release (p, tx, &next, e))
    {
      goto failed;
    }
    page_h_xfer_ownership_ptr (&next, &next_next);
  }

  while (prev.mode != PHM_NONE)
  {
    ppg = in_get_prev (page_h_ro (&prev));
    if (ppg != PGNO_NULL)
    {
      if (pgr_get_writable (&prev_prev, tx, PG_INNER_NODE, ppg, p, e))
      {
        goto failed;
      }
    }
    if (pgr_delete_and_release (p, tx, &prev, e))
    {
      goto failed;
    }
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

static err_t
rb_right_to_left (struct ns_rebalance_params *pms, error *e)
{
  struct root_update   root;
  struct three_in_pair tip_out;
  page_h               prev = page_h_create ();
  page_h               next = page_h_xfer_ownership (&pms->limit);

  if (nupd_done_left (pms->input))
  {
    const struct ns_balance_and_release_params bparams = {
        .p  = pms->p,
        .tx = pms->tx,

        .output = &tip_out,
        .root   = &root,

        .prev = &prev,
        .cur  = &pms->cur,
        .next = &next,
    };
    if (ns_balance_and_release (bparams, e))
    {
      goto failed;
    }
    if (nupd_append_tip_right (pms->output, tip_out, e))
    {
      goto failed;
    }
    pms->layer_root = root;
    return SUCCESS;
  }

  // We go left, then right - so you never need to go right again
  UNREACHABLE (); // LCOV_EXCL_LINE

failed:
  pgr_cancel_if_exists (pms->p, &prev);
  pgr_cancel_if_exists (pms->p, &next);
  return error_trace (e);
}

static err_t
rb_left_to_right (struct ns_rebalance_params *pms, error *e)
{
  struct root_update   root;
  struct three_in_pair tip_out;
  page_h               prev = page_h_xfer_ownership (&pms->limit);
  page_h               next = page_h_create ();

  // Fully done
  if (nupd_done_right (pms->input))
  {
    const struct ns_balance_and_release_params bparams = {
        .p  = pms->p,
        .tx = pms->tx,

        .output = &tip_out,
        .root   = &root,

        .prev = &prev,
        .cur  = &pms->cur,
        .next = &next,
    };
    if (ns_balance_and_release (bparams, e))
    {
      goto failed;
    }

    if (nupd_append_tip_left (pms->output, tip_out, e))
    {
      goto failed;
    }
    pms->layer_root = root;

    return SUCCESS;
  }

  // If cur == pivot, we don't need to rebalance - we can just start left
  if (page_h_pgno (&pms->cur) != nupd_pivot_pg (pms->output))
  {
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
    if (ns_balance_and_release (bparams, e))
    {
      goto failed;
    }
    if (nupd_append_tip_left (pms->output, tip_out, e))
    {
      goto failed;
    }

    // Fetch pivot
    const pgno pivot = nupd_pivot_pg (pms->output);
    if (pgr_get_writable (&pms->cur, pms->tx, PG_INNER_NODE, pivot, pms->p, e))
    {
      goto failed;
    }
  }
  else
  {
    if (pgr_release_if_exists (pms->p, &prev, PG_INNER_NODE, e))
    {
      goto failed;
    }
  }

  ASSERT (prev.mode == PHM_NONE);
  ASSERT (pms->cur.mode == PHM_X);
  ASSERT (next.mode == PHM_NONE);

  pms->lidx = in_get_len (page_h_ro (&pms->cur));
  in_set_len (page_h_w (&pms->cur), IN_MAX_KEYS);

  const pgno npg = in_get_next (page_h_ro (&pms->cur));
  if (npg != PGNO_NULL && pms->limit.mode == PHM_NONE)
  {
    if (pgr_get_writable (&pms->limit, pms->tx, PG_INNER_NODE, npg, pms->p, e))
    {
      goto failed;
    }
  }

  return SUCCESS;

failed:
  pgr_cancel_if_exists (pms->p, &prev);
  pgr_cancel_if_exists (pms->p, &next);
  return error_trace (e);
}

static err_t
rb_execute_right (struct ns_rebalance_params *pms, error *e)
{
  page_h next      = page_h_create ();
  page_h next_next = page_h_create ();

  while (true)
  {
    // [+++++++++++_______]
    //             ^
    //            lidx
    // [a, b, c] p [d, e, f, g, h, i]
    //              ^     ^        ^
    //            rcons  rlen     robs
    if (nupd_done_observing_right (pms->input))
    {
      pms->lidx +=
          nupd_append_maximally_right (pms->input, &pms->cur, pms->lidx);

      // [++++++++++++++++++]
      //                    ^
      //                  lidx
      // [a, b, c] p [d, e, f, g, h, i]
      //                   ^  ^      ^
      //                 rlen rcons  robs
      //
      // rcons didn't reach robs. That can
      // only happen if we filled up current
      // node
      if (!nupd_done_right (pms->input))
      {
        TEST_MARK ("rebalance:right:done_observing:not_done_consuming");

        ASSERT (pms->lidx == IN_MAX_KEYS);

        if (nupd_commit_1st_right (
                pms->output,
                page_h_pgno (&pms->cur),
                in_get_size (page_h_ro (&pms->cur)),
                e
            ))
        {
          goto failed;
        }

        // cur -> limit
        // cur -> new -> limit
        // new -> limit
        if (pgr_new (&next, pms->p, pms->tx, PG_INNER_NODE, e))
        {
          goto failed;
        }

        in_link (page_h_w (&pms->cur), page_h_w (&next));
        in_link (page_h_w (&next), page_h_w_or_null (&pms->limit));

        if (pgr_release (pms->p, &pms->cur, PG_INNER_NODE, e))
        {
          goto failed;
        }
        page_h_xfer_ownership_ptr (&pms->cur, &next);

        in_set_len (page_h_w (&pms->cur), IN_MAX_KEYS);

        pms->lidx = 0;

        continue;
      }
      else
      {
        // [++++++++----------]
        //          ^
        //         lidx
        // [++++++++__________]
        in_set_len (page_h_w (&pms->cur), pms->lidx);
        return rb_right_to_left (pms, e);
      }
    }

    // [+++++++++++_______]
    //             ^
    //            lidx
    // [a, b, c] p [d, e, f, g, h, i]
    //              ^     ^        ^
    //            rcons  robs     rlen
    else
    {
      TEST_MARK ("rebalance:right:not_done_observing");

      if (nupd_observe_all_right (pms->input, &pms->limit, e))
      {
        goto failed;
      }
      pms->lidx +=
          nupd_append_maximally_right (pms->input, &pms->cur, pms->lidx);

      // [++++++++++++______]
      //              ^
      //            lidx
      // [a, b, c] p [d, e, f, g, h, i]
      //                 ^        ^  ^
      //               rcons    robs rlen
      if (!nupd_done_right (pms->input) && pms->lidx > IN_MAX_KEYS / 2)
      {
        TEST_MARK ("rebalance:right:not_done_observing:not_done:still_shift");

        // Shift right (limit is effectively "empty" because it
        // was observed so we can use it as a slot for next)
        // cur -> NULL
        // cur -> limit
        // limit
        // limit -> next
        if (pms->limit.mode == PHM_NONE)
        {
          if (pgr_new (&pms->limit, pms->p, pms->tx, PG_INNER_NODE, e))
          {
            goto failed;
          }
          in_link (page_h_w (&pms->cur), page_h_w (&pms->limit));
        }

        // cur -> limit
        // limit
        // limit -> next

        // [++++++++----------]
        //          ^
        //         lidx
        // [++++++++__________]
        //          ^
        //        lidx
        in_set_len (page_h_w (&pms->cur), pms->lidx);

        if (nupd_commit_1st_right (
                pms->output,
                page_h_pgno (&pms->cur),
                in_get_size (page_h_ro (&pms->cur)),
                e
            ))
        {
          goto failed;
        }

        if (pgr_release (pms->p, &pms->cur, PG_INNER_NODE, e))
        {
          goto failed;
        }

        // cur = limit
        page_h_xfer_ownership_ptr (&pms->cur, &pms->limit);

        // Open cur for writes
        in_set_len (page_h_w (&pms->cur), IN_MAX_KEYS);
        pms->lidx = 0;

        // Shift right
        const pgno npg = in_get_next (page_h_ro (&pms->cur));
        if (npg != PGNO_NULL && pms->limit.mode == PHM_NONE)
        {
          if (pgr_get_writable (
                  &pms->limit,
                  pms->tx,
                  PG_INNER_NODE,
                  npg,
                  pms->p,
                  e
              ))
          {
            goto failed;
          }
        }
      }

      // [++++++____________]
      //        ^
      //       lidx
      // [a, b, c] p [d, e, f, g, h, i]
      //                 ^        ^  ^
      //               rcons    robs rlen
      // OR
      //
      // Node could be done:
      // TODO - (18) Maybe optimize this out
      // - right now there's an extra page load
      //
      // [a, b, c] p [d, e, f, g, h, i]
      //                          ^  ^
      //                        rlen robs
      // rcons
      else
      {
        TEST_MARK ("rebalance:right:not_done_observing:done_consuming");

        ASSERT (nupd_done_consuming_right (pms->input));
        ASSERT (nupd_done_right (pms->input) || pms->limit.mode != PHM_NONE);

        if (pms->limit.mode != PHM_NONE)
        {
          TEST_MARK (
              "rebalance:right:not_done_observing:done_consuming:limit_nn"
          );
          const pgno npg = page_h_pgno (&pms->limit);

          const pgno nnpg = in_get_next (page_h_ro (&pms->limit));
          if (nnpg != PGNO_NULL)
          {
            if (pgr_get_writable (
                    &next_next,
                    pms->tx,
                    PG_INNER_NODE,
                    nnpg,
                    pms->p,
                    e
                ))
            {
              goto failed;
            }
          }
          if (pgr_delete_and_release (pms->p, pms->tx, &pms->limit, e))
          {
            goto failed;
          }
          in_link (page_h_w (&pms->cur), page_h_w_or_null (&next_next));
          page_h_xfer_ownership_ptr (&pms->limit, &next_next);

          if (nupd_append_2nd_right (
                  pms->output,
                  pgh_unravel (&pms->cur),
                  npg,
                  0,
                  e
              ))
          {
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

static err_t
rb_execute_left (struct ns_rebalance_params *pms, error *e)
{
  page_h prev      = page_h_create ();
  page_h prev_prev = page_h_create ();

  while (true)
  {
    // [_______+++++++++++]
    // ^
    // lidx
    // [a, b, c, d, e, f] p [g, h, i]
    // ^        ^     ^
    // lobs     llen   lcons
    if (nupd_done_observing_left (pms->input))
    {
      pms->lidx -=
          nupd_append_maximally_left (pms->input, &pms->cur, pms->lidx);

      // [++++++++++++++++++]
      // ^
      // lidx
      // [a, b, c, d, e, f] p [g, h, i]
      // ^     ^  ^
      // lobs lcons llen
      // lcons didn't reach lobs. That can
      // only happen if we filled up current
      // node
      if (!nupd_done_left (pms->input))
      {
        TEST_MARK ("rebalance:left:done_observing:not_done_consuming");

        ASSERT (pms->lidx == 0);
        if (nupd_commit_1st_left (
                pms->output,
                page_h_pgno (&pms->cur),
                in_get_size (page_h_ro (&pms->cur)),
                e
            ))
        {
          goto failed;
        }

        // limit <- cur
        // limit <- new <- cur
        // limit <- new
        if (pgr_new (&prev, pms->p, pms->tx, PG_INNER_NODE, e))
        {
          goto failed;
        }
        in_link (page_h_w_or_null (&pms->limit), page_h_w (&prev));
        in_link (page_h_w (&prev), page_h_w (&pms->cur));
        if (pgr_release (pms->p, &pms->cur, PG_INNER_NODE, e))
        {
          goto failed;
        }
        pms->cur = page_h_xfer_ownership (&prev);

        in_set_len (page_h_w (&pms->cur), IN_MAX_KEYS);
        pms->lidx = IN_MAX_KEYS;

        continue;
      }
      else
      {
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
    else
    {
      if (nupd_observe_all_left (pms->input, &pms->limit, e))
      {
        goto failed;
      }
      pms->lidx -=
          nupd_append_maximally_left (pms->input, &pms->cur, pms->lidx);

      // [_______+++++++++++]
      // ^
      // lidx
      // [a, b, c, d, e, f] p [g, h, i]
      // ^  ^        ^
      // llen lobs   lcons
      if (!nupd_done_left (pms->input)
          && (IN_MAX_KEYS - pms->lidx) > IN_MAX_KEYS / 2)
      {
        TEST_MARK ("rebalance:left:not_done_observing:not_done:still_shift");

        // Shift left (limit is
        // effectively "empty"
        // because it was
        // observed so we can
        // use it as a slot for
        // next) NULL <- cur
        // limit -> cur
        // limit
        // prev <- limit
        if (pms->limit.mode == PHM_NONE)
        {
          if (pgr_new (&pms->limit, pms->p, pms->tx, PG_INNER_NODE, e))
          {
            goto failed;
          }
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
                e
            ))
        {
          goto failed;
        }

        if (pgr_release (pms->p, &pms->cur, PG_INNER_NODE, e))
        {
          goto failed;
        }

        // cur = limit
        page_h_xfer_ownership_ptr (&pms->cur, &pms->limit);

        // Open cur for writes
        in_set_len (page_h_w (&pms->cur), IN_MAX_KEYS);
        pms->lidx = IN_MAX_KEYS;

        // Shift left
        const pgno ppg = in_get_prev (page_h_ro (&pms->cur));
        if (ppg != PGNO_NULL && pms->limit.mode == PHM_NONE)
        {
          if (pgr_get_writable (
                  &pms->limit,
                  pms->tx,
                  PG_INNER_NODE,
                  ppg,
                  pms->p,
                  e
              ))
          {
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
      else
      {
        TEST_MARK ("rebalance:left:not_done_observing:done_consuming");

        ASSERT (nupd_done_consuming_left (pms->input));
        ASSERT (nupd_done_left (pms->input) || pms->limit.mode != PHM_NONE);

        if (pms->limit.mode != PHM_NONE)
        {
          TEST_MARK (
              "rebalance:left:not_done_observing:done_consuming:limit_nn"
          );
          const pgno ppg = page_h_pgno (&pms->limit);

          const pgno pppg = in_get_prev (page_h_ro (&pms->limit));
          if (pppg != PGNO_NULL)
          {
            if (pgr_get_writable (
                    &prev_prev,
                    pms->tx,
                    PG_INNER_NODE,
                    pppg,
                    pms->p,
                    e
                ))
            {
              goto failed;
            }
          }
          if (pgr_delete_and_release (pms->p, pms->tx, &pms->limit, e))
          {
            goto failed;
          }
          in_link (page_h_w_or_null (&prev_prev), page_h_w (&pms->cur));
          page_h_xfer_ownership_ptr (&pms->limit, &prev_prev);

          if (nupd_append_2nd_left (
                  pms->output,
                  pgh_unravel (&pms->cur),
                  ppg,
                  0,
                  e
              ))
          {
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

static err_t
ns_pop_stack (struct ns_rebalance_params *pms, error *e)
{
  struct seek_v *ref = &pms->pstack[--(pms->sp)];

  struct seek_v v = {
      .pg   = page_h_xfer_ownership (&ref->pg),
      .lidx = ref->lidx,
  };

  if (pms->cur.mode != PHM_NONE)
  {
    if (pgr_release (pms->p, &pms->cur, PG_INNER_NODE | PG_DATA_LIST, e))
    {
      goto failed;
    }
  }

  pms->cur  = page_h_xfer_ownership (&v.pg);
  pms->lidx = v.lidx;

  return SUCCESS;

failed:
  pgr_cancel_if_exists (pms->p, &v.pg);
  return error_trace (e);
}

static err_t ns_rebalance_move_up_stack (
    struct ns_rebalance_params *pms,
    error                      *e
);

/*
 * Maximally applies node updates
 * to the pivot node
 */
static err_t
ns_rebalance_apply_to_pivot (struct ns_rebalance_params *pms, error *e)
{
  page_h prev = page_h_create ();
  page_h next = page_h_create ();

  if (nupd_observe_pivot (pms->input, &pms->cur, pms->lidx, e))
  {
    goto failed;
  }
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
  pms->lidx = IN_MAX_KEYS
              - nupd_append_maximally_right_then_left (pms->input, &pms->cur);

  if (nupd_done_left (pms->input))
  {
    TEST_MARK ("apply_to_pivot_done_left");
    // [++++++++++++++++++__]
    // ^
    // lidx
    in_cut_left (page_h_w (&pms->cur), pms->lidx);
    pms->lidx = IN_MAX_KEYS - pms->lidx;

    // DONE EARLY
    if (nupd_done_right (pms->input))
    {
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
      if (ns_balance_and_release (bparams, e))
      {
        goto failed;
      }
      if (nupd_append_tip_right (pms->output, tip_out, e))
      {
        goto failed;
      }

      return ns_rebalance_move_up_stack (pms, e);
    }

    // Open up for right updates
    // [++++++++++++++++++--]
    // ^
    // lidx
    in_set_len (page_h_w (&pms->cur), IN_MAX_KEYS);

    // Right mode in read mode
    const pgno next_pg = in_get_next (page_h_ro (&pms->cur));
    if (next_pg != PGNO_NULL)
    {
      if (pgr_get_writable (
              &pms->limit,
              pms->tx,
              PG_INNER_NODE,
              next_pg,
              pms->p,
              e
          ))
      {
        goto failed;
      }
    }

    return SUCCESS;
  }
  else
  {
    TEST_MARK ("apply_to_pivot_not_done_left");

    // Left mode
    const pgno prev_pg = in_get_prev (page_h_ro (&pms->cur));
    if (prev_pg != PGNO_NULL)
    {
      if (pgr_get_writable (
              &pms->limit,
              pms->tx,
              PG_INNER_NODE,
              prev_pg,
              pms->p,
              e
          ))
      {
        goto failed;
      }
    }

    return SUCCESS;
  }

failed:
  pgr_cancel_if_exists (pms->p, &prev);
  pgr_cancel_if_exists (pms->p, &next);
  return error_trace (e);
}

#ifdef TESTING

/*
 * Rebalance tests construct node updates manually,
 * but if it weren't possible to have arbitrary length
 * node updates to begin with, then those tests are
 * completely useless.
 *
 * This test serves as a way to ensure that node
 * update states are actually reachable.
 */
TEST (possible_to_get_long_left_tail_on_nupd)
{
  // TODO
}

static inline spgno
build_2_layer_tree (p_size len, struct pgr_fixture *f)
{
  u8 data[DL_DATA_SIZE];
  rand_bytes (data, sizeof (data));

  struct tree_descr bottom[IN_MAX_KEYS];
  for (u32 i = 0; i < len; ++i)
  {
    bottom[i].data = data;
    bottom[i].dlen = DL_DATA_SIZE;
    bottom[i].next = NULL;
  }

  struct tree_descr descr = {
      .next = bottom,
      .nlen = len,
  };

  return build_tree_from_descr (f->p, &f->tx, descr, &f->e);
}

/*
 * These tests construct a 2 layer tree:
 *
 *              [+++++++_______________]
 *        ... ... ... ... ... ... ... ... ...
 *
 * You specify:
 * - (in_len) how much data goes into the root inner node
 *
 * Then you apply a rebalance to the root node. You specify:
 * - (rlen) how long the right updates are
 * - (llen) how long the left updates are
 *
 * When you apply a rebalance, if rlen + llen + in_len > IN_MAX_KEYS,
 * we expect a tree split, which would mean growing upwards by 1 node
 *
 * Otherwise, we expect it to remain at 2 layers.
 *
 * The function do_rebalance_on_2_layer_tree does the full test case
 * read through the code to understand exactly what it does
 */
static inline spgno
do_rebalance_on_2_layer_tree (
    struct pgr_fixture *f,
    u32                 llen,
    u32                 rlen,
    p_size              in_len,
    p_size              lidx
)
{
  // Build the 3 node tree
  spgno root = build_2_layer_tree (in_len, f);

  // Should be 3 nodes - e.g.
  ASSERT (ns_get_number_of_layers (f->p, root, &f->e) == 2);

  // Fetch the root page to reconstruct the seek stack
  page_h rpg = page_h_create ();
  pgr_get_writable (&rpg, &f->tx, PG_INNER_NODE, root, f->p, &f->e);

  // Build the mock node updates
  pgno  pivot = in_get_leaf (page_h_ro (&rpg), lidx);
  pgno *right = NULL;
  pgno *left  = NULL;

  if (rlen > 0)
  {
    right = i_malloc (rlen, sizeof *right, &f->e);

    // Ensure they are unique - TODO - make this better
    for (u32 i = 0; i < rlen; ++i)
    {
      right[i] = randu64r (1000000, 1000000000000000);
    }
  }

  if (llen > 0)
  {
    left = i_malloc (llen, sizeof *left, &f->e);

    // Ensure they are unique
    for (u32 i = 0; i < llen; ++i)
    {
      left[i] = randu64r (1000000, 1000000000000000);
    }
  }

  // Create random node updates
  struct node_updates *output =
      nupd_random_from (left, llen, pivot, right, rlen, &f->e);
  struct node_updates *input = nupd_init (0, 0, &f->e);
  ASSERT (output != NULL);

  // Do rebalance
  struct ns_rebalance_params rebalance = {
      .p    = f->p,
      .tx   = &f->tx,
      .root = root,
      .pstack =
          (struct seek_v[]){
              (struct seek_v){
                  .pg   = rpg,
                  .lidx = lidx,
              },
          },
      .sp         = 1,
      .input      = input,
      .output     = output,
      .layer_root = {.isroot = false},
  };

  ns_rebalance (&rebalance, &f->e);

  i_cfree (right);
  i_cfree (left);
  nupd_free (output);
  nupd_free (input);

  return rebalance.root;
}
TEST (ns_rebalance_apply_to_pivot_splits_2_layer_tree)
{
  struct pgr_fixture f;
  pgr_fixture_create (&f);
  pgr_begin_txn (&f.tx, f.p, &f.e);

  TEST_CASE ("Just overwrite one key")
  {
    spgno root = do_rebalance_on_2_layer_tree (&f, 0, 0, IN_MAX_KEYS, 10);
    test_assert_int_equal (ns_get_number_of_layers (f.p, root, &f.e), 2);
  }

  TEST_CASE ("Right 1 key full")
  {
    spgno root = do_rebalance_on_2_layer_tree (&f, 0, 1, IN_MAX_KEYS, 10);
    test_assert_int_equal (ns_get_number_of_layers (f.p, root, &f.e), 3);
  }

  TEST_CASE ("Left 1 key full")
  {
    spgno root = do_rebalance_on_2_layer_tree (&f, 1, 0, IN_MAX_KEYS, 10);
    test_assert_int_equal (ns_get_number_of_layers (f.p, root, &f.e), 3);
  }

  TEST_CASE ("Right and Left 1 key full")
  {
    spgno root = do_rebalance_on_2_layer_tree (&f, 1, 1, IN_MAX_KEYS, 10);
    test_assert_int_equal (ns_get_number_of_layers (f.p, root, &f.e), 3);
    root = do_rebalance_on_2_layer_tree (&f, 1, 1, IN_MAX_KEYS - 1, 10);
    test_assert_int_equal (ns_get_number_of_layers (f.p, root, &f.e), 3);
  }

  TEST_CASE ("Right 1 key don't split")
  {
    spgno root = do_rebalance_on_2_layer_tree (&f, 0, 1, IN_MAX_KEYS - 1, 10);
    test_assert_int_equal (ns_get_number_of_layers (f.p, root, &f.e), 2);
  }

  TEST_CASE ("Left 1 key don't split")
  {
    spgno root = do_rebalance_on_2_layer_tree (&f, 1, 0, IN_MAX_KEYS - 1, 10);
    test_assert_int_equal (ns_get_number_of_layers (f.p, root, &f.e), 2);
  }

  TEST_CASE ("Right and Left 1 key don't split")
  {
    spgno root = do_rebalance_on_2_layer_tree (&f, 1, 1, IN_MAX_KEYS - 2, 10);
    test_assert_int_equal (ns_get_number_of_layers (f.p, root, &f.e), 2);
  }

  /**
   * TODO - get this working with random lidx and in_len
   */
  for (u32 i = 2; i < 10; ++i)
  {
    u32    in_len = IN_MAX_KEYS;
    u32    len    = randu32r (0, IN_MAX_KEYS * 10);
    p_size lidx   = 10;

    TEST_CASE ("Right length %d", len)
    {
      spgno root = do_rebalance_on_2_layer_tree (&f, 0, len, in_len, lidx);
      if (in_len + len > IN_MAX_KEYS)
      {
        test_assert_int_equal (ns_get_number_of_layers (f.p, root, &f.e), 3);
      }
      else
      {
        test_assert_int_equal (ns_get_number_of_layers (f.p, root, &f.e), 2);
      }
    }

    TEST_CASE ("Left length %d", len)
    {
      spgno root = do_rebalance_on_2_layer_tree (&f, len, 0, in_len, lidx);
      if (in_len + len > IN_MAX_KEYS)
      {
        test_assert_int_equal (ns_get_number_of_layers (f.p, root, &f.e), 3);
      }
      else
      {
        test_assert_int_equal (ns_get_number_of_layers (f.p, root, &f.e), 2);
      }
    }

    TEST_CASE ("Right Left length %d", len)
    {
      spgno root = do_rebalance_on_2_layer_tree (&f, len, len, in_len, lidx);
      if (in_len + 2 * len > IN_MAX_KEYS)
      {
        test_assert_int_equal (ns_get_number_of_layers (f.p, root, &f.e), 3);
      }
      else
      {
        test_assert_int_equal (ns_get_number_of_layers (f.p, root, &f.e), 2);
      }
    }
  }

  pgr_commit (f.p, &f.tx, &f.e);
  pgr_fixture_teardown (&f);
}
#endif

static err_t
ns_rebalance_move_up_stack (struct ns_rebalance_params *pms, error *e)
{
  if (pms->layer_root.isroot)
  {
    // Delete all the next layers above
    while (pms->sp != 0)
    {
      if (ns_pop_stack (pms, e))
      {
        goto failed;
      }
      if (in_delete_chain (&pms->cur, pms->tx, pms->p, e))
      {
        goto failed;
      }
    }

    pms->lidx = 0;
    pms->root = pms->layer_root.root;

    return SUCCESS;
  }
  else
  {
    // We filled a layer, but need to grow upwards
    if (pms->sp == 0)
    {
      // This is where tree's grow upwards - create a new layer
      // we are now 1 layer bigger
      if (pgr_new (&pms->cur, pms->p, pms->tx, PG_INNER_NODE, e))
      {
        goto failed;
      }

      pms->root = page_h_pgno (&pms->cur);
      pms->lidx = 0;
    }
    else
    {
      // Otherwise we're just working out way upwards
      if (ns_pop_stack (pms, e))
      {
        goto failed;
      }
    }

    // Swap node updates
    struct node_updates *input  = pms->input;
    struct node_updates *output = pms->output;
    pms->output                 = input;
    pms->input                  = output;

    if (pms->output == NULL)
    {
      /*
       * The input from the previous layer is null on the first
       * (bottom) layer. Think input = NULL because there is no
       * input at the bottom layer.
       *
       * So this line is hit one the first loop always
       */
      pms->output = nupd_init (
          page_h_pgno (&pms->cur),
          in_get_size (page_h_ro (&pms->cur)),
          e
      );
      if (pms->output == NULL)
      {
        goto failed;
      }
    }
    else
    {
      /*
       * This happens when the previous layer input was not NULL,
       * e.g. at least two loops/layers happened
       */
      nupd_reset (
          pms->output,
          page_h_pgno (&pms->cur),
          in_get_size (page_h_ro (&pms->cur))
      );
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
err_t
ns_rebalance (struct ns_rebalance_params *pms, error *e)
{
  pms->cur   = page_h_create ();
  pms->limit = page_h_create ();
  pms->lidx  = 0;

  while (true)
  {
    if (ns_rebalance_move_up_stack (pms, e))
    {
      goto failed;
    }

    /*
     * In the previous function call, we covered root node,
     * so just return SUCCESS
     */
    if (pms->layer_root.isroot)
    {
      ASSERT (e->cause_code == SUCCESS);
      return error_trace (e);
    }

    bool done = true;

    // Execute left
    if (!nupd_done_left (pms->input))
    {
      done = false;
      if (rb_execute_left (pms, e))
      {
        goto failed;
      }
    }

    // Execute right
    if (!nupd_done_right (pms->input))
    {
      done = false;
      if (rb_execute_right (pms, e))
      {
        goto failed;
      }
    }

    if (done)
    {
      return SUCCESS;
    }
  }

failed:
  pgr_cancel_if_exists (pms->p, &pms->cur);
  pgr_cancel_if_exists (pms->p, &pms->limit);
  return error_trace (e);
}
