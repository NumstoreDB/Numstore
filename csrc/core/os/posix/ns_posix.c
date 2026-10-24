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

// pread/pwrite/ftruncate/fsync/posix_fallocate are POSIX, not ISO C. glibc
// only declares them when a feature-test macro asks for them, and a strict
// -std=c11 (as opposed to -std=gnu11) switches that off - so say so here
// rather than relying on the build happening to pass -std=gnu11. This has to
// come before any system header, hence before ns_platform.h.
//
// Darwin is the exception: it declares them by default, and _POSIX_C_SOURCE
// there would *hide* the non-standard fstore_t / F_PREALLOCATE that
// sys_prealloc needs, so ask for the full Darwin surface instead.
#include "core/os/ns_once.h"
#ifdef __APPLE__
#  ifndef _DARWIN_C_SOURCE
#    define _DARWIN_C_SOURCE 1
#  endif
#else
#  ifndef _POSIX_C_SOURCE
#    define _POSIX_C_SOURCE 200809L
#  endif
#  ifndef _DEFAULT_SOURCE
#    define _DEFAULT_SOURCE 1
#  endif
#endif

#include "core/ns_platform.h"
#include "core/os/posix/ns_posix_os.h"

#if PLATFORM_POSIX

#  include "core/ns_bytes.h"
#  include "core/ns_csx_assert.h"
#  include "core/ns_error.h"
#  include "core/ns_logging.h"
#  include "core/ns_stdtypes.h"
#  include "core/ns_utils.h"
#  include "core/os/ns_os.h"

#  include <errno.h>
#  include <fcntl.h>
#  include <pthread.h>
#  include <stdatomic.h>
#  include <stdbool.h>
#  include <stdio.h>
#  include <string.h>
#  include <sys/stat.h>
#  include <sys/types.h>
#  include <sys/uio.h>
#  include <time.h>
#  include <unistd.h>

static const struct os_vtable default_os_vtable;

///////////// Debug asserts

#  ifndef NDEBUG
static bool
fd_is_open (const int fd)
{
  return fcntl (fd, F_GETFD) != -1 || errno != EBADF;
}
#  endif

DEFINE_DBG_ASSERT (int, fd, fd, {
  ASSERT (fd);
  ASSERT (*fd >= 0); // not free (-1) or pending (-2)
  ASSERT (fd_is_open (*fd));
})

DEFINE_DBG_ASSERT (struct timespec, timer, t, {
  ASSERT (t);
  ASSERT (t->tv_sec >= 0);
  ASSERT (t->tv_nsec >= 0 && t->tv_nsec < 1000000000L);
})

DEFINE_DBG_ASSERT (pthread_mutex_t, mutex, m, { ASSERT (m); })

DEFINE_DBG_ASSERT (pthread_cond_t, cond, c, { ASSERT (c); })

DEFINE_DBG_ASSERT (pthread_t, thread, t, {
  ASSERT (t);
  ASSERT (
      atomic_load (&container_of ((pthread_t *)t, struct pthread_frame, thread)->occupied) == 1
  );
})

/// Reads the fd out of a file self (an fd slot) and checks it
static inline int
fd_of (void *self)
{
  _Atomic int *slot = self;
  ASSERT (slot);
  const int fd = atomic_load_explicit (slot, memory_order_acquire);
  DBG_ASSERT (fd, &fd);
  return fd;
}

///////////// System OS implementations

static err_t
sys_open_file (
    struct posix_os *os,
    i_file          *dest,
    const char      *fname,
    const int        flags,
    const char      *opname,
    error           *e
)
{
  ASSERT (os);
  ASSERT (dest);
  ASSERT (fname);

  _Atomic int *slot = posix_fd_reserve (os, e);
  if (slot == NULL) {
    return error_trace (e);
  }

  const int fd = open (fname, flags, 0644);

  if (unlikely (fd == -1)) {
    const int err = errno;
    posix_fd_free (os, slot);
    return error_causef (e, ERR_IO, "%s %s: %s", opname, fname, strerror (err));
  }

  atomic_store_explicit (slot, fd, memory_order_release);

  dest->table = &default_os_vtable;
  dest->self  = slot;

  return SUCCESS;
}

static err_t
sys_open_file_rw (void *os_self, i_file *dest, const char *fname, error *e)
{
  return sys_open_file (os_self, dest, fname, O_RDWR | O_CREAT, "open_file_rw", e);
}

static err_t
sys_open_file_r (void *os_self, i_file *dest, const char *fname, error *e)
{
  return sys_open_file (os_self, dest, fname, O_RDONLY, "open_file_r", e);
}

static err_t
sys_open_file_w (void *os_self, i_file *dest, const char *fname, error *e)
{
  return sys_open_file (os_self, dest, fname, O_WRONLY | O_CREAT, "open_file_w", e);
}

static err_t
sys_close_file (void *os_self, void *file, error *e)
{
  struct posix_os *os   = os_self;
  _Atomic int     *slot = file;
  ASSERT (os);

  int fd  = fd_of (slot);

  int ret = close (fd);
  int err = errno;

  posix_fd_free (os, slot);

  if (unlikely (ret)) {
    return error_causef (e, ERR_IO, "close: %s", strerror (err));
  }

  return SUCCESS;
}

static err_t
sys_remove_quiet (void *os_self, const char *fname, error *e)
{
  (void)os_self;
  ASSERT (fname);

  if (unlikely (remove (fname) && errno != ENOENT)) {
    return error_causef (e, ERR_IO, "remove %s: %s", fname, strerror (errno));
  }

  return SUCCESS;
}

static err_t
sys_unlink (void *os_self, const char *fname, error *e)
{
  (void)os_self;
  ASSERT (fname);

  if (unlikely (unlink (fname))) {
    return error_causef (e, ERR_IO, "unlink %s: %s", fname, strerror (errno));
  }

  return SUCCESS;
}

static err_t
sys_file_exists (void *os_self, const char *fname, bool *dest, error *e)
{
  (void)os_self;
  ASSERT (fname);
  ASSERT (dest);

  struct stat st;

  if (stat (fname, &st) != 0) {
    if (likely (errno == ENOENT)) {
      *dest = false;
      return SUCCESS;
    }
    return error_causef (e, ERR_IO, "stat %s: %s", fname, strerror (errno));
  }

  *dest = S_ISREG (st.st_mode);
  return SUCCESS;
}

///////////// System file implementations

static err_t
sys_fsync (void *self, error *e)
{
  const int fd = fd_of (self);

  if (unlikely (fsync (fd))) {
    return error_causef (e, ERR_IO, "fsync: %s", strerror (errno));
  }

  return SUCCESS;
}

static i64
sys_file_size (void *self, error *e)
{
  const int   fd = fd_of (self);

  struct stat st;

  if (unlikely (fstat (fd, &st) == -1)) {
    error_causef (e, ERR_IO, "fstat: %s", strerror (errno));
    return error_trace (e);
  }

  return (i64)st.st_size;
}

static i64
sys_pread_all (void *self, void *dest, const u64 n, const u64 offset, error *e)
{
  const int fd = fd_of (self);

  ASSERT (dest);
  ASSERT (n > 0);

  u8 *_dest = (u8 *)dest;
  u64 nread = 0;

  while (nread < n) {
    const ssize_t _nread = pread (fd, _dest + nread, n - nread, (off_t)(offset + nread));

    if (unlikely (_nread < 0)) {
      if (errno == EINTR) {
        continue;
      }
      return error_causef (e, ERR_IO, "pread: %s", strerror (errno));
    }

    if (_nread == 0) {
      return (i64)nread; // EOF
    }

    nread += (u64)_nread;
  }

  ASSERT (nread == n);
  return (i64)nread;
}

static err_t
sys_pwrite_all (void *self, const void *src, const u64 n, const u64 offset, error *e)
{
  const int fd = fd_of (self);

  ASSERT (src);
  ASSERT (n > 0);

  const u8 *_src     = (const u8 *)src;
  u64       nwritten = 0;

  while (nwritten < n) {
    const ssize_t _nw = pwrite (fd, _src + nwritten, n - nwritten, (off_t)(offset + nwritten));

    if (unlikely (_nw < 0)) {
      if (errno == EINTR) {
        continue;
      }
      return error_causef (e, ERR_IO, "pwrite: %s", strerror (errno));
    }

    nwritten += (u64)_nw;
  }

  ASSERT (nwritten == n);
  return SUCCESS;
}

static err_t
sys_writev_all (void *self, struct bytes *iov, const int iovcnt, error *e)
{
  const int fd = fd_of (self);

  ASSERT (iov);
  ASSERT (iovcnt > 0 && iovcnt <= 2);

  // Work on a copy so the caller's buffers aren't advanced under them
  struct bytes local[2];
  u64          total = 0;
  for (int i = 0; i < iovcnt; i++) {
    local[i] = iov[i];
    total += iov[i].len;
  }

  ASSERT (total > 0);

  u64           nwritten  = 0;
  struct bytes *cur       = local;
  int           remaining = iovcnt;

  while (nwritten < total) {
    ASSERT (remaining > 0);

    struct iovec sys_iov[2];
    for (int i = 0; i < remaining; i++) {
      sys_iov[i].iov_base = cur[i].head;
      sys_iov[i].iov_len  = cur[i].len;
    }

    const ssize_t ret = writev (fd, sys_iov, remaining);

    if (unlikely (ret < 0)) {
      if (errno == EINTR) {
        continue;
      }
      return error_causef (e, ERR_IO, "writev: %s", strerror (errno));
    }

    nwritten += (u64)ret;

    u64 skip = (u64)ret;
    while (remaining > 0 && skip >= cur->len) {
      skip -= cur->len;
      cur++;
      remaining--;
    }
    if (skip > 0) {
      ASSERT (remaining > 0);
      cur->head += skip;
      cur->len -= skip;
    }
  }

  ASSERT (nwritten == total);
  return SUCCESS;
}

static i64
sys_read_all (void *self, void *dest, const u64 nbytes, error *e)
{
  const int fd = fd_of (self);

  ASSERT (dest);
  ASSERT (nbytes > 0);

  u8 *_dest = (u8 *)dest;
  u64 nread = 0;

  while (nread < nbytes) {
    const ssize_t _nread = read (fd, _dest + nread, nbytes - nread);

    if (unlikely (_nread < 0)) {
      if (errno == EINTR) {
        continue;
      }
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return (i64)nread; // non-blocking: return what we have, possibly 0
      }
      return error_causef (e, ERR_IO, "read: %s", strerror (errno));
    }

    if (_nread == 0) {
      return (i64)nread; // EOF
    }

    nread += (u64)_nread;
  }

  ASSERT (nread == nbytes);
  return (i64)nread;
}

static err_t
sys_write_all (void *self, const void *src, const u64 nbytes, error *e)
{
  const int fd = fd_of (self);

  ASSERT (src);
  ASSERT (nbytes > 0);

  const u8 *_src     = (const u8 *)src;
  u64       nwritten = 0;

  while (nwritten < nbytes) {
    const ssize_t _nw = write (fd, _src + nwritten, nbytes - nwritten);

    if (unlikely (_nw < 0)) {
      if (errno == EINTR) {
        continue;
      }
      return error_causef (e, ERR_IO, "write: %s", strerror (errno));
    }

    nwritten += (u64)_nw;
  }

  ASSERT (nwritten == nbytes);
  return SUCCESS;
}

static err_t
sys_truncate (void *self, const u64 bytes, error *e)
{
  const int fd = fd_of (self);

  if (unlikely (ftruncate (fd, (off_t)bytes) == -1)) {
    return error_causef (e, ERR_IO, "ftruncate: %s", strerror (errno));
  }

  return SUCCESS;
}

/// Grows the file to at least [bytes] and never shrinks it, like
/// posix_fallocate.
static err_t
sys_prealloc (void *self, const u64 bytes, error *e)
{
  const int fd = fd_of (self);

#  ifdef __APPLE__
  // macOS has no posix_fallocate. F_PREALLOCATE reserves blocks but leaves
  // the size alone, so the size has to be set separately - and because that
  // step is an ftruncate, which shrinks as readily as it grows, a file that
  // is already big enough has to be left alone entirely.
  struct stat st;
  if (unlikely (fstat (fd, &st) == -1)) {
    return error_causef (e, ERR_IO, "prealloc (fstat): %s", strerror (errno));
  }

  if ((u64)st.st_size >= bytes) {
    return SUCCESS;
  }

  fstore_t store = {
      .fst_flags   = F_ALLOCATECONTIG,
      .fst_posmode = F_PEOFPOSMODE,
      .fst_offset  = 0,
      .fst_length  = (off_t)(bytes - (u64)st.st_size),
  };

  // Best effort: a contiguous reservation is only a fragmentation hint, and
  // some filesystems support neither flag. The ftruncate below is what
  // actually has to happen, so a failure here is not fatal.
  if (fcntl (fd, F_PREALLOCATE, &store) == -1) {
    store.fst_flags = F_ALLOCATEALL;
    (void)fcntl (fd, F_PREALLOCATE, &store);
  }

  if (unlikely (ftruncate (fd, (off_t)bytes) == -1)) {
    return error_causef (e, ERR_IO, "prealloc (ftruncate): %s", strerror (errno));
  }
#  else
  const int ret = posix_fallocate (fd, 0, (off_t)bytes);

  if (unlikely (ret != 0)) {
    return error_causef (e, ERR_IO, "posix_fallocate: %s", strerror (ret));
  }
#  endif

  return SUCCESS;
}

static i64
sys_seek (void *self, const u64 offset, const seek_t whence, error *e)
{
  const int fd = fd_of (self);

  int       w;
  switch (whence) {
    case I_SEEK_SET: {
      w = SEEK_SET;
      break;
    }
    case I_SEEK_CUR: {
      w = SEEK_CUR;
      break;
    }
    case I_SEEK_END: {
      w = SEEK_END;
      break;
    }
    default: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }

  const off_t ret = lseek (fd, (off_t)offset, w);

  if (unlikely (ret == (off_t)-1)) {
    error_causef (e, ERR_IO, "lseek: %s", strerror (errno));
    return error_trace (e);
  }

  return (i64)ret;
}

/////////////////////////////////////// Threads
//
// Note: the pthread_* functions return an error code and do NOT set errno.

static err_t
sys_thread_create (void *os_self, i_thread *dest, void *(*func) (void *), void *context, error *e)
{
  ASSERT (dest);
  ASSERT (os_self);
  ASSERT (func);

  struct posix_os *os     = os_self;
  pthread_t       *thread = posix_pthread_alloc (os, e);
  if (thread == NULL) {
    return error_trace (e);
  }

  int r;

#  ifndef NDEBUG
  pthread_attr_t attr;
  int            r1 = pthread_attr_init (&attr);
  ASSERT (!r1);

  // Examples:
  // pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  // pthread_attr_setstacksize(&attr, 1 << 20);
  // pthread_attr_setguardsize(&attr, 4096);

  r  = pthread_create (thread, &attr, func, context);

  r1 = pthread_attr_destroy (&attr);
  ASSERT (!r1);
  (void)r1;
#  else
  r = pthread_create (thread, NULL, func, context);
#  endif

  if (r) {
    posix_pthread_free (os, thread);

    switch (r) {
      case EAGAIN: {
        return error_causef (e, ERR_IO, "pthread_create: %s", strerror (r));
      }
      case EINVAL: {
        i_log_error ("pthread_create: invalid attributes: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      case EPERM: {
        i_log_error ("pthread_create: insufficient permissions: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      default: {
        i_log_error ("pthread_create: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
    }
  }

  dest->table = &default_os_vtable;
  dest->self  = thread;

  return SUCCESS;
}

static void
sys_thread_join (void *os_self, void *t)
{
  struct posix_os *os     = os_self;
  pthread_t       *thread = t;
  ASSERT (os);
  DBG_ASSERT (thread, thread);

  const int r = pthread_join (*thread, NULL);

  if (r != 0) {
    switch (r) {
      case EDEADLK: {
        i_log_error ("pthread_join: deadlock: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      case EINVAL: {
        i_log_error ("pthread_join: not joinable: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      case ESRCH: {
        i_log_error ("pthread_join: no such thread: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      default: {
        i_log_error ("pthread_join: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
    }
  }

  posix_pthread_free (os, thread);
}

///////////// System Mutex implementations

static err_t
sys_mutex_create (void *os_self, i_mutex *dest, error *e)
{
  ASSERT (dest);
  ASSERT (os_self);

  struct posix_os *os    = os_self;
  pthread_mutex_t *mutex = posix_pthread_mutex_alloc (os, e);
  if (mutex == NULL) {
    return error_trace (e);
  }

  int r;

#  ifndef NDEBUG
  pthread_mutexattr_t attr;

  // I just don't want to handle errors for debug code
  int                 r1 = pthread_mutexattr_init (&attr);
  ASSERT (!r1);

  r1 = pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_ERRORCHECK);
  ASSERT (!r1);

  r  = pthread_mutex_init (mutex, &attr);

  r1 = pthread_mutexattr_destroy (&attr);
  ASSERT (!r1);
  (void)r1;
#  else
  r = pthread_mutex_init (mutex, NULL);
#  endif

  if (r) {
    posix_pthread_mutex_free (os, mutex);

    switch (r) {
      case EAGAIN: {
        return error_causef (e, ERR_IO, "mutex_init: %s", strerror (r));
      }
      case ENOMEM: {
        return error_causef (e, ERR_NOMEM, "mutex_init: %s", strerror (r));
      }
      case EPERM: {
        i_log_error ("mutex_init: insufficient permissions: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      default: {
        i_log_error ("mutex_init: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
    }
  }

  dest->table = &default_os_vtable;
  dest->self  = mutex;

  return SUCCESS;
}

static void
sys_mutex_free (void *os_self, void *mutex)
{
  struct posix_os *os = os_self;
  pthread_mutex_t *m  = mutex;
  ASSERT (os);
  DBG_ASSERT (mutex, m);

  const int r = pthread_mutex_destroy (m);
  if (r) {
    switch (r) {
      case EBUSY: {
        i_log_error ("mutex_destroy: still locked: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      case EINVAL: {
        i_log_error ("mutex_destroy: invalid: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      default: {
        i_log_error ("mutex_destroy: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
    }
  }

  posix_pthread_mutex_free (os, m);
}

static void
sys_mutex_lock (void *mutex)
{
  pthread_mutex_t *m = mutex;
  DBG_ASSERT (mutex, m);

  const int r = pthread_mutex_lock (m);
  if (r) {
    switch (r) {
      case EINVAL: {
        i_log_error ("mutex_lock: invalid: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      case EAGAIN: {
        i_log_error ("mutex_lock: recursive lock: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      case EDEADLK: {
        i_log_error ("mutex_lock: deadlock: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      default: {
        i_log_error ("mutex_lock: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
    }
  }
}

static void
sys_mutex_unlock (void *mutex)
{
  pthread_mutex_t *m = mutex;
  DBG_ASSERT (mutex, m);

  const int r = pthread_mutex_unlock (m);
  if (r) {
    switch (r) {
      case EINVAL: {
        i_log_error ("mutex_unlock: invalid: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      case EAGAIN: {
        i_log_error ("mutex_unlock: recursive lock: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      case EPERM: {
        i_log_error ("mutex_unlock: not owner: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      default: {
        i_log_error ("mutex_unlock: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
    }
  }
}

///////////// System Condition implementations

static err_t
sys_cond_create (void *os_self, i_cond *dest, error *e)
{
  ASSERT (dest);
  ASSERT (os_self);

  struct posix_os *os   = os_self;
  pthread_cond_t  *cond = posix_pthread_cond_alloc (os, e);
  if (cond == NULL) {
    return error_trace (e);
  }

  int r;

#  ifndef NDEBUG
  pthread_condattr_t attr;

  // I just don't want to handle errors for debug code
  int                r1 = pthread_condattr_init (&attr);
  ASSERT (r1 == 0);

  r  = pthread_cond_init (cond, &attr);

  r1 = pthread_condattr_destroy (&attr);
  ASSERT (r1 == 0);
  (void)r1;
#  else
  r = pthread_cond_init (cond, NULL);
#  endif

  if (r) {
    posix_pthread_cond_free (os, cond);

    switch (r) {
      case EAGAIN: {
        return error_causef (e, ERR_IO, "pthread_cond_init: %s", strerror (r));
      }
      case ENOMEM: {
        return error_causef (e, ERR_NOMEM, "pthread_cond_init: %s", strerror (r));
      }
      case EBUSY: {
        i_log_error ("cond_create: cond already initialized: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      case EINVAL: {
        i_log_error ("cond_create: invalid attributes or cond: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      default: {
        i_log_error ("cond_create: unknown error: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
    }
  }

  dest->table = &default_os_vtable;
  dest->self  = cond;

  return SUCCESS;
}

static void
sys_cond_free (void *os_self, void *cond)
{
  struct posix_os *os = os_self;
  pthread_cond_t  *c  = cond;
  ASSERT (os);
  DBG_ASSERT (cond, c);

  const int r = pthread_cond_destroy (c);
  if (r) {
    switch (r) {
      case EBUSY: {
        i_log_error ("cond_free: cond has active waiters: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      case EINVAL: {
        i_log_error ("cond_free: invalid or uninitialized cond: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      default: {
        i_log_error ("cond_free: unknown error: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
    }
  }

  posix_pthread_cond_free (os, c);
}

static void
sys_cond_wait (void *cond, void *mutex)
{
  pthread_cond_t  *c = cond;
  pthread_mutex_t *m = mutex;
  DBG_ASSERT (cond, c);
  DBG_ASSERT (mutex, m);

  const int r = pthread_cond_wait (c, m);
  if (r) {
    switch (r) {
      case EINVAL: {
        i_log_error ("cond_wait: invalid cond or mutex: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      case EPERM: {
        i_log_error ("cond_wait: mutex not owned by thread: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      default: {
        i_log_error ("cond_wait: unknown error: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
    }
  }
}

static void
sys_cond_timed_wait (void *cond, void *mutex, u64 msec)
{
  pthread_cond_t  *c = cond;
  pthread_mutex_t *m = mutex;
  DBG_ASSERT (cond, c);
  DBG_ASSERT (mutex, m);

  struct timespec ts;
  const int       cr = clock_gettime (CLOCK_REALTIME, &ts);
  ASSERT (cr == 0);
  (void)cr;

  ts.tv_sec += (time_t)(msec / 1000);
  ts.tv_nsec += (long)((msec % 1000) * 1000000LL);
  if (ts.tv_nsec >= 1000000000L) {
    ts.tv_sec += 1;
    ts.tv_nsec -= 1000000000L;
  }

  const int r = pthread_cond_timedwait (c, m, &ts);
  if (r && r != ETIMEDOUT) {
    switch (r) {
      case EINVAL: {
        i_log_error ("cond_timed_wait: invalid cond, mutex, or abstime: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      case EPERM: {
        i_log_error ("cond_timed_wait: mutex not owned by thread: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      default: {
        i_log_error ("cond_timed_wait: unknown error: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
    }
  }
}

static void
sys_cond_signal (void *cond)
{
  pthread_cond_t *c = cond;
  DBG_ASSERT (cond, c);

  const int r = pthread_cond_signal (c);
  if (r) {
    switch (r) {
      case EINVAL: {
        i_log_error ("cond_signal: invalid or uninitialized cond: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      default: {
        i_log_error ("cond_signal: unknown error: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
    }
  }
}

static void
sys_cond_broadcast (void *cond)
{
  pthread_cond_t *c = cond;
  DBG_ASSERT (cond, c);

  const int r = pthread_cond_broadcast (c);
  if (r) {
    switch (r) {
      case EINVAL: {
        i_log_error ("cond_broadcast: invalid or uninitialized cond: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
      default: {
        i_log_error ("cond_broadcast: unknown error: %s\n", strerror (r));
        UNREACHABLE (); // LCOV_EXCL_LINE
      }
    }
  }
}

///////////// System time implementations

static err_t
sys_timer_create (void *os_self, i_timer *dest, error *e)
{
  ASSERT (os_self);
  ASSERT (dest);

  struct posix_os *os   = os_self;
  struct timespec *spec = posix_timer_alloc (os, e);
  if (spec == NULL) {
    return error_trace (e);
  }

  if (clock_gettime (CLOCK_MONOTONIC, spec) != 0) {
    const int err = errno;
    posix_timer_free (os, spec);
    return error_causef (e, ERR_IO, "clock_gettime: %s", strerror (err));
  }

  DBG_ASSERT (timer, spec);

  dest->table = &default_os_vtable;
  dest->self  = spec;

  return SUCCESS;
}

static void
sys_timer_free (void *os_self, void *timer)
{
  struct posix_os *os = os_self;
  struct timespec *t  = timer;
  ASSERT (os);
  DBG_ASSERT (timer, t);

  posix_timer_free (os, t);
}

static u64
sys_timer_now_ns (void *timer)
{
  struct timespec *self = timer;
  DBG_ASSERT (timer, self);

  struct timespec now;
  const int       r = clock_gettime (CLOCK_MONOTONIC, &now);
  ASSERT (r == 0);
  (void)r;

  const i64 sec_diff  = (i64)now.tv_sec - (i64)self->tv_sec;
  const i64 nsec_diff = (i64)now.tv_nsec - (i64)self->tv_nsec;
  const i64 total     = (sec_diff * 1000000000LL) + nsec_diff;
  ASSERT (total >= 0); // monotonic clock

  return (u64)total;
}

////////////////////////////
/// Sleep

void
i_sleep_us (const u64 us)
{
  struct timespec ts = {
      .tv_sec  = (time_t)(us / 1000000ULL),
      .tv_nsec = (long)((us % 1000000ULL) * 1000ULL),
  };

  // nanosleep returns EINTR with the remaining time in [rem] - finish the nap
  struct timespec rem;
  while (nanosleep (&ts, &rem) == -1 && errno == EINTR) {
    ts = rem;
  }
}

////////////////////////////
/// Run once

void
i_once_run (i_once *once, void (*fn) (void))
{
  ASSERT (once);
  ASSERT (fn);

  const int r = pthread_once (once, fn);
  ASSERT (r == 0);
  (void)r;
}

static const struct os_vtable default_os_vtable = {
    // Opening and closing files
    .open_file_rw    = sys_open_file_rw,
    .open_file_r     = sys_open_file_r,
    .open_file_w     = sys_open_file_w,
    .close_file      = sys_close_file,

    // File system
    .remove_quiet    = sys_remove_quiet,
    .unlink          = sys_unlink,
    .file_exists     = sys_file_exists,

    // File operations
    .fsync           = sys_fsync,
    .file_size       = sys_file_size,
    .read_all        = sys_read_all,
    .pread_all       = sys_pread_all,
    .write_all       = sys_write_all,
    .pwrite_all      = sys_pwrite_all,
    .writev_all      = sys_writev_all,
    .truncate        = sys_truncate,
    .prealloc        = sys_prealloc,
    .seek            = sys_seek,

    // Threads
    .thread_create   = sys_thread_create,
    .thread_join     = sys_thread_join,

    // Mutexes
    .mutex_create    = sys_mutex_create,
    .mutex_free      = sys_mutex_free,
    .mutex_lock      = sys_mutex_lock,
    .mutex_unlock    = sys_mutex_unlock,

    // Condition variables
    .cond_create     = sys_cond_create,
    .cond_free       = sys_cond_free,
    .cond_wait       = sys_cond_wait,
    .cond_timed_wait = sys_cond_timed_wait,
    .cond_signal     = sys_cond_signal,
    .cond_broadcast  = sys_cond_broadcast,

    // Timer
    .timer_create    = sys_timer_create,
    .timer_free      = sys_timer_free,
    .timer_now_ns    = sys_timer_now_ns,
};

err_t
system_os_create (struct i_mem mem, struct i_os *dest, error *e)
{
  ASSERT (dest);

  struct posix_os *os = posix_create (mem, e);
  if (os == NULL) {
    return error_trace (e);
  }

  dest->self  = os;
  dest->table = &default_os_vtable;
  return SUCCESS;
}

void
system_os_free (struct i_os os)
{
  ASSERT (os.table == &default_os_vtable);
  posix_free (os.self);
}

#  ifndef NDEBUG
#    include "core/ns_testing.h"

/******************************************************************************
 * SECTION: Posix OS (tests)
 * ----------------------------------------------------------------------------
 * Every sys_* function is static and reachable only through the vtable, so
 * these drive them through ctx->os, which is a system OS built by
 * system_os_create. One test per vtable group; every file is closed and every
 * thread joined, since test_ctx_free asserts on leaks of either.
 *
 * read_all / pread_all / write_all / pwrite_all all ASSERT (n > 0), so no case
 * here asks for a zero length transfer.
 ******************************************************************************/

#    define SYS_T_FILE  "ns_posix_sys_test.bin"
#    define SYS_T_FILE2 "ns_posix_sys_test2.bin"

TEST (sys_file_open_write_read_close)
{
  const u8 payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};

  // Start from a clean slate; remove_quiet tolerates the file not being there
  test_assert_int_equal (ctx->os.table->remove_quiet (ctx->os.self, SYS_T_FILE, &ctx->e), SUCCESS);

  TEST_CASE ("open_file_w creates the file and write_all fills it")
  {
    i_file f;
    test_assert_int_equal (
        ctx->os.table->open_file_w (ctx->os.self, &f, SYS_T_FILE, &ctx->e),
        SUCCESS
    );
    test_assert_int_equal (f.table->write_all (f.self, payload, sizeof payload, &ctx->e), SUCCESS);
    test_assert_int_equal (ctx->os.table->close_file (ctx->os.self, f.self, &ctx->e), SUCCESS);
  }

  TEST_CASE ("file_exists sees a regular file")
  {
    bool exists = false;
    test_assert_int_equal (
        ctx->os.table->file_exists (ctx->os.self, SYS_T_FILE, &exists, &ctx->e),
        SUCCESS
    );
    test_assert (exists);
  }

  TEST_CASE ("open_file_r reads back exactly what was written")
  {
    i_file f;
    test_assert_int_equal (
        ctx->os.table->open_file_r (ctx->os.self, &f, SYS_T_FILE, &ctx->e),
        SUCCESS
    );

    u8 got[8] = {0};
    test_assert_int_equal (f.table->read_all (f.self, got, sizeof got, &ctx->e), 8);
    test_assert_memequal (got, payload, sizeof payload);

    test_assert_int_equal (ctx->os.table->close_file (ctx->os.self, f.self, &ctx->e), SUCCESS);
  }

  TEST_CASE ("file_size reports the written length")
  {
    i_file f;
    test_assert_int_equal (
        ctx->os.table->open_file_r (ctx->os.self, &f, SYS_T_FILE, &ctx->e),
        SUCCESS
    );
    test_assert_int_equal (f.table->file_size (f.self, &ctx->e), 8);
    test_assert_int_equal (ctx->os.table->close_file (ctx->os.self, f.self, &ctx->e), SUCCESS);
  }

  TEST_CASE ("unlink removes it and file_exists says so")
  {
    test_assert_int_equal (ctx->os.table->unlink (ctx->os.self, SYS_T_FILE, &ctx->e), SUCCESS);

    bool exists = true;
    test_assert_int_equal (
        ctx->os.table->file_exists (ctx->os.self, SYS_T_FILE, &exists, &ctx->e),
        SUCCESS
    );
    test_assert (!exists);
  }
}

TEST (sys_file_offsets_and_sizing)
{
  test_assert_int_equal (ctx->os.table->remove_quiet (ctx->os.self, SYS_T_FILE, &ctx->e), SUCCESS);

  i_file f;
  test_assert_int_equal (
      ctx->os.table->open_file_rw (ctx->os.self, &f, SYS_T_FILE, &ctx->e),
      SUCCESS
  );

  const u8 payload[16] = {0};
  test_assert_int_equal (f.table->write_all (f.self, payload, sizeof payload, &ctx->e), SUCCESS);

  TEST_CASE ("pwrite_all then pread_all round trips at an offset")
  {
    const u8 marker[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    test_assert_int_equal (f.table->pwrite_all (f.self, marker, 4, 8, &ctx->e), SUCCESS);

    u8 got[4] = {0};
    test_assert_int_equal (f.table->pread_all (f.self, got, 4, 8, &ctx->e), 4);
    test_assert_memequal (got, marker, 4);
  }

  TEST_CASE ("pread_all past the end returns a short count rather than failing")
  {
    u8 got[8] = {0};
    // Only 16 bytes exist, so asking at offset 12 for 8 can only yield 4
    test_assert_int_equal (f.table->pread_all (f.self, got, 8, 12, &ctx->e), 4);
  }

  TEST_CASE ("seek reports the resulting absolute offset for each whence")
  {
    test_assert_int_equal (f.table->seek (f.self, 4, I_SEEK_SET, &ctx->e), 4);
    test_assert_int_equal (f.table->seek (f.self, 2, I_SEEK_CUR, &ctx->e), 6);
    test_assert_int_equal (f.table->seek (f.self, 0, I_SEEK_END, &ctx->e), 16);
  }

  TEST_CASE ("truncate shrinks the file")
  {
    test_assert_int_equal (f.table->truncate (f.self, 10, &ctx->e), SUCCESS);
    test_assert_int_equal (f.table->file_size (f.self, &ctx->e), 10);
  }

  TEST_CASE ("prealloc grows the file and never shrinks it")
  {
    test_assert_int_equal (f.table->prealloc (f.self, 64, &ctx->e), SUCCESS);
    test_assert (f.table->file_size (f.self, &ctx->e) >= 64);

    // Already big enough, so this must leave the size alone
    const i64 before = f.table->file_size (f.self, &ctx->e);
    test_assert_int_equal (f.table->prealloc (f.self, 8, &ctx->e), SUCCESS);
    test_assert_int_equal (f.table->file_size (f.self, &ctx->e), (i32)before);
  }

  TEST_CASE ("writev_all writes every buffer in order")
  {
    test_assert_int_equal (f.table->truncate (f.self, 0, &ctx->e), SUCCESS);
    test_assert_int_equal (f.table->seek (f.self, 0, I_SEEK_SET, &ctx->e), 0);

    u8           a[2]    = {0xA0, 0xA1};
    u8           b[3]    = {0xB0, 0xB1, 0xB2};
    struct bytes arrs[2] = {
        {.head = a, .len = sizeof a},
        {.head = b, .len = sizeof b},
    };

    test_assert_int_equal (f.table->writev_all (f.self, arrs, 2, &ctx->e), SUCCESS);

    u8 got[5] = {0};
    test_assert_int_equal (f.table->pread_all (f.self, got, 5, 0, &ctx->e), 5);

    const u8 expected[5] = {0xA0, 0xA1, 0xB0, 0xB1, 0xB2};
    test_assert_memequal (got, expected, 5);
  }

  TEST_CASE ("fsync succeeds on an open file")
  {
    test_assert_int_equal (f.table->fsync (f.self, &ctx->e), SUCCESS);
  }

  test_assert_int_equal (ctx->os.table->close_file (ctx->os.self, f.self, &ctx->e), SUCCESS);
  test_assert_int_equal (ctx->os.table->unlink (ctx->os.self, SYS_T_FILE, &ctx->e), SUCCESS);
}

TEST (sys_file_error_paths)
{
  TEST_CASE ("Opening a missing file for read fails")
  {
    i_file f;
    test_assert (
        ctx->os.table->open_file_r (ctx->os.self, &f, "ns_posix_definitely_absent.bin", &ctx->e)
        != SUCCESS
    );
    error_reset (&ctx->e);
  }

  TEST_CASE ("unlink of a missing file fails")
  {
    test_assert (
        ctx->os.table->unlink (ctx->os.self, "ns_posix_definitely_absent.bin", &ctx->e) != SUCCESS
    );
    error_reset (&ctx->e);
  }

  TEST_CASE ("remove_quiet of a missing file succeeds, which is the point of it")
  {
    test_assert_int_equal (
        ctx->os.table->remove_quiet (ctx->os.self, "ns_posix_definitely_absent.bin", &ctx->e),
        SUCCESS
    );
  }

  TEST_CASE ("file_exists reports false for a missing file without erroring")
  {
    bool exists = true;
    test_assert_int_equal (
        ctx->os.table
            ->file_exists (ctx->os.self, "ns_posix_definitely_absent.bin", &exists, &ctx->e),
        SUCCESS
    );
    test_assert (!exists);
  }
}

struct sys_thr_data
{
  struct i_os os;
  i_mutex     m;
  i_cond      c;
  u32         iters;
  int         counter;
  bool        ready;
};

static void *
sys_counter_thread (void *_d)
{
  struct sys_thr_data *d = _d;

  for (u32 i = 0; i < d->iters; ++i) {
    d->os.table->mutex_lock (d->m.self);
    d->counter += 1;
    d->os.table->mutex_unlock (d->m.self);
  }

  return NULL;
}

static void *
sys_signal_thread (void *_d)
{
  struct sys_thr_data *d = _d;

  d->os.table->mutex_lock (d->m.self);
  d->ready = true;
  d->os.table->cond_signal (d->c.self);
  d->os.table->mutex_unlock (d->m.self);

  return NULL;
}

static void *
sys_broadcast_thread (void *_d)
{
  struct sys_thr_data *d = _d;

  d->os.table->mutex_lock (d->m.self);
  d->ready = true;
  d->os.table->cond_broadcast (d->c.self);
  d->os.table->mutex_unlock (d->m.self);

  return NULL;
}

TEST (sys_threads_and_mutexes)
{
  struct sys_thr_data d = {.os = ctx->os, .iters = 500, .counter = 0};

  test_assert_int_equal (ctx->os.table->mutex_create (ctx->os.self, &d.m, &ctx->e), SUCCESS);

  // The total is only exact if mutex_lock really excludes
  i_thread threads[8];
  for (u32 i = 0; i < arrlen (threads); ++i) {
    test_assert_int_equal (
        ctx->os.table->thread_create (ctx->os.self, &threads[i], sys_counter_thread, &d, &ctx->e),
        SUCCESS
    );
  }
  for (u32 i = 0; i < arrlen (threads); ++i) {
    ctx->os.table->thread_join (ctx->os.self, threads[i].self);
  }

  test_assert_int_equal (d.counter, (int)(arrlen (threads) * 500));

  ctx->os.table->mutex_free (ctx->os.self, d.m.self);
}

TEST (sys_condition_variables)
{
  TEST_CASE ("cond_wait returns once another thread signals")
  {
    struct sys_thr_data d = {.os = ctx->os, .ready = false};
    test_assert_int_equal (ctx->os.table->mutex_create (ctx->os.self, &d.m, &ctx->e), SUCCESS);
    test_assert_int_equal (ctx->os.table->cond_create (ctx->os.self, &d.c, &ctx->e), SUCCESS);

    i_thread t;
    test_assert_int_equal (
        ctx->os.table->thread_create (ctx->os.self, &t, sys_signal_thread, &d, &ctx->e),
        SUCCESS
    );

    ctx->os.table->mutex_lock (d.m.self);
    while (!d.ready) {
      ctx->os.table->cond_wait (d.c.self, d.m.self);
    }
    ctx->os.table->mutex_unlock (d.m.self);

    ctx->os.table->thread_join (ctx->os.self, t.self);
    test_assert (d.ready);

    ctx->os.table->cond_free (ctx->os.self, d.c.self);
    ctx->os.table->mutex_free (ctx->os.self, d.m.self);
  }

  TEST_CASE ("cond_broadcast also wakes a waiter")
  {
    struct sys_thr_data d = {.os = ctx->os, .ready = false};
    test_assert_int_equal (ctx->os.table->mutex_create (ctx->os.self, &d.m, &ctx->e), SUCCESS);
    test_assert_int_equal (ctx->os.table->cond_create (ctx->os.self, &d.c, &ctx->e), SUCCESS);

    i_thread t;
    test_assert_int_equal (
        ctx->os.table->thread_create (ctx->os.self, &t, sys_broadcast_thread, &d, &ctx->e),
        SUCCESS
    );

    ctx->os.table->mutex_lock (d.m.self);
    while (!d.ready) {
      ctx->os.table->cond_wait (d.c.self, d.m.self);
    }
    ctx->os.table->mutex_unlock (d.m.self);

    ctx->os.table->thread_join (ctx->os.self, t.self);
    test_assert (d.ready);

    ctx->os.table->cond_free (ctx->os.self, d.c.self);
    ctx->os.table->mutex_free (ctx->os.self, d.m.self);
  }

  TEST_CASE ("cond_timed_wait gives up when nobody signals")
  {
    struct sys_thr_data d = {.os = ctx->os, .ready = false};
    test_assert_int_equal (ctx->os.table->mutex_create (ctx->os.self, &d.m, &ctx->e), SUCCESS);
    test_assert_int_equal (ctx->os.table->cond_create (ctx->os.self, &d.c, &ctx->e), SUCCESS);

    // Nothing will ever signal this, so reaching the next line is the test
    ctx->os.table->mutex_lock (d.m.self);
    ctx->os.table->cond_timed_wait (d.c.self, d.m.self, 10);
    ctx->os.table->mutex_unlock (d.m.self);

    test_assert (!d.ready);

    ctx->os.table->cond_free (ctx->os.self, d.c.self);
    ctx->os.table->mutex_free (ctx->os.self, d.m.self);
  }
}

TEST (sys_timer)
{
  i_timer t;
  test_assert_int_equal (ctx->os.table->timer_create (ctx->os.self, &t, &ctx->e), SUCCESS);

  // now_ns is elapsed time since timer_create, so the first reading can
  // legitimately be 0 - only the ordering between readings is guaranteed
  const u64 first = ctx->os.table->timer_now_ns (t.self);

  TEST_CASE ("The clock never goes backwards")
  {
    u64 last = first;
    for (u32 i = 0; i < 1000; ++i) {
      const u64 now = ctx->os.table->timer_now_ns (t.self);
      test_assert (now >= last);
      last = now;
    }
  }

  TEST_CASE ("The clock advances across a sleep")
  {
    const u64 before = ctx->os.table->timer_now_ns (t.self);
    i_sleep_ms (2);
    const u64 after = ctx->os.table->timer_now_ns (t.self);

    test_assert (after > before);

    // At least most of the 2ms nap should show up
    test_assert (after - before >= 1000000u);
  }

  ctx->os.table->timer_free (ctx->os.self, t.self);
}

TEST (sys_os_create_and_free)
{
  // A second, independent system OS alongside the one in ctx
  struct i_os os;
  test_assert_int_equal (system_os_create (ctx->mem, &os, &ctx->e), SUCCESS);
  test_assert (os.self != NULL);
  test_assert (os.table == &default_os_vtable);

  // Usable in its own right
  test_assert_int_equal (os.table->remove_quiet (os.self, SYS_T_FILE2, &ctx->e), SUCCESS);

  i_file f;
  test_assert_int_equal (os.table->open_file_rw (os.self, &f, SYS_T_FILE2, &ctx->e), SUCCESS);

  const u8 byte = 0x5A;
  test_assert_int_equal (f.table->write_all (f.self, &byte, 1, &ctx->e), SUCCESS);
  test_assert_int_equal (os.table->close_file (os.self, f.self, &ctx->e), SUCCESS);
  test_assert_int_equal (os.table->unlink (os.self, SYS_T_FILE2, &ctx->e), SUCCESS);

  system_os_free (os);
}
#  endif // NDEBUG

#endif // PLATFORM_POSIX
