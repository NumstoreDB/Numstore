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

#ifndef NS_WAL_ISTREAM_H
#define NS_WAL_ISTREAM_H

#include "core/ns_error.h"
#include "core/ns_latch.h"
#include "core/os/ns_malloc.h"
#include "core/os/ns_os.h"

struct wal_istream
{
  struct i_mem mem;
  struct i_os  os;
  i_file       fd;
  lsn          curlsn;
  lsn          lsnidx;

  latch        l;
};

struct wal_istream *walis_open (const char *fname, struct i_mem mem, struct i_os os, error *e);
err_t walis_close (struct wal_istream *w, error *e);
err_t walis_crash (struct wal_istream *w, error *e);

#define WAL_NOT_EOF 0
#define WAL_EOF     1
#define WAL_TERM    2
int walis_read_all (struct wal_istream *w, u32 *checksum, void *dest, u32 len, error *e);
err_t walis_seek (struct wal_istream *w, lsn pos, error *e);
lsn walis_mark (struct wal_istream *w); // Get start lsn

#endif
