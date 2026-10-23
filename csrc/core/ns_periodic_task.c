#include "core/ns_periodic_task.h"

#include "ns_error.h"

/******************************************************************************
 * SECTION: Periodic Task
 ******************************************************************************/

err_t
periodic_task_init (struct periodic_task *t, const struct i_os os, error *e)
{
  t->os             = os;
  t->stop           = false;
  t->wake_requested = false;
  t->done           = false;
  t->running        = false;

  if (t->os.table->mutex_create (t->os.self, &t->mutex, e)) {
    goto theend;
  }
  if (t->os.table->cond_create (t->os.self, &t->wake_cond, e)) {
    goto fail_mutex;
  }
  if (t->os.table->cond_create (t->os.self, &t->done_cond, e)) {
    goto fail_wake_cond;
  }

  goto theend;

  // Commented this out - I am pretty sure I can - but it was on a whim
  // t->os.table->cond_free (t->os.self, t->done_cond.self);
fail_wake_cond:
  t->os.table->cond_free (t->os.self, t->wake_cond.self);
fail_mutex:
  t->os.table->mutex_free (t->os.self, t->mutex.self);
theend:
  return error_trace (e);
}

static void *
periodic_task_thread (void *_ctx)
{
  struct periodic_task *t = _ctx;

  while (true) {
    t->mutex.table->mutex_lock (t->mutex.self);
    // TODO - spurrious wakeups
    if (!t->wake_requested && !t->stop) {
      t->wake_cond.table->cond_timed_wait (t->wake_cond.self, t->mutex.self, t->msec);
    }
    t->wake_requested = false;
    bool should_stop  = t->stop;
    t->mutex.table->mutex_unlock (t->mutex.self);

    if (should_stop) {
      break;
    }

    t->fn (t->ctx);
  }

  t->mutex.table->mutex_lock (t->mutex.self);
  t->done = true;
  t->done_cond.table->cond_signal (t->done_cond.self);
  t->mutex.table->mutex_unlock (t->mutex.self);

  return NULL;
}

err_t
periodic_task_start (struct periodic_task *t, u64 msec, periodic_task_fn fn, void *ctx, error *e)
{
  t->msec = msec;
  t->fn   = fn;
  t->ctx  = ctx;

  if (t->os.table->thread_create (t->os.self, &t->thread, periodic_task_thread, t, e)) {
    return error_trace (e);
  }

  t->running = true;

  return SUCCESS;
}

err_t
periodic_task_stop (struct periodic_task *t, error *e)
{
  if (!t->running) {
    return SUCCESS;
  }

  t->mutex.table->mutex_lock (t->mutex.self);
  t->stop = true;
  t->wake_cond.table->cond_signal (t->wake_cond.self);
  t->mutex.table->mutex_unlock (t->mutex.self);

  t->mutex.table->mutex_lock (t->mutex.self);
  while (!t->done) {
    t->done_cond.table->cond_wait (t->done_cond.self, t->mutex.self);
  }
  t->mutex.table->mutex_unlock (t->mutex.self);

  t->os.table->thread_join (t->os.self, t->thread.self);
  t->os.table->cond_free (t->os.self, t->done_cond.self);
  t->os.table->cond_free (t->os.self, t->wake_cond.self);
  t->os.table->mutex_free (t->os.self, t->mutex.self);
  t->running = false;

  return error_trace (e);
}
