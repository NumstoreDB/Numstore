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

#ifndef NS_DST_H
#define NS_DST_H

#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "core/os/ns_filesystem.h"
#include "core/os/ns_memory.h"
#include "core/os/ns_os_vtable.h"
#include "core/os/ns_threading.h"

/******************************************************************************
 * SECTION: Deterministic Simulation Testing (DST) os vtable
 * ----------------------------------------------------------------------------
 * @brief A single os_vtable implementation that sits in front of a real
 * (delegate) os_vtable - by default `default_os_vtable` - and, on every
 * call, first rolls the dice against a configured failure probability
 * before delegating to the real implementation. Every probability (and the
 * delegate itself) lives in one `struct dst_data`, which doubles as both:
 *
 *   - the `.data` blob for the file system / memory / threading interfaces
 *     (`i_file_system`, `i_mem`, `i_threading` all carry a `void *data`), and
 *   - the `test_data` slot on the vtable instance itself, which is how file
 *     level operations recover it - `i_file` has no `.data` field, only a
 *     `.table`, so `fp->table->test_data` is the only place a file created
 *     through this vtable can find its fault-injection config.
 *
 * Determinism comes from the same source as the rest of the test suite:
 * `randf`/`randu32r` are backed by libc `rand()`, and `unit_tests` seeds it
 * once with `srand(1234)` before running tests in a fixed order, so a given
 * test run reproduces the same sequence of injected faults every time.
 ******************************************************************************/

struct dst_data
{
  // Real implementation calls fall through to when a fault isn't injected.
  const struct os_vtable *delegate;

  struct
  {
    // Probability [0, 1] that the call fails outright with a simulated error.
    float close_fail_prob;
    float fsync_fail_prob;
    float file_size_fail_prob;

    // `*_fail_prob` simulates the syscall itself failing; `*_short_prob` is
    // the probability that any single underlying read/write syscall
    // transfers fewer bytes than requested (the `_all` wrapper keeps
    // retrying, so this just adds extra iterations rather than breaking the
    // read_all/write_all contract).
    float read_fail_prob;
    float read_short_prob;

    float pread_fail_prob;
    float pread_short_prob;

    float write_fail_prob;
    float write_short_prob;

    float pwrite_fail_prob;
    float pwrite_short_prob;

    float writev_fail_prob;
    float truncate_fail_prob;
    float fallocate_fail_prob;
    float seek_fail_prob;
  } file;

  struct
  {
    float open_rw_fail_prob;
    float open_r_fail_prob;
    float open_w_fail_prob;
    float remove_quiet_fail_prob;
    float unlink_fail_prob;
    float file_exists_fail_prob;
  } filesystem;

  struct
  {
    float malloc_fail_prob;
    float calloc_fail_prob;
    float realloc_fail_prob;
  } memory;

  struct
  {
    float thread_create_fail_prob;
    float mutex_create_fail_prob;
    float cond_create_fail_prob;
  } threading;

  // Populated by dst_data_init(). Every i_file_system/i_mem/i_threading/
  // i_file handed out of this dst_data points its `table` here.
  struct os_vtable vtable;
};

/*-----------------------------------------------------------------------------
 * SUBSECTION: Construction
 * ----------------------------------------------------------------------------
 * @brief All probabilities start at 0 (pure pass-through). `delegate` may be
 * NULL to fall through to `default_os_vtable`.
 *----------------------------------------------------------------------------*/

void dst_data_init (struct dst_data *d, const struct os_vtable *delegate);

struct i_file_system dst_filesystem (struct dst_data *d);
struct i_mem dst_mem (struct dst_data *d);
struct i_threading dst_threading (struct dst_data *d);

/*-----------------------------------------------------------------------------
 * SUBSECTION: File-level fault injection
 * ----------------------------------------------------------------------------
 * @brief `i_file` carries only a `table` pointer (no `.data`), so the posix
 * and win32 `impl_*` file functions recover their `struct dst_data *` via
 * `fp->table->test_data` and call these directly. Outside of TESTING builds
 * `os_vtable.test_data` doesn't exist, so both macros compile away to
 * nothing.
 *----------------------------------------------------------------------------*/

#ifdef TESTING

#  include "core/ns_numerics.h" // randf, randu32r

static inline err_t
dst_file_fault_check (float prob, error *e)
{
  if (randf () < prob) {
    return error_causef (e, ERR_IO, "dst: simulated fault");
  }
  return SUCCESS;
}

static inline u64
dst_file_short_amount (u64 amount, float prob)
{
  if (amount > 1 && randf () < prob) {
    return randu32r (1, (u32)amount);
  }
  return amount;
}

// Returns early with the simulated error when the dice roll fails.
#  define I_FILE_FAULT(fp, field, e)                                        \
    do {                                                                    \
      struct dst_data *_dst_data = (fp)->table->test_data;                  \
      if (_dst_data) {                                                      \
        const err_t _dst_fe = dst_file_fault_check (_dst_data->field, (e)); \
        if (unlikely (_dst_fe)) {                                           \
          return _dst_fe;                                                   \
        }                                                                   \
      }                                                                     \
    }                                                                       \
    while (0)

// Shrinks `amount` in place to simulate a short read/write for one syscall.
#  define I_FILE_CONDITION_AMOUNT(fp, field, amount)                   \
    do {                                                               \
      struct dst_data *_dst_data = (fp)->table->test_data;             \
      if (_dst_data) {                                                 \
        (amount) = dst_file_short_amount ((amount), _dst_data->field); \
      }                                                                \
    }                                                                  \
    while (0)

#else

#  define I_FILE_FAULT(fp, field, e)                 ((void)0)
#  define I_FILE_CONDITION_AMOUNT(fp, field, amount) ((void)0)

#endif // TESTING

#endif
