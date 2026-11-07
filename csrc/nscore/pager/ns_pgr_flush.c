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
#include "core/ns_testing.h"
#include "nscore/page/ns_page_fixture.h"
#include "nscore/page/ns_page_h.h"
#include "nscore/pager/ns_pager.h"
#include "nscore/wal/ns_wal.h"
#include "nscore/wal/ns_wal_ostream.h"

err_t
pgr_flush_unsafe (const struct pager *p, struct page_frame *mp, error *e)
{
  DBG_ASSERT (pager, p);
  ASSERT (mp->flags & PW_PRESENT);
  ASSERT (!(mp->flags & PW_X));

  // Only need to write it out if it's dirty
  if (dpgt_exists (p->dpt, mp->page.pg)) {
    // WAL invariant: always flush page to wal before
    // it's flushed to disk
    // Remember:
    //    page_lsn = latest log page that modified this page
    if (p->ww != NULL) {
      wal_flush_all (p->ww);
    }

    // Set page checksum before flushing
    page_set_checksum (&mp->page, page_compute_checksum (&mp->page));

    // Write the page to the database
    if (fpgr_write (p->fp, mp->page.raw, mp->page.pg, e) < 0) {
      goto theend;
    }

    dpgt_remove_expect (p->dpt, mp->page.pg);
  }

theend:
  return error_trace (e);
}

#ifndef NDEBUG
TEST (pgr_flush_unsafe)
{
  TEST_CASE ("the wal goes out before the page, during recovery too")
  {
    struct pgr_fixture f;
    pgr_fixture_create (&f, &ctx->e);

    // Recovery in progress. The WAL-before-page rule still has to hold here:
    // undo appends a CLR and stamps its lsn onto the page it just changed, so
    // writing that page first leaves a page naming an lsn the log never reaches.
    f.p->flags |= PGR_ISRESTARTING;

    page_h ph = page_h_create ();
    test_assert_int_equal (pgr_new (&ph, f.p, &f.tx, PG_DATA_LIST, &ctx->e), SUCCESS);

    // The log for this page is still sitting in the writer's buffer
    const lsn before = f.p->ww->ostream->flushed_lsn;
    test_assert (wal_size (f.p->ww) > before);
    test_assert (cbuffer_len (&f.p->ww->ostream->buffer) > 0);

    // Releasing with a flush writes the page, so it has to push the log first.
    // PG_PERMISSIVE because an empty data list page is not a valid one and this
    // case is about flush ordering, not page contents.
    test_assert_int_equal (pgr_release_with_flush (f.p, &ph, PG_PERMISSIVE, &ctx->e), SUCCESS);
    test_assert (f.p->ww->ostream->flushed_lsn > before);
    test_assert_type_equal (cbuffer_len (&f.p->ww->ostream->buffer), 0U, u32, PRIu32);

    f.p->flags &= ~PGR_ISRESTARTING;
    pgr_fixture_teardown (&f, &ctx->e);
  }
}
#endif
