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

#include "c_specx/error.h"
#include "nscore/pager.h"
#include "nscore/pages/page.h"

#include <c_specx.h>

void
pgr_unfix (struct pager *p, page_h *h, int flags)
{
  ASSERT (h->mode == PHM_X || h->mode == PHM_S);

  latch_lock (&h->pgr->ctrl);

  ASSERT (h->pgr->flags & PW_PRESENT);

  // Need to save this page
  if (h->mode == PHM_X)
  {
    spgno page_lsn = 0;

    // Can only save valid pages
    ASSERT (!page_validate_for_db (&h->pgw->page, flags | PG_SKIP_CHECKSUM, NULL));

    h->pgr->flags |= PW_DIRTY;

    memcpy (&h->pgr->page.raw, h->pgw->page.raw, PAGE_SIZE);

    latch_lock (&h->pgw->ctrl);

    h->pgw->flags    = 0; // Release pgw
    h->pgr->wsibling = -1;

    latch_unlock (&h->pgw->ctrl);

    h->pgw = NULL;

    h->mode = PHM_S;

    // Unlock read page data
    spx_unlock_x (&h->pgr->data);
  }
  else
  {
    // Unlock read page data
    spx_unlock_s (&h->pgr->data);
  }

  h->pgr->pin--;
  latch_unlock (&h->pgr->ctrl);

  h->pgr  = NULL;
  h->mode = PHM_NONE;
}
