#ifndef NS_POSIX_OS_H
#define NS_POSIX_OS_H

#include "core/ns_slab_alloc.h"
#include "core/ns_stdtypes.h"

#include <time.h>

struct pthread_frame
{
  pthread_t   thread;
  _Atomic int occupied;
};

struct posix_os
{
  /**
   * -1         - unoccupied
   * -2         - pending
   *  n > 0     - occupied
   */
  _Atomic int          fds[MAX_OPEN_FILES];
  _Atomic int          fd_clock;

  struct pthread_frame threads[MAX_THREADS];
  _Atomic int          pthread_clock;

  struct slab_alloc    mutex_alloc;
  struct slab_alloc    cond_alloc;
  struct slab_alloc    timer_alloc;

  struct i_mem         mem;
};

struct posix_os *posix_create (struct i_mem mem, error *e);
void posix_free (struct posix_os *p);

_Atomic int *posix_fd_reserve (struct posix_os *os, error *e);
void posix_fd_free (struct posix_os *os, _Atomic int *fd);

pthread_t *posix_pthread_alloc (struct posix_os *os, error *e);
void posix_pthread_free (struct posix_os *os, pthread_t *thread);

pthread_mutex_t *posix_pthread_mutex_alloc (struct posix_os *os, error *e);
void posix_pthread_mutex_free (struct posix_os *os, pthread_mutex_t *mutex);

pthread_cond_t *posix_pthread_cond_alloc (struct posix_os *os, error *e);
void posix_pthread_cond_free (struct posix_os *os, pthread_cond_t *cond);

struct timespec *posix_timer_alloc (struct posix_os *os, error *e);
void posix_timer_free (struct posix_os *os, struct timespec *timer);

#endif
