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

#include "c_specx/threading.h"
#include "nscore/aries.h"
#include "nscore/file_pager.h"
#include "nscore/lock_table.h"
#include "nscore/page_h.h"
#include "nscore/pager.h"
#include "nscore/pages/fsm_page.h"
#include "nscore/wal.h"
#include "nscore/wal_ostream.h"

#include <c_specx.h>

#define NAME_MAX 200

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
  u32 len = strlen (dbname);
  if (len > (NAME_MAX - 4))
  {
    error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "DBName is too big. Supported max: %d actual len: %d",
        NAME_MAX - 4,
        len
    );
    return NULL;
  }

  char fname[NAME_MAX];
  char walname[NAME_MAX];
  snprintf (fname, sizeof fname, "%s", dbname);
  snprintf (walname, sizeof walname, "%s.wal", dbname);

  // File pager
  struct file_pager *fp = fpgr_open (fname, PAGE_HEADER_LEN, e);
  if (fp == NULL)
  {
    return NULL;
  }

  struct wal *ww = wal_open (walname, e);
  if (ww == NULL)
  {
    fpgr_close (fp, e);
    return NULL;
  }

  struct lockt *lt = i_malloc (1, sizeof *lt, e);
  if (lt == NULL)
  {
    fpgr_close (fp, e);
    wal_close_and_delete (ww, e);
    return NULL;
  }
  if (lockt_init (lt, e))
  {
    fpgr_close (fp, e);
    wal_close_and_delete (ww, e);
    i_free (lt);
    lockt_destroy (lt);
    return NULL;
  }

  bool          mutex_init = false;
  page_h        root       = page_h_create ();
  struct pager *ret        = NULL;

  if ((ret = i_calloc (1, sizeof *ret, e)) == NULL)
  {
    goto failed;
  }

  // Initialize "easy" things
  *(struct file_pager **)&ret->fp = fp;
  *(struct wal **)&ret->ww        = ww;
  ret->lt                         = lt;
  if (i_mutex_create (&ret->serial_lock, e))
  {
    goto failed;
  }
  mutex_init = true;
  atomic_store (&ret->flags, fpgr_isnew (ret->fp));
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

  atomic_store (&ret->next_tid, 0);

  if (atomic_load (&ret->flags) & PGR_ISNEW)
  {
    // Reset any data in the file pager
    if (fpgr_reset (ret->fp, e))
    {
      goto failed;
    }

    // Write out the starting header
    memset (&ret->header, 0, sizeof (ret->header));
    if (pgr_write_header (ret, e))
    {
      goto failed;
    }

    if (wal_delete_and_reopen (ret->ww, e))
    {
      goto failed;
    }

    if (wal_write_start_lsn (ret->ww, 0, e))
    {
      goto failed;
    }
  }
  else
  {
    if (pgr_read_header (ret, e))
    {
      goto failed;
    }

    if (!ret->header.lsn0valid && !ret->header.lsn1valid)
    {
      error_causef (e, ERR_CORRUPT, "Invalid wal headers");
      goto failed;
    }

    if (wal_isnew (ww))
    {
      if (wal_write_start_lsn (
              ret->ww,
              MAX (ret->header.lsn0, ret->header.lsn1),
              e
          ))
      {
        goto failed;
      }
    }
    else
    {
      lsn start_lsn      = wal_start_lsn (ret->ww);
      lsn next_start_lsn = start_lsn + wal_size (ret->ww);

      if (ret->header.lsn0valid && ret->header.lsn1valid)
      {
        if (start_lsn == MAX (ret->header.lsn0, ret->header.lsn1))
        {
          WRAP_GOTO (pgr_recover (ret, e), failed);
        }
        else if (start_lsn == MIN (ret->header.lsn0, ret->header.lsn1))
        {
          WRAP_GOTO (wal_delete_and_reopen (ret->ww, e), failed);
          WRAP_GOTO (
              wal_write_start_lsn (
                  ret->ww,
                  MAX (ret->header.lsn0, ret->header.lsn1),
                  e
              ),
              failed
          );
        }
        else
        {
          error_causef (e, ERR_CORRUPT, "Existing WAL doesn't match database");
          goto failed;
        }
      }
      else
      {
        if (ret->header.lsn0valid)
        {
          if (ret->header.lsn0 != wal_start_lsn (ret->ww))
          {
            error_causef (e, ERR_CORRUPT, "Invalid header lsn");
            goto failed;
          }
          WRAP_GOTO (pgr_write_lsn1 (ret, start_lsn, e), failed);
        }
        else if (ret->header.lsn1valid)
        {
          if (ret->header.lsn1 != wal_start_lsn (ret->ww))
          {
            error_causef (e, ERR_CORRUPT, "Invalid header lsn");
            goto failed;
          }
          WRAP_GOTO (pgr_write_lsn0 (ret, start_lsn, e), failed);
        }
        else
        {
          UNREACHABLE ();
        }

        WRAP_GOTO (wal_delete_and_reopen (ret->ww, e), failed);
        WRAP_GOTO (wal_write_start_lsn (ret->ww, next_start_lsn, e), failed);
      }
    }
  }

  return ret;

failed:
  ASSERT (error_trace (e));
  if (ret)
  {
    if (mutex_init)
    {
      i_mutex_free (&ret->serial_lock);
    }
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
    wal_close_and_delete (ww, e);
  }
  if (fp)
  {
    fpgr_close (fp, e);
  }
  if (lt)
  {
    lockt_destroy (lt);
  }

  return NULL;
}

#ifndef NTEST
TEST (pager_open)
{
  error e = error_create ();

  TEST_CASE ("green path")
  {
    test_fail_if (pgr_delete_single_file ("testdb", &e));

    struct pager *p = pgr_open_single_file ("testdb", &e);

    pgr_close (p, &e);
  }

  TEST_CASE ("dbname is too long")
  {
    char *name = i_malloc (NAME_MAX, 1, &e);
    for (int i = 0; i < NAME_MAX; ++i)
    {
      name[i] = 'c';
    }
    name[NAME_MAX - 3] = '\0';

    struct pager *p = pgr_open_single_file (name, &e);
    test_assert (p == NULL);
    test_err_t_check (e.cause_code, ERR_INVALID_ARGUMENT, &e);
    e.cause_code = SUCCESS;

    name[NAME_MAX - 4] = '\0';
    p                  = pgr_open_single_file (name, &e);
    test_assert (p != NULL);

    pgr_close (p, &e);

    // Delete the obtuse name
    pgr_delete_single_file (name, &e);

    i_free (name);
  }
}
#endif

#ifndef NTEST
TEST (pgr_open_basic)
{
  error e = error_create ();
  test_fail_if (pgr_delete_single_file ("testdb", &e));

  i_file fp = {0};
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
