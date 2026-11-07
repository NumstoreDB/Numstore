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

#include "core/ns_error.h"
#include "nscore/page/ns_page_fixture.h"
#include "nscore/pager/ns_pager.h"

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

  switch (res) {
    // This page exists in memory
    case HTAR_SUCCESS: {
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
      if (wclock < 0) {
        return error_trace (e);
      }

      pgw           = &p->pages[wclock];

      pgw->pin      = 1;
      pgw->wsibling = -1;
      pgw->flags    = PW_PRESENT | PW_X;
      memcpy (pgw->page.raw, dest->pgr->page.raw, NS_PAGE_SIZE);
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

    case HTAR_DOESNT_EXIST: {
      latch_unlock (&p->htable_lock);

      // Grab both r and w slots in once
      rclock = pgr_reserve_and_ctrl_lock (p, e);
      if (rclock < 0) {
        return error_trace (e);
      }

      wclock = pgr_reserve_and_ctrl_lock (p, e);
      if (wclock < 0) {
        latch_unlock (&p->pages[rclock].ctrl);
        return error_trace (e);
      }

      data = (hdata_idx){.key = pg, .value = rclock};
      ht_insert_expect_idx (&p->pgno_to_value, data);

      pgr           = &p->pages[data.value];
      pgw           = &p->pages[wclock];

      pgr->pin      = 1;
      pgr->flags    = PW_ACCESS | PW_PRESENT;
      pgr->wsibling = wclock;
      pgr->page.pg  = pg;

      if (fpgr_read (p->fp, pgr->page.raw, pg, e) < 0) {
        latch_unlock (&pgr->ctrl);
        latch_unlock (&pgw->ctrl);
        return error_trace (e);
      }

      if (page_validate_for_db (&pgr->page, flags, e) < 0) {
        latch_unlock (&pgr->ctrl);
        latch_unlock (&pgw->ctrl);
        return error_trace (e);
      }

      // Write frame: copy of the read frame, owned by this txn.
      pgw->pin      = 1;
      pgw->wsibling = -1;
      pgw->flags    = PW_PRESENT | PW_X;
      memcpy (pgw->page.raw, pgr->page.raw, NS_PAGE_SIZE);
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
  UNREACHABLE (); // LCOV_EXCL_LINE
}

#ifndef NDEBUG
#  include "core/ns_testing.h"
#  include "nscore/page/ns_page_data_list.h"

/**
 * TODO - on a cache miss pgr_get_writable inserts the read frame into
 * p->pgno_to_value (pin = 1, PW_PRESENT) *before* fpgr_read. When the read
 * (or page_validate_for_db) fails it only drops the ctrl latches, so the
 * page table keeps a pinned frame full of garbage for [pg]:
 *
 * - the next get of [pg] hits HTAR_SUCCESS and hands out that garbage
 * - pgr_close -> pgr_evict_all_pages asserts mp->pin == 0 on it
 *
 * pgr_get does it in the right order (read, validate, then insert). Fix: do
 * the same here, or remove the entry and clear the frame on failure.
 */
TEST_DISABLED (pgr_get_writable_fail_os)
{
  struct pgr_fixture f;
  test_err_t_check (
      pgr_fixture_create_with (&f, ctx->faulty_os, ctx->mem, &ctx->e),
      SUCCESS,
      &ctx->e
  );

  // Commit one data page, then push it to disk
  page_h h = page_h_create ();
  pgr_begin_txn (&f.tx, f.p, &ctx->e);
  pgr_new (&h, f.p, &f.tx, PG_DATA_LIST, &ctx->e);
  dl_make_valid (page_h_w (&h));
  const pgno pg = page_h_pgno (&h);
  pgr_release (f.p, &h, PG_DATA_LIST, &ctx->e);
  pgr_commit (f.p, &f.tx, &ctx->e);
  pgr_evict_all_pages (f.p, &ctx->e);
  test_assert_int_equal (ctx->e.cause_code, SUCCESS);

  test_err_t_check (pgr_begin_txn (&f.tx, f.p, &ctx->e), SUCCESS, &ctx->e);

  err_t ret;
  int   fail_after = 0;

  do {
    TEST_CASE ("Fail after: %d", fail_after)
    {
      error_reset (&ctx->e);

      faulty_os_fail_after (ctx->faulty_os, fail_after);
      ret = pgr_get_writable (&h, &f.tx, PG_DATA_LIST, pg, f.p, &ctx->e);
      faulty_os_reset (ctx->faulty_os);

      fail_after++;
    }
  }
  while (ret != SUCCESS);

  // The page handed out is the one on disk, not a leftover from a failure
  test_assert_int_equal (
      page_get_checksum (page_h_ro (&h)),
      page_compute_checksum (page_h_ro (&h))
  );
  pgr_cancel (&h);
  test_err_t_check (pgr_commit (f.p, &f.tx, &ctx->e), SUCCESS, &ctx->e);

  test_err_t_check (pgr_fixture_teardown_with (&f, &ctx->e), SUCCESS, &ctx->e);
}
#endif
