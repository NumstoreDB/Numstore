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
#include "core/ns_utils.h"
#include "core/os/ns_file.h"
#include "core/os/ns_filesystem.h"
#include "core/os/ns_memory.h"
#include "nscore/disk_pager/ns_file_pager.h"
#include "nscore/dpg_table/ns_dirty_page_table.h"
#include "nscore/lock_table/ns_lock_table.h"
#include "nscore/page/ns_page_fixture.h"
#include "nscore/pager/ns_pager.h"
#include "nscore/txn_table/ns_txn_table.h"
#include "nscore/wal/ns_wal.h"

#ifdef TESTING
#  include "core/testing/ns_testing.h"
#endif

#include <limits.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

err_t
pgr_delete_single_file (const char *dbname, error *e)
{
  char fname[PATH_MAX];
  char walname[PATH_MAX];
  snprintf (fname, sizeof fname, "%s", dbname);
  snprintf (walname, sizeof walname, "%s.wal", dbname);

  i_remove_quiet (default_filesystem (), fname, e);
  i_remove_quiet (default_filesystem (), walname, e);

  return error_trace (e);
}

static err_t
pgr_read_header (struct pager *p, error *e)
{
  if (fpgr_read_header (p->fp, p->_header, 0, PAGE_HEADER_LEN, e)) {
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

/******************************************************************************
 * SECTION: pgr_open
 * ----------------------------------------------------------------------------
 * @brief Open a new pager
 ******************************************************************************/

#ifdef _WIN32
#  define NS_NAME_MAX 50
#else
#  define NS_NAME_MAX 200
#endif

/*
 * pgr_open - standard file-backed entry point.
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
pgr_open (const char *dbname, struct i_mem mem, struct i_file_system fs, error *e)
{
  u32 len = strlen (dbname);
  if (len > (NS_NAME_MAX - 4)) {
    error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "DBName is too big. Supported max: %d actual len: %d",
        NS_NAME_MAX - 4,
        len
    );
    return NULL;
  }

  char fname[NS_NAME_MAX];
  char walname[NS_NAME_MAX];
  snprintf (fname, sizeof fname, "%s", dbname);
  snprintf (walname, sizeof walname, "%s.wal", dbname);

  // File pager
  struct file_pager *fp = fpgr_open (fname, mem, fs, PAGE_HEADER_LEN, e);
  if (fp == NULL) {
    return NULL;
  }

  struct wal *ww = wal_open (walname, mem, fs, e);
  if (ww == NULL) {
    fpgr_close (fp, e);
    return NULL;
  }

  struct lockt *lt = i_malloc (mem, 1, sizeof *lt, e);
  if (lt == NULL) {
    fpgr_close (fp, e);
    wal_close_and_delete (ww, e);
    return NULL;
  }
  if (lockt_init (lt, mem, e)) {
    fpgr_close (fp, e);
    wal_close_and_delete (ww, e);
    i_free (mem, lt);
    lockt_destroy (lt);
    return NULL;
  }

  page_h        root = page_h_create ();
  struct pager *ret  = NULL;

  if ((ret = i_calloc (mem, 1, sizeof *ret, e)) == NULL) {
    goto failed;
  }

  // Initialize "easy" things
  ret->mem                        = mem;
  ret->fs                         = fs;
  *(struct file_pager **)&ret->fp = fp;
  *(struct wal **)&ret->ww        = ww;
  ret->lt                         = lt;
  atomic_store (&ret->flags, fpgr_isnew (ret->fp));
  atomic_store (&ret->clock, 0);
  ht_init_idx (&ret->pgno_to_value, ret->_hdata, MEMORY_PAGE_LEN);
  latch_init (&ret->htable_lock);
  latch_init (&ret->pgrnew_lock);

  // Open the Dirty page table
  *(struct dpg_table **)&ret->dpt = dpgt_open (mem, e);
  if (ret->dpt == NULL) {
    goto failed;
  }

  // Open the transaction table
  *(struct ns_txn_table **)&ret->tnxt = txnt_open (mem, e);
  if (ret->tnxt == NULL) {
    goto failed;
  }

  // Initialize (but don't start) the checkpoint task
  if (periodic_task_init (&ret->checkpoint_task, e)) {
    goto failed;
  }

  atomic_store (&ret->next_tid, 0);

  if (atomic_load (&ret->flags) & PGR_ISNEW) {
    // Reset any data in the file pager
    if (fpgr_reset (ret->fp, e)) {
      goto failed;
    }

    // Write out the starting header
    memset (&ret->header, 0, sizeof (ret->header));
    if (pgr_write_header (ret, e)) {
      goto failed;
    }

    if (wal_delete_and_reopen (ret->ww, e)) {
      goto failed;
    }

    if (wal_write_start_lsn (ret->ww, 0, e)) {
      goto failed;
    }
  } else {
    if (pgr_read_header (ret, e)) {
      goto failed;
    }

    if (!ret->header.lsn0valid && !ret->header.lsn1valid) {
      error_causef (e, ERR_CORRUPT, "Invalid wal headers");
      goto failed;
    }

    if (wal_isnew (ww)) {
      if (wal_write_start_lsn (ret->ww, MAX (ret->header.lsn0, ret->header.lsn1), e)) {
        goto failed;
      }
    } else {
      lsn start_lsn      = wal_start_lsn (ret->ww);
      lsn next_start_lsn = start_lsn + wal_size (ret->ww);

      if (ret->header.lsn0valid && ret->header.lsn1valid) {
        if (start_lsn == MAX (ret->header.lsn0, ret->header.lsn1)) {
          WRAP_GOTO (pgr_recover (ret, e), failed);
        } else if (start_lsn == MIN (ret->header.lsn0, ret->header.lsn1)) {
          WRAP_GOTO (wal_delete_and_reopen (ret->ww, e), failed);
          WRAP_GOTO (
              wal_write_start_lsn (ret->ww, MAX (ret->header.lsn0, ret->header.lsn1), e),
              failed
          );
        } else {
          error_causef (e, ERR_CORRUPT, "Existing WAL doesn't match database");
          goto failed;
        }
      } else {
        if (ret->header.lsn0valid) {
          if (ret->header.lsn0 != wal_start_lsn (ret->ww)) {
            error_causef (e, ERR_CORRUPT, "Invalid header lsn");
            goto failed;
          }
          WRAP_GOTO (pgr_write_lsn1 (ret, start_lsn, e), failed);
        } else if (ret->header.lsn1valid) {
          if (ret->header.lsn1 != wal_start_lsn (ret->ww)) {
            error_causef (e, ERR_CORRUPT, "Invalid header lsn");
            goto failed;
          }
          WRAP_GOTO (pgr_write_lsn0 (ret, start_lsn, e), failed);
        } else {
          UNREACHABLE (); // LCOV_EXCL_LINE
        }

        WRAP_GOTO (wal_delete_and_reopen (ret->ww, e), failed);
        WRAP_GOTO (wal_write_start_lsn (ret->ww, next_start_lsn, e), failed);
      }
    }
  }

  return ret;

failed:
  ASSERT (error_trace (e));
  if (ret) {
    pgr_cancel_if_exists (&root);
    if (ret->dpt) {
      dpgt_close (ret->dpt);
    }
    if (ret->tnxt) {
      txnt_close (ret->tnxt);
    }
    i_free (mem, ret);
  }

  if (ww) {
    wal_close_and_delete (ww, e);
  }
  if (fp) {
    fpgr_close (fp, e);
  }
  if (lt) {
    lockt_destroy (lt);
  }

  return NULL;
}

#ifdef TESTING
TEST (pager_open)
{
  error e = error_create ();

  TEST_CASE ("green path")
  {
    test_fail_if (pgr_delete_single_file ("testdb", &e));

    struct pager *p = pgr_open ("testdb", mem, fs, &e);

    pgr_close (p, &e);
  }

  TEST_CASE ("dbname is too long")
  {
    char *name = i_malloc (mem, NS_NAME_MAX, 1, &e);
    for (int i = 0; i < NS_NAME_MAX; ++i) {
      name[i] = 'c';
    }
    name[NS_NAME_MAX - 3] = '\0';

    struct pager *p       = pgr_open (name, mem, fs, &e);
    test_assert (p == NULL);
    test_err_t_check (e.cause_code, ERR_INVALID_ARGUMENT, &e);
    e.cause_code          = SUCCESS;

    name[NS_NAME_MAX - 4] = '\0';
    p                     = pgr_open (name, mem, fs, &e);
    test_assert (p != NULL);

    pgr_close (p, &e);

    // Delete the obtuse name
    pgr_delete_single_file (name, &e);

    i_free (mem, name);
  }
}
#endif

#ifdef TESTING
TEST (pgr_open_basic)
{
  error e = error_create ();

  test_fail_if (pgr_delete_single_file ("testdb", &e));

  i_file fp = {0};
  i_open_rw (fs, &fp, "testdb", &e);

  // File is shorter than page size
  test_fail_if (i_truncate (&fp, NS_PAGE_SIZE - 1, &e));
  struct pager *p = pgr_open ("testdb", mem, fs, &e);
  test_assert_int_equal (e.cause_code, ERR_CORRUPT);
  test_assert_equal (p, NULL);
  error_reset (&e);

  // Half a page
  test_fail_if (i_truncate (&fp, NS_PAGE_SIZE / 2, &e));
  p = pgr_open ("testdb", mem, fs, &e);
  test_assert_int_equal (e.cause_code, ERR_CORRUPT);
  test_assert_equal (p, NULL);
  error_reset (&e);

  // 0 pages
  test_fail_if (i_truncate (&fp, 0, &e));
  p = pgr_open ("testdb", mem, fs, &e);
  test_assert_int_equal (e.cause_code, SUCCESS);
  test_assert_int_equal ((int)pgr_get_npages (p), 0);
  test_fail_if (pgr_close (p, &e));

  // Tear down
  test_fail_if (i_close (&fp, &e));
  test_fail_if (pgr_delete_single_file ("testdb", &e));
}
#endif
