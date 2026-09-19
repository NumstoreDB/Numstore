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
#include "nscore/dpg_table/ns_dirty_page_table.h"
#include "nscore/lock_table/ns_lock_table.h"
#include "nscore/page/ns_page_fixture.h"
#include "nscore/pager/ns_pager.h"
#include "nscore/txn_table/ns_txn_table.h"
#include "nscore/wal/ns_wal.h"

err_t
aries_ctx_create (struct aries_ctx *dest, struct i_mem mem, error *e)
{
  dest->max_tid = 0;
  slab_alloc_init (&dest->alloc, mem, sizeof (struct ns_txn), 1000);
  arena_alloc_create_default (&dest->backing_alloc);

  dest->txt = txnt_open (mem, e);
  if (dest->txt == NULL) {
    goto failed;
  }

  dest->dpt = dpgt_open (mem, e);
  if (dest->dpt == NULL) {
    goto txt_failed;
  }

  if (dblb_create (&dest->txn_ptrs, &dest->backing_alloc, sizeof (struct ns_txn *), 100, e)) {
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
  arena_alloc_free_all (&ctx->backing_alloc);
}

struct ns_txn *
aries_ctx_txn_alloc (struct aries_ctx *ctx, error *e)
{
  struct ns_txn *tx = slab_alloc_alloc (&ctx->alloc, e);
  if (tx == NULL) {
    return NULL;
  }

  if (dblb_append (&ctx->txn_ptrs, &tx, 1, e)) {
    slab_alloc_free (&ctx->alloc, tx);
    return NULL;
  }

  return tx;
}

static err_t
pgr_restart_analysis (struct pager *p, struct aries_ctx *ctx, error *e)
{
  i_log_info ("Starting Analysis phase\n");

  lsn                      read_lsn = 0;

  struct wal_rec_hdr_read *log_rec  = wal_read_next (p->ww, &read_lsn, e);

  if (log_rec == NULL) {
    goto failed;
  }

  while (log_rec->type != WL_EOF) {
    stxid          tid = wrh_get_tid (log_rec);
    struct ns_txn *tx  = NULL;

    if (tid >= 0) {
      if (tid > (stxid)ctx->max_tid) {
        ctx->max_tid = tid;
      }

      slsn prev_lsn = wrh_get_prev_lsn (log_rec);
      ASSERT (prev_lsn >= 0);

      // Get or create the transaction associated with this log record
      if (!txnt_get (&tx, ctx->txt, tid)) {
        // Allocate
        tx = aries_ctx_txn_alloc (ctx, e);
        if (tx == NULL) {
          goto failed;
        }

        txn_init (
            tx,
            tid,
            (struct ns_txn_data){
                .state         = TX_CANDIDATE_FOR_UNDO,
                .last_lsn      = read_lsn,
                .undo_next_lsn = prev_lsn,
            },
            p->mem
        );

        // Insert this transaction
        txnt_insert_txn_if_not_exists (ctx->txt, tx);
      } else {
        txn_update (tx, TX_CANDIDATE_FOR_UNDO, read_lsn, prev_lsn);
      }
    }

    switch (log_rec->type) {
      case WL_UPDATE:
      case WL_CLR: {
        tx->data.last_lsn = read_lsn;

        if (log_rec->type == WL_UPDATE) {
          if (wrh_is_undoable (log_rec)) {
            tx->data.undo_next_lsn = read_lsn;
          }
        } else {
          tx->data.undo_next_lsn = log_rec->clr.undo_next;
        }

        if ((wrh_is_redoable (log_rec))
            && (dpgt_add_if_ne (ctx->dpt, wrh_get_affected_pg (log_rec), read_lsn, e))) {
          goto failed;
        }

        break;
      }
      case WL_COMMIT: {
        tx->data.last_lsn = read_lsn;
        tx->data.state    = TX_COMMITTED;
        break;
      }
      case WL_BEGIN: {
        break;
      }
      case WL_END: {
        txnt_remove_txn_expect (ctx->txt, tx);
        break;
      }
      case WL_EOF: {
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
    }

    log_rec = wal_read_next (p->ww, &read_lsn, e);

    if (log_rec == NULL) {
      goto failed;
    }
  }

  u32 before = txnt_get_size (ctx->txt);
  (void)before; // Unused in release
  i_log_info ("Analysis phase, txns in table: %d\n", before);

  // Append end logs and remove rolled back and committed txns
  for (u32 i = 0; i < ctx->txn_ptrs.nelem; ++i) {
    struct ns_txn *tx  = ((struct ns_txn **)ctx->txn_ptrs.data)[i];

    bool nothing_to_do = (tx->data.state == TX_CANDIDATE_FOR_UNDO && tx->data.undo_next_lsn == 0)
                         != 0;
    bool committed     = tx->data.state == TX_COMMITTED;

    if (nothing_to_do || committed) {
      // Append an end log
      const slsn l = wal_append_end_log (p->ww, tx->tid, tx->data.last_lsn, e);

      if (l < 0) {
        goto failed;
      }
      txnt_remove_txn_expect (ctx->txt, tx);
      txn_update_state (tx, TX_DONE);
    }
  }

  if (dpgt_get_size (ctx->dpt) == 0) {
    ctx->redo_lsn = LSN_NULL;
  } else {
    ctx->redo_lsn = dpgt_min_rec_lsn (ctx->dpt);
  }

  i_log_info ("Analysis phase: %d txns were removed\n", before - txnt_get_size (ctx->txt));
  i_log_info ("Done with Analysis. RedoLSN = %" PRlsn "\n", ctx->redo_lsn);

  return SUCCESS;

failed:
  return error_trace (e);
}

static err_t
pgr_restart_redo (struct pager *p, struct aries_ctx *ctx, error *e)
{
  i_log_info ("Starting Redo phase\n");

  lsn                      read_lsn = ctx->redo_lsn;

  // Read the redo lsn log
  struct wal_rec_hdr_read *log_rec  = wal_read_entry (p->ww, read_lsn, e);
  if (log_rec == NULL) {
    goto failed;
  }

  u32 nredone = 0;
  (void)nredone; // Unused in release

  while (log_rec->type != WL_EOF) {
    switch (log_rec->type) {
      case WL_UPDATE:
      case WL_CLR: {
        if (wrh_is_redoable (log_rec)) {
          lsn  rec_lsn;
          pgno pg = wrh_get_affected_pg (log_rec);

          if (!dpgt_get (&rec_lsn, ctx->dpt, pg)) {
            break;
          }

          if (read_lsn < rec_lsn) {
            break;
          }

          page_h ph = page_h_create ();
          if (pgr_get_writable (&ph, NULL, PG_PERMISSIVE, pg, p, e)) {
            goto failed;
          }

          pgno page_lsn = page_get_page_lsn (page_h_ro (&ph));
          if (page_lsn < read_lsn) {
            wrh_redo (log_rec, &ph);
            nredone++;
            page_set_page_lsn (page_h_w (&ph), read_lsn);
          } else {
            dpgt_update (ctx->dpt, pg, page_lsn + 1);
          }

          pgr_unfix (&ph, PG_PERMISSIVE);
        }
        break;
      }
      default: {
        // Do nothing
        break;
      }
    }

    // Read next log record
    log_rec = wal_read_next (p->ww, &read_lsn, e);
    if (log_rec == NULL) {
      goto failed;
    }
  }

  i_log_info ("Redo phase done. Total redos: %d\n", nredone);

  return SUCCESS;

failed:
  return error_trace (e);
}

static err_t
pgr_restart_undo (struct pager *p, struct aries_ctx *ctx, error *e)
{
  i_log_info ("Starting Undo phase.\n");

  while (true) {
    slsn undo_lsn = txnt_max_u_undo_lsn (ctx->txt);
    if (undo_lsn < 0) {
      break;
    }

    struct wal_rec_hdr_read *log_rec = wal_read_entry (p->ww, undo_lsn, e);
    if (log_rec == NULL) {
      goto failed;
    }

    switch (log_rec->type) {
      case WL_UPDATE: {
        struct ns_txn *tx;
        txnt_get_expect (&tx, ctx->txt, log_rec->update.tid);

        if (wrh_is_undoable (log_rec)) {
          page_h ph = page_h_create ();
          if (pgr_get_writable (&ph, NULL, PG_PERMISSIVE, log_rec->update.phys.pg, p, e)) {
            goto failed;
          }

          // Undo and Append a clr log
          slsn l = wal_append_clr_log (p->ww, wrh_undo (log_rec, tx, &ph), e);
          if (l < 0) {
            goto failed;
          }

          // Set the page lsn
          page_set_page_lsn (page_h_w (&ph), l);

          // Update the last lsn of the transaction
          tx->data.last_lsn = l;

          // Release this page
          pgr_unfix (&ph, PG_PERMISSIVE);
        }

        // Update undo next page
        tx->data.undo_next_lsn = log_rec->update.prev;

        if (log_rec->update.prev == 0) {
          slsn l = wal_append_end_log (p->ww, tx->tid, tx->data.last_lsn, e);
          if (l < 0) {
            goto failed;
          }
          txnt_remove_txn_expect (ctx->txt, tx);
          txn_update_state (tx, TX_DONE);
        }
        break;
      }

      case WL_CLR: {
        struct ns_txn *tx;
        txnt_get_expect (&tx, ctx->txt, log_rec->clr.tid);
        tx->data.undo_next_lsn = log_rec->clr.undo_next;
        break;
      }

      case WL_BEGIN: {
        struct ns_txn *tx;
        txnt_get_expect (&tx, ctx->txt, log_rec->begin.tid);

        slsn l = wal_append_end_log (p->ww, tx->tid, tx->data.last_lsn, e);
        if (l < 0) {
          goto failed;
        }
        txnt_remove_txn_expect (ctx->txt, tx);
        txn_update_state (tx, TX_DONE);
        break;
      }
      case WL_COMMIT:
      case WL_EOF:
      case WL_END: {
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
    }
  }

  i_log_info ("Undo phase done.\n");

  return SUCCESS;

failed:
  return error_trace (e);
}

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
  p->flags |= PGR_ISRESTARTING;

  // ANALYSIS
  if (pgr_restart_analysis (p, ctx, e)) {
    goto theend;
  }

  if (ctx->redo_lsn != LSN_NULL) {
    // REDO
    if (pgr_restart_redo (p, ctx, e)) {
      goto theend;
    }

    // UNDO
    if (pgr_restart_undo (p, ctx, e)) {
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

err_t
pgr_recover (struct pager *p, error *e)
{
  // Run ARIES recovery
  struct aries_ctx ctx;
  if (aries_ctx_create (&ctx, p->mem, e)) {
    return error_trace (e);
  }

  if (pgr_restart (p, &ctx, e)) {
    return error_trace (e);
  }

  // Start transactions one past maximum txid
  atomic_store (&p->next_tid, ctx.max_tid + 1);

  return SUCCESS;
}
