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

#include <c_specx.h>

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
