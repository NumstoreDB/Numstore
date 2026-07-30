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

/**
 * @file
 * @brief Anything that has to do with numbers
 *
 * Contains:
 * 1. Checksum utilities
 * 2. Random utilities
 * 3. Numeric Limit checking
 */

#ifndef NUMERICS_H
#define NUMERICS_H

#include "error.h"    // error
#include "platform.h" // math.h for cabsf / isfinite
#include "stdtypes.h" // u32 ...etc

/******************************************************************************
 * SECTION: Checksums
 ******************************************************************************/

u32  checksum_init (void);
void checksum_execute (u32 *dest, const u8 *data, u32 len);

/******************************************************************************
 * SECTION: Random
 ******************************************************************************/

u8 randu8 (void);

u32 randu32 (void);
u32 randu32r (u32 lower, u32 upper); // [lower, upper]
i32 randi32r (i32 lower, i32 upper); // [lower, upper]

u64 randu64 (void);
u64 randu64r (u64 lower, u64 upper); // [lower, upper]
u64 randu64e (u64 lower, u64 upper); // [lower, upper)
i64 randi64r (i64 lower, i64 upper); // [lower, upper]
i64 randi64e (i64 lower, i64 upper); // [lower, upper)

f32 randf (void); // [0, 1]

void rand_bytes (void *dest, u32 len);
#define decl_rand_buffer(name, type, len) \
  type name[len];                         \
  rand_bytes (name, sizeof (type) * len);

/******************************************************************************
 * SECTION: Parsing and numeric truncation
 ******************************************************************************/

err_t parse_i32_expect (i32 *dest, const char *data, u32 len, error *e);
err_t parse_i64_expect (i64 *dest, const char *data, u32 len, error *e);
err_t parse_f32_expect (f32 *dest, const char *s, u32 len, error *e);
f32   py_mod_f32 (f32 num, f32 denom);
i32   py_mod_i32 (i32 num, i32 denom);

/******************************************************************************
 * SECTION: Numeric Bounds Checking
 ******************************************************************************/

/*-----------------------------------------------------------------------------
 * SUBSECTION: Unsigned checks
 *----------------------------------------------------------------------------*/

#define _uadd_ok(a, b, max) ((a) <= ((max) - (b)))
#define _usub_ok(a, b)      ((a) >= (b))
#define _umul_ok(a, b, max) ((a) == 0 || (b) <= ((max) / (a)))

/*-----------------------------------------------------------------------------
 * SUBSECTION: Signed Checks
 *----------------------------------------------------------------------------*/
#define _sadd_ok(a, b, min, max) \
  (((b) > 0 && (a) <= ((max) - (b))) || ((b) <= 0 && (a) >= ((min) - (b))))
#define _ssub_ok(a, b, min, max) \
  (((b) < 0 && (a) <= ((max) + (b))) || ((b) >= 0 && (a) >= ((min) + (b))))
#define _smul_ok(a, b, min, max)                                        \
  ((a) == 0 || (b) == 0 || ((a) > 0 && (b) > 0 && (a) <= ((max) / (b))) \
   || ((a) < 0 && (b) < 0 && (a) >= ((max) / (b)))                      \
   || ((a) > 0 && (b) < 0 && (b) >= ((min) / (a)))                      \
   || ((a) < 0 && (b) > 0 && (a) >= ((min) / (b))))

/*-----------------------------------------------------------------------------
 * SUBSECTION: Division Checks
 *----------------------------------------------------------------------------*/

#define _div_ok(b)          ((b) != 0)
#define _sdiv_ok(a, b, min) ((b) != 0 && !((a) == (min) && (b) == -1))

/*-----------------------------------------------------------------------------
 * SUBSECTION: Unsigned Integers
 *----------------------------------------------------------------------------*/

#if HAS_BUILTIN_OVERFLOW // GCC / Clang (Uses fast built-ins)

#  define safe_add_u8(dest, a, b) (!__builtin_add_overflow (a, b, dest))
#  define safe_sub_u8(dest, a, b) (!__builtin_sub_overflow (a, b, dest))
#  define safe_mul_u8(dest, a, b) (!__builtin_mul_overflow (a, b, dest))

#  define safe_add_u16(dest, a, b) (!__builtin_add_overflow (a, b, dest))
#  define safe_sub_u16(dest, a, b) (!__builtin_sub_overflow (a, b, dest))
#  define safe_mul_u16(dest, a, b) (!__builtin_mul_overflow (a, b, dest))

#  define safe_add_u32(dest, a, b) (!__builtin_add_overflow (a, b, dest))
#  define safe_sub_u32(dest, a, b) (!__builtin_sub_overflow (a, b, dest))
#  define safe_mul_u32(dest, a, b) (!__builtin_mul_overflow (a, b, dest))

#  define safe_add_u64(dest, a, b) (!__builtin_add_overflow (a, b, dest))
#  define safe_sub_u64(dest, a, b) (!__builtin_sub_overflow (a, b, dest))
#  define safe_mul_u64(dest, a, b) (!__builtin_mul_overflow (a, b, dest))

#else // compiler without __builtin_overflow / compiler without
      // __builtin_overflow

#  define safe_add_u8(dest, a, b)              \
    (_uadd_ok ((u8)(a), (u8)(b), UINT8_MAX)    \
         ? (*(dest) = (u8)(a) + (u8)(b), true) \
         : false)
#  define safe_sub_u8(dest, a, b) \
    (_usub_ok ((u8)(a), (u8)(b)) ? (*(dest) = (u8)(a) - (u8)(b), true) : false)
#  define safe_mul_u8(dest, a, b)              \
    (_umul_ok ((u8)(a), (u8)(b), UINT8_MAX)    \
         ? (*(dest) = (u8)(a) * (u8)(b), true) \
         : false)

#  define safe_add_u16(dest, a, b)               \
    (_uadd_ok ((u16)(a), (u16)(b), UINT16_MAX)   \
         ? (*(dest) = (u16)(a) + (u16)(b), true) \
         : false)
#  define safe_sub_u16(dest, a, b)                                         \
    (_usub_ok ((u16)(a), (u16)(b)) ? (*(dest) = (u16)(a) - (u16)(b), true) \
                                   : false)
#  define safe_mul_u16(dest, a, b)               \
    (_umul_ok ((u16)(a), (u16)(b), UINT16_MAX)   \
         ? (*(dest) = (u16)(a) * (u16)(b), true) \
         : false)

#  define safe_add_u32(dest, a, b)               \
    (_uadd_ok ((u32)(a), (u32)(b), UINT32_MAX)   \
         ? (*(dest) = (u32)(a) + (u32)(b), true) \
         : false)
#  define safe_sub_u32(dest, a, b)                                         \
    (_usub_ok ((u32)(a), (u32)(b)) ? (*(dest) = (u32)(a) - (u32)(b), true) \
                                   : false)
#  define safe_mul_u32(dest, a, b)               \
    (_umul_ok ((u32)(a), (u32)(b), UINT32_MAX)   \
         ? (*(dest) = (u32)(a) * (u32)(b), true) \
         : false)

#  define safe_add_u64(dest, a, b)               \
    (_uadd_ok ((u64)(a), (u64)(b), UINT64_MAX)   \
         ? (*(dest) = (u64)(a) + (u64)(b), true) \
         : false)
#  define safe_sub_u64(dest, a, b)                                         \
    (_usub_ok ((u64)(a), (u64)(b)) ? (*(dest) = (u64)(a) - (u64)(b), true) \
                                   : false)
#  define safe_mul_u64(dest, a, b)               \
    (_umul_ok ((u64)(a), (u64)(b), UINT64_MAX)   \
         ? (*(dest) = (u64)(a) * (u64)(b), true) \
         : false)

#endif

/*-----------------------------------------------------------------------------
 * SUBSECTION: Unsigned division
 *----------------------------------------------------------------------------*/

#define safe_div_u8(dest, a, b) \
  (_div_ok (b) ? (*(dest) = (a) / (b), true) : false)
#define safe_div_u16(dest, a, b) \
  (_div_ok (b) ? (*(dest) = (a) / (b), true) : false)
#define safe_div_u32(dest, a, b) \
  (_div_ok (b) ? (*(dest) = (a) / (b), true) : false)
#define safe_div_u64(dest, a, b) \
  (_div_ok (b) ? (*(dest) = (a) / (b), true) : false)

/*-----------------------------------------------------------------------------
 * SUBSECTION: Signed Integers
 *----------------------------------------------------------------------------*/

#ifndef _MSC_VER // GCC / Clang

#  define safe_add_i8(dest, a, b) (!__builtin_add_overflow (a, b, dest))
#  define safe_sub_i8(dest, a, b) (!__builtin_sub_overflow (a, b, dest))
#  define safe_mul_i8(dest, a, b) (!__builtin_mul_overflow (a, b, dest))

#  define safe_add_i16(dest, a, b) (!__builtin_add_overflow (a, b, dest))
#  define safe_sub_i16(dest, a, b) (!__builtin_sub_overflow (a, b, dest))
#  define safe_mul_i16(dest, a, b) (!__builtin_mul_overflow (a, b, dest))

#  define safe_add_i32(dest, a, b) (!__builtin_add_overflow (a, b, dest))
#  define safe_sub_i32(dest, a, b) (!__builtin_sub_overflow (a, b, dest))
#  define safe_mul_i32(dest, a, b) (!__builtin_mul_overflow (a, b, dest))

#  define safe_add_i64(dest, a, b) (!__builtin_add_overflow (a, b, dest))
#  define safe_sub_i64(dest, a, b) (!__builtin_sub_overflow (a, b, dest))
#  define safe_mul_i64(dest, a, b) (!__builtin_mul_overflow (a, b, dest))

#else // compiler without __builtin_overflow

#  define safe_add_i8(dest, a, b)                    \
    (_sadd_ok ((i8)(a), (i8)(b), INT8_MIN, INT8_MAX) \
         ? (*(dest) = (i8)(a) + (i8)(b), true)       \
         : false)
#  define safe_sub_i8(dest, a, b)                    \
    (_ssub_ok ((i8)(a), (i8)(b), INT8_MIN, INT8_MAX) \
         ? (*(dest) = (i8)(a) - (i8)(b), true)       \
         : false)
#  define safe_mul_i8(dest, a, b)                    \
    (_smul_ok ((i8)(a), (i8)(b), INT8_MIN, INT8_MAX) \
         ? (*(dest) = (i8)(a) * (i8)(b), true)       \
         : false)

#  define safe_add_i16(dest, a, b)                       \
    (_sadd_ok ((i16)(a), (i16)(b), INT16_MIN, INT16_MAX) \
         ? (*(dest) = (i16)(a) + (i16)(b), true)         \
         : false)
#  define safe_sub_i16(dest, a, b)                       \
    (_ssub_ok ((i16)(a), (i16)(b), INT16_MIN, INT16_MAX) \
         ? (*(dest) = (i16)(a) - (i16)(b), true)         \
         : false)
#  define safe_mul_i16(dest, a, b)                       \
    (_smul_ok ((i16)(a), (i16)(b), INT16_MIN, INT16_MAX) \
         ? (*(dest) = (i16)(a) * (i16)(b), true)         \
         : false)

#  define safe_add_i32(dest, a, b)                       \
    (_sadd_ok ((i32)(a), (i32)(b), INT32_MIN, INT32_MAX) \
         ? (*(dest) = (i32)(a) + (i32)(b), true)         \
         : false)
#  define safe_sub_i32(dest, a, b)                       \
    (_ssub_ok ((i32)(a), (i32)(b), INT32_MIN, INT32_MAX) \
         ? (*(dest) = (i32)(a) - (i32)(b), true)         \
         : false)
#  define safe_mul_i32(dest, a, b)                       \
    (_smul_ok ((i32)(a), (i32)(b), INT32_MIN, INT32_MAX) \
         ? (*(dest) = (i32)(a) * (i32)(b), true)         \
         : false)

#  define safe_add_i64(dest, a, b)                       \
    (_sadd_ok ((i64)(a), (i64)(b), INT64_MIN, INT64_MAX) \
         ? (*(dest) = (i64)(a) + (i64)(b), true)         \
         : false)
#  define safe_sub_i64(dest, a, b)                       \
    (_ssub_ok ((i64)(a), (i64)(b), INT64_MIN, INT64_MAX) \
         ? (*(dest) = (i64)(a) - (i64)(b), true)         \
         : false)
#  define safe_mul_i64(dest, a, b)                       \
    (_smul_ok ((i64)(a), (i64)(b), INT64_MIN, INT64_MAX) \
         ? (*(dest) = (i64)(a) * (i64)(b), true)         \
         : false)

#endif

/*-----------------------------------------------------------------------------
 * SUBSECTION: Signed division
 *----------------------------------------------------------------------------*/
#define safe_div_i8(dest, a, b) \
  (_sdiv_ok ((i8)(a), (i8)(b), INT8_MIN) ? (*(dest) = (a) / (b), true) : false)
#define safe_div_i16(dest, a, b)                                          \
  (_sdiv_ok ((i16)(a), (i16)(b), INT16_MIN) ? (*(dest) = (a) / (b), true) \
                                            : false)
#define safe_div_i32(dest, a, b)                                          \
  (_sdiv_ok ((i32)(a), (i32)(b), INT32_MIN) ? (*(dest) = (a) / (b), true) \
                                            : false)
#define safe_div_i64(dest, a, b)                                          \
  (_sdiv_ok ((i64)(a), (i64)(b), INT64_MIN) ? (*(dest) = (a) / (b), true) \
                                            : false)

/*-----------------------------------------------------------------------------
 * SUBSECTION: Floats
 *----------------------------------------------------------------------------*/

#define safe_add_f32(dest, a, b) ((*(dest) = (a) + (b)), isfinite (*(dest)))
#define safe_sub_f32(dest, a, b) ((*(dest) = (a) - (b)), isfinite (*(dest)))
#define safe_mul_f32(dest, a, b) ((*(dest) = (a) * (b)), isfinite (*(dest)))
#define safe_div_f32(dest, a, b) \
  ((b) != 0.0f ? (*(dest) = (a) / (b), isfinite (*(dest))) : false)

#define safe_add_f64(dest, a, b) ((*(dest) = (a) + (b)), isfinite (*(dest)))
#define safe_sub_f64(dest, a, b) ((*(dest) = (a) - (b)), isfinite (*(dest)))
#define safe_mul_f64(dest, a, b) ((*(dest) = (a) * (b)), isfinite (*(dest)))
#define safe_div_f64(dest, a, b) \
  ((b) != 0.0 ? (*(dest) = (a) / (b), isfinite (*(dest))) : false)

/*-----------------------------------------------------------------------------
 * SUBSECTION: Error handling functions
 *----------------------------------------------------------------------------*/

HEADER_FUNC err_t
safe_add_u64_err (u64 *dest, const u64 a, const u64 b, error *e)
{
  if (!safe_add_u64 (dest, a, b))
  {
    return error_causef (e, ERR_ARITH, "Overflow");
  }
  return SUCCESS;
}

/******************************************************************************
 * SECTION: Math
 ******************************************************************************/

#define i_creal_64(f) (creal (f))
#define i_cimag_64(f) (cimag (f))

#define i_cabs_sqrd_64(f) ((creal (f) * creal (f)) + ((cimag (f) * cimag (f))))
#define i_cabs_64(f)      cabsf (f)
#define i_fabs_32(f)      fabsf (f)

#define arr_range(arr)                     \
  do                                       \
  {                                        \
    for (u32 i = 0; i < arrlen (arr); ++i) \
    {                                      \
      arr[i] = i;                          \
    }                                      \
  }                                        \
  while (0)

#define ptr_range(arr, size)          \
  do                                  \
  {                                   \
    for (u32 _i = 0; _i < size; ++_i) \
    {                                 \
      arr[_i] = _i;                   \
    }                                 \
  }                                   \
  while (0)

#define u32_arr_rand(arr)                  \
  do                                       \
  {                                        \
    for (u32 i = 0; i < arrlen (arr); ++i) \
    {                                      \
      arr[i] = randu32 ();                 \
    }                                      \
  }                                        \
  while (0)

#define arr_contains(arr, len, val, ret)   \
  do                                       \
  {                                        \
    ret = false;                           \
    for (u32 ___i = 0; ___i < len; ++___i) \
    {                                      \
      if (arr[___i] == val)                \
      {                                    \
        ret = arr[___i];                   \
        ret = true;                        \
        break;                             \
      }                                    \
    }                                      \
  }                                        \
  while (0)

float f16_to_f32 (u16 h);

#endif // NUMERICS_H
