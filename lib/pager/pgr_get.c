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

#include "c_specx/concurrency/spx_latch.h"
#include "c_specx/dev/assert.h"
#include "c_specx_dev.h"
#include "pager.h"
#include "pager/page_fixture.h"
#include "pager/page_h.h"
#include "pages/data_list.h"
#include "pages/page.h"

err_t
pgr_get (page_h *dest, int flags, pgno pg, struct pager *p, error *e)
{
  struct page_frame *pgr = NULL; // Read frame
  hdata_idx data;                // The data to retrieve
  i32 clock;                     // Location of the new page

  latch_lock (&p->l);
  hta_res res = ht_get_idx (&p->pgno_to_value, &data, pg);

  switch (res)
    {
    // This page exists in memory
    case HTAR_SUCCESS:
      {
        pgr = &p->pages[data.value];

        latch_lock (&pgr->ctrl);
        spx_lock_s (&pgr->data); // This won't be released here
        latch_unlock (&p->l);

        pgr->pin++;

        latch_unlock (&pgr->ctrl);

        dest->pgr = pgr;
        dest->pgw = NULL;
        dest->mode = PHM_S;

        return SUCCESS;
      }
    case HTAR_DOESNT_EXIST:
      {
        // Otherwise, we'll scan for an open spot
        latch_unlock (&p->l);

        clock = pgr_reserve_and_ctrl_lock (p, e);
        if (clock < 0)
          {
            return error_trace (e);
          }

        // ctrl is locked
        pgr = &p->pages[clock];

        if (ospgr_read (p->fp, pgr->page.raw, pg, e))
          {
            latch_unlock (&pgr->ctrl);
            return error_trace (e);
          }

        // Might remove this
        if (page_validate_for_db (&pgr->page, flags, e))
          {
            latch_unlock (&pgr->ctrl);
            return error_trace (e);
          }

        pgr->pin = 1;                        // Singular owner
        pgr->flags = PW_ACCESS | PW_PRESENT; // Accessed and present
        pgr->wsibling = -1;                  // No sibling
        pgr->page.pg = pg;                   // Set the page number

        // Insert this entry into the hash table
        ht_insert_expect_idx (
            &p->pgno_to_value,
            (hdata_idx){
                .key = pg,
                .value = clock,
            });

        spx_lock_s (&pgr->data); // This doesn't unlock here
        latch_unlock (&pgr->ctrl);

        // Set the page data
        dest->pgr = pgr;
        dest->pgw = NULL;
        dest->mode = PHM_S;

        return SUCCESS;
      }
    }

  UNREACHABLE ();
}

#ifndef NTEST
TEST (pgr_get_invalid_checksum)
{
  page_h pg = page_h_create ();
  error e = error_create ();
  struct pgr_fixture pf;
  pgr_fixture_create (&pf);

  struct txn tx;
  pgr_begin_txn (&tx, pf.p, &pf.e);

  pgr_new (&pg, pf.p, &tx, PG_DATA_LIST, &pf.e);
  dl_make_valid (page_h_w (&pg));

  const pgno _pg = page_h_pgno (&pg);
  pgr_release_with_flush (pf.p, &pg, PG_DATA_LIST, &pf.e);

  pgr_commit (pf.p, &tx, &pf.e);

  pgr_get (&pg, PG_DATA_LIST, _pg, pf.p, &pf.e);
  test_assert_int_equal (page_get_checksum (page_h_ro (&pg)),
                         page_compute_checksum (page_h_ro (&pg)));

  // Force checksum to be different
  page fake_page;
  memcpy (fake_page.raw, page_h_ro (&pg)->raw, PAGE_SIZE);
  fake_page.pg = page_h_pgno (&pg);
  page_set_checksum (&fake_page, page_get_checksum (&fake_page) + 1);

  pgr_release_with_evict (pf.p, &pg, PG_DATA_LIST, &pf.e);

  // Force a invalid write
  ospgr_write (pf.p->fp, fake_page.raw, fake_page.pg, &pf.e);

  // This one will fail
  test_err_t_check (pgr_get (&pg, PG_DATA_LIST, _pg, pf.p, &pf.e), ERR_CORRUPT, &pf.e);

  pgr_fixture_teardown (&pf);
}
#endif
