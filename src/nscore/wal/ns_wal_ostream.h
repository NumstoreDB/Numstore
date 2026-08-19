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
#include "core/ns_concurrency.h"
#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "core/os/ns_filesystem.h"
#include "core/os/ns_memory.h"

/******************************************************************************
 * SECTION: Wal Output Stream
 * ----------------------------------------------------------------------------
 * @brief Writes out a wal
 ******************************************************************************/

struct wal_ostream
{
  struct i_mem   mem;
  i_file         fd;
  latch          l;
  lsn            flushed_lsn;

  struct cbuffer buffer;
  u8             _buffer[WAL_BUFFER_CAP];
};

// Lifecycle
struct wal_ostream *walos_open (
    const char          *fname,
    struct i_mem         mem,
    struct i_file_system fs,
    error               *e
);
err_t walos_close (struct wal_ostream *w, error *e);

// Flush
err_t walos_flush_all (struct wal_ostream *w, error *e);

// Write
err_t walos_write_all (struct wal_ostream *w, u32 *checksum, const void *data, u32 len, error *e);
lsn walos_get_next_lsn (struct wal_ostream *w);
slsn walos_truncate (struct wal_ostream *w, error *e);

err_t walos_crash (struct wal_ostream *w, error *e);

#endif
