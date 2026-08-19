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

#include <inttypes.h>
#include <stdbool.h>
#include <sys/types.h>

#include "core/ns_logging.h"
#include "core/ns_platform.h"
#include "core/ns_stdtypes.h"

#if PLATFORM_POSIX

#  include <dirent.h>
#  include <errno.h>
#  include <fcntl.h>
#  include <limits.h>
#  include <pthread.h>
#  include <stdio.h>
#  include <string.h>
#  include <sys/stat.h>
#  include <sys/uio.h>
#  include <time.h>
#  include <unistd.h>

#  include "core/ns_csx_assert.h"
#  include "core/ns_error.h"
#  include "core/os/ns_threading.h"

/******************************************************************************
 * SECTION: Threading
 ******************************************************************************/

/*-----------------------------------------------------------------------------
 * SUBSECTION: Condition Variables
 *----------------------------------------------------------------------------*/

static err_t
posix_cond_create (i_threading *t, i_cond *c, error *e)
{
  (void)t;
  ASSERT (c);

#  ifndef NDEBUG
  pthread_condattr_t attr;

  // I just don't want to handle errors for debug code
  int r1 = pthread_condattr_init (&attr);
  ASSERT (r1 == 0);

  const int r2 = pthread_cond_init (&c->cond, &attr);

  r1 = pthread_condattr_destroy (&attr);
  ASSERT (r1 == 0);

  if (r2)
#  else
  if (pthread_cond_init (&c->cond, NULL))
#  endif
  {
    switch (errno)
    {
      case EAGAIN:
      {
        return error_causef (e, ERR_IO, "pthread_cond_init: %s", strerror (errno));
      }

      case ENOMEM:
      {
        return error_causef (e, ERR_NOMEM, "pthread_cond_init: %s", strerror (errno));
      }

      case EBUSY:
      {
        i_log_error (
            "cond_create: cond "
            "already initialized: "
            "%s\n",
            strerror (errno)
        );
        UNREACHABLE (); // LCOV_EXCL_LINE
      }

      case EINVAL:
      {
        i_log_error (
            "cond_create: invalid "
            "attributes or cond: %s\n",
            strerror (errno)
        );
        UNREACHABLE (); // LCOV_EXCL_LINE
      }

      default:
      {
        i_log_error (
            "cond_create: unknown "
            "error: %s\n",
            strerror (errno)
        );
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
    }
  }

  return SUCCESS;
}

static void
posix_cond_free (i_threading *t, i_cond *c)
{
  (void)t;
  ASSERT (c);

  errno = 0;
  if (pthread_cond_destroy (&c->cond))
  {
    switch (errno)
    {
      case EBUSY:
      {
        i_log_error (
            "cond_free: cond has "
            "active waiters: %s\n",
            strerror (errno)
        );
        UNREACHABLE (); // LCOV_EXCL_LINE
      }

      case EINVAL:
      {
        i_log_error (
            "cond_free: invalid or "
            "uninitialized cond: %s\n",
            strerror (errno)
        );
        UNREACHABLE (); // LCOV_EXCL_LINE
      }

      default:
      {
        i_log_error (
            "cond_free: unknown "
            "error: %s\n",
            strerror (errno)
        );
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
    }
  }
}

static void
posix_cond_wait (i_threading *t, i_cond *c, i_mutex *m)
{
  (void)t;
  ASSERT (c);
  ASSERT (m);

  errno = 0;
  if (pthread_cond_wait (&c->cond, &m->m))
  {
    switch (errno)
    {
      case EINVAL:
      {
        i_log_error (
            "cond_wait: invalid cond "
            "or mutex: %s\n",
            strerror (errno)
        );
        UNREACHABLE (); // LCOV_EXCL_LINE
      }

      case EPERM:
      {
        i_log_error (
            "cond_wait: mutex not "
            "owned by thread: %s\n",
            strerror (errno)
        );
        UNREACHABLE (); // LCOV_EXCL_LINE
      }

      default:
      {
        i_log_error (
            "cond_wait: unknown "
            "error: %s\n",
            strerror (errno)
        );
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
    }
  }
}

static void
posix_cond_timed_wait (i_threading *t, i_cond *c, i_mutex *m, u64 msec)
{
  (void)t;
  ASSERT (c);
  ASSERT (m);

  struct timespec ts;
  clock_gettime (CLOCK_REALTIME, &ts);
  ts.tv_sec += msec / 1000;
  ts.tv_nsec += (msec % 1000) * 1000000LL;
  if (ts.tv_nsec >= 1000000000LL)
  {
    ts.tv_sec += 1;
    ts.tv_nsec -= 1000000000LL;
  }

  errno   = 0;
  int ret = pthread_cond_timedwait (&c->cond, &m->m, &ts);
  if (ret && ret != ETIMEDOUT)
  {
    switch (ret)
    {
      case EINVAL:
      {
        i_log_error (
            "cond_timed_wait: invalid cond, "
            "mutex, or abstime: %s\n",
            strerror (ret)
        );
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      case EPERM:
      {
        i_log_error (
            "cond_timed_wait: mutex not "
            "owned by thread: %s\n",
            strerror (ret)
        );
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      default:
      {
        i_log_error (
            "cond_timed_wait: unknown "
            "error: %s\n",
            strerror (ret)
        );
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
    }
  }
}

static void
posix_cond_signal (i_threading *t, i_cond *c)
{
  (void)t;
  ASSERT (c);

  errno = 0;
  if (pthread_cond_signal (&c->cond))
  {
    switch (errno)
    {
      case EINVAL:
      {
        i_log_error (
            "cond_signal: invalid or "
            "uninitialized cond: %s\n",
            strerror (errno)
        );
        UNREACHABLE (); // LCOV_EXCL_LINE
      }

      default:
      {
        i_log_error (
            "cond_signal: unknown "
            "error: %s\n",
            strerror (errno)
        );
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
    }
  }
}

static void
posix_cond_broadcast (i_threading *t, i_cond *c)
{
  (void)t;
  ASSERT (c);

  errno = 0;
  if (pthread_cond_broadcast (&c->cond))
  {
    switch (errno)
    {
      case EINVAL:
      {
        i_log_error (
            "cond_broadcast: invalid "
            "or uninitialized cond: "
            "%s\n",
            strerror (errno)
        );
        UNREACHABLE (); // LCOV_EXCL_LINE
      }

      default:
      {
        i_log_error (
            "cond_broadcast: unknown "
            "error: %s\n",
            strerror (errno)
        );
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
    }
  }
}

/*-----------------------------------------------------------------------------
 * SUBSECTION: Mutex
 *----------------------------------------------------------------------------*/

struct i_mutex_s
{
  pthread_mutex_t mutex;
};

static err_t
posix_mutex_create (i_threading *t, i_mutex *dest, error *e)
{
  (void)t;
  errno = 0;
#  ifndef NDEBUG
  pthread_mutexattr_t attr;

  // I just don't want to handle errors for debug code
  int r1 = pthread_mutexattr_init (&attr);
  ASSERT (!r1);

  r1 = pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_ERRORCHECK);
  ASSERT (!r1);

  const int r2 = pthread_mutex_init (&dest->m, NULL);

  r1 = pthread_mutexattr_destroy (&attr);
  ASSERT (!r1);
  if (r2)
#  else
  if (pthread_mutex_init (&dest->m, NULL))
#  endif
  {
    switch (errno)
    {
      case EAGAIN:
      {
        return error_causef (e, ERR_IO, "mutex_init: %s", strerror (errno));
      }
      case ENOMEM:
      {
        return error_causef (e, ERR_NOMEM, "mutex_init: %s", strerror (errno));
      }
      case EPERM:
      {
        i_log_error (
            "mutex_init: insufficient "
            "permissions: %s\n",
            strerror (errno)
        );
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      default:
      {
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
    }
  }

  return SUCCESS;
}

static void
posix_mutex_free (i_threading *t, i_mutex *m)
{
  (void)t;
  ASSERT (m);

  errno = 0;

  if (pthread_mutex_destroy (&m->m))
  {
    switch (errno)
    {
      case EBUSY:
      {
        i_log_error (
            "mutex_destroy: still "
            "locked: %s\n",
            strerror (errno)
        );
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      case EINVAL:
      {
        i_log_error (
            "mutex_destroy: "
            "invalid: %s\n",
            strerror (errno)
        );
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      default:
      {
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
    }
  }
}

static void
posix_mutex_lock (i_threading *t, i_mutex *m)
{
  (void)t;
  ASSERT (m);

  int ret = pthread_mutex_lock (&m->m);
  if (ret)
  {
    switch (ret)
    {
      case EINVAL:
      {
        i_log_error (
            "mutex_lock: "
            "invalid: %s\n",
            strerror (ret)
        );
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      case EAGAIN:
      {
        i_log_error (
            "mutex_lock: recursive "
            "lock: %s\n",
            strerror (ret)
        );
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      case EDEADLK:
      {
        i_log_error (
            "mutex_lock: "
            "deadlock: %s\n",
            strerror (ret)
        );
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      default:
      {
        i_log_error ("mutex_lock: %s\n", strerror (ret));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
    }
  }
}

static void
posix_mutex_unlock (i_threading *t, i_mutex *m)
{
  (void)t;
  ASSERT (m);

  errno = 0;
  if (pthread_mutex_unlock (&m->m))
  {
    switch (errno)
    {
      case EINVAL:
      {
        i_log_error (
            "mutex_unlock: "
            "invalid: %s\n",
            strerror (errno)
        );
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      case EAGAIN:
      {
        i_log_error (
            "mutex_unlock: recursive "
            "lock: %s\n",
            strerror (errno)
        );
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      case EPERM:
      {
        i_log_error (
            "mutex_unlock: "
            "not owner: %s\n",
            strerror (errno)
        );
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      default:
      {
        i_log_error ("mutex_unlock: %s\n", strerror (errno));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
    }
  }
}

/*-----------------------------------------------------------------------------
 * SUBSECTION: Thread
 *----------------------------------------------------------------------------*/

static err_t
posix_thread_create (
    i_threading *t,
    i_thread    *dest,
    void *(*func) (void *),
    void  *context,
    error *e
)
{
  (void)t;
  ASSERT (dest);

#  ifndef NDEBUG
  pthread_attr_t attr;
  int            r1 = pthread_attr_init (&attr);
  ASSERT (!r1);

  // Examples:
  // pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  // pthread_attr_setstacksize(&attr, 1 << 20);
  // pthread_attr_setguardsize(&attr, 4096);
  // pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
  // pthread_attr_setschedpolicy(&attr, SCHED_OTHER);
  // pthread_attr_setschedparam(&attr, &sched_param);

  const int r2 = pthread_create (&dest->thread, &attr, func, context);

  r1 = pthread_attr_destroy (&attr);
  ASSERT (!r1);
  if (r2)
#  else
  if (pthread_create (&dest->thread, NULL, func, context))
#  endif
  {
    switch (errno)
    {
      case EAGAIN:
      {
        return error_causef (e, ERR_IO, "pthread_create: %s", strerror (errno));
      }
      case EINVAL:
      {
        i_log_error (
            "pthread_create: invalid "
            "attributes: %s\n",
            strerror (errno)
        );
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      case EPERM:
      {
        i_log_error (
            "pthread_create: "
            "insufficient "
            "permissions: %s\n",
            strerror (errno)
        );
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      default:
      {
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
    }
  }

  return SUCCESS;
}

static err_t
posix_thread_join (i_threading *t, i_thread *th, error *e)
{
  (void)t;
  ASSERT (th);

  const int r = pthread_join (th->thread, NULL);

  if (r != 0)
  {
    switch (r)
    {
      case EDEADLK:
      {
        i_log_error (
            "pthread_join: "
            "deadlock: %s\n",
            strerror (r)
        );
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      case EINVAL:
      {
        i_log_error (
            "pthread_join: not "
            "joinable: %s\n",
            strerror (r)
        );
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      case ESRCH:
      {
        i_log_error (
            "pthread_join: no such "
            "thread: %s\n",
            strerror (r)
        );
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      default:
      {
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
    }
  }

  return SUCCESS;
}

/*-----------------------------------------------------------------------------
 * SUBSECTION: Abstraction
 *----------------------------------------------------------------------------*/

struct i_threading default_threading = {
    .i_thread_create = posix_thread_create,
    .i_thread_join   = posix_thread_join,

    .i_mutex_create = posix_mutex_create,
    .i_mutex_free   = posix_mutex_free,
    .i_mutex_lock   = posix_mutex_lock,
    .i_mutex_unlock = posix_mutex_unlock,

    .i_cond_create     = posix_cond_create,
    .i_cond_free       = posix_cond_free,
    .i_cond_wait       = posix_cond_wait,
    .i_cond_timed_wait = posix_cond_timed_wait,
    .i_cond_signal     = posix_cond_signal,
    .i_cond_broadcast  = posix_cond_broadcast,
};

#endif
