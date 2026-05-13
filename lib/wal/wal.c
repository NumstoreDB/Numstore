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

#include "wal/wal.h"

#include "c_specx.h"
#include "c_specx/dev/error.h"
#include "c_specx/ds/string.h"
#include "c_specx/intf/logging.h"
#include "c_specx/intf/os/file_system.h"
#include "c_specx_dev.h"
#include "dpgt/dirty_page_table.h"
#include "txns/txn_table.h"
#include "wal/wal_istream.h"
#include "wal/wal_ostream.h"
#include "wal/wal_rec_hdr.h"

#include <string.h>

static err_t
wal_init (struct wal *dest, error *e)
{
  dest->ostream = NULL;
  dest->istream = NULL;
  dest->flags = 0;
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
  bool iseof;
  u32 checksum;
  lsn start_lsn;

  walis_mark_start_log (dest->istream);

  if (walis_read_all (dest->istream, &iseof, NULL, &checksum, &start_lsn, sizeof (start_lsn), e))
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
wal_flush_to (const struct wal *w, const lsn l, error *e)
{
  DBG_ASSERT (wal, w);
  ASSERT (w->ostream);
  return walos_flush_to (w->ostream, l, e);
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
