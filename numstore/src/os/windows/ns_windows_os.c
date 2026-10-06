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
#  include "core/ns_error.h"
#  include "core/ns_slab_alloc.h"
#  include "core/ns_stdtypes.h"
#  include "os/ns_malloc.h"
#  include "os/windows/ns_windows_os.h"

// Interlocked* instead of <stdatomic.h>: MSVC only ships C11 atomics behind
// /experimental:c11atomics, and these compile everywhere (MSVC, MinGW, clang-cl).

#  define IS_POW2(x) ((x) > 0 && ((x) & ((x) - 1)) == 0)

_Static_assert (IS_POW2 (MAX_OPEN_FILES), "MAX_OPEN_FILES must be a power of 2");
_Static_assert (IS_POW2 (MAX_THREADS), "MAX_THREADS must be a power of 2");

#  define THREAD_SLOT_FREE     0
#  define THREAD_SLOT_OCCUPIED 1

DEFINE_DBG_ASSERT (struct windows_os, windows_os, os, {
  ASSERT (os);
  ASSERT (os->mem.table);
})

DEFINE_DBG_ASSERT (struct windows_os, windows_os_empty, os, {
  for (int i = 0; i < MAX_OPEN_FILES; ++i) {
    ASSERT (os->handles[i] == WIN_SLOT_FREE); // leaked file
  }
  for (int i = 0; i < MAX_THREADS; ++i) {
    ASSERT (os->threads[i].occupied == THREAD_SLOT_FREE); // unjoined thread
  }
})

struct windows_os *
windows_create (struct i_mem mem, error *e)
{
  // calloc: every handle slot starts NULL (free), every thread slot 0 (free)
  struct windows_os *ret = mem.table->calloc (mem.self, 1, sizeof *ret, e);
  if (ret == NULL) {
    return NULL;
  }

  slab_alloc_init (&ret->mutex_alloc, mem, sizeof (CRITICAL_SECTION), 100);
  slab_alloc_init (&ret->cond_alloc, mem, sizeof (CONDITION_VARIABLE), 100);
  slab_alloc_init (&ret->timer_alloc, mem, sizeof (struct windows_timer), 100);

  ret->mem = mem;

  // Publish the zeroed tables before any other thread sees ret
  MemoryBarrier ();

  DBG_ASSERT (windows_os, ret);

  return ret;
}

void
windows_free (struct windows_os *w)
{
  DBG_ASSERT (windows_os, w);
  DBG_ASSERT (windows_os_empty, w);

  slab_alloc_destroy (&w->mutex_alloc);
  slab_alloc_destroy (&w->cond_alloc);
  slab_alloc_destroy (&w->timer_alloc);

  // Copy out before freeing - mem lives inside w
  struct i_mem mem = w->mem;
  mem.table->free (mem.self, w);
}

////////////////////////////////////////////////////////////
// File handles

PVOID
windows_handle_load (PVOID volatile *slot)
{
  ASSERT (slot);
  // CAS with equal operands = atomic read with a full barrier
  return InterlockedCompareExchangePointer (slot, NULL, NULL);
}

void
windows_handle_store (PVOID volatile *slot, PVOID value)
{
  ASSERT (slot);
  InterlockedExchangePointer (slot, value);
}

PVOID volatile *
windows_handle_reserve (struct windows_os *os, error *e)
{
  DBG_ASSERT (windows_os, os);

  for (int i = 0; i < MAX_OPEN_FILES; ++i) {
    // clock = (clock + 1) % MAX_OPEN_FILES
    u32 clock = (u32)InterlockedIncrement (&os->handle_clock) & (MAX_OPEN_FILES - 1);

    if (InterlockedCompareExchangePointer (&os->handles[clock], WIN_SLOT_PENDING, WIN_SLOT_FREE)
        == WIN_SLOT_FREE) {
      return &os->handles[clock];
    }
  }

  error_causef (e, ERR_TOO_MANY_FILES, "Too many files are open");
  return NULL;
}

void
windows_handle_free (struct windows_os *os, PVOID volatile *slot)
{
  DBG_ASSERT (windows_os, os);
  ASSERT (slot);
  ASSERT (slot >= os->handles && slot < os->handles + MAX_OPEN_FILES);

  const PVOID v = windows_handle_load (slot);
  ASSERT (v != WIN_SLOT_FREE);
  (void)v;

  windows_handle_store (slot, WIN_SLOT_FREE);
}

////////////////////////////////////////////////////////////
// Threads

struct windows_thread_frame *
windows_thread_alloc (struct windows_os *os, error *e)
{
  DBG_ASSERT (windows_os, os);

  for (int i = 0; i < MAX_THREADS; ++i) {
    u32 clock = (u32)InterlockedIncrement (&os->thread_clock) & (MAX_THREADS - 1);

    if (InterlockedCompareExchange (
            &os->threads[clock].occupied,
            THREAD_SLOT_OCCUPIED,
            THREAD_SLOT_FREE
        )
        == THREAD_SLOT_FREE) {
      struct windows_thread_frame *t = &os->threads[clock];
      t->handle                      = NULL;
      t->id                          = 0;
      t->func                        = NULL;
      t->arg                         = NULL;
      return t;
    }
  }

  error_causef (e, ERR_TOO_MANY_THREADS, "Failed to create a thread, too many threads are open");
  return NULL;
}

void
windows_thread_free (struct windows_os *os, struct windows_thread_frame *t)
{
  DBG_ASSERT (windows_os, os);
  ASSERT (t);
  ASSERT (t >= os->threads && t < os->threads + MAX_THREADS);
  ASSERT (t->occupied == THREAD_SLOT_OCCUPIED);

  t->handle = NULL;
  InterlockedExchange (&t->occupied, THREAD_SLOT_FREE);
}

////////////////////////////////////////////////////////////
// Slab backed objects

CRITICAL_SECTION *
windows_mutex_alloc (struct windows_os *os, error *e)
{
  DBG_ASSERT (windows_os, os);
  return slab_alloc_alloc (&os->mutex_alloc, e);
}

void
windows_mutex_free (struct windows_os *os, CRITICAL_SECTION *m)
{
  DBG_ASSERT (windows_os, os);
  ASSERT (m);
  slab_alloc_free (&os->mutex_alloc, m);
}

CONDITION_VARIABLE *
windows_cond_alloc (struct windows_os *os, error *e)
{
  DBG_ASSERT (windows_os, os);
  return slab_alloc_alloc (&os->cond_alloc, e);
}

void
windows_cond_free (struct windows_os *os, CONDITION_VARIABLE *c)
{
  DBG_ASSERT (windows_os, os);
  ASSERT (c);
  slab_alloc_free (&os->cond_alloc, c);
}

struct windows_timer *
windows_timer_alloc (struct windows_os *os, error *e)
{
  DBG_ASSERT (windows_os, os);
  return slab_alloc_alloc (&os->timer_alloc, e);
}

void
windows_timer_free (struct windows_os *os, struct windows_timer *t)
{
  DBG_ASSERT (windows_os, os);
  ASSERT (t);
  slab_alloc_free (&os->timer_alloc, t);
}

#endif // PLATFORM_WINDOWS
