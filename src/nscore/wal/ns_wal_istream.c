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

#include <stdbool.h>
#include <stddef.h>

#include "core/ns_concurrency.h"
#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_numerics.h"
#include "core/ns_stdtypes.h"
#include "core/os/ns_filesystem.h"
#include "core/os/ns_memory.h"
#include "core/testing/ns_testing.h"

struct wal_istream;

/******************************************************************************
 * SECTION: WAL Input Stream
 * ----------------------------------------------------------------------------
 * @brief Reads WAL entries from a WAL file
 *
 * curlsn is the committed LSN: the byte offset in the WAL file of the start
 * of the current log record being read, advanced by walis_mark_end_log().
 * lsnidx tracks bytes read within the current record (reset by
 * walis_mark_start_log()) so that curlsn is updated atomically per record.
 *
 * A partial read (fewer bytes than requested) means a torn record at the
 * end of the WAL; walis_read_all() treats this as soft EOF (iseof=true)
 * and seeks back to curlsn, leaving the file position at the last complete
 * record boundary.
 ******************************************************************************/

DEFINE_DBG_ASSERT (struct wal_istream, wal_istream, w, { ASSERT (w); })

struct wal_istream
{
  i_file fd;     // The file we're reading
  lsn    curlsn; // Where we are within the entire log file
  lsn    lsnidx; // Where we are within the current log

  latch latch;
};

////////////////////////////////////////////////////////////
/// LOGR Mode

struct wal_istream *
walis_open (const char *fname, error *e)
{
  struct wal_istream *dest = default_mem.i_malloc (&default_mem, 1, sizeof *dest, e);
  if (dest == NULL)
  {
    return NULL;
  }

  /**
   * We'll open in write mode too
   * because if we reach the end on a corrupt
   * record, it truncates the file.
   *
   * In the future I forsee this going away.
   */
  if (default_fsvtable.i_open_r (&default_fsvtable, &dest->fd, fname, e))
  {
    default_mem.i_free (&default_mem, dest);
    return NULL;
  }

  const i64 len = dest->fd.fvtable->i_file_size (&dest->fd, e);
  if (len < 0)
  {
    dest->fd.fvtable->i_close (&dest->fd, e);
    default_mem.i_free (&default_mem, dest);
    return NULL;
  }

  if (dest->fd.fvtable->i_seek (&dest->fd, 0, I_SEEK_SET, e) < 0)
  {
    dest->fd.fvtable->i_close (&dest->fd, e);
    default_mem.i_free (&default_mem, dest);
    return NULL;
  }

  dest->curlsn = 0;
  dest->lsnidx = 0;
  latch_init (&dest->latch);

  DBG_ASSERT (wal_istream, dest);

  return dest;
}

#ifdef TESTING
TEST (walis_open)
{
  error e = error_create ();

  TEST_CASE ("Red Path - can't open file")
  {
    err_t (*backup) (i_file_system_vtable *vfs, i_file *dest, const char *fname, error *e) =
        default_fsvtable.i_open_r;

    default_fsvtable.i_open_r = i_open_errio;

    struct wal_istream *wis = walis_open ("foo", &e);
    test_assert (wis == NULL);
    e.cause_code = SUCCESS;
    e.cmlen      = 0;

    default_fsvtable.i_open_r = backup;
  }

  TEST_CASE ("Red Path - can't seek")
  {
    i64 (*backup) (const i_file *fp, u64 offset, seek_t whence, error *e) = default_fvtable.i_seek;
    default_fvtable.i_seek                                                = i_seek_errio;

    struct wal_istream *wis = walis_open ("foo", &e);
    test_assert (wis == NULL);
    e.cause_code = SUCCESS;
    e.cmlen      = 0;

    default_fvtable.i_seek = backup;
  }
}
#endif

err_t
walis_close (struct wal_istream *w, error *e)
{
  DBG_ASSERT (wal_istream, w);
  w->fd.fvtable->i_close (&w->fd, e);
  default_mem.i_free (&default_mem, w);
  return error_trace (e);
}

err_t
walis_seek (struct wal_istream *w, const lsn pos, error *e)
{
  latch_lock (&w->latch);

  DBG_ASSERT (wal_istream, w);

  const i64 res = w->fd.fvtable->i_seek (&w->fd, pos, I_SEEK_SET, e);
  if (res < 0)
  {
    latch_unlock (&w->latch);
    return error_trace (e);
  }

  if ((u64)res != pos)
  {
    latch_unlock (&w->latch);
    return error_causef (e, ERR_CORRUPT, "seek to invalid offset");
  }

  w->curlsn = pos;

  latch_unlock (&w->latch);

  return SUCCESS;
}

err_t
walis_read_all (
    struct wal_istream *w,
    bool               *iseof,
    lsn                *rlsn,
    u32                *checksum,
    void               *data,
    const u32           len,
    error              *e
)
{
  latch_lock (&w->latch);

  DBG_ASSERT (wal_istream, w);

  *iseof = false;
  if (rlsn)
  {
    *rlsn = w->curlsn;
  }

  const i64 bread = w->fd.fvtable->i_read_all (&w->fd, data, len, e);
  if (bread < 0)
  {
    latch_unlock (&w->latch);
    return error_trace (e);
  }

  if (bread < len)
  {
    // Hit EOF - incomplete record (torn write at end of WAL)
    if (bread > 0)
    {
      // Partial read: seek back so the file position is at the
      // record start, leaving it at the last fully-written
      // record boundary
      if (walis_seek (w, w->curlsn, e))
      {
        latch_unlock (&w->latch);
        return error_trace (e);
      }
    }
    *iseof = true;
    latch_unlock (&w->latch);
    return SUCCESS;
  }

  if (checksum)
  {
    checksum_execute (checksum, data, len);
  }

  w->lsnidx += len;

  latch_unlock (&w->latch);

  return SUCCESS;
}

void
walis_mark_start_log (struct wal_istream *w)
{
  latch_lock (&w->latch);
  w->lsnidx = 0; // Reset intra-record byte counter before reading a new record
  latch_unlock (&w->latch);
}

void
walis_mark_end_log (struct wal_istream *w)
{
  latch_lock (&w->latch);
  w->curlsn += w->lsnidx; // Advance committed LSN by the bytes just consumed
  latch_unlock (&w->latch);
}

err_t
walis_crash (struct wal_istream *w, error *e)
{
  DBG_ASSERT (wal_istream, w);
  w->fd.fvtable->i_close (&w->fd, e);
  default_mem.i_free (&default_mem, w);
  return error_trace (e);
}
