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

#ifndef C_SPECX_H
#define C_SPECX_H

////////////////////////////////////////////////////////////
// DEV / SYSTEM

#define PLATFORM_WINDOWS    0
#define PLATFORM_LINUX      0
#define PLATFORM_ANDROID    0
#define PLATFORM_MAC        0
#define PLATFORM_IOS        0
#define PLATFORM_BSD        0
#define PLATFORM_EMSCRIPTEN 0
#define PLATFORM_UNIX       0
#define PLATFORM_POSIX      0

////////////////////////////////////////////////////////////
// Emscripten
#ifdef __EMSCRIPTEN__
#  undef PLATFORM_EMSCRIPTEN
#  define PLATFORM_EMSCRIPTEN 1
#  undef PLATFORM_POSIX
#  define PLATFORM_POSIX 1
#endif

////////////////////////////////////////////////////////////
// Windows
#if defined(_WIN32) || defined(_WIN64) || defined(__WINDOWS__)
#  undef PLATFORM_WINDOWS
#  define PLATFORM_WINDOWS 1
#endif

////////////////////////////////////////////////////////////
// Android
#if defined(__ANDROID__)
#  undef PLATFORM_ANDROID
#  define PLATFORM_ANDROID 1
#  undef PLATFORM_LINUX
#  define PLATFORM_LINUX 0
#  undef PLATFORM_UNIX
#  define PLATFORM_UNIX 1
#  undef PLATFORM_POSIX
#  define PLATFORM_POSIX 1
#endif

////////////////////////////////////////////////////////////
// Linux
#if defined(__linux__) && !defined(__ANDROID__)
#  undef PLATFORM_LINUX
#  define PLATFORM_LINUX 1
#  undef PLATFORM_UNIX
#  define PLATFORM_UNIX 1
#  undef PLATFORM_POSIX
#  define PLATFORM_POSIX 1
#endif

////////////////////////////////////////////////////////////
// Apple
#if defined(__APPLE__) && defined(__MACH__)
#  include "TargetConditionals.h"

////////////////////////////////////////////////////////////
// IOS
#  if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
#    undef PLATFORM_IOS
#    define PLATFORM_IOS 1

////////////////////////////////////////////////////////////
// MacOS
#  elif TARGET_OS_MAC
#    undef PLATFORM_MAC
#    define PLATFORM_MAC 1
#  endif
#  undef PLATFORM_UNIX
#  undef PLATFORM_POSIX
#  define PLATFORM_UNIX  1
#  define PLATFORM_POSIX 1
#endif

////////////////////////////////////////////////////////////
// BSD
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__) \
    || defined(__bsdi__)
#  undef PLATFORM_BSD
#  undef PLATFORM_UNIX
#  undef PLATFORM_POSIX
#  define PLATFORM_BSD   1
#  define PLATFORM_UNIX  1
#  define PLATFORM_POSIX 1
#endif

////////////////////////////////////////////////////////////
// UNIX
#if !PLATFORM_UNIX && (defined(__unix__) || defined(__unix))
#  undef PLATFORM_UNIX
#  undef PLATFORM_POSIX
#  define PLATFORM_UNIX  1
#  define PLATFORM_POSIX 1
#endif

////////////////////////////////////////////////////////////
// Verify
#if (                                                                                  \
    PLATFORM_WINDOWS + PLATFORM_LINUX + PLATFORM_ANDROID + PLATFORM_MAC + PLATFORM_IOS \
    + PLATFORM_BSD + PLATFORM_EMSCRIPTEN                                               \
) > 1
#  warning "Multiple platforms detected - check your build configuration"
#endif

////////////////////////////////////////////////////////////
// Utils
#define PLATFORM_APPLE   (PLATFORM_MAC || PLATFORM_IOS)
#define PLATFORM_MOBILE  (PLATFORM_ANDROID || PLATFORM_IOS)
#define PLATFORM_DESKTOP (PLATFORM_WINDOWS || PLATFORM_LINUX || PLATFORM_MAC || PLATFORM_BSD)

////////////////////////////////////////////////////////////
// Branch Prediction
#if PLATFORM_WINDOWS
#  define likely(x)   (x)
#  define unlikely(x) (x)
#elif PLATFORM_POSIX
#  define likely(x)   __builtin_expect (!!(x), 1)
#  define unlikely(x) __builtin_expect (!!(x), 0)
#endif

////////////////////////////////////////////////////////////
// UNREACHABLE
#if PLATFORM_WINDOWS
#  define UNREACHABLE_HINT() __assume (0)
#elif PLATFORM_POSIX
#  define UNREACHABLE_HINT() __builtin_unreachable ()
#endif

////////////////////////////////////////////////////////////
// NORETURN
#if PLATFORM_WINDOWS
#  define NORETURN __declspec (noreturn)
#elif PLATFORM_POSIX
#  define NORETURN __attribute__ ((noreturn))
#endif

////////////////////////////////////////////////////////////
// PRINTF_ATTR
#if PLATFORM_WINDOWS
#  define PRINTF_ATTR(fmt_pos, va_pos)
#else
#  define PRINTF_ATTR(fmt_pos, va_pos) __attribute__ ((format (printf, fmt_pos, va_pos)))
#endif

////////////////////////////////////////////////////////////
// MAYBE_UNUSED
#if PLATFORM_WINDOWS
#  define MAYBE_UNUSED
#else
#  define MAYBE_UNUSED __attribute__ ((unused))
#endif

////////////////////////////////////////////////////////////
// ANSI_COLORS
#if defined(_WIN32)
#  define ANSI_COLORS 0
#else
#  define ANSI_COLORS 1
#endif

////////////////////////////////////////////////////////////
// HAS_BUILTIN_OVERFLOW
#if PLATFORM_WINDOWS
#  define HAS_BUILTIN_OVERFLOW 0
#else
#  define HAS_BUILTIN_OVERFLOW 1
#endif

#define HEADER_FUNC static inline MAYBE_UNUSED

HEADER_FUNC const char *
platformstr (void)
{
  if (PLATFORM_WINDOWS) { return "Windows"; }
  else if (PLATFORM_LINUX) { return "Linux"; }
  else if (PLATFORM_ANDROID) { return "Android"; }
  else if (PLATFORM_MAC) { return "macOS"; }
  else if (PLATFORM_IOS) { return "iOS"; }
  else if (PLATFORM_BSD) { return "BSD"; }
  else if (PLATFORM_EMSCRIPTEN) { return "Emscripten/WebAssembly"; }

  return 0;
}

////////////////////////////////////////////////////////////
// SYSTEM INCLUDES

#include <complex.h>
#include <inttypes.h>
#include <math.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if PLATFORM_WINDOWS
#  define WIN32_LEAN_AND_MEAN
#  include "windows.h"
#  include "winsock2.h"
#  include "ws2tcpip.h"
#elif PLATFORM_POSIX
#  include "sys/poll.h"

#  include <pthread.h>
#  include <semaphore.h>
#  include <time.h>
#endif

////////////////////////////////////////////////////////////
// DEV / STDTYPES

// Unsigned shorthands
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// Signed shorthands
typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

// Float shorthands
typedef u16         f16;
typedef float       f32;
typedef double      f64;
typedef long double f128;

// Complex floats

#ifdef _MSC_VER

typedef _Fcomplex cf32;
typedef _Dcomplex cf64;
typedef _Lcomplex cf128;

#else

typedef u16 cf32[2];
typedef u32 cf64[2];
typedef u64 cf128[2];
// #if defined(__SIZEOF_FLOAT128__)
//     typedef __float128 _Complex cf256;
//  #endif

#endif

typedef i8  ci16[2];
typedef i16 ci32[2];
typedef i32 ci64[2];
typedef i64 ci128[2];

typedef u8  cu16[2];
typedef u16 cu32[2];
typedef u32 cu64[2];
typedef u64 cu128[2];

// Maximum unsigned
#define U8_MAX  ((u8) ~(u8)0)
#define U16_MAX ((u16) ~(u16)0)
#define U32_MAX ((u32) ~(u32)0)
#define U64_MAX ((u64) ~(u64)0)

// Maximum signed
#define I8_MAX  ((i8)(U8_MAX >> 1))
#define I16_MAX ((i16)(U16_MAX >> 1))
#define I32_MAX ((i32)(U32_MAX >> 1))
#define I64_MAX ((i64)(U64_MAX >> 1))

// Max of both negative and positive
#define I8_ABS_MAX  ((u8)(I8_MAX) + 1)
#define I16_ABS_MAX ((u16)(I16_MAX) + 1)
#define I32_ABS_MAX ((u32)(I32_MAX) + 1)
#define I64_ABS_MAX ((u64)(I64_MAX) + 1)

// Minimum signed
#define I8_MIN  ((i8)(~I8_MAX))
#define I16_MIN ((i16)(~I16_MAX))
#define I32_MIN ((i32)(~I32_MAX))
#define I64_MIN ((i64)(~I64_MAX))

#define F16_MAX  65504.0f
#define F32_MAX  3.4028235e+38f
#define F64_MAX  1.7976931348623157e+308
#define F128_MAX FLT128_MAX
#define F256_MAX 1.6113e+78913L

#define F16_MIN  (-65504.0f)
#define F32_MIN  (-3.4028235e+38f)
#define F64_MIN  (-1.7976931348623157e+308)
#define F128_MIN FLT128_MIN
#define F256_MIN (-1.6113e+78913L)

////////////////////////////////////////////////////////////
// SIMPLE UTILITIES

const char *file_basename (const char *path);

// Good format prefix string with useful information
#define FPREFIX_STR  "%s:%d (%s):             "
#define FPREFIX_ARGS file_basename (__FILE__), __LINE__, __func__

// Container of struct elements using offset
#define container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof (type, member)))

#define is_alpha(c)         (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z') || (c) == '_')
#define is_num(c)           ((c) >= '0' && (c) <= '9')
#define is_alpha_num(c)     (is_alpha (c) || is_num (c))
#define is_friendly_punc(c) (c == '.' || c == '/' || c == '-' || c == '_')
#define is_alpha_num_generous(c) (is_alpha (c) || is_num (c) || is_friendly_punc (c))

#define arrlen(a) (sizeof (a) / sizeof (*a))

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define STRINGIFY(x) #x
#define TOSTRING(x)  STRINGIFY (x)

#define case_ENUM_RETURN_STRING(en) \
  case en: return #en

#define CJOIN2(val) val, val
#define CJOIN3(val) val, val, val
#define CJOIN4(val) val, val, val, val
#define CJOIN5(val) val, val, val, val, val

#ifndef RH__CAT
#  define RH__CAT(a, b)  a##b
#  define RH__XCAT(a, b) RH__CAT (a, b)
#endif

////////////////////////////////////////////////////////////
// ERROR

typedef int err_t;

typedef struct
{
  err_t cause_code;    // Machine-readable error code. @c SUCCESS when no error is
                       // pending.
  char cause_msg[256]; // Null-terminated human-readable description of the
                       // failure.
  u32  cmlen;          // Length of @c cause_msg in bytes, excluding the null terminator.
  bool disable_log;    // disable the error log temporarily
} error;

typedef err_t (*isvalid_func) (void *ctx, error *e);

#define SUCCESS              0  // Operation completed successfully.
#define ERR_IO               -1 // Generic I/O error (read, write, or fsync failure).
#define ERR_NOMEM            -2 // Memory allocation failed.
#define ERR_ARITH            -3 // Integer arithmetic overflow detected.
#define ERR_CORRUPT          -4 // Corrupted data - user might've tampered with something
#define ERR_INVALID_ARGUMENT -5 // User provided an invalid argument

error error_create (void);
void  error_silence (error *e);
void  error_unsilence (error *e);
void  err_t_perror (FILE *output, error *e);
err_t error_causef (error *e, err_t c, const char *fmt, ...) PRINTF_ATTR (3, 4);
err_t error_change_causef (error *e, err_t c, const char *fmt, ...) PRINTF_ATTR (3, 4);
err_t
error_change_causef_from (error *e, err_t from, err_t to, const char *fmt, ...) PRINTF_ATTR (4, 5);
void          error_log_consume (error *e);
bool          error_equal (const error *left, const error *right);
NORETURN void error_fatal (const char *fmt, ...);
#define error_trace(e)                                                               \
  (e)->cause_code < 0 ? error_causef (e, (e)->cause_code, FPREFIX_STR, FPREFIX_ARGS) \
                      : (e)->cause_code
#define WRAP(expr)                                               \
  do                                                             \
  {                                                              \
    if (unlikely ((expr) < SUCCESS)) { return error_trace (e); } \
  }                                                              \
  while (0)
#define WRAP_GOTO(expr, label)                       \
  do                                                 \
  {                                                  \
    if (unlikely ((expr) < SUCCESS)) { goto label; } \
  }                                                  \
  while (0)

////////////////////////////////////////////////////////////
// THREADING

typedef struct
{
#if defined(_WIN32)
  CRITICAL_SECTION m;
#else
  pthread_mutex_t m;
#endif
} i_mutex;
err_t i_mutex_create (i_mutex *m, error *e);
void  i_mutex_free (i_mutex *m);
void  i_mutex_lock (i_mutex *m);
bool  i_mutex_try_lock (i_mutex *m);
void  i_mutex_unlock (i_mutex *m);

typedef struct
{
#if defined(_WIN32)
  HANDLE handle;
#else
  pthread_mutex_t mutex;
  pthread_cond_t  cond;
  unsigned int    count;
#endif
} i_semaphore;
err_t i_semaphore_create (i_semaphore *s, unsigned int value, error *e);
void  i_semaphore_free (i_semaphore *s);
void  i_semaphore_post (i_semaphore *s);
void  i_semaphore_wait (i_semaphore *s);
bool  i_semaphore_try_wait (i_semaphore *s);
bool  i_semaphore_timed_wait (i_semaphore *s, long nsec);

typedef struct
{
#if defined(_WIN32)
  SRWLOCK lock;
  bool    write_locked;
#else
  pthread_rwlock_t lock;
#endif
} i_rwlock;
err_t i_rwlock_create (i_rwlock *rw, error *e);
void  i_rwlock_free (i_rwlock *rw);
void  i_rwlock_rdlock (i_rwlock *rw);
void  i_rwlock_wrlock (i_rwlock *rw);
void  i_rwlock_unlock (i_rwlock *rw);
bool  i_rwlock_try_rdlock (i_rwlock *rw);
bool  i_rwlock_try_wrlock (i_rwlock *rw);

typedef struct
{
#if defined(_WIN32)
  HANDLE handle;
  DWORD  id;
#else
  pthread_t thread;
#endif
} i_thread;
err_t i_thread_create (i_thread *t, void *(*start_routine) (void *), void *arg, error *e);
err_t i_thread_join (i_thread *t, error *e);
void  i_thread_cancel (i_thread *t);
u64   get_available_threads (void);

typedef struct
{
#if defined(_WIN32)
  CONDITION_VARIABLE cond;
#else
  pthread_cond_t cond;
#endif
} i_cond;
err_t i_cond_create (i_cond *c, error *e);
void  i_cond_free (i_cond *c);
void  i_cond_wait (i_cond *c, i_mutex *m);
void  i_cond_timed_wait (i_cond *c, i_mutex *m, u64 msec);
void  i_cond_signal (i_cond *c);
void  i_cond_broadcast (i_cond *c);

////////////////////////////////////////////////////////////
// TIME

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

////////////////////////////////////////////////////////////
// SOCKET

#if PLATFORM_WINDOWS
typedef SOCKET    i_sock_fd;
typedef WSAPOLLFD i_pollfd;
#  define I_INVALID_SOCKET INVALID_SOCKET
#elif PLATFORM_POSIX
typedef int           i_sock_fd;
typedef struct pollfd i_pollfd;
#  define I_INVALID_SOCKET (-1)
#endif

typedef struct
{
  i_sock_fd fd;
} i_socket;

err_t i_socket_create (i_socket *s, error *e);
err_t i_socket_close (i_socket *s, error *e);
err_t i_socket_connect (i_socket *s, const char *host, u16 port, error *e);
err_t i_socket_bind (i_socket *s, int port, error *e);
err_t i_socket_listen (i_socket *s, error *e);
err_t i_socket_accept (
    i_socket *s,
    i_socket *dest,
    char     *ip_out,
    int       ip_out_len,
    int      *port_out,
    error    *e
);
err_t i_socket_set_reuseaddr (i_socket *s, error *e);
i64   i_socket_recv (i_socket *s, void *buf, u32 len, error *e);
i64   i_socket_send (i_socket *s, const void *buf, u32 len, error *e);
int   i_poll (i_pollfd *fds, u32 nfds, int timeout_ms, error *e);

u32 i_htonl (u32 host);
u32 i_ntohl (u32 net);
u16 i_htons (u16 host);
u16 i_ntohs (u16 net);

////////////////////////////////////////////////////////////
// BYTES

struct bytes
{
  u8 *head;
  u32 len;
};

struct cbytes
{
  const u8 *head;
  u32       len;
};

#define bytes_from(buffer) \
  (struct bytes)           \
  { .head = buffer, .len = sizeof (buffer) }

#define bytes_null() (struct bytes){.head = NULL, .len = 0}

////////////////////////////////////////////////////////////
// FILE_SYSTEM

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
  i64 (*i_pread_some) (const i_file *fp, void *dest, u64 n, u64 offset, error *e);
  i64 (*i_pread_all) (const i_file *fp, void *dest, u64 n, u64 offset, error *e);
  i64 (*i_readv_some) (const i_file *fp, const struct bytes *arrs, int iovcnt, error *e);
  i64 (*i_readv_all) (const i_file *fp, struct bytes *arrs, int iovcnt, error *e);

  ////////////////////////////////////////////////////////////
  // Write
  i64 (*i_write_some) (const i_file *fp, const void *src, u64 nbytes, error *e);
  err_t (*i_write_all) (const i_file *fp, const void *src, u64 nbytes, error *e);
  i64 (*i_pwrite_some) (const i_file *fp, const void *src, u64 n, u64 offset, error *e);
  err_t (*i_pwrite_all) (const i_file *fp, const void *src, u64 n, u64 offset, error *e);
  i64 (*i_writev_some) (const i_file *fp, const struct bytes *arrs, int iovcnt, error *e);
  err_t (*i_writev_all) (const i_file *fp, struct bytes *arrs, int iovcnt, error *e);

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
  err_t (*i_open_rw) (i_file_system_vtable *vfs, i_file *dest, const char *fname, error *e);
  err_t (*i_open_r) (i_file_system_vtable *vfs, i_file *dest, const char *fname, error *e);
  err_t (*i_open_w) (i_file_system_vtable *vfs, i_file *dest, const char *fname, error *e);

  ////////////////////////////////////////////////////////////
  // Others
  err_t (*i_remove_quiet) (i_file_system_vtable *vfs, const char *fname, error *e);
  err_t (*i_unlink) (i_file_system_vtable *vfs, const char *name, error *e);
  err_t (*i_mkdir) (i_file_system_vtable *vfs, const char *name, error *e);
  err_t (*i_mkdir_quiet) (i_file_system_vtable *vfs, const char *name, error *e);
  err_t (*i_rm_rf) (i_file_system_vtable *vfs, const char *path, error *e);

  ////////////////////////////////////////////////////////////
  // Wrappers
  err_t (*i_access_rw) (i_file_system_vtable *vfs, const char *fname, error *e);
  bool (*i_exists_rw) (i_file_system_vtable *vfs, const char *fname);
  err_t (*i_touch) (i_file_system_vtable *vfs, const char *fname, error *e);
  err_t (*i_dir_exists) (i_file_system_vtable *vfs, const char *fname, bool *dest, error *e);
  err_t (*i_file_exists) (i_file_system_vtable *vfs, const char *fname, bool *dest, error *e);
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

////////////////////////////////////////////////////////////
// Open / Close

HEADER_FUNC err_t
i_open_rw (i_file *dest, const char *fname, error *e)
{ return default_fsvtable.i_open_rw (&default_fsvtable, dest, fname, e); }
HEADER_FUNC err_t
i_open_r (i_file *dest, const char *fname, error *e)
{ return default_fsvtable.i_open_r (&default_fsvtable, dest, fname, e); }
HEADER_FUNC err_t
i_open_w (i_file *dest, const char *fname, error *e)
{ return default_fsvtable.i_open_w (&default_fsvtable, dest, fname, e); }
HEADER_FUNC err_t
i_close (i_file *fp, error *e)
{ return fp->fvtable->i_close (fp, e); }
HEADER_FUNC err_t
i_eof (i_file *fp, error *e)
{ return fp->fvtable->i_eof (fp, e); }
HEADER_FUNC err_t
i_fsync (const i_file *fp, error *e)
{ return fp->fvtable->i_fsync (fp, e); }

////////////////////////////////////////////////////////////
// Positional Read / Write

HEADER_FUNC i64
i_pread_some (const i_file *fp, void *dest, u64 n, u64 offset, error *e)
{ return fp->fvtable->i_pread_some (fp, dest, n, offset, e); }
HEADER_FUNC i64
i_pread_all (const i_file *fp, void *dest, u64 n, u64 offset, error *e)
{ return fp->fvtable->i_pread_all (fp, dest, n, offset, e); }

HEADER_FUNC i64
i_pwrite_some (const i_file *fp, const void *src, u64 n, u64 offset, error *e)
{ return fp->fvtable->i_pwrite_some (fp, src, n, offset, e); }
HEADER_FUNC err_t
i_pwrite_all (const i_file *fp, const void *src, u64 n, u64 offset, error *e)
{ return fp->fvtable->i_pwrite_all (fp, src, n, offset, e); }

////////////////////////////////////////////////////////////
// IO Vec

HEADER_FUNC i64
i_writev_some (const i_file *fp, const struct bytes *arrs, int iovcnt, error *e)
{ return fp->fvtable->i_writev_some (fp, arrs, iovcnt, e); }
HEADER_FUNC err_t
i_writev_all (const i_file *fp, struct bytes *arrs, int iovcnt, error *e)
{ return fp->fvtable->i_writev_all (fp, arrs, iovcnt, e); }
HEADER_FUNC i64
i_readv_some (const i_file *fp, const struct bytes *arrs, int iovcnt, error *e)
{ return fp->fvtable->i_readv_some (fp, arrs, iovcnt, e); }
HEADER_FUNC i64
i_readv_all (const i_file *fp, struct bytes *arrs, int iovcnt, error *e)
{ return fp->fvtable->i_readv_all (fp, arrs, iovcnt, e); }

////////////////////////////////////////////////////////////
// Stream Read / Write

HEADER_FUNC i64
i_read_some (const i_file *fp, void *dest, u64 nbytes, error *e)
{ return fp->fvtable->i_read_some (fp, dest, nbytes, e); }
HEADER_FUNC i64
i_read_all (const i_file *fp, void *dest, u64 nbytes, error *e)
{ return fp->fvtable->i_read_all (fp, dest, nbytes, e); }
HEADER_FUNC i64
i_write_some (const i_file *fp, const void *src, u64 nbytes, error *e)
{ return fp->fvtable->i_write_some (fp, src, nbytes, e); }
HEADER_FUNC err_t
i_write_all (const i_file *fp, const void *src, u64 nbytes, error *e)
{ return fp->fvtable->i_write_all (fp, src, nbytes, e); }

////////////////////////////////////////////////////////////
// Others

HEADER_FUNC err_t
i_truncate (const i_file *fp, u64 bytes, error *e)
{ return fp->fvtable->i_truncate (fp, bytes, e); }
HEADER_FUNC err_t
i_fallocate (i_file *fp, u64 bytes, error *e)
{ return fp->fvtable->i_fallocate (fp, bytes, e); }
HEADER_FUNC i64
i_file_size (const i_file *fp, error *e)
{ return fp->fvtable->i_file_size (fp, e); }
HEADER_FUNC i64
i_seek (const i_file *fp, u64 offset, seek_t whence, error *e)
{ return fp->fvtable->i_seek (fp, offset, whence, e); }
HEADER_FUNC err_t
i_remove_quiet (const char *fname, error *e)
{ return default_fsvtable.i_remove_quiet (&default_fsvtable, fname, e); }
HEADER_FUNC err_t
i_unlink (const char *name, error *e)
{ return default_fsvtable.i_unlink (&default_fsvtable, name, e); }
HEADER_FUNC err_t
i_mkdir (const char *name, error *e)
{ return default_fsvtable.i_mkdir (&default_fsvtable, name, e); }
HEADER_FUNC err_t
i_mkdir_quiet (const char *name, error *e)
{ return default_fsvtable.i_mkdir_quiet (&default_fsvtable, name, e); }
HEADER_FUNC err_t
i_rm_rf (const char *path, error *e)
{ return default_fsvtable.i_rm_rf (&default_fsvtable, path, e); }

////////////////////////////////////////////////////////////
// Wrappers

HEADER_FUNC err_t
i_access_rw (const char *fname, error *e)
{ return default_fsvtable.i_access_rw (&default_fsvtable, fname, e); }
HEADER_FUNC bool
i_exists_rw (const char *fname)
{ return default_fsvtable.i_exists_rw (&default_fsvtable, fname); }
HEADER_FUNC err_t
i_touch (const char *fname, error *e)
{ return default_fsvtable.i_touch (&default_fsvtable, fname, e); }
HEADER_FUNC err_t
i_dir_exists (const char *fname, bool *dest, error *e)
{ return default_fsvtable.i_dir_exists (&default_fsvtable, fname, dest, e); }
HEADER_FUNC err_t
i_file_exists (const char *fname, bool *dest, error *e)
{ return default_fsvtable.i_file_exists (&default_fsvtable, fname, dest, e); }

////////////////////////////////////////////////////////////
// Helpers

HEADER_FUNC err_t
i_pread_all_expect (i_file *fp, void *dest, const u64 n, const u64 offset, error *e)
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

////////////////////////////////////////////////////////////
// MEMORY

typedef struct i_vmem i_vmem;

struct i_vmem
{
  void *(*i_malloc) (i_vmem *v, u32 nelem, u32 size, error *e);
  void *(*i_calloc) (i_vmem *v, u32 nelem, u32 size, error *e);
  void *(*i_realloc_right) (i_vmem *v, void *ptr, u32 old_nelem, u32 nelem, u32 size, error *e);
  void *(*i_realloc_left) (i_vmem *v, void *ptr, u32 old_nelem, u32 nelem, u32 size, error *e);
  void *(*i_crealloc_right) (i_vmem *v, void *ptr, u32 old_nelem, u32 nelem, u32 size, error *e);
  void *(*i_crealloc_left) (i_vmem *v, void *ptr, u32 old_nelem, u32 nelem, u32 size, error *e);
  void (*i_free) (i_vmem *v, void *ptr);
};

extern struct i_vmem default_vmem;

HEADER_FUNC void *
i_malloc (u32 nelem, u32 size, error *e)
{ return default_vmem.i_malloc (&default_vmem, nelem, size, e); }
HEADER_FUNC void *
i_calloc (u32 nelem, u32 size, error *e)
{ return default_vmem.i_calloc (&default_vmem, nelem, size, e); }
HEADER_FUNC void *
i_realloc_right (void *ptr, u32 old_nelem, u32 nelem, u32 size, error *e)
{ return default_vmem.i_realloc_right (&default_vmem, ptr, old_nelem, nelem, size, e); }
HEADER_FUNC void *
i_realloc_left (void *ptr, u32 old_nelem, u32 nelem, u32 size, error *e)
{ return default_vmem.i_realloc_left (&default_vmem, ptr, old_nelem, nelem, size, e); }
HEADER_FUNC void *
i_crealloc_right (void *ptr, u32 old_nelem, u32 nelem, u32 size, error *e)
{ return default_vmem.i_crealloc_right (&default_vmem, ptr, old_nelem, nelem, size, e); }
HEADER_FUNC void *
i_crealloc_left (void *ptr, u32 old_nelem, u32 nelem, u32 size, error *e)
{ return default_vmem.i_crealloc_left (&default_vmem, ptr, old_nelem, nelem, size, e); }
HEADER_FUNC void
i_free (void *ptr)
{ default_vmem.i_free (&default_vmem, ptr); }
#define i_cfree(ptr)           \
  do                           \
  {                            \
    if (ptr) { i_free (ptr); } \
  }                            \
  while (0)

////////////////////////////////////////////////////////////
// INTF / LOGGING

#if ANSI_COLORS
#  define BLACK        "\033[0;30m"
#  define RED          "\033[0;31m"
#  define GREEN        "\033[0;32m"
#  define YELLOW       "\033[0;33m"
#  define BLUE         "\033[0;34m"
#  define MAGENTA      "\033[0;35m"
#  define CYAN         "\033[0;36m"
#  define WHITE        "\033[0;37m"
#  define BOLD_BLACK   "\033[1;30m"
#  define BOLD_RED     "\033[1;31m"
#  define BOLD_GREEN   "\033[1;32m"
#  define BOLD_YELLOW  "\033[1;33m"
#  define BOLD_BLUE    "\033[1;34m"
#  define BOLD_MAGENTA "\033[1;35m"
#  define BOLD_CYAN    "\033[1;36m"
#  define BOLD_WHITE   "\033[1;37m"
#  define RESET        "\033[0m"
#else
// ANSI escape codes not supported - use empty strings.
#  define BLACK        ""
#  define RED          ""
#  define GREEN        ""
#  define YELLOW       ""
#  define BLUE         ""
#  define MAGENTA      ""
#  define CYAN         ""
#  define WHITE        ""
#  define BOLD_BLACK   ""
#  define BOLD_RED     ""
#  define BOLD_GREEN   ""
#  define BOLD_YELLOW  ""
#  define BOLD_BLUE    ""
#  define BOLD_MAGENTA ""
#  define BOLD_CYAN    ""
#  define BOLD_WHITE   ""
#  define RESET        ""
#endif

////////////////////////////////////////////////////////////
// LOGGING LEVELS

#define LOG_NONE  0
#define LOG_ERROR 1
#define LOG_WARN  2
#define LOG_INFO  3
#define LOG_DEBUG 4
#define LOG_TRACE 5

void
i_log_internal (const char *prefix, const char *color, const char *fmt, ...) PRINTF_ATTR (3, 4);

void i_log_flush (void);

#ifndef I_LOG_LEVEL
#  define I_LOG_LEVEL LOG_DEBUG
#endif

////////////////////////////////////////////////////////////
// LOGGING WRAPPERS

#if defined(NLOG)
#  define SHOULD_LOG_AT(lvl) 0
#else
#  define SHOULD_LOG_AT(lvl) ((I_LOG_LEVEL) >= (lvl))
#endif
#if SHOULD_LOG_AT(LOG_TRACE)
#  define i_log_trace(...)    i_log_internal ("TRACE", BOLD_WHITE, __VA_ARGS__)
#  define i_printf_trace(...) fprintf (stderr, __VA_ARGS__);
#else
#  define i_log_trace(...)    ((void)0)
#  define i_printf_trace(...) ((void)0)
#endif
#if SHOULD_LOG_AT(LOG_DEBUG)
#  define i_log_debug(...)    i_log_internal ("DEBUG", BLUE, __VA_ARGS__)
#  define i_printf_debug(...) fprintf (stderr, __VA_ARGS__)
#else
#  define i_log_debug(...)    ((void)0)
#  define i_printf_debug(...) ((void)0)
#endif
#if SHOULD_LOG_AT(LOG_INFO)
#  define i_log_info(...)    i_log_internal ("INFO", GREEN, __VA_ARGS__)
#  define i_printf_info(...) fprintf (stderr, __VA_ARGS__)
#else
#  define i_log_info(...)    ((void)0)
#  define i_printf_info(...) ((void)0)
#endif
#if SHOULD_LOG_AT(LOG_WARN)
#  define i_log_warn(...)    i_log_internal ("WARN", YELLOW, __VA_ARGS__)
#  define i_printf_warn(...) fprintf (stderr, __VA_ARGS__)
#else
#  define i_log_warn(...)    ((void)0)
#  define i_printf_warn(...) ((void)0)
#endif
#if SHOULD_LOG_AT(LOG_ERROR)
#  define i_log_error(...)    i_log_internal ("ERROR", RED, __VA_ARGS__)
#  define i_printf_error(...) fprintf (stderr, __VA_ARGS__)
#else
#  define i_log_error(...)    ((void)0)
#  define i_printf_error(...) ((void)0)
#endif
// These are always logged
#ifndef NLOG
#  define i_log_assert(...)  i_log_internal ("ASSERT", RED, __VA_ARGS__)
#  define i_log_failure(...) i_log_internal ("FAILURE", BOLD_RED, __VA_ARGS__)
#  define i_log_passed(...)  i_log_internal ("PASSED", BOLD_GREEN, __VA_ARGS__)
#else
#  define i_log_assert(...)  i_log_internal ("ASSERT", RED, __VA_ARGS__)
#  define i_log_failure(...) ((void)0)
#  define i_log_passed(...)  ((void)0)
#endif

// Programatically choose the log level
#define i_log(lvl, ...)                                         \
  do                                                            \
  {                                                             \
    if ((lvl) == LOG_TRACE) { i_log_trace (__VA_ARGS__); }      \
    else if ((lvl) == LOG_DEBUG) { i_log_debug (__VA_ARGS__); } \
    else if ((lvl) == LOG_INFO) { i_log_info (__VA_ARGS__); }   \
    else if ((lvl) == LOG_WARN) { i_log_warn (__VA_ARGS__); }   \
    else if ((lvl) == LOG_ERROR) { i_log_error (__VA_ARGS__); } \
    else if ((lvl) == LOG_NONE) {}                              \
    else                                                        \
    {                                                           \
      UNREACHABLE ();                                           \
    }                                                           \
  }                                                             \
  while (0)

// Print instead of log at a certain logging level
#define i_printf(lvl, ...)                                         \
  do                                                               \
  {                                                                \
    if ((lvl) == LOG_TRACE) { i_printf_trace (__VA_ARGS__); }      \
    else if ((lvl) == LOG_DEBUG) { i_printf_debug (__VA_ARGS__); } \
    else if ((lvl) == LOG_INFO) { i_printf_info (__VA_ARGS__); }   \
    else if ((lvl) == LOG_WARN) { i_printf_warn (__VA_ARGS__); }   \
    else if ((lvl) == LOG_ERROR) { i_printf_error (__VA_ARGS__); } \
    else if ((lvl) == LOG_NONE) {}                                 \
    else                                                           \
    {                                                              \
      UNREACHABLE ();                                              \
    }                                                              \
  }                                                                \
  while (0)

////////////////////////////////////////////////////////////
// ASSERT

#define crash()             \
  do                        \
  {                         \
    *(volatile int *)0 = 1; \
    UNREACHABLE_HINT ();    \
  }                         \
  while (0)

#define UNIMPLEMENTED() UNREACHABLE ()

#define UNREACHABLE() \
  do { crash (); }    \
  while (0)

#ifndef NDEBUG

////////////////////////////////////////////////////////////
/// PANIC

#  define panic(msg)                    \
    do                                  \
    {                                   \
      i_log_error ("PANIC! %s\n", msg); \
      i_log_flush ();                   \
      crash ();                         \
    }                                   \
    while (0)

////////////////////////////////////////////////////////////
/// ASSERT

#  define ASSERT(expr)                                                                   \
    do                                                                                   \
    {                                                                                    \
      if (!(expr))                                                                       \
      {                                                                                  \
        i_log_assert ("%s failed at %s:%d (%s)\n", #expr, __FILE__, __LINE__, __func__); \
        i_log_flush ();                                                                  \
        crash ();                                                                        \
      }                                                                                  \
    }                                                                                    \
    while (0)

#  define DEFINE_DBG_ASSERT(type, name, var, body) \
    HEADER_FUNC void name##_assert__ (const type *var) body

#  define DBG_ASSERT(name, expr) name##_assert__ (expr)

#else

// Throw a compile time error to discourage various debug macros
#  define NOT_FOR_PRODUCTION() // typedef char
                               // PANIC_in_release_mode_is_not_allowed[-1]

// Release doesn't allow these two
#  define panic(msg) NOT_FOR_PRODUCTION ()
#  define ASSERT(expr)

#  define DEFINE_DBG_ASSERT(type, name, var, body)   \
    HEADER_FUNC void name##_assert (const type *var) \
    { (void)var; }

#  define DBG_ASSERT(name, expr) ((void)0)
#endif

////////////////////////////////////////////////////////////
// CONCURRENCY / LATCH

// TODO - (16) ABA problem

////////////////////////////////////////////////////////////
// spin_pause
#if PLATFORM_WINDOWS
#  define spin_pause() YieldProcessor ()
#elif defined(__x86_64__) || defined(__i386__)
#  define spin_pause() __asm__ volatile ("pause" ::: "memory")
#elif defined(__aarch64__) || defined(__arm__)
#  define spin_pause() __asm__ volatile ("yield" ::: "memory")
#else
#  define spin_pause() atomic_signal_fence (memory_order_seq_cst)
#endif

typedef _Atomic (int) latch;

HEADER_FUNC void
latch_init (latch *l)
{ *l = 0; }

/**
 * Returns true if the latch was locked, false otherwise
 */
HEADER_FUNC bool
latch_trylock (latch *l)
{
  int val = 0;

  // Fast path - it's likely that the lock will succeed if
  // l == 0, replace it with 1
  if (likely (atomic_compare_exchange_weak_explicit (
          l,
          &val,
          1,
          memory_order_acquire,
          memory_order_relaxed
      )))
  {
    return true;
  }
  return false;
}

HEADER_FUNC void
latch_lock (latch *l)
{
  int val = 0;

  // Fast path - it's likely that the lock will succeed if
  // l == 0, replace it with 1
  if (likely (atomic_compare_exchange_weak_explicit (
          l,
          &val,
          1,
          memory_order_acquire,
          memory_order_relaxed
      )))
  {
    return;
  }

  do
  {
    do
    {
      // let the CPU breath
      spin_pause ();

      // Load the value of l
      val = atomic_load_explicit (l, memory_order_relaxed);
    }
    // if l != 0: continue
    while (val != 0);
  }

  // Has it changed yet? If not - set it to locked - this risks the ABA problem
  while (!atomic_compare_exchange_weak_explicit (
      l,
      &val,
      1,
      memory_order_acquire,
      memory_order_relaxed
  ));
}

HEADER_FUNC void
latch_unlock (latch *l)
{ atomic_store_explicit (l, 0, memory_order_release); }

////////////////////////////////////////////////////////////
// CONCURRENCY / SPX_LATCH

typedef _Atomic (unsigned int) sx_latch;

#define S_MASK 0x0000FFFFu // [15:0]
#define X      0x00010000u // [16]

#define SLOCKED(val) (val & S_MASK)
#define XLOCKED(val) (val & X)

HEADER_FUNC void
spx_latch_init (sx_latch *l)
{ *l = 0; }

HEADER_FUNC bool
spx_trylock_s (sx_latch *l)
{
  u32 val = atomic_load_explicit (l, memory_order_relaxed);

  if (unlikely (XLOCKED (val))) { return false; }

  return atomic_compare_exchange_strong_explicit (
      l,
      &val,
      val + 1,
      memory_order_acquire,
      memory_order_relaxed
  );
}

HEADER_FUNC void
spx_lock_s (sx_latch *l)
{
  u32 val = atomic_load_explicit (l, memory_order_relaxed);

  while (true)
  {
    // Wait for X to clear before attempting the CAS.
    while (unlikely (XLOCKED (val)))
    {
      spin_pause ();
      val = atomic_load_explicit (l, memory_order_relaxed);
    }

    if (likely (atomic_compare_exchange_weak_explicit (
            l,
            &val,
            val + 1,
            memory_order_acquire,
            memory_order_relaxed
        )))
    {
      return;
    }
  }
}

HEADER_FUNC void
spx_unlock_s (sx_latch *l)
{ atomic_fetch_sub_explicit (l, 1, memory_order_release); }

HEADER_FUNC bool
spx_trylock_x (sx_latch *l)
{
  u32 expected = 0;
  return atomic_compare_exchange_strong_explicit (
      l,
      &expected,
      X,
      memory_order_acquire,
      memory_order_relaxed
  );
}

HEADER_FUNC void
spx_lock_x (sx_latch *l)
{
  u32 val = atomic_load_explicit (l, memory_order_relaxed);
  // Phase 1: claim the X bit.  This blocks new S acquirers
  // (spx_lock_s spins while XLOCKED is set) but does not yet
  // wait for in-flight readers.
  while (true)
  {
    if (likely (!XLOCKED (val)))
    {
      if (atomic_compare_exchange_weak_explicit (
              l,
              &val,
              val | X,
              memory_order_acquire,
              memory_order_relaxed
          ))
      {
        break;
      }
      // CAS failed; val is refreshed, retry without spinning.
      continue;
    }
    // Another writer holds X.  Wait for them to release.
    do
    {
      spin_pause ();
      val = atomic_load_explicit (l, memory_order_relaxed);
    }
    while (XLOCKED (val));
  }
  // Phase 2: drain remaining readers.  No new readers can arrive
  // because XLOCKED is now true.
  while (SLOCKED (atomic_load_explicit (l, memory_order_acquire))) { spin_pause (); }
}

HEADER_FUNC void
spx_unlock_x (sx_latch *l)
{ atomic_store_explicit (l, 0, memory_order_release); }

////////////////////////////////////////////////////////////
// MEMORY / LALLOC

struct lalloc
{
  latch latch;
  u32   used;
  u32   limit;
  u8   *data;
};

#define lalloc_create_from(buf) lalloc_create ((u8 *)buf, sizeof (buf))

struct lalloc lalloc_create (u8 *data, u32 limit);
u32           lalloc_get_state (struct lalloc *l);
void          lalloc_reset_to_state (struct lalloc *l, u32 state);
void         *lmalloc (struct lalloc *a, u32 req, u32 size, error *e);
void         *lcalloc (struct lalloc *a, u32 req, u32 size, error *e);
void          lalloc_reset (struct lalloc *a);

HEADER_FUNC void *
lmalloc_expect (struct lalloc *a, const u32 req, const u32 size)
{
  void *ret = lmalloc (a, req, size, NULL);
  ASSERT (ret);
  return ret;
}

////////////////////////////////////////////////////////////
// MEMORY / SLAB_ALLOC

struct slab;

struct slab_alloc
{
  struct slab *head;
  struct slab *current; // Cache slab with free space (hot path)
  latch        l;
  u32          size;
  u32          cap_per_slab;
};

void slab_alloc_init (struct slab_alloc *dest, u32 size, u32 cap_per_slab);
void slab_alloc_destroy (struct slab_alloc *alloc);

void *slab_alloc_alloc (struct slab_alloc *alloc, error *e);
void  slab_alloc_free (struct slab_alloc *alloc, void *ptr);

////////////////////////////////////////////////////////////
// MEMORY / CHUNK_ALLOC

/// A single chunk of memory in a chunk allocator chain
struct chunk
{
  struct lalloc alloc;  // Base allocator interface for this chunk
  struct chunk *next;   // Next chunk in the linked list, or NULL if tail
  u8            data[]; // Flexible array of chunk-owned bytes
};

/// Configuration settings for a chunk allocator
struct chunk_alloc_settings
{
  u32 max_alloc_size;      // Maximum size of a single allocation in bytes (0 =
                           // unlimited)
  u32 max_total_size;      // Maximum total memory across all chunks in bytes (0 =
                           // unlimited)
  float target_chunk_mult; // Multiplier applied to the requested size when
                           // sizing a new chunk (must be > 1)
  u32 min_chunk_size;      // Minimum size of a newly allocated chunk in bytes
  u32 max_chunk_size;      // Maximum size of a newly allocated chunk in bytes (0 =
                           // unlimited)
  u32 max_chunks;          // Maximum number of chunks that may be allocated (0 =
                           // unlimited)
};

/// A chunk-based arena allocator
struct chunk_alloc
{
  latch                       latch;           // Synchronization latch guarding this allocator
  struct chunk_alloc_settings settings;        // Configuration settings
  struct chunk               *head;            // Head of the chunk linked list
  u32                         num_chunks;      // Current number of allocated chunks
  u32                         total_allocated; // Total bytes allocated across all chunks
  u32                         total_used;      // Total bytes currently in use
};

/// Initializes a chunk allocator with the given settings
void chunk_alloc_create (
    struct chunk_alloc         *dest, // Allocator to initialize
    struct chunk_alloc_settings settings
); // Settings to apply

/// Initializes a chunk allocator with default settings
void chunk_alloc_create_default (struct chunk_alloc *dest); // Allocator to initialize

/// Frees all chunks and resets the allocator to its initial state
void chunk_alloc_free_all (struct chunk_alloc *ca); // Target allocator

/// Resets all chunk usage counters without freeing any memory, keeping chunks
/// available for reuse
void chunk_alloc_reset_all (struct chunk_alloc *ca); // Target allocator

/// Allocates uninitialized memory from the chunk allocator
void *chunk_malloc (
    struct chunk_alloc *ca,   // Target allocator
    u32                 req,  // Requested alignment in bytes
    u32                 size, // Number of bytes to allocate
    error              *e
); // The error object

/// Allocates zero-initialized memory from the chunk allocator
void *chunk_calloc (
    struct chunk_alloc *ca,   // Target allocator
    u32                 req,  // Requested alignment in bytes
    u32                 size, // Number of bytes to allocate
    error              *e
); // The error object

/// Copies memory from an external pointer into the chunk allocator and returns
/// a pointer to the copy
void *chunk_alloc_move_mem (
    struct chunk_alloc *ca,   // Target allocator
    const void         *ptr,  // Source data to copy
    u32                 size, // Number of bytes to copy
    error              *e
); // The error object

////////////////////////////////////////////////////////////
// MEMORY / SERIALIZER

///
/// You may obtain a copy of the License at
///
///
/// Unless required by applicable law or agreed to in writing, software

struct serializer
{
  latch     latch;
  u8       *data;
  u32       dlen;
  const u32 dcap;
};

struct serializer srlizr_create (u8 *data, u32 dcap);

bool srlizr_write (struct serializer *dest, const void *src, u32 len);
#define srlizr_write_expect(dest, src, len)   \
  do                                          \
  {                                           \
    bool ret = srlizr_write (dest, src, len); \
    ASSERT (ret);                             \
  }                                           \
  while (0)

////////////////////////////////////////////////////////////
// MEMORY / DESERIALIZER

///
/// You may obtain a copy of the License at
///
///
/// Unless required by applicable law or agreed to in writing, software

struct deserializer
{
  latch     latch;
  const u8 *data;
  u32       head;
  const u32 dlen;
};

struct deserializer dsrlizr_create (const u8 *data, u32 dlen);

bool dsrlizr_read (void *dest, u32 dlen, struct deserializer *src);
#define dsrlizr_read_expect(dest, dlen, src)   \
  do                                           \
  {                                            \
    bool ret = dsrlizr_read (dest, dlen, src); \
    ASSERT (ret);                              \
  }                                            \
  while (0)

////////////////////////////////////////////////////////////
// MEMORY / MALLOC_PLAN

struct malloc_plan
{
  u32   size;
  u32   blen;
  void *buffer;

  enum
  {
    MP_PLANNING,
    MP_ALLOCING
  } mode;
};

HEADER_FUNC struct malloc_plan
malloc_plan_create (void)
{
  return (struct malloc_plan){
      .size   = 0,
      .blen   = 0,
      .buffer = NULL,
      .mode   = MP_PLANNING,
  };
}

HEADER_FUNC void *
malloc_plan_head (const struct malloc_plan *plan)
{
  switch (plan->mode)
  {
    case MP_PLANNING:
    {
      return NULL;
    }
    case MP_ALLOCING:
    {
      return (u8 *)plan->buffer + plan->blen;
    }
  }
  UNREACHABLE ();
}

// Allocate memory
void *malloc_plan_memcpy (struct malloc_plan *plan, const void *data, u32 len);

// Do the planning -> alloc swap
err_t malloc_plan_alloc (struct malloc_plan *plan, error *e);

////////////////////////////////////////////////////////////
// MEMORY / ALLOC

struct alloc
{
  enum
  {
    AT_LALLOC,
    AT_CHNK_ALLOC,
    AT_MALLOC,
  } type;

  union {
    struct chunk_alloc _calloc;
    struct lalloc      _lalloc;
  };
};

void *alloc_alloc (struct alloc *a, u32 nelem, u32 size, error *e);
void *alloc_calloc (struct alloc *a, u32 nelem, u32 size, error *e);
void  alloc_free (const struct alloc *a, void *data);

////////////////////////////////////////////////////////////
// DS / LLIST

struct llnode
{
  struct llnode *next;
};

HEADER_FUNC void
llnode_init (struct llnode *n)
{ n->next = NULL; }

HEADER_FUNC u32
list_length (const struct llnode *head)
{
  u32 len = 0;
  for (const struct llnode *cur = head; cur; cur = cur->next) { len++; }
  return len;
}

HEADER_FUNC void
list_push (struct llnode **head, struct llnode *n)
{
  n->next = *head;
  *head   = n;
}

HEADER_FUNC void
list_append (struct llnode **head, struct llnode *n)
{
  n->next = NULL;
  if (!*head) { *head = n; }
  else
  {
    struct llnode *cur = *head;
    while (cur->next) { cur = cur->next; }
    cur->next = n;
  }
}

HEADER_FUNC struct llnode *
list_pop (struct llnode **head)
{
  if (!*head) { return NULL; }

  struct llnode *n = *head;
  *head            = n->next;
  n->next          = NULL;

  return n;
}

HEADER_FUNC struct llnode *
list_find (
    u32                 *didx,
    struct llnode       *head,
    const struct llnode *node,
    bool (*eq) (const struct llnode *left, const struct llnode *right)
)
{
  *didx = 0;
  for (struct llnode *iter = (head); iter; iter = iter->next, *didx = *didx + 1)
  {
    if (eq (iter, node)) { return iter; }
  }
  return NULL;
}

HEADER_FUNC void
list_remove (struct llnode **head, struct llnode *n)
{
  struct llnode **cur = head;
  while (*cur && *cur != n) { cur = &(*cur)->next; }
  if (*cur)
  {
    *cur    = n->next;
    n->next = NULL;
  }
}

HEADER_FUNC struct llnode *
llnode_get_n (struct llnode *head, const u32 index)
{
  struct llnode *cur = head;
  for (u32 i = 0; cur && i < index; ++i) { cur = cur->next; }
  return cur;
}

// Iterate over list
#define LLIST_FOR_EACH(head, iter) for (llnode *iter = (head); iter; iter = iter->next)

////////////////////////////////////////////////////////////
// DS / STRING

/// A length-prefixed, non-owning string view
struct string
{
  u32         len;  // Number of bytes in data (not necessarily null-terminated)
  const char *data; // Pointer to the string bytes
};

struct string strfcstr (const char *cstr);

u64 line_length (const char *buf, u64 max);

int strings_all_unique (const struct string *strs, u32 count);

bool string_equal (const struct string s1, const struct string s2);

struct string
string_plus (const struct string left, const struct string right, struct lalloc *alloc, error *e);

const struct string *
strings_are_disjoint (const struct string *left, u32 llen, const struct string *right, u32 rlen);

bool string_contains (const struct string superset, const struct string subset);

bool string_less_string (const struct string left, const struct string right);

bool string_greater_string (const struct string left, const struct string right);

bool string_less_equal_string (const struct string left, const struct string right);

bool string_greater_equal_string (const struct string left, const struct string right);

err_t string_copy (struct string *dest, struct string src, error *e);

////////////////////////////////////////////////////////////
// DS / STRIDE

/// A resolved, internal stride descriptor for tree operations
struct stride
{
  u64 start;  // Byte offset at which to begin
  u64 stride; // Bytes to advance between successive elements
  u64 nelems; // Number of elements to access
};

/// A user-facing stride descriptor using signed, Python-style slice semantics
struct user_stride
{
  i64 start;   // Start index (negative values index from the end)
  i64 step;    // Step between elements (negative not yet supported)
  i64 stop;    // Exclusive stop index (negative values index from the end)
  int present; // Non-zero if this stride was explicitly provided by the user
};

#define STOP_PRESENT  (1 << 0)
#define STEP_PRESENT  (1 << 1)
#define START_PRESENT (1 << 2)
#define COLON_PRESENT (1 << 3)

#define USER_STRIDE_ALL \
  ((struct user_stride){.start = 0, .step = 1, .stop = 0, .present = STEP_PRESENT | START_PRESENT})

bool  ustride_equal (struct user_stride left, struct user_stride right);
void  stride_resolve_expect (struct stride *dest, struct user_stride src, u64 arrlen);
err_t stride_resolve (struct stride *dest, struct user_stride src, u64 arrlen, error *e);

////////////////////////////////////////////////////////////
/// Small Constructors

HEADER_FUNC struct user_stride
ustride_single (const i64 start)
{
  return (struct user_stride){
      .start   = start,
      .present = START_PRESENT,
  };
}

HEADER_FUNC struct user_stride
ustride012 (const i64 start, const i64 step, const i64 stop)
{
  return (struct user_stride){
      .start   = start,
      .step    = step,
      .stop    = stop,
      .present = STOP_PRESENT | STEP_PRESENT | START_PRESENT | COLON_PRESENT,
  };
}

HEADER_FUNC struct user_stride
ustride01 (const i64 start, const i64 step)
{
  return (struct user_stride){
      .start   = start,
      .step    = step,
      .present = STEP_PRESENT | START_PRESENT | COLON_PRESENT,
  };
}

HEADER_FUNC struct user_stride
ustride0 (const i64 start)
{
  return (struct user_stride){
      .start   = start,
      .present = START_PRESENT | COLON_PRESENT,
  };
}

HEADER_FUNC struct user_stride
ustride12 (const i64 step, const i64 stop)
{
  return (struct user_stride){
      .step    = step,
      .stop    = stop,
      .present = STOP_PRESENT | STEP_PRESENT | COLON_PRESENT,
  };
}

HEADER_FUNC struct user_stride
ustride1 (const i64 step)
{
  return (struct user_stride){
      .step    = step,
      .present = STEP_PRESENT | START_PRESENT | COLON_PRESENT,
  };
}

HEADER_FUNC struct user_stride
ustride2 (const i64 stop)
{
  return (struct user_stride){
      .stop    = stop,
      .present = STOP_PRESENT | COLON_PRESENT,
  };
}

HEADER_FUNC struct user_stride
ustride (void)
{
  return (struct user_stride){
      .present = COLON_PRESENT,
  };
}

HEADER_FUNC struct user_stride
usfrms (const struct stride str)
{ return ustride012 (str.start, str.stride, str.start + str.stride * str.nelems); }

struct multi_user_stride
{
  struct user_stride *strides;
  u32                 len;
};

struct mus_llnode
{
  struct user_stride stride;
  struct llnode      link;
};

struct mus_builder
{
  struct llnode      *head;
  struct chunk_alloc *temp;
  struct chunk_alloc *persistent;
};

void
musb_create (struct mus_builder *dest, struct chunk_alloc *temp, struct chunk_alloc *persistent);

err_t musb_accept_key (struct mus_builder *eb, struct user_stride stride, error *e);
err_t musb_build (struct multi_user_stride *persistent, const struct mus_builder *eb, error *e);

////////////////////////////////////////////////////////////
// DS / DATA_WRITER

/// Function pointer type for inserting data at a byte offset
typedef err_t (*insert_func) (
    void       *ctx,  // Caller-provided context pointer
    u32         ofst, // Byte offset at which to insert
    const void *src,  // Source data to insert
    u32         slen, // Number of bytes to insert
    error      *e
); // The error object

/// Function pointer type for reading elements from a strided range
typedef i64 (*read_func) (
    void         *ctx,  // Caller-provided context pointer
    struct stride str,  // Stride descriptor defining start, step, and element count
    u32           size, // Size of each element in bytes
    void         *dest, // Destination buffer to receive the data
    error        *e
); // The error object

/// Function pointer type for writing elements into a strided range
typedef i64 (*write_func) (
    void         *ctx,  // Caller-provided context pointer
    struct stride str,  // Stride descriptor defining start, step, and element count
    u32           size, // Size of each element in bytes
    const void   *src,  // Source data to write
    error        *e
); // The error object

/// Function pointer type for removing elements from a strided range
typedef i64 (*remove_func) (
    void         *ctx,  // Caller-provided context pointer
    struct stride str,  // Stride descriptor defining start, step, and element count
    u32           size, // Size of each element in bytes
    void         *dest, // Optional destination buffer to capture removed data (NULL to
                        // discard)
    error *e
); // The error object

/// Function pointer type for querying the total number of bytes in the data
/// source
typedef i64 (*get_len_func) (
    void  *ctx, // Caller-provided context pointer
    error *e
); // The error object

/// The full set of function pointers that back a data_writer
struct data_writer_functions
{
  insert_func  insert; // Insert bytes at an offset
  read_func    read;   // Read elements from a strided range
  write_func   write;  // Overwrite elements in a strided range
  remove_func  remove; // Remove elements from a strided range
  get_len_func getlen; // Query the total byte length
};

/// A virtual data source/sink pairing a function table with its context
struct data_writer
{
  struct data_writer_functions functions; // Vtable of data operations
  void                        *ctx;       // Opaque context passed to every function call
};

////////////////////////////////////////////////////////////
// DS / HT_MODELS

// Hash table insert result
typedef enum
{
  HTIR_SUCCESS,
  HTIR_EXISTS,
  HTIR_FULL,
} hti_res;

// Hash table access result
typedef enum
{
  HTAR_SUCCESS,
  HTAR_DOESNT_EXIST,
} hta_res;

////////////////////////////////////////////////////////////
// DS / BLOCK_ARRAY

/**
 * @class block
 * @brief A block is an individual node in a block array that holds
 * data array that is sized [cap_per_node] of it's parent and it has a
 * length that is how full it is
 */
struct block
{
  struct block *next;
  struct block *prev;
  u32           len;
  u8            data[];
};

/**
 * @class block_array
 * @brief A block array is a linked listed of data blocks that
 * represent byte data. Each block has capacity [cap_per_node] and
 * links to their neighbors
 */
struct block_array
{
  struct slab_alloc block_alloc; // Allocates blocks
  u32               cap_per_node;
  struct block     *head;

  u32 tlen;   // length of tail
  u8  tail[]; // A temporary buffer used for storing the right half of a block on
              // insert
};

struct block_array *block_array_create (u32 cap_per_node, error *e);
void                block_array_free (struct block_array *r);

err_t block_array_insert (struct block_array *r, u32 ofst, const void *src, u32 slen, error *e);

u64 block_array_read (const struct block_array *r, struct stride str, u32 size, void *dest);

u64 block_array_write (const struct block_array *r, struct stride str, u32 size, const void *src);

i64 block_array_remove (struct block_array *r, struct stride str, u32 size, void *dest, error *e);

u64 block_array_getlen (const struct block_array *r);

// Array accessor pattern
void *block_array_get (struct block_array *r, u64 idx);

void block_array_set (struct block_array *r, u64 idx, const void *data, u32 dlen);

void block_array_data_writer (struct data_writer *dest, struct block_array *arr);

////////////////////////////////////////////////////////////
// DS / CBUFFER

/// A cbuffer wraps a fixed-size byte array and maintains head/tail pointers
/// to implement a FIFO queue. No heap allocation is performed - the caller
/// owns the backing memory and decides its lifetime.
///
/// Layout:
/// @code
/// [-------------++++++++++++++++------------]
///  ^tail          ^head
/// @endcode
/// Data occupies [tail, head). When the buffer is full, head == tail and
/// isfull is true.

/// @brief Circular buffer handle.
///
/// Initialize with cbuffer_create() or cbuffer_create_with(). All fields are
/// managed internally - do not modify them directly.
struct cbuffer
{
  u8  *data;   // Pointer to the caller-supplied backing array
  u32  cap;    // Total capacity of the backing array in bytes
  u32  head;   // Write cursor - next byte is written here
  u32  tail;   // Read cursor - next byte is read from here
  bool isfull; // True when head == tail and the buffer is full (not empty)
};

/// @brief Creates a cbuffer over an existing array with zero initial length.
/// @param data  Pointer to the backing array
/// @param cap   Size of the backing array in bytes
/// @return      Initialized cbuffer with head == tail == 0 and isfull == false
#define cbuffer_create_from(data) cbuffer_create (data, sizeof data)

/// @brief Creates a cbuffer over an existing array, treating it as full.
/// @param data  Pointer to the backing array (already filled)
#define cbuffer_create_full_from(data) cbuffer_create_with (data, sizeof data, sizeof data)

/// @brief Creates a cbuffer from a C string, treating the string bytes as data.
/// @param cstr  Null-terminated string to wrap (length is strlen(cstr))
#define cbuffer_create_from_cstr(cstr) cbuffer_create_with (cstr, strlen (cstr), strlen (cstr))

/// @brief Creates an empty cbuffer over a caller-supplied array.
/// @param data  Pointer to the backing array
/// @param cap   Size of the backing array in bytes
/// @return      Initialized cbuffer with no data
struct cbuffer cbuffer_create (void *data, u32 cap);

/// @brief Creates a cbuffer with an initial data length already present.
/// @param data  Pointer to the backing array (first @p len bytes are considered
/// data)
/// @param cap   Total size of the backing array in bytes
/// @param len   Number of bytes already present in the buffer
/// @return      Initialized cbuffer with head advanced by @p len
struct cbuffer cbuffer_create_with (void *data, u32 cap, u32 len);

////////////////////////////////////////////////////////////
// Utils

/// @brief Returns the number of bytes currently in the buffer.
/// @param b  The cbuffer (must not be NULL)
/// @return   Number of bytes available to read
HEADER_FUNC u32
cbuffer_len (const struct cbuffer *b)
{
  u32 len;
  if (b->isfull) { len = b->cap; }
  else if (b->head >= b->tail) { len = b->head - b->tail; }
  else
  {
    len = b->cap - (b->tail - b->head);
  }
  return len;
}

DEFINE_DBG_ASSERT (struct cbuffer, cbuffer, b, {
  ASSERT (b);
  ASSERT (b->cap > 0);
  ASSERT (b->data);
  if (b->isfull) { ASSERT (b->tail == b->head); }
  ASSERT (cbuffer_len (b) <= b->cap);
})

/// @brief Returns true if the buffer contains no data.
/// @param b  The cbuffer (must not be NULL)
HEADER_FUNC bool
cbuffer_isempty (const struct cbuffer *b)
{
  DBG_ASSERT (cbuffer, b);
  return (!b->isfull && b->head == b->tail);
}

/// @brief Returns the number of elements of @p size bytes currently in the
/// buffer.
/// @param b     The cbuffer
/// @param size  Element size in bytes - must evenly divide the current length
/// @return      Number of whole elements present
HEADER_FUNC u32
cbuffer_slen (const struct cbuffer *b, const u32 size)
{
  const u32 len = cbuffer_len (b);
  ASSERT (len % size == 0);
  return len / size;
}

/// @brief Returns the number of bytes available for writing.
/// @param b  The cbuffer (must not be NULL)
/// @return   Bytes of free space remaining
HEADER_FUNC u32
cbuffer_avail (const struct cbuffer *b)
{
  DBG_ASSERT (cbuffer, b);
  const u32 len = cbuffer_len (b);
  ASSERT (b->cap >= len);
  return b->cap - len;
}

/// @brief Returns the number of elements of @p size bytes that can still be
/// written.
/// @param b     The cbuffer (must not be NULL)
/// @param size  Element size in bytes - must evenly divide the current length
/// @return      Number of whole elements that fit in the remaining space
HEADER_FUNC u32
cbuffer_savail (const struct cbuffer *b, const u32 size)
{
  DBG_ASSERT (cbuffer, b);
  const u32 len = cbuffer_len (b);
  ASSERT (b->cap >= len);
  ASSERT (len % size == 0);
  return (b->cap - len) / size;
}

/// @brief Resets the buffer to empty, discarding all data.
/// @param b  The cbuffer to reset
void cbuffer_discard_all (struct cbuffer *b);

/// @brief Returns a bytes view of the next contiguous free region in the
/// backing array.
///
/// Use this to write directly into the buffer and then call cbuffer_fakewrite()
/// to advance the head pointer.
///
/// @param b  The cbuffer
/// @return   Bytes pointing to the next available contiguous free region
struct bytes cbuffer_get_next_avail_bytes (const struct cbuffer *b);

/// @brief Returns a bytes view of the next contiguous data region in the
/// backing array.
///
/// Use this to read directly from the buffer and then call cbuffer_fakeread()
/// to advance the tail pointer.
///
/// @param b  The cbuffer
/// @return   Bytes pointing to the next available contiguous data region
struct bytes cbuffer_get_next_data_bytes (const struct cbuffer *b);

/// @brief Advances the tail pointer by @p bytes, as if that many bytes were
/// read.
///
/// Does not copy data anywhere - use after consuming bytes obtained from
/// cbuffer_get_next_data_bytes() directly.
///
/// @param b      The cbuffer
/// @param bytes  Number of bytes to consume
void cbuffer_fakeread (struct cbuffer *b, u32 bytes);

/// @brief Advances the head pointer by @p bytes, as if that many bytes were
/// written.
///
/// Does not copy data - use after writing directly into the region returned by
/// cbuffer_get_next_avail_bytes().
///
/// @param b      The cbuffer
/// @param bytes  Number of bytes to mark as written
void cbuffer_fakewrite (struct cbuffer *b, u32 bytes);

////////////////////////////////////////////////////////////
// Raw Read / Write

/// @brief Reads up to @p n elements of @p size bytes from the buffer into @p
/// dest.
/// @param dest  Destination buffer to receive the data
/// @param size  Size of each element in bytes
/// @param n     Maximum number of elements to read
/// @param b     Source cbuffer
/// @return      Number of elements actually read (may be less than @p n if
/// buffer runs dry)
u32 cbuffer_read (void *dest, u32 size, u32 n, struct cbuffer *b);

/// @brief Copies up to @p n elements of @p size bytes from the buffer into @p
/// dest without consuming them.
/// @param dest  Destination buffer to receive the data
/// @param size  Size of each element in bytes
/// @param n     Maximum number of elements to copy
/// @param b     Source cbuffer (unchanged)
/// @return      Number of elements actually copied
u32 cbuffer_copy (void *dest, u32 size, u32 n, const struct cbuffer *b);

/// @brief Writes up to @p n elements of @p size bytes from @p src into the
/// buffer.
/// @param src   Source data to write
/// @param size  Size of each element in bytes
/// @param n     Number of elements to write
/// @param b     Destination cbuffer
/// @return      Number of elements actually written (may be less than @p n if
/// buffer is full)
u32 cbuffer_write (const void *src, u32 size, u32 n, struct cbuffer *b);

/// @brief Reads exactly @p n elements - ASSERTs if the buffer does not have
/// enough data.
#define cbuffer_read_expect(dest, size, n, b)     \
  do                                              \
  {                                               \
    u32 __read = cbuffer_read (dest, size, n, b); \
    ASSERT (__read == n);                         \
  }                                               \
  while (0)

/// @brief Writes exactly @p n elements - ASSERTs if the buffer does not have
/// enough space.
#define cbuffer_write_expect(src, size, n, b)        \
  do                                                 \
  {                                                  \
    u32 __written = cbuffer_write (src, size, n, b); \
    ASSERT (__written == n);                         \
  }                                                  \
  while (0)

////////////////////////////////////////////////////////////
// CBuffer to CBuffer

/// @brief Moves up to @p n elements of @p size bytes from @p src into @p dest,
/// consuming from @p src.
/// @param dest  Destination cbuffer
/// @param size  Element size in bytes
/// @param n     Maximum number of elements to move
/// @param src   Source cbuffer - consumed bytes are removed
/// @return      Number of elements actually moved
u32 cbuffer_cbuffer_move (struct cbuffer *dest, u32 size, u32 n, struct cbuffer *src);

/// @brief Copies up to @p n elements of @p size bytes from @p src into @p dest
/// without consuming @p src.
/// @param dest  Destination cbuffer
/// @param size  Element size in bytes
/// @param n     Maximum number of elements to copy
/// @param src   Source cbuffer (unchanged)
/// @return      Number of elements actually copied
u32 cbuffer_cbuffer_copy (struct cbuffer *dest, u32 size, u32 n, const struct cbuffer *src);

/// @brief Moves all available bytes from @p src into @p dest.
#define cbuffer_cbuffer_move_max(dest, src) cbuffer_cbuffer_move (dest, 1, cbuffer_len (src), src)

/// @brief Copies all available bytes from @p src into @p dest without consuming
/// @p src.
#define cbuffer_cbuffer_copy_max(dest, src) cbuffer_cbuffer_copy (dest, 1, cbuffer_len (src), src)

////////////////////////////////////////////////////////////
// IO Read / Write

/// @brief Stage 1 of a non-blocking write to a file: initiates the write from
/// contiguous data.
///
/// Returns the number of bytes written or a negative error code. Does not
/// advance the tail - call cbuffer_write_to_file_2() after a successful stage 1
/// to consume the bytes.
///
/// @param dest  Destination file
/// @param b     Source cbuffer
/// @param len   Maximum number of bytes to write
/// @param e     Error object
/// @return      Bytes written (>= 0) or negative error code
i32 cbuffer_write_to_file_1 (i_file *dest, const struct cbuffer *b, u32 len, error *e);

/// @brief Stage 1 write that ASSERTs the full @p len bytes were written.
err_t cbuffer_write_to_file_1_expect (i_file *dest, const struct cbuffer *b, u32 len, error *e);

/// @brief Stage 2 of a non-blocking write: advances the tail by @p nwritten
/// bytes.
/// @param b        The cbuffer to consume from
/// @param nwritten Number of bytes confirmed written by stage 1
void cbuffer_write_to_file_2 (struct cbuffer *b, u32 nwritten);

/// @brief Writes up to @p len bytes from the buffer to a file in one call
/// (stages 1 + 2).
/// @param dest  Destination file
/// @param b     Source cbuffer - consumed bytes are removed
/// @param len   Maximum number of bytes to write
/// @param e     Error object
/// @return      Bytes written (>= 0) or negative error code
i32 cbuffer_write_to_file (i_file *dest, struct cbuffer *b, u32 len, error *e);

/// @brief Stage 1 of a non-blocking read from a file: fills contiguous free
/// space.
///
/// Returns the number of bytes read or a negative error code. Does not advance
/// the head - call cbuffer_read_from_file_2() after a successful stage 1 to
/// commit the bytes.
///
/// @param src  Source file
/// @param b    Destination cbuffer
/// @param len  Maximum number of bytes to read
/// @param e    Error object
/// @return     Bytes read (>= 0) or negative error code
i32 cbuffer_read_from_file_1 (i_file *src, const struct cbuffer *b, u32 len, error *e);

/// @brief Stage 1 read that ASSERTs the full @p len bytes were read.
err_t cbuffer_read_from_file_1_expect (i_file *src, const struct cbuffer *b, u32 len, error *e);

/// @brief Stage 2 of a non-blocking read: advances the head by @p nread bytes.
/// @param b      The cbuffer to commit into
/// @param nread  Number of bytes confirmed read by stage 1
void cbuffer_read_from_file_2 (struct cbuffer *b, u32 nread);

/// @brief Reads up to @p len bytes from a file into the buffer in one call
/// (stages 1 + 2).
/// @param src  Source file
/// @param b    Destination cbuffer - head is advanced by bytes read
/// @param len  Maximum number of bytes to read
/// @param e    Error object
/// @return     Bytes read (>= 0) or negative error code
i32 cbuffer_read_from_file (i_file *src, struct cbuffer *b, u32 len, error *e);

////////////////////////////////////////////////////////////
// Single Element Read / Write

/// @brief Reads the element at logical index @p idx without consuming it.
/// @param dest  Destination buffer of at least @p size bytes
/// @param size  Element size in bytes
/// @param idx   Zero-based logical index from the tail
/// @param b     The cbuffer
/// @return      true if the index was in range and the element was copied,
/// false otherwise
bool cbuffer_get (void *dest, u32 size, u32 idx, const struct cbuffer *b);

/// @brief Appends one element of @p size bytes to the back (head side) of the
/// buffer.
/// @param src   Source element
/// @param size  Element size in bytes
/// @param b     The cbuffer
/// @return      true on success, false if the buffer is full
bool cbuffer_push_back (const void *src, u32 size, struct cbuffer *b);

/// @brief Prepends one element of @p size bytes to the front (tail side) of the
/// buffer.
/// @param src   Source element
/// @param size  Element size in bytes
/// @param b     The cbuffer
/// @return      true on success, false if the buffer is full
bool cbuffer_push_front (const void *src, u32 size, struct cbuffer *b);

/// @brief Removes and returns the element at the back (head side) of the
/// buffer.
/// @param dest  Destination buffer of at least @p size bytes
/// @param size  Element size in bytes
/// @param b     The cbuffer
/// @return      true on success, false if the buffer is empty
bool cbuffer_pop_back (void *dest, u32 size, struct cbuffer *b);

/// @brief Removes and returns the element at the front (tail side) of the
/// buffer.
/// @param dest  Destination buffer of at least @p size bytes
/// @param size  Element size in bytes
/// @param b     The cbuffer
/// @return      true on success, false if the buffer is empty
bool cbuffer_pop_front (void *dest, u32 size, struct cbuffer *b);

/// @brief Copies the element at the back without removing it.
/// @param dest  Destination buffer of at least @p size bytes
/// @param size  Element size in bytes
/// @param b     The cbuffer
/// @return      true on success, false if the buffer is empty
bool cbuffer_peek_back (void *dest, u32 size, const struct cbuffer *b);

/// @brief Copies the element at the front without removing it.
/// @param dest  Destination buffer of at least @p size bytes
/// @param size  Element size in bytes
/// @param b     The cbuffer
/// @return      true on success, false if the buffer is empty
bool cbuffer_peek_front (void *dest, u32 size, const struct cbuffer *b);

/// @brief cbuffer_push_back() that ASSERTs on failure.
#define cbuffer_push_back_expect(src, size, b)     \
  do                                               \
  {                                                \
    bool __ret = cbuffer_push_back (src, size, b); \
    ASSERT (__ret);                                \
  }                                                \
  while (0)

/// @brief cbuffer_push_front() that ASSERTs on failure.
#define cbuffer_push_front_expect(src, size, b)     \
  do                                                \
  {                                                 \
    bool __ret = cbuffer_push_front (src, size, b); \
    ASSERT (__ret);                                 \
  }                                                 \
  while (0)

/// @brief cbuffer_pop_back() that ASSERTs on failure.
#define cbuffer_pop_back_expect(dest, size, b)     \
  do                                               \
  {                                                \
    bool __ret = cbuffer_pop_back (dest, size, b); \
    ASSERT (__ret);                                \
  }                                                \
  while (0)

/// @brief cbuffer_pop_front() that ASSERTs on failure.
#define cbuffer_pop_front_expect(dest, size, b)     \
  do                                                \
  {                                                 \
    bool __ret = cbuffer_pop_front (dest, size, b); \
    ASSERT (__ret);                                 \
  }                                                 \
  while (0)

/// @brief cbuffer_peek_back() that ASSERTs on failure.
#define cbuffer_peek_back_expect(dest, size, b)     \
  do                                                \
  {                                                 \
    bool __ret = cbuffer_peek_back (dest, size, b); \
    ASSERT (__ret);                                 \
  }                                                 \
  while (0)

/// @brief cbuffer_peek_front() that ASSERTs on failure.
#define cbuffer_peek_front_expect(dest, size, b)     \
  do                                                 \
  {                                                  \
    bool __ret = cbuffer_peek_front (dest, size, b); \
    ASSERT (__ret);                                  \
  }                                                  \
  while (0)

/// @brief Pushes a single byte value to the back - ASSERTs on failure.
/// @param src  Byte value to push (not a pointer)
/// @param b    The cbuffer
#define cbuffer_pushb_back_expect(src, b)         \
  do                                              \
  {                                               \
    u8   _src  = src;                             \
    bool __ret = cbuffer_push_back (&_src, 1, b); \
    ASSERT (__ret);                               \
  }                                               \
  while (0)

/// @brief Pushes a single byte value to the front - ASSERTs on failure.
/// @param src  Byte value to push (not a pointer)
/// @param b    The cbuffer
#define cbuffer_pushb_front_expect(src, b)         \
  do                                               \
  {                                                \
    u8   _src  = src;                              \
    bool __ret = cbuffer_push_front (&_src, 1, b); \
    ASSERT (__ret);                                \
  }                                                \
  while (0)

////////////////////////////////////////////////////////////
// DS / DBL_BUFFER

/// A heap-allocated buffer that doubles in capacity when exhausted
struct dbl_buffer
{
  latch latch;     // Synchronization latch guarding this buffer
  void *data;      // Pointer to the underlying heap allocation
  u32   size;      // Size of each element in bytes
  u32   nelem_cap; // Maximum number of elements the current allocation can hold
  u32   nelem;     // Number of elements currently in use
};

/// Creates a double buffer with a given element size and initial capacity
err_t dblb_create (
    struct dbl_buffer *dest,        // Buffer to initialize
    u32                size,        // Size of each element in bytes
    u32                initial_cap, // Initial element capacity to allocate
    error             *e
); // The error object

/// Appends elements to the buffer, doubling capacity if necessary
err_t dblb_append (
    struct dbl_buffer *d,     // Target buffer
    const void        *data,  // Source elements to append
    u32                nelem, // Number of elements to append
    error             *e
); // The error object

/// Ensures the buffer has room for at least nelem additional elements,
/// reallocating if necessary
err_t dblb_ensure_space (
    struct dbl_buffer *d,     // Target buffer
    u32                nelem, // Number of additional elements to reserve space for
    error             *e
); // The error object

/// Reserves space for nelem elements at the end of the buffer and returns a
/// pointer to that region
///
/// The returned pointer is valid until the next reallocation. The caller is
/// responsible for writing valid data into the region before further mutations.
void *dblb_append_alloc (
    struct dbl_buffer *d,     // Target buffer
    u32                nelem, // Number of elements to reserve
    error             *e
); // The error object

/// Frees all memory owned by the buffer
void dblb_free (struct dbl_buffer *d); // Target buffer

////////////////////////////////////////////////////////////
// DS / EXT_ARRAY

struct ext_array
{
  u8 *data;
  u32 len;
  u32 cap;
};

struct ext_array ext_array_create (void);
void             ext_array_free (struct ext_array *r);

i64 ext_array_insert (struct ext_array *r, u32 ofst, const void *src, u32 slen, error *e);
i64 ext_array_read (const struct ext_array *r, struct stride str, u32 size, void *dest, error *e);
i64
ext_array_write (const struct ext_array *r, struct stride str, u32 size, const void *src, error *e);
i64 ext_array_remove (struct ext_array *r, struct stride str, u32 size, void *dest, error *e);
u64 ext_array_get_len (const struct ext_array *r);

void ext_array_data_writer (struct data_writer *dest, struct ext_array *arr);

////////////////////////////////////////////////////////////
// DS / HASH_TABLE

struct hnode
{
  struct hnode *next;
  u32           hcode;
};

HEADER_FUNC void
hnode_init (struct hnode *dest, const u32 hcode)
{
  dest->hcode = hcode;
  dest->next  = NULL;
}

struct htable
{
  u32           cap;
  u32           size;
  latch         latch;
  struct hnode *table[];
};

struct htable *htable_create (u32 n, error *e);
void           htable_free (struct htable *t);

void           htable_insert (struct htable *t, struct hnode *node);
struct hnode **htable_lookup (
    struct htable      *t,
    const struct hnode *key,
    bool (*eq) (const struct hnode *, const struct hnode *)
);
struct hnode  *htable_delete (struct htable *t, struct hnode **from);
struct hnode **htable_random (struct htable *t);

// Simple getters
HEADER_FUNC u32
htable_size (const struct htable *t)
{ return t->size; }

// Iterator
void
htable_foreach (const struct htable *t, void (*action) (struct hnode *v, void *ctx), void *ctx);

////////////////////////////////////////////////////////////
// DS / STREAM

/*
 * A polymorphic byte-oriented I/O interface used throughout NumStore.
 *
 * A stream wraps a (pull, push, close) vtable
 * The `done` flag signals end-of-data; a stream sets it via
 * stream_finish() when it has no more bytes to produce or accept.  Callers
 * test it with stream_isdone() to decide when to stop reading or writing.
 *
 *   stream_bread(dest, size, n, src)  - pull up to n elements of [size] bytes
 *                                       from src into the dest buffer.
 *   stream_bwrite(buf, size, n, dest) - push n elements of [size] bytes from
 *                                       buf into dest.
 *   stream_read(dest, size, n, src)   - stream-to-stream copy.
 *
 * All three return the number of elements transferred (>= 0) or a negative
 * error code.  A return value smaller than n does not indicate an error;
 * the caller should check stream_isdone() to distinguish short-read from
 * error.
 *
 * Concrete stream implementations included here:
 *   stream_ibuf    - pulls from a fixed const byte buffer (read source).
 *   stream_obuf    - pushes into a fixed mutable byte buffer (write sink).
 *   stream_sink    - discards all bytes written to it (null sink).
 *   stream_opsink  - applies a callback to each element pushed.
 *   stream_limit   - wraps another stream and enforces a byte limit.
 */
struct stream;

/// Function pointer type for pulling bytes out of a stream into a caller buffer
typedef i32 (*stream_pull_fn) (
    struct stream *s,    // The stream being read
    void          *ctx,  // Implementation-defined context
    void          *buf,  // Destination buffer to receive the data
    u32            size, // Size of each element in bytes
    u32            n,    // Maximum number of elements to pull
    error         *e
); // The error object

/// Function pointer type for pushing bytes from a caller buffer into a stream
typedef i32 (*stream_push_fn) (
    struct stream *s,    // The stream being written
    void          *ctx,  // Implementation-defined context
    const void    *buf,  // Source buffer containing the data to push
    u32            size, // Size of each element in bytes
    u32            n,    // Number of elements to push
    error         *e
); // The error object

/// Function pointer type for releasing any resources held by a stream
/// implementation
typedef void (*stream_close_fn) (void *ctx); // Implementation-defined context

/// Vtable of operations backing a stream
struct stream_ops
{
  stream_pull_fn  pull;  // Pull bytes out of the stream (may be NULL for write-only streams)
  stream_push_fn  push;  // Push bytes into the stream (may be NULL for read-only streams)
  stream_close_fn close; // Release resources held by the stream (may be NULL)
};

/// A polymorphic byte-oriented I/O stream
struct stream
{
  const struct stream_ops *ops;  // Vtable of stream operations
  void                    *ctx;  // Opaque context passed to every vtable call
  atomic_int               done; // Non-zero once the stream has no more data to produce or accept
};

/// Initializes a stream with a given vtable and context
void stream_init (
    struct stream           *s,   // Stream to initialize
    const struct stream_ops *ops, // Vtable to attach
    void                    *ctx
); // Opaque context to attach

/// Calls the stream's close function and releases any implementation resources
void stream_close (const struct stream *s); // Stream to close

/// Marks a stream as done, signaling to callers that no more data will be
/// produced or accepted
void stream_finish (struct stream *s); // Stream to mark done

/// Returns true if the stream has been marked done via stream_finish()
bool stream_isdone (const struct stream *s); // Stream to test

/// Copies up to n elements of size bytes from src to dest via their stream
/// interfaces
///
/// Returns the number of elements transferred (>= 0) or a negative error code.
/// A return smaller than n is not an error; check stream_isdone() on src to
/// distinguish.
i32 stream_read (
    struct stream *dest, // Destination stream to push into
    u32            size, // Size of each element in bytes
    u32            n,    // Maximum number of elements to transfer
    struct stream *src,  // Source stream to pull from
    error         *e
); // The error object

/// Pulls up to n elements of size bytes from src into a raw buffer
///
/// Returns the number of elements transferred (>= 0) or a negative error code.
/// A return smaller than n is not an error; check stream_isdone() on src to
/// distinguish.
i32 stream_bread (
    void          *dest, // Destination buffer to receive the data
    u32            size, // Size of each element in bytes
    u32            n,    // Maximum number of elements to pull
    struct stream *src,  // Source stream to pull from
    error         *e
); // The error object

/// Pushes n elements of size bytes from a raw buffer into dest
///
/// Returns the number of elements transferred (>= 0) or a negative error code.
i32 stream_bwrite (
    const void    *buf,  // Source buffer containing the data to push
    u32            size, // Size of each element in bytes
    u32            n,    // Number of elements to push
    struct stream *dest, // Destination stream to push into
    error         *e
); // The error object

////////////////////////////////////////////////////////////
/// Special Streams

/// Context for a read-only stream backed by a fixed const byte buffer
struct stream_ibuf_ctx
{
  const u8 *buf;  // Pointer to the source buffer
  u32       size; // Total number of bytes in buf
  u32       pos;  // Current read position in bytes
};

/// Context for a write-only stream that writes into a fixed mutable byte buffer
struct stream_obuf_ctx
{
  u8 *buf; // Pointer to the destination buffer
  u32 cap; // Total capacity of buf in bytes
  u32 pos; // Current write position in bytes
};

/// Initializes a read-only stream that pulls from a fixed const byte buffer
void stream_ibuf_init (
    struct stream          *s,   // Stream to initialize
    struct stream_ibuf_ctx *ctx, // Context to initialize and attach
    const void             *buf, // Source buffer to read from
    u32                     size
); // Number of bytes in buf

/// Initializes a write-only stream that pushes into a fixed mutable byte buffer
void stream_obuf_init (
    struct stream          *s,   // Stream to initialize
    struct stream_obuf_ctx *ctx, // Context to initialize and attach
    void                   *buf, // Destination buffer to write into
    u32                     cap
); // Capacity of buf in bytes

/// Initializes a null sink stream that discards all bytes written to it
void stream_sink_init (struct stream *s); // Stream to initialize

/// Callback type invoked on each element pushed into an opsink stream
typedef void (*byte_op) (void *buffer); // Pointer to the element being processed

/// Context for a stream that applies a callback to each element pushed into it
struct stream_opsink_ctx
{
  byte_op op;   // Callback invoked on each complete element
  void   *buf;  // Staging buffer used to accumulate one element before invoking op
  u32     size; // Size of each element in bytes
  u32     pos;  // Current write position within the staging buffer
};

/// Initializes a stream that applies op to each complete element of size bytes
/// pushed into it
void stream_opsink_init (
    struct stream            *s,   // Stream to initialize
    struct stream_opsink_ctx *ctx, // Context to initialize and attach
    byte_op                   op,  // Callback to invoke on each element
    void                     *buf, // Staging buffer of at least size bytes
    u32                       size
); // Size of each element in bytes

/// Context for a stream that forwards to an underlying stream up to a byte
/// limit
struct stream_limit_ctx
{
  struct stream *underlying; // The stream being wrapped
  u64            limit;      // Maximum number of bytes to forward
  u64            consumed;   // Number of bytes forwarded so far
};

/// Initializes a stream that forwards at most limit bytes from src before
/// marking itself done
void stream_limit_init (
    struct stream           *s,   // Stream to initialize
    struct stream_limit_ctx *ctx, // Context to initialize and attach
    struct stream           *src, // Underlying stream to wrap
    u64                      limit
); // Maximum number of bytes to forward

////////////////////////////////////////////////////////////
// MEMORY / BYTE_ACCESSOR

struct byte_accessor
{
  enum ta_type
  {
    TA_TAKE,
    TA_SELECT,
    TA_RANGE,
  } type;

  u32 src_size;  // total size this ba takes up on source
  u32 dest_size; // total size this ba puts into dest

  union {
    struct select_ba
    {
      u32                   bofst;  // Offset in bytes
      struct byte_accessor *sub_ba; // Next accessor
    } select;

    struct range_ba
    {
      struct stride         stride; // Stride on src
      struct byte_accessor *sub_ba; // For each stride, the next ba
    } range;
  };
};

u32 ba_memcpy_from (u8 *dest, const u8 *src, struct byte_accessor *acc);
u32 ba_memcpy_to (u8 *dest, const u8 *src, struct byte_accessor *acc);

////////////////////////////////////////////////////////////
// DEV / BOUNDS

// clang-format off

// Unsigned checks
#define _uadd_ok(a, b, max)      ((a) <= ((max) - (b)))
#define _usub_ok(a, b)           ((a) >= (b))
#define _umul_ok(a, b, max)      ((a) == 0 || (b) <= ((max) / (a)))

// Signed checks
#define _sadd_ok(a, b, min, max) (((b) > 0 && (a) <= ((max) - (b))) || ((b) <= 0 && (a) >= ((min) - (b))))
#define _ssub_ok(a, b, min, max) (((b) < 0 && (a) <= ((max) + (b))) || ((b) >= 0 && (a) >= ((min) + (b))))
#define _smul_ok(a, b, min, max) ((a) == 0 || (b) == 0 || \
                                  ((a) > 0 && (b) > 0 && (a) <= ((max) / (b))) || \
                                  ((a) < 0 && (b) < 0 && (a) >= ((max) / (b))) || \
                                  ((a) > 0 && (b) < 0 && (b) >= ((min) / (a))) || \
                                  ((a) < 0 && (b) > 0 && (a) >= ((min) / (b))))

// Division checks
#define _div_ok(b)               ((b) != 0)
#define _sdiv_ok(a, b, min)      ((b) != 0 && !((a) == (min) && (b) == -1))

// =============================================================================
// UNSIGNED INTEGERS
// =============================================================================

#if HAS_BUILTIN_OVERFLOW // GCC / Clang (Uses fast built-ins)

#define safe_add_u8(dest, a, b)  (!__builtin_add_overflow(a, b, dest))
#define safe_sub_u8(dest, a, b)  (!__builtin_sub_overflow(a, b, dest))
#define safe_mul_u8(dest, a, b)  (!__builtin_mul_overflow(a, b, dest))

#define safe_add_u16(dest, a, b) (!__builtin_add_overflow(a, b, dest))
#define safe_sub_u16(dest, a, b) (!__builtin_sub_overflow(a, b, dest))
#define safe_mul_u16(dest, a, b) (!__builtin_mul_overflow(a, b, dest))

#define safe_add_u32(dest, a, b) (!__builtin_add_overflow(a, b, dest))
#define safe_sub_u32(dest, a, b) (!__builtin_sub_overflow(a, b, dest))
#define safe_mul_u32(dest, a, b) (!__builtin_mul_overflow(a, b, dest))

#define safe_add_u64(dest, a, b) (!__builtin_add_overflow(a, b, dest))
#define safe_sub_u64(dest, a, b) (!__builtin_sub_overflow(a, b, dest))
#define safe_mul_u64(dest, a, b) (!__builtin_mul_overflow(a, b, dest))

#else // compiler without __builtin_overflow / compiler without __builtin_overflow

#define safe_add_u8(dest, a, b)  (_uadd_ok((u8)(a), (u8)(b), UINT8_MAX)  ? (*(dest) = (u8)(a) + (u8)(b), true) : false)
#define safe_sub_u8(dest, a, b)  (_usub_ok((u8)(a), (u8)(b))             ? (*(dest) = (u8)(a) - (u8)(b), true) : false)
#define safe_mul_u8(dest, a, b)  (_umul_ok((u8)(a), (u8)(b), UINT8_MAX)  ? (*(dest) = (u8)(a) * (u8)(b), true) : false)

#define safe_add_u16(dest, a, b) (_uadd_ok((u16)(a), (u16)(b), UINT16_MAX) ? (*(dest) = (u16)(a) + (u16)(b), true) : false)
#define safe_sub_u16(dest, a, b) (_usub_ok((u16)(a), (u16)(b))             ? (*(dest) = (u16)(a) - (u16)(b), true) : false)
#define safe_mul_u16(dest, a, b) (_umul_ok((u16)(a), (u16)(b), UINT16_MAX) ? (*(dest) = (u16)(a) * (u16)(b), true) : false)

#define safe_add_u32(dest, a, b) (_uadd_ok((u32)(a), (u32)(b), UINT32_MAX) ? (*(dest) = (u32)(a) + (u32)(b), true) : false)
#define safe_sub_u32(dest, a, b) (_usub_ok((u32)(a), (u32)(b))             ? (*(dest) = (u32)(a) - (u32)(b), true) : false)
#define safe_mul_u32(dest, a, b) (_umul_ok((u32)(a), (u32)(b), UINT32_MAX) ? (*(dest) = (u32)(a) * (u32)(b), true) : false)

#define safe_add_u64(dest, a, b) (_uadd_ok((u64)(a), (u64)(b), UINT64_MAX) ? (*(dest) = (u64)(a) + (u64)(b), true) : false)
#define safe_sub_u64(dest, a, b) (_usub_ok((u64)(a), (u64)(b))             ? (*(dest) = (u64)(a) - (u64)(b), true) : false)
#define safe_mul_u64(dest, a, b) (_umul_ok((u64)(a), (u64)(b), UINT64_MAX) ? (*(dest) = (u64)(a) * (u64)(b), true) : false)

#endif

// Unsigned Division (Standard across compilers)
#define safe_div_u8(dest, a, b)  (_div_ok(b) ? (*(dest) = (a) / (b), true) : false)
#define safe_div_u16(dest, a, b) (_div_ok(b) ? (*(dest) = (a) / (b), true) : false)
#define safe_div_u32(dest, a, b) (_div_ok(b) ? (*(dest) = (a) / (b), true) : false)
#define safe_div_u64(dest, a, b) (_div_ok(b) ? (*(dest) = (a) / (b), true) : false)

// =============================================================================
// SIGNED INTEGERS
// =============================================================================

#ifndef _MSC_VER // GCC / Clang

#define safe_add_i8(dest, a, b)  (!__builtin_add_overflow(a, b, dest))
#define safe_sub_i8(dest, a, b)  (!__builtin_sub_overflow(a, b, dest))
#define safe_mul_i8(dest, a, b)  (!__builtin_mul_overflow(a, b, dest))

#define safe_add_i16(dest, a, b) (!__builtin_add_overflow(a, b, dest))
#define safe_sub_i16(dest, a, b) (!__builtin_sub_overflow(a, b, dest))
#define safe_mul_i16(dest, a, b) (!__builtin_mul_overflow(a, b, dest))

#define safe_add_i32(dest, a, b) (!__builtin_add_overflow(a, b, dest))
#define safe_sub_i32(dest, a, b) (!__builtin_sub_overflow(a, b, dest))
#define safe_mul_i32(dest, a, b) (!__builtin_mul_overflow(a, b, dest))

#define safe_add_i64(dest, a, b) (!__builtin_add_overflow(a, b, dest))
#define safe_sub_i64(dest, a, b) (!__builtin_sub_overflow(a, b, dest))
#define safe_mul_i64(dest, a, b) (!__builtin_mul_overflow(a, b, dest))

#else // compiler without __builtin_overflow

#define safe_add_i8(dest, a, b)  (_sadd_ok((i8)(a), (i8)(b), INT8_MIN, INT8_MAX) ? (*(dest) = (i8)(a) + (i8)(b), true) : false)
#define safe_sub_i8(dest, a, b)  (_ssub_ok((i8)(a), (i8)(b), INT8_MIN, INT8_MAX) ? (*(dest) = (i8)(a) - (i8)(b), true) : false)
#define safe_mul_i8(dest, a, b)  (_smul_ok((i8)(a), (i8)(b), INT8_MIN, INT8_MAX) ? (*(dest) = (i8)(a) * (i8)(b), true) : false)

#define safe_add_i16(dest, a, b) (_sadd_ok((i16)(a), (i16)(b), INT16_MIN, INT16_MAX) ? (*(dest) = (i16)(a) + (i16)(b), true) : false)
#define safe_sub_i16(dest, a, b) (_ssub_ok((i16)(a), (i16)(b), INT16_MIN, INT16_MAX) ? (*(dest) = (i16)(a) - (i16)(b), true) : false)
#define safe_mul_i16(dest, a, b) (_smul_ok((i16)(a), (i16)(b), INT16_MIN, INT16_MAX) ? (*(dest) = (i16)(a) * (i16)(b), true) : false)

#define safe_add_i32(dest, a, b) (_sadd_ok((i32)(a), (i32)(b), INT32_MIN, INT32_MAX) ? (*(dest) = (i32)(a) + (i32)(b), true) : false)
#define safe_sub_i32(dest, a, b) (_ssub_ok((i32)(a), (i32)(b), INT32_MIN, INT32_MAX) ? (*(dest) = (i32)(a) - (i32)(b), true) : false)
#define safe_mul_i32(dest, a, b) (_smul_ok((i32)(a), (i32)(b), INT32_MIN, INT32_MAX) ? (*(dest) = (i32)(a) * (i32)(b), true) : false)

#define safe_add_i64(dest, a, b) (_sadd_ok((i64)(a), (i64)(b), INT64_MIN, INT64_MAX) ? (*(dest) = (i64)(a) + (i64)(b), true) : false)
#define safe_sub_i64(dest, a, b) (_ssub_ok((i64)(a), (i64)(b), INT64_MIN, INT64_MAX) ? (*(dest) = (i64)(a) - (i64)(b), true) : false)
#define safe_mul_i64(dest, a, b) (_smul_ok((i64)(a), (i64)(b), INT64_MIN, INT64_MAX) ? (*(dest) = (i64)(a) * (i64)(b), true) : false)

#endif

// Signed Division (Standard across compilers)
#define safe_div_i8(dest, a, b)  (_sdiv_ok((i8)(a), (i8)(b), INT8_MIN)  ? (*(dest) = (a) / (b), true) : false)
#define safe_div_i16(dest, a, b) (_sdiv_ok((i16)(a), (i16)(b), INT16_MIN) ? (*(dest) = (a) / (b), true) : false)
#define safe_div_i32(dest, a, b) (_sdiv_ok((i32)(a), (i32)(b), INT32_MIN) ? (*(dest) = (a) / (b), true) : false)
#define safe_div_i64(dest, a, b) (_sdiv_ok((i64)(a), (i64)(b), INT64_MIN) ? (*(dest) = (a) / (b), true) : false)

// =============================================================================
// FLOATING POINT
// =============================================================================

#define safe_add_f32(dest, a, b) ((*(dest) = (a) + (b)), isfinite(*(dest)))
#define safe_sub_f32(dest, a, b) ((*(dest) = (a) - (b)), isfinite(*(dest)))
#define safe_mul_f32(dest, a, b) ((*(dest) = (a) * (b)), isfinite(*(dest)))
#define safe_div_f32(dest, a, b) ((b) != 0.0f ? (*(dest) = (a) / (b), isfinite(*(dest))) : false)

#define safe_add_f64(dest, a, b) ((*(dest) = (a) + (b)), isfinite(*(dest)))
#define safe_sub_f64(dest, a, b) ((*(dest) = (a) - (b)), isfinite(*(dest)))
#define safe_mul_f64(dest, a, b) ((*(dest) = (a) * (b)), isfinite(*(dest)))
#define safe_div_f64(dest, a, b) ((b) != 0.0  ? (*(dest) = (a) / (b), isfinite(*(dest))) : false)

// =============================================================================
// ERROR HANDLING FUNCTIONS
// =============================================================================

HEADER_FUNC err_t safe_add_u64_err(u64 *dest, const u64 a, const u64 b, error *e)
{
    if (!safe_add_u64(dest, a, b)) {
        return error_causef(e, ERR_ARITH, "Overflow");
    }
    return SUCCESS;
}

// clang-format on

////////////////////////////////////////////////////////////
// CORE / CHECKSUMS

u32  checksum_init (void);
void checksum_execute (u32 *dest, const u8 *data, u32 len);

////////////////////////////////////////////////////////////
// CORE / HASHING

u32 fnv1a_hash (struct string s);

////////////////////////////////////////////////////////////
// CORE / MAX_CAPTURE

///
/// You may obtain a copy of the License at
///
///
/// Unless required by applicable law or agreed to in writing, software

////////////////////////////////////////////////////////////
// CORE / NUMBERS

err_t parse_i32_expect (i32 *dest, const char *data, u32 len, error *e);
err_t parse_i64_expect (i64 *dest, const char *data, u32 len, error *e);
err_t parse_f32_expect (f32 *dest, const char *s, u32 len, error *e);
f32   py_mod_f32 (f32 num, f32 denom);
i32   py_mod_i32 (i32 num, i32 denom);

////////////////////////////////////////////////////////////
// CORE / RANDOM

void rand_seed (void);
void rand_seed_with (u32 seed);

u8 randu8 (void);
i8 randi8 (void);
u8 randu8r (u8 lower, u8 upper); // [lower, upper]
u8 randu8e (u8 lower, u8 upper); // [lower, upper)
i8 randi8r (i8 lower, i8 upper); // [lower, upper]
i8 randi8e (i8 lower, i8 upper); // [lower, upper)

u16 randu16 (void);
i16 randi16 (void);
u16 randu16r (u16 lower, u16 upper); // [lower, upper]
u16 randu16e (u16 lower, u16 upper); // [lower, upper)
i16 randi16r (i16 lower, i16 upper); // [lower, upper]
i16 randi16e (i16 lower, i16 upper); // [lower, upper)

u32 randu32 (void);
i32 randi32 (void);
u32 randu32r (u32 lower, u32 upper); // [lower, upper]
u32 randu32e (u32 lower, u32 upper); // [lower, upper)
i32 randi32r (i32 lower, i32 upper); // [lower, upper]
i32 randi32e (i32 lower, i32 upper); // [lower, upper)

u64 randu64 (void);
i64 randi64 (void);
u64 randu64r (u64 lower, u64 upper); // [lower, upper]
u64 randu64e (u64 lower, u64 upper); // [lower, upper)
i64 randi64r (i64 lower, i64 upper); // [lower, upper]
i64 randi64e (i64 lower, i64 upper); // [lower, upper)

err_t rand_str (struct string *dest, struct chunk_alloc *alloc, u32 minlen, u32 maxlen, error *e);

void rand_bytes (void *dest, u32 len);
#define decl_rand_buffer(name, type, len) \
  type name[len];                         \
  rand_bytes (name, sizeof (type) * len);

void shuffle_u32 (u32 *array, u32 len);

////////////////////////////////////////////////////////////
// CORE / MATH

#define i_creal_64(f) (creal (f))
#define i_cimag_64(f) (cimag (f))

#define i_cabs_sqrd_64(f) ((creal (f) * creal (f)) + ((cimag (f) * cimag (f))))
#define i_cabs_64(f)      cabsf (f)
#define i_fabs_32(f)      fabsf (f)

#define arr_range(arr)                                     \
  do                                                       \
  {                                                        \
    for (u32 i = 0; i < arrlen (arr); ++i) { arr[i] = i; } \
  }                                                        \
  while (0)

#define ptr_range(arr, size)                            \
  do                                                    \
  {                                                     \
    for (u32 _i = 0; _i < size; ++_i) { arr[_i] = _i; } \
  }                                                     \
  while (0)

#define u32_arr_rand(arr)                                           \
  do                                                                \
  {                                                                 \
    for (u32 i = 0; i < arrlen (arr); ++i) { arr[i] = randu32 (); } \
  }                                                                 \
  while (0)

#define arr_contains(arr, len, val, ret)   \
  do                                       \
  {                                        \
    ret = false;                           \
    for (u32 ___i = 0; ___i < len; ++___i) \
    {                                      \
      if (arr[___i] == val)                \
      {                                    \
        ret = arr[___i];                   \
        ret = true;                        \
        break;                             \
      }                                    \
    }                                      \
  }                                        \
  while (0)

float f16_to_f32 (u16 h);

////////////////////////////////////////////////////////////
// DEV / TESTING

///
/// You may obtain a copy of the License at
///
///
/// Unless required by applicable law or agreed to in writing, software

#ifndef NTEST

// Returns if the test passed or not
typedef bool (*test_func) (void);
typedef struct
{
  test_func test;
  char     *test_name;
} test;

extern int test_ret; // Global (thread unsafe) return code for tests

////////////////////////////////////////////////////////////
/// Test Suite

// TEST_SUITE (core, 64);
//
// REGISTER (core, foo);
// REGISTER (core, bar);
#  define TEST_SUITE(name, max)    \
    enum                           \
    {                              \
      name##_max = (max)           \
    };                             \
    test name##_tests[name##_max]; \
    int  name##_count = 0

#  define REGISTER(suite, name)                \
    do                                         \
    {                                          \
      extern bool wrapper_test_##name (void);  \
      ASSERT (suite##_count < suite##_max);    \
      suite##_tests[suite##_count++] = (test){ \
          .test      = wrapper_test_##name,    \
          .test_name = #name,                  \
      };                                       \
    }                                          \
    while (0)

////////////////////////////////////////////////////////////
/// Test Marker

enum
{
  test_marks_max = 256,
  test_mark_len  = 128,
};

extern char test_marks[test_marks_max][test_mark_len];
extern int  test_marks_count;

// Match a test marker string
HEADER_FUNC bool
test_mark_match (const char *pat, const char *str)
{
  while (*pat)
  {
    if (*pat == '*')
    {
      pat++;
      if (!*pat) { return true; }

      while (*str)
      {
        if (test_mark_match (pat, str)) { return true; }
        str++;
      }
      return false;
    }
    if (*str != *pat) { return false; }
    pat++;
    str++;
  }
  return *str == '\0';
}

#  define TEST_MARK(label)                               \
    do                                                   \
    {                                                    \
      ASSERT (test_marks_count < test_marks_max);        \
      const char *_src = (label);                        \
      char       *_dst = test_marks[test_marks_count++]; \
      int         _i   = 0;                              \
      while (_i < test_mark_len - 1 && _src[_i])         \
      {                                                  \
        _dst[_i] = _src[_i];                             \
        _i++;                                            \
      }                                                  \
      _dst[_i] = '\0';                                   \
    }                                                    \
    while (0)

#  define test_reset_marks()     \
    do { test_marks_count = 0; } \
    while (0)

////////////////////////////////////////////////////////////
/// Test Wrappers

#  define TEST(name)                                                   \
    static void test_##name (void);                                    \
    bool        wrapper_test_##name (void);                            \
    bool        wrapper_test_##name (void)                             \
    {                                                                  \
      i_log_info ("========================= TEST CASE: %s\n", #name); \
      int prev = test_ret;                                             \
      test_ret = 0;                                                    \
      test_##name ();                                                  \
      if (!test_ret)                                                   \
      {                                                                \
        i_log_passed ("%s\n", #name);                                  \
        test_ret = prev;                                               \
        return true;                                                   \
      }                                                                \
      return false;                                                    \
    }                                                                  \
    static void test_##name (void)

#  define TEST_CASE(fmt, ...)                                                                \
    for (int _tc_once = (i_log_info ("------ CASE: " fmt "\n", ##__VA_ARGS__), 1),           \
             _tc_prev = test_ret;                                                            \
         _tc_once;                                                                           \
         _tc_once = 0,                                                                       \
             (test_ret == _tc_prev ? (i_log_passed ("------ : " fmt "\n", ##__VA_ARGS__), 0) \
                                   : (i_log_failure ("------ : " fmt "\n", ##__VA_ARGS__), 0)))

////////////////////////////////////////////////////////////
/// Test Runtime Methods

#  define fail_test(fmt, ...)                                     \
    do                                                            \
    {                                                             \
      i_log_failure (FPREFIX_STR fmt, FPREFIX_ARGS, __VA_ARGS__); \
      test_ret = -1;                                              \
      return;                                                     \
    }                                                             \
    while (0)

#  define test_assert_equal(left, right)                                  \
    do                                                                    \
    {                                                                     \
      if ((left) != (right)) { fail_test ("%s != %s\n", #left, #right); } \
    }                                                                     \
    while (0)

#  define test_assert_int_equal(left, right) test_assert_type_equal (left, right, i32, PRId32)

#  define test_assert_type_equal(left, right, type, fmt) \
    do                                                   \
    {                                                    \
      type _left  = left;                                \
      type _right = right;                               \
      if ((_left) != (_right))                           \
      {                                                  \
        fail_test (                                      \
            "Expression: %s != %s values: "              \
            "(%" fmt ") != (%" fmt ")\n",                \
            #left,                                       \
            #right,                                      \
            _left,                                       \
            _right                                       \
        );                                               \
      }                                                  \
    }                                                    \
    while (0)

#  define test_assert_ptr_equal(left, right) test_assert_equal ((void *)left, (void *)right)

#  define test_assert(expr)                                 \
    do                                                      \
    {                                                       \
      if (!(expr)) { fail_test ("Expected: %s\n", #expr); } \
    }                                                       \
    while (0)

#  define test_fail_if(expr)                               \
    do                                                     \
    {                                                      \
      if (expr) { fail_test ("Unexpected: %s\n", #expr); } \
    }                                                      \
    while (0)

#  define test_err_t_check(expr, exp, ename) \
    do                                       \
    {                                        \
      err_t __ret = (err_t)expr;             \
      test_assert_int_equal (__ret, exp);    \
      (ename)->cause_code = SUCCESS;         \
    }                                        \
    while (0)

#  define test_assert_memequal(a, b, size) test_assert_int_equal (memcmp (a, b, size), 0)

#  define test_assert_mark_hit(pattern)                                 \
    do                                                                  \
    {                                                                   \
      const char *_pat = (pattern);                                     \
      bool        _hit = false;                                         \
      for (int _i = 0; _i < test_marks_count; _i++)                     \
      {                                                                 \
        if (test_mark_match (_pat, test_marks[_i]))                     \
        {                                                               \
          _hit = true;                                                  \
          break;                                                        \
        }                                                               \
      }                                                                 \
      if (!_hit) { fail_test ("No mark matched pattern: %s\n", _pat); } \
    }                                                                   \
    while (0)

#  define test_assert_mark_not_hit(pattern)                         \
    do                                                              \
    {                                                               \
      const char *_pat = (pattern);                                 \
      bool        _hit = false;                                     \
      for (int _i = 0; _i < test_marks_count; _i++)                 \
      {                                                             \
        if (test_mark_match (_pat, test_marks[_i]))                 \
        {                                                           \
          _hit = true;                                              \
          break;                                                    \
        }                                                           \
      }                                                             \
      if (_hit) { fail_test ("Mark matched pattern: %s\n", _pat); } \
    }                                                               \
    while (0)

#else // NTEST

#  define TEST_SUITE(name, max) ((void)0)
#  define REGISTER(suite, name) ((void)0)
#  define TEST(type, name)                              \
    static inline void test_##name (void) MAYBE_UNUSED; \
    static inline void test_##name (void)
#  define TEST_CASE(fmt, ...)                         if (0)
#  define TEST_MARK(label)                            ((void)0)
#  define test_assert_mark_hit(pattern)               ((void)0)
#  define test_reset_marks()                          ((void)0)
#  define test_assert(expr)                           ((void)0)
#  define test_assert_equal(left, right)              ((void)0)
#  define test_assert_int_equal(left, right)          ((void)0)
#  define test_assert_type_equal(left, right, t, fmt) ((void)0)
#  define test_assert_ptr_equal(left, right)          ((void)0)
#  define test_assert_memequal(a, b, size)            ((void)0)
#  define test_fail_if(expr)                          ((void)0)
#  define test_err_t_wrap(expr, ename)                ((void)(expr))
#  define test_err_t_check(expr, exp, ename)          ((void)(expr))
#  define fail_test(fmt, ...)                         ((void)0)

#endif // NTEST

////////////////////////////////////////////////////////////
// DEV / DATA_VALIDATOR

/**
 * A data validator is used in testing to verify that a data writer
 * matches it's reference data writer. There are two data writers:
 *    ref - The reference writer. This is assumed correct
 *    sut - System under test. This is what we're testing.
 */
struct dvalidtr
{
  struct data_writer ref;
  struct data_writer sut;
  isvalid_func       isvalid;
};

/**
 * @brief Conducts a data validator random test. Loops through
 * [niters] times and calls a random method from insert read remove write
 * with random ranges. It does it for both ref and sut and compares the results
 * to ensure they match
 *
 * @param d The data validator to test on
 * @param size The size to use in the data validator test
 * @param niters The number of iterations to run
 * @param max_insert Maximum insertion length for one insert
 * @param e An error object to handle errors
 * @return Error result
 */
err_t dvalidtr_random_test (struct dvalidtr *d, u32 size, u32 niters, u64 max_insert, error *e);

////////////////////////////////////////////////////////////
// CONCURRENCY / GR_LOCK

enum lock_mode
{
  LM_IS    = 0,
  LM_IX    = 1,
  LM_S     = 2,
  LM_SIX   = 3,
  LM_X     = 4,
  LM_COUNT = 5
};

/**
 * lock waiters are allocated on the stack - so a waiter only exists
 * for as long as gr_lock is waiting. Therefore, there is no array of
 * granted lock groups, instead, I use a counter to count how many locks
 * of each type are granted.
 *
 * This comes with drawbacks.
 *
 * 1. There is no priority - The only priority that's ensured is that the order
 *    of the condition variable signals is the same as the order that they came
 * in, which is ok, but not the best.
 *
 * In order to add priority I need to keep track of the list of granted locks -
 * which would mean some sort of allocation or intrusive data structure.
 *
 * One idea would be to have a gr_lock live on the stack and it's lifecycle is
 * tied to one lock unlock flow, which is probably what I'll do, but for now,
 * locks have loose priority until it becomes a performance problem
 */
struct gr_lock_waiter
{
  enum lock_mode         mode;
  i_cond                 cond;
  struct gr_lock_waiter *prev;
  struct gr_lock_waiter *next;
};

struct gr_lock
{
  i_mutex                mutex;
  int                    holder_counts[LM_COUNT];
  struct gr_lock_waiter *head;
};

err_t gr_lock_init (struct gr_lock *l, error *e);

void gr_lock_destroy (struct gr_lock *l);

err_t gr_lock (struct gr_lock *l, enum lock_mode mode, error *e);
bool  gr_trylock (struct gr_lock *l, enum lock_mode mode);
void  gr_unlock (struct gr_lock *l, enum lock_mode mode);

const char    *gr_lock_mode_name (enum lock_mode mode);
enum lock_mode get_parent_mode (enum lock_mode child_mode);

////////////////////////////////////////////////////////////
// CONCURRENCY / PERIODIC_TASK

typedef void (*periodic_task_fn) (void *ctx);

struct periodic_task
{
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

err_t periodic_task_init (struct periodic_task *t, error *e);

err_t
periodic_task_start (struct periodic_task *t, u64 msec, periodic_task_fn fn, void *ctx, error *e);

err_t periodic_task_stop (struct periodic_task *t, error *e);
void  periodic_task_wake (struct periodic_task *t);

////////////////////////////////////////////////////////////
// NET / CLIENT

struct client
{
  i_socket sock;
};

err_t client_connect (struct client *dest, const char *host, u16 port, error *e);

err_t client_write_all (struct client *c, const void *src, u16 len, error *e);

err_t client_write_all_size_prefixed (struct client *c, const void *msg, u16 len, error *e);

err_t client_read_all (struct client *c, void *dest, u16 len, error *e);

i32 client_read_all_size_prefixed (struct client *c, void *dest, u16 len, error *e);

err_t client_disconnect (struct client *c, error *e);

////////////////////////////////////////////////////////////
// NET / POLLING_SERVER

struct connection
{
  void    *rx_buf;
  u32      rx_cap;
  u32      rx_len;
  void    *tx_buf;
  u32      tx_cap;
  u32      tx_sent;
  i_socket sock;
  latch    l;
};

struct conn_actions
{
  struct connection *(*conn_alloc) (void *ctx, error *e);
  err_t (*conn_func) (void *ctx, struct connection *conn, error *e);
  void (*conn_free) (void *ctx, struct connection *conn);
};

struct polling_server
{
  i_pollfd           *fds;
  struct connection **conns;
  u32                 len;
  u32                 cap;
  volatile int        running;
  struct conn_actions actions;
  void               *ctx;
  i_socket            server;
};

err_t
pserv_open (struct polling_server *ps, int port, struct conn_actions actions, void *ctx, error *e);

int pserv_execute (struct polling_server *ps, error *e);

err_t pserv_close (struct polling_server *ps, error *e);

////////////////////////////////////////////////////////////
// NET / ECHO_SERVER

struct echo_context
{
  const char *prefix;
  const char *suffix;
};

struct connection *echo_conn_alloc (const void *ctx, error *e);
err_t              echo_conn_func (const void *echo_ctx, struct connection *conn, error *e);
void               echo_conn_free (const void *ctx, struct connection *conn);

#endif // C_SPECX_H

////////////////////////////////////////////////////////////
// Robin Hood Hash Table

#ifdef KTYPE

#include <string.h>

#ifndef KTYPE
#  define KTYPE int
#endif
#ifndef VTYPE
#  define VTYPE int
#endif
#ifndef SUFFIX
#  define SUFFIX int
#endif

#define HDATA_T      RH__XCAT (hdata_, SUFFIX)
#define HENWRAP_T    RH__XCAT (hentry_, SUFFIX)
#define HTIR_T       RH__XCAT (htir_res_, SUFFIX)
#define HT_T         RH__XCAT (ht_, SUFFIX)
#define HASH_TABLE_T RH__XCAT (hash_table_, SUFFIX)

// Functions
#define HT_INIT          RH__XCAT (ht_init_, SUFFIX)
#define HT_INSERT        RH__XCAT (ht_insert_, SUFFIX)
#define HT_INSERT_EXPECT RH__XCAT (ht_insert_expect_, SUFFIX)
#define HT_GET           RH__XCAT (ht_get_, SUFFIX)
#define HT_GET_EXPECT    RH__XCAT (ht_get_expect_, SUFFIX)
#define HT_DELETE        RH__XCAT (ht_delete_, SUFFIX)
#define HT_COUNT         RH__XCAT (ht_count_, SUFFIX)
#define HT_DELETE_EXPECT RH__XCAT (ht_delete_expect_, SUFFIX)

typedef struct
{
  KTYPE key; // Hash key
  VTYPE value;
} HDATA_T;

typedef struct
{
  HDATA_T data;    // The data we store
  KTYPE   dib;     // Distance from initial bucket
  bool    present; // Exists or not
} HENWRAP_T;

typedef struct
{
  u32        cap;
  HENWRAP_T *elems;
  latch      l;
} HASH_TABLE_T;

HEADER_FUNC void
HT_INIT (HASH_TABLE_T *dest, HENWRAP_T *arr, const u32 nelem)
{
  ASSERT (dest);
  ASSERT (arr);

  memset (arr, 0, nelem * sizeof *arr);
  dest->elems = arr;
  dest->cap   = nelem;
  latch_init (&dest->l);
}

HEADER_FUNC hti_res
HT_INSERT (HASH_TABLE_T *ht, HDATA_T data)
{
  ASSERT (ht);
  ASSERT (ht->cap > 0);

  KTYPE   dibn = 0; // Current distance from initial bucket
  hti_res ret  = HTIR_FULL;

  latch_lock (&ht->l);

  for (KTYPE i = 0; i < (KTYPE)ht->cap; ++i, ++dibn)
  {
    // Mapped index after probing
    KTYPE _i = (data.key + dibn) % (KTYPE)ht->cap;

    // If not present, insert
    if (!ht->elems[_i].present)
    {
      ht->elems[_i].data    = data;
      ht->elems[_i].dib     = dibn;
      ht->elems[_i].present = true;
      ret                   = HTIR_SUCCESS;
      goto theend;
    }

    // Swap (lt means dib != dibn, therefore key != key)
    if (ht->elems[_i].dib < dibn)
    {
      HDATA_T temp_data = ht->elems[_i].data;
      KTYPE   temp_dib  = ht->elems[_i].dib;

      ht->elems[_i].data = data;
      ht->elems[_i].dib  = dibn;

      dibn = temp_dib;
      data = temp_data;
    }

    // Compare keys for duplicates
    if (ht->elems[_i].data.key == data.key)
    {
      ret = HTIR_EXISTS;
      goto theend;
    }
  }

theend:
  latch_unlock (&ht->l);
  return ret;
}

HEADER_FUNC void
HT_INSERT_EXPECT (HASH_TABLE_T *ht, HDATA_T data)
{
  const hti_res ret = HT_INSERT (ht, data);
  ASSERT (ret == HTIR_SUCCESS);
}

HEADER_FUNC hta_res
HT_GET (HASH_TABLE_T *ht, HDATA_T *dest, KTYPE key)
{
  ASSERT (ht);
  ASSERT (ht->cap > 0);

  KTYPE dibn = 0;

  hta_res ret = HTAR_DOESNT_EXIST;

  latch_lock (&ht->l);

  for (KTYPE i = 0; i < (KTYPE)ht->cap; ++i, ++dibn)
  {
    // Mapped index after probing
    KTYPE _i = (key + i) % (KTYPE)ht->cap;

    // If not present, return
    if (!ht->elems[_i].present)
    {
      ret = HTAR_DOESNT_EXIST;
      goto theend;
    }

    // Short cut - DIB invariant is broken
    if (ht->elems[_i].dib < dibn)
    {
      ret = HTAR_DOESNT_EXIST;
      goto theend;
    }

    // Check for key
    if (ht->elems[_i].data.key == key)
    {
      if (dest)
      {
        *dest = (HDATA_T){
            .value = ht->elems[_i].data.value,
            .key   = ht->elems[_i].data.key,
        };
      }
      ret = HTAR_SUCCESS;
      goto theend;
    }
  }

theend:
  latch_unlock (&ht->l);
  return ret;
}

HEADER_FUNC void
HT_GET_EXPECT (HASH_TABLE_T *ht, HDATA_T *dest, KTYPE key)
{
  const hta_res ret = HT_GET (ht, dest, key);
  ASSERT (ret == HTAR_SUCCESS);
}

HEADER_FUNC hta_res
HT_DELETE (HASH_TABLE_T *ht, HDATA_T *dest, KTYPE key)
{
  ASSERT (ht);
  ASSERT (ht->cap > 0);

  KTYPE dibn = 0;
  KTYPE i    = 0;

  hta_res ret = HTAR_DOESNT_EXIST;

  latch_lock (&ht->l);

  for (i = 0; i < (KTYPE)ht->cap; ++i, ++dibn)
  {
    // Mapped index after probing
    KTYPE _i = (key + i) % (KTYPE)ht->cap;

    // If not present, return
    if (!ht->elems[_i].present)
    {
      ret = HTAR_DOESNT_EXIST;
      goto theend;
    }

    // Short cut - DIB invariant is broken
    if (ht->elems[_i].dib < dibn)
    {
      ret = HTAR_DOESNT_EXIST;
      goto theend;
    }

    // Check for key
    if (ht->elems[_i].data.key == key)
    {
      if (dest) { *dest = ht->elems[_i].data; }
      break;
    }
  }

  // Shift all full elements to the left
  for (; i < (KTYPE)ht->cap; ++i)
  {
    // Mapped index after probing
    KTYPE hole = (key + i) % (KTYPE)ht->cap;
    KTYPE next = (key + i + 1) % (KTYPE)ht->cap;

    // Right value is empty, set this to empty and return
    if (!ht->elems[next].present || ht->elems[next].dib == 0)
    {
      ht->elems[hole].present = false;
      ret                     = HTAR_SUCCESS;
      goto theend;
    }

    // Shift left
    ht->elems[hole].data = ht->elems[next].data;
    ASSERT (ht->elems[next].dib > 0);
    ht->elems[hole].dib = ht->elems[next].dib - 1;
    ASSERT (ht->elems[hole].present);
  }

theend:
  latch_unlock (&ht->l);
  return ret;
}

HEADER_FUNC u32
HT_COUNT (HASH_TABLE_T *ht)
{
  ASSERT (ht);
  ASSERT (ht->cap > 0);

  u32 ret = 0;

  latch_lock (&ht->l);

  for (u32 i = 0; i < ht->cap; ++i)
  {
    if (ht->elems[i].present) { ret++; }
  }

  latch_unlock (&ht->l);

  return ret;
}

HEADER_FUNC void
HT_DELETE_EXPECT (HASH_TABLE_T *ht, HDATA_T *dest, KTYPE key)
{
  const hta_res ret = HT_DELETE (ht, dest, key);
  ASSERT (ret == HTAR_SUCCESS);
}

#undef HDATA_T
#undef HENWRAP_T
#undef HTIR_T
#undef HT_T
#undef HASH_TABLE_T
#undef HT_INIT
#undef HT_INSERT
#undef HT_INSERT_EXPECT
#undef HT_GET
#undef HT_GET_EXPECT
#undef HT_DELETE
#undef HT_COUNT
#undef HT_DELETE_EXPECT

#endif // KTYPE

////////////////////////////////////////////////////////////
// TEMPLATE: ROBIN HOOD HASH TABLE
// Define HASH_FUNC, CMP_FUNC, VTYPE, SUFFIX before re-including
////////////////////////////////////////////////////////////
#ifdef HASH_FUNC

#include <string.h>

#ifndef VTYPE
#  define VTYPE int
#endif
#ifndef SUFFIX
#  define SUFFIX int
#endif
#ifndef HASH_FUNC
#  define HASH_FUNC(v) ((u32)(v))
#endif
#ifndef CMP_FUNC
#  define CMP_FUNC(a, b) ((a) == (b))
#endif

#define HENWRAP_T    RH__XCAT (hentry_, SUFFIX)
#define HASH_TABLE_T RH__XCAT (hash_table_, SUFFIX)

#define HT_INIT          RH__XCAT (ht_init_, SUFFIX)
#define HT_INSERT        RH__XCAT (ht_insert_, SUFFIX)
#define HT_INSERT_EXPECT RH__XCAT (ht_insert_expect_, SUFFIX)
#define HT_GET           RH__XCAT (ht_get_, SUFFIX)
#define HT_GET_EXPECT    RH__XCAT (ht_get_expect_, SUFFIX)
#define HT_DELETE        RH__XCAT (ht_delete_, SUFFIX)
#define HT_DELETE_EXPECT RH__XCAT (ht_delete_expect_, SUFFIX)

typedef struct
{
  VTYPE value;   // The value we store
  u32   hash;    // Hash of the value
  u32   dib;     // Distance from initial bucket
  bool  present; // Exists or not
} HENWRAP_T;

typedef struct
{
  u32        cap;
  HENWRAP_T *elems;
} HASH_TABLE_T;

HEADER_FUNC void
HT_INIT (HASH_TABLE_T *dest, HENWRAP_T *arr, u32 nelem)
{
  ASSERT (dest);
  ASSERT (arr);

  memset (arr, 0, nelem * sizeof *arr);
  dest->elems = arr;
  dest->cap   = nelem;
}

HEADER_FUNC hti_res
HT_INSERT (HASH_TABLE_T *ht, VTYPE value)
{
  ASSERT (ht);
  ASSERT (ht->cap > 0);

  u32 hash = HASH_FUNC (value);
  u32 dibn = 0; // Current distance from initial bucket

  for (u32 i = 0; i < ht->cap; ++i, ++dibn)
  {
    // Mapped index after probing
    u32 _i = (hash + dibn) % ht->cap;

    // If not present, insert
    if (!ht->elems[_i].present)
    {
      ht->elems[_i].value   = value;
      ht->elems[_i].hash    = hash;
      ht->elems[_i].dib     = dibn;
      ht->elems[_i].present = true;
      return HTIR_SUCCESS;
    }

    // Swap (lt means dib != dibn, therefore different
    // values)
    if (ht->elems[_i].dib < dibn)
    {
      VTYPE temp_value = ht->elems[_i].value;
      u32   temp_hash  = ht->elems[_i].hash;
      u32   temp_dib   = ht->elems[_i].dib;

      ht->elems[_i].value = value;
      ht->elems[_i].hash  = hash;
      ht->elems[_i].dib   = dibn;

      dibn  = temp_dib;
      value = temp_value;
      hash  = temp_hash;
    }

    // Compare values for duplicates
    if (ht->elems[_i].hash == hash && CMP_FUNC (ht->elems[_i].value, value)) { return HTIR_EXISTS; }
  }

  return HTIR_FULL;
}

HEADER_FUNC void
HT_INSERT_EXPECT (HASH_TABLE_T *ht, VTYPE value)
{
  hti_res ret = HT_INSERT (ht, value);
  ASSERT (ret == HTIR_SUCCESS);
}

HEADER_FUNC hta_res
HT_GET (const HASH_TABLE_T *ht, VTYPE *dest, VTYPE value)
{
  ASSERT (ht);
  ASSERT (ht->cap > 0);

  u32 hash = HASH_FUNC (value);
  u32 dibn = 0;

  for (u32 i = 0; i < ht->cap; ++i, ++dibn)
  {
    // Mapped index after probing
    u32 _i = (hash + i) % ht->cap;

    // If not present, return
    if (!ht->elems[_i].present) { return HTAR_DOESNT_EXIST; }

    // Short cut - DIB invariant is broken
    if (ht->elems[_i].dib < dibn) { return HTAR_DOESNT_EXIST; }

    // Check for value match
    if (ht->elems[_i].hash == hash && CMP_FUNC (ht->elems[_i].value, value))
    {
      if (dest) { *dest = ht->elems[_i].value; }
      return HTAR_SUCCESS;
    }
  }

  return HTAR_DOESNT_EXIST;
}

HEADER_FUNC void
HT_GET_EXPECT (const HASH_TABLE_T *ht, VTYPE *dest, VTYPE value)
{
  hta_res ret = HT_GET (ht, dest, value);
  ASSERT (ret == HTAR_SUCCESS);
}

HEADER_FUNC hta_res
HT_DELETE (HASH_TABLE_T *ht, VTYPE *dest, VTYPE value)
{
  ASSERT (ht);
  ASSERT (ht->cap > 0);

  u32 hash = HASH_FUNC (value);
  u32 dibn = 0;
  u32 i    = 0;

  for (i = 0; i < ht->cap; ++i, ++dibn)
  {
    // Mapped index after probing
    u32 _i = (hash + i) % ht->cap;

    // If not present, return
    if (!ht->elems[_i].present) { return HTAR_DOESNT_EXIST; }

    // Short cut - DIB invariant is broken
    if (ht->elems[_i].dib < dibn) { return HTAR_DOESNT_EXIST; }

    // Check for value match
    if (ht->elems[_i].hash == hash && CMP_FUNC (ht->elems[_i].value, value))
    {
      if (dest) { *dest = ht->elems[_i].value; }
      break;
    }
  }

  // Shift all full elements to the left
  for (; i < ht->cap; ++i)
  {
    // Mapped index after probing
    u32 hole = (hash + i) % ht->cap;
    u32 next = (hash + i + 1) % ht->cap;

    // Right value is empty, set this to empty and return
    if (!ht->elems[next].present || ht->elems[next].dib == 0)
    {
      ht->elems[hole].present = false;
      return HTAR_SUCCESS;
    }

    // Shift left
    ht->elems[hole].value = ht->elems[next].value;
    ht->elems[hole].hash  = ht->elems[next].hash;
    ASSERT (ht->elems[next].dib > 0);
    ht->elems[hole].dib = ht->elems[next].dib - 1;
    ASSERT (ht->elems[hole].present);
  }

  return HTAR_DOESNT_EXIST;
}

HEADER_FUNC void
HT_DELETE_EXPECT (HASH_TABLE_T *ht, VTYPE *dest, VTYPE value)
{
  hta_res ret = HT_DELETE (ht, dest, value);
  ASSERT (ret == HTAR_SUCCESS);
}

#undef HENWRAP_T
#undef HASH_TABLE_T
#undef HT_INIT
#undef HT_INSERT
#undef HT_INSERT_EXPECT
#undef HT_GET
#undef HT_GET_EXPECT
#undef HT_DELETE
#undef HT_DELETE_EXPECT
#undef VTYPE
#undef SUFFIX
#undef HASH_FUNC
#undef CMP_FUNC

#endif // HASH_FUNC
