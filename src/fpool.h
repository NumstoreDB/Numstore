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

#ifndef FPOOL_H
#define FPOOL_H

/******************************************************************************
 * SECTION: File Pool
 * ----------------------------------------------------------------------------
 *
 * @brief A buffered pool of files
 ******************************************************************************/

#include "compile_config.h"
#include "concurrency.h"
#include "os.h"

#define KTYPE  u64
#define VTYPE  u32
#define SUFFIX fid
#include "robin_hood_ht.h"
#undef KTYPE
#undef VTYPE
#undef SUFFIX

typedef u64 (*fname_to_fid_map) (void *ctx, const char *fname);

struct fpool_frame
{
  i_file fp;
  u32    flags;
  u32    pin;
  latch  ctrl;
};

struct fpool
{
  fname_to_fid_map mapping;
  u32              nframes;

  hash_table_fid     table;
  hentry_fid         _entries[MAX_OPEN_FILES];
  struct fpool_frame frames[MAX_OPEN_FILES];
};

struct fpool *fpool_create (u32 nframes, fname_to_fid_map map, error *e);

err_t fpool_free (struct fpool *fp, error *e);

i_file *fpool_get (struct fpool *fp, const char *fname, error *e);

void fpool_release (struct fpool *fp, i_file *f);

#endif
