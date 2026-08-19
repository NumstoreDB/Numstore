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
#include "core/ns_stream.h"
#include "core/ns_utils.h"
#include "core/testing/ns_testing.h"
#include "nscore/algorithms/ns_rope_algorithms.h"
#include "nscore/ns_node_updates.h"
#include "nscore/ns_page_fixture.h"
#include "nscore/ns_page_h.h"
#include "nscore/page/ns_page.h"
#include "nscore/page/ns_page_data_list.h"
#include "nscore/page/ns_page_delegate.h"
#include "nscore/page/ns_page_inner_node.h"
#include "nscore/pager/ns_pager.h"

#include <stdbool.h>
#include <string.h>

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
  page_h                prev = page_h_create ();
  page_h                cur  = page_h_create ();
  page_h                next = page_h_create ();

  u8                    temp_buf[DL_DATA_SIZE];
  p_size                tbw      = 0;
  p_size                tbl      = 0;

  struct node_updates  *output   = NULL;
  struct node_updates  *rb_nupd2 = NULL;
  struct three_in_pair  tip_out;
  struct root_update    root;

  p_size                lidx          = 0;
  b_size                total_written = 0;

  struct ns_seek_params seek          = {
      .p          = params->p,
      .tx         = params->tx,
      .root       = params->root,
      .bofst      = params->bofst,
      .save_stack = true,
      .sp         = 0,
  };

  if (params->root == PGNO_NULL) {
    if (pgr_new (&cur, params->p, params->tx, PG_DATA_LIST, e)) {
      goto failed;
    }

    params->root = page_h_pgno (&cur);
  } else {
    if (ns_seek (&seek, e)) {
      goto failed;
    }

    cur  = page_h_xfer_ownership (&seek.pg);
    lidx = seek.lidx;
  }

  pgno last = dl_get_next (page_h_ro (&cur));
  tbl       = dl_read_out_from (page_h_w (&cur), temp_buf, lidx);
  output    = nupd_init (page_h_pgno (&cur), 0, params->p->mem, e);
  if (output == NULL) {
    goto failed;
  }

  while (params->bytes == 0 || total_written < params->bytes) {
    p_size avail = dl_avail (page_h_ro (&cur));

    if (avail == 0) {
      ASSERT (lidx == DL_DATA_SIZE);

      if (pgr_new (&next, params->p, params->tx, PG_DATA_LIST, e)) {
        goto failed;
      }

      dl_set_next (page_h_w (&cur), page_h_pgno (&next));
      dl_set_prev (page_h_w (&next), page_h_pgno (&cur));

      if (nupd_commit_1st_right (output, pgh_unravel (&cur), e)) {
        goto failed;
      }

      if (pgr_release (params->p, &cur, PG_DATA_LIST, e)) {
        goto failed;
      }

      cur   = page_h_xfer_ownership (&next);
      lidx  = 0;
      avail = dl_avail (page_h_ro (&cur));
    }

    p_size next_amount;
    if (params->bytes == 0) {
      next_amount = avail;
    } else {
      ASSERT (params->bytes >= total_written);
      next_amount = MIN (avail, (p_size)(params->bytes - total_written));
    }

    i32 written = stream_bread (dl_avail_data (page_h_w (&cur)), 1, next_amount, params->src, e);
    if (written < 0) {
      goto failed;
    }

    if (written == 0 && stream_isdone (params->src)) {
      break;
    }

    dl_set_used (page_h_w (&cur), dl_used (page_h_ro (&cur)) + written);
    lidx += (p_size)written;
    total_written += (b_size)written;
  }

  while (tbw < tbl) {
    p_size written = dl_append (page_h_w (&cur), temp_buf + tbw, tbl - tbw);

    lidx += written;
    tbw += written;

    if (lidx == DL_DATA_SIZE && tbw < tbl) {
      ASSERT (lidx == DL_DATA_SIZE);

      if (pgr_new (&next, params->p, params->tx, PG_DATA_LIST, e)) {
        goto failed;
      }

      dl_set_next (page_h_w (&cur), page_h_pgno (&next));
      dl_set_prev (page_h_w (&next), page_h_pgno (&cur));

      if (nupd_commit_1st_right (output, pgh_unravel (&cur), e)) {
        goto failed;
      }

      if (pgr_release (params->p, &cur, PG_DATA_LIST, e)) {
        goto failed;
      }

      page_h_xfer_ownership_ptr (&cur, &next);
      lidx = 0;
    }
  }

  if (last != PGNO_NULL && last != dl_get_next (page_h_ro (&cur))) {
    if (pgr_get_writable (&next, params->tx, PG_DATA_LIST, last, params->p, e)) {
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

  if (ns_balance_and_release (bparams, e)) {
    goto failed;
  }

  if (nupd_append_tip_right (output, tip_out, e)) {
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

  output    = NULL;
  rb_nupd2  = NULL;

  err_t ret = ns_rebalance (&rebalance, e);

  if (rebalance.output) {
    nupd_free (rebalance.output);
  }
  if (rebalance.input) {
    nupd_free (rebalance.input);
  }

  if (ret) {
    goto failed;
  }

  params->root = rebalance.root;

  return (sb_size)total_written;

failed:
  pgr_cancel_if_exists (params->p, &prev);
  pgr_cancel_if_exists (params->p, &cur);
  pgr_cancel_if_exists (params->p, &next);

  if (output) {
    nupd_free (output);
  }
  if (rb_nupd2) {
    nupd_free (rb_nupd2);
  }

  for (u32 i = 0; i < seek.sp; ++i) {
    pgr_cancel_if_exists (params->p, &seek.pstack[i].pg);
  }

  return error_trace (e);
}

#ifdef TESTING
TEST (ns_insert)
{
  struct pgr_fixture f;
  pgr_fixture_create (&f);

  TEST_CASE ("Smoke Test")
  {
    u32                    buffer[2048];
    struct stream          input;
    struct stream_ibuf_ctx ctx;
    stream_ibuf_init (&input, &ctx, buffer, sizeof (buffer));

    struct ns_insert_params params = {
        .p     = f.p,
        .src   = &input,
        .tx    = &f.tx,
        .root  = PGNO_NULL,
        .bofst = 0,
        .bytes = 40,
    };

    sb_size nelems = ns_insert (&params, &f.e);

    test_assert_int_equal (nelems, 40);
    test_assert (params.root != PGNO_NULL);
  }

  TEST_CASE ("Insert twice")
  {
    u32                    buffer[2048];
    struct stream          input;
    struct stream_ibuf_ctx ctx;
    stream_ibuf_init (&input, &ctx, buffer, sizeof (buffer));

    struct ns_insert_params params = {
        .p     = f.p,
        .src   = &input,
        .tx    = &f.tx,
        .root  = PGNO_NULL,
        .bofst = 0,
        .bytes = 40,
    };

    sb_size nelems = ns_insert (&params, &f.e);

    test_assert_int_equal (nelems, 40);
    test_assert (params.root != PGNO_NULL);

    nelems = ns_insert (&params, &f.e);

    test_assert_int_equal (nelems, 40);
    test_assert (params.root != PGNO_NULL);
  }

  /*-----------------------------------------------------------------------------
   * SUBSECTION: Testing that sizeof(stream) and provided bytes behaves
   *----------------------------------------------------------------------------*/

#  define BYTE_STREAM_SIZE_TEST(byte_size, stream_size, expected)                \
    do {                                                                         \
      TEST_CASE ("Bytes: %d Stream: %d => %d", byte_size, stream_size, expected) \
      {                                                                          \
        u8                     buffer[4096];                                     \
        struct stream          input;                                            \
        struct stream_ibuf_ctx ctx;                                              \
        stream_ibuf_init (&input, &ctx, buffer, stream_size);                    \
                                                                                 \
        struct ns_insert_params params = {                                       \
            .p     = f.p,                                                        \
            .src   = &input,                                                     \
            .tx    = &f.tx,                                                      \
            .root  = PGNO_NULL,                                                  \
            .bofst = 0,                                                          \
            .bytes = byte_size,                                                  \
        };                                                                       \
                                                                                 \
        sb_size nelems = ns_insert (&params, &f.e);                              \
                                                                                 \
        test_assert_int_equal (nelems, expected);                                \
        if (expected > 0) {                                                      \
          test_assert (params.root != PGNO_NULL);                                \
        } else {                                                                 \
          test_assert (params.root == PGNO_NULL);                                \
        }                                                                        \
      }                                                                          \
    }                                                                            \
    while (0)

  BYTE_STREAM_SIZE_TEST (0, 2048, 2048);
  BYTE_STREAM_SIZE_TEST (2048, 0, 2048);
  BYTE_STREAM_SIZE_TEST (2048, 2048, 2048);
  BYTE_STREAM_SIZE_TEST (2048, 4096, 2048);
  BYTE_STREAM_SIZE_TEST (4096, 2048, 2048);

  pgr_fixture_teardown (&f);
}

#  define DO_INSERT(f, _root, _bofst, data, params)       \
    do {                                                  \
      u32_arr_rand (data);                                \
      struct stream          src;                         \
      struct stream_ibuf_ctx ctx;                         \
      stream_ibuf_init (&src, &ctx, data, sizeof (data)); \
                                                          \
      pgr_begin_txn (&f.tx, f.p, &f.e);                   \
                                                          \
      params = (struct ns_insert_params){                 \
          .p     = f.p,                                   \
          .src   = &src,                                  \
          .tx    = &f.tx,                                 \
          .root  = _root,                                 \
          .bofst = _bofst,                                \
          .bytes = sizeof (data),                         \
      };                                                  \
                                                          \
      ns_insert (&params, &f.e);                          \
                                                          \
      pgr_commit (f.p, &f.tx, &f.e);                      \
                                                          \
      test_assert (params.root != PGNO_NULL);             \
    }                                                     \
    while (0)

#  define DO_REMOVE(f, _root, _bofst, _size, params) \
    do {                                             \
      pgr_begin_txn (&f.tx, f.p, &f.e);              \
                                                     \
      params = (struct ns_remove_params){            \
          .p      = f.p,                             \
          .dest   = NULL,                            \
          .tx     = &f.tx,                           \
          .root   = _root,                           \
          .bofst  = _bofst,                          \
          .stride = 1,                               \
          .nelem  = _size,                           \
      };                                             \
                                                     \
      ns_remove (&params, &f.e);                     \
                                                     \
      pgr_commit (f.p, &f.tx, &f.e);                 \
    }                                                \
    while (0)

#  define TEST_DATA_LIST(pgno, expected_data)                                                      \
    do {                                                                                           \
      page_h root = page_h_create ();                                                              \
      pgr_get (&root, PG_DATA_LIST, pgno, f.p, &f.e);                                              \
      test_assert_int_equal (dl_used (page_h_ro (&root)), sizeof (expected_data));                 \
      int equal = memcmp (dl_get_data (page_h_ro (&root)), expected_data, sizeof (expected_data)); \
      test_assert_int_equal (equal, 0);                                                            \
      pgr_release (f.p, &root, PG_DATA_LIST, &f.e);                                                \
    }                                                                                              \
    while (0)

TEST (ns_insert_from_empty)
{
  struct pgr_fixture f;
  pgr_fixture_create (&f);

  TEST_CASE ("Create a data list page with one element")
  {
    struct ns_insert_params params;
    u8                      data[1];
    DO_INSERT (f, PGNO_NULL, 0, data, params);
    TEST_DATA_LIST (params.root, data);
  }

  TEST_CASE ("When ofst > size, treats as ofst = size")
  {
    struct ns_insert_params params;
    u8                      data[1];
    DO_INSERT (f, PGNO_NULL, 10, data, params);
    TEST_DATA_LIST (params.root, data);
  }

  TEST_CASE ("Insert 1 element twice at offset 0")
  {
    struct ns_insert_params params;
    u8                      data[1];
    u8                      expected[2];

    DO_INSERT (f, PGNO_NULL, 0, data, params);
    expected[1] = data[0];

    DO_INSERT (f, params.root, 0, data, params);
    expected[0] = data[0];

    TEST_DATA_LIST (params.root, expected);
  }

  TEST_CASE ("Insert 1 element twice at offset -1")
  {
    struct ns_insert_params params;
    u8                      data[1];
    u8                      expected[2];

    DO_INSERT (f, PGNO_NULL, 0, data, params);
    expected[0] = data[0];

    DO_INSERT (f, params.root, 1, data, params);
    expected[1] = data[0];

    TEST_DATA_LIST (params.root, expected);
  }

  TEST_CASE ("Insert DL_DATA_SIZE elements - doesn't break into nodes")
  {
    struct ns_insert_params params;
    u8                      data[DL_DATA_SIZE];
    DO_INSERT (f, PGNO_NULL, 0, data, params);
    TEST_DATA_LIST (params.root, data);
  }

  TEST_CASE ("Insert combined DL_DATA_SIZE elements - doesn't break into nodes")
  {
    struct ns_insert_params params;
    u8                      data1[DL_DATA_SIZE - 1];
    u8                      data2[1];
    u8                      expected[DL_DATA_SIZE];

    DO_INSERT (f, PGNO_NULL, 0, data1, params);
    DO_INSERT (f, params.root, 0, data2, params);

    expected[0] = data2[0];
    memcpy (&expected[1], data1, DL_DATA_SIZE - 1);

    TEST_DATA_LIST (params.root, expected);
  }

  TEST_CASE ("Insert combined DL_DATA_SIZE elements - doesn't break into nodes")
  {
    struct ns_insert_params params;
    u8                      data1[DL_DATA_SIZE - 3];
    u8                      data2[1];
    u8                      data3[1];
    u8                      data4[1];

    u8                      expected[DL_DATA_SIZE];

    DO_INSERT (f, PGNO_NULL, 0, data1, params);
    DO_INSERT (f, params.root, 1, data2, params);
    DO_INSERT (f, params.root, 1, data3, params);
    DO_INSERT (f, params.root, 1, data4, params);

    expected[0] = data1[0];
    expected[1] = data4[0];
    expected[2] = data3[0];
    expected[3] = data2[0];
    memcpy (&expected[4], &data1[1], DL_DATA_SIZE - 4);

    TEST_DATA_LIST (params.root, expected);
  }

  pgr_fixture_teardown (&f);
}
#endif
