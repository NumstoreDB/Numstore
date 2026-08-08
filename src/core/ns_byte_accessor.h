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

/******************************************************************************
 * SECTION: Byte Accessor
 * ----------------------------------------------------------------------------
 * @brief A way to copy offset and strided data from one place to another
 *
 * The motivation for a byte accessor comes from the numstore type system.
 *
 * Consider a (packed) struct:
 *
 * struct foo {
 *    u32 a;
 *    u64 b;
 *    f32 c[20];
 * };
 *
 * Lets say I have an instance of foo and I want
 * to read into a byte buffer:
 *
 * [ foo.a, foo.a, foo.b, foo.b, foo.a ]
 *
 * (Yes, foo.a foo.b are all redundant duplicate data)
 *
 * Byte accessor defines this memcopy as:
 *
 * @code
 * [
 *    foo.a -- SELECT(src_size = 4, dest_size = 4, bofst = 0, sub_ba = TAKE)
 *    foo.a -- SELECT(src_size = 4, dest_size = 4, bofst = 0, sub_ba = TAKE)
 *    foo.b -- SELECT(src_size = 8, dest_size = 8, bofst = 4, sub_ba = TAKE)
 *    foo.b -- SELECT(src_size = 8, dest_size = 8, bofst = 4, sub_ba = TAKE)
 *    foo.a -- SELECT(src_size = 4, dest_size = 4, bofst = 0, sub_ba = TAKE)
 *    foo.a -- SELECT(src_size = 4, dest_size = 4, bofst = 0, sub_ba = TAKE)
 * ]
 * @endcode
 *
 * TODO - this example could be better
 ******************************************************************************/

struct byte_accessor
{
  enum ta_type
  {
    TA_TAKE,
    TA_SELECT,
    TA_RANGE,
  } type;

  u32 src_size;  // total size this ba takes up on source
  u32 dest_size; // total size this ba puts into dest

  union {
    struct select_ba
    {
      u32                   bofst;  // Offset in bytes
      struct byte_accessor *sub_ba; // Next accessor
    } select;

    struct range_ba
    {
      struct stride         stride; // Stride on src
      struct byte_accessor *sub_ba; // For each stride, the next ba
    } range;
  };
};

u32 ba_memcpy_from (u8 *dest, const u8 *src, struct byte_accessor *acc);
u32 ba_memcpy_to (u8 *dest, const u8 *src, struct byte_accessor *acc);

#endif
