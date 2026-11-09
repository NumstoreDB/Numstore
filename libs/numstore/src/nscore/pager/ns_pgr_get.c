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
#include "core/ns_stdtypes.h"
#include "core/ns_testing.h"
#include "core/os/ns_os.h"
#include "nscore/page/ns_page.h"
#include "nscore/page/ns_page_fixture.h"
#include "nscore/page/ns_page_h.h"
#include "nscore/pager/ns_pager.h"

#include <stdatomic.h>

err_t
pgr_get (page_h *dest, int flags, pgno pg, struct pager *p, error *e)
{
  DBG_ASSERT (pager, p);

  struct page_frame *pgr = NULL; // Read frame
  hdata_idx          data;
  i32                clock; // Frame index for a page read from disk
  hta_res            res;

  latch_lock (&p->htable_lock);
  res = ht_get_idx (&p->pgno_to_value, &data, pg);

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

      spx_lock_s (&pgr->data);

      return SUCCESS;
    }
    case HTAR_DOESNT_EXIST: {
      latch_unlock (&p->htable_lock);

      // Otherwise, we'll scan for an open spot
      clock = pgr_reserve_and_ctrl_lock (p, e);
      if (clock < 0) {
        return error_trace (e);
      }

      pgr = &p->pages[clock];

      if (fpgr_read (p->fp, pgr->page.raw, pg, e) < 0) {
        latch_unlock (&pgr->ctrl);
        return error_trace (e);
      }

      if (page_validate_for_db (&pgr->page, flags, e) < 0) {
        latch_unlock (&pgr->ctrl);
        return error_trace (e);
      }

      pgr->pin      = 1;
      pgr->flags    = PW_ACCESS | PW_PRESENT;
      pgr->wsibling = -1;
      pgr->page.pg  = pg;

      ht_insert_expect_idx (
          &p->pgno_to_value,
          (hdata_idx){
              .key   = pg,
              .value = (u32)clock,
          }
      );

      latch_unlock (&pgr->ctrl);

      dest->pgr  = pgr;
      dest->pgw  = NULL;
      dest->mode = PHM_S;

      spx_lock_s (&pgr->data);

      return SUCCESS;
    }
  }

  UNREACHABLE (); // LCOV_EXCL_LINE
}

#ifndef NDEBUG
// A page whose stored checksum doesn't match its contents is corruption
TEST (pgr_get_invalid_checksum)
{
  // Setup - one committed data page on disk
  page_h             pg = page_h_create ();
  struct pgr_fixture pf;
  pgr_fixture_create (&pf, &ctx->e);

  struct txn tx;
  pgr_begin_txn (&tx, pf.p, &ctx->e);

  pgr_new (&pg, pf.p, &tx, PG_DATA_LIST, &ctx->e);
  dl_make_valid (page_h_w (&pg));

  const pgno _pg = page_h_pgno (&pg);
  pgr_release_with_flush (pf.p, &pg, PG_DATA_LIST, &ctx->e);

  pgr_commit (pf.p, &tx, &ctx->e);

  pgr_get (&pg, PG_DATA_LIST, _pg, pf.p, &ctx->e);
  test_assert_type_equal (
      page_get_checksum (page_h_ro (&pg)),
      page_compute_checksum (page_h_ro (&pg)),
      u32,
      PRIu32
  );

  // Write a copy with a wrong checksum straight to the file
  page fake_page;
  memcpy (fake_page.raw, page_h_ro (&pg)->raw, NS_PAGE_SIZE);
  fake_page.pg = page_h_pgno (&pg);
  page_set_checksum (&fake_page, page_get_checksum (&fake_page) + 1);

  pgr_release_with_evict (pf.p, &pg, PG_DATA_LIST, &ctx->e);

  fpgr_write (pf.p->fp, fake_page.raw, fake_page.pg, &ctx->e);

  // Run - read it back from disk
  test_err_t_check (pgr_get (&pg, PG_DATA_LIST, _pg, pf.p, &ctx->e), ERR_CORRUPT, &ctx->e);

  pgr_fixture_teardown (&pf, &ctx->e);
}

TEST (pgr_get_fail_os)
{
  struct pgr_fixture f;
  pgr_fixture_create_with (&f, ctx->faulty_os, ctx->mem, &ctx->e);
  pgno   pg = pgr_fixture_new_page_txn_evict (&f, &ctx->e);

  page_h h  = page_h_create ();
  err_t  ret;
  int    fail_after = 0;

  do {
    TEST_CASE ("Fail after: %d", fail_after)
    {
      error_reset (&ctx->e);

      // Get a page
      faulty_os_fail_after (ctx->faulty_os, fail_after);
      ret = pgr_get (&h, PG_DATA_LIST, pg, f.p, &ctx->e);
      faulty_os_reset (ctx->faulty_os);

      if (ret != SUCCESS) {
        // Page should remain as NONE
        test_assert_int_equal (h.mode, PHM_NONE);
      }

      fail_after++;
    }
  }
  while (ret != SUCCESS);

  // The page read back is the page that was written
  test_assert_type_equal (page_h_pgno (&h), pg, pgno, PRpgno);

  u32 crc        = page_get_checksum (page_h_ro (&h));
  u32 actual_crc = page_compute_checksum (page_h_ro (&h));

  test_assert_type_equal (crc, actual_crc, u32, PRIu32);
  pgr_release (f.p, &h, PG_DATA_LIST, &ctx->e);

  pgr_fixture_teardown_with (&f, &ctx->e);
}

TEST (pgr_get_full_buffer_fail_os)
{
  struct pgr_fixture f;
  pgr_fixture_create_with (&f, ctx->faulty_os, ctx->mem, &ctx->e);

  // Create our interested page - this is what we'll get later
  pgno pg = pgr_fixture_new_page_txn (&f, &ctx->e);

  pgr_fixture_fill_buffer_pool (&f, MEMORY_PAGE_LEN, &ctx->e);

  // This next one should cause an eviction because
  // the buffer pool is full - meaning reserve will fail
  faulty_os_fail_after (ctx->faulty_os, 0);
  page_h h   = page_h_create ();
  err_t  ret = pgr_get (&h, PG_DATA_LIST, pg, f.p, &ctx->e);

  test_assert (ret != SUCCESS);
  test_assert (h.mode == PHM_NONE);
  error_reset (&ctx->e);

  pgr_fixture_teardown_with (&f, &ctx->e);
}
#endif
