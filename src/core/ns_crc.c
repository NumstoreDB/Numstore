#include "core/ns_crc.h"

#include "core/ns_csx_assert.h"
#include "core/testing/ns_testing.h"

#include <pthread.h>
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
#  define CRC_HW_ATTR  __attribute__ ((target ("sse4.2")))
#  define hw_u8(c, v)  _mm_crc32_u8 ((c), (v))
#  define hw_u64(c, v) ((u32)_mm_crc32_u64 ((c), (v)))
#endif

#define CRC32C_POLY 0x82F63B78U

// Bytes per interleaved stream (multiple of 8). Blocks are 3 * CRC_K.
#define CRC_K 1024

static u32            crc_sw_tbl[8][256];
static pthread_once_t crc_once = PTHREAD_ONCE_INIT;

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

  pthread_once (&crc_once, crc_init);

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

#ifdef TESTING
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
#endif
