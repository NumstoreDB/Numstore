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

#include "aries/aries.h"
#include "c_specx.h"
#include "c_specx/concurrency/periodic_task.h"
#include "c_specx/intf/os/memory.h"
#include "lockt/lock_table.h"
#include "os_pager/file_pager.h"
#include "os_pager/os_pager.h"
#include "pager.h"
#include "pager/page_h.h"
#include "pages/fsm_page.h"
#include "wal/wal.h"
#include "wal/wal_ostream.h"

#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 260
#endif

/**
 * TODO:
 *  - Lock the file on open to ensure that
 *    this is thread safe
 */
struct pager *
pgr_open (struct os_pager *fp, struct os_wal *ww, struct lockt *lt, error *e)
{
  page_h root = page_h_create ();
  struct pager *ret = NULL;

  if ((ret = i_calloc (1, sizeof *ret, e)) == NULL)
    {
      goto failed;
    }

  // Initialize "easy" things
  *(struct os_pager **)&ret->fp = fp;
  *(struct os_wal **)&ret->ww = ww;
  ret->lt = lt;
  ret->iown_fp = false;
  ret->iown_ww = false;
  ret->iown_lt = false;
  atomic_store (&ret->flags, ospgr_get_npages (ret->fp) == 0 ? PGR_ISNEW : 0);
  atomic_store (&ret->clock, 0);
  ht_init_idx (&ret->pgno_to_value, ret->_hdata, MEMORY_PAGE_LEN);
  latch_init (&ret->htable_lock);
  latch_init (&ret->pgrnew_lock);

  // Open the Dirty page table
  *(struct dpg_table **)&ret->dpt = dpgt_open (e);
  if (ret->dpt == NULL)
    {
      goto failed;
    }

  // Open the transaction table
  *(struct txn_table **)&ret->tnxt = txnt_open (e);
  if (ret->tnxt == NULL)
    {
      goto failed;
    }

  // Initialize (but don't start) the checkpoint task
  if (periodic_task_init (&ret->checkpoint_task, e))
    {
      goto failed;
    }

  if (atomic_load (&ret->flags) & PGR_ISNEW)
    {
      i_log_info ("Creating a new database\n");

      // Reset any data in the file pager
      if (ospgr_reset (ret->fp, e))
        {
          goto failed;
        }

      // Reset the wal
      if (oswal_reset (ret->ww, e))
        {
          goto failed;
        }

      // Start transactions at 0
      atomic_store (&ret->next_tid, 0);
    }
  else
    {
      // Run ARIES recovery
      struct aries_ctx ctx;
      if (aries_ctx_create (&ctx, e))
        {
          goto failed;
        }

      if (pgr_restart (ret, &ctx, e))
        {
          goto failed;
        }

      // Start transactions one past maximum txid
      atomic_store (&ret->next_tid, ctx.max_tid + 1);
    }

  return ret;

failed:
  ASSERT (e->cause_code);
  if (ret)
    {
      pgr_cancel_if_exists (ret, &root);
      if (ret->dpt)
        {
          dpgt_close (ret->dpt);
        }
      if (ret->tnxt)
        {
          txnt_close (ret->tnxt);
        }
      i_free (ret);
    }

  if (ww)
    {
      oswal_close (ww, e);
    }
  if (fp)
    {
      ospgr_close (fp, e);
    }

  return NULL;
}

/*
 * pgr_open_single_file — standard file-backed entry point.
 *
 * Creates [dbname] if it does not exist, constructs a file_pager and a
 * file-backed WAL, then delegates to pgr_open().  Directory cleanup on
 * first-open failure is handled here because only this function knows the
 * path.
 *
 * NEW DATABASE (file is empty):
 *   Sets PGR_ISNEW so the caller can distinguish new databases from existing.
 *
 * EXISTING DATABASE:
 *   Runs the three-phase ARIES restart via pgr_open().
 */
struct pager *
pgr_open_single_file (const char *dbname, error *e)
{
  char fname[PATH_MAX];
  char walname[PATH_MAX];
  snprintf (fname, sizeof fname, "%s", dbname);
  snprintf (walname, sizeof walname, "%s.wal", dbname);

  struct os_pager *fp = fpgr_open_os (fname, e);
  if (fp == NULL)
    {
      return NULL;
    }

  struct os_wal *ww = wal_open_os (walname, e);
  if (ww == NULL)
    {
      ospgr_close (fp, e);
      return NULL;
    }

  struct lockt *lt = i_malloc (1, sizeof *lt, e);
  if (lt == NULL)
    {
      ospgr_close (fp, e);
      oswal_close (ww, e);
      return NULL;
    }
  if (lockt_init (lt, e))
    {
      ospgr_close (fp, e);
      oswal_close (ww, e);
      i_free (lt);
      lockt_destroy (lt);
      return NULL;
    }

  struct pager *p = pgr_open (fp, ww, lt, e);
  if (p == NULL)
    {
      return NULL;
    }

  p->iown_ww = true;
  p->iown_fp = true;
  p->iown_lt = true;

  return p;
}

#ifndef NTEST
TEST (pager_open)
{
  error e = error_create ();

  // Green path
  {
    test_fail_if (pgr_delete_single_file ("testdb", &e));

    struct pager *p = pgr_open_single_file ("testdb", &e);

    pgr_close (p, &e);
  }
}
#endif

#ifndef NTEST
TEST (pgr_open_basic)
{
  error e = error_create ();
  test_fail_if (pgr_delete_single_file ("testdb", &e));

  i_file fp = { 0 };
  i_open_rw (&fp, "testdb", &e);

  // File is shorter than page size
  test_fail_if (i_truncate (&fp, PAGE_SIZE - 1, &e));
  struct pager *p = pgr_open_single_file ("testdb", &e);
  test_assert_int_equal (e.cause_code, ERR_CORRUPT);
  test_assert_equal (p, NULL);
  e.cause_code = SUCCESS;

  // Half a page
  test_fail_if (i_truncate (&fp, PAGE_SIZE / 2, &e));
  p = pgr_open_single_file ("testdb", &e);
  test_assert_int_equal (e.cause_code, ERR_CORRUPT);
  test_assert_equal (p, NULL);
  e.cause_code = SUCCESS;

  // 0 pages
  test_fail_if (i_truncate (&fp, 0, &e));
  p = pgr_open_single_file ("testdb", &e);
  test_assert_int_equal (e.cause_code, SUCCESS);
  test_assert_int_equal ((int)pgr_get_npages (p), 0);
  test_fail_if (pgr_close (p, &e));

  // Tear down
  test_fail_if (i_close (&fp, &e));
  test_fail_if (pgr_delete_single_file ("testdb", &e));
}
#endif
