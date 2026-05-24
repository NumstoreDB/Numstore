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

#ifndef C_SPECX_SPX_LATCH_H
#define C_SPECX_SPX_LATCH_H

#include <c_specx/platform.h>
#include <c_specx/stdtypes.h>
#include <c_specx/latch.h>

////////////////////////////////////////////////////////////
// CONCURRENCY / SPX_LATCH

typedef _Atomic (unsigned int) sx_latch;

#define S_MASK 0x0000FFFFu // [15:0]
#define X      0x00010000u // [16]

#define SLOCKED(val) (val & S_MASK)
#define XLOCKED(val) (val & X)

HEADER_FUNC void spx_latch_init (sx_latch *l) { *l = 0; }

HEADER_FUNC bool spx_trylock_s (sx_latch *l) {
  u32 val = atomic_load_explicit (l, memory_order_relaxed);

  if (unlikely (XLOCKED (val))) { return false; }

  return atomic_compare_exchange_strong_explicit (
      l,
      &val,
      val + 1,
      memory_order_acquire,
      memory_order_relaxed);
}

HEADER_FUNC void spx_lock_s (sx_latch *l) {
  u32 val = atomic_load_explicit (l, memory_order_relaxed);

  while (true) {
    // Wait for X to clear before attempting the CAS.
    while (unlikely (XLOCKED (val))) {
      spin_pause ();
      val = atomic_load_explicit (l, memory_order_relaxed);
    }

    if (likely (atomic_compare_exchange_weak_explicit (
            l,
            &val,
            val + 1,
            memory_order_acquire,
            memory_order_relaxed))) {
      return;
    }
  }
}

HEADER_FUNC void spx_unlock_s (sx_latch *l) {
  atomic_fetch_sub_explicit (l, 1, memory_order_release);
}

HEADER_FUNC bool spx_trylock_x (sx_latch *l) {
  u32 expected = 0;
  return atomic_compare_exchange_strong_explicit (
      l,
      &expected,
      X,
      memory_order_acquire,
      memory_order_relaxed);
}

HEADER_FUNC void spx_lock_x (sx_latch *l) {
  u32 val = atomic_load_explicit (l, memory_order_relaxed);
  // Phase 1: claim the X bit.  This blocks new S acquirers
  // (spx_lock_s spins while XLOCKED is set) but does not yet
  // wait for in-flight readers.
  while (true) {
    if (likely (!XLOCKED (val))) {
      if (atomic_compare_exchange_weak_explicit (
              l,
              &val,
              val | X,
              memory_order_acquire,
              memory_order_relaxed)) {
        break;
      }
      // CAS failed; val is refreshed, retry without spinning.
      continue;
    }
    // Another writer holds X.  Wait for them to release.
    do {
      spin_pause ();
      val = atomic_load_explicit (l, memory_order_relaxed);
    } while (XLOCKED (val));
  }
  // Phase 2: drain remaining readers.  No new readers can arrive
  // because XLOCKED is now true.
  while (SLOCKED (atomic_load_explicit (l, memory_order_acquire))) { spin_pause (); }
}

HEADER_FUNC void spx_unlock_x (sx_latch *l) { atomic_store_explicit (l, 0, memory_order_release); }

#endif // C_SPECX_SPX_LATCH_H
