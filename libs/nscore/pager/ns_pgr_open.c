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

#include "dpg_table/ns_dirty_page_table.h"
#include "lock_table/ns_lock_table.h"
#include "ns_error.h"
#include "ns_file_pager.h"
#include "ns_logging.h"
#include "ns_utils.h"
#include "os/ns_malloc.h"
#include "os/ns_os.h"
#include "pager/ns_page_fixture.h"
#include "pager/ns_pager.h"
#include "txn_table/ns_txn_table.h"
#include "wal/ns_wal.h"

#ifndef NDEBUG
#  include "ns_testing.h"
#endif

#include <limits.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

err_t
pgr_delete_single_file (const char *dbname, const struct i_os os, error *e)
{
  char fname[NS_PATH_MAX];
  char walname[NS_PATH_MAX];
  snprintf (fname, sizeof fname, "%s", dbname);
  snprintf (walname, sizeof walname, "%s.wal", dbname);

  // Try both even if the first fails. A failure stays in [e] for the return.
  (void)os.table->remove_quiet (os.self, fname, e);
  (void)os.table->remove_quiet (os.self, walname, e);

  return error_trace (e);
}

/******************************************************************************
 * SECTION: pgr_open
 * ----------------------------------------------------------------------------
 * @brief Open a new pager
 ******************************************************************************/

/*
 * pgr_open - standard file-backed entry point.
 *
 * Creates [dbname] if it does not exist and constructs a file_pager and a
 * file-backed WAL for it.
 *
 * NEW DATABASE (file is empty):
 *   Sets PGR_ISNEW so the caller can distinguish new databases from existing.
 *
 * EXISTING DATABASE:
 *   Runs the three-phase ARIES restart via pgr_detect_wal_and_recover().
 */
struct pager *
pgr_open (const char *dbname, struct i_mem mem, struct i_os os, error *e)
{
  // The wal name is the db name plus ".wal", so the longest db name that still
  // fits a path buffer is 5 short of it: 4 for the suffix and 1 for the
  // terminator. pgr_delete_single_file above uses the same buffers.
  // strnlen stops at NS_PATH_MAX, which is already over the limit.
  size_t len = strnlen (dbname, NS_PATH_MAX);
  if (len > (NS_PATH_MAX - 5)) {
    error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "DBName is too big. Supported max: %d actual len: %zu",
        NS_PATH_MAX - 5,
        len
    );
    return NULL;
  }

  char fname[NS_PATH_MAX];
  char walname[NS_PATH_MAX];
  snprintf (fname, sizeof fname, "%s", dbname);
  snprintf (walname, sizeof walname, "%s.wal", dbname);

  struct file_pager *fp = fpgr_open (fname, mem, os, PAGE_HEADER_LEN, e);
  if (fp == NULL) {
    return NULL;
  }

  struct wal *ww = wal_open (walname, mem, os, e);
  // The cleanup calls below run after a failure already in [e], so their own
  // failures add nothing the caller can act on.
  if (ww == NULL) {
    (void)fpgr_close (fp, e);
    return NULL;
  }

  struct lockt *lt = mem.table->malloc (mem.self, 1, sizeof *lt, e);
  if (lt == NULL) {
    (void)fpgr_close (fp, e);
    (void)wal_close_and_delete (ww, e);
    return NULL;
  }
  if (lockt_init (lt, mem, os, e) < 0) {
    (void)fpgr_close (fp, e);
    (void)wal_close_and_delete (ww, e);
    mem.table->free (mem.self, lt);
    return NULL;
  }

  page_h        root = page_h_create ();
  struct pager *ret  = NULL;

  ret                = mem.table->calloc (mem.self, 1, sizeof *ret, e);
  if (ret == NULL) {
    goto failed;
  }

  // Initialize "easy" things
  ret->mem                        = mem;
  ret->os                         = os;
  *(struct file_pager **)&ret->fp = fp;
  *(struct wal **)&ret->ww        = ww;
  ret->lt                         = lt;
  atomic_store (&ret->flags, fpgr_isnew (ret->fp));
  atomic_store (&ret->clock, 0);
  ht_init_idx (&ret->pgno_to_value, ret->_hdata, MEMORY_PAGE_LEN);
  latch_init (&ret->htable_lock);
  latch_init (&ret->pgrnew_lock);

  *(struct dpg_table **)&ret->dpt = dpgt_open (mem, e);
  if (ret->dpt == NULL) {
    goto failed;
  }

  *(struct txn_table **)&ret->tnxt = txnt_open (mem, e);
  if (ret->tnxt == NULL) {
    goto failed;
  }

  // Initialize (but don't start) the checkpoint task
  if (periodic_task_init (&ret->checkpoint_task, os, e) < 0) {
    goto failed;
  }

  atomic_store (&ret->next_tid, 0);

  if (atomic_load (&ret->flags) & PGR_ISNEW) {
    // Reset any data in the file pager
    if (fpgr_reset (ret->fp, e) < 0) {
      goto failed;
    }

    // Write out the starting header
    memset (&ret->header, 0, sizeof ret->header);
    if (pgr_write_header (ret, e) < 0) {
      goto failed;
    }

    if (wal_delete_and_reopen (ret->ww, e) < 0) {
      goto failed;
    }

    if (wal_write_start_lsn (ret->ww, 0, e) < 0) {
      goto failed;
    }
  } else {
    if (pgr_detect_wal_and_recover (ret, e) < 0) {
      goto failed;
    }
  }

  return ret;

failed:
  ASSERT (error_trace (e) < 0);
  if (ret != NULL) {
    pgr_cancel_if_exists (&root);
    if (ret->dpt != NULL) {
      dpgt_close (ret->dpt);
    }
    if (ret->tnxt != NULL) {
      txnt_close (ret->tnxt);
    }
    mem.table->free (mem.self, ret);
  }

  if (ww != NULL) {
    (void)wal_close_and_delete (ww, e);
  }
  if (fp != NULL) {
    (void)fpgr_close (fp, e);
  }
  if (lt != NULL) {
    lockt_destroy (lt);
    mem.table->free (mem.self, lt);
  }

  return NULL;
}

#ifndef NDEBUG
TEST (pager_open)
{
  TEST_CASE ("green path")
  {
    test_assert_int_equal (pgr_delete_single_file ("testdb", ctx->os, &ctx->e), SUCCESS);

    struct pager *p = pgr_open ("testdb", ctx->mem, ctx->os, &ctx->e);
    test_assert (p != NULL);

    test_assert_int_equal (pgr_close (p, &ctx->e), SUCCESS);
  }

  TEST_CASE ("dbname is too long")
  {
    // Over the limit is rejected before anything reaches the filesystem, so
    // this probe can be longer than any filesystem would accept for a name.
    u32   overlong = NS_PATH_MAX - 4;
    char *name     = ctx->mem.table->malloc (ctx->mem.self, overlong + 1, 1, &ctx->e);
    memset (name, 'c', overlong);
    name[overlong]  = '\0';

    struct pager *p = pgr_open (name, ctx->mem, ctx->os, &ctx->e);
    test_assert (p == NULL);
    test_err_t_check (ctx->e.cause_code, ERR_INVALID_ARGUMENT, &ctx->e);
    ctx->e.cause_code = SUCCESS;

    // Under it, but not at it: the limit is a path limit, while this name is a
    // single path component, which filesystems cap at 255 bytes -- and on
    // Windows the cwd plus the name still has to fit MAX_PATH.
    name[100]         = '\0';
    p                 = pgr_open (name, ctx->mem, ctx->os, &ctx->e);
    test_assert (p != NULL);

    pgr_close (p, &ctx->e);

    // Delete the obtuse name
    pgr_delete_single_file (name, ctx->os, &ctx->e);

    ctx->mem.table->free (ctx->mem.self, name);
  }
}

TEST (pgr_open_unwinds_on_allocation_failure)
{
  int  attempts = 0;
  bool opened   = false;
#  define PGR_OPEN_MAX_ATTEMPTS 64

  while (attempts < PGR_OPEN_MAX_ATTEMPTS && !opened) {
    TEST_CASE ("malloc fail_after %d", attempts)
    {
      test_assert_int_equal (pgr_delete_single_file ("testdb", ctx->os, &ctx->e), SUCCESS);

      faulty_mem_reset (ctx->faulty_mem);
      faulty_mem_fail_after (ctx->faulty_mem, attempts);

      struct pager *p = pgr_open ("testdb", ctx->faulty_mem, ctx->os, &ctx->e);

      if (p == NULL) {
        test_assert (ctx->e.cause_code < 0);
        error_reset (&ctx->e);
      } else {
        opened = true;
        test_assert_int_equal (ctx->e.cause_code, SUCCESS);
        // Closing must not be starved by a fail_after that never fired.
        faulty_mem_reset (ctx->faulty_mem);
        test_assert_int_equal (pgr_close (p, &ctx->e), SUCCESS);
      }
    }

    attempts++;
  }

  test_assert (opened);

  test_assert_int_equal (pgr_delete_single_file ("testdb", ctx->os, &ctx->e), SUCCESS);
#  undef PGR_OPEN_MAX_ATTEMPTS
}

static u32
pgr_lsn_checksum (lsn l)
{
  u32 csm = checksum_init ();
  checksum_execute (&csm, (void *)&l, sizeof l);
  return csm;
}

// Overwrites the header of testdb, leaving its pages alone. The header is four
// fields, each lsn next to its checksum:
//
//   [ lsn0 | csm0 | lsn1 | csm1 ]
//     ^0     ^8     ^12    ^20
//
// An lsn is trusted only when its checksum matches, so [lsn0valid] and
// [lsn1valid] choose between the real checksum and a deliberately wrong one.
static void
pgr_write_raw_header (struct i_os os, lsn lsn0, bool lsn0valid, lsn lsn1, bool lsn1valid, error *e)
{
  const u32 bad_csm = 0xDEADBEEFU;

  u32       csm0    = lsn0valid ? pgr_lsn_checksum (lsn0) : bad_csm;
  u32       csm1    = lsn1valid ? pgr_lsn_checksum (lsn1) : bad_csm;

  u8        hdr[PAGE_HEADER_LEN];
  memcpy (hdr + LSN0_OFST, &lsn0, sizeof lsn0);
  memcpy (hdr + LSN0_CSM_OFST, &csm0, sizeof csm0);
  memcpy (hdr + LSN1_OFST, &lsn1, sizeof lsn1);
  memcpy (hdr + LSN1_CSM_OFST, &csm1, sizeof csm1);

  i_file f = {0};
  if (os.table->open_file_rw (os.self, &f, "testdb", e) < 0) {
    return;
  }
  f.table->pwrite_all (f.self, hdr, sizeof hdr, 0, e);
  os.table->close_file (os.self, f.self, e);
}

// The seeded wal below starts at lsn 0, so a trustworthy header lsn agrees
// only when it is 0 too. The last row is the control: that one must open.
static struct
{
  const char *name;
  lsn         lsn0;
  bool        lsn0valid;
  lsn         lsn1;
  bool        lsn1valid;
  bool        opens;
} pgr_header_cases[] = {
    {"neither lsn is trustworthy", 7, false, 9, false, false},
    {"lone valid lsn ahead of the wal", 1234, true, 9, false, false},
    {"both valid, wal matches neither", 1000, true, 2000, true, false},
    {"lone valid lsn matching the wal", 0, true, 9, false, true},
};

TEST (pgr_open_rejects_a_corrupt_header)
{
  // Leaves testdb on disk with one committed page and a live wal. An empty db
  // is exactly PAGE_HEADER_LEN long, which fpgr_open flags FP_ISNEW, and
  // pgr_close deletes the wal -- so a page is committed and the seed crashes
  // rather than closing.
#  define SEED_EXISTING_DB()                                                                \
    do {                                                                                    \
      test_assert_int_equal (pgr_delete_single_file ("testdb", ctx->os, &ctx->e), SUCCESS); \
      struct pager *_seed = pgr_open ("testdb", ctx->mem, ctx->os, &ctx->e);                \
      test_assert (_seed != NULL);                                                          \
      struct txn _tx;                                                                       \
      test_assert_int_equal (pgr_begin_txn (&_tx, _seed, &ctx->e), SUCCESS);                \
      page_h _h = page_h_create ();                                                         \
      test_assert_int_equal (pgr_new (&_h, _seed, &_tx, PG_DATA_LIST, &ctx->e), SUCCESS);   \
      dl_set_used (page_h_w (&_h), DL_DATA_SIZE);                                           \
      test_assert_int_equal (pgr_release (_seed, &_h, PG_DATA_LIST, &ctx->e), SUCCESS);     \
      test_assert_int_equal (pgr_commit (_seed, &_tx, &ctx->e), SUCCESS);                   \
      test_assert_int_equal (pgr_crash (_seed, &ctx->e), SUCCESS);                          \
    }                                                                                       \
    while (0)

  for (u32 i = 0; i < arrlen (pgr_header_cases); ++i) {
    TEST_CASE ("%s", pgr_header_cases[i].name)
    {
      SEED_EXISTING_DB ();

      pgr_write_raw_header (
          ctx->os,
          pgr_header_cases[i].lsn0,
          pgr_header_cases[i].lsn0valid,
          pgr_header_cases[i].lsn1,
          pgr_header_cases[i].lsn1valid,
          &ctx->e
      );
      test_assert_int_equal (ctx->e.cause_code, SUCCESS);

      struct pager *p = pgr_open ("testdb", ctx->mem, ctx->os, &ctx->e);

      if (pgr_header_cases[i].opens) {
        test_assert (p != NULL);
        test_assert_int_equal (ctx->e.cause_code, SUCCESS);
        test_assert_int_equal (pgr_close (p, &ctx->e), SUCCESS);
      } else {
        test_assert (p == NULL);
        test_err_t_check (ctx->e.cause_code, ERR_CORRUPT, &ctx->e);
      }
    }
  }

#  undef SEED_EXISTING_DB

  test_assert_int_equal (pgr_delete_single_file ("testdb", ctx->os, &ctx->e), SUCCESS);
}
#endif

#ifndef NDEBUG
TEST (pgr_open_basic)
{
  test_assert_int_equal (pgr_delete_single_file ("testdb", ctx->os, &ctx->e), SUCCESS);

  i_file fp = {0};
  ctx->os.table->open_file_rw (ctx->os.self, &fp, "testdb", &ctx->e);

  // File is shorter than page size
  test_assert_int_equal (fp.table->truncate (fp.self, NS_PAGE_SIZE - 1, &ctx->e), SUCCESS);
  struct pager *p = pgr_open ("testdb", ctx->mem, ctx->os, &ctx->e);
  test_assert_int_equal (ctx->e.cause_code, ERR_CORRUPT);
  test_assert (p == NULL);
  error_reset (&ctx->e);

  // Half a page
  test_assert_int_equal (fp.table->truncate (fp.self, NS_PAGE_SIZE / 2, &ctx->e), SUCCESS);
  p = pgr_open ("testdb", ctx->mem, ctx->os, &ctx->e);
  test_assert_int_equal (ctx->e.cause_code, ERR_CORRUPT);
  test_assert (p == NULL);
  error_reset (&ctx->e);

  // 0 pages
  test_assert_int_equal (fp.table->truncate (fp.self, 0, &ctx->e), SUCCESS);
  p = pgr_open ("testdb", ctx->mem, ctx->os, &ctx->e);
  test_assert_int_equal (ctx->e.cause_code, SUCCESS);
  test_assert_type_equal (pgr_get_npages (p), 0U, p_size, PRp_size);
  test_assert_int_equal (pgr_close (p, &ctx->e), SUCCESS);

  // Tear down
  test_assert_int_equal (ctx->os.table->close_file (ctx->os.self, fp.self, &ctx->e), SUCCESS);
  test_assert_int_equal (pgr_delete_single_file ("testdb", ctx->os, &ctx->e), SUCCESS);
}
#endif
