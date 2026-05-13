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

#include "c_specx/concurrency/periodic_task.h"
#include "lockt/lock_table.h"
#include "os_pager/file_pager.h"
#include "pager.h"

/**
 * TODO:
 *   - Different close method for different pager / wal / lock table passed in
 */
err_t
pgr_close (struct pager *p, error *e)
{
  DBG_ASSERT (pager, p);

  // Good idea to run a checkpoint before closing
  // pgr_deletion_blocking_checkpoint (p, e);

  // Stop the checkpoint task if it's running
  periodic_task_stop (&p->checkpoint_task, e);

  // Evict all pages
  for (u32 i = 0; i < MEMORY_PAGE_LEN; ++i)
    {
      struct page_frame *mp = &p->pages[0];

      if (mp->flags & PW_PRESENT)
        {
          // Ignore error
          pgr_evict_unsafe (p, mp, e);
        }
    }

  // Free resources
  wal_close (p->ww, e);
  fpgr_close (p->fp, e);
  lockt_destroy (p->lt);
  i_free (p->lt);

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
