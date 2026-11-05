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

#ifndef NS_WAL_OSTREAM_H
#define NS_WAL_OSTREAM_H

#include "core/ns_cbuffer.h"
#include "core/ns_error.h"
#include "core/ns_latch.h"
#include "core/ns_stdtypes.h"
#include "core/os/ns_malloc.h"
#include "core/os/ns_os.h"

#include <stdbool.h>

struct wal_ostream
{
  struct i_mem   mem;
  struct i_os    os; // the OS that opened [fd] - it must close it too
  i_file         fd;
  latch          l;
  lsn            flushed_lsn;

  error          e;

  struct cbuffer buffer;
  u8             _buffer[WAL_BUFFER_CAP];
};

// Lifecycle
struct wal_ostream *walos_open (
    const char  *fname,
    struct i_mem mem,
    struct i_os  os,
    lsn          seek_to,
    error       *e
);
err_t walos_close (struct wal_ostream *w, error *e);
err_t walos_crash (struct wal_ostream *w, error *e);

// Primary API
void walos_flush_all (struct wal_ostream *w);
void walos_write_all (struct wal_ostream *w, u32 *checksum, const void *data, u32 len);
lsn walos_get_lsn (struct wal_ostream *w);
slsn walos_truncate (struct wal_ostream *w, error *e);

/// A failed write or fsync leaves a hole in the log, and neither of the two
/// calls above has anywhere to report that - they run in the middle of
/// encoding a record, under latches their caller owns. So the failure is
/// latched into the stream, which then stops touching the file altogether.
///
/// The caller checks this at its own top level, where it still owns every
/// latch it took and can drop them before taking the process down.
/// [walos_take_err] moves the latched error out for the log line on the way.
bool walos_failed (const struct wal_ostream *w);
err_t walos_take_err (struct wal_ostream *w, error *e);

#endif
