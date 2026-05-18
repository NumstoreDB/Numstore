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

#include "c_specx.h"
#include "numstore/lock_table.h"
#include "numstore/lt_lock.h"
#include "numstore/pager.h"
#include "numstore/txn_table.h"

#include <stdatomic.h>

err_t pgr_begin_txn (struct txn *tx, struct pager *p, error *e) {
  DBG_ASSERT (pager, p);

  txid tid = atomic_fetch_add (&p->next_tid, 1);

  slsn l = 0;

  l = wal_append_begin_log (p->ww, tid, e);

  if (l < 0) {
    // WAL append failed - we just wasted a transaction id - not a big deal
    return error_trace (e);
  }

  // Create a new transaction
  txn_init (
      tx,
      tid,
      (struct txn_data){
          .min_lsn       = l,
          .last_lsn      = l,
          .undo_next_lsn = 0,
          .state         = TX_RUNNING,
      });

  // Create a new transaction entry
  txnt_insert_txn (p->tnxt, tx);

  return SUCCESS;
}
