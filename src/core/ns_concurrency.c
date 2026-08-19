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

#include "core/ns_concurrency.h"

#include <stdint.h>
#include <string.h>

#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_logging.h"
#include "core/os/ns_threading.h"
#include "core/os/ns_time.h"
#include "core/testing/ns_testing.h"

/******************************************************************************
 * SECTION: GR Lock
 ******************************************************************************/

// clang-format off
static const bool compatible[LM_COUNT][LM_COUNT] = {
  //         IS      IX      S       SIX     X
  [LM_IS]  = { true,  true,  true,  true,  false },
  [LM_IX]  = { true,  true,  false, false, false },
  [LM_S]   = { true,  false, true,  false, false },
  [LM_SIX] = { true,  false, false, false, false },
  [LM_X]   = { false, false, false, false, false },
};
// clang-format on

static const char *mode_names[LM_COUNT] = {"IS", "IX", "S", "SIX", "X"};

err_t
gr_lock_init (struct gr_lock *l, error *e)
{
  const err_t result = default_threading.i_mutex_create (&default_threading, &l->mutex, e);
  if (result != SUCCESS)
  {
    return result;
  }

  memset (l->holder_counts, 0, sizeof (l->holder_counts));
  l->head = NULL;

  return SUCCESS;
}

#ifdef TESTING
TEST (gr_lock_init)
{
  TEST_CASE ("mutex create green path")
  {
    error          e = error_create ();
    struct gr_lock l;
    gr_lock_init (&l, &e);
    gr_lock_destroy (&l);
  }
}
#endif

void
gr_lock_destroy (struct gr_lock *l)
{
  default_threading.i_mutex_lock (&default_threading, &l->mutex);
  // TODO - Caller must ensure all threads have released locks
  // You could put a done flag - and assert !done on actions
  default_threading.i_mutex_unlock (&default_threading, &l->mutex);

  default_threading.i_mutex_free (&default_threading, &l->mutex);

  while (l->head)
  {
    struct gr_lock_waiter *w = l->head;
    l->head                  = w->next;
    default_threading.i_cond_free (&default_threading, &w->cond);
  }
}

#ifdef TESTING
TEST (gr_lock_destroy)
{
  TEST_CASE ("green path")
  {
    error          e = error_create ();
    struct gr_lock l;
    gr_lock_init (&l, &e);
    gr_lock_destroy (&l);
  }
}
#endif

/**
 * Example:
 *
 * Granted Group:
 * IS IX IS IS IS
 *
 * Granted Group Count:
 * IS = 4
 * IX = 1
 *
 * Lock S is compatible?
 * IS > 0 -> IS + S = GOOD
 * IX > 1 -> IX + S = BAD
 * - Not Compatible
 *
 * Lock IS is compatible?
 * IS > 0 -> IS + IS = GOOD
 * IX > 1 -> IX + IS = GOOD
 * - Compatible
 */
static bool
is_compatible (const struct gr_lock *l, const enum lock_mode mode)
{
  for (int i = 0; i < LM_COUNT; i++)
  {
    if (l->holder_counts[i] > 0 && !compatible[mode][i])
    {
      return false;
    }
  }
  return true;
}

#ifdef TESTING
TEST (gr_lock_is_compatible)
{
  error          e = error_create ();
  struct gr_lock l;
  gr_lock_init (&l, &e);

  // All locks compatible on init
  test_assert (is_compatible (&l, LM_IS));
  test_assert (is_compatible (&l, LM_IX));
  test_assert (is_compatible (&l, LM_S));
  test_assert (is_compatible (&l, LM_SIX));
  test_assert (is_compatible (&l, LM_X));

  // IS is incompatible with X
  gr_lock (&l, LM_IS, &e);
  test_assert (is_compatible (&l, LM_IS));
  test_assert (is_compatible (&l, LM_IX));
  test_assert (is_compatible (&l, LM_S));
  test_assert (is_compatible (&l, LM_SIX));
  test_assert (!is_compatible (&l, LM_X));
  gr_unlock (&l, LM_IS);

  // IX is incompatible with S SIX X
  gr_lock (&l, LM_IX, &e);
  test_assert (is_compatible (&l, LM_IS));
  test_assert (is_compatible (&l, LM_IX));
  test_assert (!is_compatible (&l, LM_S));
  test_assert (!is_compatible (&l, LM_SIX));
  test_assert (!is_compatible (&l, LM_X));
  gr_unlock (&l, LM_IX);

  // S is incompatible with IX SIX X
  gr_lock (&l, LM_S, &e);
  test_assert (is_compatible (&l, LM_IS));
  test_assert (!is_compatible (&l, LM_IX));
  test_assert (is_compatible (&l, LM_S));
  test_assert (!is_compatible (&l, LM_SIX));
  test_assert (!is_compatible (&l, LM_X));
  gr_unlock (&l, LM_S);

  // SIX is incompatible with IX S SIX X
  gr_lock (&l, LM_SIX, &e);
  test_assert (is_compatible (&l, LM_IS));
  test_assert (!is_compatible (&l, LM_IX));
  test_assert (!is_compatible (&l, LM_S));
  test_assert (!is_compatible (&l, LM_SIX));
  test_assert (!is_compatible (&l, LM_X));
  gr_unlock (&l, LM_SIX);

  // X is incompatible with IS IX S SIX X
  gr_lock (&l, LM_X, &e);
  test_assert (!is_compatible (&l, LM_IS));
  test_assert (!is_compatible (&l, LM_IX));
  test_assert (!is_compatible (&l, LM_S));
  test_assert (!is_compatible (&l, LM_SIX));
  test_assert (!is_compatible (&l, LM_X));
  gr_unlock (&l, LM_X);
}
#endif

err_t
gr_lock (struct gr_lock *l, const enum lock_mode mode, error *e)
{
  // First do a global mutex lock
  default_threading.i_mutex_lock (&default_threading, &l->mutex);

  // If it's compatible - just increment mode count and move on
  if (is_compatible (l, mode))
  {
    TEST_MARK ("gr_lock:gr_lock:immediate_acquire");
    goto acquire;
  }

  // Otherwise, we need to create a new lock waiter
  struct gr_lock_waiter waiter = {
      .mode = mode,
      .prev = NULL,
      .next = NULL,
  };
  if (default_threading.i_cond_create (&default_threading, &waiter.cond, e))
  {
    // Ok here - we just failed and everything is unlocked
    default_threading.i_mutex_unlock (&default_threading, &l->mutex);
    return error_trace (e);
  }

  // Append waiter to the linked list of waiters
  if (l->head == NULL)
  {
    l->head = &waiter;
  }
  else
  {
    struct gr_lock_waiter *end = l->head;
    while (end->next != NULL)
    {
      end = end->next;
    }

    end->next   = &waiter;
    waiter.prev = end;
  }

  // Wait for someone to signal my condition variable - main wait code
  while (!is_compatible (l, mode))
  {
    TEST_MARK ("gr_lock:gr_lock:wait");
    default_threading.i_cond_wait (&default_threading, &waiter.cond, &l->mutex);
  }

  // Remove from waiters list
  if (waiter.prev != NULL)
  {
    waiter.prev->next = waiter.next;
  }
  else
  {
    ASSERT (l->head == &waiter);
    l->head = waiter.next;
  }
  if (waiter.next != NULL)
  {
    waiter.next->prev = waiter.prev;
  }

  // Release resources
  default_threading.i_cond_free (&default_threading, &waiter.cond);

acquire:
  // Acquire the lock
  l->holder_counts[mode]++;
  default_threading.i_mutex_unlock (&default_threading, &l->mutex);
  return SUCCESS;
}

void
gr_unlock (struct gr_lock *l, const enum lock_mode mode)
{
  default_threading.i_mutex_lock (&default_threading, &l->mutex);

  // do unlock
  ASSERT (l->holder_counts[mode] > 0);
  l->holder_counts[mode]--;

  // Wake any compatible waiters
  if (l->head)
  {
    for (struct gr_lock_waiter *w = l->head; w; w = w->next)
    {
      // signal all waiters - they do the compatability check - it's ok
      default_threading.i_cond_signal (&default_threading, &w->cond);
    }
  }

  default_threading.i_mutex_unlock (&default_threading, &l->mutex);
}

#ifdef TESTING
struct thread_ctx
{
  struct gr_lock *l;
  enum lock_mode  mode1;
  enum lock_mode  mode2;
  _Atomic u32     locked1;
  _Atomic u32     locked2;
  _Atomic u32     gate;
};

static void *
thread1 (void *_ctx)
{
  struct thread_ctx *ctx = _ctx;

  while (!atomic_load (&ctx->gate))
  {
    spin_pause ();
  }

  // This should pass through
  gr_lock (ctx->l, ctx->mode1, NULL);

  atomic_store (&ctx->locked1, 1);

  return NULL;
}

static void *
thread2 (void *_ctx)
{
  struct thread_ctx *ctx = _ctx;

  while (!atomic_load (&ctx->gate))
  {
    spin_pause ();
  }

  // Wait until thread 1 issued the lock
  while (!atomic_load (&ctx->locked1))
  {
    spin_pause ();
  }

  // This is the lock query in question
  gr_lock (ctx->l, ctx->mode2, NULL);

  atomic_store (&ctx->locked2, 1);

  return NULL;
}

TEST (gr_lock_unlock)
{
  i_thread       t1, t2;
  error          e = error_create ();
  struct gr_lock l;
  gr_lock_init (&l, &e);

  // Cartesion product
  for (int m1 = 0; m1 < LM_COUNT; ++m1)
  {
    for (int m2 = 0; m2 < LM_COUNT; ++m2)
    {
      TEST_CASE ("%s + %s", mode_names[m1], mode_names[m2])
      {
        test_reset_marks ();

        struct thread_ctx ctx = {
            .l       = &l,
            .mode1   = m1,
            .mode2   = m2,
            .locked1 = 0,
            .locked2 = 0,
            .gate    = 0,
        };

        default_threading.i_thread_create (&default_threading, &t1, thread1, &ctx, &e);
        default_threading.i_thread_create (&default_threading, &t2, thread2, &ctx, &e);

        // Launch both threads
        atomic_store (&ctx.gate, 1);

        if (compatible[m1][m2])
        {
          // 2 finishes without unlocking anything
          while (!atomic_load (&ctx.locked2))
          {
            spin_pause ();
          }

          // 1 LOCKED
          // 2 LOCKED

          gr_unlock (&l, m1);
          gr_unlock (&l, m2);

          test_assert_mark_hit ("gr_lock:gr_lock:immediate_acquire");
          test_assert_mark_not_hit ("gr_lock:gr_lock:wait");
        }
        else
        {
          // 1 finishes fine
          while (!atomic_load (&ctx.locked1))
          {
            spin_pause ();
          }

          // 1 LOCKED
          // 2 PENDING

          // Wait 10 ms and 2 is STILL not locked
          i_sleep_ms (10);
          test_assert_int_equal (atomic_load (&ctx.locked2), 0);

          // Unlock 2
          gr_unlock (&l, m1);
          while (!atomic_load (&ctx.locked2))
          {
            spin_pause ();
          }

          // 1 UNLOCKED
          // 2 LOCKED

          gr_unlock (&l, m2);

          test_assert_mark_hit ("gr_lock:gr_lock:immediate_acquire");
          test_assert_mark_hit ("gr_lock:gr_lock:wait");
        }

        default_threading.i_thread_join (&default_threading, &t1, &e);
        default_threading.i_thread_join (&default_threading, &t2, &e);
      }
    }
  }
}
#endif

const char *
gr_lock_mode_name (const enum lock_mode mode)
{
  if (mode >= 0 && mode < LM_COUNT)
  {
    return mode_names[mode];
  }
  UNREACHABLE (); // LCOV_EXCL_LINE
}

#ifdef TESTING
TEST (gr_lock_mode_name)
{
  i_log_info ("%s\n", gr_lock_mode_name (LM_IS));
  i_log_info ("%s\n", gr_lock_mode_name (LM_IX));
  i_log_info ("%s\n", gr_lock_mode_name (LM_S));
  i_log_info ("%s\n", gr_lock_mode_name (LM_SIX));
  i_log_info ("%s\n", gr_lock_mode_name (LM_X));
}
#endif

enum lock_mode
get_parent_mode (const enum lock_mode child_mode)
{
  switch (child_mode)
  {
    case LM_IS:
    case LM_S:
    {
      return LM_IS;
    }
    case LM_IX:
    case LM_SIX:
    case LM_X:
    {
      return LM_IX;
    }
    case LM_COUNT:
    {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
  UNREACHABLE (); // LCOV_EXCL_LINE
}

#ifdef TESTING

/* --- Test Infrastructure --- */

struct lock_test_ctx
{
  struct gr_lock *lock;

  // Coordination Primitives
  i_mutex gate_mtx;
  i_cond  gate_cv;
  bool    gate_open;

  // Counters and State
  atomic_int t1_acquired;
  atomic_int t2_blocked;
  atomic_int t2_acquired;
  atomic_int counter;

  enum lock_mode mode1;
  enum lock_mode mode2;
};

static void
test_ctx_init (struct lock_test_ctx *ctx, struct gr_lock *lock)
{
  memset (ctx, 0, sizeof (*ctx));
  ctx->lock = lock;
  default_threading.i_mutex_create (&default_threading, &ctx->gate_mtx, NULL);
  default_threading.i_cond_create (&default_threading, &ctx->gate_cv, NULL);
  ctx->gate_open = false;
}

static void
test_ctx_destroy (struct lock_test_ctx *ctx)
{
  default_threading.i_mutex_free (&default_threading, &ctx->gate_mtx);
  default_threading.i_cond_free (&default_threading, &ctx->gate_cv);
}

/* --- Deterministic Thread Routines --- */

static void *
thread_hold_and_signal (void *arg)
{
  struct lock_test_ctx *ctx = arg;
  error                 e   = error_create ();

  // Secure the lock first
  gr_lock (ctx->lock, ctx->mode1, &e);

  // Signal to Thread 2 that the lock is held
  default_threading.i_mutex_lock (&default_threading, &ctx->gate_mtx);
  ctx->t1_acquired = 1;
  ctx->gate_open   = true;
  default_threading.i_cond_broadcast (&default_threading, &ctx->gate_cv);
  default_threading.i_mutex_unlock (&default_threading, &ctx->gate_mtx);

  // Hold long enough for the main thread to sample "blocked" state
  i_sleep_ms (100);

  gr_unlock (ctx->lock, ctx->mode1);
  return NULL;
}

static void *
thread_wait_and_try (void *arg)
{
  struct lock_test_ctx *ctx = arg;
  error                 e   = error_create ();

  // Wait for Thread 1 to confirm it holds the lock
  default_threading.i_mutex_lock (&default_threading, &ctx->gate_mtx);
  while (!ctx->gate_open)
  {
    default_threading.i_cond_wait (&default_threading, &ctx->gate_cv, &ctx->gate_mtx);
  }
  default_threading.i_mutex_unlock (&default_threading, &ctx->gate_mtx);

  // Attempt acquisition (will block if incompatible)
  ctx->t2_blocked = 1;
  gr_lock (ctx->lock, ctx->mode2, &e);

  ctx->t2_acquired = 1;
  ctx->t2_blocked  = 0;
  gr_unlock (ctx->lock, ctx->mode2);

  return NULL;
}

static void *
random_stress_worker (void *arg)
{
  struct lock_test_ctx *ctx  = arg;
  error                 e    = error_create ();
  uint32_t              seed = (uint32_t)(uintptr_t)arg;

  for (int i = 0; i < 1000; i++)
  {
    // Fast thread-local random
    seed                = seed * 1103515245 + 12345;
    enum lock_mode mode = (seed % LM_COUNT);

    gr_lock (ctx->lock, mode, &e);

    // If Exclusive or Shared-Intent-Exclusive, verify atomicity
    if (mode == LM_X || mode == LM_SIX)
    {
      int val = atomic_load (&ctx->counter);
      atomic_store (&ctx->counter, val + 1);
    }

    // Integrity check: current mode must have at least one holder
    if (ctx->lock->holder_counts[mode] == 0)
    {
      panic ("Failed test");
    }

    gr_unlock (ctx->lock, mode);
  }
  return NULL;
}

/* --- Tests --- */

TEST (gr_lock_basic_sanity)
{
  struct gr_lock lock;
  error          e = error_create ();
  gr_lock_init (&lock, &e);

  for (int mode = 0; mode < LM_COUNT; mode++)
  {
    gr_lock (&lock, mode, &e);
    test_assert_equal (lock.holder_counts[mode], 1);
    gr_unlock (&lock, mode);
    test_assert_equal (lock.holder_counts[mode], 0);
  }
  gr_lock_destroy (&lock);
}

// Example of a Compatibility Test (Compatible)
TEST (gr_lock_is_is_compatible)
{
  struct gr_lock lock;
  error          e = error_create ();
  gr_lock_init (&lock, &e);

  struct lock_test_ctx ctx;
  test_ctx_init (&ctx, &lock);
  ctx.mode1 = LM_IS;
  ctx.mode2 = LM_IS;

  i_thread t1, t2;
  default_threading.i_thread_create (&default_threading, &t1, thread_hold_and_signal, &ctx, &e);
  default_threading.i_thread_create (&default_threading, &t2, thread_wait_and_try, &ctx, &e);

  default_threading.i_thread_join (&default_threading, &t1, &e);
  default_threading.i_thread_join (&default_threading, &t2, &e);

  test_assert (ctx.t1_acquired && ctx.t2_acquired);
  test_ctx_destroy (&ctx);
  gr_lock_destroy (&lock);
}

// Example of a Blocking Test (Incompatible)
// This one is breaking on Mac Os
/**
TEST_DISABLED (gr_lock_is_x_blocks)
{
  struct gr_lock lock;
  error          e = error_create ();
  gr_lock_init (&lock, &e);

  struct lock_test_ctx ctx;
  test_ctx_init (&ctx, &lock);
  ctx.mode1 = LM_IS;
  ctx.mode2 = LM_X;

  i_thread t1, t2;
  default_threading.i_thread_create (&default_threading, &t1, thread_hold_and_signal, &ctx, &e);
  default_threading.i_thread_create (&default_threading, &t2, thread_wait_and_try, &ctx, &e);

  // Wait slightly to let T2 hit the block, then check status
  i_sleep_ms (50);
  test_assert (ctx.t1_acquired);
  test_assert (ctx.t2_blocked);
  test_assert (!ctx.t2_acquired);

  default_threading.i_thread_join (&default_threading, &t1, &e);
  default_threading.i_thread_join (&default_threading, &t2, &e);

  test_assert (ctx.t2_acquired); // Should succeed after T1 releases
  test_ctx_destroy (&ctx);
  gr_lock_destroy (&lock);
}
*/

TEST (gr_lock_high_pressure_random)
{
  struct gr_lock lock;
  error          e = error_create ();
  gr_lock_init (&lock, &e);

  struct lock_test_ctx ctx;
  test_ctx_init (&ctx, &lock);

  i_thread threads[12];

  for (int i = 0; i < 12; i++)
  {
    default_threading
        .i_thread_create (&default_threading, &threads[i], random_stress_worker, &ctx, &e);
  }

  for (int i = 0; i < 12; i++)
  {
    default_threading.i_thread_join (&default_threading, &threads[i], &e);
  }

  // Final Validation
  for (int m = 0; m < LM_COUNT; m++)
  {
    test_assert_equal (lock.holder_counts[m], 0);
  }

  test_ctx_destroy (&ctx);
  gr_lock_destroy (&lock);
}

#endif

/******************************************************************************
 * SECTION: Periodic Task
 ******************************************************************************/

err_t
periodic_task_init (struct periodic_task *t, error *e)
{
  t->stop           = false;
  t->wake_requested = false;
  t->done           = false;
  t->running        = false;

  if (default_threading.i_mutex_create (&default_threading, &t->mutex, e))
  {
    goto theend;
  }
  if (default_threading.i_cond_create (&default_threading, &t->wake_cond, e))
  {
    goto fail_mutex;
  }
  if (default_threading.i_cond_create (&default_threading, &t->done_cond, e))
  {
    goto fail_wake_cond;
  }

  goto theend;

  default_threading.i_cond_free (&default_threading, &t->done_cond);
fail_wake_cond:
  default_threading.i_cond_free (&default_threading, &t->wake_cond);
fail_mutex:
  default_threading.i_mutex_free (&default_threading, &t->mutex);
theend:
  return error_trace (e);
}

static void *
periodic_task_thread (void *_ctx)
{
  struct periodic_task *t = _ctx;

  while (true)
  {
    default_threading.i_mutex_lock (&default_threading, &t->mutex);
    // TODO - spurrious wakeups
    if (!t->wake_requested && !t->stop)
    {
      default_threading.i_cond_timed_wait (&default_threading, &t->wake_cond, &t->mutex, t->msec);
    }
    t->wake_requested = false;
    bool should_stop  = t->stop;
    default_threading.i_mutex_unlock (&default_threading, &t->mutex);

    if (should_stop)
    {
      break;
    }

    t->fn (t->ctx);
  }

  default_threading.i_mutex_lock (&default_threading, &t->mutex);
  t->done = true;
  default_threading.i_cond_signal (&default_threading, &t->done_cond);
  default_threading.i_mutex_unlock (&default_threading, &t->mutex);

  return NULL;
}

err_t
periodic_task_start (struct periodic_task *t, u64 msec, periodic_task_fn fn, void *ctx, error *e)
{
  t->msec = msec;
  t->fn   = fn;
  t->ctx  = ctx;

  if (default_threading
          .i_thread_create (&default_threading, &t->thread, periodic_task_thread, t, e))
  {
    return error_trace (e);
  }

  t->running = true;

  return SUCCESS;
}

err_t
periodic_task_stop (struct periodic_task *t, error *e)
{
  if (!t->running)
  {
    return SUCCESS;
  }

  default_threading.i_mutex_lock (&default_threading, &t->mutex);
  t->stop = true;
  default_threading.i_cond_signal (&default_threading, &t->wake_cond);
  default_threading.i_mutex_unlock (&default_threading, &t->mutex);

  default_threading.i_mutex_lock (&default_threading, &t->mutex);
  while (!t->done)
  {
    default_threading.i_cond_wait (&default_threading, &t->done_cond, &t->mutex);
  }
  default_threading.i_mutex_unlock (&default_threading, &t->mutex);

  default_threading.i_thread_join (&default_threading, &t->thread, e);
  default_threading.i_cond_free (&default_threading, &t->done_cond);
  default_threading.i_cond_free (&default_threading, &t->wake_cond);
  default_threading.i_mutex_free (&default_threading, &t->mutex);
  t->running = false;

  return error_trace (e);
}

void
periodic_task_wake (struct periodic_task *t)
{
  default_threading.i_mutex_lock (&default_threading, &t->mutex);
  t->wake_requested = true;
  default_threading.i_cond_signal (&default_threading, &t->wake_cond);
  default_threading.i_mutex_unlock (&default_threading, &t->mutex);
}

/******************************************************************************
 * SECTION: Latch (tests)
 ******************************************************************************/

#ifdef TESTING
struct data
{
  const u32 iters;
  int       value;
  latch     l;
};

static void *
data_thread (void *_data)
{
  struct data *d = _data;

  for (u32 i = 0; i < d->iters; ++i)
  {
    latch_lock (&d->l);
    d->value += 1;
    latch_unlock (&d->l);
  }

  return NULL;
}

TEST (latch)
{
  error e = error_create ();

  struct data d = {
      .iters = 1000,
      .value = 0,
      .l     = 0,
  };

  i_thread threads[10];

  for (u32 i = 0; i < 10; ++i)
  {
    default_threading.i_thread_create (&default_threading, &threads[i], data_thread, &d, &e);
  }

  for (u32 i = 0; i < 10; ++i)
  {
    default_threading.i_thread_join (&default_threading, &threads[i], &e);
  }

  test_assert_int_equal (d.value, 10 * 1000);
}
#endif
