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

#pragma once

#include "c_specx/concurrency/periodic_task.h"
#include "errors.h"
#include "lockt/lock_table.h"
#include "os_pager/os_pager.h"
#include "pager/page_h.h"
#include "txns/txn.h"
#include "wal/os_wal.h"
#include "wal/wal_rec_hdr.h"

/**
 * Database structure:
 *
 * Every FS_BTMP_NPGS pages starts with a free space
 * map, which tracks it's section's free pages. It tracks
 * pages [self, self + FS_BTMP_NPGS)
 *
 * 0                         - FSM [0, FS_BTMP_NPGS)
 * 1                         - PAGE
 * 2                         - PAGE
 * 3                         - PAGE
 * 4                         - PAGE
 *
 * ...
 *
 * FS_BTMP_NPGS              - FSM [FS_BTMP_NPGS, 2 * FS_BTMP_NPGS)
 * FS_BTMP_NPGS + 1          - PAGE
 * FS_BTMP_NPGS + 2          - PAGE
 *
 * ...
 *
 * 2 * FS_BTMP_NPGS          - FSM [2 * FS_BTMP_NPGS, 3 * FS_BTMP_NPGS)
 * 2 * FS_BTMP_NPGS + 1      - PAGE
 * 2 * FS_BTMP_NPGS + 2      - PAGE
 *
 * ...
 */

// Special Page Numbers
#define VHASH_PGNO ((pgno)1) // Free space map start

/**
 * Flags to help the buffer manager know
 * which pages are occupied, accessed or writable
 */
enum
{
  PW_ACCESS = 1u << 0, // meaningless for X pages
  PW_DIRTY = 1u << 1,  // meaningless for X pages
  PW_PRESENT = 1u << 2,
  PW_X = 1u << 3,
};

/**
 * Pager property flags
 */
enum
{
  PGR_ISNEW = 1u << 0,        // This is a pager on a new database file
  PGR_ISRESTARTING = 1u << 1, // Pager is currently restarting
};

// Robin hood hash table for buffer pool
#define KTYPE pgno
#define VTYPE u32
#define SUFFIX idx
#include "c_specx/ds/robin_hood_ht.h"
#undef KTYPE
#undef VTYPE
#undef SUFFIX

struct pager
{
  // Resources
  struct os_pager *fp; // OS pager abstraction (e.g. file_pager)
  struct os_wal *ww;   // Write-ahead log abstraction
  struct lockt *lt;    // Lock table

  bool iown_fp;
  bool iown_ww;
  bool iown_lt;

  _Atomic int flags;
  _Atomic u32 clock;

  /**
   * A hash table of pgno -> index within the buffer pool
   *
   * It's a static hash table because you can never
   * have more pages in this hash table than there are
   * pages in the buffer pool
   */
  hash_table_idx pgno_to_value;
  hentry_idx _hdata[MEMORY_PAGE_LEN];

  latch l;

  struct dpg_table *dpt;
  struct txn_table *tnxt;
  struct periodic_task checkpoint_task;
  _Atomic txid next_tid;

  /**
   * The actual buffer pool
   *
   * This is very large and takes up the most of the space
   * of pager. Pager should never be stack allocated because of
   * the buffer pool
   */
  struct page_frame pages[MEMORY_PAGE_LEN];
};

DEFINE_DBG_ASSERT (struct pager, pager, p, {
  ASSERT (p);

  latch_lock ((latch *)&p->l);
  ASSERT (p->fp);
  ASSERT (p->ww);
  ASSERT (p->lt);
  ASSERT (p->dpt);
  ASSERT (p->tnxt);
  ASSERT (atomic_load (&p->next_tid) > 0);
  latch_unlock ((latch *)&p->l);
})

////////////////////////////////////////////////////////////
/// Specializations

struct pager *pgr_open_single_file (const char *dbname, error *e);
err_t pgr_delete_single_file (const char *dbname, error *e);

////////////////////////////////////////////////////////////
/// Lifecycle

struct pager *pgr_open (struct os_pager *fp, struct os_wal *ww, struct lockt *lt, error *e);
err_t pgr_close (struct pager *p, error *e);
void pgr_attach_lock_table (struct pager *p, struct lockt *lt);
err_t pgr_crash (struct pager *p, error *e);

////////////////////////////////////////////////////////////
/// Utils

p_size pgr_get_npages (struct pager *p);
bool pgr_isnew (const struct pager *p);
void i_log_page_table (int log_level, bool only_present, struct pager *p);

////////////////////////////////////////////////////////////
/// Transaction Control

err_t pgr_begin_txn (struct txn *tx, struct pager *p, error *e);
err_t pgr_commit (struct pager *p, struct txn *tx, error *e);
err_t pgr_rollback (struct pager *p, struct txn *tx, lsn save_lsn, error *e);
err_t pgr_flush_wall (const struct pager *p, error *e);

////////////////////////////////////////////////////////////
/// Inner Utils

err_t pgr_refresh_wal (struct pager *p, error *e);
void pgr_unfix (struct pager *p, page_h *h, int flags);
i32 pgr_reserve_and_ctrl_lock (struct pager *p, error *e);
err_t pgr_evict_unsafe (struct pager *p, struct page_frame *mp, error *e);
err_t pgr_flush_unsafe (const struct pager *p, struct page_frame *mp, error *e);
err_t pgr_extend_file (const struct pager *p, pgno npages, struct txn *tx, error *e);

////////////////////////////////////////////////////////////
/// Checkpoints

err_t pgr_deletion_blocking_checkpoint (struct pager *p, error *e);
err_t pgr_launch_checkpoint_thread (struct pager *p, u64 msec, error *e);

////////////////////////////////////////////////////////////
/// Primary API

err_t pgr_get (page_h *dest, int flags, pgno pgno, struct pager *p, error *e);
err_t pgr_new (page_h *dest, struct pager *p, struct txn *tx, enum page_type ptype, error *e);
err_t pgr_make_writable (struct pager *p, struct txn *tx, page_h *h, error *e);
err_t pgr_delete_and_release (struct pager *p, struct txn *tx, page_h *h, error *e);
err_t pgr_release_with_log (struct pager *p, page_h *h, int flags, struct wal_update_write *record, error *e);
void pgr_cancel (const struct pager *p, page_h *h);

////////////////////////////////////////////////////////////
/// Short Hands

HEADER_FUNC err_t
pgr_get_writable (
    page_h *dest,
    struct txn *tx,
    const int flags,
    const pgno pg,
    struct pager *p,
    error *e)
{
  if (pgr_get (dest, flags, pg, p, e))
    {
      return error_trace (e);
    }

  if (pgr_make_writable (p, tx, dest, e))
    {
      pgr_cancel (p, dest);
    }

  return error_trace (e);
}

HEADER_FUNC err_t
pgr_maybe_make_writable (
    struct pager *p,
    struct txn *tx,
    page_h *cur,
    error *e)
{
  if (cur->mode == PHM_S)
    {
      return pgr_make_writable (p, tx, cur, e);
    }
  return SUCCESS;
}

HEADER_FUNC err_t
pgr_release (struct pager *p, page_h *h, const int flags, error *e)
{
  return pgr_release_with_log (p, h, flags, NULL, e);
}

HEADER_FUNC err_t
pgr_release_if_exists (struct pager *p, page_h *h, int flags, error *e)
{
  if (h->mode != PHM_NONE)
    {
      return pgr_release (p, h, flags, e);
    }
  return SUCCESS;
}

HEADER_FUNC err_t
pgr_release_with_flush (struct pager *p, page_h *h, const int flags, error *e)
{
  struct page_frame *pgr = h->pgr;

  if (pgr_release (p, h, flags, e))
    {
      return e->cause_code;
    }
  if (pgr_flush_unsafe (p, pgr, e))
    {
      return e->cause_code;
    }
  return SUCCESS;
}

HEADER_FUNC err_t
pgr_release_with_evict (struct pager *p, page_h *h, const int flags, error *e)
{
  struct page_frame *pgr = h->pgr;

  if (pgr_release (p, h, flags, e))
    {
      return e->cause_code;
    }
  if (pgr_evict_unsafe (p, pgr, e))
    {
      return e->cause_code;
    }
  return SUCCESS;
}

HEADER_FUNC err_t
pgr_flush_all_pages (struct pager *p, error *e)
{
  for (u32 i = 0; i < MEMORY_PAGE_LEN; ++i)
    {
      struct page_frame *mp = &p->pages[i];

      // A page is Flushable if:
      // 1. Present
      // 2. Not in X mode - meaning the page is read only
      //        - in fuzzy checkpoints, a page owned in X mode
      //          is being written to and it's updates happen after it's
      //          saved, so it's modification will be saved to the WAL, (or not
      //          committed and therefore cleaned up by ARIES on next open).
      //          So there's no need to flush currently X mode pages
      //          You just need to ensure that the page is flushed to the
      //          WAL before it's flushed to the database

      latch_lock (&mp->ctrl);

      if (mp->flags & PW_PRESENT && !(mp->flags & PW_X))
        {
          pgr_flush_unsafe (p, mp, e); // Ignore error
        }

      latch_unlock (&mp->ctrl);
    }

  return error_trace (e);
}

HEADER_FUNC err_t
pgr_evict_all_pages (struct pager *p, error *e)
{
  for (u32 i = 0; i < MEMORY_PAGE_LEN; ++i)
    {
      struct page_frame *mp = &p->pages[i];

      latch_lock (&mp->ctrl);

      if (mp->flags & PW_PRESENT)
        {
          ASSERT (!(mp->flags & PW_X)); // All pages should be in a read state
          pgr_evict_unsafe (p, mp, e);  // Ignore error
        }

      latch_unlock (&mp->ctrl);
    }

  return error_trace (e);
}

HEADER_FUNC void
pgr_cancel_if_exists (struct pager *p, page_h *h)
{
  if (h->mode == PHM_NONE)
    {
      return;
    }

  pgr_cancel (p, h);
}
