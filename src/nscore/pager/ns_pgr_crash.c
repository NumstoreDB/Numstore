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
#include "nscore/page/ns_page_fixture.h"
#include "nscore/pager/ns_pager.h"

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
  i_free (p->mem, p->lt);
  i_free (p->mem, p);

  return error_trace (e);
}
