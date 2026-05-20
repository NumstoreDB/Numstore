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

/* numstore_type.h
 *
 * The numstore type representation, exactly as provided by the spec.
 * Uses C11 anonymous unions; compile with -std=gnu11 or newer.
 */
#ifndef NUMSTORE_TYPE_H
#define NUMSTORE_TYPE_H

#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

/* A length-prefixed, non-owning string view. */
struct string {
  u32         len;
  const char *data;
};

struct type {
  enum type_t {
    T_PRIM   = 0,
    T_STRUCT = 1,
    T_UNION  = 2,
    T_SARRAY = 3,
  } type;
  union {
    enum prim_t {
      U8    = 0,
      U16   = 1,
      U32   = 2,
      U64   = 3,
      I8    = 4,
      I16   = 5,
      I32   = 6,
      I64   = 7,
      F16   = 8,
      F32   = 9,
      F64   = 10,
      F128  = 11,
      CF32  = 12,
      CF64  = 13,
      CF128 = 14,
      CF256 = 15,
      CI16  = 16,
      CI32  = 17,
      CI64  = 18,
      CI128 = 19,
      CU16  = 20,
      CU32  = 21,
      CU64  = 22,
      CU128 = 23,
    } p;
    struct struct_t {
      u16            len;
      struct string *keys;
      struct type  **types;
    } st;
    struct union_t {
      u16            len;
      struct string *keys;
      struct type  **types;
    } un;
    struct sarray_t {
      u16          rank;
      u32         *dims;
      struct type *t;
    } sa;
  };
};

#endif /* NUMSTORE_TYPE_H */
