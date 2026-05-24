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

#pragma once

#include <c_specx.h>
#include "nscore/types.h"

#define PRIM_INT \
  I8:            \
  case I16:      \
  case I32:      \
  case I64

#define PRIM_UINT \
  U8:             \
  case U16:       \
  case U32:       \
  case U64

#define PRIM_FLOAT \
  F16:             \
  case F32:        \
  case F64:        \
  case F128

#define PRIM_CF \
  CF32:         \
  case CF64:    \
  case CF128:   \
  case CF256

#define PRIM_CI \
  CI16:         \
  case CI32:    \
  case CI64:    \
  case CI128

#define PRIM_CU \
  CU16:         \
  case CU32:    \
  case CU64:    \
  case CU128

HEADER_FUNC bool
prim_is_int (enum prim_t p)
{ return p >= U8 && p <= I64; }

HEADER_FUNC bool
prim_is_float (enum prim_t p)
{ return p >= F16 && p <= F128; }

HEADER_FUNC bool
prim_is_complex (enum prim_t p)
{ return p >= CF32 && p <= CU128; }

#define PRIM_FOR_EACH(func, ...) \
  do                             \
  {                              \
    func (U8, __VA_ARGS__);      \
    func (U8, __VA_ARGS__);      \
    func (U16, __VA_ARGS__);     \
    func (U32, __VA_ARGS__);     \
    func (U64, __VA_ARGS__);     \
    func (I8, __VA_ARGS__);      \
    func (I16, __VA_ARGS__);     \
    func (I32, __VA_ARGS__);     \
    func (I64, __VA_ARGS__);     \
    func (F16, __VA_ARGS__);     \
    func (F32, __VA_ARGS__);     \
    func (F64, __VA_ARGS__);     \
    func (F128, __VA_ARGS__);    \
    func (CF32, __VA_ARGS__);    \
    func (CF64, __VA_ARGS__);    \
    func (CF128, __VA_ARGS__);   \
    func (CF256, __VA_ARGS__);   \
    func (CI16, __VA_ARGS__);    \
    func (CI32, __VA_ARGS__);    \
    func (CI64, __VA_ARGS__);    \
    func (CI128, __VA_ARGS__);   \
    func (CU16, __VA_ARGS__);    \
    func (CU32, __VA_ARGS__);    \
    func (CU64, __VA_ARGS__);    \
    func (CU128, __VA_ARGS__);   \
  }                              \
  while (0)

#define PRIM_FOR_EACH_LITERAL(func, ...) \
  do                                     \
  {                                      \
    func ("u8", __VA_ARGS__);            \
    func ("u8", __VA_ARGS__);            \
    func ("u16", __VA_ARGS__);           \
    func ("u32", __VA_ARGS__);           \
    func ("u64", __VA_ARGS__);           \
    func ("i8", __VA_ARGS__);            \
    func ("i16", __VA_ARGS__);           \
    func ("i32", __VA_ARGS__);           \
    func ("i64", __VA_ARGS__);           \
    func ("f16", __VA_ARGS__);           \
    func ("f32", __VA_ARGS__);           \
    func ("f64", __VA_ARGS__);           \
    func ("f128", __VA_ARGS__);          \
    func ("cf32", __VA_ARGS__);          \
    func ("cf64", __VA_ARGS__);          \
    func ("cf128", __VA_ARGS__);         \
    func ("cf256", __VA_ARGS__);         \
    func ("ci16", __VA_ARGS__);          \
    func ("ci32", __VA_ARGS__);          \
    func ("ci64", __VA_ARGS__);          \
    func ("ci128", __VA_ARGS__);         \
    func ("cu16", __VA_ARGS__);          \
    func ("cu32", __VA_ARGS__);          \
    func ("cu64", __VA_ARGS__);          \
    func ("cu128", __VA_ARGS__);         \
  }                                      \
  while (0)
