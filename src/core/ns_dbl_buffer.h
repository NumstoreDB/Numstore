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

#ifndef COLLECTIONS_H
#define COLLECTIONS_H

#include "alloc.h"       // slab alloc
#include "concurrency.h" // latch
#include "csx_assert.h"
#include "error.h"    // err_t
#include "os.h"       // i_file
#include "platform.h" // HEADER_FUNC
#include "stdtypes.h" // u32 ...etc

struct dbl_buffer
{
  latch             latch;
  void             *data;
  u32               size;
  u32               nelem_cap;
  u32               nelem;
  struct allocator *alloc;
};

err_t dblb_create (
    struct dbl_buffer *dest,
    struct allocator  *alloc,
    u32                size,
    u32                initial_cap,
    error             *e
);
err_t dblb_append (struct dbl_buffer *d, const void *data, u32 nelem, error *e);
err_t dblb_ensure_space (struct dbl_buffer *d, u32 nelem, error *e);
void *dblb_append_alloc (struct dbl_buffer *d, u32 nelem, error *e);
void  dblb_reset (struct dbl_buffer *d); // Target buffer

#endif
