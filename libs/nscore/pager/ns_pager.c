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

#include "pager/ns_pager.h"

#include "lock_table/ns_lock_table.h"
#include "ns_file_pager.h"
#include "ns_page.h"
#include "pager/ns_page_fixture.h"
#include "txn_table/ns_txn_table.h"

#ifndef NDEBUG
#  include "ns_crash.h"
#  include "ns_page_data_list.h"
#  include "ns_testing.h"
#endif

#include <limits.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool
pgr_isnew (const struct pager *p)
{
  DBG_ASSERT (pager, p);

  return (atomic_load (&p->flags) & PGR_ISNEW) != 0;
}

p_size
pgr_get_npages (struct pager *p)
{
  DBG_ASSERT (pager, p);
  return fpgr_get_npages (p->fp);
}

err_t
pgr_get_maybe_writable (
    page_h       *dest,
    struct txn   *tx,
    int           flags,
    pgno          pg,
    struct pager *p,
    bool          writable,
    error        *e
)
{
  if (!writable) {
    return pgr_get (dest, flags, pg, p, e);
  }
  return pgr_get_writable (dest, tx, flags, pg, p, e);
}

err_t
pgr_release (struct pager *p, page_h *h, const int flags, error *e)
{
  DBG_ASSERT (pager, p);
  return pgr_release_with_log (p, h, flags, NULL, e);
}

err_t
pgr_release_if_exists (struct pager *p, page_h *h, int flags, error *e)
{
  if (h->mode != PHM_NONE) {
    return pgr_release (p, h, flags, e);
  }
  return SUCCESS;
}

err_t
pgr_release_with_flush (struct pager *p, page_h *h, const int flags, error *e)
{
  DBG_ASSERT (pager, p);
  struct page_frame *pgr = h->pgr;

  if (pgr_release (p, h, flags, e) < 0) {
    return error_trace (e);
  }
  if (pgr_flush_unsafe (p, pgr, e) < 0) {
    return error_trace (e);
  }
  return SUCCESS;
}

err_t
pgr_release_with_evict (struct pager *p, page_h *h, const int flags, error *e)
{
  DBG_ASSERT (pager, p);
  struct page_frame *pgr = h->pgr;

  if (pgr_release (p, h, flags, e) < 0) {
    return error_trace (e);
  }
  if (pgr_evict_unsafe (p, pgr, e) < 0) {
    return error_trace (e);
  }
  return SUCCESS;
}

err_t
pgr_flush_all_pages (struct pager *p, error *e)
{
  DBG_ASSERT (pager, p);

  for (u32 i = 0; i < MEMORY_PAGE_LEN; ++i) {
    struct page_frame *mp = &p->pages[i];

    latch_lock (&mp->ctrl);

    if ((mp->flags & PW_PRESENT) && !(mp->flags & PW_X)) {
      // Keep going on failure so every other page still gets written. The
      // failure stays in [e] and is returned below.
      (void)pgr_flush_unsafe (p, mp, e);
    }

    latch_unlock (&mp->ctrl);
  }

  return error_trace (e);
}

err_t
pgr_evict_all_pages (struct pager *p, error *e)
{
  DBG_ASSERT (pager, p);

  for (u32 i = 0; i < MEMORY_PAGE_LEN; ++i) {
    struct page_frame *mp = &p->pages[i];

    latch_lock (&mp->ctrl);

    if (mp->flags & PW_PRESENT) {
      ASSERT (!(mp->flags & PW_X));
      // Keep going on failure so every other page still gets evicted. The
      // failure stays in [e] and is returned below.
      (void)pgr_evict_unsafe (p, mp, e);
    }

    latch_unlock (&mp->ctrl);
  }

  return error_trace (e);
}

void
pgr_cancel_if_exists (page_h *h)
{
  if (h->mode == PHM_NONE) {
    return;
  }

  pgr_cancel (h);
}

err_t
pgr_upgrade (page_h *_pg, struct txn *tx, int flags, struct pager *p, error *e)
{
  DBG_ASSERT (pager, p);

  pgno pg = page_h_pgno (_pg);
  // TODO: a failed release is ignored and the page is fetched again anyway.
  // Return early instead, once callers are checked for relying on this.
  (void)pgr_release (p, _pg, flags, e);
  return pgr_get_writable (_pg, tx, flags, pg, p, e);
}

#ifndef NDEBUG

TEST (pager_fill_ht)
{
  struct pgr_fixture f;
  pgr_fixture_create (&f, &ctx->e);

  struct txn tx;
  pgr_begin_txn (&tx, f.p, &ctx->e);

  page_h pgs[MEMORY_PAGE_LEN];

  {
    // Fill up - there is already one page in the pool, the root
    u32 i = 0;
    for (; i < MEMORY_PAGE_LEN / 2; ++i) {
      pgs[i] = page_h_create ();
      pgr_new (&pgs[i], f.p, &tx, PG_DATA_LIST, &ctx->e);
      test_assert_int_equal (pgs[i].mode, PHM_X);
    }

    // One more pgr_new here would block: every frame is either one of these
    // pages or its write twin

    // Release them all
    for (i = 0; i < MEMORY_PAGE_LEN / 2; ++i) {
      dl_set_used (page_h_w (&pgs[i]), DL_DATA_SIZE);
      pgr_release (f.p, &pgs[i], PG_DATA_LIST, &ctx->e);
    }
  }

  // Repeat above
  {
    // Fill half way up - good
    for (u32 i = 0; i < MEMORY_PAGE_LEN / 2; ++i) {
      pgr_new (&pgs[i], f.p, &tx, PG_DATA_LIST, &ctx->e);
      test_assert_int_equal (pgs[i].mode, PHM_X);
    }

    // Release them all
    for (u32 i = 0; i < MEMORY_PAGE_LEN / 2; ++i) {
      dl_set_used (page_h_w (&pgs[i]), DL_DATA_SIZE);
      pgr_release (f.p, &pgs[i], PG_DATA_LIST, &ctx->e);
    }
  }

  pgr_commit (f.p, &tx, &ctx->e);

  pgr_fixture_teardown (&f, &ctx->e);
}

TEST (wal_int)
{
  struct pgr_fixture f;
  page_h             h = page_h_create ();
  pgr_fixture_create (&f, &ctx->e);

  struct txn tx;
  pgr_begin_txn (&tx, f.p, &ctx->e);

  pgr_new (&h, f.p, &tx, PG_DATA_LIST, &ctx->e);

  dl_set_used (page_h_w (&h), DL_DATA_SIZE);
  pgr_release (f.p, &h, PG_DATA_LIST, &ctx->e);

  pgr_commit (f.p, &tx, &ctx->e);

  pgr_fixture_teardown (&f, &ctx->e);
}

// Commit two data pages, so a flush has more than one dirty page to write
static void
pager_test_dirty_two_pages (struct pgr_fixture *f, pgno pgs[2], struct test_ctx *ctx)
{
  page_h h = page_h_create ();
  pgr_begin_txn (&f->tx, f->p, &ctx->e);
  for (int i = 0; i < 2; ++i) {
    pgr_new (&h, f->p, &f->tx, PG_DATA_LIST, &ctx->e);
    dl_make_valid (page_h_w (&h));
    pgs[i] = page_h_pgno (&h);
    pgr_release (f->p, &h, PG_DATA_LIST, &ctx->e);
  }
  pgr_commit (f->p, &f->tx, &ctx->e);
}

// Close on a healthy OS and check both pages read back
static void
pager_test_check_two_pages (struct pgr_fixture *f, const pgno pgs[2], struct test_ctx *ctx)
{
  error_reset (&ctx->e);
  test_err_t_check (pgr_fixture_teardown_with (f, &ctx->e), SUCCESS, &ctx->e);

  struct pager *p = pgr_open ("testdb", ctx->mem, ctx->os, &ctx->e);
  test_assert (p != NULL);
  for (int i = 0; i < 2; ++i) {
    page_h h = page_h_create ();
    test_err_t_check (pgr_get (&h, PG_DATA_LIST, pgs[i], p, &ctx->e), SUCCESS, &ctx->e);
    test_err_t_check (pgr_release (p, &h, PG_DATA_LIST, &ctx->e), SUCCESS, &ctx->e);
  }
  test_err_t_check (pgr_close (p, &ctx->e), SUCCESS, &ctx->e);
}

TEST (pgr_flush_all_pages_fail_os)
{
  err_t ret;
  int   fail_after = 0;

  do {
    TEST_CASE ("Fail after: %d", fail_after)
    {
      struct pgr_fixture f;
      pgno               pgs[2];
      test_err_t_check (
          pgr_fixture_create_with (&f, ctx->faulty_os, ctx->mem, &ctx->e),
          SUCCESS,
          &ctx->e
      );
      pager_test_dirty_two_pages (&f, pgs, ctx);
      test_assert_int_equal (ctx->e.cause_code, SUCCESS);

      faulty_os_fail_after (ctx->faulty_os, fail_after);
      capture_intentional_crash (&ret, &ctx->e)
      {
        ret = pgr_flush_all_pages (f.p, &ctx->e);
      }
      faulty_os_reset (ctx->faulty_os);

      if (ret == ERR_CRASHED) {
        // walos_flush_all drops its latch before crashing - just crash the pager
        pgr_fixture_teardown_with (&f, &ctx->e);
        error_reset (&ctx->e);
      } else {
        // A failed flush leaves the rest dirty - a healthy close writes them
        pager_test_check_two_pages (&f, pgs, ctx);
      }

      fail_after++;
    }
  }
  while (ret != SUCCESS);
}

TEST (pgr_evict_all_pages_fail_os)
{
  err_t ret;
  int   fail_after = 0;

  do {
    TEST_CASE ("Fail after: %d", fail_after)
    {
      struct pgr_fixture f;
      pgno               pgs[2];
      test_err_t_check (
          pgr_fixture_create_with (&f, ctx->faulty_os, ctx->mem, &ctx->e),
          SUCCESS,
          &ctx->e
      );
      pager_test_dirty_two_pages (&f, pgs, ctx);
      test_assert_int_equal (ctx->e.cause_code, SUCCESS);

      faulty_os_fail_after (ctx->faulty_os, fail_after);
      capture_intentional_crash (&ret, &ctx->e)
      {
        ret = pgr_evict_all_pages (f.p, &ctx->e);
      }
      faulty_os_reset (ctx->faulty_os);

      if (ret == ERR_CRASHED) {
        pgr_fixture_teardown_with (&f, &ctx->e);
        error_reset (&ctx->e);
      } else {
        // Pages that failed to write are still cached and dirty
        pager_test_check_two_pages (&f, pgs, ctx);
      }

      fail_after++;
    }
  }
  while (ret != SUCCESS);
}
#endif
