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

#include "dirty_page_table.h"
#include "file_pager.h"
#include "pager.h"
#include "pages/page.h"

err_t
pgr_flush_unsafe (const struct pager *p, struct page_frame *mp, error *e)
{
  ASSERT (mp->flags & PW_PRESENT);
  ASSERT (!(mp->flags & PW_X));

  // Only need to write it out if it's dirty
  if (dpgt_exists (p->dpt, mp->page.pg))
  {
    if (!(p->flags & PGR_ISRESTARTING) && p->ww)
    {
      // WAL invariant: always flush page to wal before
      // it's flushed to disk
      // Remember:
      //    page_lsn = latest log page that modified this page
      const lsn plsn = page_get_page_lsn (&mp->page);
      if (wal_flush_all (p->ww, e))
      {
        goto theend;
      }
    }

    // Set page checksum before flushing
    page_set_checksum (&mp->page, page_compute_checksum (&mp->page));

    // Write the page to the database
    if (fpgr_write (p->fp, mp->page.raw, mp->page.pg, e))
    {
      goto theend;
    }

    dpgt_remove_expect (p->dpt, mp->page.pg);
  }

theend:
  return error_trace (e);
}
