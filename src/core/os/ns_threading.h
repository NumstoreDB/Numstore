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

#ifndef NS_THREADING_H
#define NS_THREADING_H

#include "core/ns_error.h"
#include "core/ns_stdtypes.h"

/******************************************************************************
 * SECTION: Threading
 * ----------------------------------------------------------------------------
 * @brief Wrappers around various threading utilities
 *
 * Numstore makes heavy use of threads - a single threaded environment
 * could be possible, but would require a major rework using cooperative
 * routines
 ******************************************************************************/

typedef struct
{
#if defined(_WIN32)
  CRITICAL_SECTION m;
#else
  pthread_mutex_t m;
#endif
} i_mutex;

typedef struct
{
#if defined(_WIN32)
  CONDITION_VARIABLE cond;
#else
  pthread_cond_t cond;
#endif
} i_cond;

typedef struct
{
#if defined(_WIN32)
  HANDLE handle;
  DWORD  id;
#else
  pthread_t thread;
#endif
} i_thread;

/*-----------------------------------------------------------------------------
 * SUBSECTION: Abstraction
 *----------------------------------------------------------------------------*/

typedef struct i_threading i_threading;

struct i_threading
{
  err_t (*i_thread_create) (
      i_threading *t,
      i_thread    *th,
      void *(*start_routine) (void *),
      void  *arg,
      error *e
  );
  err_t (*i_thread_join) (i_threading *t, i_thread *th, error *e);

  err_t (*i_mutex_create) (i_threading *t, i_mutex *m, error *e);
  void (*i_mutex_free) (i_threading *t, i_mutex *m);
  void (*i_mutex_lock) (i_threading *t, i_mutex *m);
  void (*i_mutex_unlock) (i_threading *t, i_mutex *m);

  err_t (*i_cond_create) (i_threading *t, i_cond *c, error *e);
  void (*i_cond_free) (i_threading *t, i_cond *c);
  void (*i_cond_wait) (i_threading *t, i_cond *c, i_mutex *m);
  void (*i_cond_timed_wait) (i_threading *t, i_cond *c, i_mutex *m, u64 msec);
  void (*i_cond_signal) (i_threading *t, i_cond *c);
  void (*i_cond_broadcast) (i_threading *t, i_cond *c);
};

extern struct i_threading default_threading;

#endif
