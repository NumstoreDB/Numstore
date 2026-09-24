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

#include "core/ns_arena_alloc.h"
#include "core/ns_concurrency.h"
#include "core/ns_csx_assert.h"
#include "core/ns_dbl_buffer.h"
#include "core/ns_error.h"
#include "core/ns_slab_alloc.h"
#include "core/ns_stdtypes.h"
#include "core/os/ns_filesystem.h"
#include "nscore/disk_pager/ns_file_pager.h"
#include "nscore/dpg_table/ns_dirty_page_table.h"
#include "nscore/page/ns_page.h"
#include "nscore/page/ns_page_h.h"
#include "nscore/txn_table/ns_txn_table.h"
#include "nscore/wal/ns_wal.h"

#include <stdbool.h>
#include <stddef.h>

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

#define VHASH_PGNO ((pgno)1)

enum
{
  PW_ACCESS  = 1U << 0,
  PW_PRESENT = 1U << 2,
  PW_X       = 1U << 3,
};

enum
{
  PGR_ISNEW        = 1U << 0,
  PGR_ISRESTARTING = 1U << 1,
};

// Header
#define PAGE_HEADER_LEN                \
  (sizeof (u32) + /* checksum(lsn0) */ \
   sizeof (lsn) + /* lsn0 */           \
   sizeof (u32) + /* checksum(lsn1) */ \
   sizeof (lsn))  /* lsn1 */

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

// Robin hood hash table for buffer pool
#define KTYPE  pgno
#define VTYPE  u32
#define SUFFIX idx
#include "core/ns_robin_hood_ht.h"
#undef KTYPE
#undef VTYPE
#undef SUFFIX

struct pager
{
  struct i_mem             mem;
  struct i_file_system     fs;

  struct pager_header      header;
  u8                       _header[PAGE_HEADER_LEN];

  // Resources / Systems
  struct file_pager *const fp;
  struct wal *const        ww;
  struct lockt            *lt;
  struct dpg_table *const  dpt;
  struct txn_table *const  tnxt;

  // Flags and concurrency
  _Atomic int              flags;
  _Atomic u32              clock;
  _Atomic txid             next_tid;

  // Properties
  latch                    pgrnew_lock;
  struct periodic_task     checkpoint_task;

  // Data
  hash_table_idx           pgno_to_value;
  hentry_idx               _hdata[MEMORY_PAGE_LEN];
  latch                    htable_lock;
  struct page_frame        pages[MEMORY_PAGE_LEN];
};

DEFINE_DBG_ASSERT (struct pager, pager, p, {
  ASSERT (p);
  ASSERT (p->fp);
  ASSERT (p->ww);
  ASSERT (p->lt);
  ASSERT (p->dpt);
  ASSERT (p->tnxt);
})

struct pager *pgr_open (const char *dbname, struct i_mem mem, struct i_file_system fs, error *e);
err_t pgr_delete_single_file (const char *dbname, error *e);
err_t pgr_close (struct pager *p, error *e);
err_t pgr_crash (struct pager *p, error *e);

err_t pgr_begin_txn (struct txn *tx, struct pager *p, error *e);
err_t pgr_commit (struct pager *p, struct txn *tx, error *e);
err_t pgr_rollback (struct pager *p, struct txn *tx, lsn save_lsn, error *e);

err_t pgr_get (page_h *dest, int flags, pgno pgno, struct pager *p, error *e);
err_t pgr_get_writable (
    page_h       *dest,
    struct txn   *tx,
    int           flags,
    pgno          pg,
    struct pager *p,
    error        *e
);
err_t pgr_get_maybe_writable (
    page_h       *dest,
    struct txn   *tx,
    int           flags,
    pgno          pg,
    struct pager *p,
    bool          writable,
    error        *e
);

err_t pgr_new (page_h *dest, struct pager *p, struct txn *tx, enum page_type ptype, error *e);
err_t pgr_delete_and_release (struct pager *p, struct txn *tx, page_h *h, error *e);
err_t pgr_release_with_log (
    struct pager            *p,
    page_h                  *h,
    int                      flags,
    struct wal_update_write *record,
    error                   *e
);
err_t pgr_release (struct pager *p, page_h *h, const int flags, error *e);
err_t pgr_release_if_exists (struct pager *p, page_h *h, int flags, error *e);
err_t pgr_release_with_flush (struct pager *p, page_h *h, const int flags, error *e);
err_t pgr_release_with_evict (struct pager *p, page_h *h, const int flags, error *e);

void pgr_unfix (page_h *h, int flags);
i32 pgr_reserve_and_ctrl_lock (struct pager *p, error *e);

err_t pgr_evict_unsafe (struct pager *p, struct page_frame *mp, error *e);
err_t pgr_evict_all_pages (struct pager *p, error *e);

err_t pgr_flush_unsafe (const struct pager *p, struct page_frame *mp, error *e);
err_t pgr_flush_all_pages (struct pager *p, error *e);

err_t pgr_upgrade (page_h *_pg, struct txn *tx, int flags, struct pager *p, error *e);
err_t pgr_launch_checkpoint_thread (struct pager *p, u64 msec, error *e);

void pgr_cancel (page_h *h);
void pgr_cancel_if_exists (page_h *h);

p_size pgr_get_npages (struct pager *p);
bool pgr_isnew (const struct pager *p);
void i_log_page_table (int log_level, bool only_present, struct pager *p);

// Header writing
err_t pgr_write_lsn0 (struct pager *p, lsn lsn0, error *e);
err_t pgr_write_lsn1 (struct pager *p, lsn lsn1, error *e);
err_t pgr_write_next_lsn (struct pager *p, lsn l, error *e);
err_t pgr_write_header (struct pager *p, error *e);

struct aries_ctx
{
  /**
   * At the end of the analysis phase,
   * this is the minimum recovery lsn
   *
   * It's the minimum page we need to read first in
   * the restart phase on recovery
   */
  lsn                redo_lsn;

  /**
   * We keep track of the maximum transaction id that
   * we see in the database in order to pick up where we left
   * off
   */
  txid               max_tid;

  /**
   * These are the reconstruction of the active
   * transaction table and the dirty page table
   * while we run recovery.
   *
   * They are ephemral and will be destroyed at the
   * end of recovery. Then the pager will create
   * them again because we're in a clean state
   */
  struct txn_table  *txt;
  struct dpg_table  *dpt;

  /**
   * While we scan through the log, we'll
   * be adding transactions to the transaction table
   * and we need a place to allocate / put those transactions
   * (normally we do it on the stack)
   */
  struct dbl_buffer  txn_ptrs;
  struct slab_alloc  alloc;
  struct arena_alloc backing_alloc;
};

err_t aries_ctx_create (struct aries_ctx *dest, struct i_mem mem, error *e);
void aries_ctx_free (struct aries_ctx *ctx);
struct txn *aries_ctx_txn_alloc (struct aries_ctx *ctx, error *e);
err_t pgr_recover (struct pager *p, error *e);

#endif // PAGER_H
