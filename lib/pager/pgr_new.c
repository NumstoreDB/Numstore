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
#include "pager.h"
#include "pager/page_fixture.h"
#include "pager/page_h.h"
#include "pages/data_list.h"
#include "pages/fsm_page.h"
#include "pages/page.h"
#include "wal/wal_rec_hdr.h"

static err_t
pgr_new_from_uninitialized_unsafe (
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
  latch_lock (&p->l);
  ht_insert_expect_idx (&p->pgno_to_value, hd);
  latch_unlock (&p->l);

  // Initialize page_h
  dest->pgr = pgr;
  dest->pgw = pgw;
  dest->mode = PHM_X;
  dest->tx = tx;

theend:
  return error_trace (e);
}

err_t
pgr_new (
    page_h *dest,
    struct pager *p,
    struct txn *tx,
    const enum page_type type,
    error *e)
{
  page_h fsm = page_h_create (); // The currently used free space map
  pgno ret = 0;                  // The return page
  pgno fsmpg = 0;                // The free space map page

  // Iterate through existing FSM's to see if there's a free lockable page
  for (; fsmpg < pgr_get_npages (p); fsmpg += FS_BTMP_NPGS)
    {
      // Fetch this free space map
      if (pgr_get (&fsm, PG_FREE_SPACE_MAP, fsmpg, p, e))
        {
          goto failed;
        }

      // Find the next free slot
      sp_size next = fsm_next_freebit (page_h_ro (&fsm), 0);

      // No free pages available
      if (next == -1)
        {
          ASSERT (fsm.mode == PHM_S);
          if (pgr_release (p, &fsm, PG_FREE_SPACE_MAP, e))
            {
              goto failed;
            }
          continue;
        }

      ret = next + fsmpg; // The actual page number

      // Update fsm bit
      {
        if (pgr_make_writable (p, tx, &fsm, e))
          {
            goto failed;
          }
        fsm_set_bit (page_h_w (&fsm), next);
        if (pgr_release_with_log (
                p,
                &fsm,
                PG_FREE_SPACE_MAP,
                &(struct wal_update_write){
                    .type = WUP_FSM,
                    .tid = tx->tid,
                    .prev = tx->data.last_lsn,
                    .fsm = {
                        .pg = page_h_pgno (&fsm),
                        .bit = pgtoidx (ret),
                        .undo = 0,
                        .redo = 1,
                    },
                },
                e))
          {
            goto failed;
          }
      }

      if (pgr_get_writable (dest, tx, PG_PERMISSIVE, ret, p, e))
        {
          goto failed;
        }
      page_init_empty (page_h_w (dest), type);

      goto theend;
    }

  // Exceeded the number of free space maps we have - Need to create a new free space map
  if (pgr_new_from_uninitialized_unsafe (&fsm, p, tx, PG_FREE_SPACE_MAP, fsmpg, e))
    {
      goto failed;
    }

  // Creating a new FSM means we are tracking this many pages
  if (pgr_extend_file (p, fsmpg + FS_BTMP_NPGS, tx, e))
    {
      goto failed;
    }

  // the next free bit must be 1 because this is a new fsm
  ASSERT (fsm_next_freebit (page_h_ro (&fsm), 0) == 1);
  ret = fsmpg + 1;

  fsm_set_bit (page_h_w (&fsm), pgtoidx (ret));

  if (pgr_release_with_log (
          p,
          &fsm,
          PG_FREE_SPACE_MAP,
          &(struct wal_update_write){
              .type = WUP_FSM,
              .tid = tx->tid,
              .prev = tx->data.last_lsn,
              .fsm = {
                  .pg = page_h_pgno (&fsm),
                  .bit = pgtoidx (ret),
                  .undo = 0,
                  .redo = 1,
              },
          },
          e))
    {
      goto failed;
    }

  if (pgr_new_from_uninitialized_unsafe (dest, p, tx, type, ret, e))
    {
      goto failed;
    }

theend:
  return SUCCESS;

failed:
  pgr_cancel_if_exists (p, dest);
  pgr_cancel_if_exists (p, &fsm);
  return error_trace (e);
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
