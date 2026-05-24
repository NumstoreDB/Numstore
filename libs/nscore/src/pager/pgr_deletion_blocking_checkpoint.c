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

#include "c_specx/error.h"
#include "nscore/lock_table.h"
#include "nscore/lt_lock.h"
#include "nscore/page_h.h"
#include "nscore/pager.h"
#include "nscore/wal.h"

#include <c_specx.h>

err_t
pgr_deletion_blocking_checkpoint (struct pager *p, error *e)
{
  ASSERT (p->ww);

  i_log_debug ("Starting Checkpoint - locking the database\n");

  // This is what makes the checkpoint blocking
  // it will wait for all open transactions to complete
  if (lockt_lock (p->lt, lock_db (), LM_X, NULL, e)) { return error_trace (e); }

  // Flush all pages - so the database is consistent
  if (pgr_flush_all_pages (p, e) < 0) { goto theend; }

  // Flush the WAL
  if (wal_flush_all (p->ww, e)) { goto theend; }

  // Get the end_lsn
  lsn end_lsn = wal_start_lsn (p->ww) + wal_size (p->ww);
  i_log_info ("CHECKPOINT: Next start of the lsn = %" PRlsn "\n", end_lsn);

  // Write the next min lsn slot
  if (pgr_write_next_lsn (p, end_lsn, e)) { goto theend; }

  // Delete the WAL and replace it with a fresh one
  if (pgr_refresh_wal (p, e) < 0) { goto theend; }

  ASSERT (wal_isnew (p->ww));

  // Write the next start lsn for the WAL
  if (wal_write_start_lsn (p->ww, end_lsn, e)) { goto theend; }

theend:

  // Unlock the database lock
  i_log_debug ("Checkpoint Done - unlocking the database\n");
  return lockt_unlock (p->lt, lock_db (), LM_X, e);
}
