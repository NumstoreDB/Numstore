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

#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_numerics.h"
#include "core/ns_stdtypes.h"
#include "core/testing/ns_testing.h"
#include "nscore/algorithms/rope/ns_rope_algorithms_internal.h"
#include "nscore/page/ns_page.h"
#include "nscore/page/ns_page_data_list.h"
#include "nscore/page/ns_page_delegate.h"
#include "nscore/page/ns_page_h.h"
#include "nscore/page/ns_page_inner_node.h"
#include "nscore/pager/ns_pager.h"
#include "nscore/testing/ns_page_fixture.h"
#include "nscore/txn_table/ns_txn_table.h"

#include <stdbool.h>
#include <stddef.h>

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
  if (cur_len == 0) {
    return;
  }

  // Also valid
  if (cur_len >= maxlen / 2) {
    return;
  }

  // There's enough data to balaance max / 2 for each node
  if (prev_len + cur_len >= maxlen) {
    dlgt_move_right (page_h_w (prev), page_h_w (cur), (maxlen / 2) - cur_len);
    return;
  }

  // Move all the data left to prev
  dlgt_move_left (page_h_w (prev), page_h_w (cur), cur_len);
}

#ifdef TESTING
TEST (dlgt_balance_with_prev)
{
  struct pgr_fixture f;
  error             *e = &f.e;
  pgr_fixture_create (&f);

  u8 _prev[DL_DATA_SIZE];
  u8 _cur[DL_DATA_SIZE];
  u32_arr_rand (_prev);
  u32_arr_rand (_cur);

  struct ns_txn tx;
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
    for (; i < DL_DATA_SIZE / 2 + DL_REM; ++i) {
      test_assert_equal (dl_get_byte (page_h_ro (prev), i), _prev[i]);
    }
    i = 0;
    for (; i < DL_DATA_SIZE - 10 - DL_DATA_SIZE / 2 - DL_REM; ++i) {
      test_assert_equal (dl_get_byte (page_h_ro (cur), i), _prev[DL_DATA_SIZE / 2 + DL_REM + i]);
    }
    const u32 k = i;
    for (; i < DL_DATA_SIZE / 2; ++i) {
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
    for (; i < DL_DATA_SIZE - 10; ++i) {
      test_assert_equal (dl_get_byte (page_h_ro (prev), i), _prev[i]);
    }
    const u32 k = i;
    for (; i < 9; ++i) {
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
  if (cur_len == 0) {
    return;
  }

  if (cur_len >= maxlen / 2) {
    return;
  }

  if (next_len + cur_len >= maxlen) {
    dlgt_move_left (page_h_w (cur), page_h_w (next), (maxlen / 2) - cur_len);
    return;
  }

  dlgt_move_right (page_h_w (cur), page_h_w (next), cur_len);
}

#ifdef TESTING
TEST (dlgt_balance_with_next)
{
  struct pgr_fixture f;
  error             *e = &f.e;
  pgr_fixture_create (&f);

  u8 _cur[DL_DATA_SIZE];
  u8 _next[DL_DATA_SIZE];
  u32_arr_rand (_next);
  u32_arr_rand (_cur);

  struct ns_txn tx;
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
    for (; i < 10; ++i) {
      test_assert_equal (dl_get_byte (page_h_ro (cur), i), _cur[i]);
    }
    for (; i < DL_DATA_SIZE / 2; ++i) {
      test_assert_equal (dl_get_byte (page_h_ro (cur), i), _next[i - 10]);
    }
    i = 0;
    for (; i < DL_DATA_SIZE / 2 + DL_REM; ++i) {
      test_assert_equal (dl_get_byte (page_h_ro (next), i), _next[i + DL_DATA_SIZE / 2 - 10]);
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
    for (; i < 9; ++i) {
      test_assert_equal (dl_get_byte (page_h_ro (next), i), _cur[i]);
    }

    // next data
    for (; i < DL_DATA_SIZE - 1; ++i) {
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

  if (prev) {
    ret.prev = in_pair_from_pgh (prev);
  }
  if (cur) {
    ret.cur = in_pair_from_pgh (cur);
  }
  if (next) {
    ret.next = in_pair_from_pgh (next);
  }

  return ret;
}

/*
 * Tries to balance with page next or prev that
 * is loaded into memory
 *
 * if neither is loaded, tries again after
 * loading them.
 *
 * Takes preference towards balancing with nodes
 * that are already loaded in memory
 */
static err_t
ns_balance_with_next_or_prev (
    page_h               *prev,
    page_h               *cur,
    page_h               *next,
    struct pager         *p,
    struct ns_txn        *tx,
    struct three_in_pair *output,
    error                *e
)
{
  int    flags = PG_INNER_NODE | PG_DATA_LIST;
  p_size csize = dlgt_get_len (page_h_ro (cur));
  *output      = three_in_pair_from (NULL, cur, NULL);

  // Cur needs balancing because it is less than maxlen / 2
  if (csize > 0 && csize < dlgt_get_max_len (page_h_ro (cur)) / 2) {
    // If next is loaded - balance with it
    if (next->mode != PHM_NONE) {
      dlgt_balance_with_next (cur, next);
      *output = three_in_pair_from (NULL, cur, next);
    }

    // If prev is present - balance with it
    else if (prev->mode != PHM_NONE) {
      dlgt_balance_with_prev (prev, cur);
      *output = three_in_pair_from (prev, cur, NULL);
    }

    // None were pre loaded - try again with next by loading it into memory
    else if (dlgt_get_next (page_h_ro (cur)) != PGNO_NULL) {
      pgno npg = dlgt_get_next (page_h_ro (cur));
      WRAP (pgr_get_writable (next, tx, flags, npg, p, e));
      dlgt_balance_with_next (cur, next);
      *output = three_in_pair_from (NULL, cur, next);
    }

    // Next isn't present - try again with next by loading it into memory
    else if (dlgt_get_prev (page_h_ro (cur)) != PGNO_NULL) {
      pgno ppg = dlgt_get_prev (page_h_ro (cur));
      WRAP (pgr_get_writable (prev, tx, flags, ppg, p, e));
      dlgt_balance_with_prev (prev, cur);
      *output = three_in_pair_from (prev, cur, NULL);
    } else {
      // This balance was performed on a root  node
      ASSERT (dlgt_is_root (page_h_ro (cur)));
    }
  } else {
    // there's no need to balance
  }

  return SUCCESS;
}

/*
 * Tries to balance with page next or prev that
 * is loaded into memory
 *
 * if neither is loaded, tries again after
 * loading them.
 *
 * Takes preference towards balancing with nodes
 * that are already loaded in memory
 */
static err_t
ns_maybe_delete_cur (
    page_h             *prev,
    page_h             *cur,
    page_h             *next,
    struct pager       *p,
    struct ns_txn      *tx,
    struct root_update *root,
    error              *e
)
{
  int flags    = PG_INNER_NODE | PG_DATA_LIST;

  root->isroot = false;
  if (dlgt_is_root (page_h_ro (cur))) {
    root->isroot = true;
    root->root   = page_h_pgno (cur);
  }

  // Need to delete cur
  if (dlgt_get_len (page_h_ro (cur)) == 0) {
    // Fetch prev and next for link re writing
    if (!root->isroot) {
      // Load prev sibling if the caller did not already pin it
      if (prev->mode == PHM_NONE) {
        pgno ppg = dlgt_get_prev (page_h_ro (cur));
        if (ppg != PGNO_NULL) {
          WRAP (pgr_get_writable (prev, tx, flags, ppg, p, e));
        }
      }

      // Load next sibling if the caller did not already pin it
      if (next->mode == PHM_NONE) {
        const pgno npg = dlgt_get_next (page_h_ro (cur));
        if (npg != PGNO_NULL) {
          WRAP (pgr_get_writable (next, tx, flags, npg, p, e));
        }
      }

      // Bridge the gap: prev->next = next, next->prev = prev
      dlgt_link (page_h_w_or_null (prev), page_h_w_or_null (next));

      // We might have turned prev / next into a new root by deleting cur
      if (prev->mode != PHM_NONE && dlgt_is_root (page_h_ro (prev))) {
        root->root   = page_h_pgno (prev);
        root->isroot = true;
      } else if (next->mode != PHM_NONE && dlgt_is_root (page_h_ro (next))) {
        root->root   = page_h_pgno (next);
        root->isroot = true;
      }
    }

    // Otherwise cur is still root but we will delete it so now it's NULL
    else {
      // balance performed on root and deleted
      root->root = PGNO_NULL;
    }

    WRAP (pgr_delete_and_release (p, tx, cur, e));
  }

  return SUCCESS;
}

// TODO - graceful error handling and clean up of partial pages
err_t
ns_balance_and_release (struct ns_balance_and_release_params params, error *e)
{
  ASSERT (params.output);

  // First - do the balance -
  // e.g. transfer data between prev / cur / next
  // to make all nodes valid
  if (ns_balance_with_next_or_prev (
          params.prev,
          params.cur,
          params.next,
          params.p,
          params.tx,
          params.output,
          e
      )) {
    return error_trace (e);
  }

  // Clean up - delete cur if needed
  if (ns_maybe_delete_cur (
          params.prev,
          params.cur,
          params.next,
          params.p,
          params.tx,
          params.root,
          e
      )) {
    return error_trace (e);
  }

  int flags = PG_INNER_NODE | PG_DATA_LIST;

  // One final common cleanup
  WRAP (pgr_release_if_exists (params.p, params.prev, flags, e));
  WRAP (pgr_release_if_exists (params.p, params.cur, flags, e));
  WRAP (pgr_release_if_exists (params.p, params.next, flags, e));

  return SUCCESS;
}
