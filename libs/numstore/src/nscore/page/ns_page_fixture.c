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

#include "nscore/page/ns_page_fixture.h"

#include <string.h>

#ifndef NDEBUG
#  include "core/ns_arena_alloc.h"
#  include "core/ns_csx_assert.h"
#  include "core/ns_error.h"
#  include "core/ns_numerics.h"
#  include "core/ns_platform.h"
#  include "core/ns_testing.h"
#  include "core/os/ns_malloc.h"
#  include "core/os/ns_os.h"
#  include "nscore/page/ns_page.h"
#  include "nscore/page/ns_page_delegate.h"
#  include "nscore/pager/ns_pager.h"

DEFINE_DBG_ASSERT (struct pgr_fixture, pgr_fixture, f, {
  ASSERT (f);
  ASSERT (f->p);
})

DEFINE_DBG_ASSERT (struct in_page_builder, in_page_builder, in, {
  ASSERT (in);
  ASSERT (in->dclen <= IN_MAX_KEYS);
  ASSERT (in->children.len <= in->dclen);
})

DEFINE_DBG_ASSERT (struct dl_page_builder, dl_page_builder, in, {
  ASSERT (in);
  ASSERT (in->dclen <= DL_DATA_SIZE);
  ASSERT (in->data.blen <= in->dclen);
  if (in->data.blen == 0) {
    ASSERT (in->data.data == NULL);
  } else {
    ASSERT (in->data.data != NULL);
  }
})

err_t
pgr_fixture_create_with (struct pgr_fixture *dest, struct i_os os, struct i_mem mem, error *e)
{
  ASSERT (dest);
  dest->mem = mem;
  dest->os  = os;

  if (unlikely (pgr_delete_single_file ("testdb", dest->os, e) < 0)) {
    return error_trace (e);
  }

  struct pager *p = pgr_open ("testdb", dest->mem, dest->os, e);
  if (p == NULL) {
    return error_trace (e);
  }

  dest->p  = p;
  dest->tx = (struct txn){0};

  DBG_ASSERT (pgr_fixture, dest);

  arena_alloc_create_default (&dest->alloc);

  return SUCCESS;
}

err_t
pgr_fixture_create (struct pgr_fixture *dest, error *e)
{
  ASSERT (dest);
  struct i_mem mem = default_mem ();
  struct i_os  os;

  if (unlikely (system_os_create (mem, &os, e) < 0)) {
    return error_trace (e);
  }

  if (pgr_fixture_create_with (dest, os, mem, e) < 0) {
    system_os_free (os);
    return error_trace (e);
  }

  return SUCCESS;
}

err_t
pgr_fixture_teardown (struct pgr_fixture *f, error *e)
{
  DBG_ASSERT (pgr_fixture, f);

  pgr_fixture_teardown_with (f, e);
  system_os_free (f->os);
  return e->cause_code;
}

err_t
pgr_fixture_teardown_with (struct pgr_fixture *f, error *e)
{
  DBG_ASSERT (pgr_fixture, f);

  if (e->cause_code == ERR_CRASHED) {
    error_reset (e);
    pgr_crash (f->p, e);
  } else {
    error_reset (e);
    pgr_close (f->p, e);
  }
  arena_alloc_free_all (&f->alloc);
  return e->cause_code;
}

struct tree_result
{
  spgno  root;
  b_size size;
};

static err_t
build_tree_from_descr_inner (
    struct tree_result *dest,
    struct pager       *p,
    struct txn         *tx,
    struct tree_descr   descr,
    error              *e
)
{
  page_h cur = page_h_create ();

  if (descr.next == NULL) {
    if (pgr_new (&cur, p, tx, PG_DATA_LIST, e) < 0) {
      goto failed;
    }

    dl_set_data (page_h_w (&cur), (struct dl_data){.data = descr.data, .blen = descr.dlen});
  } else {
    if (pgr_new (&cur, p, tx, PG_INNER_NODE, e) < 0) {
      goto failed;
    }

    for (u32 i = 0; i < descr.nlen; ++i) {
      struct tree_result result;

      if (build_tree_from_descr_inner (&result, p, tx, descr.next[i], e) < 0) {
        goto failed;
      }

      in_push_end (page_h_w (&cur), result.size, result.root);
    }
  }

  const pgno root = page_h_pgno (&cur);
  ASSERT (root <= (pgno)I64_MAX);
  dest->root = (spgno)root;
  // TODO: for an inner node this is the key count, not the subtree's byte
  // size, so a parent's key is wrong for any tree more than two levels deep
  dest->size = dlgt_get_len (page_h_ro (&cur));

  return pgr_release (p, &cur, PG_INNER_NODE | PG_DATA_LIST, e);

failed:
  pgr_cancel_if_exists (&cur);
  return error_trace (e);
}

spgno
build_tree_from_descr (struct pager *p, struct txn *tx, struct tree_descr descr, error *e)
{
  struct tree_result result;
  if (build_tree_from_descr_inner (&result, p, tx, descr, e) < 0) {
    return error_trace (e);
  }
  return result.root;
}

err_t
build_fake_inner_node (page_h *dest, const struct in_page_builder b, error *e)
{
  DBG_ASSERT (in_page_builder, &b);

  WRAP (pgr_new (dest, b.pager, b.txn, PG_INNER_NODE, e));

  if (b.prev != NULL) {
    dlgt_link (page_h_w (b.prev), page_h_w (dest));
  }

  if (b.next != NULL) {
    dlgt_link (page_h_w (dest), page_h_w (b.next));
  }

  struct in_pair data[IN_MAX_KEYS];

  if (b.children.nodes != NULL) {
    memcpy (data, b.children.nodes, b.children.len * sizeof *data);
  }

  // Filler leaves count up from 99999, assuming callers' own pages stay below
  // that - in_validate_for_db rejects a node with a duplicate leaf
  static pgno pg = 99999;

  if (b.children.len < b.dclen) {
    for (u32 i = b.children.len; i < b.dclen; ++i) {
      data[i].pg  = pg++;
      data[i].key = 1;
    }
  }

  in_set_data (page_h_w (dest), (struct in_data){.nodes = data, .len = b.dclen});

  return SUCCESS;
}

err_t
build_fake_data_list (page_h *dest, const struct dl_page_builder b, error *e)
{
  DBG_ASSERT (dl_page_builder, &b);

  WRAP (pgr_new (dest, b.pager, b.txn, PG_DATA_LIST, e));

  if (b.prev != NULL) {
    dlgt_link (page_h_w (b.prev), page_h_w (dest));
  }

  if (b.next != NULL) {
    dlgt_link (page_h_w (dest), page_h_w (b.next));
  }

  u8 data[DL_DATA_SIZE];

  if (b.data.data != NULL) {
    memcpy (data, b.data.data, b.data.blen);
  }

  // Fill the rest with random bytes
  if (b.data.blen < b.dclen) {
    rand_bytes (&data[b.data.blen], (b.dclen - b.data.blen));
  }

  dl_set_data (page_h_w (dest), (struct dl_data){.data = data, .blen = b.dclen});

  return SUCCESS;
}

////////////////////////////////////////////////////////////
/// DECLARATIVE API

static err_t build_page_desc (
    struct page_desc *desc,
    struct pager     *pager,
    struct txn       *txn,
    error            *e
);

err_t
build_page_tree (struct page_tree_builder *builder, error *e)
{
  ASSERT (builder);
  return build_page_desc (&builder->root, builder->pager, builder->txn, e);
}

static err_t
build_page_desc (struct page_desc *desc, struct pager *pager, struct txn *txn, error *e)
{
  switch (desc->type) {
    case PG_INNER_NODE: {
      // Children first, so the inner node can hold their page numbers
      const page_h  *prev = NULL;
      struct in_pair children[IN_MAX_KEYS];

      for (u32 i = 0; i < desc->inner.clen; i++) {
        page_h *cur = &desc->inner.children[i].out;
        ASSERT (cur->mode == PHM_NONE);

        WRAP (build_page_desc (&desc->inner.children[i], pager, txn, e));

        if (prev != NULL) {
          dlgt_link (page_h_w (prev), page_h_w (cur));
        }

        prev        = cur;

        children[i] = (struct in_pair){
            .pg  = page_h_pgno (&desc->inner.children[i].out),
            .key = desc->inner.children[i].size,
        };
      }

      const struct in_page_builder builder = {
          .pager = pager,
          .txn   = txn,
          .prev  = NULL,
          .next  = NULL, // The caller links this layer
          .children =
              (struct in_data){
                  .nodes = children,
                  .len   = desc->inner.clen,
              },
          .dclen = desc->inner.dclen
      };

      WRAP (build_fake_inner_node (&desc->out, builder, e));
      desc->size = in_get_size (page_h_ro (&desc->out));

      return SUCCESS;
    }

    case PG_DATA_LIST: {
      ASSERT (desc->size <= UINT32_MAX);
      const struct dl_page_builder builder = {
          .pager = pager,
          .txn   = txn,
          .prev  = NULL,
          .next  = NULL, // The caller links this layer
          .data  = desc->data_list,
          .dclen = (u32)desc->size,
      };

      return build_fake_data_list (&desc->out, builder, e);
    }

    default: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }

  return 0;
}

TEST (build_page_tree)
{
  struct pgr_fixture f;
  pgr_fixture_create (&f, &ctx->e);

  struct txn tx;
  pgr_begin_txn (&tx, f.p, &ctx->e);

  struct page_tree_builder builder = {
      .pager = f.p,
      .root =
          {
              .type = PG_INNER_NODE,
              .out  = page_h_create (),
              .inner =
                  {
                      .children =
                          (struct page_desc[]){
                              {
                                  .type = PG_INNER_NODE,
                                  .out  = page_h_create (),
                                  .inner =
                                      {
                                          .children =
                                              (struct page_desc[]){
                                                  {
                                                      .type = PG_DATA_LIST,
                                                      .out  = page_h_create (),
                                                      .size = DL_DATA_SIZE,
                                                      .data_list =
                                                          (struct dl_data){.data = NULL, .blen = 0},
                                                  },
                                              },
                                          .clen  = 1,
                                          .dclen = IN_MAX_KEYS,
                                      },
                              },
                              {
                                  .type      = PG_DATA_LIST,
                                  .out       = page_h_create (),
                                  .size      = DL_DATA_SIZE,
                                  .data_list = (struct dl_data){.data = NULL, .blen = 0},
                              },
                          },
                      .clen  = 2,
                      .dclen = 3,
                  },
          },
      .txn = &tx,
  };

  build_page_tree (&builder, &ctx->e);

  page_h *root = &builder.root.out;
  page_h *inl1 = &builder.root.inner.children[0].out;
  page_h *dll1 = &builder.root.inner.children[1].out;
  page_h *dll2 = &builder.root.inner.children[0].inner.children[0].out;

  pgr_release (f.p, root, PG_INNER_NODE, &ctx->e);
  pgr_release (f.p, inl1, PG_INNER_NODE, &ctx->e);
  pgr_release (f.p, dll1, PG_DATA_LIST, &ctx->e);
  pgr_release (f.p, dll2, PG_DATA_LIST, &ctx->e);

  pgr_commit (f.p, &tx, &ctx->e);

  pgr_fixture_teardown (&f, &ctx->e);
}

TEST (build_fake_pages_link_neighbors)
{
  // Setup
  struct pgr_fixture f;
  test_err_t_check (pgr_fixture_create (&f, &ctx->e), SUCCESS, &ctx->e);

  struct txn tx;
  test_err_t_check (pgr_begin_txn (&tx, f.p, &ctx->e), SUCCESS, &ctx->e);

  page_h                 dl_a = page_h_create ();
  page_h                 dl_b = page_h_create ();
  page_h                 in_a = page_h_create ();
  page_h                 in_b = page_h_create ();

  struct dl_page_builder dlb  = {
      .pager = f.p,
      .txn   = &tx,
      .prev  = NULL,
      .next  = NULL,
      .data  = {.data = NULL, .blen = 0},
      .dclen = DL_DATA_SIZE,
  };
  struct in_page_builder inb = {
      .pager    = f.p,
      .txn      = &tx,
      .prev     = NULL,
      .next     = NULL,
      .children = {.nodes = NULL, .len = 0},
      .dclen    = IN_MAX_KEYS,
  };

  // Run
  TEST_CASE ("A data list built with prev links after it")
  {
    test_err_t_check (build_fake_data_list (&dl_a, dlb, &ctx->e), SUCCESS, &ctx->e);
    dlb.prev = &dl_a;
    test_err_t_check (build_fake_data_list (&dl_b, dlb, &ctx->e), SUCCESS, &ctx->e);

    test_assert_type_equal (dl_get_next (page_h_ro (&dl_a)), page_h_pgno (&dl_b), pgno, PRpgno);
    test_assert_type_equal (dl_get_prev (page_h_ro (&dl_b)), page_h_pgno (&dl_a), pgno, PRpgno);
  }

  TEST_CASE ("An inner node built with next links before it")
  {
    test_err_t_check (build_fake_inner_node (&in_b, inb, &ctx->e), SUCCESS, &ctx->e);
    inb.next = &in_b;
    test_err_t_check (build_fake_inner_node (&in_a, inb, &ctx->e), SUCCESS, &ctx->e);

    test_assert_type_equal (in_get_next (page_h_ro (&in_a)), page_h_pgno (&in_b), pgno, PRpgno);
    test_assert_type_equal (in_get_prev (page_h_ro (&in_b)), page_h_pgno (&in_a), pgno, PRpgno);
  }

  TEST_CASE ("The remaining link directions")
  {
    page_h dl_c = page_h_create ();
    page_h in_c = page_h_create ();

    dlb.prev    = NULL;
    dlb.next    = &dl_a;
    test_err_t_check (build_fake_data_list (&dl_c, dlb, &ctx->e), SUCCESS, &ctx->e);
    test_assert_type_equal (dl_get_prev (page_h_ro (&dl_a)), page_h_pgno (&dl_c), pgno, PRpgno);

    inb.next = NULL;
    inb.prev = &in_b;
    test_err_t_check (build_fake_inner_node (&in_c, inb, &ctx->e), SUCCESS, &ctx->e);
    test_assert_type_equal (in_get_next (page_h_ro (&in_b)), page_h_pgno (&in_c), pgno, PRpgno);

    test_err_t_check (pgr_release (f.p, &dl_c, PG_DATA_LIST, &ctx->e), SUCCESS, &ctx->e);
    test_err_t_check (pgr_release (f.p, &in_c, PG_INNER_NODE, &ctx->e), SUCCESS, &ctx->e);
  }

  // Teardown
  test_err_t_check (pgr_release (f.p, &dl_a, PG_DATA_LIST, &ctx->e), SUCCESS, &ctx->e);
  test_err_t_check (pgr_release (f.p, &dl_b, PG_DATA_LIST, &ctx->e), SUCCESS, &ctx->e);
  test_err_t_check (pgr_release (f.p, &in_a, PG_INNER_NODE, &ctx->e), SUCCESS, &ctx->e);
  test_err_t_check (pgr_release (f.p, &in_b, PG_INNER_NODE, &ctx->e), SUCCESS, &ctx->e);
  test_err_t_check (pgr_commit (f.p, &tx, &ctx->e), SUCCESS, &ctx->e);
  test_err_t_check (pgr_fixture_teardown (&f, &ctx->e), SUCCESS, &ctx->e);
}

TEST (page_h_none_handles)
{
  page_h h = page_h_create ();

  test_assert_type_equal (page_h_pgno_or_null (&h), PGNO_NULL, pgno, PRpgno);
  test_assert_ptr_equal (page_h_ro_or_null (&h), NULL);
  test_assert_ptr_equal (page_h_w_or_null (&h), NULL);
  test_assert (in_pair_is_empty (in_pair_from_pgh (&h)));
}
#endif
