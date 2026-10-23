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

#ifndef NS_PERIODIC_TASK_H
#define NS_PERIODIC_TASK_H

#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "core/os/ns_os.h"

#include <stdatomic.h>
#include <stdbool.h>

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
  // The OS that created the thread, mutex and conds below
  struct i_os      os;
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

err_t periodic_task_init (struct periodic_task *t, struct i_os os, error *e);

err_t periodic_task_start (
    struct periodic_task *t,
    u64                   msec,
    periodic_task_fn      fn,
    void                 *ctx,
    error                *e
);

err_t periodic_task_stop (struct periodic_task *t, error *e);

#endif // NS_PERIODIC_TASK_H
