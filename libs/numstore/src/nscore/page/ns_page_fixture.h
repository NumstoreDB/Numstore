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

#include "core/ns_arena_alloc.h" // allocator
#include "core/ns_error.h"       // error
#include "core/ns_stdtypes.h"    // u32 ...etc
#include "core/os/ns_malloc.h"
#include "core/os/ns_os.h"
#include "nscore/page/ns_page.h" // dl_data
#include "nscore/page/ns_page_data_list.h"
#include "nscore/page/ns_page_h.h"
#include "nscore/page/ns_page_inner_node.h"
#include "nscore/pager/ns_pager.h" // page_h
#include "nscore/txn_table/ns_txn_table.h"

#include <stddef.h>

struct pager;

/******************************************************************************
 * SECTION: Page Fixture
 * ----------------------------------------------------------------------------
 * @brief A fixture that saves keystrokes on common pager initialization in
 * tests
 ******************************************************************************/

struct pgr_fixture
{
  struct pager      *p;
  struct arena_alloc alloc;
  struct txn         tx;
  struct i_mem       mem;
  struct i_os        os;
};

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

/*-----------------------------------------------------------------------------
 * SUBSECTION: Declarative api for page building
 *----------------------------------------------------------------------------*/

struct page_desc
{
  enum page_type type;
  page_h         out;
  b_size         size;

  union {
    struct
    {
      struct page_desc *children;
      u32               clen;
      u32               dclen;
    } inner;

    struct dl_data data_list;
  };
};

struct page_tree_builder
{
  struct pager    *pager;
  struct page_desc root;
  struct txn      *txn;
};

/*-----------------------------------------------------------------------------
 * SUBSECTION: Common Builders
 *----------------------------------------------------------------------------*/

err_t build_page_tree (struct page_tree_builder *builder, error *e);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Common Trees
 *----------------------------------------------------------------------------*/

// IN -> DL, DL, ...
// TODO: clen is 2 but only one child is listed, so build_page_tree reads
// past the children array - set clen to 1 before using this macro
#define in1dl(_pager, txid, inl, dl0)                                                     \
  {                                                                                       \
      .pager = (_pager),                                                                  \
      .root =                                                                             \
          {                                                                               \
              .type = PG_INNER_NODE,                                                      \
              .out  = page_h_create (),                                                   \
              .inner =                                                                    \
                  {                                                                       \
                      .dclen = (inl),                                                     \
                      .clen  = 2,                                                         \
                      .children =                                                         \
                          (struct page_desc[]){                                           \
                              {                                                           \
                                  .type      = PG_DATA_LIST,                              \
                                  .out       = page_h_create (),                          \
                                  .size      = (dl0),                                     \
                                  .data_list = (struct dl_data){.data = NULL, .blen = 0}, \
                              },                                                          \
                                                                                          \
                          },                                                              \
                  },                                                                      \
          },                                                                              \
      .txn = (txid),                                                                      \
  }

#define in2dl(_pager, txid, inl, dl0, dl1)                                                \
  {                                                                                       \
      .pager = (_pager),                                                                  \
      .root =                                                                             \
          {                                                                               \
              .type = PG_INNER_NODE,                                                      \
              .out  = page_h_create (),                                                   \
              .inner =                                                                    \
                  {                                                                       \
                      .dclen = (inl),                                                     \
                      .clen  = 2,                                                         \
                      .children =                                                         \
                          (struct page_desc[]){                                           \
                              {                                                           \
                                  .type      = PG_DATA_LIST,                              \
                                  .out       = page_h_create (),                          \
                                  .size      = (dl0),                                     \
                                  .data_list = (struct dl_data){.data = NULL, .blen = 0}, \
                              },                                                          \
                                                                                          \
                              {                                                           \
                                  .type      = PG_DATA_LIST,                              \
                                  .out       = page_h_create (),                          \
                                  .size      = (dl1),                                     \
                                  .data_list = (struct dl_data){.data = NULL, .blen = 0}, \
                              },                                                          \
                                                                                          \
                          },                                                              \
                  },                                                                      \
          },                                                                              \
      .txn = (txid),                                                                      \
  }

#define in3dl(_pager, txid, inl, dl0, dl1, dl2)                                           \
  {                                                                                       \
      .pager = (_pager),                                                                  \
      .root =                                                                             \
          {                                                                               \
              .type = PG_INNER_NODE,                                                      \
              .out  = page_h_create (),                                                   \
              .inner =                                                                    \
                  {                                                                       \
                      .dclen = (inl),                                                     \
                      .clen  = 3,                                                         \
                      .children =                                                         \
                          (struct page_desc[]){                                           \
                              {                                                           \
                                  .type      = PG_DATA_LIST,                              \
                                  .out       = page_h_create (),                          \
                                  .size      = (dl0),                                     \
                                  .data_list = (struct dl_data){.data = NULL, .blen = 0}, \
                              },                                                          \
                                                                                          \
                              {                                                           \
                                  .type      = PG_DATA_LIST,                              \
                                  .out       = page_h_create (),                          \
                                  .size      = (dl1),                                     \
                                  .data_list = (struct dl_data){.data = NULL, .blen = 0}, \
                              },                                                          \
                                                                                          \
                              {                                                           \
                                  .type      = PG_DATA_LIST,                              \
                                  .out       = page_h_create (),                          \
                                  .size      = (dl2),                                     \
                                  .data_list = (struct dl_data){.data = NULL, .blen = 0}, \
                              },                                                          \
                                                                                          \
                          },                                                              \
                  },                                                                      \
          },                                                                              \
      .txn = (txid),                                                                      \
  }

#define in4dl(_pager, txid, inl, dl0, dl1, dl2, dl3)                                      \
  {                                                                                       \
      .pager = (_pager),                                                                  \
      .root =                                                                             \
          {                                                                               \
              .type = PG_INNER_NODE,                                                      \
              .out  = page_h_create (),                                                   \
              .inner =                                                                    \
                  {                                                                       \
                      .dclen = (inl),                                                     \
                      .clen  = 4,                                                         \
                      .children =                                                         \
                          (struct page_desc[]){                                           \
                              {                                                           \
                                  .type      = PG_DATA_LIST,                              \
                                  .out       = page_h_create (),                          \
                                  .size      = (dl0),                                     \
                                  .data_list = (struct dl_data){.data = NULL, .blen = 0}, \
                              },                                                          \
                                                                                          \
                              {                                                           \
                                  .type      = PG_DATA_LIST,                              \
                                  .out       = page_h_create (),                          \
                                  .size      = (dl1),                                     \
                                  .data_list = (struct dl_data){.data = NULL, .blen = 0}, \
                              },                                                          \
                                                                                          \
                              {                                                           \
                                  .type      = PG_DATA_LIST,                              \
                                  .out       = page_h_create (),                          \
                                  .size      = (dl2),                                     \
                                  .data_list = (struct dl_data){.data = NULL, .blen = 0}, \
                              },                                                          \
                                                                                          \
                              {                                                           \
                                  .type      = PG_DATA_LIST,                              \
                                  .out       = page_h_create (),                          \
                                  .size      = (dl3),                                     \
                                  .data_list = (struct dl_data){.data = NULL, .blen = 0}, \
                              },                                                          \
                                                                                          \
                          },                                                              \
                  },                                                                      \
          },                                                                              \
      .txn = (txid),                                                                      \
  }

#define in5dl(_pager, txid, inl, dl0, dl1, dl2, dl3, dl4)                                 \
  {                                                                                       \
      .pager = (_pager),                                                                  \
      .root =                                                                             \
          {                                                                               \
              .type = PG_INNER_NODE,                                                      \
              .out  = page_h_create (),                                                   \
              .inner =                                                                    \
                  {                                                                       \
                      .dclen = (inl),                                                     \
                      .clen  = 5,                                                         \
                      .children =                                                         \
                          (struct page_desc[]){                                           \
                              {                                                           \
                                  .type      = PG_DATA_LIST,                              \
                                  .out       = page_h_create (),                          \
                                  .size      = (dl0),                                     \
                                  .data_list = (struct dl_data){.data = NULL, .blen = 0}, \
                              },                                                          \
                                                                                          \
                              {                                                           \
                                  .type      = PG_DATA_LIST,                              \
                                  .out       = page_h_create (),                          \
                                  .size      = (dl1),                                     \
                                  .data_list = (struct dl_data){.data = NULL, .blen = 0}, \
                              },                                                          \
                                                                                          \
                              {                                                           \
                                  .type      = PG_DATA_LIST,                              \
                                  .out       = page_h_create (),                          \
                                  .size      = (dl2),                                     \
                                  .data_list = (struct dl_data){.data = NULL, .blen = 0}, \
                              },                                                          \
                                                                                          \
                              {                                                           \
                                  .type      = PG_DATA_LIST,                              \
                                  .out       = page_h_create (),                          \
                                  .size      = (dl3),                                     \
                                  .data_list = (struct dl_data){.data = NULL, .blen = 0}, \
                              },                                                          \
                                                                                          \
                              {                                                           \
                                  .type      = PG_DATA_LIST,                              \
                                  .out       = page_h_create (),                          \
                                  .size      = (dl4),                                     \
                                  .data_list = (struct dl_data){.data = NULL, .blen = 0}, \
                              },                                                          \
                          },                                                              \
                  },                                                                      \
          },                                                                              \
      .txn = (txid),                                                                      \
  }

// IN -> IN, IN,
// TODO: clen is 2 but only one child is listed, so build_page_tree reads
// past the children array - set clen to 1 before using this macro
#define in1in(_pager, txid, inl, in0)                                   \
  {                                                                     \
      .pager = (_pager),                                                \
      .root =                                                           \
          {                                                             \
              .type = PG_INNER_NODE,                                    \
              .out  = page_h_create (),                                 \
              .inner =                                                  \
                  {                                                     \
                      .dclen = (inl),                                   \
                      .clen  = 2,                                       \
                      .children =                                       \
                          (struct page_desc[]){                         \
                              {                                         \
                                  .type  = PG_INNER_NODE,               \
                                  .out   = page_h_create (),            \
                                  .inner = {.dclen = (in0), .clen = 0}, \
                              },                                        \
                                                                        \
                          },                                            \
                  },                                                    \
          },                                                            \
      .txn = (txid),                                                    \
  }

#define in2in(_pager, txid, inl, in0, in1)                              \
  {                                                                     \
      .pager = (_pager),                                                \
      .root =                                                           \
          {                                                             \
              .type = PG_INNER_NODE,                                    \
              .out  = page_h_create (),                                 \
              .inner =                                                  \
                  {                                                     \
                      .dclen = (inl),                                   \
                      .clen  = 2,                                       \
                      .children =                                       \
                          (struct page_desc[]){                         \
                              {                                         \
                                  .type  = PG_INNER_NODE,               \
                                  .out   = page_h_create (),            \
                                  .inner = {.dclen = (in0), .clen = 0}, \
                              },                                        \
                                                                        \
                              {                                         \
                                  .type  = PG_INNER_NODE,               \
                                  .out   = page_h_create (),            \
                                  .inner = {.dclen = (in1), .clen = 0}, \
                              },                                        \
                                                                        \
                          },                                            \
                  },                                                    \
          },                                                            \
      .txn = (txid),                                                    \
  }

#define in3in(_pager, txid, inl, in0, in1, in2)                         \
  {                                                                     \
      .pager = (_pager),                                                \
      .root =                                                           \
          {                                                             \
              .type = PG_INNER_NODE,                                    \
              .out  = page_h_create (),                                 \
              .inner =                                                  \
                  {                                                     \
                      .dclen = (inl),                                   \
                      .clen  = 3,                                       \
                      .children =                                       \
                          (struct page_desc[]){                         \
                              {                                         \
                                  .type  = PG_INNER_NODE,               \
                                  .out   = page_h_create (),            \
                                  .inner = {.dclen = (in0), .clen = 0}, \
                              },                                        \
                                                                        \
                              {                                         \
                                  .type  = PG_INNER_NODE,               \
                                  .out   = page_h_create (),            \
                                  .inner = {.dclen = (in1), .clen = 0}, \
                              },                                        \
                                                                        \
                              {                                         \
                                  .type  = PG_INNER_NODE,               \
                                  .out   = page_h_create (),            \
                                  .inner = {.dclen = (in2), .clen = 0}, \
                              },                                        \
                                                                        \
                          },                                            \
                  },                                                    \
          },                                                            \
      .txn = (txid),                                                    \
  }

#define in4in(_pager, txid, inl, in0, in1, in2, in3)                    \
  {                                                                     \
      .pager = (_pager),                                                \
      .root =                                                           \
          {                                                             \
              .type = PG_INNER_NODE,                                    \
              .out  = page_h_create (),                                 \
              .inner =                                                  \
                  {                                                     \
                      .dclen = (inl),                                   \
                      .clen  = 4,                                       \
                      .children =                                       \
                          (struct page_desc[]){                         \
                              {                                         \
                                  .type  = PG_INNER_NODE,               \
                                  .out   = page_h_create (),            \
                                  .inner = {.dclen = (in0), .clen = 0}, \
                              },                                        \
                                                                        \
                              {                                         \
                                  .type  = PG_INNER_NODE,               \
                                  .out   = page_h_create (),            \
                                  .inner = {.dclen = (in1), .clen = 0}, \
                              },                                        \
                                                                        \
                              {                                         \
                                  .type  = PG_INNER_NODE,               \
                                  .out   = page_h_create (),            \
                                  .inner = {.dclen = (in2), .clen = 0}, \
                              },                                        \
                                                                        \
                              {                                         \
                                  .type  = PG_INNER_NODE,               \
                                  .out   = page_h_create (),            \
                                  .inner = {.dclen = (in3), .clen = 0}, \
                              },                                        \
                                                                        \
                          },                                            \
                  },                                                    \
          },                                                            \
      .txn = (txid),                                                    \
  }

#define in5in(_pager, txid, inl, in0, in1, in2, in3, in4)               \
  {                                                                     \
      .pager = (_pager),                                                \
      .root =                                                           \
          {                                                             \
              .type = PG_INNER_NODE,                                    \
              .out  = page_h_create (),                                 \
              .inner =                                                  \
                  {                                                     \
                      .dclen = (inl),                                   \
                      .clen  = 5,                                       \
                      .children =                                       \
                          (struct page_desc[]){                         \
                              {                                         \
                                  .type  = PG_INNER_NODE,               \
                                  .out   = page_h_create (),            \
                                  .inner = {.dclen = (in0), .clen = 0}, \
                              },                                        \
                                                                        \
                              {                                         \
                                  .type  = PG_INNER_NODE,               \
                                  .out   = page_h_create (),            \
                                  .inner = {.dclen = (in1), .clen = 0}, \
                              },                                        \
                                                                        \
                              {                                         \
                                  .type  = PG_INNER_NODE,               \
                                  .out   = page_h_create (),            \
                                  .inner = {.dclen = (in2), .clen = 0}, \
                              },                                        \
                                                                        \
                              {                                         \
                                  .type  = PG_INNER_NODE,               \
                                  .out   = page_h_create (),            \
                                  .inner = {.dclen = (in3), .clen = 0}, \
                              },                                        \
                                                                        \
                              {                                         \
                                  .type  = PG_INNER_NODE,               \
                                  .out   = page_h_create (),            \
                                  .inner = {.dclen = (in4), .clen = 0}, \
                              },                                        \
                          },                                            \
                  },                                                    \
          },                                                            \
      .txn = (txid),                                                    \
  }
#endif
