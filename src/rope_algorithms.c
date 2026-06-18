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

#include "rope_algorithms.h"

#include "node_updates.h"
#include "page.h"
#include "pager.h"
#include "testing/page_fixture.h"

/******************************************************************************
 * SECTION: Common Data Structures
 * ----------------------------------------------------------------------------
 * @brief
 *
 *
 ******************************************************************************/

enum stride_phase
{
  ACTIVE,
  SKIPPING,
};

/******************************************************************************
 * SECTION: ns_balance_and_release
 * ----------------------------------------------------------------------------
 * @brief Balance a page with it's neighbor then release it
 ******************************************************************************/

/*
 * Rebalance a data-list leaf against its left (prev) sibling.
 * If cur is at or above the half-full threshold, no action is needed.
 *
 * If sizeof(cur) + sizeof(prev) >= max node length, then that means we
 * have enough data to balance at least max / 2 for each node. Otherwise,
 * we _have_ to delete a node.
 *
 * If we need to delete a node, it's always [cur]
 */
static void
dlgt_balance_with_prev (const page_h *prev, const page_h *cur)
{
  ASSERT (prev->mode == PHM_X);
  ASSERT (cur->mode == PHM_X);
  ASSERT (dlgt_valid_neighbors (page_h_ro (prev), page_h_ro (cur)));

  const p_size prev_len = dlgt_get_len (page_h_ro (prev));
  const p_size cur_len  = dlgt_get_len (page_h_ro (cur));
  const p_size maxlen   = dlgt_get_max_len (page_h_ro (prev));

  // Already valid
  if (cur_len == 0)
  {
    return;
  }

  // Also valid
  if (cur_len >= maxlen / 2)
  {
    return;
  }

  // There's enough data to balaance max / 2 for each node
  if (prev_len + cur_len >= maxlen)
  {
    dlgt_move_right (page_h_w (prev), page_h_w (cur), maxlen / 2 - cur_len);
    return;
  }

  // Move all the data left to prev
  dlgt_move_left (page_h_w (prev), page_h_w (cur), cur_len);
}

#ifndef NTEST
TEST (dlgt_balance_with_prev)
{
  struct pgr_fixture f;
  error             *e = &f.e;
  pgr_fixture_create (&f);

  u8 _prev[DL_DATA_SIZE];
  u8 _cur[DL_DATA_SIZE];
  u32_arr_rand (_prev);
  u32_arr_rand (_cur);

  struct txn tx;
  pgr_begin_txn (&tx, f.p, &f.e);

  struct page_tree_builder builder = {
      .root =
          {
              .type = PG_INNER_NODE,
              .out  = page_h_create (),
              .inner =
                  {
                      .dclen = 2,
                      .clen  = 2,
                      .children =
                          (struct page_desc[]){
                              {
                                  .type = PG_DATA_LIST,
                                  .out  = page_h_create (),
                                  .size = DL_DATA_SIZE,
                                  .data_list =
                                      {
                                          .data = _prev,
                                          .blen = DL_DATA_SIZE,
                                      },
                              },
                              {
                                  .type = PG_DATA_LIST,
                                  .out  = page_h_create (),
                                  .size = DL_DATA_SIZE,
                                  .data_list =
                                      {
                                          .data = _cur,
                                          .blen = DL_DATA_SIZE,
                                      },
                              },
                          },
                  },
          },
      .pager = f.p,
      .txn   = &tx,
  };

  build_page_tree (&builder, &f.e);

  page_h *prev = &builder.root.inner.children[0].out;
  page_h *cur  = &builder.root.inner.children[1].out;

  pgr_release (f.p, &builder.root.out, PG_INNER_NODE, e);

  TEST_CASE ("Both Full no change")
  {
    dlgt_balance_with_prev (prev, cur);
    test_assert_equal (dl_used (page_h_ro (prev)), DL_DATA_SIZE);
    test_assert_equal (dl_used (page_h_ro (cur)), DL_DATA_SIZE);
    test_assert_memequal (dl_get_data (page_h_ro (prev)), _prev, DL_DATA_SIZE);
    test_assert_memequal (dl_get_data (page_h_ro (cur)), _cur, DL_DATA_SIZE);
  }

  // BEFORE
  // [++++++++++++|****___10____]
  // [+++10+++____|_____________]
  // AFTER
  // [++++++++++++|_____________]
  // [****++++++++|_____________]
  TEST_CASE ("No Delete")
  {
    dl_memset (page_h_w (prev), _prev, DL_DATA_SIZE - 10);
    dl_memset (page_h_w (cur), _cur, 10);

    dlgt_balance_with_prev (prev, cur);
    test_assert_equal (dl_used (page_h_ro (prev)), DL_DATA_SIZE / 2 + DL_REM);
    test_assert_equal (dl_used (page_h_ro (cur)), DL_DATA_SIZE / 2);

    u32 i = 0;
    for (; i < DL_DATA_SIZE / 2 + DL_REM; ++i)
    {
      test_assert_equal (dl_get_byte (page_h_ro (prev), i), _prev[i]);
    }
    i = 0;
    for (; i < DL_DATA_SIZE - 10 - DL_DATA_SIZE / 2 - DL_REM; ++i)
    {
      test_assert_equal (
          dl_get_byte (page_h_ro (cur), i),
          _prev[DL_DATA_SIZE / 2 + DL_REM + i]
      );
    }
    const u32 k = i;
    for (; i < DL_DATA_SIZE / 2; ++i)
    {
      test_assert_equal (dl_get_byte (page_h_ro (cur), i), _cur[i - k]);
    }
  }

  // BEFORE
  // [++++++++++++|++++___10____]
  // [***9***____|_____________]
  // AFTER
  // [++++++++++++|++++***9***_]
  // [____________|_____________]
  TEST_CASE ("Delete")
  {
    dl_memset (page_h_w (prev), _prev, DL_DATA_SIZE - 10);
    dl_memset (page_h_w (cur), _cur, 9);

    dlgt_balance_with_prev (prev, cur);
    test_assert_equal (dl_used (page_h_ro (prev)), DL_DATA_SIZE - 1);
    test_assert_equal (dl_used (page_h_ro (cur)), 0);

    u32 i = 0;
    // next data
    for (; i < DL_DATA_SIZE - 10; ++i)
    {
      test_assert_equal (dl_get_byte (page_h_ro (prev), i), _prev[i]);
    }
    const u32 k = i;
    for (; i < 9; ++i)
    {
      test_assert_equal (dl_get_byte (page_h_ro (prev), i), _cur[i - k]);
    }
  }

  pgr_release (f.p, prev, PG_DATA_LIST, e);
  pgr_delete_and_release (f.p, &tx, cur, e);

  pgr_commit (f.p, &tx, &f.e);

  pgr_fixture_teardown (&f);
}
#endif

/*
 * Rebalance a data-list leaf against its right (next) sibling.
 *
 * Mirror of dlgt_balance_with_prev.  If cur is below half-full and
 * cur + next exceed a page, bytes are moved from the head of next into the
 * tail of cur (borrow).  If they fit together, all of cur is moved into
 * next, leaving cur empty (merge).
 */
static void
dlgt_balance_with_next (const page_h *cur, const page_h *next)
{
  ASSERT (cur->mode == PHM_X);
  ASSERT (next->mode == PHM_X);
  ASSERT (dlgt_valid_neighbors (page_h_ro (cur), page_h_ro (next)));

  const p_size next_len = dlgt_get_len (page_h_ro (next));
  const p_size cur_len  = dlgt_get_len (page_h_ro (cur));
  const p_size maxlen   = dlgt_get_max_len (page_h_ro (next));

  // Already valid
  if (cur_len == 0)
  {
    return;
  }

  if (cur_len >= maxlen / 2)
  {
    return;
  }

  if (next_len + cur_len >= maxlen)
  {
    dlgt_move_left (page_h_w (cur), page_h_w (next), maxlen / 2 - cur_len);
    return;
  }

  dlgt_move_right (page_h_w (cur), page_h_w (next), cur_len);
}

#ifndef NTEST
TEST (dlgt_balance_with_next)
{
  struct pgr_fixture f;
  error             *e = &f.e;
  pgr_fixture_create (&f);

  u8 _cur[DL_DATA_SIZE];
  u8 _next[DL_DATA_SIZE];
  u32_arr_rand (_next);
  u32_arr_rand (_cur);

  struct txn tx;
  pgr_begin_txn (&tx, f.p, &f.e);

  struct page_tree_builder builder = {
      .root =
          {
              .type = PG_INNER_NODE,
              .out  = page_h_create (),
              .inner =
                  {
                      .dclen = 2,
                      .clen  = 2,
                      .children =
                          (struct page_desc[]){
                              {
                                  .type = PG_DATA_LIST,
                                  .out  = page_h_create (),
                                  .size = DL_DATA_SIZE,
                                  .data_list =
                                      {
                                          .data = _cur,
                                          .blen = DL_DATA_SIZE,
                                      },
                              },
                              {
                                  .type = PG_DATA_LIST,
                                  .out  = page_h_create (),
                                  .size = DL_DATA_SIZE,
                                  .data_list =
                                      {
                                          .data = _next,
                                          .blen = DL_DATA_SIZE,
                                      },
                              },
                          },
                  },
          },
      .pager = f.p,
      .txn   = &tx,
  };

  build_page_tree (&builder, &f.e);

  page_h *cur  = &builder.root.inner.children[0].out;
  page_h *next = &builder.root.inner.children[1].out;

  pgr_release (f.p, &builder.root.out, PG_INNER_NODE, e);

  TEST_CASE ("Both Full no change")
  {
    dlgt_balance_with_next (cur, next);
    test_assert_equal (dl_used (page_h_ro (cur)), DL_DATA_SIZE);
    test_assert_equal (dl_used (page_h_ro (next)), DL_DATA_SIZE);
    test_assert_memequal (dl_get_data (page_h_ro (cur)), _cur, DL_DATA_SIZE);
    test_assert_memequal (dl_get_data (page_h_ro (next)), _next, DL_DATA_SIZE);
  }

  // BEFORE
  // [+++10+++____|_____________]
  // [****++++++++|++++___10____]
  // AFTER
  // [+++10+++****|_____________]
  // [++++++++++++|___10____]
  TEST_CASE ("No Delete")
  {
    _Static_assert (DL_DATA_SIZE > 10, "This test needs DL_DATA_SIZE > 10");
    dl_memset (page_h_w (cur), _cur, 10);
    dl_memset (page_h_w (next), _next, DL_DATA_SIZE - 10);

    dlgt_balance_with_next (cur, next);
    test_assert_equal (dl_used (page_h_ro (cur)), DL_DATA_SIZE / 2);
    test_assert_equal (dl_used (page_h_ro (next)), DL_DATA_SIZE / 2 + DL_REM);

    u32 i = 0;
    for (; i < 10; ++i)
    {
      test_assert_equal (dl_get_byte (page_h_ro (cur), i), _cur[i]);
    }
    for (; i < DL_DATA_SIZE / 2; ++i)
    {
      test_assert_equal (dl_get_byte (page_h_ro (cur), i), _next[i - 10]);
    }
    i = 0;
    for (; i < DL_DATA_SIZE / 2 + DL_REM; ++i)
    {
      test_assert_equal (
          dl_get_byte (page_h_ro (next), i),
          _next[i + DL_DATA_SIZE / 2 - 10]
      );
    }
  }

  // BEFORE
  // [+++10+++____|_____________]
  // [****++++++++|++++___10____]
  // AFTER
  // [+++10+++****|_____________]
  // [++++++++++++|___10____]
  TEST_CASE ("Delete")
  {
    dl_memset (page_h_w (cur), _cur, 9);
    dl_memset (page_h_w (next), _next, DL_DATA_SIZE - 10);

    dlgt_balance_with_next (cur, next);
    test_assert_equal (dl_used (page_h_ro (cur)), 0);
    test_assert_equal (dl_used (page_h_ro (next)), DL_DATA_SIZE - 1);

    u32 i = 0;
    for (; i < 9; ++i)
    {
      test_assert_equal (dl_get_byte (page_h_ro (next), i), _cur[i]);
    }

    // next data
    for (; i < DL_DATA_SIZE - 1; ++i)
    {
      test_assert_equal (dl_get_byte (page_h_ro (next), i), _next[i - 9]);
    }
  }

  pgr_release (f.p, next, PG_DATA_LIST, e);
  pgr_delete_and_release (f.p, &tx, cur, e);

  pgr_commit (f.p, &tx, &f.e);

  pgr_fixture_teardown (&f);
}
#endif

static struct three_in_pair
three_in_pair_from (const page_h *prev, const page_h *cur, const page_h *next)
{
  ASSERT (prev == NULL || prev->mode != PHM_NONE);
  ASSERT (cur == NULL || cur->mode != PHM_NONE);
  ASSERT (next == NULL || next->mode != PHM_NONE);

  struct three_in_pair ret = {
      .prev = in_pair_empty,
      .cur  = in_pair_empty,
      .next = in_pair_empty,
  };

  if (prev)
  {
    ret.prev = in_pair_from_pgh (prev);
  }
  if (cur)
  {
    ret.cur = in_pair_from_pgh (cur);
  }
  if (next)
  {
    ret.next = in_pair_from_pgh (next);
  }

  return ret;
}

// TODO - graceful error handling and clean up of partial pages
err_t
ns_balance_and_release (
    const struct ns_balance_and_release_params params,
    error                                     *e
)
{
  ASSERT (
      params.prev->mode == PHM_NONE
      || dlgt_valid_neighbors (page_h_ro (params.prev), page_h_ro (params.cur))
  );
  ASSERT (
      params.next->mode == PHM_NONE
      || dlgt_valid_neighbors (page_h_ro (params.cur), page_h_ro (params.next))
  );
  ASSERT (params.output);

  // Upgrade cur to writable - so far there's no garuntees that cur
  // is already writable on entry
  const p_size csize = dlgt_get_len (page_h_ro (params.cur));

  *params.output = three_in_pair_from (NULL, params.cur, NULL);

  // Cur needs balancing because it is less than maxlen / 2
  if (csize > 0 && csize < dlgt_get_max_len (page_h_ro (params.cur)) / 2)
  {
    // If next is present - try balancing with next
    if (params.next->mode != PHM_NONE)
    {
      dlgt_balance_with_next (params.cur, params.next);
      *params.output = three_in_pair_from (NULL, params.cur, params.next);
    }

    // If prev is present - try balancing with prev
    else if (params.prev->mode != PHM_NONE)
    {
      dlgt_balance_with_prev (params.prev, params.cur);
      *params.output = three_in_pair_from (params.prev, params.cur, NULL);
    }

    // Loop back to next - load next and try again (if next even exists)
    else if (dlgt_get_next (page_h_ro (params.cur)) != PGNO_NULL)
    {
      WRAP (pgr_get_writable (
          params.next,
          params.tx,
          PG_INNER_NODE | PG_DATA_LIST,
          dlgt_get_next (page_h_ro (params.cur)),
          params.p,
          e
      ));
      dlgt_balance_with_next (params.cur, params.next);
      *params.output = three_in_pair_from (NULL, params.cur, params.next);
    }

    // Loop back to start - load prev and try again (if prev even exists)
    else if (dlgt_get_prev (page_h_ro (params.cur)) != PGNO_NULL)
    {
      WRAP (pgr_get_writable (
          params.prev,
          params.tx,
          PG_INNER_NODE | PG_DATA_LIST,
          dlgt_get_prev (page_h_ro (params.cur)),
          params.p,
          e
      ));
      dlgt_balance_with_prev (params.prev, params.cur);
      *params.output = three_in_pair_from (params.prev, params.cur, NULL);
    }
    else
    {
      // This balance was performed on a root  node
      ASSERT (dlgt_is_root (page_h_ro (params.cur)));
    }
  }
  else
  {
    // there's no need to balance
  }

  // Assume cur is not a root; override below if it is
  params.root->isroot = false;
  if (dlgt_is_root (page_h_ro (params.cur)))
  {
    params.root->isroot = true;
    params.root->root   = page_h_pgno (params.cur);
  }

  // Need to delete cur
  if (dlgt_get_len (page_h_ro (params.cur)) == 0)
  {
    i_log_trace (
        "balance: deleting page %" PRpgno "\n",
        page_h_pgno (params.cur)
    );

    // Fetch prev and next for link re writing
    if (!params.root->isroot)
    {
      // Load prev sibling if the caller did not already pin it
      if (params.prev->mode == PHM_NONE)
      {
        const pgno prev_pg = dlgt_get_prev (page_h_ro (params.cur));
        if (prev_pg != PGNO_NULL)
        {
          WRAP (pgr_get_writable (
              params.prev,
              params.tx,
              PG_INNER_NODE | PG_DATA_LIST,
              prev_pg,
              params.p,
              e
          ));
        }
      }

      // Load next sibling if the caller did not already pin it
      if (params.next->mode == PHM_NONE)
      {
        const pgno next_pg = dlgt_get_next (page_h_ro (params.cur));
        if (next_pg != PGNO_NULL)
        {
          WRAP (pgr_get_writable (
              params.next,
              params.tx,
              PG_INNER_NODE | PG_DATA_LIST,
              dlgt_get_next (page_h_ro (params.cur)),
              params.p,
              e
          ));
        }
      }

      // Bridge the gap: prev->next = next, next->prev = prev
      dlgt_link (
          page_h_w_or_null (params.prev),
          page_h_w_or_null (params.next)
      );

      // We might have turned prev / next into a new root by deleting cur
      if (params.prev->mode != PHM_NONE
          && dlgt_is_root (page_h_ro (params.prev)))
      {
        params.root->root   = page_h_pgno (params.prev);
        params.root->isroot = true;
      }
      else if (
          params.next->mode != PHM_NONE
          && dlgt_is_root (page_h_ro (params.next))
      )
      {
        params.root->root   = page_h_pgno (params.next);
        params.root->isroot = true;
      }
    }

    // Otherwise cur is still root but we will delete it so now it's NULL
    else
    {
      // balance performed on root and deleted
      params.root->root = PGNO_NULL;
    }

    WRAP (pgr_delete_and_release (params.p, params.tx, params.cur, e));
  }

  // One final common cleanup
  WRAP (pgr_release_if_exists (
      params.p,
      params.prev,
      PG_DATA_LIST | PG_INNER_NODE,
      e
  ));
  WRAP (pgr_release_if_exists (
      params.p,
      params.cur,
      PG_DATA_LIST | PG_INNER_NODE,
      e
  ));
  WRAP (pgr_release_if_exists (
      params.p,
      params.next,
      PG_DATA_LIST | PG_INNER_NODE,
      e
  ));

  return SUCCESS;
}

/******************************************************************************
 * SECTION: ns_insert
 * ----------------------------------------------------------------------------
 * @brief Insert data into a byte array
 ******************************************************************************/

/*
 * Insert data into the R+Tree at the byte offset given by params->bofst.
 *
 * Seeks to the target data-list page, splits it at the insertion point,
 * streams new bytes from params->src into the page chain, re-appends the
 * displaced tail, re-links the chain, then balances the leaf and propagates
 * size changes up the inner-node tree via ns_rebalance().
 *
 * When nelem is 0, bytes are consumed from src until it is exhausted.
 * params->root is updated in place if the root changes.
 */
sb_size
ns_insert (struct ns_insert_params *params, error *e)
{
  page_h prev = page_h_create ();
  page_h cur  = page_h_create ();
  page_h next = page_h_create ();

  u8     temp_buf[DL_DATA_SIZE];
  p_size tbw = 0;
  p_size tbl = 0;

  struct node_updates *output   = NULL;
  struct node_updates *rb_nupd2 = NULL;
  struct three_in_pair tip_out;
  struct root_update   root;

  p_size lidx          = 0;
  b_size total_written = 0;

  struct ns_seek_params seek = {
      .p          = params->p,
      .tx         = params->tx,
      .root       = params->root,
      .bofst      = params->bofst,
      .save_stack = true,
      .sp         = 0,
  };

  if (params->root == PGNO_NULL)
  {
    if (pgr_new (&cur, params->p, params->tx, PG_DATA_LIST, e))
    {
      goto failed;
    }

    params->root = page_h_pgno (&cur);
  }
  else
  {
    if (ns_seek (&seek, e))
    {
      goto failed;
    }

    cur  = page_h_xfer_ownership (&seek.pg);
    lidx = seek.lidx;
  }

  pgno last = dl_get_next (page_h_ro (&cur));
  tbl       = dl_read_out_from (page_h_w (&cur), temp_buf, lidx);
  output    = nupd_init (page_h_pgno (&cur), 0, e);
  if (output == NULL)
  {
    goto failed;
  }

  const b_size total_to_write = params->size * params->nelem;

  while (params->nelem == 0 || total_written < total_to_write)
  {
    p_size avail = dl_avail (page_h_ro (&cur));

    if (avail == 0)
    {
      ASSERT (lidx == DL_DATA_SIZE);

      if (pgr_new (&next, params->p, params->tx, PG_DATA_LIST, e))
      {
        goto failed;
      }

      dl_set_next (page_h_w (&cur), page_h_pgno (&next));
      dl_set_prev (page_h_w (&next), page_h_pgno (&cur));

      if (nupd_commit_1st_right (output, pgh_unravel (&cur), e))
      {
        goto failed;
      }

      if (pgr_release (params->p, &cur, PG_DATA_LIST, e))
      {
        goto failed;
      }

      cur   = page_h_xfer_ownership (&next);
      lidx  = 0;
      avail = dl_avail (page_h_ro (&cur));
    }

    p_size next_amount;
    if (params->nelem == 0)
    {
      next_amount = avail;
    }
    else
    {
      next_amount = MIN (avail, (p_size)(total_to_write - total_written));
    }

    i32 written = stream_bread (
        dl_avail_data (page_h_w (&cur)),
        1,
        next_amount,
        params->src,
        e
    );
    if (written < 0)
    {
      goto failed;
    }

    if (written == 0 && stream_isdone (params->src))
    {
      break;
    }

    dl_set_used (page_h_w (&cur), dl_used (page_h_ro (&cur)) + written);
    lidx += (p_size)written;
    total_written += (b_size)written;
  }

  while (tbw < tbl)
  {
    p_size written = dl_append (page_h_w (&cur), temp_buf + tbw, tbl - tbw);

    lidx += written;
    tbw += written;

    if (lidx == DL_DATA_SIZE && tbw < tbl)
    {
      ASSERT (lidx == DL_DATA_SIZE);

      if (pgr_new (&next, params->p, params->tx, PG_DATA_LIST, e))
      {
        goto failed;
      }

      dl_set_next (page_h_w (&cur), page_h_pgno (&next));
      dl_set_prev (page_h_w (&next), page_h_pgno (&cur));

      if (nupd_commit_1st_right (output, pgh_unravel (&cur), e))
      {
        goto failed;
      }

      if (pgr_release (params->p, &cur, PG_DATA_LIST, e))
      {
        goto failed;
      }

      page_h_xfer_ownership_ptr (&cur, &next);
      lidx = 0;
    }
  }

  if (last != PGNO_NULL && last != dl_get_next (page_h_ro (&cur)))
  {
    if (pgr_get_writable (&next, params->tx, PG_DATA_LIST, last, params->p, e))
    {
      goto failed;
    }

    dlgt_link (page_h_w (&cur), page_h_w (&next));
  }

  struct ns_balance_and_release_params bparams = {
      .p      = params->p,
      .tx     = params->tx,
      .output = &tip_out,
      .root   = &root,
      .prev   = &prev,
      .cur    = &cur,
      .next   = &next,
  };

  if (ns_balance_and_release (bparams, e))
  {
    goto failed;
  }

  if (nupd_append_tip_right (output, tip_out, e))
  {
    goto failed;
  }

  struct ns_rebalance_params rebalance = {
      .p          = params->p,
      .tx         = params->tx,
      .root       = params->root,
      .pstack     = seek.pstack,
      .sp         = seek.sp,
      .input      = rb_nupd2,
      .output     = output,
      .layer_root = root,
  };

  output   = NULL;
  rb_nupd2 = NULL;

  err_t ret = ns_rebalance (&rebalance, e);

  if (rebalance.output)
  {
    nupd_free (rebalance.output);
  }
  if (rebalance.input)
  {
    nupd_free (rebalance.input);
  }

  if (ret)
  {
    goto failed;
  }

  params->root = rebalance.root;

  return (sb_size)total_written;

failed:
  pgr_cancel_if_exists (params->p, &prev);
  pgr_cancel_if_exists (params->p, &cur);
  pgr_cancel_if_exists (params->p, &next);

  if (output)
  {
    nupd_free (output);
  }
  if (rb_nupd2)
  {
    nupd_free (rb_nupd2);
  }

  for (u32 i = 0; i < seek.sp; ++i)
  {
    pgr_cancel_if_exists (params->p, &seek.pstack[i].pg);
  }

  return error_trace (e);
}

/******************************************************************************
 * SECTION: ns_read
 * ----------------------------------------------------------------------------
 * @brief Main read implementation
 ******************************************************************************/

/*
 * Compute how many bytes to transfer from the current page position.
 *
 * Returns the minimum of:
 *   - bytes remaining in the page from lidx to dl_used(),
 *   - bytes remaining in the current read/skip window (bnext), and
 *   - if ACTIVE and max_bread > 0, the bytes remaining before the global
 *     read limit is reached.
 *
 * Skip-window bytes never count toward max_bread, so stride gaps are not
 * subject to the element count limit.
 */
static t_size
ns_read_next_amount (
    const page             *curp,
    const t_size            lidx,
    const b_size            bnext,
    const b_size            max_bread,
    const b_size            total_bread,
    const enum stride_phase state
)
{
  // Bytes available from the current page position.
  p_size next_amount = dl_used (curp) - lidx;

  // Capped by how far into this element/stride window we still need to
  // go.
  next_amount = MIN (next_amount, bnext);

  // Capped by the global read limit (only in ACTIVE state; skip bytes do
  // not count toward max_bread).
  if (max_bread > 0 && state == ACTIVE)
  {
    next_amount = MIN (next_amount, max_bread - total_bread);
  }

  return next_amount;
}

// TODO - (4) tighten up the while loop to loop inside a page - rather than one
// read per loop
static sb_size
ns_read_forward (const struct ns_read_params params, error *e)
{
  ASSERT (params.stride > 0);

  page_h       cur         = page_h_create ();
  page_h       next        = page_h_create ();
  p_size       lidx        = 0;
  b_size       total_bread = 0;
  const b_size max_bread   = params.size * params.nelem;
  b_size bnext = params.size; // bytes remaining in the current read/skip window

  struct ns_seek_params seek = {
      .p          = params.p,
      .tx         = params.tx,
      .root       = params.root,
      .bofst      = params.bofst,
      .save_stack = false,
      .sp         = 0,
  };

  enum stride_phase state = ACTIVE;

  // Nothing to read from an empty tree.
  if (params.root == PGNO_NULL)
  {
    return 0;
  }

  if (ns_seek (&seek, e))
  {
    goto failed;
  }

  cur  = page_h_xfer_ownership (&seek.pg);
  lidx = seek.lidx;

  const page *curp = page_h_ro (&cur);

  enum
  {
    HIT_MAX_READ,
    DEST_DONE_READING,
    DATA_EXHAUSTED,
  } termination = HIT_MAX_READ;

  while (max_bread == 0 || total_bread < max_bread)
  {
    t_size next_amount =
        ns_read_next_amount (curp, lidx, bnext, max_bread, total_bread, state);

    if (next_amount == 0)
    {
      ASSERT (lidx <= dl_used (curp));
      if (lidx == dl_used (curp))
      {
        const pgno npg = dlgt_get_next (curp);

        if (npg == PGNO_NULL)
        {
          // No
          // more
          // nodes
          // to
          // the
          // right
          termination = DATA_EXHAUSTED;
          break;
        }

        if (pgr_get (&next, PG_DATA_LIST, npg, params.p, e))
        {
          goto failed;
        }

        if (pgr_release (params.p, &cur, PG_DATA_LIST, e))
        {
          goto failed;
        }

        // Reset stuff
        lidx        = 0;
        cur         = page_h_xfer_ownership (&next);
        curp        = page_h_ro (&cur);
        next_amount = ns_read_next_amount (
            curp,
            lidx,
            bnext,
            max_bread,
            total_bread,
            state
        );

        ASSERT (next_amount > 0);
      }
      else
      {
        UNREACHABLE ();
      }
    }

    switch (state)
    {
      case ACTIVE:
      {
        const sp_size read = stream_bwrite (
            (u8 *)dl_get_data (curp) + lidx,
            1,
            next_amount,
            params.dest,
            e
        );

        if (read < 0)
        {
          goto failed;
        }

        lidx += (p_size)read;
        total_bread += (b_size)read;
        bnext -= (b_size)read;

        if (bnext == 0)
        {
          bnext = (b_size)(params.stride - 1) * params.size;
          state = SKIPPING;

          // TODO - (5)
          // Optimize
          // stride = 1
          if (bnext == 0)
          {
            bnext = params.size;
            state = ACTIVE;
          }
        }
        break;
      }

      case SKIPPING:
      {
        const p_size read = dl_read (curp, NULL, lidx, next_amount);
        lidx += read;
        bnext -= read;

        if (bnext == 0)
        {
          bnext = params.size;
          state = ACTIVE;
        }
        break;
      }
    }

    if (stream_isdone (params.dest))
    {
      termination = DEST_DONE_READING;
      break;
    }
  }

  // Release the final page.
  WRAP (pgr_release (params.p, &cur, page_get_type (curp), e));

  // Verify that we always stopped on a complete element boundary.
  if (total_bread % params.size != 0)
  {
    error_causef (
        e,
        ERR_CORRUPT,
        "read %" PRb_size
        " bytes, not a multiple of element size "
        "%" PRt_size,
        total_bread,
        params.size
    );
    goto failed;
  }

  return total_bread / params.size;

failed:
  pgr_cancel_if_exists (params.p, &cur);
  pgr_cancel_if_exists (params.p, &next);
  return error_trace (e);
}

static sb_size
ns_read_backward (const struct ns_read_params params, error *e)
{
  panic ("TODO - (12)");
  return 0;
}

sb_size
ns_read (const struct ns_read_params params, error *e)
{
  if (params.stride > 0)
  {
    return ns_read_forward (params, e);
  }

  if (params.stride < 0)
  {
    return ns_read_backward (params, e);
  }

  return error_causef (e, ERR_INVALID_ARGUMENT, "read stride is 0");
}

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
  UNREACHABLE ();

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
    // ^
    // lidx
    // [a, b, c] p [d, e, f, g, h, i]
    // ^     ^        ^
    // rcons  rlen     robs
    if (nupd_done_observing_right (pms->input))
    {
      pms->lidx +=
          nupd_append_maximally_right (pms->input, &pms->cur, pms->lidx);

      // [++++++++++++++++++]
      // ^
      // lidx
      // [a, b, c] p [d, e, f, g, h, i]
      // ^  ^      ^
      // rlen rcons  robs
      // rcons didn't reach robs. That can
      // only happen if we filled up current
      // node
      if (!nupd_done_right (pms->input))
      {
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
    else
    {
      if (nupd_observe_all_right (pms->input, &pms->limit, e))
      {
        goto failed;
      }
      pms->lidx +=
          nupd_append_maximally_right (pms->input, &pms->cur, pms->lidx);

      // [++++++++++++______]
      // ^
      // lidx
      // [a, b, c] p [d, e, f, g, h, i]
      // ^        ^  ^
      // rcons    robs rlen
      if (!nupd_done_right (pms->input) && pms->lidx > IN_MAX_KEYS / 2)
      {
        // Shift right (limit
        // is effectively
        // "empty" because it
        // was observed so we
        // can use it as a slot
        // for next) cur ->
        // NULL cur
        // -> limit limit limit
        // -> next
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
      else
      {
        ASSERT (nupd_done_consuming_right (pms->input));
        ASSERT (nupd_done_right (pms->input) || pms->limit.mode != PHM_NONE);

        if (pms->limit.mode != PHM_NONE)
        {
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
        ASSERT (nupd_done_consuming_left (pms->input));
        ASSERT (nupd_done_left (pms->input) || pms->limit.mode != PHM_NONE);

        if (pms->limit.mode != PHM_NONE)
        {
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

static err_t
ns_rebalance_move_up_stack (struct ns_rebalance_params *pms, error *e);

static err_t
ns_rebalance_apply_to_pivot (struct ns_rebalance_params *pms, error *e)
{
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

  // Left mode
  const pgno prev_pg = in_get_prev (page_h_ro (&pms->cur));
  if (prev_pg != PGNO_NULL)
  {
    if (pgr_get_writable (
            &pms->limit,
            pms->tx,
            PG_INNER_NODE,
            in_get_prev (page_h_ro (&pms->cur)),
            pms->p,
            e
        ))
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
    if (pms->sp == 0)
    {
      if (pgr_new (&pms->cur, pms->p, pms->tx, PG_INNER_NODE, e))
      {
        goto failed;
      }

      pms->root = page_h_pgno (&pms->cur);
      pms->lidx = 0;
    }
    else
    {
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

    // Pop up the stack once
    if (pms->layer_root.isroot)
    {
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

/******************************************************************************
 * SECTION: ns_remove
 * ----------------------------------------------------------------------------
 * @brief Main remove function
 ******************************************************************************/

/*
 * Remove elements from the R+Tree with an optional stride.
 *
 * Unlike insert/write, remove must compact the byte stream in place: the gap
 * left by deleted bytes is closed by sliding the trailing data forward.  To
 * avoid re-reading pages, two cursors scan the leaf level simultaneously:
 *
 *   writer â€” the destination cursor; data is compacted into this position.
 *   reader â€” the source cursor; always ahead of (or equal to) writer.
 *
 * When writer == reader (reader.mode == PHM_NONE), there is no separation
 * yet and both cursors refer to the same page via s.writer.
 *
 * The outer loop alternates between two phases:
 *
 *   ACTIVE â€” the reader advances by [size] bytes without copying them to
 *              the writer; this is what "removes" the elements.  If
 *              params->dest is non-NULL the removed bytes are streamed out
 *              before being discarded.  After [size] bytes, bnext resets to
 *              (stride-1)*size and phase switches to SKIPPING.
 *
 *   SKIPPING â€” [size*(stride-1)] bytes are copied from reader to writer
 *              (these are the elements that must survive).  The writer
 *              page is flushed when full, and the reader page is deleted
 *              once fully consumed if it is distinct from the writer page.
 *
 * After the remove/skip loop finishes ("drain" label), all remaining reader
 * data is copied into writer pages and exhausted reader pages are deleted.
 *
 * Phase 3 validates that total_removed is a multiple of [size], then calls
 * ns_balance_and_release() and ns_rebalance() to fix up the leaf level
 * and propagate size decrements up the inner-node tree.
 */

struct remove_state
{
  // Pages
  page_h writer;
  page_h reader;

  // Indices
  p_size write_idx;
  p_size read_idx;

  // Accumulated node updates
  struct node_updates *output;

  // Pager / transaction context
  struct pager *p;
  struct txn   *tx;

  // Remove progress
  b_size total_removed;
  b_size max_remove;
  p_size bnext;

  enum stride_phase phase;
};

static page_h *
remove_creader (struct remove_state *s)
{
  if (s->reader.mode == PHM_NONE)
  {
    return &s->writer;
  }
  return &s->reader;
}

/*
 * Flush the writer page and advance to the next one.
 *
 * Sets the page's used-byte count to write_idx, records the size delta in
 * output, releases the page, then advances writer to the next page in the
 * chain (or to the current reader page if one is open).
 */
static err_t
advance_writer (struct remove_state *s, error *e)
{
  ASSERT (s->write_idx > DL_DATA_SIZE / 2);

  in_set_len (page_h_w (&s->writer), s->write_idx);
  if (nupd_commit_1st_right (s->output, pgh_unravel (&s->writer), e))
  {
    goto failed;
  }

  if (s->reader.mode == PHM_NONE)
  {
    const pgno npg = in_get_next (page_h_ro (&s->writer));

    if (pgr_release (s->p, &s->writer, PG_DATA_LIST, e))
    {
      goto failed;
    }

    if (npg != PGNO_NULL)
    {
      if (pgr_get_writable (&s->writer, s->tx, PG_DATA_LIST, npg, s->p, e))
      {
        goto failed;
      }
    }
  }
  else
  {
    if (pgr_release (s->p, &s->writer, PG_DATA_LIST, e))
    {
      goto failed;
    }
    page_h_xfer_ownership_ptr (&s->writer, &s->reader);
  }

  s->write_idx = 0;

  return SUCCESS;

failed:
  return error_trace (e);
}

/*
 * Advance the reader to the next page.
 *
 * Three cases:
 *   1. reader == writer (no separation yet): look at writer's next link;
 *      open the successor as the new reader.
 *   2. writer page is more than half full: flush writer first
 *      (advance_writer), then open the current page's next as the new reader.
 *   3. reader page is fully consumed: delete it, re-link writer â†’ next, open
 *      the next page as the new reader, and record the deletion in output.
 *
 * Sets *iseof=true and finalizes write_idx when the chain has no successor.
 */
static err_t
advance_reader (struct remove_state *s, bool *iseof, error *e)
{
  page_h next = page_h_create ();
  *iseof      = false;

  if (s->reader.mode == PHM_NONE)
  {
    const pgno npg = dlgt_get_next (page_h_ro (&s->writer));

    if (npg != PGNO_NULL)
    {
      if (pgr_get_writable (&s->reader, s->tx, PG_DATA_LIST, npg, s->p, e))
      {
        goto failed;
      }
    }
  }
  else if (s->write_idx > DL_DATA_SIZE / 2)
  {
    if (advance_writer (s, e))
    {
      goto failed;
    }

    ASSERT (page_h_pgno (&s->writer) == page_h_pgno (remove_creader (s)));

    const pgno npg = dlgt_get_next (page_h_ro (&s->writer));

    if (npg != PGNO_NULL)
    {
      if (pgr_get_writable (&s->reader, s->tx, PG_DATA_LIST, npg, s->p, e))
      {
        goto failed;
      }
    }
  }
  else
  {
    const pgno rpg = page_h_pgno (&s->reader);
    const pgno npg = in_get_next (page_h_ro (&s->reader));

    if (npg != PGNO_NULL)
    {
      if (pgr_get_writable (&next, s->tx, PG_DATA_LIST, npg, s->p, e))
      {
        goto failed;
      }
    }

    if (pgr_delete_and_release (s->p, s->tx, &s->reader, e))
    {
      goto failed;
    }

    dlgt_link (page_h_w (&s->writer), page_h_w_or_null (&next));
    page_h_xfer_ownership_ptr (&s->reader, &next);

    if (nupd_append_2nd_right (s->output, pgh_unravel (&s->writer), rpg, 0, e))
    {
      goto failed;
    }
  }

  if (s->reader.mode == PHM_NONE)
  {
    in_set_len (page_h_w (&s->writer), s->write_idx);
    s->read_idx = s->write_idx;
    *iseof      = true;
  }
  else
  {
    s->read_idx = 0;
  }

  return SUCCESS;

failed:
  pgr_cancel_if_exists (s->p, &next);
  return error_trace (e);
}

static p_size
removing_next (const struct remove_state *s, const page *sro)
{
  p_size next = s->bnext;
  next        = MIN (next, dl_used (sro) - s->read_idx);

  if (s->max_remove > 0)
  {
    next = MIN (next, s->max_remove - s->total_removed);
  }

  return next;
}

static p_size
skipping_next (const struct remove_state *s, const page *sro)
{
  p_size next = s->bnext;
  next        = MIN (next, DL_DATA_SIZE - s->write_idx);
  next        = MIN (next, dl_used (sro) - s->read_idx);
  return next;
}

static p_size
drain_reader_next (const struct remove_state *s, const page *sro)
{
  p_size next = DL_DATA_SIZE - s->write_idx;
  next        = MIN (next, dl_used (sro) - s->read_idx);
  return next;
}

/*
 * Remove elements from the R+Tree at the byte offset given by params->bofst.
 *
 * Seeks to the target data-list page, then compacts the leaf level in place
 * using a dual-cursor (writer/reader) scan.  Balances the leaf and propagates
 * size decrements up the inner-node tree via ns_rebalance().
 *
 * params->root is updated in place if the root changes.
 */
sb_size
ns_remove (struct ns_remove_params *params, error *e)
{
  struct remove_state s = {
      .writer        = page_h_create (),
      .reader        = page_h_create (),
      .write_idx     = 0,
      .read_idx      = 0,
      .total_removed = 0,
      .max_remove    = params->nelem * params->size,
      .bnext         = params->size,
      .phase         = ACTIVE,
      .output        = NULL,
      .p             = params->p,
      .tx            = params->tx,
  };

  page_h prev = page_h_create ();
  page_h next = page_h_create ();

  struct node_updates *rb_nupd2 = NULL;
  struct three_in_pair tip_out;
  struct root_update   root = {0};

  struct ns_seek_params seek = {
      .p          = params->p,
      .tx         = params->tx,
      .root       = params->root,
      .bofst      = params->bofst,
      .save_stack = true,
      .sp         = 0,
  };

  if (params->root == PGNO_NULL)
  {
    return 0;
  }

  if (ns_seek (&seek, e))
  {
    goto failed;
  }

  s.writer    = page_h_xfer_ownership (&seek.pg);
  s.write_idx = seek.lidx;

  s.output =
      nupd_init (page_h_pgno (&s.writer), dl_used (page_h_ro (&s.writer)), e);
  if (s.output == NULL)
  {
    goto failed;
  }

  s.read_idx = s.write_idx;

  // Phase 1: Remove / Skip
  while (s.max_remove == 0 || s.total_removed < s.max_remove)
  {
    const page *sro  = page_h_ro (remove_creader (&s));
    p_size      rlen = dl_used (sro);

    switch (s.phase)
    {
      case ACTIVE:
      {
        p_size next_amount = removing_next (&s, sro);

        if (next_amount == 0)
        {
          ASSERT (s.read_idx == rlen);

          bool iseof;
          if (advance_reader (&s, &iseof, e))
          {
            goto failed;
          }

          if (iseof)
          {
            goto drain;
          }

          continue;
        }

        if (params->dest)
        {
          i32 written = stream_bwrite (
              (u8 *)dl_get_data (sro) + s.read_idx,
              1,
              next_amount,
              params->dest,
              e
          );

          if (written < 0)
          {
            goto failed;
          }
          ASSERT ((p_size)written == next_amount);
        }

        s.read_idx += next_amount;
        s.total_removed += next_amount;
        s.bnext -= next_amount;

        if (s.bnext == 0)
        {
          s.bnext = params->size * (params->stride - 1);
          if (s.bnext > 0)
          {
            s.phase = SKIPPING;
          }
          else
          {
            s.bnext = params->size;
          }
        }

        if (s.max_remove > 0 && s.total_removed == s.max_remove)
        {
          goto drain;
        }

        break;
      }

      case SKIPPING:
      {
        p_size next_amount = skipping_next (&s, sro);

        if (next_amount == 0)
        {
          if (s.read_idx == rlen)
          {
            bool iseof;
            if (advance_reader (&s, &iseof, e))
            {
              goto failed;
            }

            if (iseof)
            {
              goto drain;
            }

            continue;
          }
          else if (s.write_idx == DL_DATA_SIZE)
          {
            if (advance_writer (&s, e))
            {
              goto failed;
            }

            continue;
          }

          UNREACHABLE ();
        }

        dl_dl_memmove_permissive (
            page_h_w (&s.writer),
            page_h_ro (remove_creader (&s)),
            s.write_idx,
            s.read_idx,
            next_amount
        );

        s.write_idx += next_amount;
        s.read_idx += next_amount;
        s.bnext -= next_amount;

        if (s.bnext == 0)
        {
          s.bnext = params->size;
          s.phase = ACTIVE;
        }

        break;
      }
    }

    if (params->dest && stream_isdone (params->dest))
    {
      goto drain;
    }
  }

drain:
  // Phase 2: Drain remaining reader pages into writer

  while (true)
  {
    const page *sro  = page_h_ro (remove_creader (&s));
    p_size      rlen = dl_used (sro);

    p_size next_amount = drain_reader_next (&s, sro);

    if (next_amount == 0)
    {
      if (s.read_idx == rlen)
      {
        dl_set_used (page_h_w (&s.writer), s.write_idx);

        if (s.reader.mode != PHM_NONE)
        {
          pgno rpg = page_h_pgno (&s.reader);
          pgno npg = in_get_next (page_h_ro (&s.reader));

          if (npg != PGNO_NULL)
          {
            if (pgr_get_writable (
                    &next,
                    params->tx,
                    PG_DATA_LIST,
                    npg,
                    params->p,
                    e
                ))
            {
              goto failed;
            }
          }

          if (pgr_delete_and_release (params->p, params->tx, &s.reader, e))
          {
            goto failed;
          }

          dlgt_link (page_h_w (&s.writer), page_h_w_or_null (&next));
          page_h_xfer_ownership_ptr (&s.reader, &next);

          if (nupd_append_2nd_right (
                  s.output,
                  pgh_unravel (&s.writer),
                  rpg,
                  0,
                  e
              ))
          {
            goto failed;
          }
          s.read_idx = 0;

          if (s.reader.mode == PHM_NONE)
          {
            break;
          }

          continue;
        }
        else
        {
          break;
        }
      }
      else if (s.write_idx >= DL_DATA_SIZE)
      {
        if (advance_writer (&s, e))
        {
          goto failed;
        }
        continue;
      }
      else
      {
        UNREACHABLE ();
      }
    }

    dl_dl_memmove_permissive (
        page_h_w (&s.writer),
        page_h_ro (remove_creader (&s)),
        s.write_idx,
        s.read_idx,
        next_amount
    );

    s.write_idx += next_amount;
    s.read_idx += next_amount;
  }

  // Phase 3: Validate, balance, rebalance

  if (s.total_removed % params->size != 0)
  {
    error_causef (
        e,
        ERR_CORRUPT,
        "removed %" PRb_size
        " bytes, not a multiple of element size %" PRb_size,
        s.total_removed,
        params->size
    );
    goto failed;
  }

  next = page_h_xfer_ownership (&s.reader);

  struct ns_balance_and_release_params bparams = {
      .p      = params->p,
      .tx     = params->tx,
      .output = &tip_out,
      .root   = &root,
      .prev   = &prev,
      .cur    = &s.writer,
      .next   = &next,
  };

  if (ns_balance_and_release (bparams, e))
  {
    goto failed;
  }

  if (nupd_append_tip_right (s.output, tip_out, e))
  {
    goto failed;
  }

  struct ns_rebalance_params rebalance = {
      .p          = params->p,
      .tx         = params->tx,
      .root       = params->root,
      .pstack     = seek.pstack,
      .sp         = seek.sp,
      .input      = rb_nupd2,
      .output     = s.output,
      .layer_root = root,
  };

  rb_nupd2 = NULL;
  s.output = NULL;

  err_t ret = ns_rebalance (&rebalance, e);

  if (rebalance.output)
  {
    nupd_free (rebalance.output);
  }
  if (rebalance.input)
  {
    nupd_free (rebalance.input);
  }

  if (ret)
  {
    goto failed;
  }

  params->root = rebalance.root;

  return (sb_size)(s.total_removed / params->size);

failed:
  pgr_cancel_if_exists (params->p, &prev);
  pgr_cancel_if_exists (params->p, &s.writer);
  pgr_cancel_if_exists (params->p, &next);
  pgr_cancel_if_exists (params->p, &s.reader);

  if (rb_nupd2)
  {
    nupd_free (rb_nupd2);
  }
  if (s.output)
  {
    nupd_free (s.output);
  }

  for (u32 i = 0; i < seek.sp; ++i)
  {
    pgr_cancel_if_exists (params->p, &seek.pstack[i].pg);
  }

  return error_trace (e);
}

/******************************************************************************
 * SECTION: rptree_valid
 * ----------------------------------------------------------------------------
 * @brief Verifies if a rptree is valid or not
 ******************************************************************************/

struct frame
{
  pgno         pg;
  struct hnode node;
};

struct rptree_valid_ctx
{
  struct slab_alloc pg_alloc;
  struct htable    *table;
};

static bool
frame_eq (const struct hnode *left, const struct hnode *right)
{
  const struct frame *_left  = container_of (left, struct frame, node);
  const struct frame *_right = container_of (right, struct frame, node);
  return _left->pg == _right->pg;
}

static err_t
rptree_valid_recursive (
    struct pager            *p,
    const pgno               pg,
    bool                     isroot,
    const b_size             exp_size,
    struct rptree_valid_ctx *ctx,
    error                   *e
)
{
  // Check if this page was double counted
  struct frame key = {
      .pg = pg,
  };
  hnode_init (&key.node, (u32)pg);
  struct hnode **dup = htable_lookup (ctx->table, &key.node, frame_eq);
  if (dup != NULL)
  {
    error_causef (e, ERR_CORRUPT, "Page: %" PRpgno " was double counted", pg);
    goto failed;
  }
  else
  {
    struct frame *f = slab_alloc_alloc (&ctx->pg_alloc, e);
    if (f == NULL)
    {
      goto failed;
    }
    f->pg = pg;
    hnode_init (&f->node, pg);
    htable_insert (ctx->table, &f->node);
  }

  // Validate this page
  page_h pivot = page_h_create ();
  if (pgr_get (&pivot, PG_DATA_LIST | PG_VAR_PAGE, pg, p, e))
  {
    goto failed;
  }

  // Check that root matches
  if (isroot != dlgt_is_root (page_h_ro (&pivot)))
  {
    error_causef (e, ERR_CORRUPT, "page %" PRpgno " should be non-root", pg);
    goto failed;
  }

  // Check that size matches
  if (dlgt_get_size (page_h_ro (&pivot)) != exp_size)
  {
    error_causef (
        e,
        ERR_CORRUPT,
        "page %" PRpgno ": expected %" PRb_size " bytes, got %" PRb_size,
        pg,
        exp_size,
        in_get_size (page_h_ro (&pivot))
    );
    goto failed;
  }

  switch (page_h_type (&pivot))
  {
    case PG_DATA_LIST:
    {
      if (pgr_release (p, &pivot, PG_DATA_LIST, e))
      {
        goto failed;
      }
      return error_trace (e);
    }
    case PG_VAR_PAGE:
    {
      struct in_pair       nodes[IN_MAX_KEYS];
      const struct in_data data = in_get_data (page_h_ro (&pivot), nodes);

      if (pgr_release (p, &pivot, PG_DATA_LIST, e))
      {
        goto failed;
      }

      // Validate each child
      for (u32 i = 0; i < data.len; ++i)
      {
        if (rptree_valid_recursive (
                p,
                data.nodes[i].pg,
                false,
                data.nodes[i].key,
                ctx,
                e
            ))
        {
          goto failed;
        }
      }

      return error_trace (e);
    }
    default:
    {
      UNREACHABLE ();
    }
  }

failed:
  pgr_cancel_if_exists (p, &pivot);
  return error_trace (e);
}

err_t
ns_rptree_valid (
    struct pager *db,
    const pgno    rpt_root,
    const b_size  nbytes,
    error        *e
)
{
  struct rptree_valid_ctx ctx;
  slab_alloc_init (&ctx.pg_alloc, sizeof (struct frame), 512);

  ctx.table = htable_create (1000, e);
  if (ctx.table == NULL)
  {
    return error_trace (e);
  }

  if (rptree_valid_recursive (db, rpt_root, true, nbytes, &ctx, e))
  {
    goto theend;
  }

theend:
  htable_free (ctx.table);
  slab_alloc_destroy (&ctx.pg_alloc);
  return error_trace (e);
}

/******************************************************************************
 * SECTION: ns_seek
 * ----------------------------------------------------------------------------
 * @brief Seek to a location on a tree
 ******************************************************************************/

/*
 * Descend the R+Tree to the data-list page containing byte offset [bofst].
 *
 * Starting from the root, the tree is traversed level by level.  At each
 * inner node, in_choose_lidx() identifies the child subtree that contains
 * [bofst] and subtracts the cumulative key total for all preceding subtrees
 * from the remaining offset, so that [bofst] is expressed relative to the
 * chosen subtree.  This is the standard R+Tree prefix-sum descent.
 *
 * If [save_stack] is true, each inner node is pushed onto pstack before
 * following its child; this gives the caller a traversal path for rebalancing
 * without re-reading any pages.  If [save_stack] is false, each inner node
 * is released immediately after the child page is fetched.
 *
 * When the descent lands on a data-list page, lidx is set to
 * MIN(bofst, dl_used()) â€” it saturates at the page's current content length,
 * meaning seeks past EOF land at the end of the last page rather than
 * producing an error.  This is intentional: inserts after EOF are valid.
 */
err_t
ns_seek (struct ns_seek_params *a, error *e)
{
  page_h next = page_h_create ();
  a->pg       = page_h_create ();
  a->sp       = 0;
  a->lidx     = 0;

  if (pgr_get_maybe_writable (
          &a->pg,
          a->tx,
          PG_DATA_LIST | PG_INNER_NODE,
          a->root,
          a->p,
          a->save_stack,
          e
      ))
  {
    goto failed;
  }

  while (true)
  {
    switch (page_h_type (&a->pg))
    {
      case PG_INNER_NODE:
      {
        // Stack overflow
        if (a->sp == 20)
        {
          error_causef (
              e,
              ERR_RPTREE_PAGE_STACK_OVERFLOW,
              "page stack overflow (depth 20)"
          );
          goto failed;
        }

        // Make decision
        b_size nleft;
        in_choose_lidx (&a->lidx, &nleft, page_h_ro (&a->pg), a->bofst);
        ASSERT (nleft <= a->bofst);
        a->bofst -= nleft;

        // Fetch that next page
        const pgno npg = in_get_leaf (page_h_ro (&a->pg), a->lidx);
        if (pgr_get_maybe_writable (
                &next,
                a->tx,
                PG_DATA_LIST | PG_INNER_NODE,
                npg,
                a->p,
                a->save_stack,
                e
            ))
        {
          goto failed;
        }

        // Append a->pg to the stack
        if (a->save_stack)
        {
          a->pstack[(a->sp)++] = (struct seek_v){
              .pg   = page_h_xfer_ownership (&a->pg),
              .lidx = a->lidx,
          };
        }
        else
        {
          if (pgr_release (a->p, &a->pg, PG_INNER_NODE, e))
          {
            goto failed;
          }
        }

        // Trade a->pg
        a->pg = page_h_xfer_ownership (&next);
        break;
      }

      case PG_DATA_LIST:
      {
        const p_size used = dl_used (page_h_ro (&a->pg));
        a->lidx           = MIN (a->bofst, used);
        return SUCCESS;
      }

      default:
      {
        UNREACHABLE ();
      }
    }
  }

failed:
  // Release used pages
  pgr_cancel_if_exists (a->p, &a->pg);
  pgr_cancel_if_exists (a->p, &next);
  for (u32 i = 0; i < a->sp; ++i)
  {
    pgr_cancel_if_exists (a->p, &a->pstack[i].pg);
  }
  a->sp = 0;
  return error_trace (e);
}

/******************************************************************************
 * SECTION: ns_write
 * ----------------------------------------------------------------------------
 * @brief Overwrite data - don't insert
 ******************************************************************************/

/*
 * Compute how many bytes to overwrite starting at the current page position.
 *
 * Mirrors ns_read_next_amount exactly: minimum of bytes remaining on the
 * page from lidx, bytes remaining in the current write/skip window (bnext),
 * and â€” in ACTIVE state only â€” bytes remaining before the global write
 * limit (max_bwrite).  Skip-window bytes do not count toward max_bwrite.
 */
static t_size
ns_write_next_amount (
    const page             *curp,
    const t_size            lidx,
    const b_size            bnext,
    const b_size            max_bwrite,
    const b_size            total_bwrite,
    const enum stride_phase state
)
{
  // Available in the current page
  p_size next_amount = dl_used (curp) - lidx;

  // Available in this write state
  next_amount = MIN (next_amount, bnext);

  // Available globally to write
  if (max_bwrite > 0 && state == ACTIVE)
  {
    next_amount = MIN (next_amount, max_bwrite - total_bwrite);
  }

  return next_amount;
}

// TODO - (4) tighten up the while loop to loop inside a page - rather than one
// read per loop
/*
 * Overwrite elements in the R+Tree with an optional stride, scanning forward.
 *
 * Structurally identical to ns_read_forward, but writes to the data-list
 * chain instead of reading from it.  The tree size does not change: write
 * overwrites existing bytes in place and stops at EOF rather than extending
 * the chain.  Requires pgr_make_writable() on each page before writing.
 *
 * ACTIVE state: pulls bytes from params->src and stamps them over the page
 * data starting at lidx.  SKIPPING state: advances lidx without reading from
 * src, leaving those bytes untouched.  When stride == 1 the SKIPPING phase
 * has zero bytes and is bypassed.
 *
 * Returns the number of complete elements written, not bytes.  Returns 0
 * immediately if root == PGNO_NULL (empty tree).
 */
static sb_size
ns_write_forward (const struct ns_write_params params, error *e)
{
  ASSERT (params.stride > 0);

  page_h       cur          = page_h_create ();
  page_h       next         = page_h_create ();
  p_size       lidx         = 0; // Local index on the current page
  b_size       total_bwrite = 0;
  const b_size max_bwrite   = params.size * params.nelem;
  b_size       bnext        = params.size;

  struct ns_seek_params seek = {
      .p          = params.p,
      .tx         = params.tx,
      .root       = params.root,
      .bofst      = params.bofst,
      .save_stack = false,
      .sp         = 0,
  };

  enum stride_phase state = ACTIVE;

  // Nothing to do
  if (params.root == PGNO_NULL)
  {
    return 0;
  }

  // Otherwise seek
  else
  {
    if (ns_seek (&seek, e))
    {
      goto failed;
    }

    // Transition from Seeked -> inserting
    cur  = page_h_xfer_ownership (&seek.pg);
    lidx = seek.lidx;

    // Upgrade to X lock
    if (pgr_upgrade (&cur, params.tx, PG_DATA_LIST, params.p, e))
    {
      goto failed;
    }
  }

  page *curp = page_h_w (&cur);

  enum
  {
    HIT_MAX_WRITE,
    SRC_DONE_WRITING,
    DATA_EXHAUSTED,
  } termination = HIT_MAX_WRITE;

  while (max_bwrite == 0 || total_bwrite < max_bwrite)
  {
    p_size next_amount = ns_write_next_amount (
        curp,
        lidx,
        bnext,
        max_bwrite,
        total_bwrite,
        state
    );

    if (next_amount == 0)
    {
      // Reached end of current page, advance
      // to next
      if (lidx >= dl_used (curp))
      {
        const pgno npg = dlgt_get_next (curp);

        if (npg != PGNO_NULL)
        {
          WRAP (pgr_get_writable (
              &next,
              params.tx,
              PG_DATA_LIST,
              npg,
              params.p,
              e
          ));
        }

        // Reached EOF
        else
        {
          termination = DATA_EXHAUSTED;
          goto done;
        }

        WRAP (pgr_release (params.p, &cur, PG_DATA_LIST, e));

        lidx = 0;
        cur  = page_h_xfer_ownership (&next);

        curp = page_h_w (&cur);

        next_amount = ns_write_next_amount (
            curp,
            lidx,
            bnext,
            max_bwrite,
            total_bwrite,
            state
        );

        ASSERT (next_amount > 0);
      }

      else
      {
        UNREACHABLE ();
      }
    }

    switch (state)
    {
      case ACTIVE:
      {
        if (next_amount > 0)
        {
          // Pull bytes from caller's source stream and
          // stamp them into the page
          const sp_size write = stream_bread (
              (u8 *)dl_get_data (curp) + lidx,
              1,
              next_amount,
              params.src,
              e
          );

          if (write < 0)
          {
            goto failed;
          }

          lidx += write;
          total_bwrite += write;
          bnext -= write;
        }

        if (bnext == 0)
        {
          // Finished writing one element; transition to
          // skip (stride-1) elements
          bnext = (params.stride - 1) * params.size;
          state = SKIPPING;

          // TODO - (5)
          // Optimize
          // stride = 1: skip window is zero, stay ACTIVE
          if (bnext == 0)
          {
            bnext = params.size;
            state = ACTIVE;
          }
        }
        break;
      }

      case SKIPPING:
      {
        if (next_amount > 0)
        {
          // Advance lidx without writing; NULL src means
          // leave bytes untouched
          const p_size write = dl_write (curp, NULL, lidx, next_amount);
          lidx += write;
          bnext -= write;
        }

        if (bnext == 0)
        {
          // Skip window exhausted; start writing the
          // next element
          bnext = params.size;
          state = ACTIVE;
        }
        break;
      }

      default:
      {
        UNREACHABLE ();
      }
    }

    // Caller's source exhausted before we hit the byte limit
    if (stream_isdone (params.src))
    {
      termination = SRC_DONE_WRITING;
      break;
    }
  }

done:

  // Release current page
  WRAP (pgr_release (params.p, &cur, PG_DATA_LIST, e));

  // Validate we write complete elements
  if (total_bwrite % params.size != 0)
  {
    error_causef (
        e,
        ERR_CORRUPT,
        "wrote %" PRb_size
        " bytes, not a multiple of element size "
        "%" PRt_size,
        total_bwrite,
        params.size
    );
    goto failed;
  }

  return total_bwrite / params.size;

failed:
  pgr_cancel_if_exists (params.p, &cur);
  pgr_cancel_if_exists (params.p, &next);
  return error_trace (e);
}

static sb_size
ns_write_backward (const struct ns_write_params params, error *e)
{
  panic ("TODO - (12)");
  return 0;
}

sb_size
ns_write (const struct ns_write_params params, error *e)
{
  if (params.stride > 0)
  {
    return ns_write_forward (params, e);
  }
  else if (params.stride < 0)
  {
    return ns_write_backward (params, e);
  }
  else
  {
    return error_causef (e, ERR_INVALID_ARGUMENT, "write stride is 0");
  }
}
