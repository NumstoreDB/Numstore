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

#include "csx_assert.h"
#include "error.h"
#include "os.h"

#ifdef TESTING
#  include "testing/testing.h"
#endif

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

#ifdef TESTING
TEST (gr_lock_init)
{
  TEST_CASE ("mutex create fails")
  {
    error e = error_create ();

    err_t (*backup) (i_threading *t, i_mutex *m, error *e) =
        default_threading.i_mutex_create;
    default_threading.i_mutex_create = i_mutex_create_errio;

    struct gr_lock l;
    test_err_t_check (gr_lock_init (&l, &e), ERR_IO, &e);

    default_threading.i_mutex_create = backup;
  }

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
  i_mutex_lock (&l->mutex);
  // TODO - Caller must ensure all threads have released locks
  // You could put a done flag - and assert !done on actions
  i_mutex_unlock (&l->mutex);

  i_mutex_free (&l->mutex);

  while (l->head)
  {
    struct gr_lock_waiter *w = l->head;
    l->head                  = w->next;
    i_cond_free (&w->cond);
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
  i_mutex_lock (&l->mutex);

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
  if (i_cond_create (&waiter.cond, e))
  {
    // Ok here - we just failed and everything is unlocked
    i_mutex_unlock (&l->mutex);
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
    i_cond_wait (&waiter.cond, &l->mutex);
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
  i_cond_free (&waiter.cond);

acquire:
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

  // acquire
  l->holder_counts[mode]++;
  i_mutex_unlock (&l->mutex);

  return true;
}

void
gr_unlock (struct gr_lock *l, const enum lock_mode mode)
{
  i_mutex_lock (&l->mutex);

  // do unlock
  ASSERT (l->holder_counts[mode] > 0);
  l->holder_counts[mode]--;

  // Wake any compatible waiters
  if (l->head)
  {
    for (struct gr_lock_waiter *w = l->head; w; w = w->next)
    {
      // signal all waiters - they do the compatability check - it's ok
      i_cond_signal (&w->cond);
    }
  }

  i_mutex_unlock (&l->mutex);
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

        i_thread_create (&t1, thread1, &ctx, &e);
        i_thread_create (&t2, thread2, &ctx, &e);

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

        i_thread_join (&t1, &e);
        i_thread_join (&t2, &e);
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
    i_thread_create (&threads[i], data_thread, &d, &e);
  }

  for (u32 i = 0; i < 10; ++i)
  {
    i_thread_join (&threads[i], &e);
  }

  test_assert_int_equal (d.value, 10 * 1000);
}
#endif
