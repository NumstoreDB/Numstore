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

#include "nscore/pager/ns_pager.h"

#include "nscore/disk_pager/ns_file_pager.h"
#include "nscore/lock_table/ns_lock_table.h"
#include "nscore/page/ns_page.h"
#include "nscore/page/ns_page_fixture.h"
#include "nscore/txn_table/ns_txn_table.h"

#ifdef TESTING
#  include "core/testing/ns_testing.h"
#  include "nscore/page/ns_page_data_list.h"
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

  return atomic_load (&p->flags) & PGR_ISNEW;
}

p_size
pgr_get_npages (struct pager *p)
{
  return fpgr_get_npages (p->fp);
}

err_t
pgr_get_maybe_writable (
    page_h        *dest,
    struct ns_txn *tx,
    int            flags,
    pgno           pg,
    struct pager  *p,
    bool           writable,
    error         *e
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
  struct page_frame *pgr = h->pgr;

  if (pgr_release (p, h, flags, e)) {
    return error_trace (e);
  }
  if (pgr_flush_unsafe (p, pgr, e)) {
    return error_trace (e);
  }
  return SUCCESS;
}

err_t
pgr_release_with_evict (struct pager *p, page_h *h, const int flags, error *e)
{
  struct page_frame *pgr = h->pgr;

  if (pgr_release (p, h, flags, e)) {
    return error_trace (e);
  }
  if (pgr_evict_unsafe (p, pgr, e)) {
    return error_trace (e);
  }
  return SUCCESS;
}

err_t
pgr_flush_all_pages (struct pager *p, error *e)
{
  for (u32 i = 0; i < MEMORY_PAGE_LEN; ++i) {
    struct page_frame *mp = &p->pages[i];

    latch_lock (&mp->ctrl);

    if (mp->flags & PW_PRESENT && !(mp->flags & PW_X)) {
      ASSERT (!(mp->flags & PW_X));
      pgr_flush_unsafe (p, mp, e);
    }

    latch_unlock (&mp->ctrl);
  }

  return error_trace (e);
}

err_t
pgr_evict_all_pages (struct pager *p, error *e)
{
  for (u32 i = 0; i < MEMORY_PAGE_LEN; ++i) {
    struct page_frame *mp = &p->pages[i];

    latch_lock (&mp->ctrl);

    if (mp->flags & PW_PRESENT) {
      ASSERT (!(mp->flags & PW_X));
      pgr_evict_unsafe (p, mp, e);
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
pgr_upgrade (page_h *_pg, struct ns_txn *tx, int flags, struct pager *p, error *e)
{
  pgno pg = page_h_pgno (_pg);
  pgr_release (p, _pg, flags, e);
  return pgr_get_writable (_pg, tx, flags, pg, p, e);
}

#ifdef TESTING

TEST (pager_fill_ht)
{
  struct pgr_fixture f;
  pgr_fixture_create (&f);

  struct ns_txn tx;
  pgr_begin_txn (&tx, f.p, &f.e);

  page_h pgs[MEMORY_PAGE_LEN];

  {
    // Fill up - there is already one page in the pool, the root
    u32 i = 0;
    for (; i < MEMORY_PAGE_LEN / 2; ++i) {
      pgs[i] = page_h_create ();
      pgr_new (&pgs[i], f.p, &tx, PG_DATA_LIST, &f.e);
      test_assert_equal (pgs[i].mode, PHM_X);
    }

    // This would block
    // pgr_new (&bad, f.p, &tx, PG_DATA_LIST, &f.e);

    // Release them all
    for (i = 0; i < MEMORY_PAGE_LEN / 2; ++i) {
      dl_set_used (page_h_w (&pgs[i]), DL_DATA_SIZE);
      pgr_release (f.p, &pgs[i], PG_DATA_LIST, &f.e);
    }
  }

  // Repeat above
  {
    // Fill half way up - good
    for (u32 i = 0; i < MEMORY_PAGE_LEN / 2; ++i) {
      pgr_new (&pgs[i], f.p, &tx, PG_DATA_LIST, &f.e);
      test_assert_equal (pgs[i].mode, PHM_X);
    }

    // Release them all
    for (u32 i = 0; i < MEMORY_PAGE_LEN / 2; ++i) {
      dl_set_used (page_h_w (&pgs[i]), DL_DATA_SIZE);
      pgr_release (f.p, &pgs[i], PG_DATA_LIST, &f.e);
    }
  }

  pgr_commit (f.p, &tx, &f.e);

  pgr_fixture_teardown (&f);
}

TEST (wal_int)
{
  struct pgr_fixture f;
  page_h             h = page_h_create ();
  pgr_fixture_create (&f);

  struct ns_txn tx;
  pgr_begin_txn (&tx, f.p, &f.e);

  pgr_new (&h, f.p, &tx, PG_DATA_LIST, &f.e);

  dl_set_used (page_h_w (&h), DL_DATA_SIZE);
  pgr_release (f.p, &h, PG_DATA_LIST, &f.e);

  pgr_commit (f.p, &tx, &f.e);

  pgr_fixture_teardown (&f);
}
#endif
