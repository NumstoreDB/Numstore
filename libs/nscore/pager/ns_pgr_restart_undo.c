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

#include "ns_csx_assert.h"
#include "ns_error.h"
#include "ns_stdtypes.h"
#include "pager/ns_page_fixture.h"
#include "pager/ns_pager.h"
#include "txn_table/ns_txn_table.h"
#include "wal/ns_wal.h"

err_t
pgr_restart_undo (struct pager *p, struct aries_ctx *ctx, error *e)
{
  DBG_ASSERT (pager, p);
  ASSERT (ctx->redo_lsn != LSN_NULL);

  i_log_info ("Starting Undo phase.\n");

  while (true) {
    slsn undo_lsn = txnt_max_u_undo_lsn (ctx->txt);
    if (undo_lsn < 0) {
      break;
    }

    struct wal_rec_hdr_read *log_rec = wal_read_entry (p->ww, NULL, (lsn)undo_lsn, e);
    if (log_rec == NULL) {
      goto failed;
    }

    switch (log_rec->type) {
      case WL_UPDATE: {
        struct txn *tx;
        txnt_get_expect (&tx, ctx->txt, log_rec->update.tid);

        if (wrh_is_undoable (log_rec)) {
          page_h ph = page_h_create ();
          if (pgr_get_writable (&ph, NULL, PG_PERMISSIVE, log_rec->update.phys.pg, p, e) < 0) {
            goto failed;
          }

          // Undo and Append a clr log
          lsn l = wal_append_clr_log (p->ww, wrh_undo (log_rec, tx, &ph));

          page_set_page_lsn (page_h_w (&ph), l);
          tx->data.last_lsn = l;

          // Same as pgr_rollback and pgr_restart_redo: the undo dirtied
          // this page, and pgr_flush only writes pages in p->dpt
          err_t dirty       = dpgt_add_if_ne (p->dpt, log_rec->update.phys.pg, l, e);

          pgr_unfix (&ph, PG_PERMISSIVE);

          if (dirty < 0) {
            goto failed;
          }
        }

        tx->data.undo_next_lsn = log_rec->update.prev;

        if (log_rec->update.prev == 0) {
          wal_append_end_log (p->ww, tx->tid, tx->data.last_lsn);
          txnt_remove_txn_expect (ctx->txt, tx);
          txn_update_state (tx, TX_DONE);
        }
        break;
      }

      case WL_CLR: {
        struct txn *tx;
        txnt_get_expect (&tx, ctx->txt, log_rec->clr.tid);
        tx->data.undo_next_lsn = log_rec->clr.undo_next;
        break;
      }

      case WL_BEGIN: {
        struct txn *tx;
        txnt_get_expect (&tx, ctx->txt, log_rec->begin.tid);

        wal_append_end_log (p->ww, tx->tid, tx->data.last_lsn);
        txnt_remove_txn_expect (ctx->txt, tx);
        txn_update_state (tx, TX_DONE);
        break;
      }
      case WL_COMMIT:
      case WL_END: {
        UNREACHABLE ();
      }
    }
  }

  i_log_info ("Undo phase done.\n");

  return SUCCESS;

failed:
  return error_trace (e);
}
