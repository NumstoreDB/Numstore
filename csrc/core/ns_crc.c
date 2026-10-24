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

#include "core/ns_crc.h"

#include "core/ns_csx_assert.h"
#include "core/ns_testing.h"
#include "core/os/ns_once.h"

#include <string.h>

/******************************************************************************
 *  Platform hooks
 ******************************************************************************/

#if defined(__ARM_FEATURE_CRC32)
#  include <arm_acle.h>
#  define CRC_HW 1
#  define CRC_HW_ATTR
#  define hw_u8(c, v)  __crc32cb ((c), (v))
#  define hw_u64(c, v) __crc32cd ((c), (v))
#elif defined(__x86_64__) && (defined(__GNUC__) || defined(__clang__))
#  include <nmmintrin.h>
#  define CRC_HW       1
// clang 17+ splits crc32 out of sse4.2 as its own subtarget feature, so
// target("sse4.2") alone is no longer enough to let _mm_crc32_* inline here.
// GCC accepts both names (-mcrc32 exists there too), so naming both is
// portable across the x86 toolchains.
#  define CRC_HW_ATTR  __attribute__ ((target ("sse4.2,crc32")))
#  define hw_u8(c, v)  _mm_crc32_u8 ((c), (v))
#  define hw_u64(c, v) ((u32)_mm_crc32_u64 ((c), (v)))
#endif

#define CRC32C_POLY 0x82F63B78U

// Bytes per interleaved stream (multiple of 8). Blocks are 3 * CRC_K.
#define CRC_K 1024

static u32    crc_sw_tbl[8][256];
static i_once crc_once = I_ONCE_INIT;

#ifdef CRC_HW
static u32 crc_shift_tbl[4][256]; // operator: advance a raw crc by CRC_K zero bytes
static int crc_use_hw;
#endif

/******************************************************************************
 *  Portable slicing-by-8 (fallback and non-HW targets)
 ******************************************************************************/

static u32
crc_sw (u32 c, const u8 *p, u32 len)
{
  while (len >= 8) {
    u32 lo, hi;
    memcpy (&lo, p, 4); // little-endian hosts only
    memcpy (&hi, p + 4, 4);
    lo ^= c;
    c = crc_sw_tbl[7][lo & 0xFF] ^ crc_sw_tbl[6][(lo >> 8) & 0xFF]
        ^ crc_sw_tbl[5][(lo >> 16) & 0xFF] ^ crc_sw_tbl[4][lo >> 24] ^ crc_sw_tbl[3][hi & 0xFF]
        ^ crc_sw_tbl[2][(hi >> 8) & 0xFF] ^ crc_sw_tbl[1][(hi >> 16) & 0xFF]
        ^ crc_sw_tbl[0][hi >> 24];
    p += 8;
    len -= 8;
  }
  while (len--) {
    c = (c >> 8) ^ crc_sw_tbl[0][(c ^ *p++) & 0xFF];
  }
  return c;
}

/******************************************************************************
 *  Hardware path, 3-way interleaved
 ******************************************************************************/

#ifdef CRC_HW

static inline u32
crc_shift (u32 v)
{
  return crc_shift_tbl[0][v & 0xFF] ^ crc_shift_tbl[1][(v >> 8) & 0xFF]
         ^ crc_shift_tbl[2][(v >> 16) & 0xFF] ^ crc_shift_tbl[3][v >> 24];
}

CRC_HW_ATTR static u32
crc_hw_zeros (u32 c, u32 n)
{
  for (u32 i = 0; i < n; i += 8) {
    c = hw_u64 (c, 0);
  }
  return c;
}

CRC_HW_ATTR static u32
crc_hw (u32 c, const u8 *p, u32 len)
{
  // Three independent streams hide the instruction latency
  while (len >= 3 * CRC_K) {
    u32       c0 = c, c1 = 0, c2 = 0;
    const u8 *p1 = p + CRC_K;
    const u8 *p2 = p + 2 * CRC_K;

    for (u32 i = 0; i < CRC_K; i += 8) {
      u64 a, b, d;
      memcpy (&a, p + i, 8);
      memcpy (&b, p1 + i, 8);
      memcpy (&d, p2 + i, 8);
      c0 = hw_u64 (c0, a);
      c1 = hw_u64 (c1, b);
      c2 = hw_u64 (c2, d);
    }

    // state after S0||S1||S2, by linearity
    c = crc_shift (crc_shift (c0) ^ c1) ^ c2;

    p += 3 * CRC_K;
    len -= 3 * CRC_K;
  }

  while (len >= 8) {
    u64 v;
    memcpy (&v, p, 8);
    c = hw_u64 (c, v);
    p += 8;
    len -= 8;
  }

  while (len--) {
    c = hw_u8 (c, *p++);
  }

  return c;
}

#endif

/******************************************************************************
 *  Init and public API
 ******************************************************************************/

static void
crc_init (void)
{
  for (u32 i = 0; i < 256; ++i) {
    u32 c = i;
    for (int k = 0; k < 8; ++k) {
      c = (c >> 1) ^ (CRC32C_POLY & -(c & 1));
    }
    crc_sw_tbl[0][i] = c;
  }
  for (u32 i = 0; i < 256; ++i) {
    for (int k = 1; k < 8; ++k) {
      u32 prev         = crc_sw_tbl[k - 1][i];
      crc_sw_tbl[k][i] = (prev >> 8) ^ crc_sw_tbl[0][prev & 0xFF];
    }
  }

#ifdef CRC_HW
#  if defined(__x86_64__) && !defined(__ARM_FEATURE_CRC32)
  crc_use_hw = __builtin_cpu_supports ("sse4.2");
#  else
  crc_use_hw = 1;
#  endif
  if (crc_use_hw) {
    for (int j = 0; j < 4; ++j) {
      for (u32 b = 0; b < 256; ++b) {
        crc_shift_tbl[j][b] = crc_hw_zeros (b << (8 * j), CRC_K);
      }
    }
  }
#endif
}

u32
checksum_init (void)
{
  return 0;
}

void
checksum_execute (u32 *state, const u8 *data, const u32 len)
{
  ASSERT (state);
  ASSERT (data || len == 0);

  if (len == 0) {
    return;
  }

  i_once_run (&crc_once, crc_init);

  u32 c = ~(*state);

#ifdef CRC_HW
  if (crc_use_hw) {
    c      = crc_hw (c, data, len);
    *state = ~c;
    return;
  }
#endif

  c      = crc_sw (c, data, len);
  *state = ~c;
}

#ifndef NDEBUG
TEST (checksum_execute_simple)
{
  const u8 data[] = {1, 2, 3, 4};
  u32      state  = checksum_init ();
  checksum_execute (&state, data, 4);

  // Should produce some non-zero checksum

  test_assert (state != 0);
}

TEST (checksum_execute_deterministic)
{
  const u8 data[] = {5, 10, 15, 20};
  u32      state1 = checksum_init ();
  u32      state2 = checksum_init ();

  checksum_execute (&state1, data, 4);
  checksum_execute (&state2, data, 4);

  test_assert_equal (state1, state2);
}

TEST (checksum_execute_incremental)
{
  const u8 data[] = {1, 2, 3, 4, 5, 6};

  // All at once
  u32      state1 = checksum_init ();
  checksum_execute (&state1, data, 6);

  // Incremental
  u32 state2 = checksum_init ();
  checksum_execute (&state2, data, 3);
  checksum_execute (&state2, data + 3, 3);

  test_assert_equal (state1, state2);
}

/******************************************************************************
 * SECTION: crc_sw
 * ----------------------------------------------------------------------------
 * crc_sw is the portable slicing-by-8 fallback. On a host with CRC_HW it is
 * not the path checksum_execute takes, so it is tested directly here: against
 * an independent bit-at-a-time oracle, against the published CRC-32C vectors,
 * and against the hardware path it has to stay bit-identical to.
 *
 * It works on the raw (inverted) state, so a finished CRC-32C of [buf] is
 * ~crc_sw (0xFFFFFFFFu, buf, len).
 ******************************************************************************/

// Bit-at-a-time CRC-32C. Deliberately the dumbest possible implementation: it
// shares no tables or structure with crc_sw, so agreement is real evidence.
static u32
crc_bitwise (u32 c, const u8 *p, u32 len)
{
  for (u32 i = 0; i < len; ++i) {
    c ^= p[i];
    for (int k = 0; k < 8; ++k) {
      c = (c >> 1) ^ (CRC32C_POLY & -(c & 1));
    }
  }
  return c;
}

// crc_sw reads its tables, so they have to be built before it is called
// directly rather than through checksum_execute
static void
crc_tables_ready (void)
{
  i_once_run (&crc_once, crc_init);
}

static u32
crc32c_of (const u8 *p, const u32 len)
{
  return ~crc_sw (0xFFFFFFFFu, p, len);
}

static const struct
{
  const char *name;
  u32         len;
  u8          fill_kind; // 0: zeros, 1: 0xff, 2: incrementing, 3: decrementing
  u32         expected;
} crc_rfc3720_cases[] = {
    // RFC 3720 (iSCSI) appendix B.4 CRC-32C vectors
    {"32 bytes of zeros", 32, 0, 0x8A9136AAu},
    {"32 bytes of 0xff", 32, 1, 0x62A8AB43u},
    {"32 bytes incrementing 00..1f", 32, 2, 0x46DD794Eu},
    {"32 bytes decrementing 1f..00", 32, 3, 0x113FDB5Cu},
};

TEST (crc_sw_known_answers)
{
  crc_tables_ready ();

  TEST_CASE ("The CRC-32C check value for \"123456789\"")
  {
    const u8 *check = (const u8 *)"123456789";
    test_assert_type_equal (crc32c_of (check, 9), 0xE3069283u, u32, PRIx32);
  }

  TEST_CASE ("An empty buffer leaves the CRC at its initial value")
  {
    test_assert_type_equal (crc32c_of ((const u8 *)"", 0), 0x00000000u, u32, PRIx32);
  }

  TEST_CASE ("Single byte \"a\"")
  {
    test_assert_type_equal (crc32c_of ((const u8 *)"a", 1), 0xC1D04330u, u32, PRIx32);
  }

  TEST_CASE ("Five bytes \"hello\" exercises the trailing byte loop alone")
  {
    test_assert_type_equal (crc32c_of ((const u8 *)"hello", 5), 0x9A71BB4Cu, u32, PRIx32);
  }

  for (u32 i = 0; i < arrlen (crc_rfc3720_cases); ++i) {
    TEST_CASE ("%s", crc_rfc3720_cases[i].name)
    {
      u8 buf[32];
      for (u32 j = 0; j < crc_rfc3720_cases[i].len; ++j) {
        switch (crc_rfc3720_cases[i].fill_kind) {
          case 0: buf[j] = 0x00; break;
          case 1: buf[j] = 0xFF; break;
          case 2: buf[j] = (u8)j; break;
          default: buf[j] = (u8)(crc_rfc3720_cases[i].len - 1 - j); break;
        }
      }
      test_assert_type_equal (
          crc32c_of (buf, crc_rfc3720_cases[i].len),
          crc_rfc3720_cases[i].expected,
          u32,
          PRIx32
      );
    }
  }
}

// Every length from 0 to 40 covers the 8-byte slicing loop, the 1-byte tail
// loop, and each combination of the two
TEST (crc_sw_matches_bitwise_oracle_at_every_length)
{
  crc_tables_ready ();

  u8 buf[40];
  for (u32 i = 0; i < arrlen (buf); ++i) {
    buf[i] = (u8)(i * 7u + 1u);
  }

  for (u32 len = 0; len <= arrlen (buf); ++len) {
    TEST_CASE ("len=%u", len)
    {
      test_assert_type_equal (
          crc_sw (0xFFFFFFFFu, buf, len),
          crc_bitwise (0xFFFFFFFFu, buf, len),
          u32,
          PRIx32
      );
    }
  }
}

// A non-default seed must be carried through both loops, not discarded
TEST (crc_sw_honours_the_incoming_state)
{
  crc_tables_ready ();

  const u8  buf[11] = {9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 255};
  const u32 seeds[] = {0x00000000u, 0x00000001u, 0xDEADBEEFu, 0xFFFFFFFFu};

  for (u32 i = 0; i < arrlen (seeds); ++i) {
    TEST_CASE ("seed=0x%08X", seeds[i])
    {
      test_assert_type_equal (
          crc_sw (seeds[i], buf, arrlen (buf)),
          crc_bitwise (seeds[i], buf, arrlen (buf)),
          u32,
          PRIx32
      );
    }
  }

  TEST_CASE ("A zero length run returns the state untouched")
  {
    test_assert_type_equal (crc_sw (0x12345678u, buf, 0), 0x12345678u, u32, PRIx32);
  }
}

// Splitting a buffer anywhere must not change the result, which is what lets
// checksum_execute be called incrementally
TEST (crc_sw_is_resumable_at_every_split)
{
  crc_tables_ready ();

  u8 buf[24];
  for (u32 i = 0; i < arrlen (buf); ++i) {
    buf[i] = (u8)(i * 31u + 5u);
  }

  const u32 whole = crc_sw (0xFFFFFFFFu, buf, arrlen (buf));

  for (u32 split = 0; split <= arrlen (buf); ++split) {
    TEST_CASE ("split at %u", split)
    {
      const u32 first = crc_sw (0xFFFFFFFFu, buf, split);
      const u32 both  = crc_sw (first, buf + split, (u32)arrlen (buf) - split);
      test_assert_type_equal (both, whole, u32, PRIx32);
    }
  }
}

// On a CRC_HW host this is the test that matters: the fallback and the
// hardware path must agree exactly, including across the 3 * CRC_K interleaved
// block boundary the hardware path takes for large inputs.
TEST (crc_sw_agrees_with_checksum_execute)
{
  crc_tables_ready ();

  // Spans the hardware path's 3 * CRC_K == 3072 byte block, plus a remainder
  const u32 len = 3 * CRC_K + 77;
  u8       *buf = ctx->mem.table->malloc (ctx->mem.self, len, 1, &ctx->e);
  test_assert (buf != NULL);

  for (u32 i = 0; i < len; ++i) {
    buf[i] = (u8)(i * 13u + (i >> 5));
  }

  const u32 lens[] = {0, 1, 7, 8, 9, 63, 64, 1024, 3071, 3072, 3073, 3 * CRC_K + 77};

  for (u32 i = 0; i < arrlen (lens); ++i) {
    TEST_CASE ("len=%u", lens[i])
    {
      u32 state = checksum_init ();
      checksum_execute (&state, buf, lens[i]);
      test_assert_type_equal (state, crc32c_of (buf, lens[i]), u32, PRIx32);
    }
  }

  ctx->mem.table->free (ctx->mem.self, buf);
}
#endif
