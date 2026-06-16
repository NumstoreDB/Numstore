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

/**
 * @file
 * @brief Operating System Interface - for multi platform support
 *
 * A list of interfaces for operating system agnostic code
 */

#ifndef OS_H
#define OS_H

#include "error.h"    // error
#include "stdtypes.h" // u32 ...etc

/******************************************************************************
 * SECTION: Memory
 * ----------------------------------------------------------------------------
 * @brief Allocation, and reallocation
 *
 * Interfaces for allocation and reallocation with a vtable to allow tests
 * and consumers to change default malloc behavior
 ******************************************************************************/

typedef struct i_vmem i_vmem;

struct i_vmem
{
  void *(*i_malloc) (i_vmem *v, u32 nelem, u32 size, error *e);
  void *(*i_calloc) (i_vmem *v, u32 nelem, u32 size, error *e);
  void *(*i_realloc_right) (
      i_vmem *v,
      void   *ptr,
      u32     old_nelem,
      u32     nelem,
      u32     size,
      error  *e
  );
  void *(*i_realloc_left) (
      i_vmem *v,
      void   *ptr,
      u32     old_nelem,
      u32     nelem,
      u32     size,
      error  *e
  );
  void *(*i_crealloc_right) (
      i_vmem *v,
      void   *ptr,
      u32     old_nelem,
      u32     nelem,
      u32     size,
      error  *e
  );
  void *(*i_crealloc_left) (
      i_vmem *v,
      void   *ptr,
      u32     old_nelem,
      u32     nelem,
      u32     size,
      error  *e
  );
  void (*i_free) (i_vmem *v, void *ptr);
};

extern struct i_vmem default_vmem;

HEADER_FUNC void *
i_malloc (u32 nelem, u32 size, error *e)
{
  return default_vmem.i_malloc (&default_vmem, nelem, size, e);
}
HEADER_FUNC void *
i_calloc (u32 nelem, u32 size, error *e)
{
  return default_vmem.i_calloc (&default_vmem, nelem, size, e);
}
HEADER_FUNC void *
i_realloc_right (void *ptr, u32 old_nelem, u32 nelem, u32 size, error *e)
{
  return default_vmem
      .i_realloc_right (&default_vmem, ptr, old_nelem, nelem, size, e);
}
HEADER_FUNC void *
i_realloc_left (void *ptr, u32 old_nelem, u32 nelem, u32 size, error *e)
{
  return default_vmem
      .i_realloc_left (&default_vmem, ptr, old_nelem, nelem, size, e);
}
HEADER_FUNC void *
i_crealloc_right (void *ptr, u32 old_nelem, u32 nelem, u32 size, error *e)
{
  return default_vmem
      .i_crealloc_right (&default_vmem, ptr, old_nelem, nelem, size, e);
}
HEADER_FUNC void *
i_crealloc_left (void *ptr, u32 old_nelem, u32 nelem, u32 size, error *e)
{
  return default_vmem
      .i_crealloc_left (&default_vmem, ptr, old_nelem, nelem, size, e);
}
HEADER_FUNC void
i_free (void *ptr)
{
  default_vmem.i_free (&default_vmem, ptr);
}
#define i_cfree(ptr) \
  do                 \
  {                  \
    if (ptr)         \
    {                \
      i_free (ptr);  \
    }                \
  }                  \
  while (0)

/******************************************************************************
 * SECTION: File System
 * ----------------------------------------------------------------------------
 * @brief File System
 *
 * Interfaces for reading and writing from a file with a vtable to allow
 * and consumers to change default file behavior
 ******************************************************************************/

typedef struct i_file_vtable        i_file_vtable;
typedef struct i_file_system_vtable i_file_system_vtable;
typedef struct i_file               i_file;

typedef enum
{
  I_SEEK_END,
  I_SEEK_CUR,
  I_SEEK_SET,
} seek_t;

struct i_file_vtable
{
  ////////////////////////////////////////////////////////////
  // Properties
  err_t (*i_close) (i_file *fp, error *e);
  err_t (*i_eof) (i_file *fp, error *e);
  err_t (*i_fsync) (const i_file *fp, error *e);
  i64 (*i_file_size) (const i_file *fp, error *e);

  ////////////////////////////////////////////////////////////
  // Read
  i64 (*i_read_some) (const i_file *fp, void *dest, u64 nbytes, error *e);
  i64 (*i_read_all) (const i_file *fp, void *dest, u64 nbytes, error *e);
  i64 (*i_pread_some) (
      const i_file *fp,
      void         *dest,
      u64           n,
      u64           offset,
      error        *e
  );
  i64 (*i_pread_all) (
      const i_file *fp,
      void         *dest,
      u64           n,
      u64           offset,
      error        *e
  );
  i64 (*i_readv_some) (
      const i_file       *fp,
      const struct bytes *arrs,
      int                 iovcnt,
      error              *e
  );
  i64 (*i_readv_all) (
      const i_file *fp,
      struct bytes *arrs,
      int           iovcnt,
      error        *e
  );

  ////////////////////////////////////////////////////////////
  // Write
  i64 (*i_write_some) (const i_file *fp, const void *src, u64 nbytes, error *e);
  err_t (*i_write_all) (
      const i_file *fp,
      const void   *src,
      u64           nbytes,
      error        *e
  );
  i64 (*i_pwrite_some) (
      const i_file *fp,
      const void   *src,
      u64           n,
      u64           offset,
      error        *e
  );
  err_t (*i_pwrite_all) (
      const i_file *fp,
      const void   *src,
      u64           n,
      u64           offset,
      error        *e
  );
  i64 (*i_writev_some) (
      const i_file       *fp,
      const struct bytes *arrs,
      int                 iovcnt,
      error              *e
  );
  err_t (*i_writev_all) (
      const i_file *fp,
      struct bytes *arrs,
      int           iovcnt,
      error        *e
  );

  ////////////////////////////////////////////////////////////
  // Other
  err_t (*i_truncate) (const i_file *fp, u64 bytes, error *e);
  err_t (*i_fallocate) (i_file *fp, u64 bytes, error *e);
  i64 (*i_seek) (const i_file *fp, u64 offset, seek_t whence, error *e);
};

struct i_file_system_vtable
{
  ////////////////////////////////////////////////////////////
  // Open
  err_t (*i_open_rw) (
      i_file_system_vtable *vfs,
      i_file               *dest,
      const char           *fname,
      error                *e
  );
  err_t (*i_open_r) (
      i_file_system_vtable *vfs,
      i_file               *dest,
      const char           *fname,
      error                *e
  );
  err_t (*i_open_w) (
      i_file_system_vtable *vfs,
      i_file               *dest,
      const char           *fname,
      error                *e
  );

  ////////////////////////////////////////////////////////////
  // Others
  err_t (*i_remove_quiet) (
      i_file_system_vtable *vfs,
      const char           *fname,
      error                *e
  );
  err_t (*i_unlink) (i_file_system_vtable *vfs, const char *name, error *e);
  err_t (*i_mkdir) (i_file_system_vtable *vfs, const char *name, error *e);
  err_t (*i_mkdir_quiet) (
      i_file_system_vtable *vfs,
      const char           *name,
      error                *e
  );
  err_t (*i_rm_rf) (i_file_system_vtable *vfs, const char *path, error *e);

  ////////////////////////////////////////////////////////////
  // Wrappers
  err_t (*i_access_rw) (i_file_system_vtable *vfs, const char *fname, error *e);
  bool (*i_exists_rw) (i_file_system_vtable *vfs, const char *fname);
  err_t (*i_touch) (i_file_system_vtable *vfs, const char *fname, error *e);
  err_t (*i_dir_exists) (
      i_file_system_vtable *vfs,
      const char           *fname,
      bool                 *dest,
      error                *e
  );
  err_t (*i_file_exists) (
      i_file_system_vtable *vfs,
      const char           *fname,
      bool                 *dest,
      error                *e
  );
};

struct i_file
{
#if PLATFORM_WINDOWS
  HANDLE handle;
#else
  int fd;
#endif
  i_file_vtable        *fvtable;
  i_file_system_vtable *fsvtable;
};

extern struct i_file_vtable        default_fvtable;
extern struct i_file_system_vtable default_fsvtable;

/*-----------------------------------------------------------------------------
 * SUBSECTION: Open / Closing Files
 *----------------------------------------------------------------------------*/

HEADER_FUNC err_t
i_open_rw (i_file *dest, const char *fname, error *e)
{
  return default_fsvtable.i_open_rw (&default_fsvtable, dest, fname, e);
}
HEADER_FUNC err_t
i_open_r (i_file *dest, const char *fname, error *e)
{
  return default_fsvtable.i_open_r (&default_fsvtable, dest, fname, e);
}
HEADER_FUNC err_t
i_open_w (i_file *dest, const char *fname, error *e)
{
  return default_fsvtable.i_open_w (&default_fsvtable, dest, fname, e);
}
HEADER_FUNC err_t
i_close (i_file *fp, error *e)
{
  return fp->fvtable->i_close (fp, e);
}
HEADER_FUNC err_t
i_eof (i_file *fp, error *e)
{
  return fp->fvtable->i_eof (fp, e);
}
HEADER_FUNC err_t
i_fsync (const i_file *fp, error *e)
{
  return fp->fvtable->i_fsync (fp, e);
}

/*-----------------------------------------------------------------------------
 * SUBSECTION: Positional Read / Write
 *----------------------------------------------------------------------------*/

HEADER_FUNC i64
i_pread_some (const i_file *fp, void *dest, u64 n, u64 offset, error *e)
{
  return fp->fvtable->i_pread_some (fp, dest, n, offset, e);
}
HEADER_FUNC i64
i_pread_all (const i_file *fp, void *dest, u64 n, u64 offset, error *e)
{
  return fp->fvtable->i_pread_all (fp, dest, n, offset, e);
}

HEADER_FUNC i64
i_pwrite_some (const i_file *fp, const void *src, u64 n, u64 offset, error *e)
{
  return fp->fvtable->i_pwrite_some (fp, src, n, offset, e);
}
HEADER_FUNC err_t
i_pwrite_all (const i_file *fp, const void *src, u64 n, u64 offset, error *e)
{
  return fp->fvtable->i_pwrite_all (fp, src, n, offset, e);
}

/*-----------------------------------------------------------------------------
 * SUBSECTION: IO Vec read / write
 *----------------------------------------------------------------------------*/

HEADER_FUNC i64
i_writev_some (const i_file *fp, const struct bytes *arrs, int iovcnt, error *e)
{
  return fp->fvtable->i_writev_some (fp, arrs, iovcnt, e);
}
HEADER_FUNC err_t
i_writev_all (const i_file *fp, struct bytes *arrs, int iovcnt, error *e)
{
  return fp->fvtable->i_writev_all (fp, arrs, iovcnt, e);
}
HEADER_FUNC i64
i_readv_some (const i_file *fp, const struct bytes *arrs, int iovcnt, error *e)
{
  return fp->fvtable->i_readv_some (fp, arrs, iovcnt, e);
}
HEADER_FUNC i64
i_readv_all (const i_file *fp, struct bytes *arrs, int iovcnt, error *e)
{
  return fp->fvtable->i_readv_all (fp, arrs, iovcnt, e);
}

/*-----------------------------------------------------------------------------
 * SUBSECTION: Basic Read / Write
 *----------------------------------------------------------------------------*/

HEADER_FUNC i64
i_read_some (const i_file *fp, void *dest, u64 nbytes, error *e)
{
  return fp->fvtable->i_read_some (fp, dest, nbytes, e);
}
HEADER_FUNC i64
i_read_all (const i_file *fp, void *dest, u64 nbytes, error *e)
{
  return fp->fvtable->i_read_all (fp, dest, nbytes, e);
}
HEADER_FUNC i64
i_write_some (const i_file *fp, const void *src, u64 nbytes, error *e)
{
  return fp->fvtable->i_write_some (fp, src, nbytes, e);
}
HEADER_FUNC err_t
i_write_all (const i_file *fp, const void *src, u64 nbytes, error *e)
{
  return fp->fvtable->i_write_all (fp, src, nbytes, e);
}

/*-----------------------------------------------------------------------------
 * SUBSECTION: Others
 *----------------------------------------------------------------------------*/

HEADER_FUNC err_t
i_truncate (const i_file *fp, u64 bytes, error *e)
{
  return fp->fvtable->i_truncate (fp, bytes, e);
}
HEADER_FUNC err_t
i_fallocate (i_file *fp, u64 bytes, error *e)
{
  return fp->fvtable->i_fallocate (fp, bytes, e);
}
HEADER_FUNC i64
i_file_size (const i_file *fp, error *e)
{
  return fp->fvtable->i_file_size (fp, e);
}
HEADER_FUNC i64
i_seek (const i_file *fp, u64 offset, seek_t whence, error *e)
{
  return fp->fvtable->i_seek (fp, offset, whence, e);
}
HEADER_FUNC err_t
i_remove_quiet (const char *fname, error *e)
{
  return default_fsvtable.i_remove_quiet (&default_fsvtable, fname, e);
}
HEADER_FUNC err_t
i_unlink (const char *name, error *e)
{
  return default_fsvtable.i_unlink (&default_fsvtable, name, e);
}
HEADER_FUNC err_t
i_mkdir (const char *name, error *e)
{
  return default_fsvtable.i_mkdir (&default_fsvtable, name, e);
}
HEADER_FUNC err_t
i_mkdir_quiet (const char *name, error *e)
{
  return default_fsvtable.i_mkdir_quiet (&default_fsvtable, name, e);
}
HEADER_FUNC err_t
i_rm_rf (const char *path, error *e)
{
  return default_fsvtable.i_rm_rf (&default_fsvtable, path, e);
}

HEADER_FUNC err_t
i_access_rw (const char *fname, error *e)
{
  return default_fsvtable.i_access_rw (&default_fsvtable, fname, e);
}
HEADER_FUNC bool
i_exists_rw (const char *fname)
{
  return default_fsvtable.i_exists_rw (&default_fsvtable, fname);
}
HEADER_FUNC err_t
i_touch (const char *fname, error *e)
{
  return default_fsvtable.i_touch (&default_fsvtable, fname, e);
}
HEADER_FUNC err_t
i_dir_exists (const char *fname, bool *dest, error *e)
{
  return default_fsvtable.i_dir_exists (&default_fsvtable, fname, dest, e);
}
HEADER_FUNC err_t
i_file_exists (const char *fname, bool *dest, error *e)
{
  return default_fsvtable.i_file_exists (&default_fsvtable, fname, dest, e);
}

/*-----------------------------------------------------------------------------
 * SUBSECTION: Short hands / wrappers
 *----------------------------------------------------------------------------*/

HEADER_FUNC err_t
i_pread_all_expect (
    i_file   *fp,
    void     *dest,
    const u64 n,
    const u64 offset,
    error    *e
)
{
  const i64 ret = i_pread_all (fp, dest, n, offset, e);
  WRAP (ret);

  if (unlikely ((u64)ret != n))
  {
    return error_causef (
        e,
        ERR_CORRUPT,
        "pread: short read (got %" PRId64 " of %" PRId64 " bytes)",
        ret,
        (i64)n
    );
  }

  return SUCCESS;
}

HEADER_FUNC i64
i_read_all_expect (i_file *fp, void *dest, const u64 nbytes, error *e)
{
  const i64 ret = i_read_all (fp, dest, nbytes, e);
  WRAP (ret);

  if (unlikely ((u64)ret != nbytes))
  {
    return error_causef (
        e,
        ERR_CORRUPT,
        "read: short read (got %" PRId64 " of %" PRId64 " bytes)",
        ret,
        (i64)nbytes
    );
  }

  return SUCCESS;
}

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
  bool (*i_mutex_try_lock) (i_threading *t, i_mutex *m);
  void (*i_mutex_unlock) (i_threading *t, i_mutex *m);

  err_t (*i_cond_create) (i_threading *t, i_cond *c, error *e);
  void (*i_cond_free) (i_threading *t, i_cond *c);
  void (*i_cond_wait) (i_threading *t, i_cond *c, i_mutex *m);
  void (*i_cond_timed_wait) (i_threading *t, i_cond *c, i_mutex *m, u64 msec);
  void (*i_cond_signal) (i_threading *t, i_cond *c);
  void (*i_cond_broadcast) (i_threading *t, i_cond *c);
};

extern struct i_threading default_threading;

/*-----------------------------------------------------------------------------
 * SUBSECTION: Thread
 *----------------------------------------------------------------------------*/

HEADER_FUNC err_t
i_thread_create (
    i_thread *th,
    void *(*start_routine) (void *),
    void  *arg,
    error *e
)
{
  return default_threading
      .i_thread_create (&default_threading, th, start_routine, arg, e);
}
HEADER_FUNC err_t
i_thread_join (i_thread *th, error *e)
{
  return default_threading.i_thread_join (&default_threading, th, e);
}

/*-----------------------------------------------------------------------------
 * SUBSECTION: Mutex
 *----------------------------------------------------------------------------*/

HEADER_FUNC err_t
i_mutex_create (i_mutex *m, error *e)
{
  return default_threading.i_mutex_create (&default_threading, m, e);
}
HEADER_FUNC void
i_mutex_free (i_mutex *m)
{
  default_threading.i_mutex_free (&default_threading, m);
}
HEADER_FUNC void
i_mutex_lock (i_mutex *m)
{
  default_threading.i_mutex_lock (&default_threading, m);
}
HEADER_FUNC bool
i_mutex_try_lock (i_mutex *m)
{
  return default_threading.i_mutex_try_lock (&default_threading, m);
}
HEADER_FUNC void
i_mutex_unlock (i_mutex *m)
{
  default_threading.i_mutex_unlock (&default_threading, m);
}

/*-----------------------------------------------------------------------------
 * SUBSECTION: Condition Variable
 *----------------------------------------------------------------------------*/

HEADER_FUNC err_t
i_cond_create (i_cond *c, error *e)
{
  return default_threading.i_cond_create (&default_threading, c, e);
}
HEADER_FUNC void
i_cond_free (i_cond *c)
{
  default_threading.i_cond_free (&default_threading, c);
}
HEADER_FUNC void
i_cond_wait (i_cond *c, i_mutex *m)
{
  default_threading.i_cond_wait (&default_threading, c, m);
}
HEADER_FUNC void
i_cond_timed_wait (i_cond *c, i_mutex *m, u64 msec)
{
  default_threading.i_cond_timed_wait (&default_threading, c, m, msec);
}
HEADER_FUNC void
i_cond_signal (i_cond *c)
{
  default_threading.i_cond_signal (&default_threading, c);
}
HEADER_FUNC void
i_cond_broadcast (i_cond *c)
{
  default_threading.i_cond_broadcast (&default_threading, c);
}

/******************************************************************************
 * SECTION: Time
 * ----------------------------------------------------------------------------
 * @brief Time utilities for each platform
 ******************************************************************************/

typedef struct i_timer i_timer;

#if PLATFORM_WINDOWS
struct i_timer
{
  LARGE_INTEGER frequency;
  LARGE_INTEGER start;
};
#elif PLATFORM_IOS
struct i_timer
{
  u64 start;
  u64 numer;
  u64 denom;
};
#elif PLATFORM_EMSCRIPTEN
struct i_timer
{
  f64 start;
};
#else // POSIX (Linux, Android, BSD)
struct i_timer
{
  struct timespec start;
};
#endif

err_t i_timer_create (i_timer *timer, error *e);
void  i_timer_free (i_timer *timer);
u64   i_timer_now_ns (i_timer *timer);
u64   i_timer_now_us (i_timer *timer);
u64   i_timer_now_ms (i_timer *timer);
f64   i_timer_now_s (i_timer *timer);
void  i_sleep_us (u64 us);
#define i_sleep_ms(ms) i_sleep_us (1000 * ms)
#define i_sleep_s(s)   i_sleep_us (1000000 * s)

/*-----------------------------------------------------------------------------
 * SUBSECTION: Fault Injection
 *----------------------------------------------------------------------------*/

/**
 * @brief Returns ERR_IO on mutex creation
 *
 * @return Garunteed error
 */
err_t i_mutex_create_errio (i_threading *t, i_mutex *m, error *e);

/**
 * @brief Returns ERR_IO on condition variable creation
 *
 * @return Garunteed error
 */
err_t i_cond_create_errio (i_threading *t, i_cond *m, error *e);

/**
 * @brief Mallocs and returns ERR_NOMEM
 *
 * @return garunteed NULL
 */
void *i_malloc_nomem (i_vmem *v, u32 nelem, u32 size, error *e);

/**
 * @brief An erroring open_w
 *
 * @return A garunteed error open
 */
err_t i_open_errio (
    i_file_system_vtable *vfs,
    i_file               *dest,
    const char           *fname,
    error                *e
);

/**
 * @brief An erroring seek
 *
 * @return A garunteed error seek
 */
i64 i_seek_errio (const i_file *fp, u64 offset, seek_t whence, error *e);

#endif // OS_H
