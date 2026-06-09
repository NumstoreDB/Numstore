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

#include "lock_table.h"
#include "pager.h"
#include "txn_table.h"

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
