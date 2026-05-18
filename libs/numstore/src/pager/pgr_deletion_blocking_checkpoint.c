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
#include "numstore/page_h.h"
#include "numstore/pager.h"

err_t pgr_deletion_blocking_checkpoint (struct pager *p, error *e) {
  ASSERT (p->ww);

  // This is what makes the checkpoint blocking
  // it will wait for all open transactions to complete
  if (lockt_lock (p->lt, lock_db (), LM_X, NULL, e)) { return error_trace (e); }

  // The main checkpoint stuff
  if (pgr_flush_all_pages (p, e) < 0) { goto theend; }

  // Delete the WAL and replace it with a fresh one
  if (pgr_refresh_wal (p, e) < 0) { goto theend; }

theend:

  // Unlock the database lock
  return lockt_unlock (p->lt, lock_db (), LM_X, e);
}
