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

#ifndef C_SPECX_LATCH_H
#define C_SPECX_LATCH_H

#include <c_specx/logging.h>
#include <c_specx/platform.h>
#include <c_specx/stdtypes.h>

////////////////////////////////////////////////////////////
// CONCURRENCY / LATCH

// TODO - (16) ABA problem

////////////////////////////////////////////////////////////
// spin_pause
#if PLATFORM_WINDOWS
#  define spin_pause() YieldProcessor ()
#elif defined(__x86_64__) || defined(__i386__)
#  define spin_pause() __asm__ volatile ("pause" ::: "memory")
#elif defined(__aarch64__) || defined(__arm__)
#  define spin_pause() __asm__ volatile ("yield" ::: "memory")
#else
#  define spin_pause() atomic_signal_fence (memory_order_seq_cst)
#endif

typedef _Atomic (int) latch;

HEADER_FUNC void
latch_init (latch *l)
{ *l = 0; }

/**
 * Returns true if the latch was locked, false otherwise
 */
HEADER_FUNC bool
latch_trylock (latch *l)
{
  int val = 0;

  // Fast path - it's likely that the lock will succeed if
  // l == 0, replace it with 1
  if (likely (atomic_compare_exchange_weak_explicit (
          l,
          &val,
          1,
          memory_order_acquire,
          memory_order_relaxed
      )))
  {
    return true;
  }
  return false;
}

HEADER_FUNC void
latch_lock (latch *l)
{
  int val = 0;

  // Fast path - it's likely that the lock will succeed if
  // l == 0, replace it with 1
  if (likely (atomic_compare_exchange_weak_explicit (
          l,
          &val,
          1,
          memory_order_acquire,
          memory_order_relaxed
      )))
  {
    return;
  }

  do
  {
    do
    {
      // let the CPU breath
      spin_pause ();

      // Load the value of l
      val = atomic_load_explicit (l, memory_order_relaxed);
    }
    // if l != 0: continue
    while (val != 0);
  }

  // Has it changed yet? If not - set it to locked - this risks the ABA problem
  while (!atomic_compare_exchange_weak_explicit (
      l,
      &val,
      1,
      memory_order_acquire,
      memory_order_relaxed
  ));
}

HEADER_FUNC void
latch_unlock (latch *l)
{ atomic_store_explicit (l, 0, memory_order_release); }

#endif // C_SPECX_LATCH_H
