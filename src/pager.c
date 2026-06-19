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

#include "pager.h"

#include <limits.h>
#include <stdatomic.h>
#include <stdlib.h>

#include "compile_config.h"
#include "dirty_page_table.h"
#include "error.h"
#include "file_pager.h"
#include "lock_table.h"
#include "page.h"
#include "pager.h"
#include "testing/page_fixture.h"
#include "txn_table.h"
#include "var_algorithms.h"
#include "wal.h"

/******************************************************************************
 * SECTION: ns_init_var_hash_map
 * ----------------------------------------------------------------------------
 * @brief Initialize the first hash map page of a pager
 ******************************************************************************/

err_t
ns_init_var_hash_map (struct pager *p, error *e)
{
  page_h hp = page_h_create ();

  // BEGIN TXN
  struct txn tx;
  if (pgr_begin_txn (&tx, p, e))
  {
    goto failed;
  }

  // Upfront initialization
  if (pgr_isnew (p))
  {
    // Create a new variable hash page
    if (pgr_new (&hp, p, &tx, PG_VAR_HASH_PAGE, e))
    {
      goto failed;
    }

    // Next page should be valid
    //   this is a weak contract
    //   but assumes the structure of the pager,
    //   it's good enough but might need to change
    ASSERT (page_h_pgno (&hp) == VHASH_PGNO);

    if (pgr_release (p, &hp, PG_VAR_HASH_PAGE, e))
    {
      goto failed;
    }
  }

  // COMMIT
  if (pgr_commit (p, &tx, e))
  {
    goto failed;
  }

failed:
  return error_trace (e);
}

/******************************************************************************
 * SECTION: Pager Simple Functions
 * ----------------------------------------------------------------------------
 * @brief A bunch of short simple pager functions
 ******************************************************************************/

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

static void
pgr_unfix (struct pager *p, page_h *h, int flags)
{
  ASSERT (h->mode == PHM_X || h->mode == PHM_S);

  latch_lock (&h->pgr->ctrl);

  ASSERT (h->pgr->flags & PW_PRESENT);

  // Need to save this page
  if (h->mode == PHM_X)
  {
    spgno page_lsn = 0;

    // Can only save valid pages
    ASSERT (
        !page_validate_for_db (&h->pgw->page, flags | PG_SKIP_CHECKSUM, NULL)
    );

    memcpy (&h->pgr->page.raw, h->pgw->page.raw, NS_PAGE_SIZE);

    latch_lock (&h->pgw->ctrl);

    h->pgw->flags    = 0; // Release pgw
    h->pgr->wsibling = -1;

    latch_unlock (&h->pgw->ctrl);

    h->pgw = NULL;

    h->mode = PHM_S;

    // Unlock read page data
    spx_unlock_x (&h->pgr->data);
  }
  else
  {
    // Unlock read page data
    spx_unlock_s (&h->pgr->data);
  }

  h->pgr->pin--;
  latch_unlock (&h->pgr->ctrl);

  h->pgr  = NULL;
  h->mode = PHM_NONE;
}

static err_t
pgr_flush_wall (struct pager *p, error *e)
{
  DBG_ASSERT (pager, p);
  return wal_flush_all (p->ww, e);
}

static err_t
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

static err_t
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

static err_t
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

static err_t
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

static err_t
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

#ifdef TESTING

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
          " pin: %d acess: %d dirty: %d present: %d "
          "sibling: %d type: %d data: %d ctrl: %d|\n",
          i,
          mp->page.pg,
          mp->pin,
          (mp->flags & PW_ACCESS) != 0,
          dpgt_exists (p->dpt, mp->page.pg),
          (mp->flags & PW_PRESENT) != 0,
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

static err_t
pgr_refresh_wal (struct pager *p, error *e)
{
  DBG_ASSERT (pager, p);
  wal_delete_and_reopen (p->ww, e);
  return error_trace (e);
}

/*-----------------------------------------------------------------------------
 * SUBSECTION: ARIES Recovery
 * @brief Classic ARIES Recovery algorithms
 *----------------------------------------------------------------------------*/

err_t
aries_ctx_create (struct aries_ctx *dest, error *e)
{
  dest->max_tid = 0;
  slab_alloc_init (&dest->alloc, sizeof (struct txn), 1000);

  dest->txt = txnt_open (e);
  if (dest->txt == NULL)
  {
    goto failed;
  }

  dest->dpt = dpgt_open (e);
  if (dest->dpt == NULL)
  {
    goto txt_failed;
  }

  if (dblb_create (&dest->txn_ptrs, sizeof (struct txn *), 100, e))
  {
    goto dpt_failed;
  }

  return SUCCESS;

dpt_failed:
  dpgt_close (dest->dpt);
txt_failed:
  txnt_close (dest->txt);
failed:
  slab_alloc_destroy (&dest->alloc);

  return error_trace (e);
}

void
aries_ctx_free (struct aries_ctx *ctx)
{
  ASSERT (ctx);
  slab_alloc_destroy (&ctx->alloc);
  txnt_close (ctx->txt);
  dpgt_close (ctx->dpt);
  dblb_free (&ctx->txn_ptrs);
}

struct txn *
aries_ctx_txn_alloc (struct aries_ctx *ctx, error *e)
{
  struct txn *tx = slab_alloc_alloc (&ctx->alloc, e);
  if (tx == NULL)
  {
    return NULL;
  }

  if (dblb_append (&ctx->txn_ptrs, &tx, 1, e))
  {
    slab_alloc_free (&ctx->alloc, tx);
    return NULL;
  }

  return tx;
}

/******************************************************************************
 * SECTION: pgr_begin_txn
 * ----------------------------------------------------------------------------
 * @brief Begin a new transaction
 ******************************************************************************/

err_t
pgr_begin_txn (struct txn *tx, struct pager *p, error *e)
{
  DBG_ASSERT (pager, p);
  slsn l = 0;

  // Get the next transaction id
  txid tid = atomic_fetch_add (&p->next_tid, 1);

  // Initialize transaction object with empty data
  txn_init (
      tx,
      tid,
      (struct txn_data){
          .min_lsn       = 0,
          .last_lsn      = 0,
          .undo_next_lsn = 0,
          .state         = TX_RUNNING,
      }
  );

  // Lock the database before doing any changes to
  // the log or page table
  if (lockt_lock (p->lt, lock_db (), LM_X, tx, e))
  {
    // Failing here is fine - just couldn't lock the lock table
    // for some reason
    return error_trace (e);
  }

  // Append a begin log record
  l = wal_append_begin_log (p->ww, tid, e);

  if (l < 0)
  {
    // WAL append failed - we just wasted a transaction id - not a big deal
    return error_trace (e);
  }

  // Update transaction meta data
  txn_update_data (
      tx,
      (struct txn_data){
          .min_lsn       = l,
          .last_lsn      = l,
          .undo_next_lsn = 0,
          .state         = TX_RUNNING,
      }
  );

  // Insert txn into the txn table
  txnt_insert_txn (p->tnxt, tx);

  return SUCCESS;
}

/******************************************************************************
 * SECTION: pgr_commit
 * ----------------------------------------------------------------------------
 * @brief Commit an open transaction
 ******************************************************************************/

err_t
pgr_commit (struct pager *p, struct txn *tx, error *e)
{
  DBG_ASSERT (pager, p);

  if (tx->data.state != TX_RUNNING)
  {
    error_causef (e, ERR_DUPLICATE_COMMIT, "txn already committed");

    // Failure here is fine

    goto theend;
  }

  // COMMIT
  slsn l = wal_append_commit_log (p->ww, tx->tid, tx->data.last_lsn, e);
  if (l < 0)
  {
    // Failure here is fine

    goto theend;
  }

  // FLUSH
  if (wal_flush_all (p->ww, e))
  {
    // We have a commit log appended to the WAL.
    // It may or may not be written to disk.
    // If it is written to disk, then good, next recovery,
    // we batch it in the list of commits to append an end
    // to

    goto theend;
  }

  // END
  l = wal_append_end_log (p->ww, tx->tid, l, e);
  if (l < 0)
  {
    // Failing to append an end log isn't a big deal,
    // we'll append it during the next pgr_recovery

    goto theend;
  }

  // Remove the transaction from the txn table
  txnt_remove_txn_expect (p->tnxt, tx);

  // Unlock all locks from the txn (2PL shrinking phase)
  lockt_unlock_tx (p->lt, tx);

  tx->data.state = TX_DONE;

theend:
  return error_trace (e);
}

/******************************************************************************
 * SECTION: pgr_cancel
 * ----------------------------------------------------------------------------
 * @brief Cancel a page that we attempted to write to
 ******************************************************************************/

/*
 * Discard an in-flight page handle without writing changes to disk.
 *
 * For a write handle (PHM_X), the write frame is returned to the pool
 * without copying its contents to the read frame: the mutation is thrown
 * away.  The read frame's wsibling link is cleared so the pool no longer
 * knows a write twin existed.
 *
 * For either handle mode, the read frame's pin is decremented and the
 * handle is reset to PHM_NONE.  The read frame remains in the pool
 * (pinned by other holders or eligible for clock eviction).
 *
 * Used on all error paths to release pages without committing partial
 * changes — the WAL-before-page invariant is never violated because
 * no WAL record is written for cancelled mutations.
 */
void
pgr_cancel (const struct pager *p, page_h *h)
{
  DBG_ASSERT (pager, p);

  ASSERT (h->mode == PHM_X || h->mode == PHM_S);
  ASSERT (h->pgr->flags & PW_PRESENT);

  // Cancel write page
  if (h->mode == PHM_X)
  {
    h->pgw->flags    = 0;
    h->pgr->wsibling = -1;
    h->pgw           = NULL;
    h->mode          = PHM_S;
    spx_unlock_x (&h->pgr->data);
  }
  else
  {
    spx_unlock_s (&h->pgr->data);
  }

  // Decrement pin
  h->pgr->pin--;
  h->pgr  = NULL;
  h->mode = PHM_NONE;
}

/******************************************************************************
 * SECTION: pgr_restart_analysis
 * ----------------------------------------------------------------------------
 * @brief Analysis phase of ARIES (Figure 10)
 ******************************************************************************/

static err_t
pgr_restart_analysis (struct pager *p, struct aries_ctx *ctx, error *e)
{
  i_log_info ("Starting Analysis phase\n");

  lsn read_lsn = 0;

  struct wal_rec_hdr_read *log_rec = wal_read_next (p->ww, &read_lsn, e);

  if (log_rec == NULL)
  {
    goto failed;
  }

  while (log_rec->type != WL_EOF)
  {
    stxid       tid = wrh_get_tid (log_rec);
    struct txn *tx  = NULL;

    if (tid >= 0)
    {
      if (tid > (stxid)ctx->max_tid)
      {
        ctx->max_tid = tid;
      }

      slsn prev_lsn = wrh_get_prev_lsn (log_rec);
      ASSERT (prev_lsn >= 0);

      // Get or create the transaction associated with this log record
      if (!txnt_get (&tx, ctx->txt, tid))
      {
        // Allocate
        tx = aries_ctx_txn_alloc (ctx, e);
        if (tx == NULL)
        {
          goto failed;
        }

        txn_init (
            tx,
            tid,
            (struct txn_data){
                .state         = TX_CANDIDATE_FOR_UNDO,
                .last_lsn      = read_lsn,
                .undo_next_lsn = prev_lsn,
            }
        );

        // Insert this transaction
        txnt_insert_txn_if_not_exists (ctx->txt, tx);
      }
      else
      {
        txn_update (tx, TX_CANDIDATE_FOR_UNDO, read_lsn, prev_lsn);
      }
    }

    switch (log_rec->type)
    {
      case WL_UPDATE:
      case WL_CLR:
      {
        tx->data.last_lsn = read_lsn;

        if (log_rec->type == WL_UPDATE)
        {
          if (wrh_is_undoable (log_rec))
          {
            tx->data.undo_next_lsn = read_lsn;
          }
        }
        else
        {
          tx->data.undo_next_lsn = log_rec->clr.undo_next;
        }

        if (wrh_is_redoable (log_rec))
        {
          if (dpgt_add_if_ne (
                  ctx->dpt,
                  wrh_get_affected_pg (log_rec),
                  read_lsn,
                  e
              ))
          {
            goto failed;
          }
        }

        break;
      }
      case WL_COMMIT:
      {
        tx->data.last_lsn = read_lsn;
        tx->data.state    = TX_COMMITTED;
        break;
      }
      case WL_BEGIN:
      {
        break;
      }
      case WL_END:
      {
        txnt_remove_txn_expect (ctx->txt, tx);
        break;
      }
      case WL_EOF:
      {
        UNREACHABLE ();
      }
    }

    log_rec = wal_read_next (p->ww, &read_lsn, e);

    if (log_rec == NULL)
    {
      goto failed;
    }
  }

  u32 before = txnt_get_size (ctx->txt);
  i_log_info ("Analysis phase, txns in table: %d\n", before);

  // Append end logs and remove rolled back and committed txns
  for (u32 i = 0; i < ctx->txn_ptrs.nelem; ++i)
  {
    struct txn *tx = ((struct txn **)ctx->txn_ptrs.data)[i];

    bool nothing_to_do =
        tx->data.state == TX_CANDIDATE_FOR_UNDO && tx->data.undo_next_lsn == 0;
    bool committed = tx->data.state == TX_COMMITTED;

    if (nothing_to_do || committed)
    {
      // Append an end log
      const slsn l = wal_append_end_log (p->ww, tx->tid, tx->data.last_lsn, e);

      if (l < 0)
      {
        goto failed;
      }
      txnt_remove_txn_expect (ctx->txt, tx);
      txn_update_state (tx, TX_DONE);
    }
  }

  if (dpgt_get_size (ctx->dpt) == 0)
  {
    ctx->redo_lsn = LSN_NULL;
  }
  else
  {
    ctx->redo_lsn = dpgt_min_rec_lsn (ctx->dpt);
  }

  i_log_info (
      "Analysis phase: %d txns were removed\n",
      before - txnt_get_size (ctx->txt)
  );
  i_log_info ("Done with Analysis. RedoLSN = %" PRlsn "\n", ctx->redo_lsn);

  return SUCCESS;

failed:
  return error_trace (e);
}

/******************************************************************************
 * SECTION: pgr_restart_redo
 * ----------------------------------------------------------------------------
 * @brief Redo phase of ARIES (Figure 11)
 ******************************************************************************/

static err_t
pgr_restart_redo (struct pager *p, struct aries_ctx *ctx, error *e)
{
  i_log_info ("Starting Redo phase\n");

  lsn read_lsn = ctx->redo_lsn;

  // Read the redo lsn log
  struct wal_rec_hdr_read *log_rec = wal_read_entry (p->ww, read_lsn, e);
  if (log_rec == NULL)
  {
    goto failed;
  }

  u32 nredone = 0;

  while (log_rec->type != WL_EOF)
  {
    switch (log_rec->type)
    {
      case WL_UPDATE:
      case WL_CLR:
      {
        if (wrh_is_redoable (log_rec))
        {
          lsn  rec_lsn;
          pgno pg = wrh_get_affected_pg (log_rec);

          if (!dpgt_get (&rec_lsn, ctx->dpt, pg))
          {
            break;
          }

          if (read_lsn < rec_lsn)
          {
            break;
          }

          page_h ph = page_h_create ();
          if (pgr_get_writable (&ph, NULL, PG_PERMISSIVE, pg, p, e))
          {
            goto failed;
          }

          pgno page_lsn = page_get_page_lsn (page_h_ro (&ph));
          if (page_lsn < read_lsn)
          {
            wrh_redo (log_rec, &ph);
            nredone++;
            page_set_page_lsn (page_h_w (&ph), read_lsn);
          }
          else
          {
            dpgt_update (ctx->dpt, pg, page_lsn + 1);
          }

          pgr_unfix (p, &ph, PG_PERMISSIVE);
        }
        break;
      }
      default:
      {
        // Do nothing
        break;
      }
    }

    // Read next log record
    log_rec = wal_read_next (p->ww, &read_lsn, e);
    if (log_rec == NULL)
    {
      goto failed;
    }
  }

  i_log_info ("Redo phase done. Total redos: %d\n", nredone);

  return SUCCESS;

failed:
  return error_trace (e);
}

/******************************************************************************
 * SECTION: pgr_restart_undo
 * ----------------------------------------------------------------------------
 * @brief Undo phase of ARIES (Figure 12)
 ******************************************************************************/

static err_t
pgr_restart_undo (struct pager *p, struct aries_ctx *ctx, error *e)
{
  i_log_info ("Starting Undo phase.\n");

  while (true)
  {
    slsn undo_lsn = txnt_max_u_undo_lsn (ctx->txt);
    if (undo_lsn < 0)
    {
      break;
    }

    struct wal_rec_hdr_read *log_rec = wal_read_entry (p->ww, undo_lsn, e);
    if (log_rec == NULL)
    {
      goto failed;
    }

    switch (log_rec->type)
    {
      case WL_UPDATE:
      {
        struct txn *tx;
        txnt_get_expect (&tx, ctx->txt, log_rec->update.tid);

        if (wrh_is_undoable (log_rec))
        {
          page_h ph = page_h_create ();
          if (pgr_get_writable (
                  &ph,
                  NULL,
                  PG_PERMISSIVE,
                  log_rec->update.phys.pg,
                  p,
                  e
              ))
          {
            goto failed;
          }

          // Undo and Append a clr log
          slsn l = wal_append_clr_log (p->ww, wrh_undo (log_rec, tx, &ph), e);
          if (l < 0)
          {
            goto failed;
          }

          // Set the page lsn
          page_set_page_lsn (page_h_w (&ph), l);

          // Update the last lsn of the transaction
          tx->data.last_lsn = l;

          // Release this page
          pgr_unfix (p, &ph, PG_PERMISSIVE);
        }

        // Update undo next page
        tx->data.undo_next_lsn = log_rec->update.prev;

        if (log_rec->update.prev == 0)
        {
          slsn l = wal_append_end_log (p->ww, tx->tid, tx->data.last_lsn, e);
          if (l < 0)
          {
            goto failed;
          }
          txnt_remove_txn_expect (ctx->txt, tx);
          txn_update_state (tx, TX_DONE);
        }
        break;
      }

      case WL_CLR:
      {
        struct txn *tx;
        txnt_get_expect (&tx, ctx->txt, log_rec->clr.tid);
        tx->data.undo_next_lsn = log_rec->clr.undo_next;
        break;
      }

      case WL_BEGIN:
      {
        struct txn *tx;
        txnt_get_expect (&tx, ctx->txt, log_rec->begin.tid);

        slsn l = wal_append_end_log (p->ww, tx->tid, tx->data.last_lsn, e);
        if (l < 0)
        {
          goto failed;
        }
        txnt_remove_txn_expect (ctx->txt, tx);
        txn_update_state (tx, TX_DONE);
        break;
      }
      case WL_COMMIT:
      case WL_EOF:
      case WL_END:
      {
        UNREACHABLE ();
      }
    }
  }

  i_log_info ("Undo phase done.\n");

  return SUCCESS;

failed:
  return error_trace (e);
}

/******************************************************************************
 * SECTION: pgr_restart
 * ----------------------------------------------------------------------------
 * @brief ARIES implementation of pager restart
 ******************************************************************************/

/*
 * Entry point for ARIES crash recovery.
 *
 * Sets PGR_ISRESTARTING for the duration of recovery so that pgr_flush()
 * skips the WAL-before-page flush (the WAL is already ahead of any page
 * being replayed).  Runs the three phases in order and frees the aries_ctx
 * on completion, whether or not an error occurred.
 */
static err_t
pgr_restart (struct pager *p, struct aries_ctx *ctx, error *e)
{
  err_t ret = SUCCESS;
  p->flags |= PGR_ISRESTARTING;

  // ANALYSIS
  if (pgr_restart_analysis (p, ctx, e))
  {
    goto theend;
  }

  if (ctx->redo_lsn != LSN_NULL)
  {
    // REDO
    if (pgr_restart_redo (p, ctx, e))
    {
      goto theend;
    }

    // UNDO
    if (pgr_restart_undo (p, ctx, e))
    {
      goto theend;
    }
  }

  // This is a good time to do a checkpoint
  // pgr_deletion_blocking_checkpoint (p, e);

theend:
  dpgt_merge_into (p->dpt, ctx->dpt, e);
  aries_ctx_free (ctx);
  p->flags &= ~PGR_ISRESTARTING;

  return error_trace (e);
}

static err_t
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
  if (len > (NS_NAME_MAX - 4))
  {
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

  page_h        root = page_h_create ();
  struct pager *ret  = NULL;

  if ((ret = i_calloc (1, sizeof *ret, e)) == NULL)
  {
    goto failed;
  }

  // Initialize "easy" things
  *(struct file_pager **)&ret->fp = fp;
  *(struct wal **)&ret->ww        = ww;
  ret->lt                         = lt;
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

#ifdef TESTING
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
    char *name = i_malloc (NS_NAME_MAX, 1, &e);
    for (int i = 0; i < NS_NAME_MAX; ++i)
    {
      name[i] = 'c';
    }
    name[NS_NAME_MAX - 3] = '\0';

    struct pager *p = pgr_open_single_file (name, &e);
    test_assert (p == NULL);
    test_err_t_check (e.cause_code, ERR_INVALID_ARGUMENT, &e);
    e.cause_code = SUCCESS;

    name[NS_NAME_MAX - 4] = '\0';
    p                     = pgr_open_single_file (name, &e);
    test_assert (p != NULL);

    pgr_close (p, &e);

    // Delete the obtuse name
    pgr_delete_single_file (name, &e);

    i_free (name);
  }
}
#endif

#ifdef TESTING
TEST (pgr_open_basic)
{
  error e = error_create ();
  test_fail_if (pgr_delete_single_file ("testdb", &e));

  i_file fp = {0};
  i_open_rw (&fp, "testdb", &e);

  // File is shorter than page size
  test_fail_if (i_truncate (&fp, NS_PAGE_SIZE - 1, &e));
  struct pager *p = pgr_open_single_file ("testdb", &e);
  test_assert_int_equal (e.cause_code, ERR_CORRUPT);
  test_assert_equal (p, NULL);
  e.cause_code = SUCCESS;

  // Half a page
  test_fail_if (i_truncate (&fp, NS_PAGE_SIZE / 2, &e));
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

/******************************************************************************
 * SECTION: pgr_close
 * ----------------------------------------------------------------------------
 * @brief Close an open pager
 ******************************************************************************/

err_t
pgr_close (struct pager *p, error *e)
{
  DBG_ASSERT (pager, p);

  i_log_debug ("Closing Database - waiting for checkpoint task to complete\n");

  // Stop the checkpoint task if it's running
  periodic_task_stop (&p->checkpoint_task, e);

  i_log_debug ("Checkpoint task complete\n");

  lockt_lock (p->lt, lock_db (), LM_X, NULL, e);

  // Similar to a checkpoint
  {
    // Evict all pages- so the database is consistent
    i_log_debug ("Evicting pages\n");
    pgr_evict_all_pages (p, e);

    // Flush the WAL
    i_log_debug ("Flushing WAL\n");
    wal_flush_all (p->ww, e);

    // Get the end_lsn
    lsn end_lsn = wal_start_lsn (p->ww) + wal_size (p->ww);
    i_log_info (
        "Writing next WAL next start_lsn = %" PRlsn " to the database\n",
        end_lsn
    );

    // Write the next min lsn slot
    pgr_write_next_lsn (p, end_lsn, e);

    // Delete the WAL
    wal_close_and_delete (p->ww, e);
    i_log_debug ("WAL is deleted\n");
  }

  fpgr_close (p->fp, e);
  lockt_unlock (p->lt, lock_db (), LM_X, e);
  lockt_destroy (p->lt);
  i_free (p->lt);

  txnt_close (p->tnxt);
  dpgt_close (p->dpt);

  i_free (p);

  return error_trace (e);
}

#ifdef TESTING
TEST (pgr_close_success)
{
  error e = error_create ();
  test_fail_if (pgr_delete_single_file ("testdb", &e));

  struct pager *p = pgr_open_single_file ("testdb", &e);
  // Delete file i_close should fail
  test_assert_equal (pgr_close (p, &e), SUCCESS);
  test_fail_if (pgr_delete_single_file ("foodir", &e));
}
#endif

static void
txntforeach (struct txn *tx, void *ctx)
{
  // Unlock all locks from the txn (2PL shrinking phase)
  lockt_unlock_tx (((struct pager *)ctx)->lt, tx);
}

err_t
pgr_crash (struct pager *p, error *e)
{
  periodic_task_stop (&p->checkpoint_task, e);

  txnt_foreach (p->tnxt, txntforeach, p);

  wal_crash (p->ww, e);
  fpgr_crash (p->fp, e);

  txnt_crash (p->tnxt);
  dpgt_crash (p->dpt);
  lockt_destroy (p->lt);
  i_free (p->lt);

  i_free (p);

  return error_trace (e);
}

/******************************************************************************
 * SECTION: pgr_delete_and_release
 * ----------------------------------------------------------------------------
 * @brief Delete and release a page
 ******************************************************************************/

/*
 * Free a page and release its handle.
 *
 * Freeing a page means clearing its bit in the FSM so that pgr_new() can
 * reuse it.  Two WAL records are written:
 *
 *   1. The page itself is released with PG_PERMISSIVE and NULL update, which
 *      writes a PHYSICAL WAL record for the page content.  This undo image
 *      lets recovery restore the page if the transaction is rolled back.
 *
 *   2. The FSM page update is logged as WUP_FSM with undo=1 (the bit was 1,
 *      meaning allocated) and redo=0 (after commit the bit is 0, meaning
 *      free).  This compact record avoids logging the full FSM bitmap.
 *
 * The pgno-to-FSM mapping: FSM page for pgno is at (pgno / FS_BTMP_NPGS) *
 * FS_BTMP_NPGS, and the bit index within that FSM page is pgno % FS_BTMP_NPGS.
 */
err_t
pgr_delete_and_release (struct pager *p, struct txn *tx, page_h *h, error *e)
{
  DBG_ASSERT (pager, p);
  page_h fsm = page_h_create ();

  // Truncate to 8 * FS_BTMP_NPGS to get the tracking fsm
  const pgno pg = page_h_pgno (h);
  // FSM page that tracks this pgno lives at the aligned section base
  pgno fsmpg = page_h_pgno (h) / FS_BTMP_NPGS;
  fsmpg *= FS_BTMP_NPGS;

  // Bit index within that FSM page's bitmap
  const p_size idx = page_h_pgno (h) % FS_BTMP_NPGS;

  if (pgr_get_writable (&fsm, tx, PG_FREE_SPACE_MAP, fsmpg, p, e))
  {
    goto failed;
  }

  // Mark the page as free in the bitmap before logging
  fsm_clr_bit (page_h_w (&fsm), idx);

  // Log the freed page with a physical record (NULL = full page image as
  // undo)
  if (pgr_release_with_log (p, h, PG_PERMISSIVE, NULL, e))
  {
    goto failed;
  }

  struct wal_update_write log = wup_fsm (fsmpg, tx, pgtoidx (pg), 1, 0);
  if (pgr_release_with_log (p, &fsm, PG_FREE_SPACE_MAP, &log, e))
  {
    goto failed;
  }

  return SUCCESS;

failed:
  pgr_cancel_if_exists (p, &fsm);

  return error_trace (e);
}

#ifdef TESTING
TEST (pgr_delete)
{
  struct pgr_fixture f;
  error             *e = &f.e;
  pgr_fixture_create (&f);

  struct txn tx;
  pgr_begin_txn (&tx, f.p, e);

  page_h a = page_h_create ();
  page_h b = page_h_create ();
  page_h c = page_h_create ();
  page_h d = page_h_create ();

  pgr_new (&a, f.p, &tx, PG_DATA_LIST, e);
  pgr_new (&b, f.p, &tx, PG_DATA_LIST, e);
  pgr_new (&c, f.p, &tx, PG_DATA_LIST, e);
  pgr_new (&d, f.p, &tx, PG_DATA_LIST, e);

  const pgno apg = page_h_pgno (&a);
  const pgno bpg = page_h_pgno (&b);
  const pgno cpg = page_h_pgno (&c);
  const pgno dpg = page_h_pgno (&d);

  pgr_delete_and_release (f.p, &tx, &a, e);
  pgr_delete_and_release (f.p, &tx, &b, e);
  pgr_delete_and_release (f.p, &tx, &c, e);
  pgr_delete_and_release (f.p, &tx, &d, e);

  pgr_new (&a, f.p, &tx, PG_DATA_LIST, e);
  pgr_new (&b, f.p, &tx, PG_DATA_LIST, e);
  pgr_new (&c, f.p, &tx, PG_DATA_LIST, e);
  pgr_new (&d, f.p, &tx, PG_DATA_LIST, e);

  test_assert_equal (page_h_pgno (&a), apg);
  test_assert_equal (page_h_pgno (&b), bpg);
  test_assert_equal (page_h_pgno (&c), cpg);
  test_assert_equal (page_h_pgno (&d), dpg);

  pgr_delete_and_release (f.p, &tx, &a, e);
  pgr_delete_and_release (f.p, &tx, &b, e);
  pgr_delete_and_release (f.p, &tx, &c, e);
  pgr_delete_and_release (f.p, &tx, &d, e);

  pgr_new (&a, f.p, &tx, PG_DATA_LIST, e);
  pgr_new (&b, f.p, &tx, PG_DATA_LIST, e);
  pgr_new (&c, f.p, &tx, PG_DATA_LIST, e);
  pgr_new (&d, f.p, &tx, PG_DATA_LIST, e);

  test_assert_equal (page_h_pgno (&a), apg);
  test_assert_equal (page_h_pgno (&b), bpg);
  test_assert_equal (page_h_pgno (&c), cpg);
  test_assert_equal (page_h_pgno (&d), dpg);

  dl_set_used (page_h_w (&a), DL_DATA_SIZE);
  dl_set_used (page_h_w (&b), DL_DATA_SIZE);
  dl_set_used (page_h_w (&c), DL_DATA_SIZE);
  dl_set_used (page_h_w (&d), DL_DATA_SIZE);

  pgr_release (f.p, &a, PG_DATA_LIST, e);
  pgr_delete_and_release (f.p, &tx, &b, e);
  pgr_delete_and_release (f.p, &tx, &c, e);
  pgr_release (f.p, &d, PG_DATA_LIST, e);

  pgr_commit (f.p, &tx, &f.e);

  pgr_fixture_teardown (&f);
}
#endif

#ifndef PATH_MAX
#  define PATH_MAX 260
#endif

err_t
pgr_delete_single_file (const char *dbname, error *e)
{
  char fname[PATH_MAX];
  char walname[PATH_MAX];
  snprintf (fname, sizeof fname, "%s", dbname);
  snprintf (walname, sizeof walname, "%s.wal", dbname);

  i_remove_quiet (fname, e);
  i_remove_quiet (walname, e);

  return error_trace (e);
}

/******************************************************************************
 * SECTION: pgr_deletion_blocking_checkpoint
 * ----------------------------------------------------------------------------
 * @brief Blocking checkpoint that delete's the WAL
 ******************************************************************************/

static err_t
pgr_deletion_blocking_checkpoint (struct pager *p, error *e)
{
  ASSERT (p->ww);

  i_log_debug ("Starting Checkpoint - locking the database\n");

  // This is what makes the checkpoint blocking
  // it will wait for all open transactions to complete
  lockt_lock (p->lt, lock_db (), LM_X, NULL, e);
  i_log_debug ("Checkpoint - lock acquired\n");

  // Flush all pages - so the database is consistent
  if (pgr_flush_all_pages (p, e) < 0)
  {
    goto theend;
  }

  // Flush the WAL
  if (wal_flush_all (p->ww, e))
  {
    goto theend;
  }

  // Get the end_lsn
  lsn end_lsn = wal_start_lsn (p->ww) + wal_size (p->ww);
  i_log_info ("CHECKPOINT: Next start of the lsn = %" PRlsn "\n", end_lsn);

  // Write the next min lsn slot
  if (pgr_write_next_lsn (p, end_lsn, e))
  {
    goto theend;
  }

  // Delete the WAL and replace it with a fresh one
  if (pgr_refresh_wal (p, e) < 0)
  {
    goto theend;
  }

  ASSERT (wal_isnew (p->ww));

  // Write the next start lsn for the WAL
  if (wal_write_start_lsn (p->ww, end_lsn, e))
  {
    goto theend;
  }

theend:

  // Unlock the database lock
  i_log_debug ("Checkpoint Done - unlocking the database\n");
  lockt_unlock (p->lt, lock_db (), LM_X, e);
  return SUCCESS;
}

/******************************************************************************
 * SECTION: pgr_evict
 * ----------------------------------------------------------------------------
 * @brief Evict a page frame and flush to disk
 ******************************************************************************/

/*
 * Evict a page frame from the buffer pool.
 *
 * Flushes the frame to disk (honoring WAL-before-page), removes it from the
 * pgno→slot hash table, and clears the frame's flags so the slot can be
 * reused.  The frame must be unpinned (pin == 0) and must not be a write
 * frame (PW_X).
 */
err_t
pgr_evict_unsafe (struct pager *p, struct page_frame *mp, error *e)
{
  ASSERT ((mp->flags & PW_PRESENT));
  ASSERT (!(mp->flags & PW_X));
  ASSERT (mp->pin == 0);

  // Caller holds mp->latch, so use the unsafe (no-latch) flush variant
  if (pgr_flush_unsafe (p, mp, e))
  {
    goto failed;
  }

  ht_delete_expect_idx (&p->pgno_to_value, NULL, mp->page.pg);
  mp->flags = 0;

  return SUCCESS;

failed:
  return error_trace (e);
}

/******************************************************************************
 * SECTION: pgr_extend_file
 * ----------------------------------------------------------------------------
 * @brief Extend a database file by a certain amount
 ******************************************************************************/

/*
 * Extend the database file by npages and record it as a Nested Top Action.
 *
 * File extension is a physical operation (adding pages to the file) that
 * must be undoable if the transaction rolls back, but must NOT be re-undone
 * during a subsequent crash if it was already undone once.  ARIES models
 * this as a Nested Top Action (NTA):
 *
 *   1. Save the current tx's last_lsn as undo_next (the LSN to jump back
 *      to during undo, skipping the extension entirely).
 *
 *   2. Write a WUP_FEXT update record with undo=current_npages and
 *      redo=npages_after_extension.  This record lets redo replay the
 *      extension and undo shrink the file.
 *
 *   3. Write a WCLR_DUMMY CLR whose undo_next points to the LSN saved in
 *      step 1.  During rollback, when the undo cursor hits this CLR it
 *      jumps directly to undo_next, skipping the WUP_FEXT record and
 *      leaving the file extension undone already.  After a crash-during-undo
 *      the CLR is replayed by redo but has no physical effect, guaranteeing
 *      idempotency.
 *
 *   4. Advance tx's last_lsn and undo_next_lsn to the CLR's LSN so the
 *      undo chain is correctly anchored.
 *
 *   5. Actually extend the file on disk.
 */
static err_t
pgr_extend_file (
    const struct pager *p,
    const pgno          npages,
    struct txn         *tx,
    error              *e
)
{
  // Do a Nested Top Action

  // 1. Ascertain the position of the current tx's last log record
  const lsn undo_next = tx->data.last_lsn;

  // 2. Logging the redo and undp information
  slsn top_lsn = wal_append_update_log (
      p->ww,
      (struct wal_update_write){
          .type = WUP_FEXT,
          .tid  = tx->tid,
          .prev = tx->data.last_lsn,
          .fext =
              {
                  .undo = fpgr_get_npages (p->fp),
                  .redo = npages,
              },
      },
      e
  );

  // 3. Writing a dummy CLR whose UNL points to the log record whose
  // position was remembered in 1
  top_lsn = wal_append_clr_log (
      p->ww,
      (struct wal_clr_write){
          .type      = WCLR_DUMMY,
          .tid       = tx->tid,
          .prev      = tx->data.last_lsn,
          .undo_next = undo_next,
      },
      e
  );

  // 4. Anchor both LSN fields to the CLR so the undo chain jumps over the FEXT
  tx->data.last_lsn      = top_lsn;
  tx->data.undo_next_lsn = top_lsn;

  // 5. Physically extend the file; if this fails the WAL records are already
  // durable
  if (fpgr_extend (p->fp, npages, e))
  {
    return error_trace (e);
  }

  return error_trace (e);
}

/******************************************************************************
 * SECTION: pgr_flush
 * ----------------------------------------------------------------------------
 * @brief Flush a page frame to disk - don't remove it from the buffer pool
 * though
 ******************************************************************************/

err_t
pgr_flush_unsafe (const struct pager *p, struct page_frame *mp, error *e)
{
  ASSERT (mp->flags & PW_PRESENT);
  ASSERT (!(mp->flags & PW_X));

  // Only need to write it out if it's dirty
  if (dpgt_exists (p->dpt, mp->page.pg))
  {
    if (!(p->flags & PGR_ISRESTARTING) && p->ww)
    {
      // WAL invariant: always flush page to wal before
      // it's flushed to disk
      // Remember:
      //    page_lsn = latest log page that modified this page
      const lsn plsn = page_get_page_lsn (&mp->page);
      if (wal_flush_all (p->ww, e))
      {
        goto theend;
      }
    }

    // Set page checksum before flushing
    page_set_checksum (&mp->page, page_compute_checksum (&mp->page));

    // Write the page to the database
    if (fpgr_write (p->fp, mp->page.raw, mp->page.pg, e))
    {
      goto theend;
    }

    dpgt_remove_expect (p->dpt, mp->page.pg);
  }

theend:
  return error_trace (e);
}

/******************************************************************************
 * SECTION: pgr_reserve_and_ctrl_lock
 * ----------------------------------------------------------------------------
 * @brief Reserves a page frame - at the end - page is control locked
 ******************************************************************************/

static inline u32
pgr_spin_clock (struct pager *p)
{
  ASSERT (MEMORY_PAGE_LEN % 2 == 0); // For overflow and faster modulo
  return atomic_fetch_add (&p->clock, 1) & (MEMORY_PAGE_LEN - 1);
}

static i32
pgr_reserve_and_ctrl_lock (struct pager *p, error *e)
{
  DBG_ASSERT (pager, p);

  struct page_frame *mp    = NULL; // The working page frame
  u32                clock = pgr_spin_clock (p);
  bool ready_to_evict      = false; // First round - don't evict any pages

  /**
   * Loop forever - this is highly concurrent
   */
  for (;; clock = pgr_spin_clock (p))
  {
    mp = &p->pages[clock];

    if (!latch_trylock (&mp->ctrl))
    {
      // If we can't lock - don't spin - just move on and find a new slot
      continue;
    }

    // Found an empty spot
    if (!(mp->flags & PW_PRESENT))
    {
      goto found_spot;
    }

    // Pinned, skip it
    if (mp->pin > 0)
    {
      latch_unlock (&mp->ctrl);
      continue;
    }

    // Access bit is on - set off and continue
    if (mp->flags & PW_ACCESS)
    {
      mp->flags &= ~PW_ACCESS;

      latch_unlock (&mp->ctrl);
      continue;
    }

    if (ready_to_evict)
    {
      // Found a spot - but it's not being used - safe to evict it
      if (pgr_evict_unsafe (p, mp, e) < 0)
      {
        return error_trace (e);
      }

      goto found_spot;
    }
    else
    {
      // The first round - don't evict anything
      latch_unlock (&mp->ctrl);
      ready_to_evict = true;
    }
  }

found_spot:
  return clock;
}

#ifdef TESTING
TEST (pgr_reserve_and_ctrl_lock_st)
{
  TEST_CASE ("single threaded clock iterator test")
  {
    struct pgr_fixture pgr;
    pgr_fixture_create (&pgr);

    i32 clock1   = pgr_reserve_and_ctrl_lock (pgr.p, &pgr.e);
    pgr.p->clock = 0;
    i32 clock2   = pgr_reserve_and_ctrl_lock (pgr.p, &pgr.e);
    pgr.p->clock = 0;
    i32 clock3   = pgr_reserve_and_ctrl_lock (pgr.p, &pgr.e);
    pgr.p->clock = 0;

    test_assert_int_equal (clock1, 0);
    test_assert_int_equal (clock2, 1);
    test_assert_int_equal (clock3, 2);

    latch_unlock (&pgr.p->pages[0].ctrl);
    latch_unlock (&pgr.p->pages[1].ctrl);
    latch_unlock (&pgr.p->pages[2].ctrl);

    // If I don't do anything and just unlock - it should repeat
    clock1       = pgr_reserve_and_ctrl_lock (pgr.p, &pgr.e);
    pgr.p->clock = 0;
    clock2       = pgr_reserve_and_ctrl_lock (pgr.p, &pgr.e);
    pgr.p->clock = 0;
    clock3       = pgr_reserve_and_ctrl_lock (pgr.p, &pgr.e);
    pgr.p->clock = 0;

    test_assert_int_equal (clock1, 0);
    test_assert_int_equal (clock2, 1);
    test_assert_int_equal (clock3, 2);

    latch_unlock (&pgr.p->pages[0].ctrl);
    latch_unlock (&pgr.p->pages[1].ctrl);
    latch_unlock (&pgr.p->pages[2].ctrl);

    pgr_fixture_teardown (&pgr);
  }
}
#endif

/******************************************************************************
 * SECTION: pgr_get
 * ----------------------------------------------------------------------------
 * @brief Get a readable only page
 ******************************************************************************/

err_t
pgr_get (page_h *dest, int flags, pgno pg, struct pager *p, error *e)
{
  struct page_frame *pgr = NULL; // Read frame
  hdata_idx          data;       // The data to retrieve
  i32                clock;      // Location of the new page
  hta_res            res;

  latch_lock (&p->htable_lock);
  res = ht_get_idx (&p->pgno_to_value, &data, pg);

  switch (res)
  {
    // This page exists in memory
    case HTAR_SUCCESS:
    {
      pgr = &p->pages[data.value];

      latch_lock (&pgr->ctrl);
      latch_unlock (&p->htable_lock);

      pgr->pin++;

      latch_unlock (&pgr->ctrl);

      dest->pgr  = pgr;
      dest->pgw  = NULL;
      dest->mode = PHM_S;

      spx_lock_s (&pgr->data);

      return SUCCESS;
    }
    case HTAR_DOESNT_EXIST:
    {
      latch_unlock (&p->htable_lock);

      // Otherwise, we'll scan for an open spot
      clock = pgr_reserve_and_ctrl_lock (p, e);
      if (clock < 0)
      {
        return error_trace (e);
      }

      pgr = &p->pages[clock];

      if (fpgr_read (p->fp, pgr->page.raw, pg, e))
      {
        latch_unlock (&pgr->ctrl);
        return error_trace (e);
      }

      // Might remove this
      if (page_validate_for_db (&pgr->page, flags, e))
      {
        latch_unlock (&pgr->ctrl);
        return error_trace (e);
      }

      pgr->pin      = 1;                      // Singular owner
      pgr->flags    = PW_ACCESS | PW_PRESENT; // Accessed and present
      pgr->wsibling = -1;                     // No sibling
      pgr->page.pg  = pg;                     // Set the page number

      // Insert this entry into the hash table
      ht_insert_expect_idx (
          &p->pgno_to_value,
          (hdata_idx){
              .key   = pg,
              .value = clock,
          }
      );

      latch_unlock (&pgr->ctrl);

      // Set the page data
      dest->pgr  = pgr;
      dest->pgw  = NULL;
      dest->mode = PHM_S;

      spx_lock_s (&pgr->data);

      return SUCCESS;
    }
  }

  UNREACHABLE ();
}

#ifdef TESTING
TEST (pgr_get_invalid_checksum)
{
  page_h             pg = page_h_create ();
  error              e  = error_create ();
  struct pgr_fixture pf;
  pgr_fixture_create (&pf);

  struct txn tx;
  pgr_begin_txn (&tx, pf.p, &pf.e);

  pgr_new (&pg, pf.p, &tx, PG_DATA_LIST, &pf.e);
  dl_make_valid (page_h_w (&pg));

  const pgno _pg = page_h_pgno (&pg);
  pgr_release_with_flush (pf.p, &pg, PG_DATA_LIST, &pf.e);

  pgr_commit (pf.p, &tx, &pf.e);

  pgr_get (&pg, PG_DATA_LIST, _pg, pf.p, &pf.e);
  test_assert_int_equal (
      page_get_checksum (page_h_ro (&pg)),
      page_compute_checksum (page_h_ro (&pg))
  );

  // Force checksum to be different
  page fake_page;
  memcpy (fake_page.raw, page_h_ro (&pg)->raw, NS_PAGE_SIZE);
  fake_page.pg = page_h_pgno (&pg);
  page_set_checksum (&fake_page, page_get_checksum (&fake_page) + 1);

  pgr_release_with_evict (pf.p, &pg, PG_DATA_LIST, &pf.e);

  // Force a invalid write
  fpgr_write (pf.p->fp, fake_page.raw, fake_page.pg, &pf.e);

  // This one will fail
  test_err_t_check (
      pgr_get (&pg, PG_DATA_LIST, _pg, pf.p, &pf.e),
      ERR_CORRUPT,
      &pf.e
  );

  pgr_fixture_teardown (&pf);
}
#endif

/******************************************************************************
 * SECTION: pgr_get_writable
 * ----------------------------------------------------------------------------
 * @brief Get a page in write mode
 ******************************************************************************/

err_t
pgr_get_writable (
    page_h       *dest,
    struct txn   *tx,
    const int     flags,
    const pgno    pg,
    struct pager *p,
    error        *e
)
{
  struct page_frame *pgr = NULL; // Read frame
  struct page_frame *pgw = NULL; // Write frame
  hdata_idx          data;       // The data to retrieve
  i32                rclock;     // Location of the new page
  i32                wclock;     // Write page location

  latch_lock (&p->htable_lock);
  hta_res res = ht_get_idx (&p->pgno_to_value, &data, pg);

  switch (res)
  {
    // This page exists in memory
    case HTAR_SUCCESS:
    {
      pgr = &p->pages[data.value];

      latch_lock (&pgr->ctrl);
      latch_unlock (&p->htable_lock);

      pgr->pin++;

      latch_unlock (&pgr->ctrl);

      dest->pgr  = pgr;
      dest->pgw  = NULL;
      dest->mode = PHM_S;

      spx_lock_x (&pgr->data);

      // Make a copy in a new slot

      wclock = pgr_reserve_and_ctrl_lock (p, e);
      if (wclock < 0)
      {
        return error_trace (e);
      }

      pgw = &p->pages[wclock];

      pgw->pin      = 1;
      pgw->wsibling = -1;
      pgw->flags    = PW_PRESENT | PW_X;
      memcpy (pgw->page.raw, dest->pgr->page.raw, NS_PAGE_SIZE);
      pgw->page.pg = dest->pgr->page.pg;

      latch_unlock (&pgw->ctrl);

      // Set this page as the sibling of the read page
      latch_lock (&dest->pgr->ctrl);
      dest->pgr->wsibling = wclock;
      latch_unlock (&dest->pgr->ctrl);

      // Set h
      dest->pgw  = pgw;
      dest->mode = PHM_X;
      dest->tx   = tx;

      return SUCCESS;
    }

    case HTAR_DOESNT_EXIST:
    {
      latch_unlock (&p->htable_lock);

      // Grab both r and w slots in once
      rclock = pgr_reserve_and_ctrl_lock (p, e);
      if (rclock < 0)
      {
        return error_trace (e);
      }

      wclock = pgr_reserve_and_ctrl_lock (p, e);
      if (wclock < 0)
      {
        latch_unlock (&p->pages[rclock].ctrl);
        return error_trace (e);
      }

      data = (hdata_idx){.key = pg, .value = rclock};
      ht_insert_expect_idx (&p->pgno_to_value, data);

      pgr = &p->pages[data.value];
      pgw = &p->pages[wclock];

      pgr->pin      = 1;
      pgr->flags    = PW_ACCESS | PW_PRESENT;
      pgr->wsibling = wclock;
      pgr->page.pg  = pg;

      if (fpgr_read (p->fp, pgr->page.raw, pg, e))
      {
        latch_unlock (&pgr->ctrl);
        latch_unlock (&pgw->ctrl);
        return error_trace (e);
      }

      if (page_validate_for_db (&pgr->page, flags, e))
      {
        latch_unlock (&pgr->ctrl);
        latch_unlock (&pgw->ctrl);
        return error_trace (e);
      }

      // Write frame: copy of the read frame, owned by this txn.
      pgw->pin      = 1;
      pgw->wsibling = -1;
      pgw->flags    = PW_PRESENT | PW_X;
      memcpy (pgw->page.raw, pgr->page.raw, NS_PAGE_SIZE);
      pgw->page.pg = pg;

      latch_unlock (&pgr->ctrl);
      latch_unlock (&pgw->ctrl);

      spx_lock_x (&pgr->data);

      dest->pgr  = pgr;
      dest->pgw  = pgw;
      dest->mode = PHM_X;
      dest->tx   = tx;

      return SUCCESS;
    }
  }
  UNREACHABLE ();
}

#ifdef TESTING

/**
struct thread_ctx
{
  struct wal   *w;
  struct pager *p;
  i_semaphore  *begin;
  pgno          a;
  pgno          b;
  pgno          c;
  pgno          d;
  bool          success;
  err_t         ret;
};

static void *
simple_pager_ops (void *_ctx)
{
  struct thread_ctx *ctx = _ctx;
  struct pager      *p   = ctx->p;
  struct txn         tx;
  error              e = error_create ();

  page_h a = page_h_create ();
  page_h b = page_h_create ();
  page_h c = page_h_create ();
  page_h d = page_h_create ();

  // Create some random data
  decl_rand_buffer (abytes, u8, DL_DATA_SIZE);
  decl_rand_buffer (bbytes, u8, DL_DATA_SIZE);
  decl_rand_buffer (cbytes, u8, DL_DATA_SIZE);
  decl_rand_buffer (dbytes, u8, DL_DATA_SIZE);

  i_semaphore_wait (ctx->begin);

  pgr_begin_txn (&tx, p, &e);

  // Create 4 new pages
  pgr_new (&a, p, &tx, PG_DATA_LIST, &e);
  pgr_new (&b, p, &tx, PG_DATA_LIST, &e);
  pgr_new (&c, p, &tx, PG_DATA_LIST, &e);
  pgr_new (&d, p, &tx, PG_DATA_LIST, &e);

  dl_memset (page_h_w (&a), abytes, DL_DATA_SIZE);
  dl_memset (page_h_w (&b), bbytes, DL_DATA_SIZE);
  dl_memset (page_h_w (&c), cbytes, DL_DATA_SIZE);
  dl_memset (page_h_w (&d), dbytes, DL_DATA_SIZE);

  // Get their page numbers
  pgno ap = page_h_pgno (&a);
  pgno bp = page_h_pgno (&b);
  pgno cp = page_h_pgno (&c);
  pgno dp = page_h_pgno (&d);

  // Release all of them
  pgr_release (p, &a, PG_DATA_LIST, &e);
  pgr_release (p, &b, PG_DATA_LIST, &e);
  pgr_release (p, &c, PG_DATA_LIST, &e);
  pgr_release (p, &d, PG_DATA_LIST, &e);

  // Get them
  pgr_get (&a, PG_DATA_LIST, ap, p, &e);
  pgr_get (&b, PG_DATA_LIST, bp, p, &e);
  pgr_get (&c, PG_DATA_LIST, cp, p, &e);
  pgr_get (&d, PG_DATA_LIST, dp, p, &e);

  // Check data
  ctx->success = memcmp (dl_get_data (page_h_ro (&a)), abytes, DL_DATA_SIZE) ==
0; ctx->success = ctx->success && memcmp (dl_get_data (page_h_ro (&b)), bbytes,
DL_DATA_SIZE) == 0; ctx->success = ctx->success && memcmp (dl_get_data
(page_h_ro (&c)), cbytes, DL_DATA_SIZE) == 0; ctx->success = ctx->success &&
memcmp (dl_get_data (page_h_ro (&d)), dbytes, DL_DATA_SIZE) == 0;

  // Release them all
  pgr_release (p, &a, PG_DATA_LIST, &e);
  pgr_release (p, &b, PG_DATA_LIST, &e);
  pgr_release (p, &c, PG_DATA_LIST, &e);
  pgr_release (p, &d, PG_DATA_LIST, &e);

  ctx->ret = e.cause_code;

  ctx->a = ap;
  ctx->b = bp;
  ctx->c = cp;
  ctx->d = dp;

  return NULL;
}

// Robin hood hash table for buffer pool
#  define KTYPE  pgno
#  define VTYPE  bool
#  define SUFFIX pg
#  include "common.h"
#  undef KTYPE
#  undef VTYPE
#  undef SUFFIX

TEST_DISABLED (pager_mt)
{
  struct pgr_fixture pf;
  pgr_fixture_create (&pf);

  i_semaphore begin;

  struct thread_ctx ctx[] = {
      {.p = pf.p, .begin = &begin, .success = false},
      {.p = pf.p, .begin = &begin, .success = false},
      {.p = pf.p, .begin = &begin, .success = false},
      {.p = pf.p, .begin = &begin, .success = false},
      {.p = pf.p, .begin = &begin, .success = false},
      {.p = pf.p, .begin = &begin, .success = false},
      {.p = pf.p, .begin = &begin, .success = false},
      {.p = pf.p, .begin = &begin, .success = false},
      {.p = pf.p, .begin = &begin, .success = false},
      {.p = pf.p, .begin = &begin, .success = false},
      {.p = pf.p, .begin = &begin, .success = false},
      {.p = pf.p, .begin = &begin, .success = false},
      {.p = pf.p, .begin = &begin, .success = false},
      {.p = pf.p, .begin = &begin, .success = false},
      {.p = pf.p, .begin = &begin, .success = false},
  };

  i_thread threads[arrlen (ctx)];

  i_semaphore_create (&begin, arrlen (threads), &pf.e);

  // Create all threads
  for (u32 i = 0; i < arrlen (threads); ++i)
  {
    i_thread_create (&threads[i], simple_pager_ops, &ctx[i], &pf.e);
  }

  // Post semaphore for the number of threads there are
  for (u32 i = 0; i < arrlen (threads); ++i) { i_semaphore_post (&begin); }

  // Join them all
  for (u32 i = 0; i < arrlen (threads); ++i) { i_thread_join (&threads[i],
&pf.e); }

  hash_table_pg unique_set;
  hentry_pg     _hdata[4 * arrlen (threads)];
  ht_init_pg (&unique_set, _hdata, arrlen (_hdata));

  // Check results
  for (u32 i = 0; i < arrlen (threads); ++i)
  {
    test_assert (ctx[i].success);

    test_assert_int_equal (ctx[i].ret, SUCCESS);
    hdata_pg data = {
        .key   = ctx[i].a,
        .value = 0,
    };
    hti_res res = ht_insert_pg (&unique_set, data);
    test_assert_int_equal (res, HTIR_SUCCESS);

    data = (hdata_pg){
        .key   = ctx[i].b,
        .value = 0,
    };
    res = ht_insert_pg (&unique_set, data);
    test_assert_int_equal (res, HTIR_SUCCESS);

    data = (hdata_pg){
        .key   = ctx[i].c,
        .value = 0,
    };
    res = ht_insert_pg (&unique_set, data);
    test_assert_int_equal (res, HTIR_SUCCESS);

    data = (hdata_pg){
        .key   = ctx[i].d,
        .value = 0,
    };
    res = ht_insert_pg (&unique_set, data);
    test_assert_int_equal (res, HTIR_SUCCESS);
  }

  pgr_fixture_teardown (&pf);
}
*/
#endif

/******************************************************************************
 * SECTION: pgr_new
 * ----------------------------------------------------------------------------
 * @brief Create a new page
 ******************************************************************************/

static err_t
pgr_new_impl (
    page_h              *dest,
    struct pager        *p,
    struct txn          *tx,
    const enum page_type type,
    const pgno           pg,
    error               *e
)
{
  DBG_ASSERT (pager, p);
  DBG_ASSERT (page_h, dest);
  ASSERT (dest->mode == PHM_NONE);

  struct page_frame *pgr = NULL;
  struct page_frame *pgw = NULL;

  // Reserve two slots (for read and write pages)
  i32 rclock = pgr_reserve_and_ctrl_lock (p, e);
  if (rclock < 0)
  {
    goto theend;
  }

  i32 wclock = pgr_reserve_and_ctrl_lock (p, e);
  if (wclock < 0)
  {
    latch_unlock (&p->pages[rclock].ctrl);
    goto theend;
  }

  pgr = &p->pages[rclock];
  pgw = &p->pages[wclock];

  // Initialize pgr
  pgr->pin      = 1;
  pgr->flags    = PW_ACCESS | PW_PRESENT;
  pgr->wsibling = wclock;
  page_set_type (&pgr->page, PG_TRASH);
  pgr->page.pg = pg;

  // Initialize pgw
  pgw->pin      = 1;
  pgw->flags    = PW_PRESENT | PW_X;
  pgw->wsibling = -1;
  page_init_empty (&pgw->page, type);
  pgw->page.pg = pg;

  spx_lock_x (&pgr->data);

  latch_unlock (&pgr->ctrl);
  latch_unlock (&pgw->ctrl);

  // Insert page into the hash table
  const hdata_idx hd = (hdata_idx){
      .key   = pg,
      .value = rclock,
  };

  ht_insert_expect_idx (&p->pgno_to_value, hd);

  // Initialize page_h
  dest->pgr  = pgr;
  dest->pgw  = pgw;
  dest->mode = PHM_X;
  dest->tx   = tx;

theend:
  return error_trace (e);
}

static inline err_t
pgr_new_fsmpg (page_h *fsm, struct pager *p, struct txn *tx, error *e)
{
  pgno fsmpg = pgr_get_npages (p);

  // Create a new free space map page
  if (pgr_new_impl (fsm, p, tx, PG_FREE_SPACE_MAP, fsmpg, e))
  {
    return error_trace (e);
  }

  // Create one upfront physical log from nothing to valid fsm page
  // just release with physical log and get again
  if (pgr_release (p, fsm, PG_FREE_SPACE_MAP, e))
  {
    return error_trace (e);
  }
  if (pgr_get_writable (fsm, tx, PG_FREE_SPACE_MAP, fsmpg, p, e))
  {
    return error_trace (e);
  }

  // Creating a new FSM means we are tracking this many pages
  if (pgr_extend_file (p, fsmpg + FS_BTMP_NPGS, tx, e))
  {
    pgr_cancel (p, fsm);
    return error_trace (e);
  }

  return SUCCESS;
}

err_t
pgr_new (
    page_h              *dest,
    struct pager        *p,
    struct txn          *tx,
    const enum page_type type,
    error               *e
)
{
  int    r     = rand ();
  page_h fsm   = page_h_create ();
  pgno   fsmpg = 0;

retry:

  // Iterate through existing FSM's to see if there's a free lockable page
  for (; fsmpg < pgr_get_npages (p); fsmpg += FS_BTMP_NPGS)
  {
    // X(fsm)
    if (pgr_get_writable (&fsm, tx, PG_FREE_SPACE_MAP, fsmpg, p, e))
    {
      return error_trace (e);
    }

    // Find the next free slot
    sp_size next = fsm_next_freebit (page_h_ro (&fsm), 0);

    // No free pages available - move on
    if (next == -1)
    {
      pgr_cancel (p, &fsm);
      continue;
    }

    // Update fsm bit
    fsm_set_bit (page_h_w (&fsm), next);

    // Save with (special) log
    struct wal_update_write log = wup_fsm (page_h_pgno (&fsm), tx, next, 0, 1);
    if (pgr_release_with_log (p, &fsm, PG_FREE_SPACE_MAP, &log, e))
    {
      pgr_cancel (p, &fsm);
      return error_trace (e);
    }

    // Get the requested page
    if (pgr_get_writable (dest, tx, PG_PERMISSIVE, fsmpg + next, p, e))
    {
      return error_trace (e);
    }
    page_init_empty (page_h_w (dest), type);

    return SUCCESS;
  }

  // Used to serialize threads into this function
  latch_lock (&p->pgrnew_lock);
  {
    if (fsmpg < pgr_get_npages (p))
    {
      latch_unlock (&p->pgrnew_lock);
      goto retry;
    }

    if (pgr_new_fsmpg (&fsm, p, tx, e))
    {
      return error_trace (e);
    }
  }
  latch_unlock (&p->pgrnew_lock);

  fsm_set_bit (page_h_w (&fsm), 1);

  if (pgr_new_impl (dest, p, tx, type, fsmpg + 1, e))
  {
    return error_trace (e);
  }

  struct wal_update_write log = wup_fsm (fsmpg, tx, 1, 0, 1);
  if (pgr_release_with_log (p, &fsm, PG_FREE_SPACE_MAP, &log, e))
  {
    pgr_cancel (p, &fsm);
    pgr_cancel (p, dest);
    return error_trace (e);
  }

  return SUCCESS;
}

#ifdef TESTING
TEST (pgr_new_get_save)
{
  struct pgr_fixture f;
  page_h             h = page_h_create ();
  pgr_fixture_create (&f);

  struct txn tx;
  pgr_begin_txn (&tx, f.p, &f.e);

  pgr_new (&h, f.p, &tx, PG_DATA_LIST, &f.e);
  test_assert_int_equal ((int)pgr_get_npages (f.p), FS_BTMP_NPGS);

  // Make it valid
  dl_set_used (page_h_w (&h), DL_DATA_SIZE);

  pgr_release (f.p, &h, PG_DATA_LIST, &f.e);

  pgr_commit (f.p, &tx, &f.e);

  pgr_fixture_teardown (&f);
}
#endif

/******************************************************************************
 * SECTION: Launch checkpoint thread
 * ----------------------------------------------------------------------------
 * @brief Launches a checkpoint periodic thread
 ******************************************************************************/

static void
pgr_do_checkpoint (void *ctx)
{
  struct pager *p = ctx;
  error         e = error_create ();
  if (pgr_deletion_blocking_checkpoint (p, &e))
  {
    error_log_consume (&e);
  }
}

err_t
pgr_launch_checkpoint_thread (struct pager *p, u64 msec, error *e)
{
  return periodic_task_start (
      &p->checkpoint_task,
      msec,
      pgr_do_checkpoint,
      p,
      e
  );
}

/******************************************************************************
 * SECTION: pgr_release_with_log
 * ----------------------------------------------------------------------------
 * @brief Release a page with a log
 ******************************************************************************/

err_t
pgr_release_with_log (
    struct pager            *p,
    page_h                  *h,
    int                      flags,
    struct wal_update_write *record,
    error                   *e
)
{
  ASSERT (h->mode == PHM_X || h->mode == PHM_S);
  ASSERT (h->pgr->flags & PW_PRESENT);

  if (h->mode == PHM_X)
  {
    spgno page_lsn = 0;

    // Can only save valid pages
    ASSERT (!page_validate_for_db (&h->pgw->page, flags | PG_SKIP_CHECKSUM, e));

    // Append WAL information
    if (h->tx) // If you pass tx == NULL - there's no WAL logging - used mid
               // ARIES
    {
      // If no record was supplied, append a physical record
      if (record == NULL)
      {
        page_lsn = wal_append_update_log (
            p->ww,
            (struct wal_update_write){
                .tid  = h->tx->tid,
                .type = WUP_PHYSICAL,
                .prev = h->tx->data.last_lsn,
                .phys =
                    {
                        .pg   = page_h_pgno (h),
                        .undo = h->pgr->page.raw,
                        .redo = h->pgw->page.raw,
                    },
            },
            e
        );
      }
      else
      {
        page_lsn = wal_append_update_log (p->ww, *record, e);
      }

      if (page_lsn < 0)
      {
        return error_trace (e);
      }

      // Update the page lsn
      page_set_page_lsn (page_h_w (h), (lsn)page_lsn);

      h->tx->data.last_lsn      = page_lsn;
      h->tx->data.undo_next_lsn = page_lsn;
    }

    // Add page to DPT if this is the first update (RecLSN = LSN of first
    // update)
    if (!dpgt_exists (p->dpt, page_h_pgno (h)))
    {
      if (dpgt_add (p->dpt, page_h_pgno (h), (lsn)page_lsn, e))
      {
        return error_trace (e);
      }
    }

    memcpy (&h->pgr->page.raw, h->pgw->page.raw, NS_PAGE_SIZE);

    // Update pgw
    latch_lock (&h->pgw->ctrl);
    h->pgw->flags = 0;
    h->pgw->pin   = 0;
    latch_unlock (&h->pgw->ctrl);

    h->pgw = NULL;

    // Update pgr
    latch_lock (&h->pgr->ctrl);
    h->pgr->pin--;
    h->pgr->wsibling = -1;
    latch_unlock (&h->pgr->ctrl);

    h->mode = PHM_S;

    spx_unlock_x (&h->pgr->data);
  }
  else
  {
    // Update pgr
    latch_lock (&h->pgr->ctrl);
    h->pgr->pin--;
    latch_unlock (&h->pgr->ctrl);

    // Unlock from s mode
    spx_unlock_s (&h->pgr->data);
  }

  h->pgr  = NULL;
  h->mode = PHM_NONE;

  return SUCCESS;
}

/******************************************************************************
 * SECTION: pgr_rollback
 * ----------------------------------------------------------------------------
 * @brief Rollback algorithm from ARIES (Figure 8)
 ******************************************************************************/

err_t
pgr_rollback (struct pager *p, struct txn *tx, lsn save_lsn, error *e)
{
  struct wal_rec_hdr_read *log_rec = NULL; // Next record to read
  struct wal_clr_write     clr;            // Next record to write
  page_h ph           = page_h_create (); // The page handle used for all undo's
  lsn    undo_nxt_lsn = tx->data.undo_next_lsn; // Starting undo lsn
  slsn   prev_lsn     = undo_nxt_lsn; // The lsn of the previously written log
  txid   tid          = tx->tid;      // The transaction id

  // First ensure the wal is flushed so that any undoable log is readable
  if (undo_nxt_lsn > 0)
  {
    if (wal_flush_all (p->ww, e))
    {
      goto failed;
    }
  }

  while (save_lsn < undo_nxt_lsn)
  {
    // Read the next undo log entry
    if ((log_rec = wal_read_entry (p->ww, undo_nxt_lsn, e)) == NULL)
    {
      return error_trace (e);
    }

    // Early Done  - this is a corrupt sequence of logs written - we expect a
    // BEGIN
    if (log_rec->type == WL_EOF)
    {
      return error_causef (
          e,
          ERR_CORRUPT,
          "Transaction does not have a valid top level log"
      );
    }

    switch (log_rec->type)
    {
      case WL_UPDATE:
      {
        if (wrh_is_undoable (log_rec))
        {
          pgno pg = wrh_get_affected_pg (log_rec);
          if (pgr_get_writable (&ph, NULL, PG_PERMISSIVE, pg, p, e))
          {
            goto failed;
          }

          // Undo and append a clr log
          prev_lsn = wal_append_clr_log (p->ww, wrh_undo (log_rec, tx, &ph), e);
          if (prev_lsn < 0)
          {
            goto failed;
          }

          // Set the last page lsn
          page_set_page_lsn (page_h_w (&ph), prev_lsn);

          // Set the update lsn
          tx->data.last_lsn = prev_lsn;

          pgr_unfix (p, &ph, PG_PERMISSIVE);
        }

        undo_nxt_lsn = log_rec->update.prev;

        if (undo_nxt_lsn == 0)
        {
          slsn l = wal_append_end_log (p->ww, tx->tid, prev_lsn, e);
          if (l < 0)
          {
            goto failed;
          }
          // We'll break next
        }
        break;
      }

      case WL_CLR:
      {
        undo_nxt_lsn = log_rec->clr.undo_next;
        break;
      }

      case WL_BEGIN:
      {
        undo_nxt_lsn = 0; // Done
        break;
      }
      case WL_COMMIT:
      case WL_END:
      {
        return error_causef (
            e,
            ERR_CORRUPT,
            "unexpected log record during rollback (lsn=%" PRlsn ")",
            undo_nxt_lsn
        );
      }

      case WL_EOF:
      {
        goto theend;
      }
    }

    tx->data.undo_next_lsn = undo_nxt_lsn;
  }

theend:
  lockt_unlock_tx (p->lt, tx);

  if (undo_nxt_lsn > 0)
  {
    if (wal_flush_all (p->ww, e))
    {
      goto failed;
    }
  }

  txnt_remove_txn_expect (p->tnxt, tx);
  tx->data.state = TX_DONE;

  return SUCCESS;

failed:
  return error_trace (e);
}

#ifdef TESTING
TEST (aries_rollback_basic)
{
  error e = error_create ();
  test_fail_if (pgr_delete_single_file ("testdb", &e));

  struct pager *p = pgr_open_single_file ("testdb", &e);
  struct txn    tx;
  page_h        fsm = page_h_create ();
  page_h        pg  = page_h_create ();

  // Create pages in a transaction
  {
    pgr_begin_txn (&tx, p, &e);

    for (int i = 0; i < 3; ++i)
    {
      page_h dl_page = page_h_create ();
      test_fail_if (pgr_new (&dl_page, p, &tx, PG_DATA_LIST, &e));
      dl_make_valid (page_h_w (&dl_page));
      test_fail_if (pgr_release (p, &dl_page, PG_DATA_LIST, &e));
    }

    pgr_flush_wall (p, &e);
  }

  // Verify FSM has all pages before rollback
  {
    test_fail_if (pgr_get (&fsm, PG_FREE_SPACE_MAP, 0, p, &e));

    for (p_size i = 0; i < FS_BTMP_NPGS; ++i)
    {
      if (i < 4)
      {
        test_assert_int_equal (fsm_get_bit (page_h_ro (&fsm), i), 1);
      }
      else
      {
        test_assert_int_equal (fsm_get_bit (page_h_ro (&fsm), i), 0);
      }
    }

    test_fail_if (pgr_release (p, &fsm, PG_FREE_SPACE_MAP, &e));
  }

  // Rollback the transaction
  {
    pgr_rollback (p, &tx, 0, &e);
  }

  // Verify pages are trash after rollback
  {
    for (int i = 1; i < 3; ++i)
    {
      pgr_get (&pg, PG_TRASH, i, p, &e);
      pgr_release (p, &pg, PG_TRASH, &e);
    }
  }

  // Verify FSM reflects rolled back state
  {
    test_fail_if (pgr_get (&fsm, PG_FREE_SPACE_MAP, 0, p, &e));

    for (p_size i = 1; i < FS_BTMP_NPGS; ++i)
    {
      test_assert_int_equal (fsm_get_bit (page_h_ro (&fsm), i), 0);
    }

    test_fail_if (pgr_release (p, &fsm, PG_FREE_SPACE_MAP, &e));
  }

  pgr_close (p, &e);
}

TEST (aries_rollback_multiple_updates)
{
  error e = error_create ();
  test_fail_if (pgr_delete_single_file ("testdb", &e));

  struct pager *p = pgr_open_single_file ("testdb", &e);
  struct txn    tx;
  struct txn    tx2;
  page_h        dl_page = page_h_create ();
  pgno          pgno1;
  u8            initial_data[DL_DATA_SIZE];

  // Txn 1: create and commit initial data
  {
    pgr_begin_txn (&tx, p, &e);

    test_fail_if (pgr_new (&dl_page, p, &tx, PG_DATA_LIST, &e));
    dl_make_valid (page_h_w (&dl_page));

    memset (initial_data, 0xAA, DL_DATA_SIZE);
    dl_set_data (
        page_h_w (&dl_page),
        (struct dl_data){.data = initial_data, .blen = DL_DATA_SIZE}
    );

    pgno1 = page_h_ro (&dl_page)->pg;
    test_fail_if (pgr_release (p, &dl_page, PG_DATA_LIST, &e));
    pgr_commit (p, &tx, &e);
  }

  // Txn 2: make multiple updates then rollback
  {
    pgr_begin_txn (&tx2, p, &e);

    pgr_get_writable (&dl_page, &tx2, PG_DATA_LIST, pgno1, p, &e);
    u8 update1_data[DL_DATA_SIZE];
    memset (update1_data, 0xBB, DL_DATA_SIZE);
    dl_set_data (
        page_h_w (&dl_page),
        (struct dl_data){.data = update1_data, .blen = DL_DATA_SIZE}
    );
    test_fail_if (pgr_release (p, &dl_page, PG_DATA_LIST, &e));

    pgr_get_writable (&dl_page, &tx2, PG_DATA_LIST, pgno1, p, &e);
    u8 update2_data[DL_DATA_SIZE];
    memset (update2_data, 0xCC, DL_DATA_SIZE);
    dl_set_data (
        page_h_w (&dl_page),
        (struct dl_data){.data = update2_data, .blen = DL_DATA_SIZE}
    );
    test_fail_if (pgr_release (p, &dl_page, PG_DATA_LIST, &e));

    pgr_get_writable (&dl_page, &tx2, PG_DATA_LIST, pgno1, p, &e);
    u8 update3_data[DL_DATA_SIZE];
    memset (update3_data, 0xDD, DL_DATA_SIZE);
    dl_set_data (
        page_h_w (&dl_page),
        (struct dl_data){.data = update3_data, .blen = DL_DATA_SIZE}
    );
    test_fail_if (pgr_release (p, &dl_page, PG_DATA_LIST, &e));

    pgr_flush_wall (p, &e);
    pgr_rollback (p, &tx2, 0, &e);
  }

  // Verify data is back to initial state
  {
    pgr_get (&dl_page, PG_DATA_LIST, pgno1, p, &e);
    test_assert_memequal (
        dl_get_data (page_h_ro (&dl_page)),
        initial_data,
        DL_DATA_SIZE
    );
    pgr_release (p, &dl_page, PG_DATA_LIST, &e);
  }

  pgr_close (p, &e);
}

TEST (aries_rollback_with_crash_recovery)
{
  error e = error_create ();
  test_fail_if (pgr_delete_single_file ("testdb", &e));

  struct pager *p = pgr_open_single_file ("testdb", &e);
  struct txn    tx;
  struct txn    tx2;
  page_h        dl_page = page_h_create ();
  pgno          pgno1;
  u8            committed_data[DL_DATA_SIZE];

  // Txn 1: create and commit data
  {
    pgr_begin_txn (&tx, p, &e);

    test_fail_if (pgr_new (&dl_page, p, &tx, PG_DATA_LIST, &e));

    memset (committed_data, 0xAA, DL_DATA_SIZE);
    dl_set_data (
        page_h_w (&dl_page),
        (struct dl_data){.data = committed_data, .blen = DL_DATA_SIZE}
    );

    pgno1 = page_h_ro (&dl_page)->pg;
    test_fail_if (pgr_release (p, &dl_page, PG_DATA_LIST, &e));
    pgr_commit (p, &tx, &e);
  }

  // Txn 2: make changes then crash without commit or rollback
  {
    pgr_begin_txn (&tx2, p, &e);

    pgr_get_writable (&dl_page, &tx2, PG_DATA_LIST, pgno1, p, &e);
    u8 uncommitted_data[DL_DATA_SIZE];
    memset (uncommitted_data, 0xBB, DL_DATA_SIZE);
    dl_set_data (
        page_h_w (&dl_page),
        (struct dl_data){.data = uncommitted_data, .blen = DL_DATA_SIZE}
    );
    test_fail_if (pgr_release (p, &dl_page, PG_DATA_LIST, &e));

    pgr_flush_wall (p, &e);
    test_fail_if (pgr_crash (p, &e));
  }

  // Verify data is back to committed state after recovery
  {
    p = pgr_open_single_file ("testdb", &e);
    pgr_get (&dl_page, PG_DATA_LIST, pgno1, p, &e);
    test_assert_memequal (
        dl_get_data (page_h_ro (&dl_page)),
        committed_data,
        DL_DATA_SIZE
    );
    pgr_release (p, &dl_page, PG_DATA_LIST, &e);
  }

  pgr_close (p, &e);
}

TEST (aries_rollback_clr_not_undone)
{
  error e = error_create ();

  test_fail_if (pgr_delete_single_file ("testdb", &e));

  struct pager *p = pgr_open_single_file ("testdb", &e);
  struct txn    tx;
  struct txn    tx2;
  page_h        dl_page = page_h_create ();
  pgno          pgno1;
  u8            initial_data[DL_DATA_SIZE];

  // Txn 1 - normal commit
  {
    pgr_begin_txn (&tx, p, &e);

    test_fail_if (pgr_new (&dl_page, p, &tx, PG_DATA_LIST, &e));
    dl_make_valid (page_h_w (&dl_page));

    memset (initial_data, 0xAA, DL_DATA_SIZE);
    dl_set_data (
        page_h_w (&dl_page),
        (struct dl_data){.data = initial_data, .blen = DL_DATA_SIZE}
    );

    pgno1 = page_h_ro (&dl_page)->pg;
    test_fail_if (pgr_release (p, &dl_page, PG_DATA_LIST, &e));
    pgr_commit (p, &tx, &e);
  }

  // Txn 2: make update then rollback (generates CLRs)
  {
    pgr_begin_txn (&tx2, p, &e);

    pgr_get_writable (&dl_page, &tx2, PG_DATA_LIST, pgno1, p, &e);

    u8 temp_data[DL_DATA_SIZE];
    memset (temp_data, 0xBB, DL_DATA_SIZE);
    dl_set_data (
        page_h_w (&dl_page),
        (struct dl_data){.data = temp_data, .blen = DL_DATA_SIZE}
    );
    test_fail_if (pgr_release (p, &dl_page, PG_DATA_LIST, &e));

    pgr_rollback (p, &tx2, 0, &e);
  }

  // Verify data is back to initial
  {
    pgr_get (&dl_page, PG_DATA_LIST, pgno1, p, &e);
    test_assert_memequal (
        dl_get_data (page_h_ro (&dl_page)),
        initial_data,
        DL_DATA_SIZE
    );
    pgr_release (p, &dl_page, PG_DATA_LIST, &e);
  }

  // Crash and recover - verify CLRs were not undone
  {
    test_fail_if (pgr_crash (p, &e));
    p = pgr_open_single_file ("testdb", &e);
    pgr_get (&dl_page, PG_DATA_LIST, pgno1, p, &e);
    test_assert_memequal (
        dl_get_data (page_h_ro (&dl_page)),
        initial_data,
        DL_DATA_SIZE
    );
    pgr_release (p, &dl_page, PG_DATA_LIST, &e);
  }

  pgr_close (p, &e);
}
#endif
