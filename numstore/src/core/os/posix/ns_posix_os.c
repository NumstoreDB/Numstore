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

#include "core/os/posix/ns_posix_os.h"

#if PLATFORM_POSIX

#  include "core/ns_csx_assert.h"
#  include "core/ns_error.h"
#  include "core/ns_slab_alloc.h"
#  include "core/ns_stdtypes.h"
#  include "core/ns_utils.h"
#  include "core/os/ns_malloc.h"

#  include <pthread.h>
#  include <stdatomic.h>
#  include <stdlib.h>
#  include <string.h>
#  include <time.h>

#  define IS_POW2(x) ((x) > 0 && ((x) & ((x) - 1)) == 0)

_Static_assert (IS_POW2 (MAX_OPEN_FILES), "MAX_OPEN_FILES must be a power of 2");
_Static_assert (IS_POW2 (MAX_THREADS), "MAX_THREADS must be a power of 2");

DEFINE_DBG_ASSERT (struct posix_os, posix_os, os, {
  ASSERT (os);
  ASSERT (os->mem.table);
})

DEFINE_DBG_ASSERT (struct posix_os, posix_os_empty, os, {
  for (int i = 0; i < MAX_OPEN_FILES; ++i) {
    ASSERT (atomic_load (&os->fds[i]) == -1);
  }
  for (int i = 0; i < MAX_THREADS; ++i) {
    ASSERT (atomic_load (&os->threads[i].occupied) == 0);
  }
})

struct posix_os *
posix_create (struct i_mem mem, error *e)
{
  struct posix_os *ret = mem.table->calloc (mem.self, 1, sizeof *ret, e);
  if (ret == NULL) {
    return NULL;
  }

  // All are unoccupied
  for (int i = 0; i < MAX_OPEN_FILES; ++i) {
    atomic_init (&ret->fds[i], -1);
  }

  // All are unoccupied
  for (int i = 0; i < MAX_THREADS; ++i) {
    atomic_init (&ret->threads[i].occupied, 0);
  }

  atomic_init (&ret->fd_clock, 0);
  atomic_init (&ret->pthread_clock, 0);

  slab_alloc_init (&ret->mutex_alloc, mem, sizeof (pthread_mutex_t), 100);
  slab_alloc_init (&ret->cond_alloc, mem, sizeof (pthread_cond_t), 100);
  slab_alloc_init (&ret->timer_alloc, mem, sizeof (struct timespec), 100);

  ret->mem = mem;

  DBG_ASSERT (posix_os, ret);

  return ret;
}

void
posix_free (struct posix_os *p)
{
  DBG_ASSERT (posix_os, p);
  DBG_ASSERT (posix_os_empty, p);

  slab_alloc_destroy (&p->mutex_alloc);
  slab_alloc_destroy (&p->cond_alloc);
  slab_alloc_destroy (&p->timer_alloc);

  // Copy out before freeing - mem lives inside p
  struct i_mem mem = p->mem;
  mem.table->free (mem.self, p);
}

////////////////////////////////////////////////////////////
// File descriptors

_Atomic int *
posix_fd_reserve (struct posix_os *os, error *e)
{
  DBG_ASSERT (posix_os, os);

  for (int i = 0; i < MAX_OPEN_FILES; ++i) {
    // clock = (clock + 1) % MAX_OPEN_FILES
    u32 clock      = (u32)atomic_fetch_add (&os->fd_clock, 1) & (MAX_OPEN_FILES - 1);

    int unoccupied = -1;
    if (atomic_compare_exchange_strong (&os->fds[clock], &unoccupied, -2)) {
      return &os->fds[clock];
    }
  }

  error_causef (e, ERR_TOO_MANY_FILES, "Too many files are open");
  return NULL;
}

void
posix_fd_free (struct posix_os *os, _Atomic int *fd)
{
  DBG_ASSERT (posix_os, os);
  ASSERT (fd);
  ASSERT (fd >= os->fds && fd < os->fds + MAX_OPEN_FILES);

  const int v = atomic_load (fd);
  ASSERT (v >= 0 || v == -2);
  (void)v;

  atomic_store_explicit (fd, -1, memory_order_release);
}

////////////////////////////////////////////////////////////
// Threads

pthread_t *
posix_pthread_alloc (struct posix_os *os, error *e)
{
  DBG_ASSERT (posix_os, os);

  for (int i = 0; i < MAX_THREADS; ++i) {
    u32 clock      = (u32)atomic_fetch_add (&os->pthread_clock, 1) & (MAX_THREADS - 1);

    int unoccupied = 0;
    if (atomic_compare_exchange_strong (&os->threads[clock].occupied, &unoccupied, 1)) {
      return &os->threads[clock].thread;
    }
  }

  error_causef (e, ERR_TOO_MANY_THREADS, "Failed to create a thread, too many threads are open");
  return NULL;
}

void
posix_pthread_free (struct posix_os *os, pthread_t *thread)
{
  DBG_ASSERT (posix_os, os);
  ASSERT (thread);

  struct pthread_frame *frame = container_of (thread, struct pthread_frame, thread);
  ASSERT (frame >= os->threads && frame < os->threads + MAX_THREADS);
  ASSERT (atomic_load (&frame->occupied) == 1);

  atomic_store_explicit (&frame->occupied, 0, memory_order_release);
}

////////////////////////////////////////////////////////////
// Slab backed objects

pthread_mutex_t *
posix_pthread_mutex_alloc (struct posix_os *os, error *e)
{
  DBG_ASSERT (posix_os, os);
  return slab_alloc_alloc (&os->mutex_alloc, e);
}

void
posix_pthread_mutex_free (struct posix_os *os, pthread_mutex_t *mutex)
{
  DBG_ASSERT (posix_os, os);
  ASSERT (mutex);
  slab_alloc_free (&os->mutex_alloc, mutex);
}

pthread_cond_t *
posix_pthread_cond_alloc (struct posix_os *os, error *e)
{
  DBG_ASSERT (posix_os, os);
  return slab_alloc_alloc (&os->cond_alloc, e);
}

void
posix_pthread_cond_free (struct posix_os *os, pthread_cond_t *cond)
{
  DBG_ASSERT (posix_os, os);
  ASSERT (cond);
  slab_alloc_free (&os->cond_alloc, cond);
}

struct timespec *
posix_timer_alloc (struct posix_os *os, error *e)
{
  DBG_ASSERT (posix_os, os);
  return slab_alloc_alloc (&os->timer_alloc, e);
}

void
posix_timer_free (struct posix_os *os, struct timespec *timer)
{
  DBG_ASSERT (posix_os, os);
  ASSERT (timer);
  slab_alloc_free (&os->timer_alloc, timer);
}

#endif // PLATFORM_POSIX
