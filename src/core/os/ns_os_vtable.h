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

#ifndef NS_OS_VTABLE_H
#define NS_OS_VTABLE_H

#include "core/ns_bytes.h"
#include "core/ns_error.h"
#include "core/ns_stdtypes.h"

/******************************************************************************
 * SECTION: OS Virtual Table
 * ----------------------------------------------------------------------------
 * @brief Single virtual table shape for every os subsystem (file, filesystem,
 * memory, threading). There is exactly one instance of this table -
 * `default_os_vtable`, defined in os/common/ns_os_vtable.c - wired to the
 * impl_<name> functions declared below. Each impl_<name> symbol is provided
 * by exactly one platform's os/posix or os/windows sources (guarded by
 * PLATFORM_POSIX / PLATFORM_WINDOWS) or by os/common sources for subsystems
 * that don't vary by platform (memory).
 ******************************************************************************/

typedef enum
{
  I_SEEK_END,
  I_SEEK_CUR,
  I_SEEK_SET,
} seek_t;

typedef struct i_file   i_file;
typedef struct i_mutex  i_mutex;
typedef struct i_cond   i_cond;
typedef struct i_thread i_thread;

struct os_vtable
{
  // File - Properties
  err_t (*close) (void *fp, error *e);
  err_t (*eof) (void *fp, error *e);
  err_t (*fsync) (void *fp, error *e);
  i64 (*file_size) (void *fp, error *e);

  // File - Read
  i64 (*read_all) (void *fp, void *dest, u64 nbytes, error *e);
  i64 (*pread_all) (void *fp, void *dest, u64 n, u64 offset, error *e);

  // File - Write
  err_t (*write_all) (void *fp, const void *src, u64 nbytes, error *e);
  err_t (*pwrite_all) (void *fp, const void *src, u64 n, u64 offset, error *e);
  err_t (*writev_all) (void *fp, struct bytes *arrs, int iovcnt, error *e);

  // File - Other
  err_t (*truncate) (void *fp, u64 bytes, error *e);
  err_t (*fallocate) (void *fp, u64 bytes, error *e);
  i64 (*seek) (void *fp, u64 offset, seek_t whence, error *e);

#ifdef TESTING
  void *test_data;
#endif

  // File System
  err_t (*open_rw) (void *vfs, i_file *dest, const char *fname, error *e);
  err_t (*open_r) (void *vfs, i_file *dest, const char *fname, error *e);
  err_t (*open_w) (void *vfs, i_file *dest, const char *fname, error *e);
  err_t (*remove_quiet) (void *vfs, const char *fname, error *e);
  err_t (*unlink) (void *vfs, const char *name, error *e);
  err_t (*file_exists) (void *vfs, const char *fname, bool *dest, error *e);

  // Memory
  void *(*malloc) (void *v, u32 nelem, u32 size, error *e);
  void *(*calloc) (void *v, u32 nelem, u32 size, error *e);
  void *(*realloc) (void *v, void *ptr, u32 nelem, u32 size, error *e);
  void (*free) (void *v, void *ptr);

  // Threading
  err_t (*thread_create) (
      void     *t,
      i_thread *th,
      void *(*start_routine) (void *),
      void  *arg,
      error *e
  );
  err_t (*thread_join) (void *t, i_thread *th, error *e);

  err_t (*mutex_create) (void *t, i_mutex *m, error *e);
  void (*mutex_free) (void *t, i_mutex *m);
  void (*mutex_lock) (void *t, i_mutex *m);
  void (*mutex_unlock) (void *t, i_mutex *m);

  err_t (*cond_create) (void *t, i_cond *c, error *e);
  void (*cond_free) (void *t, i_cond *c);
  void (*cond_wait) (void *t, i_cond *c, i_mutex *m);
  void (*cond_timed_wait) (void *t, i_cond *c, i_mutex *m, u64 msec);
  void (*cond_signal) (void *t, i_cond *c);
  void (*cond_broadcast) (void *t, i_cond *c);
};

/*-----------------------------------------------------------------------------
 * SUBSECTION: Default implementation
 *----------------------------------------------------------------------------*/

err_t impl_close (void *fp, error *e);
err_t impl_fsync (void *fp, error *e);
i64 impl_file_size (void *fp, error *e);
i64 impl_read_all (void *fp, void *dest, u64 nbytes, error *e);
i64 impl_pread_all (void *fp, void *dest, u64 n, u64 offset, error *e);
err_t impl_write_all (void *fp, const void *src, u64 nbytes, error *e);
err_t impl_pwrite_all (void *fp, const void *src, u64 n, u64 offset, error *e);
err_t impl_writev_all (void *fp, struct bytes *arrs, int iovcnt, error *e);
err_t impl_truncate (void *fp, u64 bytes, error *e);
err_t impl_fallocate (void *fp, u64 bytes, error *e);
i64 impl_seek (void *fp, u64 offset, seek_t whence, error *e);

err_t impl_open_rw (void *vfs, i_file *dest, const char *fname, error *e);
err_t impl_open_r (void *vfs, i_file *dest, const char *fname, error *e);
err_t impl_open_w (void *vfs, i_file *dest, const char *fname, error *e);
err_t impl_remove_quiet (void *vfs, const char *fname, error *e);
err_t impl_unlink (void *vfs, const char *name, error *e);
err_t impl_file_exists (void *vfs, const char *fname, bool *dest, error *e);

void *impl_malloc (void *v, u32 nelem, u32 size, error *e);
void *impl_calloc (void *v, u32 nelem, u32 size, error *e);
void *impl_realloc (void *v, void *ptr, u32 nelem, u32 size, error *e);
void impl_free (void *v, void *ptr);

err_t impl_thread_create (
    void     *t,
    i_thread *th,
    void *(*start_routine) (void *),
    void  *arg,
    error *e
);
err_t impl_thread_join (void *t, i_thread *th, error *e);

err_t impl_mutex_create (void *t, i_mutex *m, error *e);
void impl_mutex_free (void *t, i_mutex *m);
void impl_mutex_lock (void *t, i_mutex *m);
void impl_mutex_unlock (void *t, i_mutex *m);

err_t impl_cond_create (void *t, i_cond *c, error *e);
void impl_cond_free (void *t, i_cond *c);
void impl_cond_wait (void *t, i_cond *c, i_mutex *m);
void impl_cond_timed_wait (void *t, i_cond *c, i_mutex *m, u64 msec);
void impl_cond_signal (void *t, i_cond *c);
void impl_cond_broadcast (void *t, i_cond *c);

extern const struct os_vtable default_os_vtable;

#endif
