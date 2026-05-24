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

#include "nscore/pager.h"
#include "nscore/pages/page.h"

#include <c_specx.h>

err_t
pgr_release_with_log (
    struct pager            *p,
    page_h                  *h,
    int                      flags,
    struct wal_update_write *record,
    error                   *e
)
{
  ASSERT (h->mode == PHM_X || h->mode == PHM_S);
  ASSERT (h->pgr->flags & PW_PRESENT);

  if (h->mode == PHM_X)
  {
    spgno page_lsn = 0;

    // Can only save valid pages
    ASSERT (!page_validate_for_db (&h->pgw->page, flags | PG_SKIP_CHECKSUM, e));

    // Append WAL information
    if (h->tx) // If you pass tx == NULL - there's no WAL logging - used mid
               // ARIES
    {
      // If no record was supplied, append a physical record
      if (record == NULL)
      {
        page_lsn = wal_append_update_log (
            p->ww,
            (struct wal_update_write){
                .tid  = h->tx->tid,
                .type = WUP_PHYSICAL,
                .prev = h->tx->data.last_lsn,
                .phys =
                    {
                        .pg   = page_h_pgno (h),
                        .undo = h->pgr->page.raw,
                        .redo = h->pgw->page.raw,
                    },
            },
            e
        );
      }
      else
      {
        page_lsn = wal_append_update_log (p->ww, *record, e);
      }

      if (page_lsn < 0) { return error_trace (e); }

      // Update the page lsn
      page_set_page_lsn (page_h_w (h), (lsn)page_lsn);

      h->tx->data.last_lsn      = page_lsn;
      h->tx->data.undo_next_lsn = page_lsn;
    }

    // Add page to DPT if this is the first update (RecLSN = LSN of first
    // update)
    if (!dpgt_exists (p->dpt, page_h_pgno (h)))
    {
      if (dpgt_add (p->dpt, page_h_pgno (h), (lsn)page_lsn, e)) { return error_trace (e); }
      h->pgr->flags |= PW_DIRTY;
    }

    memcpy (&h->pgr->page.raw, h->pgw->page.raw, PAGE_SIZE);

    // Update pgw
    latch_lock (&h->pgw->ctrl);
    h->pgw->flags = 0;
    h->pgw->pin   = 0;
    latch_unlock (&h->pgw->ctrl);

    h->pgw = NULL;

    // Update pgr
    latch_lock (&h->pgr->ctrl);
    h->pgr->pin--;
    h->pgr->wsibling = -1;
    latch_unlock (&h->pgr->ctrl);

    h->mode = PHM_S;

    spx_unlock_x (&h->pgr->data);
  }
  else
  {
    // Update pgr
    latch_lock (&h->pgr->ctrl);
    h->pgr->pin--;
    latch_unlock (&h->pgr->ctrl);

    // Unlock from s mode
    spx_unlock_s (&h->pgr->data);
  }

  h->pgr  = NULL;
  h->mode = PHM_NONE;

  return SUCCESS;
}
