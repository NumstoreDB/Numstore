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

#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "nscore/dpg_table/ns_dirty_page_table.h"
#include "nscore/page/ns_page_fixture.h"
#include "nscore/pager/ns_pager.h"
#include "nscore/wal/ns_wal.h"

struct redo_ctx
{
  struct aries_ctx *aries;
  struct pager     *p;
  u32               nredone;
};

static err_t
pgr_redo_log (struct wal_rec_hdr_read *log_rec, lsn read_lsn, void *_ctx, error *e)
{
  struct redo_ctx  *r_ctx = _ctx;
  struct aries_ctx *ctx   = r_ctx->aries;
  struct pager     *p     = r_ctx->p;

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
        if (pgr_get_writable (&ph, NULL, PG_PERMISSIVE, pg, p, e) < 0) {
          return error_trace (e);
        }

        err_t dirty    = SUCCESS;
        lsn   page_lsn = page_get_page_lsn (page_h_ro (&ph));
        if (page_lsn < read_lsn) {
          wrh_redo (log_rec, &ph);
          r_ctx->nredone++;
          page_set_page_lsn (page_h_w (&ph), read_lsn);

          // The redo dirtied this page. ctx->dpt (the table analysis
          // built) only gets merged into p->dpt when restart is done, and
          // pgr_flush looks at p->dpt - so without this a page evicted
          // during restart is dropped without a write and the redo is lost
          //
          // ctx->dpt still decides what gets redone: a page that's flushed
          // mid restart leaves p->dpt, but later records for it still
          // need to be looked at
          dirty = dpgt_add_if_ne (p->dpt, pg, read_lsn, e);
        } else {
          dpgt_update (ctx->dpt, pg, page_lsn + 1);
        }

        pgr_unfix (&ph, PG_PERMISSIVE);

        if (dirty < 0) {
          return error_trace (e);
        }
      }
      break;
    }
    default: {
      break;
    }
  }

  return SUCCESS;
}

err_t
pgr_restart_redo (struct pager *p, struct aries_ctx *ctx, error *e)
{
  DBG_ASSERT (pager, p);
  ASSERT (ctx->redo_lsn != LSN_NULL);

  i_log_info ("Starting Redo phase\n");

  struct redo_ctx r_ctx = {
      .p       = p,
      .aries   = ctx,
      .nredone = 0,
  };
  if (wal_forward_scan (p->ww, ctx->redo_lsn, pgr_redo_log, &r_ctx, e) < 0) {
    return error_trace (e);
  }

  i_log_info ("Redo phase done. Total redos: %" PRIu32 "\n", r_ctx.nredone);

  return SUCCESS;
}
