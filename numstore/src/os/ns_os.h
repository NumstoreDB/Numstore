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
#include "os/ns_malloc.h"

#include <stdbool.h>
#include <time.h>

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef _WIN32_WINNT
#    define _WIN32_WINNT 0x0601 // Windows 7+: INIT_ONCE, SetFileInformationByHandle
#  endif
#  include <windows.h>
#else
#  include <pthread.h>
#endif

typedef struct i_file   i_file;
typedef struct i_thread i_thread;
typedef struct i_mutex  i_mutex;
typedef struct i_cond   i_cond;
typedef struct i_timer  i_timer;

typedef enum
{
  I_SEEK_END,
  I_SEEK_CUR,
  I_SEEK_SET,
} seek_t;

////////////////////////////
/// Operating system vtable
///
/// Conventions:
///   - os_self:   the i_os.self of the OS that created the object
///   - file_self, mutex, cond, thread, timer: the .self of that object
///   - An object must be freed through the same OS that created it

struct os_vtable
{
  // Opening and closing files
  err_t (*open_file_rw) (void *os_self, i_file *dest, const char *fname, error *e);
  err_t (*open_file_r) (void *os_self, i_file *dest, const char *fname, error *e);
  err_t (*open_file_w) (void *os_self, i_file *dest, const char *fname, error *e);
  err_t (*close_file) (void *os_self, void *file, error *e);

  // File system
  err_t (*remove_quiet) (void *os_self, const char *fname, error *e);
  err_t (*unlink) (void *os_self, const char *fname, error *e);
  err_t (*file_exists) (void *os_self, const char *fname, bool *dest, error *e);

  // File operations
  err_t (*fsync) (void *file_self, error *e);
  i64 (*file_size) (void *file_self, error *e);
  i64 (*read_all) (void *file_self, void *dest, u64 nbytes, error *e);
  i64 (*pread_all) (void *file_self, void *dest, u64 n, u64 offset, error *e);
  err_t (*write_all) (void *file_self, const void *src, u64 nbytes, error *e);
  err_t (*pwrite_all) (void *file_self, const void *src, u64 n, u64 offset, error *e);
  err_t (*writev_all) (void *file_self, struct bytes *arrs, int iovcnt, error *e);
  err_t (*truncate) (void *file_self, u64 bytes, error *e);
  err_t (*prealloc) (void *file_self, u64 bytes, error *e);
  i64 (*seek) (void *file_self, u64 offset, seek_t whence, error *e);

  // Threads
  err_t (*thread_create) (
      void     *os_self,
      i_thread *dest,
      void *(*start) (void *),
      void  *arg,
      error *e
  );
  err_t (*thread_join) (void *os_self, void *thread, error *e);

  // Mutexes
  err_t (*mutex_create) (void *os_self, i_mutex *dest, error *e);
  void (*mutex_free) (void *os_self, void *mutex);
  void (*mutex_lock) (void *mutex);
  void (*mutex_unlock) (void *mutex);

  // Condition variables
  err_t (*cond_create) (void *os_self, i_cond *dest, error *e);
  void (*cond_free) (void *os_self, void *cond);
  void (*cond_wait) (void *cond, void *mutex);
  void (*cond_timed_wait) (void *cond, void *mutex, u64 msec);
  void (*cond_signal) (void *cond);
  void (*cond_broadcast) (void *cond);

  // Timer
  err_t (*timer_create) (void *os_self, i_timer *dest, error *e);
  void (*timer_free) (void *os_self, void *timer);
  u64 (*timer_now_ns) (void *timer);
};

typedef struct i_os
{
  const struct os_vtable *table;
  void                   *self;
} i_os;

struct i_file
{
  const struct os_vtable *table;
  void                   *self;
};

struct i_mutex
{
  const struct os_vtable *table;
  void                   *self;
};

struct i_cond
{
  const struct os_vtable *table;
  void                   *self;
};

struct i_thread
{
  const struct os_vtable *table;
  void                   *self;
};

struct i_timer
{
  const struct os_vtable *table;
  void                   *self;
};

////////////////////////////
/// Constructors

err_t system_os_create (struct i_mem mem, struct i_os *dest, error *e);
void system_os_free (struct i_os os);

/// Wraps [delegate]: every fallible call fails with probability
/// [fail_percent] in [0, 1], otherwise forwards to [delegate].
/// [delegate] is borrowed - it must outlive the faulty OS and is not freed by it.
err_t faulty_os_create (
    struct i_mem mem,
    struct i_os  delegate,
    float        fail_percent,
    struct i_os *dest,
    error       *e
);
void faulty_os_free (struct i_os os);

////////////////////////////
/// Run once

#ifdef _WIN32
typedef INIT_ONCE i_once;
#  define I_ONCE_INIT INIT_ONCE_STATIC_INIT
#else
typedef pthread_once_t i_once;
#  define I_ONCE_INIT PTHREAD_ONCE_INIT
#endif

void i_once_run (i_once *once, void (*fn) (void));

#endif
