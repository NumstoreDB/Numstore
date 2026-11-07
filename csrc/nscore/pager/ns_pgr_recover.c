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
#include "nscore/page/ns_page_fixture.h"
#include "nscore/pager/ns_pager.h"

err_t pgr_restart_analysis (struct pager *p, struct aries_ctx *ctx, error *e);
err_t pgr_restart_redo (struct pager *p, struct aries_ctx *ctx, error *e);
err_t pgr_restart_undo (struct pager *p, struct aries_ctx *ctx, error *e);

/*
 * Entry point for ARIES crash recovery.
 *
 * Marks PGR_ISRESTARTING for the duration of recovery. This no longer gates
 * the WAL-before-page flush in pgr_flush() - undo appends CLRs, so the rule
 * holds during recovery too.  Runs the three phases in order and frees the
 * aries_ctx on completion, whether or not an error occurred.
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
