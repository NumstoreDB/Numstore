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

#include "concurrency.h"

#include "testing/testing.h"

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
  const err_t result = i_mutex_create (&l->mutex, e);
  if (result != SUCCESS)
  {
    return result;
  }

  memset (l->holder_counts, 0, sizeof (l->holder_counts));
  l->head = NULL;

  return SUCCESS;
}

void
gr_lock_destroy (struct gr_lock *l)
{
  i_mutex_free (&l->mutex);

  // Note: This assumes no threads are still waiting
  // TODO - (6) Caller must ensure all threads have released locks

  while (l->head)
  {
    struct gr_lock_waiter *w = l->head;
    l->head                  = w->next;
    i_cond_free (&w->cond);
  }
}

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

static void
wake_waiters (struct gr_lock *l)
{
  for (struct gr_lock_waiter *w = l->head; w; w = w->next)
  {
    if (is_compatible (l, w->mode))
    {
      i_cond_signal (&w->cond);
    }
  }
}

static err_t
gr_lock_waiter_init (
    struct gr_lock_waiter *dest,
    const enum lock_mode   mode,
    error                 *e
)
{
  dest->mode = mode;
  dest->prev = NULL;
  dest->next = NULL;

  if (i_cond_create (&dest->cond, e))
  {
    return error_trace (e);
  }

  return SUCCESS;
}

static void
gr_lock_waiter_append_unsafe (struct gr_lock *l, struct gr_lock_waiter *w)
{
  // Append on the front
  if (l->head == NULL)
  {
    l->head = w;
  }
  else
  {
    // Search for the end
    struct gr_lock_waiter *head = l->head;
    while (head->next != NULL)
    {
      head = head->next;
    }

    // Append on the end
    head->next = w;
    w->prev    = head;
  }
}

static void
gr_lock_waiter_remove_unsafe (struct gr_lock *l, const struct gr_lock_waiter *w)
{
  if (w->prev != NULL)
  {
    w->prev->next = w->next;
  }
  else
  {
    ASSERT (l->head == w);
    l->head = w->next;
  }

  if (w->next != NULL)
  {
    w->next->prev = w->prev;
  }
}

err_t
gr_lock (struct gr_lock *l, const enum lock_mode mode, error *e)
{
  i_mutex_lock (&l->mutex);

  // Is compatible - add this lock mode to the lock group
  // and move on
  if (is_compatible (l, mode))
  {
    l->holder_counts[mode]++;
    i_mutex_unlock (&l->mutex);
    return SUCCESS;
  }

  // Create a new waiter
  struct gr_lock_waiter waiter;
  if (gr_lock_waiter_init (&waiter, mode, e))
  {
    i_mutex_unlock (&l->mutex);
    return error_trace (e);
  }

  // Append it to the end of the waiter list
  gr_lock_waiter_append_unsafe (l, &waiter);

  // Main wait code
  while (!is_compatible (l, mode))
  {
    i_cond_wait (&waiter.cond, &l->mutex);
  }

  // Remove from waiters list
  gr_lock_waiter_remove_unsafe (l, &waiter);

  i_cond_free (&waiter.cond);

  // Acquire the lock
  l->holder_counts[mode]++;

  i_mutex_unlock (&l->mutex);

  return SUCCESS;
}

bool
gr_trylock (struct gr_lock *l, const enum lock_mode mode)
{
  ASSERT (l);

  if (!i_mutex_try_lock (&l->mutex))
  {
    return false;
  }

  if (!is_compatible (l, mode))
  {
    i_mutex_unlock (&l->mutex);
    return false;
  }

  l->holder_counts[mode]++;
  i_mutex_unlock (&l->mutex);

  return true;
}

void
gr_unlock (struct gr_lock *l, const enum lock_mode mode)
{
  i_mutex_lock (&l->mutex);

  ASSERT (l->holder_counts[mode] > 0);

  l->holder_counts[mode]--;

  // Wake any compatible waiters
  if (l->head)
  {
    wake_waiters (l);
  }

  i_mutex_unlock (&l->mutex);
}

const char *
gr_lock_mode_name (const enum lock_mode mode)
{
  if (mode >= 0 && mode < LM_COUNT)
  {
    return mode_names[mode];
  }
  return "INVALID";
}

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
      UNREACHABLE ();
    }
  }
  UNREACHABLE ();
}

#ifndef NTEST
#  include <stdatomic.h>
#  include <stdint.h>
#  include <string.h>

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
  i_mutex_create (&ctx->gate_mtx, NULL);
  i_cond_create (&ctx->gate_cv, NULL);
  ctx->gate_open = false;
}

static void
test_ctx_destroy (struct lock_test_ctx *ctx)
{
  i_mutex_free (&ctx->gate_mtx);
  i_cond_free (&ctx->gate_cv);
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
  i_mutex_lock (&ctx->gate_mtx);
  ctx->t1_acquired = 1;
  ctx->gate_open   = true;
  i_cond_broadcast (&ctx->gate_cv);
  i_mutex_unlock (&ctx->gate_mtx);

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
  i_mutex_lock (&ctx->gate_mtx);
  while (!ctx->gate_open)
  {
    i_cond_wait (&ctx->gate_cv, &ctx->gate_mtx);
  }
  i_mutex_unlock (&ctx->gate_mtx);

  // Attempt acquisition (will block if incompatible)
  ctx->t2_blocked = 1;
  gr_lock (ctx->lock, ctx->mode2, &e);

  ctx->t2_acquired = 1;
  ctx->t2_blocked  = 0;
  gr_unlock (ctx->lock, ctx->mode2);

  return NULL;
}

/* --- Randomized Stress Test Routine --- */

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
  i_thread_create (&t1, thread_hold_and_signal, &ctx, &e);
  i_thread_create (&t2, thread_wait_and_try, &ctx, &e);

  i_thread_join (&t1, &e);
  i_thread_join (&t2, &e);

  test_assert (ctx.t1_acquired && ctx.t2_acquired);
  test_ctx_destroy (&ctx);
  gr_lock_destroy (&lock);
}

/**
// Example of a Blocking Test (Incompatible)
// This one is breaking on Mac Os
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
  i_thread_create (&t1, thread_hold_and_signal, &ctx, &e);
  i_thread_create (&t2, thread_wait_and_try, &ctx, &e);

  // Wait slightly to let T2 hit the block, then check status
  i_sleep_ms (50);
  test_assert (ctx.t1_acquired);
  test_assert (ctx.t2_blocked);
  test_assert (!ctx.t2_acquired);

  i_thread_join (&t1, &e);
  i_thread_join (&t2, &e);

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
    i_thread_create (&threads[i], random_stress_worker, &ctx, &e);
  }

  for (int i = 0; i < 12; i++)
  {
    i_thread_join (&threads[i], &e);
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

  if (i_mutex_create (&t->mutex, e))
  {
    goto theend;
  }
  if (i_cond_create (&t->wake_cond, e))
  {
    goto fail_mutex;
  }
  if (i_cond_create (&t->done_cond, e))
  {
    goto fail_wake_cond;
  }

  goto theend;

  i_cond_free (&t->done_cond);
fail_wake_cond:
  i_cond_free (&t->wake_cond);
fail_mutex:
  i_mutex_free (&t->mutex);
theend:
  return error_trace (e);
}

static void *
periodic_task_thread (void *_ctx)
{
  struct periodic_task *t = _ctx;

  while (true)
  {
    i_mutex_lock (&t->mutex);
    // TODO - spurrious wakeups
    if (!t->wake_requested && !t->stop)
    {
      i_cond_timed_wait (&t->wake_cond, &t->mutex, t->msec);
    }
    t->wake_requested = false;
    bool should_stop  = t->stop;
    i_mutex_unlock (&t->mutex);

    if (should_stop)
    {
      break;
    }

    t->fn (t->ctx);
  }

  i_mutex_lock (&t->mutex);
  t->done = true;
  i_cond_signal (&t->done_cond);
  i_mutex_unlock (&t->mutex);

  return NULL;
}

err_t
periodic_task_start (
    struct periodic_task *t,
    u64                   msec,
    periodic_task_fn      fn,
    void                 *ctx,
    error                *e
)
{
  t->msec = msec;
  t->fn   = fn;
  t->ctx  = ctx;

  if (i_thread_create (&t->thread, periodic_task_thread, t, e))
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

  i_mutex_lock (&t->mutex);
  t->stop = true;
  i_cond_signal (&t->wake_cond);
  i_mutex_unlock (&t->mutex);

  i_mutex_lock (&t->mutex);
  while (!t->done)
  {
    i_cond_wait (&t->done_cond, &t->mutex);
  }
  i_mutex_unlock (&t->mutex);

  i_thread_join (&t->thread, e);
  i_cond_free (&t->done_cond);
  i_cond_free (&t->wake_cond);
  i_mutex_free (&t->mutex);
  t->running = false;

  return error_trace (e);
}

void
periodic_task_wake (struct periodic_task *t)
{
  i_mutex_lock (&t->mutex);
  t->wake_requested = true;
  i_cond_signal (&t->wake_cond);
  i_mutex_unlock (&t->mutex);
}

/******************************************************************************
 * SECTION: Latch (tests)
 ******************************************************************************/

#ifndef NTEST
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
    i_thread_create (&threads[i], data_thread, &d, &e);
  }

  for (u32 i = 0; i < 10; ++i)
  {
    i_thread_join (&threads[i], &e);
  }

  test_assert_int_equal (d.value, 10 * 1000);
}
#endif
