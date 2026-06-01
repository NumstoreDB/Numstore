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

#include "nscore/wal_ostream.h"

#include <c_specx.h>

#include "nscore/compile_config.h"

DEFINE_DBG_ASSERT (struct wal_ostream, wal_ostream, w, { ASSERT (w); })

////////////////////////////////////////////////////////////
/// Lifecycle

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

////////////////////////////////////////////////////////////
/// Flush

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
walos_flush_to (struct wal_ostream *w, const lsn l, error *e)
{
  latch_lock (&w->l);
  err_t ret = SUCCESS;
  if (l > w->flushed_lsn)
  {
    ret = walos_flush_impl (w, e);
  }
  latch_unlock (&w->l);
  return ret;
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

////////////////////////////////////////////////////////////
/// Write

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
