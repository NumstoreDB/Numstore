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
#include "ns_error.h"
#include "pager/ns_pager.h"
#include "recovery/ns_pager_recovery.h"
#include "txn_table/ns_txn_table.h"
#include "wal/ns_wal.h"

static struct txn *
upsert_txn (struct pager *p, struct aries_ctx *ctx, txid tid, lsn read_lsn, lsn prev_lsn, error *e)
{
  struct txn *tx;
  if (!txnt_get (&tx, ctx->txt, tid)) {
    // Doesn't exist - allocate it
    tx = aries_ctx_txn_alloc (ctx, e);
    if (tx == NULL) {
      return NULL;
    }

    txn_init (
        tx,
        tid,
        (struct txn_data){
            .state         = TX_CANDIDATE_FOR_UNDO,
            .min_lsn       = 0,
            .last_lsn      = read_lsn,
            .undo_next_lsn = prev_lsn,
        },
        p->mem
    );

    txnt_insert_txn_if_not_exists (ctx->txt, tx);
  } else {
    // Otherwise, just update its read and prev lsn
    txn_update (tx, TX_CANDIDATE_FOR_UNDO, read_lsn, prev_lsn);
  }

  return tx;
}

struct analyze_context
{
  struct aries_ctx *aries;
  struct pager     *p;
};

static err_t
pgr_analyze_log (struct wal_rec_hdr_read *log_rec, lsn read_lsn, void *_ctx, error *e)
{
  struct analyze_context *a_ctx    = _ctx;
  struct aries_ctx       *ctx      = a_ctx->aries;
  struct pager           *p        = a_ctx->p;

  txid                    tid      = wrh_get_tid (log_rec);
  lsn                     prev_lsn = wrh_get_prev_lsn (log_rec);

  if (tid > ctx->max_tid) {
    ctx->max_tid = tid;
  }

  struct txn *tx = upsert_txn (p, ctx, tid, read_lsn, prev_lsn, e);
  if (tx == NULL) {
    return error_trace (e);
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

      // Add log to dirty page table if it's redoable
      if (wrh_is_redoable (log_rec)) {
        if (dpgt_add_if_ne (ctx->dpt, wrh_get_affected_pg (log_rec), read_lsn, e) < 0) {
          return error_trace (e);
        }
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
  }

  return SUCCESS;
}

static void
flush_txns (struct pager *p, struct aries_ctx *ctx)
{
  // Append end logs and remove rolled back and committed txns
  for (u32 i = 0; i < ctx->txn_ptrs.nelem; ++i) {
    struct txn *tx            = ((struct txn **)ctx->txn_ptrs.data)[i];

    bool        is_candidate  = tx->data.state == TX_CANDIDATE_FOR_UNDO;
    bool        undo_done     = tx->data.undo_next_lsn == 0;
    bool        nothing_to_do = is_candidate && undo_done;
    bool        committed     = tx->data.state == TX_COMMITTED;

    if (nothing_to_do || committed) {
      wal_append_end_log (p->ww, tx->tid, tx->data.last_lsn);
      txnt_remove_txn_expect (ctx->txt, tx);
      txn_update_state (tx, TX_DONE);
    }
  }
}

err_t
pgr_restart_analysis (struct pager *p, struct aries_ctx *ctx, error *e)
{
  DBG_ASSERT (pager, p);
  i_log_info ("Starting Analysis phase\n");

  // Sequential forward scan to reconstruct the txn table and dirty page table
  struct analyze_context a_ctx = {.aries = ctx, .p = p};
  if (wal_forward_scan (p->ww, LSN_NULL, pgr_analyze_log, &a_ctx, e) < 0) {
    return error_trace (e);
  }

  // Flush all txns that need an end log
  MAYBE_UNUSED const u32 before = txnt_get_size (ctx->txt);
  i_log_info ("Analysis phase, txns in table: %" PRIu32 "\n", before);
  flush_txns (p, ctx);
  i_log_info ("Analysis phase: %" PRIu32 " txns were removed\n", before - txnt_get_size (ctx->txt));

  // Set redo_lsn
  if (dpgt_get_size (ctx->dpt) == 0) {
    ctx->redo_lsn = LSN_NULL;
  } else {
    ctx->redo_lsn = dpgt_min_rec_lsn (ctx->dpt);
  }

  i_log_info ("Done with Analysis. RedoLSN = %" PRlsn "\n", ctx->redo_lsn);

  return SUCCESS;
}
