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

#include "c_specx/logging.h"
#include "c_specx/threading.h"
#include "nscore/file_pager.h"
#include "nscore/lock_table.h"
#include "nscore/lt_lock.h"
#include "nscore/pager.h"
#include "nscore/wal.h"

#include <c_specx.h>

err_t
pgr_close (struct pager *p, error *e)
{
  DBG_ASSERT (pager, p);

  i_log_debug ("Closing Database - waiting for checkpoint task to complete\n");

  // Stop the checkpoint task if it's running
  periodic_task_stop (&p->checkpoint_task, e);

  i_log_debug ("Checkpoint task complete\n");

  i_mutex_lock (&p->serial_lock);

  // Similar to a checkpoint
  {
    // Evict all pages- so the database is consistent
    i_log_debug ("Evicting pages\n");
    pgr_evict_all_pages (p, e);

    // Flush the WAL
    i_log_debug ("Flushing WAL\n");
    wal_flush_all (p->ww, e);

    // Get the end_lsn
    lsn end_lsn = wal_start_lsn (p->ww) + wal_size (p->ww);
    i_log_info ("Writing next WAL next start_lsn = %" PRlsn " to the database\n", end_lsn);

    // Write the next min lsn slot
    pgr_write_next_lsn (p, end_lsn, e);

    // Delete the WAL
    wal_close_and_delete (p->ww, e);
    i_log_debug ("WAL is deleted\n");
  }

  fpgr_close (p->fp, e);
  lockt_destroy (p->lt);
  i_free (p->lt);
  i_mutex_unlock (&p->serial_lock);
  i_mutex_free (&p->serial_lock);

  txnt_close (p->tnxt);
  dpgt_close (p->dpt);

  i_free (p);

  return error_trace (e);
}

#ifndef NTEST
TEST (pgr_close_success)
{
  error e = error_create ();
  test_fail_if (pgr_delete_single_file ("testdb", &e));

  struct pager *p = pgr_open_single_file ("testdb", &e);
  // Delete file i_close should fail
  test_assert_equal (pgr_close (p, &e), SUCCESS);
  test_fail_if (pgr_delete_single_file ("foodir", &e));
}
#endif

err_t
pgr_crash (struct pager *p, error *e)
{
  periodic_task_stop (&p->checkpoint_task, e);

  wal_crash (p->ww, e);
  fpgr_crash (p->fp, e);

  txnt_crash (p->tnxt);
  dpgt_crash (p->dpt);
  lockt_destroy (p->lt);
  i_free (p->lt);

  i_free (p);

  return error_trace (e);
}
