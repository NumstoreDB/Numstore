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

#include "core/ns_platform.h"

#if PLATFORM_WINDOWS

#  include "core/ns_csx_assert.h"
#  include "core/ns_error.h" // error
#  include "core/ns_serial.h"
#  include "core/os/ns_threading.h"

#  include <stddef.h>
#  include <stdio.h>
#  include <string.h>

#  define WIN32_LEAN_AND_MEAN
#  include "windows.h"

/******************************************************************************
 * SECTION: Threading
 ******************************************************************************/

/*-----------------------------------------------------------------------------
 * SUBSECTION: Condition Variable
 *----------------------------------------------------------------------------*/

static err_t
win32_cond_create (i_threading *t, i_cond *c, error *e)
{
  (void)t;
  ASSERT (c);
  (void)e;
  InitializeConditionVariable (&c->cond);
  return SUCCESS;
}

static void
win32_cond_free (i_threading *t, i_cond *c)
{
  (void)t;
  ASSERT (c);
  // No-op: CONDITION_VARIABLE has no destroy function.
}

static void
win32_cond_wait (i_threading *t, i_cond *c, i_mutex *m)
{
  (void)t;
  ASSERT (c);
  ASSERT (m);

  if (!SleepConditionVariableCS (&c->cond, &m->m, INFINITE)) {
    i_log_error ("cond_wait: SleepConditionVariableCS failed: %lu\n", GetLastError ());
    UNREACHABLE (); // LCOV_EXCL_LINE
  }
}

static void
win32_cond_timed_wait (i_threading *t, i_cond *c, i_mutex *m, u64 msec)
{
  (void)t;
  ASSERT (c);
  ASSERT (m);
  if (!SleepConditionVariableCS (&c->cond, &m->m, (DWORD)msec)) {
    DWORD err = GetLastError ();
    if (err != ERROR_TIMEOUT) {
      i_log_error ("cond_timed_wait: SleepConditionVariableCS failed: %lu\n", err);
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
}

static void
win32_cond_signal (i_threading *t, i_cond *c)
{
  (void)t;
  ASSERT (c);
  WakeConditionVariable (&c->cond);
}

static void
win32_cond_broadcast (i_threading *t, i_cond *c)
{
  (void)t;
  ASSERT (c);
  WakeAllConditionVariable (&c->cond);
}

/*-----------------------------------------------------------------------------
 * SUBSECTION: Mutex
 *----------------------------------------------------------------------------*/

#  ifndef NDEBUG
static DWORD
cs_owner (i_mutex *m)
{
  // OwningThread is documented as a HANDLE but is actually the
  // thread ID cast to a HANDLE on all shipping Windows versions.
  return (DWORD)(uintptr_t)((CRITICAL_SECTION *)&m->m)->OwningThread;
}
#  endif

static err_t
win32_mutex_create (i_threading *t, i_mutex *dest, error *e)
{
  (void)t;
  ASSERT (dest);
  (void)e;
  // dwSpinCount=0: no spinning, go straight to kernel wait.
  // Use a non-zero value (e.g. 4000) if profiling shows contention.
  InitializeCriticalSectionAndSpinCount (&dest->m, 0);
  return SUCCESS;
}

static void
win32_mutex_free (i_threading *t, i_mutex *m)
{
  (void)t;
  ASSERT (m);
#  ifndef NDEBUG
  DWORD owner = cs_owner (m);
  if (owner != 0) {
    i_log_error ("mutex_destroy: still locked by thread %lu\n", owner);
    UNREACHABLE (); // LCOV_EXCL_LINE
  }
#  endif
  DeleteCriticalSection (&m->m);
}

static void
win32_mutex_lock (i_threading *t, i_mutex *m)
{
  (void)t;
  ASSERT (m);
#  ifndef NDEBUG
  DWORD tid = GetCurrentThreadId ();
  if (cs_owner (m) == tid) {
    i_log_error ("mutex_lock: deadlock - thread %lu already owns mutex\n", tid);
    UNREACHABLE (); // LCOV_EXCL_LINE
  }
#  endif
  EnterCriticalSection (&m->m);
}

static void
win32_mutex_unlock (i_threading *t, i_mutex *m)
{
  (void)t;
  ASSERT (m);
#  ifndef NDEBUG
  DWORD tid = GetCurrentThreadId ();
  if (cs_owner (m) != tid) {
    i_log_error ("mutex_unlock: thread %lu does not own mutex\n", tid);
    UNREACHABLE (); // LCOV_EXCL_LINE
  }
#  endif
  LeaveCriticalSection (&m->m);
}

/*-----------------------------------------------------------------------------
 * SUBSECTION: Threading
 *----------------------------------------------------------------------------*/

typedef struct
{
  void *(*func) (void *);
  void *arg;
} thread_trampoline_args;

static DWORD WINAPI
thread_trampoline (LPVOID param)
{
  thread_trampoline_args *args = (thread_trampoline_args *)param;
  void *(*func) (void *)       = args->func;
  void *arg                    = args->arg;
  HeapFree (GetProcessHeap (), 0, args);
  func (arg);
  return 0;
}

static err_t
win32_thread_create (
    i_threading *t,
    i_thread    *dest,
    void *(*func) (void *),
    void  *context,
    error *e
)
{
  (void)t;
  ASSERT (dest);

  thread_trampoline_args *args = HeapAlloc (GetProcessHeap (), 0, sizeof *args);
  if (!args) {
    return error_causef (e, ERR_NOMEM, "thread_create: HeapAlloc failed");
  }

  args->func   = func;
  args->arg    = context;

  dest->handle = CreateThread (NULL, 0, thread_trampoline, args, 0, &dest->id);
  if (!dest->handle) {
    HeapFree (GetProcessHeap (), 0, args);
    char buf[256];
    FormatMessageA (
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError (),
        0,
        buf,
        sizeof (buf),
        NULL
    );
    return error_causef (e, ERR_IO, "CreateThread: %s", buf);
  }

  return SUCCESS;
}

static err_t
win32_thread_join (i_threading *t, i_thread *th, error *e)
{
  (void)t;
  ASSERT (th);
  ASSERT (th->handle);

  DWORD ret = WaitForSingleObject (th->handle, INFINITE);
  if (ret != WAIT_OBJECT_0) {
    char buf[256];
    FormatMessageA (
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError (),
        0,
        buf,
        sizeof (buf),
        NULL
    );
    return error_causef (e, ERR_IO, "thread_join: %s", buf);
  }

  CloseHandle (th->handle);
  th->handle = NULL;
  return SUCCESS;
}

////////////////////////////////////////////////////////////
// Default threading vtable

struct i_threading default_threading = {
    .i_thread_create   = win32_thread_create,
    .i_thread_join     = win32_thread_join,

    .i_mutex_create    = win32_mutex_create,
    .i_mutex_free      = win32_mutex_free,
    .i_mutex_lock      = win32_mutex_lock,
    .i_mutex_unlock    = win32_mutex_unlock,

    .i_cond_create     = win32_cond_create,
    .i_cond_free       = win32_cond_free,
    .i_cond_wait       = win32_cond_wait,
    .i_cond_timed_wait = win32_cond_timed_wait,
    .i_cond_signal     = win32_cond_signal,
    .i_cond_broadcast  = win32_cond_broadcast,
};

#endif
