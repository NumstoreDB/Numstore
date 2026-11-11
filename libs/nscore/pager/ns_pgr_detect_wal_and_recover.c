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

#include "ns_csx_assert.h"
#include "ns_error.h"
#include "ns_file_pager.h"
#include "ns_logging.h"
#include "ns_utils.h"
#include "pager/ns_page_fixture.h"
#include "pager/ns_pager.h"
#include "wal/ns_wal.h"

#include <limits.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * WAL Existence based recovery. This is tricky logic and should be carefully
 * modified.
 *
 *************************************************************
 * Start Lsn
 *
 * Every WAL has a "start_lsn" which is the lsn that it starts on.
 * Byte offset 10 into an existing running WAL is really start_lsn + 10
 *
 *************************************************************
 * Closing / Checkpointing the database
 *
 * The basic pattern is to always have either the wal or the database
 * have an up to date start_lsn for the "next" wal. So the database gets
 * two slots, because WAL start_lsns are monotonic, so in a "fresh" database,
 * the larger lsn is the most up to date
 *
 * For each wal deletion step, the order goes:
 *    compute end_lsn = sizeof(wal) + wal.start_lsn
 *    overwrite minimum lsn in the database with end_lsn
 *    flush database header
 *    delete the wal
 *
 *    For a CLOSE:
 *      return - wal is deleted
 *
 *    For a CHECKPOINT:
 *      reopen the wal
 *      write end_lsn to the start_lsn slot
 *
 *************************************************************
 * Swapping of LSN's - largest one is the source of truth
 *
 * This pattern, for every new wal, creates a swapping
 * lsn pair in the database. whichever is larger is the
 * actual lsn:
 *
 *  database          WAL
 * start_lsn_0   start_lsn_1
 * start_lsn_1
 *            ||
 *            V
 *  database          WAL
 * start_lsn_2   start_lsn_2
 * start_lsn_1
 *            ||
 *            V
 *  database          WAL
 * start_lsn_2   start_lsn_3
 * start_lsn_3
 *
 * For a fresh open - we need to consider if the WAL
 * was deleted or crashed mid flush or not:
 *
 *************************************************************
 * Stages of Execution during a Close / Checkpoint
 *
 * 1. Normal Execution
 *    - Database:
 *      - a = 900 (High)
 *      - b = 100 (Low)
 *    - WAL:
 *      - start_lsn = 900
 *      - size = 100
 *    - If we crash during normal execution:
 *      - Recognize MAX(a, b) == wal.start_lsn (900 == 900):
 *        - Execute Recovery
 *
 * Compute end_lsn = 900 + 100 = 1000
 *
 * 2. Database is writing end_lsn
 *    - Database:
 *      - a = 900 (High)
 *      - b = 10-- (Partly Written / Invalid - CRC isn't valid)
 *    - WAL:
 *      - start_lsn = 900
 *    - If we crash while writing end_lsn to the database:
 *      - Recognize that either a or b are valid - but not both
 *          - Do the remaining checkpoint tasks:
 *            - compute end_lsn = sizeof(wal) + wal.start_lsn
 *            - overwrite INVALID lsn in the database with end_lsn
 *            - flush database header
 *            - delete the wal
 *            - reopen the wal
 *            - write end_lsn to the start_lsn slot
 *
 * 3. Database finished writing end_lsn
 *    - Database:
 *      - a = 900 (Low)
 *      - b = 1000 (High)
 *    - WAL:
 *      - start_lsn = 900
 *    - If we crash after writing the lsn, but before deleting the WAL:
 *      - Recognize MIN(a, b) == wal.start_lsn (900 == 900):
 *          - Do the remaining checkpoint tasks:
 *            - delete the wal
 *            - reopen the wal
 *            - write end_lsn to the start_lsn slot
 *
 * 4. DELETE WAL
 *    - Database:
 *      - a = 900 (Low)
 *      - b = 1000 (High)
 *    - Close is done here
 *    - To the point of view of the user - a delete is atomic, so no way to crash here
 *
 * 5. Database re-opens
 *    - Database:
 *      - a = 900 (Low)
 *      - b = 1000 (High)
 *    - WAL:
 *      - start_lsn = 10-- (Non existent or Partly Written / Invalid)
 *    - If the database crashes here:
 *      - Next database open will see that it's invalid and try again
 *
 * Some invariants:
 *  1. The database always has at least one "valid" lsn - if both are invalid, it's corruption
 *  2. The WAL either always either (other cases are corruption):
 *      - Has a full valid lsn that matches one of the database lsn's
 *      - Has a partially written lsn
 *  3. If the WAL doesn't exist, then both the pager's lsn's are valid
 */

static err_t
pgr_read_header (struct pager *p, error *e)
{
  DBG_ASSERT (pager, p);

  if (fpgr_read_header (p->fp, p->_header, 0, PAGE_HEADER_LEN, e) < 0) {
    return error_trace (e);
  }

  lsn lsn0;
  u32 lsn0csm;
  u32 lsn0csm_actual = checksum_init ();

  lsn lsn1;
  u32 lsn1csm;
  u32 lsn1csm_actual = checksum_init ();

  memcpy (&lsn0, p->_header + LSN0_OFST, sizeof (lsn));
  memcpy (&lsn0csm, p->_header + LSN0_CSM_OFST, sizeof (u32));

  memcpy (&lsn1, p->_header + LSN1_OFST, sizeof (lsn));
  memcpy (&lsn1csm, p->_header + LSN1_CSM_OFST, sizeof (u32));

  checksum_execute (&lsn0csm_actual, (void *)&lsn0, sizeof (lsn));
  checksum_execute (&lsn1csm_actual, (void *)&lsn1, sizeof (lsn));

  p->header = (struct pager_header){
      .lsn0      = lsn0,
      .lsn0csm   = lsn0csm,
      .lsn0valid = lsn0csm == lsn0csm_actual,
      .lsn1      = lsn1,
      .lsn1csm   = lsn1csm,
      .lsn1valid = lsn1csm == lsn1csm_actual,
  };

  return SUCCESS;
}

/**
 * The WAL doesn't exist (or was never fully written), so both of the pager's
 * lsn's must be valid. Start the new WAL at the maximum of them.
 */
static err_t
pgr_recover_new_wal (struct pager *p, error *e)
{
  DBG_ASSERT (pager, p);
  ASSERT (wal_isnew (p->ww));

  i_log_debug ("Opened a new WAL");

  // Invariant 3 was violated
  if (!p->header.lsn0valid || !p->header.lsn1valid) {

    return error_causef (
        e,
        ERR_CORRUPT,
        "Pager invariant was violated. If the WAL doesn't exist, then both of the pager's "
        "lsn's must be valid"
    );
  }

  lsn start_lsn = MAX (p->header.lsn0, p->header.lsn1);
  i_log_debug ("Writing maximum lsn from the pager to the WAL: %" PRlsn, start_lsn);
  return wal_write_start_lsn (p->ww, start_lsn, e);
}

/**
 * The WAL exists and both of the pager's lsn's are valid. Decide, based on
 * where the WAL start_lsn falls relative to them, where we crashed.
 */
static err_t
pgr_recover_both_lsns_valid (struct pager *p, lsn end_lsn, error *e)
{
  DBG_ASSERT (pager, p);
  ASSERT (p->header.lsn0valid && p->header.lsn1valid);

  // The WAL start_lsn is equal to the maximum of the pager lsn's, meaning
  // we crashed during normal execution
  if (wal_start_lsn (p->ww) == MAX (p->header.lsn0, p->header.lsn1)) {

    i_log_debug (
        "Database crashed during normal execution because wal lsn: %" PRlsn
        " is equal to maximum of page start lsn's: %" PRlsn ", %" PRlsn,
        wal_start_lsn (p->ww),
        p->header.lsn0,
        p->header.lsn1
    );

    return pgr_recover (p, e);
  }

  if (wal_start_lsn (p->ww) == MIN (p->header.lsn0, p->header.lsn1)) {
    // The WAL start_lsn is equal to the minimum of the pager lsn's meaning
    // we crashed after the database finished writing start_lsn, but before
    // the WAL was deleted

    i_log_debug (
        "Database crashed after flushing lsn's to pager, but before deleting the WAL (This "
        "is rare!) because wal lsn: %" PRlsn " is equal to minimum page start lsn's: %" PRlsn
        ", %" PRlsn,
        wal_start_lsn (p->ww),
        p->header.lsn0,
        p->header.lsn1
    );

    // We crashed after flushing the pager lsn's but before deleting the WAL.
    // Before we crashed, we computed the end_lsn to be the wal end lsn,
    // so in a consistent database, the next time the pager is opened, that
    // fact should remain
    if (end_lsn != MAX (p->header.lsn0, p->header.lsn1)) {

      return error_causef (
          e,
          ERR_CORRUPT,
          "Computed WAL lsn: %" PRlsn " does not match any existing pager lsn's: %" PRlsn
          ", %" PRlsn " meaning WAL was truncated or shortened after a crash",
          end_lsn,
          p->header.lsn0,
          p->header.lsn1
      );
    }

    // Finish up the work that the previous crash left us,
    // e.g. delete, reopen, and write the maximum lsn
    if (wal_delete_and_reopen (p->ww, e) < 0) {
      return error_trace (e);
    }

    return wal_write_start_lsn (p->ww, MAX (p->header.lsn0, p->header.lsn1), e);
  }

  ASSERT (wal_start_lsn (p->ww) != p->header.lsn0);
  ASSERT (wal_start_lsn (p->ww) != p->header.lsn1);

  // Invariant 2 was violated because the WAL start_lsn does not match any of the
  // pager's valid lsn's
  return error_causef (
      e,
      ERR_CORRUPT,
      "Pager invariant was violated, WAL lsn: %" PRlsn
      " doesn't match any of the pager lsn's: %" PRlsn ", %" PRlsn,
      wal_start_lsn (p->ww),
      p->header.lsn0,
      p->header.lsn1
  );
}

/**
 * The WAL exists and exactly one of the pager's lsn's is valid, meaning we
 * crashed while writing end_lsn to the pager. Repair the invalid slot with
 * end_lsn, then finish the checkpoint (delete, reopen, write end_lsn).
 */
static err_t
pgr_recover_one_lsn_valid (struct pager *p, lsn end_lsn, error *e)
{
  DBG_ASSERT (pager, p);
  ASSERT (p->header.lsn0valid || p->header.lsn1valid);
  ASSERT (p->header.lsn0valid != p->header.lsn1valid);

  // One lsn is valid
  i_log_debug (
      "Database crashed while writing out an lsn to the pager because one of the lsn's is "
      "invalid (This is rare!)"
  );

  if (p->header.lsn0valid) {
    ASSERT (!p->header.lsn1valid);

    i_log_debug ("Lsn0: %" PRlsn ", LSN1: INVALID", p->header.lsn0);

    if (p->header.lsn0 != wal_start_lsn (p->ww)) {

      return error_causef (
          e,
          ERR_CORRUPT,
          "Pager invariant was violated, WAL lsn: %" PRlsn " does not equal LSN0: %" PRlsn,
          wal_start_lsn (p->ww),
          p->header.lsn0
      );
    }

    if (pgr_write_lsn1 (p, end_lsn, e) < 0) {
      return error_trace (e);
    }

  } else if (p->header.lsn1valid) {
    ASSERT (!p->header.lsn0valid);

    i_log_debug ("Lsn0: INVALID, LSN1: %" PRlsn, p->header.lsn1);

    if (p->header.lsn1 != wal_start_lsn (p->ww)) {

      return error_causef (
          e,
          ERR_CORRUPT,
          "Pager invariant was violated, WAL lsn: %" PRlsn " does not equal LSN1: %" PRlsn,
          wal_start_lsn (p->ww),
          p->header.lsn1
      );
    }

    if (pgr_write_lsn0 (p, end_lsn, e) < 0) {
      return error_trace (e);
    }

  } else {
    UNREACHABLE (); // LCOV_EXCL_LINE
  }

  if (wal_delete_and_reopen (p->ww, e) < 0) {
    return error_trace (e);
  }

  return wal_write_start_lsn (p->ww, end_lsn, e);
}

err_t
pgr_detect_wal_and_recover (struct pager *p, error *e)
{
  DBG_ASSERT (pager, p);

  if (pgr_read_header (p, e) < 0) {
    return error_trace (e);
  }

  // Invariant 1 was violated
  if (!p->header.lsn0valid && !p->header.lsn1valid) {

    return error_causef (
        e,
        ERR_CORRUPT,
        "Pager invariant was violated. Pager must always have at least 1 valid start_lsn in its "
        "header at all times"
    );
  }

  if (wal_isnew (p->ww)) {
    return pgr_recover_new_wal (p, e);
  }

  slsn end_lsn = wal_get_end_lsn (p->ww, e);
  if (end_lsn < 0) {
    return error_trace (e);
  }

  // Both lsn's are valid
  if (p->header.lsn0valid && p->header.lsn1valid) {
    return pgr_recover_both_lsns_valid (p, (lsn)end_lsn, e);
  }

  // Only one lsn is valid
  if (p->header.lsn0valid != p->header.lsn1valid) {
    return pgr_recover_one_lsn_valid (p, (lsn)end_lsn, e);
  }

  UNREACHABLE ();
}
