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

#ifndef WAL_H
#define WAL_H

#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_latch.h"
#include "core/ns_stdtypes.h"
#include "core/ns_string.h"
#include "core/os/ns_malloc.h"
#include "core/os/ns_os.h"
#include "nscore/wal/ns_wal_ostream.h"
#include "nscore/wal/ns_wal_record.h"

#include <stdbool.h>

#define WAL_RECORDS_POS   sizeof (lsn)
#define WAL_START_LSN_POS 0

enum wal_flags
{
  WAL_ISNEW = (1 << 0),
};

struct wal
{
  struct i_mem             mem;
  struct i_os              os;

  struct string            fname;

  struct wal_ostream      *ostream;
  struct wal_istream      *istream;

  struct wal_rec_hdr_read  rhdr;
  struct wal_rec_hdr_write whdr;

  int                      flags;
  lsn                      start_lsn;

  latch                    l;
};

DEFINE_DBG_ASSERT (struct wal, wal, w, {
  ASSERT (w);
  // ostream can be null
  ASSERT (w->istream);
})

// Lifecycle
struct wal *wal_open (const char *fname, struct i_mem mem, struct i_os os, error *e);
err_t wal_close (struct wal *w, error *e);
err_t wal_crash (struct wal *w, error *e);
err_t wal_close_and_delete (struct wal *w, error *e);
err_t wal_delete_and_reopen (struct wal *w, error *e);
err_t wal_write_start_lsn (struct wal *w, lsn start_lsn, error *e);

// Getters
#define wal_isnew(w)     (((w)->flags & WAL_ISNEW) != 0)
#define wal_start_lsn(w) ((w)->start_lsn)
#define wal_size(w)      (walos_get_lsn ((w)->ostream))
#define wal_flush_all(w) (walos_flush_all ((w)->ostream))
slsn wal_get_end_lsn (struct wal *w, error *e);

// Reading
typedef err_t (*wal_log_func) (struct wal_rec_hdr_read *log, lsn l, void *ctx, error *e);
err_t wal_forward_scan (struct wal *w, lsn start, wal_log_func func, void *ctx, error *e);
struct wal_rec_hdr_read *wal_read_entry (
    struct wal              *w,
    struct wal_rec_hdr_read *dest,
    const lsn                id,
    error                   *e
);

// Appending
lsn wal_append_begin_log (struct wal *w, txid tid);
lsn wal_append_commit_log (struct wal *w, txid tid, lsn prev);
lsn wal_append_end_log (struct wal *w, txid tid, lsn prev);
lsn wal_append_ckpt_begin (struct wal *w);
lsn wal_append_update_log (struct wal *w, struct wal_update_write update);
lsn wal_append_clr_log (struct wal *w, struct wal_clr_write clr);

#ifndef NDEBUG
void wal_ensure_flush_next (struct wal *w);
#endif

#endif // WAL_H
