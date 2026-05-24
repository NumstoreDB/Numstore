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

#include <c_specx.h>
#include "nscore/file_pager.h"
#include "nscore/page_fixture.h"
#include "nscore/page_h.h"
#include "nscore/pager.h"
#include "nscore/pages/data_list.h"
#include "nscore/pages/page.h"

err_t
pgr_get_writable (
    page_h       *dest,
    struct txn   *tx,
    const int     flags,
    const pgno    pg,
    struct pager *p,
    error        *e
)
{
  struct page_frame *pgr = NULL; // Read frame
  struct page_frame *pgw = NULL; // Write frame
  hdata_idx          data;       // The data to retrieve
  i32                rclock;     // Location of the new page
  i32                wclock;     // Write page location

  latch_lock (&p->htable_lock);
  hta_res res = ht_get_idx (&p->pgno_to_value, &data, pg);

  switch (res)
  {
    // This page exists in memory
    case HTAR_SUCCESS:
    {
      pgr = &p->pages[data.value];

      latch_lock (&pgr->ctrl);
      latch_unlock (&p->htable_lock);

      pgr->pin++;

      latch_unlock (&pgr->ctrl);

      dest->pgr  = pgr;
      dest->pgw  = NULL;
      dest->mode = PHM_S;

      spx_lock_x (&pgr->data);

      // Make a copy in a new slot

      wclock = pgr_reserve_and_ctrl_lock (p, e);
      if (wclock < 0) { return error_trace (e); }

      pgw = &p->pages[wclock];

      pgw->pin      = 1;
      pgw->wsibling = -1;
      pgw->flags    = PW_PRESENT | PW_X;
      memcpy (pgw->page.raw, dest->pgr->page.raw, PAGE_SIZE);
      pgw->page.pg = dest->pgr->page.pg;

      latch_unlock (&pgw->ctrl);

      // Set this page as the sibling of the read page
      latch_lock (&dest->pgr->ctrl);
      dest->pgr->wsibling = wclock;
      latch_unlock (&dest->pgr->ctrl);

      // Set h
      dest->pgw  = pgw;
      dest->mode = PHM_X;
      dest->tx   = tx;

      return SUCCESS;
    }

    case HTAR_DOESNT_EXIST:
    {
      latch_unlock (&p->htable_lock);

      // Grab both r and w slots in once
      rclock = pgr_reserve_and_ctrl_lock (p, e);
      if (rclock < 0) { return error_trace (e); }

      wclock = pgr_reserve_and_ctrl_lock (p, e);
      if (wclock < 0)
      {
        latch_unlock (&p->pages[rclock].ctrl);
        return error_trace (e);
      }

      data = (hdata_idx){.key = pg, .value = rclock};
      ht_insert_expect_idx (&p->pgno_to_value, data);

      pgr = &p->pages[data.value];
      pgw = &p->pages[wclock];

      pgr->pin      = 1;
      pgr->flags    = PW_ACCESS | PW_PRESENT;
      pgr->wsibling = wclock;
      pgr->page.pg  = pg;

      if (fpgr_read (p->fp, pgr->page.raw, pg, e))
      {
        latch_unlock (&pgr->ctrl);
        latch_unlock (&pgw->ctrl);
        return error_trace (e);
      }

      if (page_validate_for_db (&pgr->page, flags, e))
      {
        latch_unlock (&pgr->ctrl);
        latch_unlock (&pgw->ctrl);
        return error_trace (e);
      }

      // Write frame: copy of the read frame, owned by this txn.
      pgw->pin      = 1;
      pgw->wsibling = -1;
      pgw->flags    = PW_PRESENT | PW_X;
      memcpy (pgw->page.raw, pgr->page.raw, PAGE_SIZE);
      pgw->page.pg = pg;

      latch_unlock (&pgr->ctrl);
      latch_unlock (&pgw->ctrl);

      spx_lock_x (&pgr->data);

      dest->pgr  = pgr;
      dest->pgw  = pgw;
      dest->mode = PHM_X;
      dest->tx   = tx;

      return SUCCESS;
    }
  }
  UNREACHABLE ();
}
