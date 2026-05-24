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

#ifndef C_SPECX_BYTE_ACCESSOR_H
#define C_SPECX_BYTE_ACCESSOR_H

#include <c_specx/platform.h>
#include <c_specx/stdtypes.h>
#include <c_specx/stride.h>

////////////////////////////////////////////////////////////
// MEMORY / BYTE_ACCESSOR

struct byte_accessor {
  enum ta_type {
    TA_TAKE,
    TA_SELECT,
    TA_RANGE,
  } type;

  u32 src_size;  // total size this ba takes up on source
  u32 dest_size; // total size this ba puts into dest

  union {
    struct select_ba {
      u32                   bofst;  // Offset in bytes
      struct byte_accessor *sub_ba; // Next accessor
    } select;

    struct range_ba {
      struct stride         stride; // Stride on src
      struct byte_accessor *sub_ba; // For each stride, the next ba
    } range;
  };
};

u32 ba_memcpy_from (u8 *dest, const u8 *src, struct byte_accessor *acc);
u32 ba_memcpy_to (u8 *dest, const u8 *src, struct byte_accessor *acc);

#endif // C_SPECX_BYTE_ACCESSOR_H
