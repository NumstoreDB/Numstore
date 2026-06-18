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

#ifndef PAGER_H
#define PAGER_H

#include "compile_config.h"   // lsn ...etc
#include "concurrency.h"      // latch / lock table / periodic_task
#include "dirty_page_table.h" // dpgt_table
#include "error.h"            // error
#include "file_pager.h"       // file_pager
#include "page_h.h"           // page_header
#include "stdtypes.h"         // u8 ...etc
#include "txn_table.h"        // txn_table
#include "wal.h"              // wal

/******************************************************************************
 * SECTION: Database structure
 * ----------------------------------------------------------------------------
 *
 * @brief The database is seperated into fixed sized "pages"
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
 ******************************************************************************/

/**
 * @def The page number of the variable hash page
 * @brief The variable hash page is always located at the same place
 *
 * It starts right after the first free space map
 */
#define VHASH_PGNO ((pgno)1)

/**
 * @brief Flags to help the buffer manager know which pages are occupied,
 * accessed or writable
 *
 * @var ::PW_ACCESS
 * @brief The Access bit - set to 1 anytime the pager "accesses" it e.g. invokes
 * an operation that touches this page outside of allocation.
 * Meaningfless for X type pages
 *
 * @var ::PW_PRESENT
 * @brief The present bit - Set to 1 if the page is present - This is the first
 * bit to check when reserving pages
 *
 * @var ::PW_X
 * @brief Exclusive bit - locked in write mode - only one thread is accessing
 * this page at a time
 */
enum
{
  PW_ACCESS  = 1u << 0,
  PW_PRESENT = 1u << 2,
  PW_X       = 1u << 3,
};

/**
 * @brief Global pager property flags
 *
 * @var ::PGR_ISNEW
 * @brief Set to true if this is the first time the pager was created
 *
 * @var ::PGR_ISRESTARTING
 * @brief Set to true if the pager is in the recover stages
 */
enum
{
  PGR_ISNEW        = 1u << 0,
  PGR_ISRESTARTING = 1u << 1,
};

/*-----------------------------------------------------------------------------
 * SUBSECTION: Page Header
 * @brief First few bytes of a database file
 *
 * The first bytes of a database file include the header. The header
 * contains two lsns - start and end lsn. These are the lsn's that the
 * WAL starts at. The bigger one is usually the source of truth. There is
 * two of them to ensure that the WAL is deleted durably.
 *
 * 1. FLUSH PAGES: Flush all dirty database pages to disk to guarantee data is
 * safe.
 *
 * 2. CHOOSE SLOT: Get the new start LSN and identify the database header slot
 * with the minimum LSN value (the low slot).
 *
 * 3. WRITE & SYNC: Write the new LSN into that low slot and `fsync` the header.
 * (The old high slot remains intact as a rollback fallback if we crash here).
 *
 * 4. ATOMIC DELETE: Delete the WAL file from disk.
 * (At this moment, the newly written LSN becomes the highest on disk and the
 * new truth).
 *
 * 5. REOPEN WAL: Create and initialize a new empty WAL file matching the new
 * LSN.
 *----------------------------------------------------------------------------*/

#define PAGE_HEADER_LEN                   \
  sizeof (u32) +     /* checksum(lsn0) */ \
      sizeof (lsn) + /* lsn0 */           \
      sizeof (u32) + /* checksum(lsn1) */ \
      sizeof (lsn)   /* lsn1 */

struct pager_header
{
  lsn  lsn0;
  u32  lsn0csm;
  bool lsn0valid;
  lsn  lsn1;
  u32  lsn1csm;
  bool lsn1valid;
};

#define LSN0_OFST     0
#define LSN0_CSM_OFST (LSN0_OFST + sizeof (lsn))
#define LSN1_OFST     (LSN0_CSM_OFST + sizeof (u32))
#define LSN1_CSM_OFST (LSN1_OFST + sizeof (lsn))

/*-----------------------------------------------------------------------------
 * SUBSECTION: Pager Data
 * @brief The pager struct and data
 *----------------------------------------------------------------------------*/

// Robin hood hash table for buffer pool
#define KTYPE  pgno
#define VTYPE  u32
#define SUFFIX idx
#include "robin_hood_ht.h"
#undef KTYPE
#undef VTYPE
#undef SUFFIX

/**
 * @struct pager
 *
 * @brief The system responsible for reading and writing pages durably.
 *
 * Manages the in-memory buffer pool, interfaces with the underlying file
 * system and write-ahead log (WAL), handles concurrency/latching, and
 * coordinates the transaction and dirty page tables for ARIES recovery.
 *
 * @var pager::header
 * @brief Structured, parsed representation of the database file header.
 *
 * @var pager::_header
 * @brief Raw byte buffer containing the serialized representation of the page
 * header.
 *
 * @var pager::fp
 * @brief The file pager reads and writes pages from the actual file.
 *
 * @var pager::ww
 * @brief The write-ahead log (WAL) abstraction for durability and crash
 * recovery.
 *
 * @var pager::lt
 * @brief Pointer to the system-wide lock table for concurrency control.
 *
 * @var pager::flags
 * @brief Atomic bit-flags denoting internal runtime state (e.g., closing,
 * dirty).
 *
 * @var pager::clock
 * @brief Atomic logical clock counter used for page eviction policies (e.g.,
 * Clock Sweep).
 *
 * @var pager::pgrnew_lock
 * @brief Latch used to synchronize internal pager operations when creating new
 * pages.
 *
 * @var pager::pgno_to_value
 * @brief A static hash table mapping a page number (`pgno`) to its buffer pool
 * index.
 *
 * @var pager::_hdata
 * @brief Backing memory array for the `pgno_to_value` static hash table.
 * @var pager::htable_lock
 *
 * @brief Synchronizes multi-threaded access and modifications to the hash
 * table.
 *
 * @var pager::dpt
 * @brief Dirty Page Table tracker containing the `page_lsn` references needed
 * for ARIES.
 *
 * @var pager::tnxt
 * @brief Transaction Table tracking active transactions and their last LSN
 * status.
 *
 * @var pager::checkpoint_task
 * @brief Periodic background engine state capturing checkpoints to safely
 * truncate/delete the WAL.
 *
 * @var pager::next_tid
 * @brief Monotonically increasing atomic tracker for issuing the next
 * Transaction ID.
 *
 * @var pager::pages
 * @brief The actual large in-memory buffer pool frame array.
 *
 * @note This is very large; the pager structure should never be allocated on
 * the thread stack.
 */
struct pager
{
  struct pager_header header;
  u8                  _header[PAGE_HEADER_LEN];

  // Resources / Systems
  struct file_pager *const fp;
  struct wal *const        ww;
  struct lockt            *lt;
  struct dpg_table *const  dpt;
  struct txn_table *const  tnxt;

  // Flags and concurrency
  _Atomic int  flags;
  _Atomic u32  clock;
  _Atomic txid next_tid;

  // Properties
  latch                pgrnew_lock;
  struct periodic_task checkpoint_task;

  // Data
  hash_table_idx    pgno_to_value;
  hentry_idx        _hdata[MEMORY_PAGE_LEN];
  latch             htable_lock;
  struct page_frame pages[MEMORY_PAGE_LEN];
};

DEFINE_DBG_ASSERT (struct pager, pager, p, {
  ASSERT (p);
  ASSERT (p->fp);
  ASSERT (p->ww);
  ASSERT (p->lt);
  ASSERT (p->dpt);
  ASSERT (p->tnxt);
})

/*-----------------------------------------------------------------------------
 * SUBSECTION: Specializations
 * @brief Initialization and destruction of single-file database engines
 *----------------------------------------------------------------------------*/

struct pager *pgr_open_single_file (const char *dbname, error *e);
err_t         pgr_delete_single_file (const char *dbname, error *e);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Lifecycle
 * @brief Management of active pager context resources and shutdown states
 *----------------------------------------------------------------------------*/

err_t pgr_close (struct pager *p, error *e);
void  pgr_attach_lock_table (struct pager *p, struct lockt *lt);
err_t pgr_crash (struct pager *p, error *e);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Utils
 * @brief Inspection utilities and low-level header serialization helpers
 *----------------------------------------------------------------------------*/

p_size pgr_get_npages (struct pager *p);
bool   pgr_isnew (const struct pager *p);
void   i_log_page_table (int log_level, bool only_present, struct pager *p);
err_t  pgr_read_header (struct pager *p, error *e);
err_t  pgr_write_header (struct pager *p, error *e);
err_t  pgr_write_next_lsn (struct pager *p, lsn lsn, error *e);
err_t  pgr_write_lsn0 (struct pager *p, lsn lsn0, error *e);
err_t  pgr_write_lsn1 (struct pager *p, lsn lsn1, error *e);

err_t pgr_recover (struct pager *p, error *e);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Transaction Control
 * @brief Boundary boundaries and log flushing mechanics for transactions
 *----------------------------------------------------------------------------*/

err_t pgr_begin_txn (struct txn *tx, struct pager *p, error *e);
err_t pgr_commit (struct pager *p, struct txn *tx, error *e);
err_t pgr_rollback (struct pager *p, struct txn *tx, lsn save_lsn, error *e);
err_t pgr_flush_wall (struct pager *p, error *e);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Inner Utils
 * @brief Internal structural maintenance, unsafe flushes, and file sizing
 *----------------------------------------------------------------------------*/

err_t pgr_refresh_wal (struct pager *p, error *e);
void  pgr_unfix (struct pager *p, page_h *h, int flags);
i32   pgr_reserve_and_ctrl_lock (struct pager *p, error *e);
err_t pgr_evict_unsafe (struct pager *p, struct page_frame *mp, error *e);
err_t pgr_flush_unsafe (const struct pager *p, struct page_frame *mp, error *e);
err_t
pgr_extend_file (const struct pager *p, pgno npages, struct txn *tx, error *e);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Checkpoints
 * @brief Synchronization barriers to flush dirty state and truncate the WAL
 *----------------------------------------------------------------------------*/

err_t pgr_deletion_blocking_checkpoint (struct pager *p, error *e);
err_t pgr_launch_checkpoint_thread (struct pager *p, u64 msec, error *e);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Primary API
 * @brief Main engine interfaces for page lookup, reservation, and lifecycle
 *----------------------------------------------------------------------------*/

err_t pgr_get (page_h *dest, int flags, pgno pgno, struct pager *p, error *e);
err_t pgr_get_writable (
    page_h       *dest,
    struct txn   *tx,
    int           flags,
    pgno          pg,
    struct pager *p,
    error        *e
);
err_t pgr_new (
    page_h        *dest,
    struct pager  *p,
    struct txn    *tx,
    enum page_type ptype,
    error         *e
);
err_t
pgr_delete_and_release (struct pager *p, struct txn *tx, page_h *h, error *e);
err_t pgr_release_with_log (
    struct pager            *p,
    page_h                  *h,
    int                      flags,
    struct wal_update_write *record,
    error                   *e
);
void pgr_cancel (const struct pager *p, page_h *h);

/*-----------------------------------------------------------------------------
 * SUBSECTION: ARIES Recovery
 * @brief Classic ARIES Recovery algorithms
 *----------------------------------------------------------------------------*/

struct aries_ctx
{
  /**
   * At the end of the analysis phase,
   * this is the minimum recovery lsn
   *
   * It's the minimum page we need to read first in
   * the restart phase on recovery
   */
  lsn redo_lsn;

  /**
   * We keep track of the maximum transaction id that
   * we see in the database in order to pick up where we left
   * off
   */
  txid max_tid;

  /**
   * These are the reconstruction of the active
   * transaction table and the dirty page table
   * while we run recovery.
   *
   * They are ephemral and will be destroyed at the
   * end of recovery. Then the pager will create
   * them again because we're in a clean state
   */
  struct txn_table *txt;
  struct dpg_table *dpt;

  /**
   * While we scan through the log, we'll
   * be adding transactions to the transaction table
   * and we need a place to allocate / put those transactions
   * (normally we do it on the stack)
   */
  struct dbl_buffer txn_ptrs;
  struct slab_alloc alloc;
};

err_t       aries_ctx_create (struct aries_ctx *dest, error *e);
void        aries_ctx_free (struct aries_ctx *ctx);
struct txn *aries_ctx_txn_alloc (struct aries_ctx *ctx, error *e);

err_t pgr_restart (struct pager *p, struct aries_ctx *ctx, error *e);
err_t pgr_restart_analysis (struct pager *p, struct aries_ctx *ctx, error *e);
err_t pgr_restart_redo (struct pager *p, struct aries_ctx *ctx, error *e);
err_t pgr_restart_undo (struct pager *p, struct aries_ctx *ctx, error *e);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Short Hands
 * @brief Inline wrappers combining pin, unpin, flush, and eviction pipelines
 *----------------------------------------------------------------------------*/

HEADER_FUNC err_t
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
  if (!writable)
  {
    return pgr_get (dest, flags, pg, p, e);
  }
  else
  {
    return pgr_get_writable (dest, tx, flags, pg, p, e);
  }
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
    return error_trace (e);
  }
  if (pgr_flush_unsafe (p, pgr, e))
  {
    return error_trace (e);
  }
  return SUCCESS;
}

HEADER_FUNC err_t
pgr_release_with_evict (struct pager *p, page_h *h, const int flags, error *e)
{
  struct page_frame *pgr = h->pgr;

  if (pgr_release (p, h, flags, e))
  {
    return error_trace (e);
  }
  if (pgr_evict_unsafe (p, pgr, e))
  {
    return error_trace (e);
  }
  return SUCCESS;
}

HEADER_FUNC err_t
pgr_flush_all_pages (struct pager *p, error *e)
{
  for (u32 i = 0; i < MEMORY_PAGE_LEN; ++i)
  {
    struct page_frame *mp = &p->pages[i];

    latch_lock (&mp->ctrl);

    if (mp->flags & PW_PRESENT && !(mp->flags & PW_X))
    {
      ASSERT (!(mp->flags & PW_X));
      pgr_flush_unsafe (p, mp, e);
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
      ASSERT (!(mp->flags & PW_X));
      pgr_evict_unsafe (p, mp, e);
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

HEADER_FUNC err_t
pgr_upgrade (page_h *_pg, struct txn *tx, int flags, struct pager *p, error *e)
{
  pgno pg = page_h_pgno (_pg);
  pgr_release (p, _pg, flags, e);
  return pgr_get_writable (_pg, tx, flags, pg, p, e);
}

#endif // PAGER_H
