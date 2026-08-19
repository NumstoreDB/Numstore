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

#ifndef EXT_ARRAY_H
#define EXT_ARRAY_H

#include "core/ns_data_writer.h"
#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "core/ns_stride.h"

struct data_writer;
struct stride;

/******************************************************************************
 * SECTION: Extending array
 * ----------------------------------------------------------------------------
 * @brief A [data_writer] that doubles in length when it reaches a limit
 ******************************************************************************/

struct ext_array
{
  struct i_mem mem;
  u8          *data;
  u32          len;
  u32          cap;
};

struct ext_array ext_array_create ();
void             ext_array_free (struct ext_array *r);

i64 ext_array_insert (struct ext_array *r, u32 ofst, const void *src, u32 slen, error *e);
i64 ext_array_read (const struct ext_array *r, struct stride str, u32 size, void *dest, error *e);
i64 ext_array_write (
    const struct ext_array *r,
    struct stride           str,
    u32                     size,
    const void             *src,
    error                  *e
);
i64 ext_array_remove (struct ext_array *r, struct stride str, u32 size, void *dest, error *e);
u64 ext_array_get_len (const struct ext_array *r);

void ext_array_data_writer (struct data_writer *dest, struct ext_array *arr);

#endif
