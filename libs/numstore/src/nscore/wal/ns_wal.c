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

#include "nscore/wal/ns_wal.h"

#include "core/ns_cbuffer.h"
#include "core/ns_crash.h"
#include "core/ns_crc.h"
#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_latch.h"
#include "core/ns_stdtypes.h"
#include "core/ns_string.h"
#include "core/ns_utils.h"
#include "core/os/ns_malloc.h"
#include "core/os/ns_os.h"
#include "nscore/wal/ns_wal_istream.h"
#include "nscore/wal/ns_wal_ostream.h"
#include "nscore/wal/ns_wal_record.h"

#ifndef NDEBUG
#  include "core/ns_testing.h"
#endif

#include <stdatomic.h>
#include <string.h>

static err_t
wal_init (struct wal *dest, error *e)
{
  dest->ostream   = NULL;
  dest->istream   = NULL;
  dest->flags     = 0;
  dest->start_lsn = 0;
  latch_init (&dest->l);

  dest->istream = walis_open (dest->fname.data, dest->mem, dest->os, e);
  if (dest->istream == NULL) {
    return error_trace (e);
  }

  // Read the start lsn
  {
    lsn start_lsn = 0;

    lsn start     = walis_mark (dest->istream);
    ASSERT (start == 0);
    int iseof = walis_read_all (dest->istream, NULL, &start_lsn, sizeof (start_lsn), e);

    // Failed
    if (iseof < 0) {
      goto err_walis_close;
    }

    // Eof - this is a fresh wal
    if (iseof) {
      // We can open ostream because we know the wal is fresh
      dest->ostream = walos_open (dest->fname.data, dest->mem, dest->os, 0, e);
      if (dest->ostream == NULL) {
        goto err_walis_close;
      }
      dest->flags |= WAL_ISNEW;

    } else {
      // Keep the ostream closed - we can't open it until we do a full WAL scan
      start = walis_mark (dest->istream);
      ASSERT (start == sizeof (start_lsn));
      dest->start_lsn = start_lsn;
    }
  }

  DBG_ASSERT (wal, dest);

  return error_trace (e);

err_walis_close:
  walis_close (dest->istream, e);
  dest->istream = NULL;
  return error_trace (e);
}

err_t
wal_write_start_lsn (struct wal *w, lsn start_lsn, error *e)
{
  DBG_ASSERT (wal, w);
  ASSERT (w->flags & WAL_ISNEW);

  // Only call write_start_lsn on a fresh wal - start lsn's are
  // sequential and never re written - so this is a fresh wal
  ASSERT (w->ostream);
  ASSERT (walos_get_lsn (w->ostream) == 0);

  walos_write_all (w->ostream, NULL, &start_lsn, sizeof (start_lsn));
  walos_flush_all (w->ostream);

  w->start_lsn = start_lsn;
  w->flags &= ~WAL_ISNEW;

  // Advance istream forward one lsn worth
  {
    // We should only be at position 0 calling this
    ASSERT (walis_mark (w->istream) == 0);

    lsn read_start_lsn;
    if (walis_read_all (w->istream, NULL, &read_start_lsn, sizeof (start_lsn), e) < 0) {
      return error_trace (e);
    }

    // Verify we can read what we just wrote
    if (start_lsn != read_start_lsn) {
      return error_causef (e, ERR_CORRUPT, "Failed to read start lsn just written");
    }

    // We should now be at the start of the records now
    ASSERT (walis_mark (w->istream) == WAL_RECORDS_POS);
  }

  return SUCCESS;
}

slsn
wal_get_end_lsn (struct wal *w, error *e)
{
  DBG_ASSERT (wal, w);
  if (wal_forward_scan (w, LSN_NULL, NULL, NULL, e) < 0) {
    return error_trace (e);
  }

  ASSERT (w->ostream);

  const lsn end = w->start_lsn + walos_get_lsn (w->ostream);
  ASSERT (end <= INT64_MAX);
  return (slsn)end;
}

struct wal *
wal_open (const char *fname, struct i_mem mem, struct i_os os, error *e)
{
  struct wal *dest = mem.table->malloc (mem.self, 1, sizeof *dest, e);
  if (dest == NULL) {
    return NULL;
  }

  dest->mem = mem;
  dest->os  = os;

  if (string_copy (&dest->fname, strfcstr (fname), mem, e) < 0) {
    goto err_free_dest;
  }

  if (wal_init (dest, e) < 0) {
    goto err_free_fname;
  }

  return dest;

err_free_fname:
  mem.table->free (mem.self, (char *)dest->fname.data);
err_free_dest:
  mem.table->free (mem.self, dest);
  return NULL;
}

static inline err_t
wal_destroy (struct wal *w, error *e)
{
  if (w->ostream != NULL) {
    // No wal_flush_all here. walos_close flushes on its way out and reports
    // what it could not write - a flush of its own would crash instead, and a
    // close has somewhere to put the error.
    walos_close (w->ostream, e);
    w->ostream = NULL;
  }
  if (w->istream != NULL) {
    walis_close (w->istream, e);
    w->istream = NULL;
  }

  if (w->fname.data != NULL) {
    w->mem.table->free (w->mem.self, (void *)w->fname.data);
    w->fname.data = NULL;
  }
  return error_trace (e);
}

err_t
wal_close (struct wal *w, error *e)
{
  DBG_ASSERT (wal, w);
  struct i_mem mem = w->mem;
  wal_destroy (w, e);
  mem.table->free (mem.self, w);
  return error_trace (e);
}

err_t
wal_close_and_delete (struct wal *w, error *e)
{
  DBG_ASSERT (wal, w);
  struct i_mem  mem   = w->mem;
  struct i_os   os    = w->os;
  struct string fname = w->fname;
  w->fname.data       = NULL;

  wal_destroy (w, e);
  mem.table->free (mem.self, w);

  os.table->remove_quiet (os.self, fname.data, e);
  mem.table->free (mem.self, (char *)fname.data);

  return error_trace (e);
}

err_t
wal_delete_and_reopen (struct wal *w, error *e)
{
  latch_lock (&w->l);
  DBG_ASSERT (wal, w);

  struct string fname = w->fname;

  // Set to NULL so we don't free it
  w->fname.data       = NULL;

  if (wal_destroy (w, e) < 0) {
    // Hand the name back so wal_close frees it
    w->fname = fname;
    latch_unlock (&w->l);
    return error_trace (e);
  }

  if (w->os.table->remove_quiet (w->os.self, fname.data, e) < 0) {
    w->fname = fname;
    latch_unlock (&w->l);
    return error_trace (e);
  }

  w->fname = fname;

  return wal_init (w, e);
}

err_t
wal_crash (struct wal *w, error *e)
{
  DBG_ASSERT (wal, w);

  struct i_mem mem = w->mem;

  if (w->ostream != NULL) {
    walos_crash (w->ostream, e);
  }
  if (w->istream != NULL) {
    walis_crash (w->istream, e);
  }
  if (w->fname.data != NULL) {
    mem.table->free (mem.self, (void *)w->fname.data);
  }
  mem.table->free (mem.self, w);

  return SUCCESS;
}

#ifndef NDEBUG
void
wal_ensure_flush_next (struct wal *w)
{
  DBG_ASSERT (wal, w);

  // Zero padding in small chunks - a WAL_BUFFER_CAP array overflows the 1 MiB
  // default stack on Windows
  u8  zeros[4096] = {0};
  u32 len         = cbuffer_len (&w->ostream->buffer);
  ASSERT (len <= WAL_BUFFER_CAP);

  u32 towrite = WAL_BUFFER_CAP - len;
  while (towrite > 0) {
    const u32 n = MIN (towrite, (u32)sizeof (zeros));
    walos_write_all (w->ostream, NULL, zeros, n);
    towrite -= n;
  }
}
#endif

#undef GUARD_EOF
#define GUARD_EOF(expr)       \
  do {                        \
    int iseof = (expr);       \
    if (iseof < 0 || iseof) { \
      return iseof;           \
    }                         \
  }                           \
  while (0)

static int
wal_read_full (struct wal *w, u32 *checksum, wlh type1, wlh type2, u8 *buf, u32 total_len, error *e)
{
  ASSERT (total_len >= sizeof (wlh) + sizeof (u32));

  u8 *head = buf;

  /**
   * ____ _____ _________________________ ____
   * ^
   * buf
   * head
   */
  head += memcpy_inc (head, &type1, sizeof (wlh));

  /**
   * ____ _____ _________________________ ____
   * ^    ^
   * buf  head
   */
  if (type2 != WLH_NULL) {
    head += memcpy_inc (head, &type2, sizeof (wlh));
  }

  // Do main body read
  {
    /**
     * ____ _____ _________________________ ____
     * ^          ^
     * buf       head
     *           |         toread          |
     */
    ASSERT ((u64)(head - buf) + sizeof (u32) <= total_len);
    u32 toread = total_len - (u32)(head - buf) - (u32)sizeof (u32);
    if (toread > 0) {
      // Do a 1 time read of the rest of the record
      GUARD_EOF (walis_read_all (w->istream, checksum, head, toread, e));
    }
    head += toread;

    /**
     * ____ _____ _________________________ ____
     * ^                                    ^
     * buf                                 head
     */
    GUARD_EOF (walis_read_all (w->istream, NULL, head, sizeof (u32), e));
  }

  // Compare checksums
  u32 actual_crc;
  memcpy (&actual_crc, buf + total_len - sizeof (u32), sizeof (u32));
  if (*checksum != actual_crc) {
    return error_causef (
        e,
        ERR_CORRUPT,
        "Invalid CRC: %" PRIu32 " != %" PRIu32,
        *checksum,
        actual_crc
    );
  }

  return WAL_NOT_EOF;
}

static int
wal_read_physical_update (struct wal *w, u32 *checksum, struct wal_rec_hdr_read *dest, error *e)
{
  ASSERT (dest->type == WL_UPDATE);
  u8 buf[WL_UPDATE_LEN];
  GUARD_EOF (wal_read_full (w, checksum, dest->type, dest->update.type, buf, WL_UPDATE_LEN, e));
  walf_decode_physical_update (dest, buf);
  return WAL_NOT_EOF;
}

static int
wal_read_fsm_update (struct wal *w, u32 *checksum, struct wal_rec_hdr_read *dest, error *e)
{
  ASSERT (dest->type == WL_UPDATE);
  u8 buf[WL_FSM_UPDATE_LEN];
  GUARD_EOF (wal_read_full (w, checksum, dest->type, dest->update.type, buf, WL_FSM_UPDATE_LEN, e));
  walf_decode_fsm_update (dest, buf);
  return WAL_NOT_EOF;
}

static int
wal_read_file_extend_update (struct wal *w, u32 *checksum, struct wal_rec_hdr_read *dest, error *e)
{
  ASSERT (dest->type == WL_UPDATE);
  u8 buf[WL_FILE_EXT_LEN];
  GUARD_EOF (wal_read_full (w, checksum, dest->type, dest->update.type, buf, WL_FILE_EXT_LEN, e));
  walf_decode_file_extend_update (dest, buf);
  return WAL_NOT_EOF;
}

static int
wal_read_physical_clr (struct wal *w, u32 *checksum, struct wal_rec_hdr_read *dest, error *e)
{
  ASSERT (dest->type == WL_CLR);
  u8 buf[WL_CLR_LEN];
  GUARD_EOF (wal_read_full (w, checksum, dest->type, dest->clr.type, buf, WL_CLR_LEN, e));
  walf_decode_physical_clr (dest, buf);
  return WAL_NOT_EOF;
}

static int
wal_read_fsm_clr (struct wal *w, u32 *checksum, struct wal_rec_hdr_read *dest, error *e)
{
  ASSERT (dest->type == WL_CLR);
  u8 buf[WL_FSM_CLR_LEN];
  GUARD_EOF (wal_read_full (w, checksum, dest->type, dest->clr.type, buf, WL_FSM_CLR_LEN, e));
  walf_decode_fsm_clr (dest, buf);
  return WAL_NOT_EOF;
}

static int
wal_read_dummy_clr (struct wal *w, u32 *checksum, struct wal_rec_hdr_read *dest, error *e)
{
  ASSERT (dest->type == WL_CLR);
  u8 buf[WL_DUMMY_CLR_LEN];
  GUARD_EOF (wal_read_full (w, checksum, dest->type, dest->clr.type, buf, WL_DUMMY_CLR_LEN, e));
  walf_decode_dummy_clr (dest, buf);
  return WAL_NOT_EOF;
}

static int
wal_read_begin (struct wal *w, u32 *checksum, struct wal_rec_hdr_read *dest, error *e)
{
  ASSERT (dest->type == WL_BEGIN);
  u8 buf[WL_BEGIN_LEN];
  GUARD_EOF (wal_read_full (w, checksum, dest->type, WLH_NULL, buf, WL_BEGIN_LEN, e));
  walf_decode_begin (dest, buf);
  return WAL_NOT_EOF;
}

static int
wal_read_commit (struct wal *w, u32 *checksum, struct wal_rec_hdr_read *dest, error *e)
{
  ASSERT (dest->type == WL_COMMIT);
  u8 buf[WL_COMMIT_LEN];
  GUARD_EOF (wal_read_full (w, checksum, dest->type, WLH_NULL, buf, WL_COMMIT_LEN, e));
  walf_decode_commit (dest, buf);
  return WAL_NOT_EOF;
}

static int
wal_read_end (struct wal *w, u32 *checksum, struct wal_rec_hdr_read *dest, error *e)
{
  ASSERT (dest->type == WL_END);
  u8 buf[WL_END_LEN];
  GUARD_EOF (wal_read_full (w, checksum, dest->type, WLH_NULL, buf, WL_END_LEN, e));
  walf_decode_end (dest, buf);
  return WAL_NOT_EOF;
}

static int
wal_read_one (struct wal *w, struct wal_rec_hdr_read *dest, error *e)
{
  u32 checksum = checksum_init ();

  // Read the header
  GUARD_EOF (walis_read_all (w->istream, &checksum, &dest->type, sizeof (wlh), e));

  if (dest->type == WL_TERM) {
    return WAL_TERM;
  }

  switch (dest->type) {
    case WL_UPDATE: {
      // Read the sub type
      GUARD_EOF (walis_read_all (w->istream, &checksum, &dest->update.type, sizeof (wlh), e));

      switch (dest->update.type) {
        case WUP_PHYSICAL: {
          GUARD_EOF (wal_read_physical_update (w, &checksum, dest, e));
          break;
        }
        case WUP_FEXT: {
          GUARD_EOF (wal_read_file_extend_update (w, &checksum, dest, e));
          break;
        }
        case WUP_FSM: {
          GUARD_EOF (wal_read_fsm_update (w, &checksum, dest, e));
          break;
        }
        default: {
          return error_causef (
              e,
              ERR_CORRUPT,
              "Invalid update log sub type: %" PRwlh ". Expected one of [%" PRwlh ", %" PRwlh
              ", %" PRwlh "]",
              dest->update.type,
              WUP_PHYSICAL,
              WUP_FEXT,
              WUP_FSM
          );
        }
      }
      break;
    }
    case WL_CLR: {
      // Read the sub type
      GUARD_EOF (walis_read_all (w->istream, &checksum, &dest->clr.type, sizeof (wlh), e));

      switch (dest->clr.type) {
        case WCLR_PHYSICAL: {
          GUARD_EOF (wal_read_physical_clr (w, &checksum, dest, e));
          break;
        }
        case WCLR_FSM: {
          GUARD_EOF (wal_read_fsm_clr (w, &checksum, dest, e));
          break;
        }
        case WCLR_DUMMY: {
          GUARD_EOF (wal_read_dummy_clr (w, &checksum, dest, e));
          break;
        }
        default: {
          return error_causef (
              e,
              ERR_CORRUPT,
              "Invalid CLR sub type: %" PRwlh ". Expected one of [%" PRwlh ", %" PRwlh ", %" PRwlh
              "]",
              dest->clr.type,
              WCLR_PHYSICAL,
              WCLR_FSM,
              WCLR_DUMMY
          );
        }
      }
      break;
    }
    case WL_BEGIN: {
      GUARD_EOF (wal_read_begin (w, &checksum, dest, e));
      break;
    }
    case WL_COMMIT: {
      GUARD_EOF (wal_read_commit (w, &checksum, dest, e));
      break;
    }
    case WL_END: {
      GUARD_EOF (wal_read_end (w, &checksum, dest, e));
      break;
    }
    default: {
      return error_causef (e, ERR_CORRUPT, "Invalid log type: %" PRwlh, dest->type);
    }
  }

  return WAL_NOT_EOF;
}

static err_t
wal_seek_unsafe (struct wal *w, lsn id, error *e)
{
  DBG_ASSERT (wal, w);

  if (id == LSN_NULL) {
    return walis_seek (w->istream, WAL_RECORDS_POS, e);
  }

  // Reading before a start lsn
  if (id < w->start_lsn) {
    return error_causef (
        e,
        ERR_CORRUPT,
        "Tried to read previous deleted log %" PRlsn " %" PRlsn,
        id,
        w->start_lsn
    );
  }

  return walis_seek (w->istream, id - w->start_lsn, e);
}

err_t
wal_forward_scan (struct wal *w, lsn start, wal_log_func func, void *ctx, error *e)
{
  DBG_ASSERT (wal, w);

  /**
   * If we're currently in random access mode,
   * swap to forward scan
   */
  if (wal_seek_unsafe (w, start, e) < 0) {
    return error_trace (e);
  }

  // Used to ensure progress is being made
  lsn prevlsn = 0;

  while (true) {
    // Mark the start of a log
    lsn curlsn = walis_mark (w->istream);

    // Assert that we're making progress
    ASSERT (prevlsn <= curlsn);
    prevlsn   = curlsn;

    int iseof = wal_read_one (w, &w->rhdr, e);

    if (iseof < 0) {
      return error_trace (e);
    }

    // End of log file - transition to random access mode
    if (iseof) {

      if (w->ostream == NULL) {

        // Open the ostream where the read left off
        w->ostream = walos_open (w->fname.data, w->mem, w->os, curlsn, e);
        if (w->ostream == NULL) {
          return error_trace (e);
        }
      }

      // Zero out the end, which is maximum length MAX(log size)
      static const u8 zeros[MAX_LOG_SIZE] = {0};
      if (iseof == WAL_EOF && walos_zero_tail (w->ostream, zeros, sizeof zeros, e) < 0) {
        return error_trace (e);
      }

      return SUCCESS;
    }

    // Else, execute the user supplied function
    if (func != NULL && func (&w->rhdr, w->start_lsn + curlsn, ctx, e) < 0) {
      return error_trace (e);
    }
  }
}

struct wal_rec_hdr_read *
wal_read_entry (struct wal *w, struct wal_rec_hdr_read *dest, lsn id, error *e)
{
  DBG_ASSERT (wal, w);
  struct wal_rec_hdr_read *ret = NULL;
  if (dest != NULL) {
    ret = dest;
  } else {
    ret = &w->rhdr;
  }

  latch_lock (&w->l);
  DBG_ASSERT (wal, w);

  // You can only read a wal in random mode if you've done
  // a full wal scan - so ostream must be open
  ASSERT (w->ostream);

  // Flush the ostream if we're requesting data past
  // the flushed lsn
  if (id >= w->start_lsn + w->ostream->flushed_lsn) {
    walos_flush_all (w->ostream);
  }

  if (wal_seek_unsafe (w, id, e) < 0) {
    goto failed;
  }

  int iseof = wal_read_one (w, ret, e);
  if (iseof < 0) {
    goto failed;
  }
  if (iseof) {
    error_causef (e, ERR_CORRUPT, "Tried to read a log at the end of the WAL");
    goto failed;
  }

  latch_unlock (&w->l);
  return ret;

failed:
  latch_unlock (&w->l);
  return NULL;
}

/******************************************************************************
 * SECTION: WAL Write Logic
 * ----------------------------------------------------------------------------
 * @brief Write WAL entries
 ******************************************************************************/

static void
wal_write_begin (const struct wal *w, const struct wal_rec_hdr_write *r)
{
  ASSERT (r->type == WL_BEGIN);

  ASSERT (w->ostream);

  u32       checksum = checksum_init ();
  const wlh t        = r->type;
  walos_write_all (w->ostream, &checksum, &t, sizeof (wlh));
  walos_write_all (w->ostream, &checksum, &r->begin.tid, sizeof (txid));
  walos_write_all (w->ostream, NULL, &checksum, sizeof (u32));
}

static void
wal_write_commit (const struct wal *w, const struct wal_rec_hdr_write *r)
{
  ASSERT (r->type == WL_COMMIT);

  ASSERT (w->ostream);

  u32       checksum = checksum_init ();
  const wlh t        = r->type;
  walos_write_all (w->ostream, &checksum, &t, sizeof (wlh));
  walos_write_all (w->ostream, &checksum, &r->commit.tid, sizeof (txid));
  walos_write_all (w->ostream, &checksum, &r->commit.prev, sizeof (lsn));
  walos_write_all (w->ostream, NULL, &checksum, sizeof (u32));
}

static void
wal_write_end (const struct wal *w, const struct wal_rec_hdr_write *r)
{
  ASSERT (r->type == WL_END);

  ASSERT (w->ostream);

  u32       checksum = checksum_init ();
  const wlh t        = r->type;
  walos_write_all (w->ostream, &checksum, &t, sizeof (wlh));
  walos_write_all (w->ostream, &checksum, &r->end.tid, sizeof (txid));
  walos_write_all (w->ostream, &checksum, &r->end.prev, sizeof (lsn));
  walos_write_all (w->ostream, NULL, &checksum, sizeof (u32));
}

static void
wal_write_physical_update (const struct wal *w, const struct wal_rec_hdr_write *r)
{
  ASSERT (w->ostream);

  u32       checksum = checksum_init ();
  const wlh t        = (wlh)r->type;
  const wlh ut       = (wlh)r->update.type;
  walos_write_all (w->ostream, &checksum, &t, sizeof (wlh));
  walos_write_all (w->ostream, &checksum, &ut, sizeof (wlh));
  walos_write_all (w->ostream, &checksum, &r->update.tid, sizeof (txid));
  walos_write_all (w->ostream, &checksum, &r->update.prev, sizeof (lsn));
  walos_write_all (w->ostream, &checksum, &r->update.phys.pg, sizeof (pgno));
  walos_write_all (w->ostream, &checksum, r->update.phys.undo, NS_PAGE_SIZE);
  walos_write_all (w->ostream, &checksum, r->update.phys.redo, NS_PAGE_SIZE);
  walos_write_all (w->ostream, NULL, &checksum, sizeof (u32));
}

static void
wal_write_fsm_update (const struct wal *w, const struct wal_rec_hdr_write *r)
{
  ASSERT (w->ostream);

  u32       checksum = checksum_init ();
  const wlh t        = (wlh)r->type;
  const wlh ut       = (wlh)r->update.type;
  walos_write_all (w->ostream, &checksum, &t, sizeof (wlh));
  walos_write_all (w->ostream, &checksum, &ut, sizeof (wlh));
  walos_write_all (w->ostream, &checksum, &r->update.tid, sizeof (txid));
  walos_write_all (w->ostream, &checksum, &r->update.prev, sizeof (lsn));
  walos_write_all (w->ostream, &checksum, &r->update.fsm.pg, sizeof (pgno));
  walos_write_all (w->ostream, &checksum, &r->update.fsm.bit, sizeof (p_size));
  walos_write_all (w->ostream, &checksum, &r->update.fsm.undo, sizeof (u8));
  walos_write_all (w->ostream, &checksum, &r->update.fsm.redo, sizeof (u8));
  walos_write_all (w->ostream, NULL, &checksum, sizeof (u32));
}

static void
wal_write_file_extend_update (const struct wal *w, const struct wal_rec_hdr_write *r)
{
  ASSERT (w->ostream);

  u32       checksum = checksum_init ();
  const wlh t        = (wlh)r->type;
  const wlh ut       = (wlh)r->update.type;
  walos_write_all (w->ostream, &checksum, &t, sizeof (wlh));
  walos_write_all (w->ostream, &checksum, &ut, sizeof (wlh));
  walos_write_all (w->ostream, &checksum, &r->update.tid, sizeof (txid));
  walos_write_all (w->ostream, &checksum, &r->update.prev, sizeof (lsn));
  walos_write_all (w->ostream, &checksum, &r->update.fext.undo, sizeof (pgno));
  walos_write_all (w->ostream, &checksum, &r->update.fext.redo, sizeof (pgno));
  walos_write_all (w->ostream, NULL, &checksum, sizeof (u32));
}

static void
wal_write_physical_clr (const struct wal *w, const struct wal_rec_hdr_write *r)
{
  ASSERT (r->type == WL_CLR);

  ASSERT (w->ostream);

  u32       checksum = checksum_init ();
  const wlh t        = r->type;
  const wlh ut       = r->clr.type;
  walos_write_all (w->ostream, &checksum, &t, sizeof (wlh));
  walos_write_all (w->ostream, &checksum, &ut, sizeof (wlh));
  walos_write_all (w->ostream, &checksum, &r->clr.tid, sizeof (txid));
  walos_write_all (w->ostream, &checksum, &r->clr.prev, sizeof (lsn));
  walos_write_all (w->ostream, &checksum, &r->clr.phys.pg, sizeof (pgno));
  walos_write_all (w->ostream, &checksum, &r->clr.undo_next, sizeof (lsn));
  walos_write_all (w->ostream, &checksum, r->clr.phys.redo, NS_PAGE_SIZE);
  walos_write_all (w->ostream, NULL, &checksum, sizeof (u32));
}

static void
wal_write_fsm_clr (const struct wal *w, const struct wal_rec_hdr_write *r)
{
  ASSERT (r->type == WL_CLR);

  ASSERT (w->ostream);

  u32       checksum = checksum_init ();
  const wlh t        = r->type;
  const wlh ut       = r->clr.type;
  walos_write_all (w->ostream, &checksum, &t, sizeof (wlh));
  walos_write_all (w->ostream, &checksum, &ut, sizeof (wlh));
  walos_write_all (w->ostream, &checksum, &r->clr.tid, sizeof (txid));
  walos_write_all (w->ostream, &checksum, &r->clr.prev, sizeof (lsn));
  walos_write_all (w->ostream, &checksum, &r->clr.fsm.pg, sizeof (pgno));
  walos_write_all (w->ostream, &checksum, &r->clr.undo_next, sizeof (lsn));
  walos_write_all (w->ostream, &checksum, &r->clr.fsm.bit, sizeof (p_size));
  walos_write_all (w->ostream, &checksum, &r->clr.fsm.redo, sizeof (u8));
  walos_write_all (w->ostream, NULL, &checksum, sizeof (u32));
}

static void
wal_write_dummy_clr (const struct wal *w, const struct wal_rec_hdr_write *r)
{
  ASSERT (r->type == WL_CLR);

  ASSERT (w->ostream);

  u32       checksum = checksum_init ();
  const wlh t        = r->type;
  const wlh ut       = r->clr.type;
  walos_write_all (w->ostream, &checksum, &t, sizeof (wlh));
  walos_write_all (w->ostream, &checksum, &ut, sizeof (wlh));
  walos_write_all (w->ostream, &checksum, &r->clr.tid, sizeof (txid));
  walos_write_all (w->ostream, &checksum, &r->clr.prev, sizeof (lsn));
  walos_write_all (w->ostream, &checksum, &r->clr.undo_next, sizeof (lsn));
  walos_write_all (w->ostream, NULL, &checksum, sizeof (u32));
}

static lsn
wal_write_locked (struct wal *w)
{
  ASSERT (w->ostream);
  ASSERT (!(w->flags & WAL_ISNEW));

  lsn ret = walos_get_lsn (w->ostream) + w->start_lsn;

  switch (w->whdr.type) {
    case WL_BEGIN: {
      wal_write_begin (w, &w->whdr);
      break;
    }
    case WL_COMMIT: {
      wal_write_commit (w, &w->whdr);
      break;
    }
    case WL_END: {
      wal_write_end (w, &w->whdr);
      break;
    }
    case WL_UPDATE: {
      switch (w->whdr.update.type) {
        case WUP_PHYSICAL: {
          wal_write_physical_update (w, &w->whdr);
          break;
        }
        case WUP_FSM: {
          wal_write_fsm_update (w, &w->whdr);
          break;
        }
        case WUP_FEXT: {
          wal_write_file_extend_update (w, &w->whdr);
          break;
        }
      }
      break;
    }
    case WL_CLR: {
      switch (w->whdr.clr.type) {
        case WCLR_PHYSICAL: {
          wal_write_physical_clr (w, &w->whdr);
          break;
        }
        case WCLR_FSM: {
          wal_write_fsm_clr (w, &w->whdr);
          break;
        }
        case WCLR_DUMMY: {
          wal_write_dummy_clr (w, &w->whdr);
          break;
        }
      }
      break;
    }
  }

  return ret;
}

/*-----------------------------------------------------------------------------
 * SUBSECTION: WAL Write
 * @brief Write implementation functions
 *----------------------------------------------------------------------------*/

lsn
wal_append_begin_log (struct wal *w, const txid tid)
{
  latch_lock (&w->l);
  DBG_ASSERT (wal, w);
  w->whdr.type  = WL_BEGIN;
  w->whdr.begin = (struct wal_begin){.tid = tid};
  lsn result    = wal_write_locked (w);
  latch_unlock (&w->l);
  return result;
}

lsn
wal_append_commit_log (struct wal *w, const txid tid, lsn prev)
{
  latch_lock (&w->l);
  DBG_ASSERT (wal, w);
  w->whdr.type   = WL_COMMIT;
  w->whdr.commit = (struct wal_commit){.tid = tid, .prev = prev};
  lsn result     = wal_write_locked (w);
  latch_unlock (&w->l);
  return result;
}

lsn
wal_append_end_log (struct wal *w, const txid tid, lsn prev)
{
  latch_lock (&w->l);
  DBG_ASSERT (wal, w);
  w->whdr.type = WL_END;
  w->whdr.end  = (struct wal_end){.tid = tid, .prev = prev};
  lsn result   = wal_write_locked (w);
  latch_unlock (&w->l);
  return result;
}

lsn
wal_append_update_log (struct wal *w, const struct wal_update_write update)
{
  latch_lock (&w->l);
  DBG_ASSERT (wal, w);
  w->whdr.type   = WL_UPDATE;
  w->whdr.update = update;
  lsn result     = wal_write_locked (w);
  latch_unlock (&w->l);
  return result;
}

lsn
wal_append_clr_log (struct wal *w, const struct wal_clr_write clr)
{
  latch_lock (&w->l);
  DBG_ASSERT (wal, w);
  w->whdr.type = WL_CLR;
  w->whdr.clr  = clr;
  lsn result   = wal_write_locked (w);
  latch_unlock (&w->l);
  return result;
}

/*****************************************************************************
 * SECTION: Tests
 ****************************************************************************/

// These two came in with the named-constants fix (problem #2). Until that
// lands, the tests need something to compile against - both guards go away on
// their own once ns_wal.h defines the real ones.
#ifndef WAL_START_LSN
#  define WAL_START_LSN ((lsn)0)
#endif
#ifndef WAL_RECORDS_START
#  define WAL_RECORDS_START ((lsn)sizeof (lsn))
#endif

#ifndef NDEBUG

#  define WAL_T_FILE "wal_t.wal"

/// Byte length of the log on disk, or -1 if it isn't there
static i64
wal_test_file_size (struct test_ctx *ctx, const char *fname)
{
  // [open_file_r] is O_CREAT, so opening is not a test for existence - it would
  // make the file and report it as empty. Ask first.
  bool exists = false;
  if (ctx->os.table->file_exists (ctx->os.self, fname, &exists, &ctx->e) != SUCCESS) {
    error_reset (&ctx->e);
    return -1;
  }
  if (!exists) {
    return -1;
  }

  i_file f;
  if (ctx->os.table->open_file_r (ctx->os.self, &f, fname, &ctx->e) != SUCCESS) {
    error_reset (&ctx->e);
    return -1;
  }
  const i64 size = f.table->file_size (f.self, &ctx->e);
  ctx->os.table->close_file (ctx->os.self, f.self, &ctx->e);
  return size;
}

/// Reads [n] bytes of the log at [ofst] into [dest]
static void
wal_test_read_at (struct test_ctx *ctx, const char *fname, void *dest, u64 n, u64 ofst)
{
  i_file f;
  test_assert_int_equal (ctx->os.table->open_file_r (ctx->os.self, &f, fname, &ctx->e), SUCCESS);
  test_assert_int_equal ((i32)f.table->pread_all (f.self, dest, n, ofst, &ctx->e), (i32)n);
  test_assert_int_equal (ctx->os.table->close_file (ctx->os.self, f.self, &ctx->e), SUCCESS);
}

static void
wal_test_truncate (struct test_ctx *ctx, const char *fname, u64 len)
{
  i_file f;
  test_assert_int_equal (ctx->os.table->open_file_w (ctx->os.self, &f, fname, &ctx->e), SUCCESS);
  test_assert_int_equal (f.table->truncate (f.self, len, &ctx->e), SUCCESS);
  test_assert_int_equal (ctx->os.table->close_file (ctx->os.self, f.self, &ctx->e), SUCCESS);
}

static struct wal *
wal_test_fresh (struct test_ctx *ctx, struct i_mem mem, struct i_os os, lsn start_lsn)
{
  os.table->remove_quiet (os.self, WAL_T_FILE, &ctx->e);

  struct wal *w = wal_open (WAL_T_FILE, mem, os, &ctx->e);
  if (w == NULL) {
    return NULL;
  }

  if (wal_write_start_lsn (w, start_lsn, &ctx->e) < 0) {
    wal_close (w, &ctx->e);
    return NULL;
  }

  return w;
}

struct wal_test_scan
{
  struct wal_rec_hdr_read recs[16];
  lsn                     lsns[16];
  u32                     n;
  u32                     stop_after;
};

static err_t
wal_test_collect (struct wal_rec_hdr_read *rec, lsn l, void *vctx, error *e)
{
  struct wal_test_scan *c = vctx;

  if (c->stop_after && c->n + 1 >= c->stop_after) {
    return error_causef (e, ERR_IO, "wal_test_collect: asked to stop");
  }

  ASSERT (c->n < arrlen (c->recs));
  c->recs[c->n] = *rec;
  c->lsns[c->n] = l;
  c->n++;

  return SUCCESS;
}

/// Appends whatever record [r] describes, going through the same per-type
/// entry points the pager uses
static lsn
wal_test_append (struct wal *w, struct wal_rec_hdr_read *r)
{
  struct wal_rec_hdr_write out = wrhw_from_wrhr (r);

  switch (out.type) {
    case WL_BEGIN: {
      return wal_append_begin_log (w, out.begin.tid);
    }
    case WL_COMMIT: {
      return wal_append_commit_log (w, out.commit.tid, out.commit.prev);
    }
    case WL_END: {
      return wal_append_end_log (w, out.end.tid, out.end.prev);
    }
    case WL_UPDATE: {
      return wal_append_update_log (w, out.update);
    }
    case WL_CLR: {
      return wal_append_clr_log (w, out.clr);
    }
  }

  UNREACHABLE (); // LCOV_EXCL_LINE
}

/// One record of every shape the log can hold, so a round trip covers every
/// encode/decode pair. [phys] backs the two page sized payloads.
static u32
wal_test_all_shapes (struct wal_rec_hdr_read *dest, u32 cap, u8 (*phys)[NS_PAGE_SIZE])
{
  ASSERT (cap >= 8);

  rand_bytes (phys[0], NS_PAGE_SIZE);
  rand_bytes (phys[1], NS_PAGE_SIZE);
  rand_bytes (phys[2], NS_PAGE_SIZE);

  u32 n     = 0;

  dest[n++] = (struct wal_rec_hdr_read){.type = WL_BEGIN, .begin = {.tid = 1}};
  dest[n++] = (struct wal_rec_hdr_read){.type = WL_COMMIT, .commit = {.tid = 2, .prev = 11}};
  dest[n++] = (struct wal_rec_hdr_read){.type = WL_END, .end = {.tid = 3, .prev = 22}};

  dest[n]   = (struct wal_rec_hdr_read){
      .type   = WL_UPDATE,
      .update = {.type = WUP_PHYSICAL, .tid = 4, .prev = 33, .phys = {.pg = 101}},
  };
  memcpy (dest[n].update.phys.undo, phys[0], NS_PAGE_SIZE);
  memcpy (dest[n].update.phys.redo, phys[1], NS_PAGE_SIZE);
  n++;

  dest[n++] = (struct wal_rec_hdr_read){
      .type   = WL_UPDATE,
      .update = {
          .type = WUP_FSM,
          .tid  = 5,
          .prev = 44,
          .fsm  = {.pg = 0, .bit = 7, .undo = 0, .redo = 1},
      },
  };

  dest[n++] = (struct wal_rec_hdr_read){
      .type   = WL_UPDATE,
      .update = {.type = WUP_FEXT, .tid = 6, .prev = 55, .fext = {.undo = 9, .redo = 17}},
  };

  dest[n] = (struct wal_rec_hdr_read){
      .type = WL_CLR,
      .clr  = {
          .type      = WCLR_PHYSICAL,
          .tid       = 7,
          .prev      = 66,
          .undo_next = 65,
          .phys      = {.pg = 202},
      },
  };
  memcpy (dest[n].clr.phys.redo, phys[2], NS_PAGE_SIZE);
  n++;

  dest[n++] = (struct wal_rec_hdr_read){
      .type = WL_CLR,
      .clr  = {
          .type      = WCLR_FSM,
          .tid       = 8,
          .prev      = 77,
          .undo_next = 76,
          .fsm       = {.pg = 0, .bit = 3, .redo = 1},
      },
  };

  dest[n++] = (struct wal_rec_hdr_read){
      .type = WL_CLR,
      .clr  = {.type = WCLR_DUMMY, .tid = 9, .prev = 88, .undo_next = 87},
  };

  ASSERT (n <= cap);
  return n;
}

TEST (wal_open)
{
  TEST_CASE ("happy path")
  {
    ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);

    struct wal *w = wal_open (WAL_T_FILE, ctx->mem, ctx->os, &ctx->e);
    test_assert (w != NULL);

    // Nothing was on disk, so this is a brand new log: the reader opens read
    // only and would have failed outright if the file had not been created
    test_assert (wal_isnew (w));
    test_assert_int_equal ((i32)wal_start_lsn (w), 0);
    test_assert_int_equal ((i32)wal_test_file_size (ctx, WAL_T_FILE), 0);

    // A fresh log has its writer open from the start - there is nothing to
    // scan, so there is nothing to wait for
    test_assert (w->ostream != NULL);
    test_assert_int_equal ((i32)wal_size (w), 0);

    test_assert_int_equal (wal_close (w, &ctx->e), SUCCESS);
    ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);
  }

  TEST_CASE ("reopening an existing log")
  {
    struct wal *w = wal_test_fresh (ctx, ctx->mem, ctx->os, 4096);
    test_assert (w != NULL);

    test_assert (
        wal_test_append (
            w,
            &(struct wal_rec_hdr_read){
                .type  = WL_BEGIN,
                .begin = {.tid = 1},
            }
        )
        > 0
    );
    test_assert_int_equal (wal_close (w, &ctx->e), SUCCESS);

    w = wal_open (WAL_T_FILE, ctx->mem, ctx->os, &ctx->e);
    test_assert (w != NULL);

    // The header is still there, so this log is not new and remembers where
    // its lsn space starts
    test_assert (!wal_isnew (w));
    test_assert_int_equal ((i32)wal_start_lsn (w), 4096);

    // And the writer stays shut until a scan has found the end of the records
    test_assert (w->ostream == NULL);

    test_assert_int_equal (wal_close (w, &ctx->e), SUCCESS);
    ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);
  }

  TEST_CASE ("a half written header counts as a fresh log")
  {
    struct wal *w = wal_test_fresh (ctx, ctx->mem, ctx->os, 4096);
    test_assert (w != NULL);
    test_assert_int_equal (wal_close (w, &ctx->e), SUCCESS);

    // Die part way through writing the start lsn
    wal_test_truncate (ctx, WAL_T_FILE, 3);

    w = wal_open (WAL_T_FILE, ctx->mem, ctx->os, &ctx->e);
    test_assert (w != NULL);

    // There is no usable header, so the log starts over. The 3 byte stub is
    // still on disk underneath - the header write covers it, and anything it
    // did not cover is not a valid record, so a scan stops there
    test_assert (wal_isnew (w));

    test_assert_int_equal (wal_close (w, &ctx->e), SUCCESS);
    ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);
  }

  TEST_CASE ("Flaky malloc")
  {
    int   fail_after = 0;
    err_t ret;

    do {
      TEST_CASE ("Failure flaky malloc. Fail after: %d", fail_after)
      {
        // Setup
        ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);

        // Do test
        faulty_mem_fail_after (ctx->faulty_mem, fail_after);
        struct wal *w = wal_open (WAL_T_FILE, ctx->faulty_mem, ctx->os, &ctx->e);
        ret           = error_trace (&ctx->e);
        if (ret != SUCCESS) {
          test_assert (w == NULL);
        }
        faulty_mem_reset (ctx->faulty_mem);
        error_reset (&ctx->e);

        // Clean up
        if (w != NULL) {
          wal_close (w, &ctx->e);
        }
        ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);

        // Next fail after
        fail_after++;
      }
    }
    while (ret != SUCCESS);
  }

  TEST_CASE ("Flaky os")
  {
    int   fail_after = 0;
    err_t ret;

    do {
      TEST_CASE ("Failure flaky os. Fail after: %d", fail_after)
      {
        // Setup
        ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);

        // Do test
        faulty_os_fail_after (ctx->faulty_os, fail_after);
        struct wal *w = wal_open (WAL_T_FILE, ctx->mem, ctx->faulty_os, &ctx->e);
        ret           = error_trace (&ctx->e);
        if (ret != SUCCESS) {
          test_assert (w == NULL);
        }
        faulty_os_reset (ctx->faulty_os);
        error_reset (&ctx->e);

        // Clean up
        if (w != NULL) {
          wal_close (w, &ctx->e);
          error_reset (&ctx->e);
        }
        ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);

        // Next fail after
        fail_after++;
      }
    }
    while (ret != SUCCESS);
  }
}

TEST (wal_write_start_lsn)
{
  TEST_CASE ("happy path")
  {
    ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);

    struct wal *w = wal_open (WAL_T_FILE, ctx->mem, ctx->os, &ctx->e);
    test_assert (w != NULL);
    test_assert (wal_isnew (w));

    test_assert_int_equal (wal_write_start_lsn (w, 4096, &ctx->e), SUCCESS);

    // The log is now open for business at the lsn it was handed
    test_assert (!wal_isnew (w));
    test_assert_int_equal ((i32)wal_start_lsn (w), 4096);

    // The header is on disk, and records start right after it
    test_assert_int_equal ((i32)wal_size (w), (i32)WAL_RECORDS_START);
    test_assert_int_equal ((i32)wal_test_file_size (ctx, WAL_T_FILE), (i32)WAL_RECORDS_START);

    // So the first record lands at the first lsn of this log
    lsn first = wal_append_begin_log (w, 1);
    test_assert_int_equal ((i32)first, (i32)(4096 + WAL_RECORDS_START));

    test_assert_int_equal (wal_close (w, &ctx->e), SUCCESS);
    ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);
  }

  TEST_CASE ("Flaky os")
  {
    int   fail_after = 0;
    err_t ret;

    do {
      TEST_CASE ("Failure flaky os. Fail after: %d", fail_after)
      {
        // Setup
        ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);
        struct wal *w = wal_open (WAL_T_FILE, ctx->mem, ctx->faulty_os, &ctx->e);
        test_assert (w != NULL);

        // Do test. Writing the header reports what it can, but it also
        // flushes - and a flush that cannot reach the file takes the process
        // down rather than claim the header is durable.
        faulty_os_fail_after (ctx->faulty_os, fail_after);
        capture_intentional_crash (&ret, &ctx->e)
        {
          ret = wal_write_start_lsn (w, 4096, &ctx->e);
        }

        // Clean up
        faulty_os_reset (ctx->faulty_os);
        error_reset (&ctx->e);
        wal_close (w, &ctx->e);
        error_reset (&ctx->e);
        ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);

        // Next fail after
        fail_after++;
      }
    }
    while (ret != SUCCESS);
  }
}
#endif

#ifndef NDEBUG
TEST (wal_size)
{
  TEST_CASE ("happy path")
  {
    struct wal *w = wal_test_fresh (ctx, ctx->mem, ctx->os, 1024);
    test_assert (w != NULL);

    // Just the header so far
    test_assert_int_equal ((i32)wal_size (w), (i32)WAL_RECORDS_START);

    // Buffered records count towards it
    wal_append_begin_log (w, 1);
    lsn buffered = wal_size (w);
    test_assert (buffered > WAL_RECORDS_START);
    test_assert_int_equal ((i32)wal_test_file_size (ctx, WAL_T_FILE), (i32)WAL_RECORDS_START);

    // Flushing doesn't change the size, it just moves it to the file
    wal_flush_all (w);
    test_assert_int_equal ((i32)wal_size (w), (i32)buffered);
    test_assert_int_equal ((i32)wal_test_file_size (ctx, WAL_T_FILE), (i32)buffered);

    // Flushing an empty buffer changes nothing
    wal_flush_all (w);
    test_assert_int_equal ((i32)wal_size (w), (i32)buffered);

    // The next log of this database starts where this one ends
    test_assert_int_equal ((i32)(wal_start_lsn (w) + wal_size (w)), (i32)(1024 + buffered));

    test_assert_int_equal (wal_close (w, &ctx->e), SUCCESS);
    ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);
  }
}
#endif

#ifndef NDEBUG
TEST (wal_close)
{
  TEST_CASE ("happy path")
  {
    struct wal *w = wal_test_fresh (ctx, ctx->mem, ctx->os, 0);
    test_assert (w != NULL);

    // Buffered, not yet on disk
    test_assert (wal_append_begin_log (w, 1) > 0);
    lsn size = wal_size (w);
    test_assert_int_equal ((i32)wal_test_file_size (ctx, WAL_T_FILE), (i32)WAL_RECORDS_START);

    // Closing pushes it out, and leaves the log behind to be reopened
    test_assert_int_equal (wal_close (w, &ctx->e), SUCCESS);
    test_assert_int_equal ((i32)wal_test_file_size (ctx, WAL_T_FILE), (i32)size);

    ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);
  }

  TEST_CASE ("Flaky os")
  {
    int   fail_after = 0;
    err_t ret;

    do {
      TEST_CASE ("Failure flaky os. Fail after: %d", fail_after)
      {
        // Setup
        struct wal *w = wal_test_fresh (ctx, ctx->mem, ctx->faulty_os, 0);
        test_assert (w != NULL);
        test_assert (wal_append_begin_log (w, 1) > 0);

        // Do test - closing reports instead of dying, and frees either way
        faulty_os_fail_after (ctx->faulty_os, fail_after);
        ret = wal_close (w, &ctx->e);

        // Clean up
        faulty_os_reset (ctx->faulty_os);
        error_reset (&ctx->e);
        ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);

        // Next fail after
        fail_after++;
      }
    }
    while (ret != SUCCESS);
  }
}
#endif

#ifndef NDEBUG
TEST (wal_close_and_delete)
{
  TEST_CASE ("happy path")
  {
    struct wal *w = wal_test_fresh (ctx, ctx->mem, ctx->os, 0);
    test_assert (w != NULL);
    test_assert (wal_append_begin_log (w, 1) > 0);

    test_assert_int_equal (wal_close_and_delete (w, &ctx->e), SUCCESS);

    // The log is gone, not merely closed
    test_assert_int_equal ((i32)wal_test_file_size (ctx, WAL_T_FILE), -1);
  }

  TEST_CASE ("Flaky os")
  {
    int   fail_after = 0;
    err_t ret;

    do {
      TEST_CASE ("Failure flaky os. Fail after: %d", fail_after)
      {
        // Setup
        struct wal *w = wal_test_fresh (ctx, ctx->mem, ctx->faulty_os, 0);
        test_assert (w != NULL);

        // Do test (the wal is freed regardless of the error)
        faulty_os_fail_after (ctx->faulty_os, fail_after);
        ret = wal_close_and_delete (w, &ctx->e);

        // Clean up
        faulty_os_reset (ctx->faulty_os);
        error_reset (&ctx->e);
        ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);

        // Next fail after
        fail_after++;
      }
    }
    while (ret != SUCCESS);
  }
}
#endif

#ifndef NDEBUG
TEST (wal_delete_and_reopen)
{
  TEST_CASE ("happy path")
  {
    struct wal *w = wal_test_fresh (ctx, ctx->mem, ctx->os, 1024);
    test_assert (w != NULL);
    test_assert (wal_append_begin_log (w, 1) > 0);
    test_assert (wal_size (w) > WAL_RECORDS_START);

    test_assert_int_equal (wal_delete_and_reopen (w, &ctx->e), SUCCESS);

    // Everything that was in it is gone and it is a fresh log again, waiting
    // for a start lsn
    test_assert (wal_isnew (w));
    test_assert_int_equal ((i32)wal_size (w), 0);
    test_assert_int_equal ((i32)wal_test_file_size (ctx, WAL_T_FILE), 0);

    // Which it will take, at a new place in the lsn space
    test_assert_int_equal (wal_write_start_lsn (w, 2048, &ctx->e), SUCCESS);
    test_assert_int_equal ((i32)wal_start_lsn (w), 2048);

    // The latch went back, so the next append does not hang
    test_assert (latch_trylock (&w->l));
    latch_unlock (&w->l);
    test_assert (wal_append_begin_log (w, 2) > 0);

    test_assert_int_equal (wal_close (w, &ctx->e), SUCCESS);
    ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);
  }

  TEST_CASE ("Flaky os")
  {
    int   fail_after = 0;
    err_t ret;

    do {
      TEST_CASE ("Failure flaky os. Fail after: %d", fail_after)
      {
        // Setup
        struct wal *w = wal_test_fresh (ctx, ctx->mem, ctx->faulty_os, 0);
        test_assert (w != NULL);

        // Do test
        faulty_os_fail_after (ctx->faulty_os, fail_after);
        ret = wal_delete_and_reopen (w, &ctx->e);

        // However it went, the latch is not still held
        test_assert (latch_trylock (&w->l));
        latch_unlock (&w->l);

        // Clean up
        faulty_os_reset (ctx->faulty_os);
        error_reset (&ctx->e);
        wal_close (w, &ctx->e);
        error_reset (&ctx->e);
        ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);

        // Next fail after
        fail_after++;
      }
    }
    while (ret != SUCCESS);
  }
}
#endif

#ifndef NDEBUG
/// wal_crash walks away from a live log: the handles and the allocation go
/// back, the buffer does not reach the file, and whatever was already flushed
/// has to still be readable afterwards.
TEST (wal_crash)
{
  TEST_CASE ("unflushed records never reach the log")
  {
    struct wal *w = wal_test_fresh (ctx, ctx->mem, ctx->os, 0);
    test_assert (w != NULL);

    test_assert (wal_append_begin_log (w, 1) > 0);
    test_assert (wal_size (w) > WAL_RECORDS_START);

    test_assert_int_equal (wal_crash (w, &ctx->e), SUCCESS);

    // Only the header, which wal_write_start_lsn flushed, survived
    test_assert_int_equal ((i32)wal_test_file_size (ctx, WAL_T_FILE), (i32)WAL_RECORDS_START);

    ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);
  }

  TEST_CASE ("flushed records survive and are still readable")
  {
    struct wal *w = wal_test_fresh (ctx, ctx->mem, ctx->os, 0);
    test_assert (w != NULL);

    lsn kept = wal_append_begin_log (w, 7);
    wal_flush_all (w);
    lsn size = wal_size (w);

    // A second record that is still in the buffer when we die
    test_assert (wal_append_commit_log (w, 7, kept) > 0);
    test_assert (wal_size (w) > size);

    test_assert_int_equal (wal_crash (w, &ctx->e), SUCCESS);

    // Exactly the flushed prefix is there
    test_assert_int_equal ((i32)wal_test_file_size (ctx, WAL_T_FILE), (i32)size);

    // And it reads back as the record it was
    w = wal_open (WAL_T_FILE, ctx->mem, ctx->os, &ctx->e);
    test_assert (w != NULL);
    test_assert (!wal_isnew (w));

    // The scan is what opens the writer again, at the end of the intact log
    struct wal_test_scan seen = {0};
    test_assert_int_equal (
        wal_forward_scan (w, LSN_NULL, wal_test_collect, &seen, &ctx->e),
        SUCCESS
    );
    test_assert_int_equal ((i32)seen.n, 1);
    test_assert_int_equal (seen.recs[0].type, WL_BEGIN);
    test_assert_int_equal ((i32)seen.lsns[0], (i32)kept);
    test_assert_int_equal ((i32)wal_size (w), (i32)size);

    test_assert_int_equal (wal_close (w, &ctx->e), SUCCESS);
    ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);
  }

  TEST_CASE ("crashing straight after open leaves an empty log behind")
  {
    ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);

    struct wal *w = wal_open (WAL_T_FILE, ctx->mem, ctx->os, &ctx->e);
    test_assert (w != NULL);
    test_assert_int_equal (wal_crash (w, &ctx->e), SUCCESS);

    // wal_open created the file, so it exists and is empty - not missing
    test_assert_int_equal ((i32)wal_test_file_size (ctx, WAL_T_FILE), 0);

    ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);
  }

  TEST_CASE ("Flaky os")
  {
    int   fail_after = 0;
    err_t ret;

    do {
      TEST_CASE ("Failure flaky os. Fail after: %d", fail_after)
      {
        // Setup
        struct wal *w = wal_test_fresh (ctx, ctx->mem, ctx->faulty_os, 0);
        test_assert (w != NULL);

        // Do test (the wal is freed regardless of the error)
        faulty_os_fail_after (ctx->faulty_os, fail_after);
        wal_crash (w, &ctx->e);
        ret = error_trace (&ctx->e);

        // Clean up
        faulty_os_reset (ctx->faulty_os);
        error_reset (&ctx->e);
        ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);

        // Next fail after
        fail_after++;
      }
    }
    while (ret != SUCCESS);
  }
}
#endif

#ifndef NDEBUG
TEST (wal_forward_scan)
{
  u8                      phys[3][NS_PAGE_SIZE];
  struct wal_rec_hdr_read want[16];
  lsn                     at[16];

  TEST_CASE ("every record shape round trips, in order and at its own lsn")
  {
    struct wal *w = wal_test_fresh (ctx, ctx->mem, ctx->os, 4096);
    test_assert (w != NULL);

    const u32 n = wal_test_all_shapes (want, arrlen (want), phys);
    for (u32 i = 0; i < n; ++i) {
      at[i] = wal_test_append (w, &want[i]);
      test_assert (at[i] > 0);
    }
    wal_flush_all (w);

    struct wal_test_scan seen = {0};
    test_assert_int_equal (
        wal_forward_scan (w, LSN_NULL, wal_test_collect, &seen, &ctx->e),
        SUCCESS
    );

    test_assert_int_equal ((i32)seen.n, (i32)n);
    for (u32 i = 0; i < n; ++i) {
      test_assert (wal_rec_hdr_read_equal (&seen.recs[i], &want[i]));
      test_assert_int_equal ((i32)seen.lsns[i], (i32)at[i]);
    }

    // The first record sits right after the header, in this log's lsn space
    test_assert_int_equal ((i32)seen.lsns[0], (i32)(4096 + WAL_RECORDS_START));

    test_assert_int_equal (wal_close (w, &ctx->e), SUCCESS);
    ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);
  }

  TEST_CASE ("scanning from part way in skips what came before")
  {
    struct wal *w = wal_test_fresh (ctx, ctx->mem, ctx->os, 0);
    test_assert (w != NULL);

    const u32 n = wal_test_all_shapes (want, arrlen (want), phys);
    for (u32 i = 0; i < n; ++i) {
      at[i] = wal_test_append (w, &want[i]);
    }
    wal_flush_all (w);

    struct wal_test_scan seen = {0};
    test_assert_int_equal (
        wal_forward_scan (w, at[n - 2], wal_test_collect, &seen, &ctx->e),
        SUCCESS
    );

    test_assert_int_equal ((i32)seen.n, 2);
    test_assert (wal_rec_hdr_read_equal (&seen.recs[0], &want[n - 2]));
    test_assert (wal_rec_hdr_read_equal (&seen.recs[1], &want[n - 1]));

    test_assert_int_equal (wal_close (w, &ctx->e), SUCCESS);
    ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);
  }

  TEST_CASE ("an empty log reports nothing")
  {
    struct wal *w = wal_test_fresh (ctx, ctx->mem, ctx->os, 0);
    test_assert (w != NULL);

    struct wal_test_scan seen = {0};
    test_assert_int_equal (
        wal_forward_scan (w, LSN_NULL, wal_test_collect, &seen, &ctx->e),
        SUCCESS
    );
    test_assert_int_equal ((i32)seen.n, 0);

    test_assert_int_equal (wal_close (w, &ctx->e), SUCCESS);
    ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);
  }

  TEST_CASE ("the callback can stop the scan")
  {
    struct wal *w = wal_test_fresh (ctx, ctx->mem, ctx->os, 0);
    test_assert (w != NULL);

    const u32 n = wal_test_all_shapes (want, arrlen (want), phys);
    for (u32 i = 0; i < n; ++i) {
      wal_test_append (w, &want[i]);
    }
    wal_flush_all (w);

    struct wal_test_scan seen = {.stop_after = 3};
    test_assert (wal_forward_scan (w, LSN_NULL, wal_test_collect, &seen, &ctx->e) != SUCCESS);
    test_assert_int_equal ((i32)seen.n, 2);
    error_reset (&ctx->e);

    test_assert_int_equal (wal_close (w, &ctx->e), SUCCESS);
    ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);
  }

  TEST_CASE ("Invalid tail is where the log ends and the writer resumes")
  {
    struct wal *w = wal_test_fresh (ctx, ctx->mem, ctx->os, 0);
    test_assert (w != NULL);

    lsn first  = wal_append_begin_log (w, 1);
    lsn second = wal_append_commit_log (w, 1, first);
    wal_flush_all (w);
    lsn intact = wal_size (w);

    // One more record, flushed, then cut part way through it
    test_assert (wal_append_end_log (w, 1, second) > 0);
    wal_flush_all (w);

    // The last bit is an END log
    test_assert (wal_size (w) > intact);
    lsn last_size = wal_size (w) - intact;
    test_assert (last_size == WL_END_LEN);

    test_assert_int_equal (wal_close (w, &ctx->e), SUCCESS);

    // Remove the last byte of the last log
    wal_test_truncate (ctx, WAL_T_FILE, intact + WL_END_LEN - 1);

    // When you open up on a non scanned WAL - it's
    // ostream isn't open until you do a forward scan
    w = wal_open (WAL_T_FILE, ctx->mem, ctx->os, &ctx->e);
    test_assert (w != NULL);
    test_assert (w->ostream == NULL);

    // Do a forward scan
    struct wal_test_scan seen = {0};
    err_t                ret  = wal_forward_scan (w, LSN_NULL, wal_test_collect, &seen, &ctx->e);
    test_assert_int_equal (ret, SUCCESS);

    // The stump is not a record
    test_assert_int_equal ((i32)seen.n, 2);
    test_assert_int_equal ((i32)seen.lsns[1], (i32)second);

    // The ostream is opened at the end of a full log scan
    test_assert (w->ostream != NULL);
    test_assert_int_equal ((i32)wal_size (w), (i32)intact);

    // Next log starts at the end of the "intact logs"
    lsn third = wal_append_begin_log (w, 1);
    test_assert_int_equal ((i32)third, (i32)intact);
    wal_flush_all (w);
    test_assert (wal_test_file_size (ctx, WAL_T_FILE) > (i64)wal_size (w));

    // Do another forward scan
    struct wal_test_scan again = {0};
    ret                        = wal_forward_scan (w, LSN_NULL, wal_test_collect, &again, &ctx->e);
    test_assert_int_equal (ret, SUCCESS);
    test_assert_int_equal ((i32)again.n, 3);
    test_assert_int_equal (again.recs[2].type, WL_BEGIN);

    test_assert_int_equal (wal_close (w, &ctx->e), SUCCESS);
    ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);
  }

  TEST_CASE ("a record with a bad checksum is corruption, not an ending")
  {
    struct wal *w = wal_test_fresh (ctx, ctx->mem, ctx->os, 0);
    test_assert (w != NULL);

    wal_append_begin_log (w, 1);
    lsn second = wal_append_commit_log (w, 1, 0);
    wal_flush_all (w);
    test_assert_int_equal (wal_close (w, &ctx->e), SUCCESS);

    // Flip a byte in the body of the second record
    {
      i_file f;
      test_assert_int_equal (
          ctx->os.table->open_file_rw (ctx->os.self, &f, WAL_T_FILE, &ctx->e),
          SUCCESS
      );
      u8 b = 0;
      test_assert_int_equal ((i32)f.table->pread_all (f.self, &b, 1, second + 2, &ctx->e), 1);
      b ^= 0xFF;
      test_assert_int_equal (f.table->pwrite_all (f.self, &b, 1, second + 2, &ctx->e), SUCCESS);
      ctx->os.table->close_file (ctx->os.self, f.self, &ctx->e);
    }

    w = wal_open (WAL_T_FILE, ctx->mem, ctx->os, &ctx->e);
    test_assert (w != NULL);

    struct wal_test_scan seen = {0};
    const err_t          ret  = wal_forward_scan (w, LSN_NULL, wal_test_collect, &seen, &ctx->e);

    // The scan hands back the error it is carrying, traced through this frame -
    // not a raw int off the read path
    test_assert (ret != SUCCESS);
    test_assert_int_equal (ret, ctx->e.cause_code);
    test_err_t_check (ctx->e.cause_code, ERR_CORRUPT, &ctx->e);

    // The good record ahead of it still came back
    test_assert_int_equal ((i32)seen.n, 1);

    wal_crash (w, &ctx->e);
    ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);
  }
}
#endif

#ifndef NDEBUG
TEST (wal_read_entry)
{
  u8                      phys[3][NS_PAGE_SIZE];
  struct wal_rec_hdr_read want[16];
  lsn                     at[16];

  TEST_CASE ("every record shape reads back by lsn")
  {
    struct wal *w = wal_test_fresh (ctx, ctx->mem, ctx->os, 4096);
    test_assert (w != NULL);

    const u32 n = wal_test_all_shapes (want, arrlen (want), phys);
    for (u32 i = 0; i < n; ++i) {
      at[i] = wal_test_append (w, &want[i]);
    }
    wal_flush_all (w);

    // Backwards, the way an undo chain walks it
    struct wal_rec_hdr_read got;
    for (u32 i = n; i > 0; --i) {
      test_assert (wal_read_entry (w, &got, at[i - 1], &ctx->e) != NULL);
      test_assert (wal_rec_hdr_read_equal (&got, &want[i - 1]));
    }

    test_assert_int_equal (wal_close (w, &ctx->e), SUCCESS);
    ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);
  }

  TEST_CASE ("a record still in the buffer is flushed to be read")
  {
    struct wal *w = wal_test_fresh (ctx, ctx->mem, ctx->os, 0);
    test_assert (w != NULL);

    // No flush: the reader has its own handle and cannot see this yet
    lsn only = wal_append_begin_log (w, 42);
    test_assert_int_equal ((i32)w->ostream->flushed_lsn, (i32)WAL_RECORDS_START);

    struct wal_rec_hdr_read got;
    test_assert (wal_read_entry (w, &got, only, &ctx->e) != NULL);
    test_assert_int_equal (got.type, WL_BEGIN);
    test_assert_int_equal ((i32)got.begin.tid, 42);
    test_assert_int_equal ((i32)w->ostream->flushed_lsn, (i32)wal_size (w));

    test_assert_int_equal (wal_close (w, &ctx->e), SUCCESS);
    ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);
  }

  TEST_CASE ("with no destination it reads into the wal's own header")
  {
    struct wal *w = wal_test_fresh (ctx, ctx->mem, ctx->os, 0);
    test_assert (w != NULL);

    lsn only = wal_append_commit_log (w, 5, 0);
    wal_flush_all (w);

    struct wal_rec_hdr_read *got = wal_read_entry (w, NULL, only, &ctx->e);
    test_assert (got == &w->rhdr);
    test_assert_int_equal (got->type, WL_COMMIT);
    test_assert_int_equal ((i32)got->commit.tid, 5);

    test_assert_int_equal (wal_close (w, &ctx->e), SUCCESS);
    ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);
  }

  TEST_CASE ("an lsn from a log that was already thrown away is corruption")
  {
    struct wal *w = wal_test_fresh (ctx, ctx->mem, ctx->os, 4096);
    test_assert (w != NULL);

    wal_append_begin_log (w, 1);
    wal_flush_all (w);

    // Anything below this log's start lsn lived in its predecessor
    struct wal_rec_hdr_read got;
    test_assert (wal_read_entry (w, &got, 10, &ctx->e) == NULL);
    test_err_t_check (ctx->e.cause_code, ERR_CORRUPT, &ctx->e);

    // The latch went back on the way out
    test_assert (latch_trylock (&w->l));
    latch_unlock (&w->l);

    test_assert_int_equal (wal_close (w, &ctx->e), SUCCESS);
    ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);
  }

  TEST_CASE ("reading off the end of the log is corruption")
  {
    struct wal *w = wal_test_fresh (ctx, ctx->mem, ctx->os, 0);
    test_assert (w != NULL);

    wal_append_begin_log (w, 1);
    wal_flush_all (w);

    struct wal_rec_hdr_read got;
    test_assert (wal_read_entry (w, &got, wal_start_lsn (w) + wal_size (w), &ctx->e) == NULL);
    test_err_t_check (ctx->e.cause_code, ERR_CORRUPT, &ctx->e);

    test_assert_int_equal (wal_close (w, &ctx->e), SUCCESS);
    ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);
  }

  TEST_CASE ("Flaky os")
  {
    int   fail_after = 0;
    err_t ret;

    do {
      TEST_CASE ("Failure flaky os. Fail after: %d", fail_after)
      {
        // Setup
        struct wal *w = wal_test_fresh (ctx, ctx->mem, ctx->faulty_os, 0);
        test_assert (w != NULL);
        lsn only = wal_append_begin_log (w, 1);
        wal_flush_all (w);

        // Do test
        struct wal_rec_hdr_read got;
        faulty_os_fail_after (ctx->faulty_os, fail_after);
        const bool ok = wal_read_entry (w, &got, only, &ctx->e) != NULL;
        ret           = ok ? SUCCESS : error_trace (&ctx->e);
        test_assert (ok == (ret == SUCCESS));

        // Either way the latch came back
        test_assert (latch_trylock (&w->l));
        latch_unlock (&w->l);

        // Clean up
        faulty_os_reset (ctx->faulty_os);
        error_reset (&ctx->e);
        wal_close (w, &ctx->e);
        error_reset (&ctx->e);
        ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);

        // Next fail after
        fail_after++;
      }
    }
    while (ret != SUCCESS);
  }
}
#endif

#ifndef NDEBUG
TEST (wal_append_log)
{
  u8                      phys[3][NS_PAGE_SIZE];
  struct wal_rec_hdr_read want[16];

  TEST_CASE ("lsns are the running size of the log")
  {
    struct wal *w = wal_test_fresh (ctx, ctx->mem, ctx->os, 1024);
    test_assert (w != NULL);

    const u32 n    = wal_test_all_shapes (want, arrlen (want), phys);

    lsn       prev = 0;
    for (u32 i = 0; i < n; ++i) {
      // Every append hands back where it started, which is the size the log
      // had just before it
      lsn before = wal_size (w);
      lsn got    = wal_test_append (w, &want[i]);

      test_assert_int_equal ((i32)got, (i32)(wal_start_lsn (w) + before));
      test_assert (got > prev);
      test_assert (wal_size (w) > before);
      prev = got;

      // The latch is free again between appends
      test_assert (latch_trylock (&w->l));
      latch_unlock (&w->l);
    }

    test_assert_int_equal (wal_close (w, &ctx->e), SUCCESS);
    ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);
  }

  TEST_CASE ("records larger than the buffer still land whole")
  {
    struct wal *w = wal_test_fresh (ctx, ctx->mem, ctx->os, 0);
    test_assert (w != NULL);

    // Leave the buffer with less room than a physical update needs, so the
    // record has to straddle a flush
    wal_ensure_flush_next (w);
    test_assert_int_equal ((i32)cbuffer_avail (&w->ostream->buffer), 0);

    const u32 n = wal_test_all_shapes (want, arrlen (want), phys);
    lsn       at[16];
    for (u32 i = 0; i < n; ++i) {
      at[i] = wal_test_append (w, &want[i]);
    }
    wal_flush_all (w);

    struct wal_rec_hdr_read got;
    for (u32 i = 0; i < n; ++i) {
      test_assert (wal_read_entry (w, &got, at[i], &ctx->e) != NULL);
      test_assert (wal_rec_hdr_read_equal (&got, &want[i]));
    }

    test_assert_int_equal (wal_close (w, &ctx->e), SUCCESS);
    ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);
  }

  TEST_CASE ("the padding that forces a flush is zeros, not stack garbage")
  {
    struct wal *w = wal_test_fresh (ctx, ctx->mem, ctx->os, 0);
    test_assert (w != NULL);

    // The start lsn is all that is in the log so far, so the pad starts here
    const lsn pad_at = wal_size (w);

    wal_ensure_flush_next (w);
    test_assert_int_equal ((i32)cbuffer_avail (&w->ostream->buffer), 0);
    wal_flush_all (w);

    // wal_ensure_flush_next fills the buffer to force the next append to
    // flush. Whatever it pads with ends up in the log, so it has to be zeros -
    // an uninitialized buffer would write stack contents into the wal.
    const u32 padlen = (u32)(wal_size (w) - pad_at);
    test_assert (padlen > 0);

    u8 *got = ctx->mem.table->malloc (ctx->mem.self, 1, padlen, &ctx->e);
    test_assert (got != NULL);
    wal_test_read_at (ctx, WAL_T_FILE, got, padlen, pad_at);
    for (u32 i = 0; i < padlen; ++i) {
      test_assert_int_equal (got[i], 0);
    }
    ctx->mem.table->free (ctx->mem.self, got);

    test_assert_int_equal (wal_close (w, &ctx->e), SUCCESS);
    ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);
  }

  TEST_CASE ("a log that cannot be written takes the process down")
  {
    int   fail_after = 0;
    err_t ret;

    do {
      TEST_CASE ("Failure flaky os. Fail after: %d", fail_after)
      {
        // Setup: the buffer is full, so the next append has to flush
        struct wal *w = wal_test_fresh (ctx, ctx->mem, ctx->faulty_os, 0);
        test_assert (w != NULL);
        wal_ensure_flush_next (w);

        // Do test
        faulty_os_fail_after (ctx->faulty_os, fail_after);
        capture_intentional_crash (&ret, &ctx->e)
        {
          wal_append_begin_log (w, 1);
          ret = SUCCESS;
        }

        // Manually unlock the wal
        latch_unlock (&w->l);

        // Clean up
        faulty_os_reset (ctx->faulty_os);
        error_reset (&ctx->e);
        wal_crash (w, &ctx->e);
        error_reset (&ctx->e);
        ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);

        // Next fail after
        fail_after++;
      }
    }
    while (ret != SUCCESS);
  }
}
#endif

#ifndef NDEBUG
/// Gives the page sized payloads of every physical record in [batch] random
/// contents, so a round trip can't pass by comparing zeros with zeros
static void
wal_test_fill_batch (struct wal_rec_hdr_read *batch, u32 len)
{
  for (u32 i = 0; i < len; i++) {
    struct wal_rec_hdr_read *r = &batch[i];

    if (r->type == WL_UPDATE && r->update.type == WUP_PHYSICAL) {
      rand_bytes (r->update.phys.undo, NS_PAGE_SIZE);
      rand_bytes (r->update.phys.redo, NS_PAGE_SIZE);
    } else if (r->type == WL_CLR && r->clr.type == WCLR_PHYSICAL) {
      rand_bytes (r->clr.phys.redo, NS_PAGE_SIZE);
    }
  }
}

/// Scans the whole log from the start and checks it holds exactly [want]
static void
wal_test_expect_log (
    struct test_ctx               *ctx,
    struct wal                    *w,
    const struct wal_rec_hdr_read *want,
    u32                            n
)
{
  struct wal_test_scan scan = {0};
  test_assert_int_equal (wal_forward_scan (w, LSN_NULL, wal_test_collect, &scan, &ctx->e), SUCCESS);
  test_assert_type_equal (scan.n, n, u32, PRIu32);

  for (u32 i = 0; i < n; i++) {
    test_assert (wal_rec_hdr_read_equal (&scan.recs[i], &want[i]));
  }
}

struct wal_test_params
{
  struct wal_rec_hdr_read *batch1;
  u32                      batch1_len;
  struct wal_rec_hdr_read *batch2;
  u32                      batch2_len;
};

static void
run_wal_test (struct test_ctx *ctx, const struct wal_test_params *p)
{
  struct wal_rec_hdr_read all[16];
  ASSERT (p->batch1_len + p->batch2_len <= arrlen (all));

  // Setup - an empty log
  struct wal *w = wal_test_fresh (ctx, ctx->mem, ctx->os, 0);
  test_assert (w != NULL);

  // Run - write the first batch, lsns strictly increasing
  lsn l = 0;
  for (u32 i = 0; i < p->batch1_len; i++) {
    const lsn next = wal_test_append (w, &p->batch1[i]);
    test_assert (next > l);
    l = next;
  }
  wal_flush_all (w);

  // It reads back as written
  wal_test_expect_log (ctx, w, p->batch1, p->batch1_len);

  // Append a second batch after the scan
  for (u32 i = 0; i < p->batch2_len; i++) {
    const lsn next = wal_test_append (w, &p->batch2[i]);
    test_assert (next > l);
    l = next;
  }
  wal_flush_all (w);

  // The log is the first batch followed by the second
  memcpy (all, p->batch1, p->batch1_len * sizeof *all);
  memcpy (all + p->batch1_len, p->batch2, p->batch2_len * sizeof *all);
  wal_test_expect_log (ctx, w, all, p->batch1_len + p->batch2_len);

  // Teardown
  test_assert_int_equal (wal_close (w, &ctx->e), SUCCESS);
  ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);
}

TEST (wal)
{
  struct wal_rec_hdr_read batch1_full[] = {
      {.type = WL_BEGIN, .begin = {.tid = 1}},
      {.type = WL_COMMIT, .commit = {.tid = 3, .prev = 20}},
      {.type = WL_END, .end = {.tid = 4, .prev = 30}},
      {
          .type   = WL_UPDATE,
          .update = {.type = WUP_PHYSICAL, .tid = 5, .prev = 40, .phys = {.pg = 111}},
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
          .update = {.type = WUP_PHYSICAL, .tid = 6, .prev = 41, .phys = {.pg = 112}},
      },
  };

  struct wal_rec_hdr_read batch1_begin_only[] = {
      {.type = WL_BEGIN, .begin = {.tid = 1}},
  };

  struct wal_rec_hdr_read batch2_begin_only[] = {
      {.type = WL_BEGIN, .begin = {.tid = 2}},
  };

  struct wal_rec_hdr_read batch1_no_begin[] = {
      {.type = WL_COMMIT, .commit = {.tid = 3, .prev = 20}},
      {.type = WL_END, .end = {.tid = 4, .prev = 30}},
      {
          .type   = WL_UPDATE,
          .update = {.type = WUP_FSM, .tid = 5, .prev = 40, .fsm = {.pg = 0, .bit = 9, .redo = 1}},
      },
      {
          .type = WL_CLR,
          .clr  = {.type = WCLR_DUMMY, .tid = 6, .prev = 50, .undo_next = 42},
      },
  };

  const struct
  {
    const char            *name;
    struct wal_test_params p;
  } cases[] = {
      {"full", {batch1_full, arrlen (batch1_full), batch2_full, arrlen (batch2_full)}},
      {"begin only",
       {batch1_begin_only,
        arrlen (batch1_begin_only),
        batch2_begin_only,
        arrlen (batch2_begin_only)}},
      {"no begin", {batch1_no_begin, arrlen (batch1_no_begin), batch2_full, arrlen (batch2_full)}},
  };

  for (u32 i = 0; i < arrlen (cases); i++) {
    TEST_CASE ("%s", cases[i].name)
    {
      wal_test_fill_batch (cases[i].p.batch1, cases[i].p.batch1_len);
      wal_test_fill_batch (cases[i].p.batch2, cases[i].p.batch2_len);

      run_wal_test (ctx, &cases[i].p);
    }
  }
}

TEST (wal_single_entry)
{
  u8                      phys[3][NS_PAGE_SIZE];
  struct wal_rec_hdr_read cases[16];
  const u32               n = wal_test_all_shapes (cases, arrlen (cases), phys);

  for (u32 i = 0; i < n; i++) {
    TEST_CASE ("%" PRIu32 ": %s", i, wal_rec_hdr_type_tostr (cases[i].type))
    {
      // Setup
      struct wal *w = wal_test_fresh (ctx, ctx->mem, ctx->os, 0);
      test_assert (w != NULL);

      // Run - one record, written then read back both ways
      const lsn l = wal_test_append (w, &cases[i]);
      test_assert_type_equal (l, WAL_RECORDS_START, lsn, PRlsn);
      wal_flush_all (w);
      i_print_wal_rec_hdr_read_light (&cases[i], l);

      wal_test_expect_log (ctx, w, &cases[i], 1);

      struct wal_rec_hdr_read got;
      test_assert (wal_read_entry (w, &got, l, &ctx->e) != NULL);
      test_assert (wal_rec_hdr_read_equal (&got, &cases[i]));

      // Teardown
      test_assert_int_equal (wal_close (w, &ctx->e), SUCCESS);
      ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);
    }
  }
}

struct wal_queue
{
  _Atomic u32              sync;
  struct wal              *ww;
  atomic_int               idx;

  struct wal_rec_hdr_read *read;
  int                      len;
};

/// Appends records off the shared queue until it runs dry, flushing after
/// each one so flushes race with other threads' appends
static void *
wal_thread (void *vctx)
{
  struct wal_queue *q = vctx;

  while (atomic_load (&q->sync) > 0) {
    spin_pause ();
  }

  while (true) {
    const int idx = atomic_fetch_add (&q->idx, 1);
    if (idx >= q->len) {
      return NULL;
    }

    wal_test_append (q->ww, &q->read[idx]);
    wal_flush_all (q->ww);
  }
}

struct wal_mt_scan
{
  struct wal_rec_hdr_read *read;
  u32                      len;
  bool                    *seen;
  u32                      read_back;
  u32                      found;

  // Records land roughly in queue order, so start each search just past the
  // last match
  u32                      finger;
};

/// Checks the record read back is one of the ones the threads wrote, and
/// that no record shows up twice
static err_t
wal_mt_match (struct wal_rec_hdr_read *rec, lsn l, void *vctx, error *e)
{
  (void)l;
  (void)e;
  struct wal_mt_scan *s = vctx;
  s->read_back++;

  for (u32 k = 0; k < s->len; ++k) {
    const u32 idx = (s->finger + k) % s->len;
    if (!s->seen[idx] && wal_rec_hdr_read_equal (rec, &s->read[idx])) {
      s->seen[idx] = true;
      s->finger    = (idx + 1) % s->len;
      s->found++;
      return SUCCESS;
    }
  }

  return SUCCESS;
}

TEST (wal_multi_threaded)
{
  const u32   N  = 500;

  // Setup - N random records and an empty log
  struct wal *ww = wal_test_fresh (ctx, ctx->mem, ctx->os, 0);
  test_assert (ww != NULL);

  struct wal_rec_hdr_read *read = ctx->mem.table->malloc (ctx->mem.self, N, sizeof *read, &ctx->e);
  test_assert (read != NULL);
  bool *seen = ctx->mem.table->calloc (ctx->mem.self, N, sizeof *seen, &ctx->e);
  test_assert (seen != NULL);

  for (u32 i = 0; i < N; ++i) {
    wal_rec_hdr_read_random (&read[i]);
  }

  struct wal_queue queue = {
      .sync = 1,
      .ww   = ww,
      .idx  = 0,
      .read = read,
      .len  = (int)N,
  };

  // Run - every thread appends at once
  u32      nthreads;
  i_thread threads[10];
  for (nthreads = 0; nthreads < arrlen (threads); ++nthreads) {
    test_assert_int_equal (
        ctx->os.table
            ->thread_create (ctx->os.self, &threads[nthreads], wal_thread, &queue, &ctx->e),
        SUCCESS
    );
  }

  atomic_store (&queue.sync, 0);

  for (; nthreads > 0; --nthreads) {
    ctx->os.table->thread_join (ctx->os.self, threads[nthreads - 1].self);
  }

  // Every record is in the log exactly once, and nothing else is
  struct wal_mt_scan scan =
      {.read = read, .len = N, .seen = seen, .read_back = 0, .found = 0, .finger = 0};
  test_assert_int_equal (wal_forward_scan (ww, LSN_NULL, wal_mt_match, &scan, &ctx->e), SUCCESS);
  test_assert_type_equal (scan.read_back, N, u32, PRIu32);
  test_assert_type_equal (scan.found, N, u32, PRIu32);

  // Teardown
  test_assert_int_equal (wal_close (ww, &ctx->e), SUCCESS);
  ctx->os.table->remove_quiet (ctx->os.self, WAL_T_FILE, &ctx->e);
  ctx->mem.table->free (ctx->mem.self, seen);
  ctx->mem.table->free (ctx->mem.self, read);
}
#endif
