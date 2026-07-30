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
 * @brief Concurrency primitives
 *
 * Contains:
 * 1. gr lock - lock with precedence
 * 2. Periodic Task - A repeated task on a timer
 * 3. Latch - A spin lock
 * 4. SPX Latch - A read write spin lock
 */

#ifndef CONCURRENCY_H
#define CONCURRENCY_H

#include "error.h"    // error
#include "os.h"       // i_cond/i_mutex ...etc
#include "platform.h" // HEADER_FUNC/PLATFORM_WINDOWS ...etc
#include "stdtypes.h" // u32 ...etc

/******************************************************************************
 * SECTION: GRLock
 * ----------------------------------------------------------------------------
 * @brief A Lock with precedence
 ******************************************************************************/

enum lock_mode
{
  LM_IS    = 0,
  LM_IX    = 1,
  LM_S     = 2,
  LM_SIX   = 3,
  LM_X     = 4,
  LM_COUNT = 5
};

/**
 * lock waiters are allocated on the stack - so a waiter only exists
 * for as long as gr_lock is waiting. Therefore, there is no array of
 * granted lock groups, instead, I use a counter to count how many locks
 * of each type are granted.
 *
 * This comes with drawbacks.
 *
 * 1. There is no priority - The only priority that's ensured is that the order
 *    of the condition variable signals is the same as the order that they came
 * in, which is ok, but not the best.
 *
 * In order to add priority I need to keep track of the list of granted locks -
 * which would mean some sort of allocation or intrusive data structure.
 *
 * One idea would be to have a gr_lock live on the stack and it's lifecycle is
 * tied to one lock unlock flow, which is probably what I'll do, but for now,
 * locks have loose priority until it becomes a performance problem
 */
struct gr_lock_waiter
{
  enum lock_mode         mode;
  i_cond                 cond;
  struct gr_lock_waiter *prev;
  struct gr_lock_waiter *next;
};

struct gr_lock
{
  i_mutex                mutex;
  int                    holder_counts[LM_COUNT];
  struct gr_lock_waiter *head;
};

err_t gr_lock_init (struct gr_lock *l, error *e);

void gr_lock_destroy (struct gr_lock *l);

err_t gr_lock (struct gr_lock *l, enum lock_mode mode, error *e);
void  gr_unlock (struct gr_lock *l, enum lock_mode mode);

const char    *gr_lock_mode_name (enum lock_mode mode);
enum lock_mode get_parent_mode (enum lock_mode child_mode);

/******************************************************************************
 * SECTION: Periodic Task
 * ----------------------------------------------------------------------------
 * @brief A utility for a periodic task that occurs a fixed timer
 *
 * TODO - usage
 ******************************************************************************/

typedef void (*periodic_task_fn) (void *ctx);

struct periodic_task
{
  i_thread         thread;
  i_mutex          mutex;
  i_cond           wake_cond;
  i_cond           done_cond;
  bool             wake_requested;
  bool             done;
  _Atomic (bool)   stop;
  bool             running;
  u64              msec;
  periodic_task_fn fn;
  void            *ctx;
};

err_t periodic_task_init (struct periodic_task *t, error *e);

err_t periodic_task_start (
    struct periodic_task *t,
    u64                   msec,
    periodic_task_fn      fn,
    void                 *ctx,
    error                *e
);

err_t periodic_task_stop (struct periodic_task *t, error *e);
void  periodic_task_wake (struct periodic_task *t);

/******************************************************************************
 * SECTION: Latch
 * ----------------------------------------------------------------------------
 * @brief A spin lock - a lightweight lock to use for internal data structures
 *
 * TODO - (16) ABA problem
 ******************************************************************************/

/*-----------------------------------------------------------------------------
 * SUBSECTION: Spin Pause
 * @brief assembly primitives to indicate to the os to pause
 *----------------------------------------------------------------------------*/

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
{
  *l = 0;
}

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
{
  atomic_store_explicit (l, 0, memory_order_release);
}

/******************************************************************************
 * SECTION: SPX Latch
 * ----------------------------------------------------------------------------
 * @brief Read write latch with a shared, pending and exclusive state
 *
 * A latch for read write mode
 ******************************************************************************/

typedef _Atomic (unsigned int) sx_latch;

#define S_MASK 0x0000FFFFu // [15:0]
#define X      0x00010000u // [16]

#define SLOCKED(val) (val & S_MASK)
#define XLOCKED(val) (val & X)

HEADER_FUNC void
spx_latch_init (sx_latch *l)
{
  *l = 0;
}

HEADER_FUNC bool
spx_trylock_s (sx_latch *l)
{
  u32 val = atomic_load_explicit (l, memory_order_relaxed);

  if (unlikely (XLOCKED (val)))
  {
    return false;
  }

  return atomic_compare_exchange_strong_explicit (
      l,
      &val,
      val + 1,
      memory_order_acquire,
      memory_order_relaxed
  );
}

HEADER_FUNC void
spx_lock_s (sx_latch *l)
{
  u32 val = atomic_load_explicit (l, memory_order_relaxed);

  while (true)
  {
    // Wait for X to clear before attempting the CAS.
    while (unlikely (XLOCKED (val)))
    {
      spin_pause ();
      val = atomic_load_explicit (l, memory_order_relaxed);
    }

    if (likely (atomic_compare_exchange_weak_explicit (
            l,
            &val,
            val + 1,
            memory_order_acquire,
            memory_order_relaxed
        )))
    {
      return;
    }
  }
}

HEADER_FUNC void
spx_unlock_s (sx_latch *l)
{
  atomic_fetch_sub_explicit (l, 1, memory_order_release);
}

HEADER_FUNC bool
spx_trylock_x (sx_latch *l)
{
  u32 expected = 0;
  return atomic_compare_exchange_strong_explicit (
      l,
      &expected,
      X,
      memory_order_acquire,
      memory_order_relaxed
  );
}

HEADER_FUNC void
spx_lock_x (sx_latch *l)
{
  u32 val = atomic_load_explicit (l, memory_order_relaxed);
  // Phase 1: claim the X bit.  This blocks new S acquirers
  // (spx_lock_s spins while XLOCKED is set) but does not yet
  // wait for in-flight readers.
  while (true)
  {
    if (likely (!XLOCKED (val)))
    {
      if (atomic_compare_exchange_weak_explicit (
              l,
              &val,
              val | X,
              memory_order_acquire,
              memory_order_relaxed
          ))
      {
        break;
      }
      // CAS failed; val is refreshed, retry without spinning.
      continue;
    }
    // Another writer holds X.  Wait for them to release.
    do
    {
      spin_pause ();
      val = atomic_load_explicit (l, memory_order_relaxed);
    }
    while (XLOCKED (val));
  }
  // Phase 2: drain remaining readers.  No new readers can arrive
  // because XLOCKED is now true.
  while (SLOCKED (atomic_load_explicit (l, memory_order_acquire)))
  {
    spin_pause ();
  }
}

HEADER_FUNC void
spx_unlock_x (sx_latch *l)
{
  atomic_store_explicit (l, 0, memory_order_release);
}

#endif // CONCURRENCY_H
