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

#include "nscore/aries.h"
#include "nscore/compile_config.h"
#include "nscore/file_pager.h"
#include "nscore/lock_table.h"
#include "nscore/page_fixture.h"

#include <c_specx.h>

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
pgr_flush_wall (struct pager *p, error *e)
{
  DBG_ASSERT (pager, p);
  return wal_flush_all (p->ww, e);
}

void
pgr_attach_lock_table (struct pager *p, struct lockt *lt)
{
  DBG_ASSERT (pager, p);
  ASSERT (p->lt == NULL);
  p->lt = lt;
}

err_t
pgr_read_header (struct pager *p, error *e)
{
  if (fpgr_read_header (p->fp, p->_header, 0, PAGE_HEADER_LEN, e))
  {
    return error_trace (e);
  }

  lsn lsn0;
  u32 lsn0csm;
  u32 lsn0csm_actual = checksum_init ();

  lsn lsn1;
  u32 lsn1csm;
  u32 lsn1csm_actual = checksum_init ();

  memcpy (&lsn0, p->_header + LSN0_OFST, sizeof (lsn));
  memcpy (&lsn0csm, p->_header + LSN0_CSM_OFST, sizeof (u32));

  memcpy (&lsn1, p->_header + LSN1_OFST, sizeof (lsn));
  memcpy (&lsn1csm, p->_header + LSN1_CSM_OFST, sizeof (u32));

  checksum_execute (&lsn0csm_actual, (void *)&lsn0, sizeof (lsn));
  checksum_execute (&lsn1csm_actual, (void *)&lsn1, sizeof (lsn));

  p->header = (struct pager_header){
      .lsn0      = lsn0,
      .lsn0csm   = lsn0csm,
      .lsn0valid = lsn0csm == lsn0csm_actual,
      .lsn1      = lsn1,
      .lsn1csm   = lsn1csm,
      .lsn1valid = lsn1csm == lsn1csm_actual,
  };

  return SUCCESS;
}

err_t
pgr_write_header (struct pager *p, error *e)
{
  p->header.lsn0csm = checksum_init ();
  p->header.lsn1csm = checksum_init ();

  checksum_execute (&p->header.lsn0csm, (void *)&p->header.lsn0, sizeof (lsn));
  checksum_execute (&p->header.lsn1csm, (void *)&p->header.lsn1, sizeof (lsn));

  memcpy (p->_header + LSN0_OFST, &p->header.lsn0, sizeof (lsn));
  memcpy (p->_header + LSN0_CSM_OFST, &p->header.lsn0csm, sizeof (u32));
  memcpy (p->_header + LSN1_OFST, &p->header.lsn1, sizeof (lsn));
  memcpy (p->_header + LSN1_CSM_OFST, &p->header.lsn1csm, sizeof (u32));

  return fpgr_write_header (p->fp, p->_header, 0, PAGE_HEADER_LEN, e);
}

err_t
pgr_write_lsn0 (struct pager *p, lsn lsn0, error *e)
{
  p->header.lsn0    = lsn0;
  p->header.lsn0csm = checksum_init ();
  checksum_execute (&p->header.lsn0csm, (void *)&lsn0, sizeof (lsn));

  memcpy (p->_header + LSN0_OFST, &p->header.lsn0, sizeof (lsn));
  memcpy (p->_header + LSN0_CSM_OFST, &p->header.lsn0csm, sizeof (u32));

  return fpgr_write_header (
      p->fp,
      p->_header,
      LSN0_OFST,
      sizeof (lsn) + sizeof (u32),
      e
  );
}

err_t
pgr_write_lsn1 (struct pager *p, lsn lsn1, error *e)
{
  p->header.lsn1    = lsn1;
  p->header.lsn1csm = checksum_init ();
  checksum_execute (&p->header.lsn1csm, (void *)&lsn1, sizeof (lsn));

  memcpy (p->_header + LSN1_OFST, &p->header.lsn1, sizeof (lsn));
  memcpy (p->_header + LSN1_CSM_OFST, &p->header.lsn1csm, sizeof (u32));

  return fpgr_write_header (
      p->fp,
      p->_header + LSN1_OFST,
      LSN1_OFST,
      sizeof (lsn) + sizeof (u32),
      e
  );
}

err_t
pgr_write_next_lsn (struct pager *p, lsn l, error *e)
{
  if (p->header.lsn0 > p->header.lsn1)
  {
    return pgr_write_lsn1 (p, l, e);
  }
  else
  {
    return pgr_write_lsn0 (p, l, e);
  }
}

err_t
pgr_recover (struct pager *p, error *e)
{
  // Run ARIES recovery
  struct aries_ctx ctx;
  if (aries_ctx_create (&ctx, e))
  {
    return error_trace (e);
  }

  if (pgr_restart (p, &ctx, e))
  {
    return error_trace (e);
  }

  // Start transactions one past maximum txid
  atomic_store (&p->next_tid, ctx.max_tid + 1);

  return SUCCESS;
}

#ifndef NTEST

TEST (pager_fill_ht)
{
  struct pgr_fixture f;
  pgr_fixture_create (&f);

  struct txn tx;
  pgr_begin_txn (&tx, f.p, &f.e);

  page_h pgs[MEMORY_PAGE_LEN];
  page_h bad = page_h_create ();

  {
    // Fill up - there is already one page in the pool, the root
    u32 i = 0;
    for (; i < MEMORY_PAGE_LEN / 2; ++i)
    {
      pgs[i] = page_h_create ();
      pgr_new (&pgs[i], f.p, &tx, PG_DATA_LIST, &f.e);
      test_assert_equal (pgs[i].mode, PHM_X);
    }

    // This would block
    // pgr_new (&bad, f.p, &tx, PG_DATA_LIST, &f.e);

    // Release them all
    for (i = 0; i < MEMORY_PAGE_LEN / 2; ++i)
    {
      dl_set_used (page_h_w (&pgs[i]), DL_DATA_SIZE);
      pgr_release (f.p, &pgs[i], PG_DATA_LIST, &f.e);
    }
  }

  // Repeat above
  {
    // Fill half way up - good
    for (u32 i = 0; i < MEMORY_PAGE_LEN / 2; ++i)
    {
      pgr_new (&pgs[i], f.p, &tx, PG_DATA_LIST, &f.e);
      test_assert_equal (pgs[i].mode, PHM_X);
    }

    // Release them all
    for (u32 i = 0; i < MEMORY_PAGE_LEN / 2; ++i)
    {
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

  struct txn tx;
  pgr_begin_txn (&tx, f.p, &f.e);

  pgr_new (&h, f.p, &tx, PG_DATA_LIST, &f.e);

  dl_set_used (page_h_w (&h), DL_DATA_SIZE);
  pgr_release (f.p, &h, PG_DATA_LIST, &f.e);

  pgr_commit (f.p, &tx, &f.e);

  pgr_fixture_teardown (&f);
}
#endif

void
i_log_page_table (const int log_level, bool only_present, struct pager *p)
{
  DBG_ASSERT (pager, p);

  for (u32 i = 0; i < MEMORY_PAGE_LEN; ++i)
  {
    const struct page_frame *mp = &p->pages[i];
    if (mp->flags & PW_PRESENT)
    {
      i_printf (
          log_level,
          "%u |(PAGE)    pg: %" PRpgno
          " pin: %d ax: %d drt: %d prsn: %d "
          "sib: %d type: %d data: %d ctrl: %d|\n",
          i,
          mp->page.pg,
          mp->pin,
          mp->flags & PW_ACCESS,
          mp->flags & PW_DIRTY,
          mp->flags & PW_PRESENT,
          mp->wsibling,
          page_get_type (&mp->page),
          mp->data,
          mp->ctrl
      );
    }
    else if (!only_present)
    {
      i_printf (log_level, "%u | |\n", i);
    }
  }
}

err_t
pgr_refresh_wal (struct pager *p, error *e)
{
  DBG_ASSERT (pager, p);
  wal_delete_and_reopen (p->ww, e);
  return error_trace (e);
}
