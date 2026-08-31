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

#include "core/os/ns_os_vtable.h"

/******************************************************************************
 * SECTION: Threading
 * ----------------------------------------------------------------------------
 * @brief Wrappers around various threading utilities
 *
 * Numstore makes heavy use of threads - a single threaded environment
 * could be possible, but would require a major rework using cooperative
 * routines
 ******************************************************************************/

struct i_mutex
{
#ifdef _WIN32
  CRITICAL_SECTION m;
#else
  pthread_mutex_t m;
#endif
};

struct i_cond
{
#ifdef _WIN32
  CONDITION_VARIABLE cond;
#else
  pthread_cond_t cond;
#endif
};

struct i_thread
{
#ifdef _WIN32
  HANDLE handle;
  DWORD  id;
#else
  pthread_t thread;
#endif
};

/*-----------------------------------------------------------------------------
 * SUBSECTION: Abstraction
 *----------------------------------------------------------------------------*/

struct i_threading
{
  const struct os_vtable *table;
  void                   *data;
};

struct i_threading default_threading (void);

#define i_thread_create(th, dest, start_routine, arg, e) \
  (th).table->thread_create ((th).data, dest, start_routine, arg, e)
#define i_thread_join(th, dest, e) (th).table->thread_join ((th).data, dest, e)

#define i_mutex_create(th, m, e) (th).table->mutex_create ((th).data, m, e)
#define i_mutex_free(th, m)      (th).table->mutex_free ((th).data, m)
#define i_mutex_lock(th, m)      (th).table->mutex_lock ((th).data, m)
#define i_mutex_unlock(th, m)    (th).table->mutex_unlock ((th).data, m)

#define i_cond_create(th, c, e)         (th).table->cond_create ((th).data, c, e)
#define i_cond_free(th, c)              (th).table->cond_free ((th).data, c)
#define i_cond_wait(th, c, m)           (th).table->cond_wait ((th).data, c, m)
#define i_cond_timed_wait(th, c, m, ms) (th).table->cond_timed_wait ((th).data, c, m, ms)
#define i_cond_signal(th, c)            (th).table->cond_signal ((th).data, c)
#define i_cond_broadcast(th, c)         (th).table->cond_broadcast ((th).data, c)

#endif
