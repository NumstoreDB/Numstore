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

#ifndef TYPES_H
#define TYPES_H

#include "alloc.h"
#include "collections.h" // llnode
#include "error.h"       // error
#include "numstore.h"    // pgno ...etc
#include "serial.h"      // string
#include "stdtypes.h"    // u32 ...etc

struct type
{
  enum type_t
  {
    T_PRIM   = 0,
    T_STRUCT = 1,
    T_UNION  = 2,
    T_SARRAY = 3,
  } type;

  union {
    enum prim_t
    {
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

    struct struct_t
    {
      u16            len;
      struct string *keys;
      struct type  **types;
    } st;

    struct union_t
    {
      u16            len;
      struct string *keys;
      struct type  **types;
    } un;

    struct sarray_t
    {
      u16          rank;
      u32         *dims;
      struct type *t;
    } sa;
  };
};

// Core api
err_t        type_validate (const struct type *t, error *e);
i32          type_snprintf (char *str, u32 size, struct type *t);
char        *type_tostr (struct type *t);
u32          type_byte_size (const struct type *t);
u32          type_get_string_size (const struct type *t);
void         type_generate_string (char *dest, const struct type *t);
u32          type_get_serial_size (const struct type *t);
void         type_serialize (struct serializer *dest, const struct type *src);
struct type *type_deserialize (
    struct deserializer *src,
    struct allocator    *alloc,
    error               *e
);
struct type *type_random (struct allocator *alloc, u32 depth, error *e);
bool         type_equal (const struct type *left, const struct type *right);
char        *get_var_str (struct type *t, u32 *dlen, error *e);
err_t        i_print_type (struct type *t, error *e);
err_t        i_log_type (struct type *t, error *e);
struct type *type_movemem (struct type *src, struct allocator *alloc, error *e);
void         type_print_data (
    int                log_level,
    const u8          *buf,
    const struct type *t,
    u32                max_elems
);
err_t type_stream_printer_init (struct stream *s, struct type *t, error *e);

#define _mk_prim(_p) {.type = T_PRIM, .p = _p}
HEADER_FUNC struct type
mk_prim (enum prim_t p)
{
  return (struct type)_mk_prim (p);
}

HEADER_FUNC struct type
mk_struct (u16 len, struct string *keys, struct type **types)
{
  return (struct type){
      .type = T_STRUCT,
      .st   = (struct struct_t){
          .len   = len,
          .keys  = keys,
          .types = types,
      },
  };
}

HEADER_FUNC struct type
mk_union (u16 len, struct string *keys, struct type **types)
{
  return (struct type){
      .type = T_UNION,
      .un   = (struct union_t){
          .len   = len,
          .keys  = keys,
          .types = types,
      },
  };
}

HEADER_FUNC struct type
mk_sarray (u16 rank, u32 *dims, struct type *sub)
{
  return (struct type){
      .type = T_SARRAY,
      .sa   = (struct sarray_t){
          .rank = rank,
          .dims = dims,
          .t    = sub,
      },
  };
}

enum prim_t strtoprim (const char *text, u32 len);

#ifdef TESTING
static struct type TU8    = _mk_prim (U8);
static struct type TU16   = _mk_prim (U16);
static struct type TU32   = _mk_prim (U32);
static struct type TU64   = _mk_prim (U64);
static struct type TI8    = _mk_prim (I8);
static struct type TI16   = _mk_prim (I16);
static struct type TI32   = _mk_prim (I32);
static struct type TI64   = _mk_prim (I64);
static struct type TF16   = _mk_prim (F16);
static struct type TF32   = _mk_prim (F32);
static struct type TF64   = _mk_prim (F64);
static struct type TF128  = _mk_prim (F128);
static struct type TCF32  = _mk_prim (CF32);
static struct type TCF64  = _mk_prim (CF64);
static struct type TCF128 = _mk_prim (CF128);
static struct type TCF256 = _mk_prim (CF256);
static struct type TCI16  = _mk_prim (CI16);
static struct type TCI32  = _mk_prim (CI32);
static struct type TCI64  = _mk_prim (CI64);
static struct type TCI128 = _mk_prim (CI128);
static struct type TCU16  = _mk_prim (CU16);
static struct type TCU32  = _mk_prim (CU32);
static struct type TCU64  = _mk_prim (CU64);
static struct type TCU128 = _mk_prim (CU128);
#endif

#endif // TYPES_H
