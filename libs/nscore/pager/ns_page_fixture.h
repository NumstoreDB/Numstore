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

#ifndef NS_PAGE_FIXTURE_H
#define NS_PAGE_FIXTURE_H

#include "ns_arena_alloc.h"
#include "ns_error.h"
#include "ns_page.h"
#include "ns_page_data_list.h"
#include "ns_page_h.h"
#include "ns_page_inner_node.h"
#include "ns_stdtypes.h"
#include "os/ns_malloc.h"
#include "os/ns_os.h"
#include "pager/ns_pager.h"

#include <stddef.h>

/**
 * TODO - split these into seperate modules - keep this just page fixture
 */

struct pgr_fixture
{
  struct pager      *p;
  struct arena_alloc alloc;
  struct txn         tx;
  struct i_mem       mem;
  struct i_os        os;
};

// Lifecycle
err_t pgr_fixture_create (struct pgr_fixture *dest, error *e);
err_t pgr_fixture_teardown (struct pgr_fixture *f, error *e);
err_t pgr_fixture_create_with (
    struct pgr_fixture *dest,
    struct i_os         os,
    struct i_mem        mem,
    error              *e
);
err_t pgr_fixture_teardown_with (struct pgr_fixture *f, error *e);

// Utilities
spgno pgr_fixture_new_page (struct pgr_fixture *f, error *e);
spgno pgr_fixture_new_page_txn (struct pgr_fixture *f, error *e);
pgno pgr_fixture_new_page_txn_evict (struct pgr_fixture *f, error *e);
err_t pgr_load_page (struct pgr_fixture *f, pgno pg, error *e);
err_t pgr_fixture_fill_buffer_pool (struct pgr_fixture *f, u32 npages, error *e);

/******************************************************************************
 * SECTION: Tree fixture 2
 * ----------------------------------------------------------------------------
 * @brief easier to use than page tree builder
 ******************************************************************************/

struct tree_descr
{
  u8                *data;
  u32                dlen;
  struct tree_descr *next;
  u32                nlen;
};

spgno build_tree_from_descr (struct pager *p, struct txn *tx, struct tree_descr descr, error *e);

/******************************************************************************
 * SECTION: Fake Database Building
 * ----------------------------------------------------------------------------
 * @brief Building fake page layouts to test with
 ******************************************************************************/

struct in_page_builder
{
  struct pager  *pager;
  struct txn    *txn;

  // NULL = no linking, else link to this page
  page_h        *prev;
  page_h        *next;

  struct in_data children;

  u32            dclen; // desired len of [children] (fills the rest with random)
};

struct dl_page_builder
{
  struct pager  *pager;
  struct txn    *txn;

  // NULL = no linking, else link to this page
  page_h        *prev;
  page_h        *next;

  struct dl_data data; // NULL = all random, else use these first

  u32            dclen; // desired len of [children] (fills the rest with random)
};

err_t build_fake_inner_node (page_h *dest, struct in_page_builder b, error *e);
err_t build_fake_data_list (page_h *dest, struct dl_page_builder b, error *e);

// Declarative api for building page trees

struct page_desc
{
  enum page_type type; // INNER_NODE | DATA_LIST
  page_h         out;  // output page
  b_size         size; // For DATA_LIST, the size

  union {
    // Children inner nodes
    struct
    {
      struct page_desc *children; // List of child pages
      u32               clen;     // length of [children]
      u32               dclen;    //
    } inner;

    // Children data list
    struct dl_data data_list;
  };
};

struct page_tree_builder
{
  struct pager    *pager;
  struct page_desc root;
  struct txn      *txn;
};

err_t build_page_tree (struct page_tree_builder *builder, error *e);

#endif
