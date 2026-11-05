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

#include <stdatomic.h>
#include <string.h>

static err_t
wal_init (struct wal *dest, error *e)
{
  dest->ostream = NULL;
  dest->istream = NULL;
  dest->flags   = 0;
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
  ASSERT (w->flags & WAL_ISNEW);

  // Use a temporary ostream
  {
    ASSERT (w->ostream == NULL);
    struct wal_ostream *ostream = walos_open (w->fname.data, w->mem, w->os, 0, e);
    if (ostream == NULL) {
      return error_trace (e);
    }
    walos_write_all (ostream, NULL, &start_lsn, sizeof (start_lsn));
    if (walos_close (ostream, e) < 0) {
      return error_trace (e);
    }
  }

  w->start_lsn = start_lsn;
  w->flags &= ~WAL_ISNEW;

  // Advance istream forward one lsn worth
  {
    // We should only be at position 0 calling this
    lsn curlsn = walis_mark (w->istream);
    ASSERT (curlsn == 0);

    // Do read
    lsn read_start_lsn;
    if (walis_read_all (w->istream, NULL, &read_start_lsn, sizeof (start_lsn), e) < 0) {
      return error_trace (e);
    }

    // Verify we can read what we just wrote
    if (start_lsn != read_start_lsn) {
      return error_causef (e, ERR_CORRUPT, "Failed to read start lsn just written");
    }

    // We should now be at position sizeof(start_lsn)
    curlsn = walis_mark (w->istream);
    ASSERT (curlsn == sizeof (start_lsn));
  }

  return SUCCESS;
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

  if (string_copy (&dest->fname, strfcstr (fname), mem, e)) {
    goto err_free_dest;
  }

  if (wal_init (dest, e)) {
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
  if (w->ostream) {
    wal_flush_all (w);
    walos_close (w->ostream, e);
    w->ostream = NULL;
  }
  if (w->istream) {
    walis_close (w->istream, e);
    w->istream = NULL;
  }

  if (w->fname.data) {
    w->mem.table->free (w->mem.self, (void *)w->fname.data);
    w->fname.data = NULL;
  }
  return error_trace (e);
}

err_t
wal_close (struct wal *w, error *e)
{
  struct i_mem mem = w->mem;
  wal_destroy (w, e);
  mem.table->free (mem.self, w);
  return error_trace (e);
}

err_t
wal_close_and_delete (struct wal *w, error *e)
{
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

  // Copy fname to pass in
  struct string fname = w->fname;

  // Set to NULL so we don't free it
  w->fname.data       = NULL;

  if (wal_destroy (w, e)) {
    latch_unlock (&w->l);
    return error_trace (e);
  }

  if (w->os.table->remove_quiet (w->os.self, fname.data, e)) {
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

  if (w->ostream) {
    walos_crash (w->ostream, e);
  }
  if (w->istream) {
    walis_crash (w->istream, e);
  }
  if (w->fname.data) {
    mem.table->free (mem.self, (void *)w->fname.data);
  }
  mem.table->free (mem.self, w);

  return SUCCESS;
}

#ifndef NDEBUG
void
wal_ensure_flush_next (struct wal *w)
{
  u8  buffer[WAL_BUFFER_CAP];
  u32 len = cbuffer_len (&w->ostream->buffer);
  ASSERT (len <= WAL_BUFFER_CAP);
  u32 towrite = WAL_BUFFER_CAP - len;
  if (towrite > 0) {
    error e = error_create ();
    walos_write_all (w->ostream, NULL, buffer, towrite);
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
    u32 toread = total_len - (head - buf) - sizeof (u32);
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
    return error_causef (e, ERR_CORRUPT, "Invalid CRC: %d != %d", *checksum, actual_crc);
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
  UNREACHABLE ();
}

err_t
wal_seek_unsafe (struct wal *w, lsn id, error *e)
{
  DBG_ASSERT (wal, w);

  if (id == LSN_NULL) {
    return walis_seek (w->istream, LSN_NULL, e);
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

  // Seek
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

    // Read one full log
    int iseof = wal_read_one (w, &w->rhdr, e);

    // Error
    if (iseof < 0) {
      return iseof;
    }

    // End of log file - transition to random access mode
    else if (iseof) {
      if (w->ostream == NULL) {
        // Open the ostream where the read left off
        w->ostream = walos_open (w->fname.data, w->mem, w->os, curlsn, e);
      }
      return error_trace (e);
    }

    else {
      // Else, execute the user supplied function
      if (func (&w->rhdr, w->start_lsn + curlsn, ctx, e) < 0) {
        return error_trace (e);
      }
    }
  }
}

struct wal_rec_hdr_read *
wal_read_entry (struct wal *w, struct wal_rec_hdr_read *dest, const lsn id, error *e)
{
  struct wal_rec_hdr_read *ret = NULL;
  if (dest) {
    ret = dest;
  } else {
    ret = &w->rhdr;
  }

  latch_lock (&w->l);

  if (wal_seek_unsafe (w, id, e) < 0) {
    goto failed;
  }

  // Read
  int iseof = wal_read_one (w, dest, e);
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
  w->whdr.type     = WL_BEGIN;
  w->whdr.begin    = (struct wal_begin){.tid = tid};
  const lsn result = wal_write_locked (w);
  latch_unlock (&w->l);
  return result;
}

lsn
wal_append_commit_log (struct wal *w, const txid tid, const lsn prev)
{
  latch_lock (&w->l);
  DBG_ASSERT (wal, w);
  w->whdr.type     = WL_COMMIT;
  w->whdr.commit   = (struct wal_commit){.tid = tid, .prev = prev};
  const lsn result = wal_write_locked (w);
  latch_unlock (&w->l);
  return result;
}

lsn
wal_append_end_log (struct wal *w, const txid tid, const lsn prev)
{
  latch_lock (&w->l);
  DBG_ASSERT (wal, w);
  w->whdr.type     = WL_END;
  w->whdr.end      = (struct wal_end){.tid = tid, .prev = prev};
  const lsn result = wal_write_locked (w);
  latch_unlock (&w->l);
  return result;
}

lsn
wal_append_update_log (struct wal *w, const struct wal_update_write update)
{
  latch_lock (&w->l);
  DBG_ASSERT (wal, w);
  w->whdr.type     = WL_UPDATE;
  w->whdr.update   = update;
  const lsn result = wal_write_locked (w);
  latch_unlock (&w->l);
  return result;
}

lsn
wal_append_clr_log (struct wal *w, const struct wal_clr_write clr)
{
  latch_lock (&w->l);
  DBG_ASSERT (wal, w);
  w->whdr.type     = WL_CLR;
  w->whdr.clr      = clr;
  const lsn result = wal_write_locked (w);
  latch_unlock (&w->l);
  return result;
}

// #ifndef NDEBUG
//
// static lsn
// wal_append_log (struct wal *w, const struct wal_rec_hdr_write *whdr)
// {
//   switch (whdr->type) {
//     case WL_BEGIN: {
//       return wal_append_begin_log (w, whdr->begin.tid);
//     }
//     case WL_COMMIT: {
//       return wal_append_commit_log (w, whdr->commit.tid, whdr->commit.prev);
//     }
//     case WL_END: {
//       return wal_append_end_log (w, whdr->end.tid, whdr->end.prev);
//     }
//     case WL_UPDATE: {
//       return wal_append_update_log (w, whdr->update);
//     }
//     case WL_CLR: {
//       return wal_append_clr_log (w, whdr->clr);
//     }
//   }
//
//   UNREACHABLE ();
// }
//
// struct wal_queue
// {
//   _Atomic u32              sync;
//   struct wal              *ww;
//   atomic_int               idx;
//
//   struct wal_rec_hdr_read *read;
//   int                      len;
// };
//
// static void *
// wal_thread (void *ctx)
// {
//   error             e = error_create ();
//   struct wal_queue *q = ctx;
//
//   while (atomic_load (&q->sync) > 0) {
//     spin_pause ();
//   }
//
//   while (true) {
//     int idx = atomic_fetch_add (&q->idx, 1);
//     if (idx >= q->len) {
//       return NULL;
//     }
//
//     struct wal_rec_hdr_write write = wrhw_from_wrhr (&q->read[idx]);
//
//     const lsn                l     = wal_append_log (q->ww, &write);
//
//     wal_flush_all (q->ww);
//   }
// }
//
// struct wal_rec_hdr_read *
// wal_read_first (struct wal *w, error *e)
// {
//   return wal_read_entry (w, NULL, sizeof (lsn), e);
// }
//
// TEST_DISABLED (wal_multi_threaded)
// {
//   ctx->os.table->remove_quiet (ctx->os.self, "test.wal", &ctx->e);
//   struct wal *ww = wal_open ("test.wal", ctx->mem, ctx->os, &ctx->e);
//   wal_write_start_lsn (ww, 0, &ctx->e);
//
//   u32                      N    = 5000;
//
//   struct wal_rec_hdr_read *read = ctx->mem.table->malloc (ctx->mem.self, N, sizeof *read,
//   &ctx->e);
//
//   for (u32 i = 0; i < N; ++i) {
//     wal_rec_hdr_read_random (&read[i]);
//   }
//
//   struct wal_queue queue = {
//       .sync = 1,
//       .ww   = ww,
//       .idx  = 0,
//       .read = read,
//       .len  = N,
//   };
//
//   u32      nthreads;
//   i_thread threads[10];
//   for (nthreads = 0; nthreads < arrlen (threads); ++nthreads) {
//     ctx->os.table->thread_create (ctx->os.self, &threads[nthreads], wal_thread, &queue, &ctx->e);
//   }
//
//   // launch
//   atomic_store (&queue.sync, 0);
//
//   i_log_info ("Threads active\n");
//
//   for (; nthreads > 0; --nthreads) {
//     ctx->os.table->thread_join (ctx->os.self, threads[nthreads - 1].self);
//   }
//
//   // To speed up searches, keep a "finger" which is "near" the
//   // most recent found log
//   u32 finger   = 0;
//   lsn read_lsn = 0;
//
//   for (u32 i = 0; i < N; ++i) {
//     struct wal_rec_hdr_read *actual = wal_read_next (ww, &read_lsn, &ctx->e);
//     i_print_wal_rec_hdr_read_light (actual, read_lsn);
//     test_assert (actual->type != WL_EOF);
//
//     // Search through all records to ensure it's there
//     bool found = false;
//     for (u32 k = 0; k < N; ++k) {
//       u32 idx = (finger + k) % N;
//       if (wal_rec_hdr_read_equal (actual, &read[idx])) {
//         finger         = (idx + 1) % N;
//         read[idx].type = WL_EOF;
//         found          = true;
//         break;
//       }
//     }
//     test_assert (found);
//   }
//
//   const struct wal_rec_hdr_read *actual = wal_read_next (ww, &read_lsn, &ctx->e);
//   test_assert_int_equal (actual->type, WL_EOF);
//
//   wal_close (ww, &ctx->e);
//   ctx->mem.table->free (ctx->mem.self, read);
// }
//
// struct wal_test_params
// {
//   const char              *fname;
//   struct wal_rec_hdr_read *batch1;
//   u32                      batch1_len;
//   struct wal_rec_hdr_read *batch2;
//   u32                      batch2_len;
// };
//
// static void
// wal_test_fill_batch (struct wal_rec_hdr_read *batch, u32 len)
// {
//   // I just removed error for this because it wasn't being used -
//   // no other reason - maybe breaks
//
//   for (u32 i = 0; i < len; i++) {
//     struct wal_rec_hdr_read *r = &batch[i];
//
//     switch (r->type) {
//       case WL_UPDATE: {
//         rand_bytes (r->update.phys.undo, NS_PAGE_SIZE);
//         rand_bytes (r->update.phys.redo, NS_PAGE_SIZE);
//         break;
//       }
//       case WL_CLR: {
//         rand_bytes (r->clr.phys.redo, NS_PAGE_SIZE);
//         break;
//       }
//       default: {
//         break;
//       }
//     }
//   }
// }
//
// static void
// wal_test_free_batch (const struct wal_rec_hdr_read *batch, u32 len)
// {
//   for (u32 i = 0; i < len; i++) {
//     const struct wal_rec_hdr_read *r = &batch[i];
//     test_assert (r != NULL);
//   }
// }
//
// static void
// run_wal_test (const struct wal_test_params *p, const struct i_os os)
// {
//   error        e   = error_create ();
//   struct i_mem mem = default_mem ();
//
//   os.table->remove_quiet (os.self, p->fname, &e);
//   struct wal *ww = wal_open (p->fname, mem, os, &e);
//   wal_write_start_lsn (ww, 0, &e);
//   /**
//    * Write all the input logs
//    */
//   {
//     slsn l = -1;
//     for (u32 i = 0; i < p->batch1_len; i++) {
//       struct wal_rec_hdr_write out   = wrhw_from_wrhr (&p->batch1[i]);
//       slsn                     nextl = wal_append_log (ww, &out);
//       test_assert (nextl >= 0);
//       test_assert (nextl > l);
//       l = nextl;
//     }
//     wal_flush_all (ww);
//   }
//
//   /**
//    * Read all the input logs and expect
//    * that they are the same as the
//    * first batch written ones
//    */
//   {
//     for (u32 i = 0; i < p->batch1_len; i++) {
//       lsn                      read_lsn;
//       struct wal_rec_hdr_read *next = NULL;
//       if (i == 0) {
//         next = wal_read_first (ww, &e);
//       } else {
//         next = wal_read_next (ww, &read_lsn, &e);
//       }
//       test_assert (wal_rec_hdr_read_equal (next, &p->batch1[i]));
//     }
//   }
//
//   /**
//    * Write a second batch of input logs
//    */
//   {
//     slsn l = 0;
//     for (u32 i = 0; i < p->batch2_len; i++) {
//       struct wal_rec_hdr_write out = wrhw_from_wrhr (&p->batch2[i]);
//       l                            = wal_append_log (ww, &out);
//       test_assert (l > 0);
//     }
//     wal_flush_all (ww);
//   }
//
//   /**
//    * Read from the start and confirm all the logs
//    */
//   {
//     for (u32 i = 0; i < p->batch1_len; i++) {
//       lsn                      read_lsn;
//       struct wal_rec_hdr_read *next = NULL;
//       if (i == 0) {
//         next = wal_read_first (ww, &e);
//       } else {
//         next = wal_read_next (ww, &read_lsn, &e);
//       }
//       test_assert (wal_rec_hdr_read_equal (next, &p->batch1[i]));
//     }
//
//     for (u32 i = 0; i < p->batch2_len; i++) {
//       lsn                      read_lsn;
//       struct wal_rec_hdr_read *next = wal_read_next (ww, &read_lsn, &e);
//       test_assert (wal_rec_hdr_read_equal (next, &p->batch2[i]));
//     }
//   }
//
//   wal_close (ww, &e);
// }
//
// ////////////////////////////////////////////////////////////
// // WAL test cases
//
// TEST_DISABLED (wal)
// {
//   struct wal_rec_hdr_read batch1_full[] = {
//       {.type = WL_BEGIN, .begin = {.tid = 1}},
//       {.type = WL_COMMIT, .commit = {.tid = 3, .prev = 20}},
//       {.type = WL_END, .end = {.tid = 4, .prev = 30}},
//       {
//           .type = WL_UPDATE,
//           .update =
//               {
//                   .type = WUP_PHYSICAL,
//                   .tid  = 5,
//                   .prev = 40,
//                   .phys = {.pg = 111},
//               },
//       },
//       {
//           .type = WL_CLR,
//           .clr  = {
//               .type      = WCLR_PHYSICAL,
//               .tid       = 6,
//               .prev      = 50,
//               .undo_next = 42,
//               .phys      = {.pg = 222},
//           },
//       },
//   };
//
//   struct wal_rec_hdr_read batch2_full[] = {
//       {.type = WL_BEGIN, .begin = {.tid = 2}},
//       {
//           .type   = WL_UPDATE,
//           .update = {
//               .type = WUP_PHYSICAL,
//               .tid  = 6,
//               .prev = 41,
//               .phys = {.pg = 112},
//           },
//       },
//   };
//
//   struct wal_rec_hdr_read batch1_begin_only[] = {
//       {.type = WL_BEGIN, .begin = {.tid = 1}},
//   };
//
//   struct wal_rec_hdr_read batch2_begin_only[] = {
//       {.type = WL_BEGIN, .begin = {.tid = 2}},
//   };
//
//   struct wal_rec_hdr_read batch1_no_ckpt[] = {
//       {.type = WL_BEGIN, .begin = {.tid = 1}},
//       {.type = WL_COMMIT, .commit = {.tid = 3, .prev = 20}},
//       {.type = WL_END, .end = {.tid = 4, .prev = 30}},
//       {
//           .type = WL_UPDATE,
//           .update =
//               {
//                   .type = WUP_PHYSICAL,
//                   .tid  = 5,
//                   .prev = 40,
//                   .phys = {.pg = 111},
//               },
//       },
//       {
//           .type = WL_CLR,
//           .clr  = {
//               .type      = WCLR_PHYSICAL,
//               .tid       = 6,
//               .prev      = 50,
//               .undo_next = 42,
//               .phys      = {.pg = 222},
//           },
//       },
//   };
//
//   struct wal_test_params cases[] = {
//       {
//           .fname      = "test_full.wal",
//           .batch1     = batch1_full,
//           .batch1_len = arrlen (batch1_full),
//           .batch2     = batch2_full,
//           .batch2_len = arrlen (batch2_full),
//       },
//       {
//           .fname      = "test_begin_only.wal",
//           .batch1     = batch1_begin_only,
//           .batch1_len = arrlen (batch1_begin_only),
//           .batch2     = batch2_begin_only,
//           .batch2_len = arrlen (batch2_begin_only),
//       },
//       {
//           .fname      = "test_no_ckpt.wal",
//           .batch1     = batch1_no_ckpt,
//           .batch1_len = arrlen (batch1_no_ckpt),
//           .batch2     = batch2_full,
//           .batch2_len = arrlen (batch2_full),
//       },
//   };
//
//   for (u32 i = 0; i < arrlen (cases); i++) {
//     TEST_CASE ("Wal: %d", i)
//     {
//       const struct wal_test_params *c = &cases[i];
//
//       wal_test_fill_batch (c->batch1, c->batch1_len);
//       wal_test_fill_batch (c->batch2, c->batch2_len);
//
//       run_wal_test (c, ctx->os);
//
//       wal_test_free_batch (c->batch1, c->batch1_len);
//       wal_test_free_batch (c->batch2, c->batch2_len);
//     }
//   }
// }
//
// TEST_DISABLED (wal_single_entry)
// {
//   struct wal_rec_hdr_read cases[] = {
//       {.type = WL_BEGIN, .begin = {.tid = 1}},
//       {.type = WL_COMMIT, .commit = {.tid = 2, .prev = 10}},
//       {.type = WL_END, .end = {.tid = 3, .prev = 20}},
//       {.type   = WL_UPDATE,
//        .update = {.type = WUP_PHYSICAL, .tid = 4, .prev = 30, .phys = {.pg = 111}}},
//       {.type = WL_CLR,
//        .clr  = {.type = WCLR_PHYSICAL, .tid = 5, .prev = 40, .undo_next = 42, .phys = {.pg =
//        222}}},
//   };
//
//   for (u32 i = 0; i < arrlen (cases); i++) {
//     TEST_CASE ("wal_single_entry: %d", i)
//     {
//       struct wal_rec_hdr_read *c = &cases[i];
//
//       wal_test_fill_batch (c, 1);
//
//       ctx->os.table->remove_quiet (ctx->os.self, "test_single_entry.wal", &ctx->e);
//       struct wal *ww = wal_open ("test_single_entry.wal", ctx->mem, ctx->os, &ctx->e);
//       wal_write_start_lsn (ww, 0, &ctx->e);
//
//       // WRITE
//       struct wal_rec_hdr_write out = wrhw_from_wrhr (c);
//       const slsn               l   = wal_append_log (ww, &out);
//       test_assert (l >= 0);
//
//       wal_flush_all (ww);
//
//       // READ
//       struct wal_rec_hdr_read *next = wal_read_first (ww, &ctx->e);
//       test_assert (wal_rec_hdr_read_equal (next, c));
//
//       wal_close (ww, &ctx->e);
//
//       wal_test_free_batch (c, 1);
//     }
//   }
// }
//
// #endif
