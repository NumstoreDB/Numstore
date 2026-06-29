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

#include "wal.h"

#include "error.h"
#include "numstore.h"
#include "os.h"
#include "txn_table.h"
#include "wal.h"

#ifdef TESTING
#  include "testing/testing.h"
#endif

/******************************************************************************
 * SECTION: WAL OStream
 * ----------------------------------------------------------------------------
 * @brief Output stream for a write ahead log
 *
 * Responsible for writing wal entries to a destination WAL file
 ******************************************************************************/

DEFINE_DBG_ASSERT (struct wal_ostream, wal_ostream, w, { ASSERT (w); })

struct wal_ostream *
walos_open (const char *fname, error *e)
{
  struct wal_ostream *ret = i_malloc (1, sizeof *ret, e);
  if (ret == NULL)
  {
    return NULL;
  }

  if (i_open_w (&ret->fd, fname, e))
  {
    goto err_free;
  }

  const i64 len = i_seek (&ret->fd, 0, I_SEEK_END, e);
  if (len < 0)
  {
    goto err_close;
  }

  latch_init (&ret->l);
  ret->buffer      = cbuffer_create (ret->_buffer, sizeof (ret->_buffer));
  ret->flushed_lsn = len;

  DBG_ASSERT (wal_ostream, ret);
  return ret;

err_close:
  i_close (&ret->fd, e);
err_free:
  i_free (ret);
  return NULL;
}

#ifdef TESTING
TEST (walos_open)
{
  error e = error_create ();

  TEST_CASE ("Red Path - No Memory")
  {
    void *(*backup) (i_vmem *, u32, u32, error *) = default_vmem.i_malloc;
    default_vmem.i_malloc                         = i_malloc_nomem;

    struct wal_ostream *wos = walos_open ("foo", &e);
    test_assert (wos == NULL);
    e.cause_code = SUCCESS;
    e.cmlen      = 0;

    default_vmem.i_malloc = backup;
  }

  TEST_CASE ("Red Path - can't open file")
  {
    err_t (*backup) (
        i_file_system_vtable *vfs,
        i_file               *dest,
        const char           *fname,
        error                *e
    ) = default_fsvtable.i_open_w;

    default_fsvtable.i_open_w = i_open_errio;

    struct wal_ostream *wos = walos_open ("foo", &e);
    test_assert (wos == NULL);
    e.cause_code = SUCCESS;
    e.cmlen      = 0;

    default_fsvtable.i_open_w = backup;
  }

  TEST_CASE ("Red Path - can't seek")
  {
    i64 (*backup) (const i_file *fp, u64 offset, seek_t whence, error *e) =
        default_fvtable.i_seek;
    default_fvtable.i_seek = i_seek_errio;

    struct wal_ostream *wos = walos_open ("foo", &e);
    test_assert (wos == NULL);
    e.cause_code = SUCCESS;
    e.cmlen      = 0;

    default_fvtable.i_seek = backup;
  }
}
#endif

err_t
walos_close (struct wal_ostream *w, error *e)
{
  DBG_ASSERT (wal_ostream, w);
  walos_flush_all (w, e);
  i_close (&w->fd, e);
  i_free (w);
  return error_trace (e);
}

err_t
walos_crash (struct wal_ostream *w, error *e)
{
  DBG_ASSERT (wal_ostream, w);
  i_close (&w->fd, e);
  i_free (w);
  return error_trace (e);
}

static err_t
walos_flush_impl (struct wal_ostream *w, error *e)
{
  const u32 towrite = cbuffer_len (&w->buffer);
  if (towrite == 0)
  {
    return SUCCESS;
  }

  if (cbuffer_write_to_file_1_expect (&w->fd, &w->buffer, towrite, e))
  {
    panic ("Wal write failed");
  }
  cbuffer_write_to_file_2 (&w->buffer, towrite);

  if (i_fsync (&w->fd, e))
  {
    panic ("Wal fsync failed");
  }

  w->flushed_lsn += towrite;
  return SUCCESS;
}

err_t
walos_flush_all (struct wal_ostream *w, error *e)
{
  DBG_ASSERT (wal_ostream, w);
  latch_lock (&w->l);
  const err_t ret = walos_flush_impl (w, e);
  latch_unlock (&w->l);
  return ret;
}

err_t
walos_write_all (
    struct wal_ostream *w,
    u32                *checksum,
    const void         *data,
    const u32           len,
    error              *e
)
{
  DBG_ASSERT (wal_ostream, w);

  if (checksum)
  {
    checksum_execute (checksum, data, len);
  }

  u32       written = 0;
  const u8 *src     = data;

  latch_lock (&w->l);

  while (written < len)
  {
    if (cbuffer_avail (&w->buffer) == 0)
    {
      if (walos_flush_impl (w, e))
      {
        latch_unlock (&w->l);
        return error_trace (e);
      }
    }

    const u32 towrite = MIN (len - written, cbuffer_avail (&w->buffer));
    if (towrite > 0)
    {
      cbuffer_write_expect (src + written, 1, towrite, &w->buffer);
      written += towrite;
    }
  }

  latch_unlock (&w->l);
  return SUCCESS;
}

lsn
walos_get_next_lsn (struct wal_ostream *w)
{
  latch_lock (&w->l);
  lsn ret = w->flushed_lsn + cbuffer_len (&w->buffer);
  latch_unlock (&w->l);
  return ret;
}

slsn
walos_truncate (struct wal_ostream *w, error *e)
{
  latch_lock (&w->l);
  if (i_truncate (&w->fd, 0, e))
  {
    goto theend;
  }
  if (i_seek (&w->fd, 0, I_SEEK_SET, e))
  {
    goto theend;
  }

theend:
  latch_unlock (&w->l);
  return error_trace (e);
}

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
  struct wal_istream *dest = i_malloc (1, sizeof *dest, e);
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
  if (i_open_r (&dest->fd, fname, e))
  {
    i_free (dest);
    return NULL;
  }

  const i64 len = i_file_size (&dest->fd, e);
  if (len < 0)
  {
    i_close (&dest->fd, e);
    i_free (dest);
    return NULL;
  }

  if (i_seek (&dest->fd, 0, I_SEEK_SET, e) < 0)
  {
    i_close (&dest->fd, e);
    i_free (dest);
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

  TEST_CASE ("Red Path - No Memory")
  {
    void *(*backup) (i_vmem *, u32, u32, error *) = default_vmem.i_malloc;
    default_vmem.i_malloc                         = i_malloc_nomem;

    struct wal_istream *wis = walis_open ("foo", &e);
    test_assert (wis == NULL);
    e.cause_code = SUCCESS;
    e.cmlen      = 0;

    default_vmem.i_malloc = backup;
  }

  TEST_CASE ("Red Path - can't open file")
  {
    err_t (*backup) (
        i_file_system_vtable *vfs,
        i_file               *dest,
        const char           *fname,
        error                *e
    ) = default_fsvtable.i_open_r;

    default_fsvtable.i_open_r = i_open_errio;

    struct wal_istream *wis = walis_open ("foo", &e);
    test_assert (wis == NULL);
    e.cause_code = SUCCESS;
    e.cmlen      = 0;

    default_fsvtable.i_open_r = backup;
  }

  TEST_CASE ("Red Path - can't seek")
  {
    i64 (*backup) (const i_file *fp, u64 offset, seek_t whence, error *e) =
        default_fvtable.i_seek;
    default_fvtable.i_seek = i_seek_errio;

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
  i_close (&w->fd, e);
  i_free (w);
  return error_trace (e);
}

err_t
walis_seek (struct wal_istream *w, const lsn pos, error *e)
{
  latch_lock (&w->latch);

  DBG_ASSERT (wal_istream, w);

  const i64 res = i_seek (&w->fd, pos, I_SEEK_SET, e);
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

  const i64 bread = i_read_all (&w->fd, data, len, e);
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
  i_close (&w->fd, e);
  i_free (w);
  return error_trace (e);
}

/******************************************************************************
 * SECTION: WAL
 * ----------------------------------------------------------------------------
 * @brief Main WAL object for writing WAL logs
 ******************************************************************************/

static err_t
wal_init (struct wal *dest, error *e)
{
  dest->ostream = NULL;
  dest->istream = NULL;
  dest->flags   = 0;
  latch_init (&dest->latch);

  dest->ostream = walos_open (dest->fname.data, e);
  if (dest->ostream == NULL)
  {
    i_free ((char *)dest->fname.data);
    return error_trace (e);
  }

  dest->istream = walis_open (dest->fname.data, e);
  if (dest->istream == NULL)
  {
    i_free ((char *)dest->fname.data);
    walos_close (dest->ostream, e);
    return error_trace (e);
  }

  // Read the start lsn
  bool iseof     = false;
  u32  checksum  = checksum_init ();
  lsn  start_lsn = 0;

  walis_mark_start_log (dest->istream);

  if (walis_read_all (
          dest->istream,
          &iseof,
          NULL,
          &checksum,
          &start_lsn,
          sizeof (start_lsn),
          e
      ))
  {
    i_free ((char *)dest->fname.data);
    walos_close (dest->ostream, e);
    walis_close (dest->istream, e);
    return error_trace (e);
  }

  walis_mark_end_log (dest->istream);

  if (iseof)
  {
    dest->flags |= WAL_ISNEW;

    // Truncate the output wal
    if (walos_truncate (dest->ostream, e))
    {
      i_free ((char *)dest->fname.data);
      walos_close (dest->ostream, e);
      walis_close (dest->istream, e);
      return error_trace (e);
    }
  }
  else
  {
    dest->start_lsn = start_lsn;
  }

  DBG_ASSERT (wal, dest);

  return error_trace (e);
}

err_t
wal_write_start_lsn (struct wal *w, lsn start_lsn, error *e)
{
  ASSERT (w->flags & WAL_ISNEW);

  WRAP (walos_write_all (w->ostream, NULL, &start_lsn, sizeof (start_lsn), e));

  w->start_lsn = start_lsn;
  w->flags &= ~WAL_ISNEW;

  // Seek the reader to the start too
  WRAP (walis_seek (w->istream, sizeof (start_lsn), e));

  return SUCCESS;
}

static struct wal *
wal_open_internal (const char *fname, error *e)
{
  struct wal *dest = i_malloc (1, sizeof *dest, e);
  if (dest == NULL)
  {
    return NULL;
  }

  if (string_copy (&dest->fname, strfcstr (fname), e))
  {
    i_free (dest);
    return NULL;
  }

  if (wal_init (dest, e))
  {
    i_free ((char *)dest->fname.data);
    i_free (dest);
    return NULL;
  }

  return dest;
}

struct wal *
wal_open (const char *fname, error *e)
{
  return wal_open_internal (fname, e);
}

static inline err_t
wal_destroy (struct wal *w, error *e)
{
  wal_flush_all (w, e);
  walos_close (w->ostream, e);
  walis_close (w->istream, e);

  if (w->fname.data)
  {
    i_free ((void *)w->fname.data);
  }
  return error_trace (e);
}

err_t
wal_close (struct wal *w, error *e)
{
  wal_destroy (w, e);
  i_free (w);
  return error_trace (e);
}

err_t
wal_close_and_delete (struct wal *w, error *e)
{
  struct string fname = w->fname;
  w->fname.data       = NULL;

  wal_destroy (w, e);
  i_free (w);

  i_remove_quiet (fname.data, e);
  i_free ((char *)fname.data);

  return error_trace (e);
}

err_t
wal_delete_and_reopen (struct wal *w, error *e)
{
  latch_lock (&w->latch);

  // Copy fname to pass in
  struct string fname = w->fname;

  // Set to NULL so we don't free it
  w->fname.data = NULL;

  if (wal_destroy (w, e))
  {
    latch_unlock (&w->latch);
    return error_trace (e);
  }

  if (i_remove_quiet (fname.data, e))
  {
    latch_unlock (&w->latch);
    return error_trace (e);
  }

  w->fname = fname;

  return wal_init (w, e);
}

bool
wal_isnew (const struct wal *w)
{
  return w->flags & WAL_ISNEW;
}

lsn
wal_start_lsn (struct wal *w)
{
  return w->start_lsn;
}

lsn
wal_size (struct wal *w)
{
  return walos_get_next_lsn (w->ostream);
}

err_t
wal_flush_all (const struct wal *w, error *e)
{
  DBG_ASSERT (wal, w);
  ASSERT (w->ostream);
  return walos_flush_all (w->ostream, e);
}

err_t
wal_crash (struct wal *w, error *e)
{
  DBG_ASSERT (wal, w);

  walos_close (w->ostream, e);
  walis_close (w->istream, e);
  if (w->fname.data)
  {
    i_free ((void *)w->fname.data);
  }
  i_free (w);

  return SUCCESS;
}

/*-----------------------------------------------------------------------------
 * SUBSECTION: WAL Write
 * @brief Write implementation functions
 *----------------------------------------------------------------------------*/

slsn
wal_append_begin_log (struct wal *w, const txid tid, error *e)
{
  latch_lock (&w->latch);
  DBG_ASSERT (wal, w);
  w->whdr.type      = WL_BEGIN;
  w->whdr.begin     = (struct wal_begin){.tid = tid};
  const slsn result = wal_write_locked (w, e);
  latch_unlock (&w->latch);
  return result;
}

slsn
wal_append_commit_log (struct wal *w, const txid tid, const lsn prev, error *e)
{
  latch_lock (&w->latch);
  DBG_ASSERT (wal, w);
  w->whdr.type      = WL_COMMIT;
  w->whdr.commit    = (struct wal_commit){.tid = tid, .prev = prev};
  const slsn result = wal_write_locked (w, e);
  latch_unlock (&w->latch);
  return result;
}

slsn
wal_append_end_log (struct wal *w, const txid tid, const lsn prev, error *e)
{
  latch_lock (&w->latch);
  DBG_ASSERT (wal, w);
  w->whdr.type      = WL_END;
  w->whdr.end       = (struct wal_end){.tid = tid, .prev = prev};
  const slsn result = wal_write_locked (w, e);
  latch_unlock (&w->latch);
  return result;
}

slsn
wal_append_update_log (
    struct wal                   *w,
    const struct wal_update_write update,
    error                        *e
)
{
  latch_lock (&w->latch);
  DBG_ASSERT (wal, w);
  w->whdr.type      = WL_UPDATE;
  w->whdr.update    = update;
  const slsn result = wal_write_locked (w, e);
  latch_unlock (&w->latch);
  return result;
}

slsn
wal_append_clr_log (struct wal *w, const struct wal_clr_write clr, error *e)
{
  latch_lock (&w->latch);
  DBG_ASSERT (wal, w);
  w->whdr.type      = WL_CLR;
  w->whdr.clr       = clr;
  const slsn result = wal_write_locked (w, e);
  latch_unlock (&w->latch);
  return result;
}

slsn
wal_append_log (struct wal *w, const struct wal_rec_hdr_write *whdr, error *e)
{
  switch (whdr->type)
  {
    case WL_BEGIN:
    {
      return wal_append_begin_log (w, whdr->begin.tid, e);
    }
    case WL_COMMIT:
    {
      return wal_append_commit_log (w, whdr->commit.tid, whdr->commit.prev, e);
    }
    case WL_END:
    {
      return wal_append_end_log (w, whdr->end.tid, whdr->end.prev, e);
    }
    case WL_UPDATE:
    {
      return wal_append_update_log (w, whdr->update, e);
    }
    case WL_CLR:
    {
      return wal_append_clr_log (w, whdr->clr, e);
    }
    case WL_EOF: UNREACHABLE (); // LCOV_EXCL_LINE
  }

  UNREACHABLE (); // LCOV_EXCL_LINE
}

/*-----------------------------------------------------------------------------
 * SUBSECTION: WAL Read
 * @brief Read implementation functions
 *----------------------------------------------------------------------------*/

static int
wal_read_full (
    const struct wal *w,
    u32              *checksum,
    const wlh         type,
    const wlh         second_type,
    u8               *buf,
    const u32         total_len,
    error            *e
)
{
  ASSERT (total_len >= sizeof (wlh) + sizeof (u32));

  u8 *head = buf;

  memcpy (head, &type, sizeof (wlh));
  head += sizeof (wlh);

  if (second_type != WLH_NULL)
  {
    memcpy (head, &second_type, sizeof (wlh));
    head += sizeof (wlh);
  }

  {
    const u32 toread = total_len - (head - buf) - sizeof (u32);
    if (toread > 0)
    {
      bool iseof;
      WRAP (
          walis_read_all (w->istream, &iseof, NULL, checksum, head, toread, e)
      );
      if (iseof)
      {
        return WL_EOF;
      }
    }

    head += toread;
    bool iseof;
    WRAP (
        walis_read_all (w->istream, &iseof, NULL, NULL, head, sizeof (u32), e)
    );
    if (iseof)
    {
      return WL_EOF;
    }
  }

  u32 actual_crc;
  memcpy (&actual_crc, buf + total_len - sizeof (u32), sizeof (u32));
  if (*checksum != actual_crc)
  {
    return error_causef (e, ERR_CORRUPT, "Invalid CRC");
  }

  return SUCCESS;
}

static err_t
wal_read_physical_update (
    struct wal              *w,
    u32                     *checksum,
    struct wal_rec_hdr_read *r,
    error                   *e
)
{
  ASSERT (r->type == WL_UPDATE);
  u8        buf[WL_UPDATE_LEN];
  const int ret = wal_read_full (
      w,
      checksum,
      r->type,
      r->update.type,
      buf,
      WL_UPDATE_LEN,
      e
  );
  WRAP (ret);
  if (ret == WL_EOF)
  {
    r->type = WL_EOF;
    return SUCCESS;
  }
  ASSERT (ret == SUCCESS);
  walf_decode_physical_update (r, buf);
  return SUCCESS;
}

static err_t
wal_read_fsm_update (
    struct wal              *w,
    u32                     *checksum,
    struct wal_rec_hdr_read *r,
    error                   *e
)
{
  ASSERT (r->type == WL_UPDATE);
  u8        buf[WL_FSM_UPDATE_LEN];
  const int ret = wal_read_full (
      w,
      checksum,
      r->type,
      r->update.type,
      buf,
      WL_FSM_UPDATE_LEN,
      e
  );
  WRAP (ret);
  if (ret == WL_EOF)
  {
    r->type = WL_EOF;
    return SUCCESS;
  }
  ASSERT (ret == SUCCESS);
  walf_decode_fsm_update (r, buf);
  return SUCCESS;
}

static err_t
wal_read_file_extend_update (
    struct wal              *w,
    u32                     *checksum,
    struct wal_rec_hdr_read *r,
    error                   *e
)
{
  ASSERT (r->type == WL_UPDATE);
  u8        buf[WL_FILE_EXT_LEN];
  const int ret = wal_read_full (
      w,
      checksum,
      r->type,
      r->update.type,
      buf,
      WL_FILE_EXT_LEN,
      e
  );
  WRAP (ret);
  if (ret == WL_EOF)
  {
    r->type = WL_EOF;
    return SUCCESS;
  }
  ASSERT (ret == SUCCESS);
  walf_decode_file_extend_update (r, buf);
  return SUCCESS;
}

static err_t
wal_read_physical_clr (
    struct wal              *w,
    u32                     *checksum,
    struct wal_rec_hdr_read *r,
    error                   *e
)
{
  ASSERT (r->type == WL_CLR);
  u8        buf[WL_CLR_LEN];
  const int ret =
      wal_read_full (w, checksum, r->type, r->clr.type, buf, WL_CLR_LEN, e);
  WRAP (ret);
  if (ret == WL_EOF)
  {
    r->type = WL_EOF;
    return SUCCESS;
  }
  ASSERT (ret == SUCCESS);
  walf_decode_physical_clr (r, buf);
  return SUCCESS;
}

static err_t
wal_read_fsm_clr (
    struct wal              *w,
    u32                     *checksum,
    struct wal_rec_hdr_read *r,
    error                   *e
)
{
  ASSERT (r->type == WL_CLR);
  u8        buf[WL_FSM_CLR_LEN];
  const int ret =
      wal_read_full (w, checksum, r->type, r->clr.type, buf, WL_FSM_CLR_LEN, e);
  WRAP (ret);
  if (ret == WL_EOF)
  {
    r->type = WL_EOF;
    return SUCCESS;
  }
  ASSERT (ret == SUCCESS);
  walf_decode_fsm_clr (r, buf);
  return SUCCESS;
}

static err_t
wal_read_dummy_clr (
    struct wal              *w,
    u32                     *checksum,
    struct wal_rec_hdr_read *r,
    error                   *e
)
{
  ASSERT (r->type == WL_CLR);
  u8        buf[WL_DUMMY_CLR_LEN];
  const int ret = wal_read_full (
      w,
      checksum,
      r->type,
      r->clr.type,
      buf,
      WL_DUMMY_CLR_LEN,
      e
  );
  WRAP (ret);
  if (ret == WL_EOF)
  {
    r->type = WL_EOF;
    return SUCCESS;
  }
  ASSERT (ret == SUCCESS);
  walf_decode_dummy_clr (r, buf);
  return SUCCESS;
}

static err_t
wal_read_begin (
    struct wal              *w,
    u32                     *checksum,
    struct wal_rec_hdr_read *r,
    error                   *e
)
{
  ASSERT (r->type == WL_BEGIN);
  u8        buf[WL_BEGIN_LEN];
  const int ret =
      wal_read_full (w, checksum, r->type, WLH_NULL, buf, WL_BEGIN_LEN, e);
  WRAP (ret);
  if (ret == WL_EOF)
  {
    r->type = WL_EOF;
    return SUCCESS;
  }
  ASSERT (ret == SUCCESS);
  walf_decode_begin (r, buf);
  return SUCCESS;
}

static err_t
wal_read_commit (
    struct wal              *w,
    u32                     *checksum,
    struct wal_rec_hdr_read *r,
    error                   *e
)
{
  ASSERT (r->type == WL_COMMIT);
  u8        buf[WL_COMMIT_LEN];
  const int ret =
      wal_read_full (w, checksum, r->type, WLH_NULL, buf, WL_COMMIT_LEN, e);
  WRAP (ret);
  if (ret == WL_EOF)
  {
    r->type = WL_EOF;
    return SUCCESS;
  }
  ASSERT (ret == SUCCESS);
  walf_decode_commit (r, buf);
  return SUCCESS;
}

static err_t
wal_read_end (
    struct wal              *w,
    u32                     *checksum,
    struct wal_rec_hdr_read *r,
    error                   *e
)
{
  ASSERT (r->type == WL_END);
  u8        buf[WL_END_LEN];
  const int ret =
      wal_read_full (w, checksum, r->type, WLH_NULL, buf, WL_END_LEN, e);
  WRAP (ret);
  if (ret == WL_EOF)
  {
    r->type = WL_EOF;
    return SUCCESS;
  }
  ASSERT (ret == SUCCESS);
  walf_decode_end (r, buf);
  return SUCCESS;
}

static err_t
wal_read_sequential (
    struct wal              *w,
    struct wal_rec_hdr_read *dest,
    lsn                     *rlsn,
    error                   *e
)
{
  u32  checksum = checksum_init ();
  wlh  t;
  bool iseof;

  walis_mark_start_log (w->istream);

  WRAP (
      walis_read_all (w->istream, &iseof, rlsn, &checksum, &t, sizeof (t), e)
  );
  if (rlsn)
  {
    *rlsn += w->start_lsn;
  }
  if (iseof)
  {
    dest->type = WL_EOF;
    return SUCCESS;
  }

  dest->type = -1;

  switch (t)
  {
    case WL_UPDATE:
    {
      dest->type        = t;
      dest->update.type = -1;
      WRAP (walis_read_all (
          w->istream,
          &iseof,
          rlsn,
          &checksum,
          &t,
          sizeof (t),
          e
      ));
      if (rlsn)
      {
        *rlsn += w->start_lsn;
      }
      if (iseof)
      {
        dest->type = WL_EOF;
        return SUCCESS;
      }
      switch (t)
      {
        case WUP_PHYSICAL:
          dest->update.type = t;
          WRAP (wal_read_physical_update (w, &checksum, dest, e));
          break;
        case WUP_FEXT:
          dest->update.type = t;
          WRAP (wal_read_file_extend_update (w, &checksum, dest, e));
          break;
        case WUP_FSM:
          dest->update.type = t;
          WRAP (wal_read_fsm_update (w, &checksum, dest, e));
          break;
      }
      if ((int)dest->update.type == -1)
      {
        dest->type = -1;
      }
      break;
    }
    case WL_CLR:
    {
      dest->type     = t;
      dest->clr.type = -1;
      WRAP (walis_read_all (
          w->istream,
          &iseof,
          rlsn,
          &checksum,
          &t,
          sizeof (t),
          e
      ));
      if (rlsn)
      {
        *rlsn += w->start_lsn;
      }
      if (iseof)
      {
        dest->type = WL_EOF;
        return SUCCESS;
      }
      switch (t)
      {
        case WCLR_PHYSICAL:
          dest->clr.type = t;
          WRAP (wal_read_physical_clr (w, &checksum, dest, e));
          break;
        case WCLR_FSM:
          dest->clr.type = t;
          WRAP (wal_read_fsm_clr (w, &checksum, dest, e));
          break;
        case WCLR_DUMMY:
          dest->clr.type = t;
          WRAP (wal_read_dummy_clr (w, &checksum, dest, e));
          break;
      }
      if ((int)dest->clr.type == -1)
      {
        dest->type = -1;
      }
      break;
    }
    case WL_BEGIN:
    {
      dest->type = t;
      WRAP (wal_read_begin (w, &checksum, dest, e));
      break;
    }
    case WL_COMMIT:
    {
      dest->type = t;
      WRAP (wal_read_commit (w, &checksum, dest, e));
      break;
    }
    case WL_END:
    {
      dest->type = t;
      WRAP (wal_read_end (w, &checksum, dest, e));
      break;
    }
  }

  if ((int)dest->type == -1)
  {
    return error_causef (e, ERR_CORRUPT, "Invalid wal header type");
  }

  walis_mark_end_log (w->istream);

  return SUCCESS;
}

struct wal_rec_hdr_read *
wal_read_next (struct wal *w, lsn *rlsn, error *e)
{
  latch_lock (&w->latch);
  DBG_ASSERT (wal, w);

  ASSERT (w->istream);
  if (wal_read_sequential (w, &w->rhdr, rlsn, e))
  {
    latch_unlock (&w->latch);
    return NULL;
  }

  latch_unlock (&w->latch);
  return &w->rhdr;
}

struct wal_rec_hdr_read *
wal_read_first (struct wal *w, error *e)
{
  return wal_read_entry (w, sizeof (lsn), e);
}

struct wal_rec_hdr_read *
wal_read_entry (struct wal *w, const lsn id, error *e)
{
  latch_lock (&w->latch);
  DBG_ASSERT (wal, w);

  ASSERT (w->istream);
  if (id < w->start_lsn)
  {
    error_causef (
        e,
        ERR_CORRUPT,
        "Tried to read previous deleted log %" PRlsn " %" PRlsn,
        id,
        w->start_lsn
    );
    latch_unlock (&w->latch);
    return NULL;
  }

  if (walis_seek (w->istream, id - w->start_lsn, e))
  {
    latch_unlock (&w->latch);
    return NULL;
  }

  lsn rlsn;
  if (wal_read_sequential (w, &w->rhdr, &rlsn, e))
  {
    latch_unlock (&w->latch);
    return NULL;
  }

  latch_unlock (&w->latch);
  return &w->rhdr;
}

/******************************************************************************
 * SECTION: WAL Record Header
 ******************************************************************************/

void
wal_rec_hdr_read_random (struct wal_rec_hdr_read *dest)
{
  dest->type = randu32r (WL_BEGIN, WL_CLR);
  switch (dest->type)
  {
    case WL_BEGIN:
    {
      dest->begin.tid = randu32 ();
      break;
    }
    case WL_COMMIT:
    {
      dest->commit.tid  = randu32 ();
      dest->commit.prev = randu32 ();
      break;
    }
    case WL_END:
    {
      dest->end.tid  = randu32 ();
      dest->end.prev = randu32 ();
      break;
    }
    case WL_UPDATE:
    {
      dest->update.type = randu32r (WUP_PHYSICAL, WUP_FEXT);
      dest->update.tid  = randu32 ();
      dest->update.prev = randu32 ();
      switch (dest->update.type)
      {
        case WUP_PHYSICAL:
        {
          dest->update.phys.pg = randu32 ();
          rand_bytes (dest->update.phys.undo, NS_PAGE_SIZE);
          rand_bytes (dest->update.phys.redo, NS_PAGE_SIZE);
          break;
        }
        case WUP_FSM:
        {
          dest->update.fsm.pg   = randu32 ();
          dest->update.fsm.bit  = randu32 ();
          dest->update.fsm.undo = randu8 ();
          dest->update.fsm.redo = randu8 ();
          break;
        }
        case WUP_FEXT:
        {
          dest->update.fext.undo = randu32 ();
          dest->update.fext.redo = randu32 ();
          break;
        }
      }
      break;
    }
    case WL_CLR:
    {
      dest->clr.type      = randu32r (WCLR_PHYSICAL, WCLR_DUMMY);
      dest->clr.tid       = randu32 ();
      dest->clr.prev      = randu32 ();
      dest->clr.undo_next = randu32 ();
      switch (dest->clr.type)
      {
        case WCLR_PHYSICAL:
        {
          dest->clr.phys.pg = randu32 ();
          rand_bytes (dest->clr.phys.redo, NS_PAGE_SIZE);
          break;
        }
        case WCLR_FSM:
        {
          dest->clr.fsm.pg   = randu32 ();
          dest->clr.fsm.bit  = randu32 ();
          dest->clr.fsm.redo = randu8 ();
          break;
        }
        case WCLR_DUMMY:
        {
          break;
        }
      }
      break;
    }
    case WL_EOF:
    {
      ASSERT (false);
    }
  }
}

const char *
wal_rec_hdr_type_tostr (const enum wal_rec_hdr_type type)
{
  switch (type)
  {
    case WL_UPDATE:
    {
      return "WL_UPDATE";
    }
    case WL_CLR:
    {
      return "WL_CLR";
    }
    case WL_BEGIN:
    {
      return "WL_BEGIN";
    }
    case WL_COMMIT:
    {
      return "WL_COMMIT";
    }
    case WL_END:
    {
      return "WL_END";
    }
    case WL_EOF:
    {
      return "WL_EOF";
    }
  }

  UNREACHABLE (); // LCOV_EXCL_LINE
}

#ifdef TESTING
TEST (wal_rec_hdr_type_tostr)
{
  test_assert (wal_rec_hdr_type_tostr (WL_UPDATE) != NULL);
  test_assert (wal_rec_hdr_type_tostr (WL_CLR) != NULL);
  test_assert (wal_rec_hdr_type_tostr (WL_BEGIN) != NULL);
  test_assert (wal_rec_hdr_type_tostr (WL_COMMIT) != NULL);
  test_assert (wal_rec_hdr_type_tostr (WL_END) != NULL);
  test_assert (wal_rec_hdr_type_tostr (WL_EOF) != NULL);
}
#endif

struct wal_rec_hdr_write
wrhw_from_wrhr (struct wal_rec_hdr_read *src)
{
  switch (src->type)
  {
    case WL_BEGIN:
    {
      return (struct wal_rec_hdr_write){
          .type  = WL_BEGIN,
          .begin = src->begin,
      };
    }
    case WL_COMMIT:
    {
      return (struct wal_rec_hdr_write){
          .type   = WL_COMMIT,
          .commit = src->commit,
      };
    }
    case WL_END:
    {
      return (struct wal_rec_hdr_write){
          .type = WL_END,
          .end  = src->end,
      };
    }
    case WL_UPDATE:
    {
      switch (src->update.type)
      {
        case WUP_PHYSICAL:
        {
          return (struct wal_rec_hdr_write){
              .type   = WL_UPDATE,
              .update = {
                  .type = WUP_PHYSICAL,
                  .tid  = src->update.tid,
                  .prev = src->update.prev,
                  .phys = (struct physical_write_update){
                      .pg   = src->update.phys.pg,
                      .redo = src->update.phys.redo,
                      .undo = src->update.phys.undo,
                  },
              },
          };
        }
        case WUP_FSM:
        {
          return (struct wal_rec_hdr_write){
              .type   = WL_UPDATE,
              .update = {
                  .type = WUP_FSM,
                  .tid  = src->update.tid,
                  .prev = src->update.prev,
                  .fsm  = src->update.fsm,
              },
          };
        }
        case WUP_FEXT:
        {
          return (struct wal_rec_hdr_write){
              .type   = WL_UPDATE,
              .update = {
                  .type = WUP_FEXT,
                  .tid  = src->update.tid,
                  .prev = src->update.prev,
                  .fext = src->update.fext,
              },
          };
        }
      }
      break;
    }
    case WL_CLR:
    {
      switch (src->clr.type)
      {
        case WCLR_PHYSICAL:
        {
          return (struct wal_rec_hdr_write){
              .type = WL_CLR,
              .clr  = {
                  .type      = WCLR_PHYSICAL,
                  .tid       = src->clr.tid,
                  .prev      = src->clr.prev,
                  .undo_next = src->clr.undo_next,
                  .phys      = (struct physical_write_clr){
                      .pg   = src->clr.phys.pg,
                      .redo = src->clr.phys.redo,
                  },
              },
          };
        }
        case WCLR_FSM:
        {
          return (struct wal_rec_hdr_write){
              .type = WL_CLR,
              .clr  = {
                  .type      = WCLR_FSM,
                  .tid       = src->clr.tid,
                  .prev      = src->clr.prev,
                  .undo_next = src->clr.undo_next,
                  .fsm       = src->clr.fsm,
              },
          };
        }
        case WCLR_DUMMY:
        {
          return (struct wal_rec_hdr_write){
              .type = WL_CLR,
              .clr  = {
                  .type      = WCLR_DUMMY,
                  .tid       = src->clr.tid,
                  .prev      = src->clr.prev,
                  .undo_next = src->clr.undo_next,
              },
          };
        }
      }
      break;
    }
    case WL_EOF:
    {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
  UNREACHABLE (); // LCOV_EXCL_LINE
}

stxid
wrh_get_tid (const struct wal_rec_hdr_read *h)
{
  switch (h->type)
  {
    case WL_BEGIN:
    {
      return h->begin.tid;
    }
    case WL_COMMIT:
    {
      return h->commit.tid;
    }
    case WL_END:
    {
      return h->end.tid;
    }
    case WL_UPDATE:
    {
      return h->update.tid;
    }
    case WL_CLR:
    {
      return h->clr.tid;
    }
    case WL_EOF:
    {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
  UNREACHABLE (); // LCOV_EXCL_LINE
}

slsn
wrh_get_prev_lsn (const struct wal_rec_hdr_read *h)
{
  switch (h->type)
  {
    case WL_BEGIN:
    {
      return 0;
    }
    case WL_COMMIT:
    {
      return h->commit.prev;
    }
    case WL_END:
    {
      return h->end.prev;
    }
    case WL_UPDATE:
    {
      return h->update.prev;
    }
    case WL_CLR:
    {
      return h->clr.prev;
    }
    case WL_EOF:
    {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
  UNREACHABLE (); // LCOV_EXCL_LINE
}

bool
wrh_is_undoable (const struct wal_rec_hdr_read *h)
{
  switch (h->type)
  {
    case WL_BEGIN:
    {
      return false;
    }
    case WL_COMMIT:
    {
      return false;
    }
    case WL_END:
    {
      return false;
    }
    case WL_UPDATE:
    {
      switch (h->update.type)
      {
        case WUP_PHYSICAL:
        {
          return true;
        }
        case WUP_FSM:
        {
          return true;
        }
        case WUP_FEXT:
        {
          return false;
        }
      }
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_CLR:
    {
      return false;
    }
    case WL_EOF:
    {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
  UNREACHABLE (); // LCOV_EXCL_LINE
}

bool
wrh_is_redoable (const struct wal_rec_hdr_read *h)
{
  switch (h->type)
  {
    case WL_BEGIN:
    {
      return false;
    }
    case WL_COMMIT:
    {
      return false;
    }
    case WL_END:
    {
      return false;
    }
    case WL_UPDATE:
    {
      switch (h->update.type)
      {
        case WUP_PHYSICAL:
        {
          return true;
        }
        case WUP_FSM:
        {
          return true;
        }
        case WUP_FEXT:
        {
          return false;
        }
      }
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_CLR:
    {
      switch (h->clr.type)
      {
        case WCLR_PHYSICAL:
        {
          return true;
        }
        case WCLR_FSM:
        {
          return true;
        }
        case WCLR_DUMMY:
        {
          return false;
        }
      }
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_EOF:
    {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
  UNREACHABLE (); // LCOV_EXCL_LINE
}

pgno
wrh_get_affected_pg (const struct wal_rec_hdr_read *h)
{
  switch (h->type)
  {
    case WL_BEGIN:
    {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_COMMIT:
    {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_END:
    {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_UPDATE:
    {
      switch (h->update.type)
      {
        case WUP_PHYSICAL:
        {
          return h->update.phys.pg;
        }
        case WUP_FSM:
        {
          return h->update.fsm.pg;
        }
        case WUP_FEXT:
        {
          UNREACHABLE (); // LCOV_EXCL_LINE
        }
      }
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_CLR:
    {
      switch (h->clr.type)
      {
        case WCLR_PHYSICAL:
        {
          return h->clr.phys.pg;
        }
        case WCLR_FSM:
        {
          return h->clr.fsm.pg;
        }
        case WCLR_DUMMY:
        {
          UNREACHABLE (); // LCOV_EXCL_LINE
        }
      }
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_EOF:
    {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
  UNREACHABLE (); // LCOV_EXCL_LINE
}

#ifdef TESTING
bool
wal_rec_hdr_read_equal (
    const struct wal_rec_hdr_read *left,
    const struct wal_rec_hdr_read *right
)
{
  if (left->type != right->type)
  {
    return false;
  }

  bool match = true;

  switch (left->type)
  {
    case WL_UPDATE:
    {
      if (left->update.type != right->update.type)
      {
        return false;
      }

      match = match && left->update.tid == right->update.tid;
      match = match && left->update.prev == right->update.prev;

      switch (left->update.type)
      {
        case WUP_FSM:
        {
          match = match && left->update.fsm.pg == right->update.fsm.pg;
          match = match && left->update.fsm.undo == right->update.fsm.undo;
          match = match && left->update.fsm.redo == right->update.fsm.redo;
          break;
        }
        case WUP_PHYSICAL:
        {
          match = match && left->update.phys.pg == right->update.phys.pg;
          match = match
                  && memcmp (
                         left->update.phys.undo,
                         right->update.phys.undo,
                         NS_PAGE_SIZE
                     ) == 0;
          match = match
                  && memcmp (
                         left->update.phys.redo,
                         right->update.phys.redo,
                         NS_PAGE_SIZE
                     ) == 0;
          break;
        }
        case WUP_FEXT:
        {
          match = match && left->update.fext.undo == right->update.fext.undo;
          match = match && left->update.fext.redo == right->update.fext.redo;
          break;
        }
      }
      break;
    }

    case WL_CLR:
    {
      if (left->clr.type != right->clr.type)
      {
        return false;
      }

      match = match && left->clr.tid == right->clr.tid;
      match = match && left->clr.prev == right->clr.prev;
      match = match && left->clr.undo_next == right->clr.undo_next;

      switch (left->clr.type)
      {
        case WCLR_PHYSICAL:
        {
          match = match && left->clr.phys.pg == right->clr.phys.pg;
          match = match
                  && memcmp (
                         left->clr.phys.redo,
                         right->clr.phys.redo,
                         NS_PAGE_SIZE
                     ) == 0;
          break;
        }
        case WCLR_FSM:
        {
          match = match && left->clr.fsm.pg == right->clr.fsm.pg;
          match = match && left->clr.fsm.redo == right->clr.fsm.redo;
          break;
        }
        case WCLR_DUMMY:
        {
          break;
        }
      }

      break;
    }

    case WL_BEGIN:
    {
      match = match && left->begin.tid == right->begin.tid;
      break;
    }

    case WL_END:
    {
      match = match && left->end.tid == right->end.tid;
      match = match && left->end.prev == right->end.prev;
      break;
    }

    case WL_COMMIT:
    {
      match = match && left->commit.tid == right->commit.tid;
      match = match && left->commit.prev == right->commit.prev;
      break;
    }

    case WL_EOF:
    {
      return true;
    }
  }

  return match;
}
#endif

void
i_print_wal_rec_hdr_read_light (
    const int                      log_level,
    const struct wal_rec_hdr_read *r,
    const lsn                      l
)
{
  char        fields[128];
  const char *name = "?";
  const lsn  *prev = NULL;

  switch (r->type)
  {
    case WL_UPDATE:
      switch (r->update.type)
      {
        case WUP_PHYSICAL:
        {
          name = "UPDATE PHYS";
          snprintf (
              fields,
              sizeof fields,
              "txid = %8" PRtxid ", pg   = %8" PRpgno,
              r->update.tid,
              r->update.phys.pg
          );
          prev = &r->update.prev;
          break;
        }
        case WUP_FSM:
        {
          name = "UPDATE FSM";
          snprintf (
              fields,
              sizeof fields,
              "txid = %8" PRtxid ", pg   = %8" PRpgno
              ", undo = 0x%02x, redo = 0x%02x",
              r->update.tid,
              r->update.fsm.pg,
              (unsigned)r->update.fsm.undo,
              (unsigned)r->update.fsm.redo
          );
          prev = &r->update.prev;
          break;
        }
        case WUP_FEXT:
        {
          name = "UPDATE FEXT";
          snprintf (
              fields,
              sizeof fields,
              "txid = %8" PRtxid ", undo_pgs = %8" PRpgno
              ", redo_pgs = %8" PRpgno,
              r->update.tid,
              r->update.fext.undo,
              r->update.fext.redo
          );
          prev = &r->update.prev;
          break;
        }
      }
      break;

    case WL_CLR:
      switch (r->clr.type)
      {
        case WCLR_PHYSICAL:
        {
          name = "CLR PHYS";
          snprintf (
              fields,
              sizeof fields,
              "txid = %8" PRtxid ", pg   = %8" PRpgno ", undoNxt = %15" PRlsn,
              r->clr.tid,
              r->clr.phys.pg,
              r->clr.undo_next
          );
          prev = &r->clr.prev;
          break;
        }
        case WCLR_FSM:
        {
          name = "CLR FSM";
          snprintf (
              fields,
              sizeof fields,
              "txid = %8" PRtxid ", pg   = %8" PRpgno
              ", redo = 0x%02x, undoNxt = %15" PRlsn,
              r->clr.tid,
              r->clr.fsm.pg,
              (unsigned)r->clr.fsm.redo,
              r->clr.undo_next
          );
          prev = &r->clr.prev;
          break;
        }
        case WCLR_DUMMY:
        {
          name = "CLR DUMMY";
          snprintf (
              fields,
              sizeof fields,
              "txid = %8" PRtxid ", undoNxt = %15" PRlsn,
              r->clr.tid,
              r->clr.undo_next
          );
          prev = &r->clr.prev;
          break;
        }
      }
      break;

    case WL_BEGIN:
    {
      name = "BEGIN";
      snprintf (fields, sizeof fields, "txid = %8" PRtxid, r->begin.tid);
      break;
    }

    case WL_COMMIT:
    {
      name = "COMMIT";
      snprintf (fields, sizeof fields, "txid = %8" PRtxid, r->commit.tid);
      prev = &r->commit.prev;
      break;
    }

    case WL_END:
    {
      name = "END";
      snprintf (fields, sizeof fields, "txid = %8" PRtxid, r->end.tid);
      prev = &r->end.prev;
      break;
    }

    case WL_EOF:
    {
      i_printf (log_level, "%15" PRlsn "  WL_EOF\n", l);
      return;
    }
  }

  /* Widths set in one place:
       11 = strlen("UPDATE FEXT")  -- widest type name
       72 = widest fields line     -- "CLR FSM" case
     Bump them if a new record type pushes past these. */
  if (prev)
  {
    i_printf (
        log_level,
        "%15" PRlsn "  %-11s  [ %-72s ] --> %" PRlsn "\n",
        l,
        name,
        fields,
        *prev
    );
  }
  else
  {
    i_printf (log_level, "%15" PRlsn "  %-11s  [ %-72s ]\n", l, name, fields);
  }
}

struct wal_clr_write
wrh_undo (struct wal_rec_hdr_read *h, struct txn *tx, page_h *ph)
{
  switch (h->type)
  {
    case WL_BEGIN:
    {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_COMMIT:
    {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_END:
    {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_UPDATE:
    {
      switch (h->update.type)
      {
        case WUP_PHYSICAL:
        {
          memcpy (page_h_w (ph), h->update.phys.undo, NS_PAGE_SIZE);
          return (struct wal_clr_write){
              .type      = WCLR_PHYSICAL,
              .tid       = h->update.tid,
              .prev      = tx->data.last_lsn,
              .undo_next = h->update.prev,
              .phys      = {
                  .pg   = wrh_get_affected_pg (h),
                  .redo = h->update.phys.undo,
              },
          };
        }
        case WUP_FSM:
        {
          if (h->update.fsm.undo)
          {
            fsm_set_bit (page_h_w (ph), h->update.fsm.bit);
          }
          else
          {
            fsm_clr_bit (page_h_w (ph), h->update.fsm.bit);
          }
          return (struct wal_clr_write){
              .type      = WCLR_FSM,
              .tid       = h->update.tid,
              .prev      = tx->data.last_lsn,
              .undo_next = h->update.prev,
              .fsm       = {
                  .pg   = page_h_pgno (ph),
                  .bit  = h->update.fsm.bit,
                  .redo = h->update.fsm.undo,
              },
          };
        }
        case WUP_FEXT:
        {
          UNREACHABLE (); // LCOV_EXCL_LINE
        }
      }
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_CLR:
    {
      switch (h->clr.type)
      {
        case WCLR_PHYSICAL:
        {
          UNREACHABLE (); // LCOV_EXCL_LINE
        }
        case WCLR_FSM:
        {
          UNREACHABLE (); // LCOV_EXCL_LINE
        }
        case WCLR_DUMMY:
        {
          UNREACHABLE (); // LCOV_EXCL_LINE
        }
      }
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_EOF:
    {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
  UNREACHABLE (); // LCOV_EXCL_LINE
}

void
wrh_redo (struct wal_rec_hdr_read *h, page_h *ph)
{
  switch (h->type)
  {
    case WL_BEGIN:
    {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_COMMIT:
    {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_END:
    {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_UPDATE:
    {
      switch (h->update.type)
      {
        case WUP_PHYSICAL:
        {
          memcpy (page_h_w (ph)->raw, h->update.phys.redo, NS_PAGE_SIZE);
          return;
        }
        case WUP_FSM:
        {
          if (h->update.fsm.redo)
          {
            fsm_set_bit (page_h_w (ph), h->update.fsm.bit);
          }
          else
          {
            fsm_clr_bit (page_h_w (ph), h->update.fsm.bit);
          }
          return;
        }
        case WUP_FEXT:
        {
          UNREACHABLE (); // LCOV_EXCL_LINE
        }
      }
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_CLR:
    {
      switch (h->clr.type)
      {
        case WCLR_PHYSICAL:
        {
          memcpy (page_h_w (ph)->raw, h->clr.phys.redo, NS_PAGE_SIZE);
          return;
        }
        case WCLR_FSM:
        {
          if (h->clr.fsm.redo)
          {
            fsm_set_bit (page_h_w (ph), h->clr.fsm.bit);
          }
          else
          {
            fsm_clr_bit (page_h_w (ph), h->clr.fsm.bit);
          }
          return;
        }
        case WCLR_DUMMY:
        {
          UNREACHABLE (); // LCOV_EXCL_LINE
        }
      }
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_EOF:
    {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
  UNREACHABLE (); // LCOV_EXCL_LINE
}

/******************************************************************************
 * SECTION: WAL Decoding
 * ----------------------------------------------------------------------------
 * @brief Quick Decoding implementations for the WAL
 ******************************************************************************/

void
walf_decode_physical_update (
    struct wal_rec_hdr_read *r,
    const u8                 buf[WL_UPDATE_LEN]
)
{
  ASSERT (r->type == WL_UPDATE);

  u32 head = 2 * sizeof (wlh);

  // TID
  memcpy (&r->update.tid, buf + head, sizeof (r->update.tid));
  head += sizeof (r->update.tid);

  // PREV
  memcpy (&r->update.prev, buf + head, sizeof (r->update.prev));
  head += sizeof (r->update.prev);

  // PG
  memcpy (&r->update.phys.pg, buf + head, sizeof (r->update.phys.pg));
  head += sizeof (r->update.phys.pg);

  // UNDO
  memcpy (r->update.phys.undo, buf + head, NS_PAGE_SIZE);
  head += NS_PAGE_SIZE;

  // REDO
  memcpy (r->update.phys.redo, buf + head, NS_PAGE_SIZE);
}

void
walf_decode_fsm_update (
    struct wal_rec_hdr_read *r,
    const u8                 buf[WL_FSM_UPDATE_LEN]
)
{
  ASSERT (r->type == WL_UPDATE);
  ASSERT (r->update.type == WUP_FSM);

  u32 head = 2 * sizeof (wlh);

  // TID
  memcpy (&r->update.tid, buf + head, sizeof (r->update.tid));
  head += sizeof (r->update.tid);

  // PREV
  memcpy (&r->update.prev, buf + head, sizeof (r->update.prev));
  head += sizeof (r->update.prev);

  // PG
  memcpy (&r->update.fsm.pg, buf + head, sizeof (r->update.fsm.pg));
  head += sizeof (r->update.fsm.pg);

  // BIT
  memcpy (&r->update.fsm.bit, buf + head, sizeof (r->update.fsm.bit));
  head += sizeof (r->update.fsm.bit);

  // UNDO
  memcpy (&r->update.fsm.undo, buf + head, sizeof (r->update.fsm.undo));
  head += sizeof (r->update.fsm.undo);

  // REDO
  memcpy (&r->update.fsm.redo, buf + head, sizeof (r->update.fsm.redo));
}

void
walf_decode_file_extend_update (
    struct wal_rec_hdr_read *r,
    const u8                 buf[WL_FILE_EXT_LEN]
)
{
  ASSERT (r->type == WL_UPDATE);
  ASSERT (r->update.type == WUP_FEXT);

  u32 head = 2 * sizeof (wlh);

  // TID
  memcpy (&r->update.tid, buf + head, sizeof (r->update.tid));
  head += sizeof (r->update.tid);

  // PREV
  memcpy (&r->update.prev, buf + head, sizeof (r->update.prev));
  head += sizeof (r->update.prev);

  // UNDO
  memcpy (&r->update.fext.undo, buf + head, sizeof (r->update.fext.undo));
  head += sizeof (r->update.fext.undo);

  // REDO
  memcpy (&r->update.fext.redo, buf + head, sizeof (r->update.fext.redo));
}

void
walf_decode_physical_clr (struct wal_rec_hdr_read *r, const u8 buf[WL_CLR_LEN])
{
  ASSERT (r->type == WL_CLR);
  ASSERT (r->clr.type == WCLR_PHYSICAL);

  u32 head = 2 * sizeof (wlh);

  // TID
  memcpy (&r->clr.tid, buf + head, sizeof (r->clr.tid));
  head += sizeof (r->clr.tid);

  // PREV
  memcpy (&r->clr.prev, buf + head, sizeof (r->clr.prev));
  head += sizeof (r->clr.prev);

  // PG
  memcpy (&r->clr.phys.pg, buf + head, sizeof (r->clr.phys.pg));
  head += sizeof (r->clr.phys.pg);

  // UNDO_NEXT
  memcpy (&r->clr.undo_next, buf + head, sizeof (r->clr.undo_next));
  head += sizeof (r->clr.undo_next);

  // REDO
  memcpy (r->clr.phys.redo, buf + head, NS_PAGE_SIZE);
}

void
walf_decode_fsm_clr (struct wal_rec_hdr_read *r, const u8 buf[WL_FSM_CLR_LEN])
{
  ASSERT (r->type == WL_CLR);
  ASSERT (r->clr.type == WCLR_FSM);

  u32 head = 2 * sizeof (wlh);

  // TID
  memcpy (&r->clr.tid, buf + head, sizeof (r->clr.tid));
  head += sizeof (r->clr.tid);

  // PREV
  memcpy (&r->clr.prev, buf + head, sizeof (r->clr.prev));
  head += sizeof (r->clr.prev);

  // PG
  memcpy (&r->clr.fsm.pg, buf + head, sizeof (r->clr.fsm.pg));
  head += sizeof (r->clr.fsm.pg);

  // UNDO_NEXT
  memcpy (&r->clr.undo_next, buf + head, sizeof (r->clr.undo_next));
  head += sizeof (r->clr.undo_next);

  // BIT
  memcpy (&r->clr.fsm.bit, buf + head, sizeof (r->clr.fsm.bit));
  head += sizeof (r->clr.fsm.bit);

  // REDO
  memcpy (&r->clr.fsm.redo, buf + head, sizeof (r->clr.fsm.redo));
}

void
walf_decode_dummy_clr (
    struct wal_rec_hdr_read *r,
    const u8                 buf[WL_DUMMY_CLR_LEN]
)
{
  ASSERT (r->type == WL_CLR);
  ASSERT (r->clr.type == WCLR_DUMMY);

  u32 head = 2 * sizeof (wlh);

  // TID
  memcpy (&r->clr.tid, buf + head, sizeof (r->clr.tid));
  head += sizeof (r->clr.tid);

  // PREV
  memcpy (&r->clr.prev, buf + head, sizeof (r->clr.prev));
  head += sizeof (r->clr.prev);

  // UNDO_NEXT
  memcpy (&r->clr.undo_next, buf + head, sizeof (r->clr.undo_next));
  head += sizeof (r->clr.undo_next);
}

void
walf_decode_begin (struct wal_rec_hdr_read *r, const u8 buf[WL_BEGIN_LEN])
{
  ASSERT (r->type == WL_BEGIN);

  u32 head = sizeof (wlh);

  // TID
  memcpy (&r->begin.tid, buf + head, sizeof (r->begin.tid));
}

void
walf_decode_commit (struct wal_rec_hdr_read *r, const u8 buf[WL_COMMIT_LEN])
{
  ASSERT (r->type == WL_COMMIT);

  u32 head = sizeof (wlh);

  // TID
  memcpy (&r->commit.tid, buf + head, sizeof (r->commit.tid));
  head += sizeof (r->commit.tid);

  // PREV
  memcpy (&r->commit.prev, buf + head, sizeof (r->commit.prev));
}

void
walf_decode_end (struct wal_rec_hdr_read *r, const u8 buf[WL_END_LEN])
{
  ASSERT (r->type == WL_END);

  u32 head = sizeof (wlh);

  // TID
  memcpy (&r->end.tid, buf + head, sizeof (r->end.tid));
  head += sizeof (r->end.tid);

  // PREV
  memcpy (&r->end.prev, buf + head, sizeof (r->end.prev));
}

/******************************************************************************
 * SECTION: WAL Write Logic
 * ----------------------------------------------------------------------------
 * @brief Write WAL entries
 ******************************************************************************/

static err_t
wal_write_begin (
    const struct wal               *w,
    const struct wal_rec_hdr_write *r,
    error                          *e
)
{
  ASSERT (r->type == WL_BEGIN);

  ASSERT (w->ostream);

  u32       checksum = checksum_init ();
  const wlh t        = r->type;
  WRAP (walos_write_all (w->ostream, &checksum, &t, sizeof (wlh), e));
  WRAP (
      walos_write_all (w->ostream, &checksum, &r->begin.tid, sizeof (txid), e)
  );
  WRAP (walos_write_all (w->ostream, NULL, &checksum, sizeof (u32), e));

  return SUCCESS;
}

static err_t
wal_write_commit (
    const struct wal               *w,
    const struct wal_rec_hdr_write *r,
    error                          *e
)
{
  ASSERT (r->type == WL_COMMIT);

  ASSERT (w->ostream);

  u32       checksum = checksum_init ();
  const wlh t        = r->type;
  WRAP (walos_write_all (w->ostream, &checksum, &t, sizeof (wlh), e));
  WRAP (
      walos_write_all (w->ostream, &checksum, &r->commit.tid, sizeof (txid), e)
  );
  WRAP (
      walos_write_all (w->ostream, &checksum, &r->commit.prev, sizeof (lsn), e)
  );
  WRAP (walos_write_all (w->ostream, NULL, &checksum, sizeof (u32), e));

  return SUCCESS;
}

static err_t
wal_write_end (const struct wal *w, const struct wal_rec_hdr_write *r, error *e)
{
  ASSERT (r->type == WL_END);

  ASSERT (w->ostream);

  u32       checksum = checksum_init ();
  const wlh t        = r->type;
  WRAP (walos_write_all (w->ostream, &checksum, &t, sizeof (wlh), e));
  WRAP (walos_write_all (w->ostream, &checksum, &r->end.tid, sizeof (txid), e));
  WRAP (walos_write_all (w->ostream, &checksum, &r->end.prev, sizeof (lsn), e));
  WRAP (walos_write_all (w->ostream, NULL, &checksum, sizeof (u32), e));

  return SUCCESS;
}

static err_t
wal_write_physical_update (
    const struct wal               *w,
    const struct wal_rec_hdr_write *r,
    error                          *e
)
{
  ASSERT (w->ostream);

  u32       checksum = checksum_init ();
  const wlh t        = (wlh)r->type;
  const wlh ut       = (wlh)r->update.type;
  WRAP (walos_write_all (w->ostream, &checksum, &t, sizeof (wlh), e));
  WRAP (walos_write_all (w->ostream, &checksum, &ut, sizeof (wlh), e));
  WRAP (
      walos_write_all (w->ostream, &checksum, &r->update.tid, sizeof (txid), e)
  );
  WRAP (
      walos_write_all (w->ostream, &checksum, &r->update.prev, sizeof (lsn), e)
  );
  WRAP (walos_write_all (
      w->ostream,
      &checksum,
      &r->update.phys.pg,
      sizeof (pgno),
      e
  ));
  WRAP (walos_write_all (
      w->ostream,
      &checksum,
      r->update.phys.undo,
      NS_PAGE_SIZE,
      e
  ));
  WRAP (walos_write_all (
      w->ostream,
      &checksum,
      r->update.phys.redo,
      NS_PAGE_SIZE,
      e
  ));
  WRAP (walos_write_all (w->ostream, NULL, &checksum, sizeof (u32), e));

  return SUCCESS;
}

static err_t
wal_write_fsm_update (
    const struct wal               *w,
    const struct wal_rec_hdr_write *r,
    error                          *e
)
{
  ASSERT (w->ostream);

  u32       checksum = checksum_init ();
  const wlh t        = (wlh)r->type;
  const wlh ut       = (wlh)r->update.type;
  WRAP (walos_write_all (w->ostream, &checksum, &t, sizeof (wlh), e));
  WRAP (walos_write_all (w->ostream, &checksum, &ut, sizeof (wlh), e));
  WRAP (
      walos_write_all (w->ostream, &checksum, &r->update.tid, sizeof (txid), e)
  );
  WRAP (
      walos_write_all (w->ostream, &checksum, &r->update.prev, sizeof (lsn), e)
  );
  WRAP (walos_write_all (
      w->ostream,
      &checksum,
      &r->update.fsm.pg,
      sizeof (pgno),
      e
  ));
  WRAP (walos_write_all (
      w->ostream,
      &checksum,
      &r->update.fsm.bit,
      sizeof (p_size),
      e
  ));
  WRAP (walos_write_all (
      w->ostream,
      &checksum,
      &r->update.fsm.undo,
      sizeof (u8),
      e
  ));
  WRAP (walos_write_all (
      w->ostream,
      &checksum,
      &r->update.fsm.redo,
      sizeof (u8),
      e
  ));
  WRAP (walos_write_all (w->ostream, NULL, &checksum, sizeof (u32), e));

  return SUCCESS;
}

static err_t
wal_write_file_extend_update (
    const struct wal               *w,
    const struct wal_rec_hdr_write *r,
    error                          *e
)
{
  ASSERT (w->ostream);

  u32       checksum = checksum_init ();
  const wlh t        = (wlh)r->type;
  const wlh ut       = (wlh)r->update.type;
  WRAP (walos_write_all (w->ostream, &checksum, &t, sizeof (wlh), e));
  WRAP (walos_write_all (w->ostream, &checksum, &ut, sizeof (wlh), e));
  WRAP (
      walos_write_all (w->ostream, &checksum, &r->update.tid, sizeof (txid), e)
  );
  WRAP (
      walos_write_all (w->ostream, &checksum, &r->update.prev, sizeof (lsn), e)
  );
  WRAP (walos_write_all (
      w->ostream,
      &checksum,
      &r->update.fext.undo,
      sizeof (pgno),
      e
  ));
  WRAP (walos_write_all (
      w->ostream,
      &checksum,
      &r->update.fext.redo,
      sizeof (pgno),
      e
  ));
  WRAP (walos_write_all (w->ostream, NULL, &checksum, sizeof (u32), e));

  return SUCCESS;
}

static err_t
wal_write_physical_clr (
    const struct wal               *w,
    const struct wal_rec_hdr_write *r,
    error                          *e
)
{
  ASSERT (r->type == WL_CLR);

  ASSERT (w->ostream);

  u32       checksum = checksum_init ();
  const wlh t        = r->type;
  const wlh ut       = r->clr.type;
  WRAP (walos_write_all (w->ostream, &checksum, &t, sizeof (wlh), e));
  WRAP (walos_write_all (w->ostream, &checksum, &ut, sizeof (wlh), e));
  WRAP (walos_write_all (w->ostream, &checksum, &r->clr.tid, sizeof (txid), e));
  WRAP (walos_write_all (w->ostream, &checksum, &r->clr.prev, sizeof (lsn), e));
  WRAP (
      walos_write_all (w->ostream, &checksum, &r->clr.phys.pg, sizeof (pgno), e)
  );
  WRAP (walos_write_all (
      w->ostream,
      &checksum,
      &r->clr.undo_next,
      sizeof (lsn),
      e
  ));
  WRAP (
      walos_write_all (w->ostream, &checksum, r->clr.phys.redo, NS_PAGE_SIZE, e)
  );
  WRAP (walos_write_all (w->ostream, NULL, &checksum, sizeof (u32), e));

  return SUCCESS;
}

static err_t
wal_write_fsm_clr (
    const struct wal               *w,
    const struct wal_rec_hdr_write *r,
    error                          *e
)
{
  ASSERT (r->type == WL_CLR);

  ASSERT (w->ostream);

  u32       checksum = checksum_init ();
  const wlh t        = r->type;
  const wlh ut       = r->clr.type;
  WRAP (walos_write_all (w->ostream, &checksum, &t, sizeof (wlh), e));
  WRAP (walos_write_all (w->ostream, &checksum, &ut, sizeof (wlh), e));
  WRAP (walos_write_all (w->ostream, &checksum, &r->clr.tid, sizeof (txid), e));
  WRAP (walos_write_all (w->ostream, &checksum, &r->clr.prev, sizeof (lsn), e));
  WRAP (
      walos_write_all (w->ostream, &checksum, &r->clr.fsm.pg, sizeof (pgno), e)
  );
  WRAP (walos_write_all (
      w->ostream,
      &checksum,
      &r->clr.undo_next,
      sizeof (lsn),
      e
  ));
  WRAP (walos_write_all (
      w->ostream,
      &checksum,
      &r->clr.fsm.bit,
      sizeof (p_size),
      e
  ));
  WRAP (
      walos_write_all (w->ostream, &checksum, &r->clr.fsm.redo, sizeof (u8), e)
  );
  WRAP (walos_write_all (w->ostream, NULL, &checksum, sizeof (u32), e));

  return SUCCESS;
}

static err_t
wal_write_dummy_clr (
    const struct wal               *w,
    const struct wal_rec_hdr_write *r,
    error                          *e
)
{
  ASSERT (r->type == WL_CLR);

  ASSERT (w->ostream);

  u32       checksum = checksum_init ();
  const wlh t        = r->type;
  const wlh ut       = r->clr.type;
  WRAP (walos_write_all (w->ostream, &checksum, &t, sizeof (wlh), e));
  WRAP (walos_write_all (w->ostream, &checksum, &ut, sizeof (wlh), e));
  WRAP (walos_write_all (w->ostream, &checksum, &r->clr.tid, sizeof (txid), e));
  WRAP (walos_write_all (w->ostream, &checksum, &r->clr.prev, sizeof (lsn), e));
  WRAP (walos_write_all (
      w->ostream,
      &checksum,
      &r->clr.undo_next,
      sizeof (lsn),
      e
  ));
  WRAP (walos_write_all (w->ostream, NULL, &checksum, sizeof (u32), e));

  return SUCCESS;
}

slsn
wal_write_locked (struct wal *w, error *e)
{
  ASSERT (w->ostream);
  ASSERT (!(w->flags & WAL_ISNEW));

  const lsn ret = walos_get_next_lsn (w->ostream) + w->start_lsn;

  switch (w->whdr.type)
  {
    case WL_BEGIN:
    {
      WRAP (wal_write_begin (w, &w->whdr, e));
      break;
    }
    case WL_COMMIT:
    {
      WRAP (wal_write_commit (w, &w->whdr, e));
      break;
    }
    case WL_END:
    {
      WRAP (wal_write_end (w, &w->whdr, e));
      break;
    }
    case WL_UPDATE:
    {
      switch (w->whdr.update.type)
      {
        case WUP_PHYSICAL:
        {
          WRAP (wal_write_physical_update (w, &w->whdr, e));
          break;
        }
        case WUP_FSM:
        {
          WRAP (wal_write_fsm_update (w, &w->whdr, e));
          break;
        }
        case WUP_FEXT:
        {
          WRAP (wal_write_file_extend_update (w, &w->whdr, e));
          break;
        }
      }
      break;
    }
    case WL_CLR:
    {
      switch (w->whdr.clr.type)
      {
        case WCLR_PHYSICAL:
        {
          WRAP (wal_write_physical_clr (w, &w->whdr, e));
          break;
        }
        case WCLR_FSM:
        {
          WRAP (wal_write_fsm_clr (w, &w->whdr, e));
          break;
        }
        case WCLR_DUMMY:
        {
          WRAP (wal_write_dummy_clr (w, &w->whdr, e));
          break;
        }
      }
      break;
    }
    case WL_EOF:
    {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }

  return ret;
}

#ifdef TESTING

/**
 * @struct wal_queue
 * @brief Parameters for the wal_multi_threaded test
 *
 * @var wal_queue::sync
 * @brief The "start" signal so all threads start at the same time
 *
 * @var wal_queue::ww
 * @brief The shared wal writer
 *
 * @var wal_queue::idx
 * @brief The next log to write - this is atomically added to for each writer
 *
 * @var wal_queue::read
 * @brief The list of log records to write (as read to make it easier to build)
 *
 * @var wal_queue::len
 * @brief Length of [read]
 */
struct wal_queue
{
  _Atomic u32 sync;
  struct wal *ww;
  atomic_int  idx;

  struct wal_rec_hdr_read *read;
  const int                len;
};

static void *
wal_thread (void *ctx)
{
  error             e = error_create ();
  struct wal_queue *q = ctx;

  while (atomic_load (&q->sync) > 0)
  {
    spin_pause ();
  }

  while (true)
  {
    const int idx = atomic_fetch_add (&q->idx, 1);
    if (idx >= q->len)
    {
      return NULL;
    }

    struct wal_rec_hdr_write write = wrhw_from_wrhr (&q->read[idx]);

    const slsn l = wal_append_log (q->ww, &write, &e);
    if (l < 0)
    {
      panic ("Failed to write log");
    }

    if (wal_flush_all (q->ww, &e))
    {
      panic ("Failed to flush wal");
    }
  }
}

TEST (wal_multi_threaded)
{
  error e = error_create ();
  i_remove_quiet ("test.wal", &e);
  struct wal *ww = wal_open ("test.wal", &e);
  wal_write_start_lsn (ww, 0, &e);

  const u32 N = 5000;

  struct wal_rec_hdr_read *read = i_malloc (N, sizeof *read, &e);

  for (u32 i = 0; i < N; ++i)
  {
    wal_rec_hdr_read_random (&read[i]);
  }

  struct wal_queue ctx = {
      .sync = 1,
      .ww   = ww,
      .idx  = 0,
      .read = read,
      .len  = N,
  };

  u32      nthreads;
  i_thread threads[10];
  for (nthreads = 0; nthreads < arrlen (threads); ++nthreads)
  {
    i_thread_create (&threads[nthreads], wal_thread, &ctx, &e);
  }

  // launch
  atomic_store (&ctx.sync, 0);

  i_log_info ("Threads active\n");

  for (; nthreads > 0; --nthreads)
  {
    i_thread_join (&threads[nthreads - 1], &e);
  }

  // To speed up searches, keep a "finger" which is "near" the
  // most recent found log
  u32 finger   = 0;
  lsn read_lsn = 0;

  for (u32 i = 0; i < N; ++i)
  {
    struct wal_rec_hdr_read *actual = wal_read_next (ww, &read_lsn, &e);
    i_print_wal_rec_hdr_read_light (LOG_INFO, actual, read_lsn);
    test_assert (actual->type != WL_EOF);

    // Search through all records to ensure it's there
    bool found = false;
    for (u32 k = 0; k < N; ++k)
    {
      const u32 idx = (finger + k) % N;
      if (wal_rec_hdr_read_equal (actual, &read[idx]))
      {
        finger         = (idx + 1) % N;
        read[idx].type = WL_EOF;
        found          = true;
        break;
      }
    }
    test_assert (found);
  }

  const struct wal_rec_hdr_read *actual = wal_read_next (ww, &read_lsn, &e);
  test_assert_int_equal (actual->type, WL_EOF);

  wal_close (ww, &e);
  i_free (read);
}

struct wal_test_params
{
  const char              *fname;
  struct wal_rec_hdr_read *batch1;
  u32                      batch1_len;
  struct wal_rec_hdr_read *batch2;
  u32                      batch2_len;
};

static void
wal_test_fill_batch (struct wal_rec_hdr_read *batch, const u32 len, error *e)
{
  for (u32 i = 0; i < len; i++)
  {
    struct wal_rec_hdr_read *r = &batch[i];

    switch (r->type)
    {
      case WL_UPDATE:
      {
        rand_bytes (r->update.phys.undo, NS_PAGE_SIZE);
        rand_bytes (r->update.phys.redo, NS_PAGE_SIZE);
        break;
      }
      case WL_CLR:
      {
        rand_bytes (r->clr.phys.redo, NS_PAGE_SIZE);
        break;
      }
      default:
      {
        break;
      }
    }
  }
}

static void
wal_test_free_batch (const struct wal_rec_hdr_read *batch, const u32 len)
{
  for (u32 i = 0; i < len; i++)
  {
    const struct wal_rec_hdr_read *r = &batch[i];
  }
}

static void
run_wal_test (const struct wal_test_params *p)
{
  error e = error_create ();

  i_remove_quiet (p->fname, &e);
  struct wal *ww = wal_open (p->fname, &e);
  wal_write_start_lsn (ww, 0, &e);
  /**
   * Write all the input logs
   */
  {
    slsn l = -1;
    for (u32 i = 0; i < p->batch1_len; i++)
    {
      struct wal_rec_hdr_write out   = wrhw_from_wrhr (&p->batch1[i]);
      slsn                     nextl = wal_append_log (ww, &out, &e);
      test_assert (nextl >= 0);
      test_assert (nextl > l);
      l = nextl;
    }
    wal_flush_all (ww, &e);
  }

  /**
   * Read all the input logs and expect
   * that they are the same as the
   * first batch written ones
   */
  {
    for (u32 i = 0; i < p->batch1_len; i++)
    {
      lsn                      read_lsn;
      struct wal_rec_hdr_read *next = NULL;
      if (i == 0)
      {
        next = wal_read_first (ww, &e);
      }
      else
      {
        next = wal_read_next (ww, &read_lsn, &e);
      }
      test_assert (wal_rec_hdr_read_equal (next, &p->batch1[i]));
    }
  }

  /**
   * Write a second batch of input logs
   */
  {
    slsn l = 0;
    for (u32 i = 0; i < p->batch2_len; i++)
    {
      struct wal_rec_hdr_write out = wrhw_from_wrhr (&p->batch2[i]);
      l                            = wal_append_log (ww, &out, &e);
    }
    wal_flush_all (ww, &e);
  }

  /**
   * Read from the start and confirm all the logs
   */
  {
    for (u32 i = 0; i < p->batch1_len; i++)
    {
      lsn                      read_lsn;
      struct wal_rec_hdr_read *next = NULL;
      if (i == 0)
      {
        next = wal_read_first (ww, &e);
      }
      else
      {
        next = wal_read_next (ww, &read_lsn, &e);
      }
      test_assert (wal_rec_hdr_read_equal (next, &p->batch1[i]));
    }

    for (u32 i = 0; i < p->batch2_len; i++)
    {
      lsn                      read_lsn;
      struct wal_rec_hdr_read *next = wal_read_next (ww, &read_lsn, &e);
      test_assert (wal_rec_hdr_read_equal (next, &p->batch2[i]));
    }
  }

  wal_close (ww, &e);
}

////////////////////////////////////////////////////////////
// WAL test cases

TEST (wal)
{
  error e = error_create ();

  struct wal_rec_hdr_read batch1_full[] = {
      {.type = WL_BEGIN, .begin = {.tid = 1}},
      {.type = WL_COMMIT, .commit = {.tid = 3, .prev = 20}},
      {.type = WL_END, .end = {.tid = 4, .prev = 30}},
      {
          .type = WL_UPDATE,
          .update =
              {
                  .type = WUP_PHYSICAL,
                  .tid  = 5,
                  .prev = 40,
                  .phys = {.pg = 111},
              },
      },
      {
          .type = WL_CLR,
          .clr  = {
              .type      = WCLR_PHYSICAL,
              .tid       = 6,
              .prev      = 50,
              .undo_next = 42,
              .phys      = {.pg = 222},
          },
      },
  };

  struct wal_rec_hdr_read batch2_full[] = {
      {.type = WL_BEGIN, .begin = {.tid = 2}},
      {
          .type   = WL_UPDATE,
          .update = {
              .type = WUP_PHYSICAL,
              .tid  = 6,
              .prev = 41,
              .phys = {.pg = 112},
          },
      },
  };

  struct wal_rec_hdr_read batch1_begin_only[] = {
      {.type = WL_BEGIN, .begin = {.tid = 1}},
  };

  struct wal_rec_hdr_read batch2_begin_only[] = {
      {.type = WL_BEGIN, .begin = {.tid = 2}},
  };

  struct wal_rec_hdr_read batch1_no_ckpt[] = {
      {.type = WL_BEGIN, .begin = {.tid = 1}},
      {.type = WL_COMMIT, .commit = {.tid = 3, .prev = 20}},
      {.type = WL_END, .end = {.tid = 4, .prev = 30}},
      {
          .type = WL_UPDATE,
          .update =
              {
                  .type = WUP_PHYSICAL,
                  .tid  = 5,
                  .prev = 40,
                  .phys = {.pg = 111},
              },
      },
      {
          .type = WL_CLR,
          .clr  = {
              .type      = WCLR_PHYSICAL,
              .tid       = 6,
              .prev      = 50,
              .undo_next = 42,
              .phys      = {.pg = 222},
          },
      },
  };

  struct wal_test_params cases[] = {
      {
          .fname      = "test_full.wal",
          .batch1     = batch1_full,
          .batch1_len = arrlen (batch1_full),
          .batch2     = batch2_full,
          .batch2_len = arrlen (batch2_full),
      },
      {
          .fname      = "test_begin_only.wal",
          .batch1     = batch1_begin_only,
          .batch1_len = arrlen (batch1_begin_only),
          .batch2     = batch2_begin_only,
          .batch2_len = arrlen (batch2_begin_only),
      },
      {
          .fname      = "test_no_ckpt.wal",
          .batch1     = batch1_no_ckpt,
          .batch1_len = arrlen (batch1_no_ckpt),
          .batch2     = batch2_full,
          .batch2_len = arrlen (batch2_full),
      },
  };

  for (u32 i = 0; i < arrlen (cases); i++)
  {
    TEST_CASE ("Wal: %d", i)
    {
      const struct wal_test_params *c = &cases[i];

      wal_test_fill_batch (c->batch1, c->batch1_len, &e);
      wal_test_fill_batch (c->batch2, c->batch2_len, &e);

      run_wal_test (c);

      wal_test_free_batch (c->batch1, c->batch1_len);
      wal_test_free_batch (c->batch2, c->batch2_len);
    }
  }
}

TEST (wal_single_entry)
{
  error e = error_create ();

  struct wal_rec_hdr_read cases[] = {
      {.type = WL_BEGIN, .begin = {.tid = 1}},
      {.type = WL_COMMIT, .commit = {.tid = 2, .prev = 10}},
      {.type = WL_END, .end = {.tid = 3, .prev = 20}},
      {.type = WL_UPDATE,
       .update =
           {.type = WUP_PHYSICAL, .tid = 4, .prev = 30, .phys = {.pg = 111}}},
      {.type = WL_CLR,
       .clr  = {
           .type      = WCLR_PHYSICAL,
           .tid       = 5,
           .prev      = 40,
           .undo_next = 42,
           .phys      = {.pg = 222}
       }},
  };

  for (u32 i = 0; i < arrlen (cases); i++)
  {
    TEST_CASE ("wal_single_entry: %d", i)
    {
      struct wal_rec_hdr_read *c = &cases[i];

      wal_test_fill_batch (c, 1, &e);

      i_remove_quiet ("test_single_entry.wal", &e);
      struct wal *ww = wal_open ("test_single_entry.wal", &e);
      wal_write_start_lsn (ww, 0, &e);

      // WRITE
      struct wal_rec_hdr_write out = wrhw_from_wrhr (c);
      const slsn               l   = wal_append_log (ww, &out, &e);
      test_assert (l >= 0);

      wal_flush_all (ww, &e);

      // READ
      struct wal_rec_hdr_read *next = wal_read_first (ww, &e);
      test_assert (wal_rec_hdr_read_equal (next, c));

      wal_close (ww, &e);

      wal_test_free_batch (c, 1);
    }
  }
}

#endif
