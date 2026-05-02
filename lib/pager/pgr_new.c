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
#include "c_specx/concurrency/spx_latch.h"
#include "c_specx/dev/error.h"
#include "pager.h"
#include "pager/page_fixture.h"
#include "pager/page_h.h"
#include "pages/data_list.h"
#include "pages/fsm_page.h"
#include "pages/page.h"
#include "wal/wal_rec_hdr.h"

#include <stdlib.h>

static err_t
pgr_new_impl (
    page_h *dest,
    struct pager *p,
    struct txn *tx,
    const enum page_type type,
    const pgno pg,
    error *e)
{
  DBG_ASSERT (pager, p);
  DBG_ASSERT (page_h, dest);
  ASSERT (dest->mode == PHM_NONE);

  struct page_frame *pgr = NULL;
  struct page_frame *pgw = NULL;

  // Reserve two slots (for read and write pages)
  i32 rclock = pgr_reserve_and_ctrl_lock (p, e);
  if (rclock < 0)
    {
      goto theend;
    }

  i32 wclock = pgr_reserve_and_ctrl_lock (p, e);
  if (wclock < 0)
    {
      latch_unlock (&p->pages[rclock].ctrl);
      goto theend;
    }

  pgr = &p->pages[rclock];
  pgw = &p->pages[wclock];

  // Initialize pgr
  pgr->pin = 1;
  pgr->flags = PW_DIRTY | PW_ACCESS | PW_PRESENT;
  pgr->wsibling = wclock;
  page_set_type (&pgr->page, PG_TRASH);
  pgr->page.pg = pg;

  // Initialize pgw
  pgw->pin = 1;
  pgw->flags = PW_PRESENT | PW_X;
  pgw->wsibling = -1;
  page_init_empty (&pgw->page, type);
  pgw->page.pg = pg;

  spx_lock_s (&pgr->data);
  spx_upgrade_s_x (&pgr->data);

  latch_unlock (&pgr->ctrl);
  latch_unlock (&pgw->ctrl);

  // Insert page into the hash table
  const hdata_idx hd = (hdata_idx){
    .key = pg,
    .value = rclock,
  };

  ht_insert_expect_idx (&p->pgno_to_value, hd);

  // Initialize page_h
  dest->pgr = pgr;
  dest->pgw = pgw;
  dest->mode = PHM_X;
  dest->tx = tx;

theend:
  return error_trace (e);
}

static inline err_t
pgr_new_fsmpg (page_h *fsm, struct pager *p, struct txn *tx, error *e)
{
  pgno fsmpg = pgr_get_npages (p);

  // Create a new free space map page
  if (pgr_new_impl (fsm, p, tx, PG_FREE_SPACE_MAP, fsmpg, e))
    {
      return error_trace (e);
    }

  // Creating a new FSM means we are tracking this many pages
  if (pgr_extend_file (p, fsmpg + FS_BTMP_NPGS, tx, e))
    {
      pgr_cancel (p, fsm);
      return error_trace (e);
    }

  return SUCCESS;
}

err_t
pgr_new (
    page_h *dest,
    struct pager *p,
    struct txn *tx,
    const enum page_type type,
    error *e)
{
  int r = rand ();
  page_h fsm = page_h_create ();
  pgno fsmpg = 0;

retry:

  // Iterate through existing FSM's to see if there's a free lockable page
  for (; fsmpg < pgr_get_npages (p); fsmpg += FS_BTMP_NPGS)
    {
      // X(fsm)
      if (pgr_get_writable (&fsm, tx, PG_FREE_SPACE_MAP, fsmpg, p, e))
        {
          return error_trace (e);
        }

      // Find the next free slot
      sp_size next = fsm_next_freebit (page_h_ro (&fsm), 0);

      // No free pages available - move on
      if (next == -1)
        {
          pgr_cancel (p, &fsm);
          continue;
        }

      // Update fsm bit
      fsm_set_bit (page_h_w (&fsm), next);

      // Get the requested page
      if (pgr_get_writable (dest, tx, PG_PERMISSIVE, fsmpg + next, p, e))
        {
          return error_trace (e);
        }
      page_init_empty (page_h_w (dest), type);

      // Save with (special) log
      struct wal_update_write log = wup_fsm (page_h_pgno (&fsm), tx, next, 0, 1);
      if (pgr_release_with_log (p, &fsm, PG_FREE_SPACE_MAP, &log, e))
        {
          pgr_cancel (p, &fsm);
          pgr_cancel (p, dest);
          return error_trace (e);
        }

      return SUCCESS;
    }

  latch_lock (&p->l);
  {
    if (fsmpg < pgr_get_npages (p))
      {
        latch_unlock (&p->l);
        goto retry;
      }

    if (pgr_new_fsmpg (&fsm, p, tx, e))
      {
        return error_trace (e);
      }
  }
  latch_unlock (&p->l);

  fsm_set_bit (page_h_w (&fsm), 1);

  if (pgr_new_impl (dest, p, tx, type, fsmpg + 1, e))
    {
      return error_trace (e);
    }

  struct wal_update_write log = wup_fsm (fsmpg, tx, 1, 0, 1);
  if (pgr_release_with_log (p, &fsm, PG_FREE_SPACE_MAP, &log, e))
    {
      pgr_cancel (p, &fsm);
      pgr_cancel (p, dest);
      return error_trace (e);
    }

  return SUCCESS;
}

#ifndef NTEST
TEST (pgr_new_get_save)
{
  struct pgr_fixture f;
  page_h h = page_h_create ();
  pgr_fixture_create (&f);

  struct txn tx;
  pgr_begin_txn (&tx, f.p, &f.e);

  pgr_new (&h, f.p, &tx, PG_DATA_LIST, &f.e);
  test_assert_int_equal ((int)pgr_get_npages (f.p), FS_BTMP_NPGS);

  // Make it valid
  dl_set_used (page_h_w (&h), DL_DATA_SIZE);

  pgr_release (f.p, &h, PG_DATA_LIST, &f.e);

  pgr_commit (f.p, &tx, &f.e);

  pgr_fixture_teardown (&f);
}
#endif
