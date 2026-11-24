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

#include "lock_table/ns_lock_table.h"
#include "ns_error.h"
#include "pager/ns_page_fixture.h"
#include "pager/ns_pager.h"

#include <stdatomic.h>

static void
txntforeach (struct txn *tx, void *ctx)
{
  // Unlock all locks from the txn (2PL shrinking phase)
  lockt_unlock_tx (((struct pager *)ctx)->lt, tx);
}

err_t
pgr_crash (struct pager *p, error *e)
{
  DBG_ASSERT (pager, p);

  // The checkpoint thread can be waiting for the database lock behind a
  // transaction that will never finish now. Stopping the task first would
  // wait for it forever, so let go of the locks first. PGR_CRASHING makes the
  // checkpoint give the lock straight back - a real checkpoint here would
  // flush uncommitted pages and then delete the wal that can undo them
  atomic_fetch_or (&p->flags, PGR_CRASHING);
  txnt_foreach (p->tnxt, txntforeach, p);

  // A crash tears everything down regardless of failures along the way. Each
  // failure is still recorded in [e] and reported by the return below.
  (void)periodic_task_stop (&p->checkpoint_task, e);

  (void)wal_crash (p->ww, e);
  (void)fpgr_crash (p->fp, e);

  txnt_crash (p->tnxt);
  dpgt_crash (p->dpt);
  lockt_free (p->lt, p->mem);
  p->mem.table->free (p->mem.self, p);

  return error_trace (e);
}
