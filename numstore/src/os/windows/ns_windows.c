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

#  include "core/ns_bytes.h"
#  include "core/ns_csx_assert.h"
#  include "core/ns_error.h"
#  include "core/ns_stdtypes.h"
#  include "os/ns_os.h"
#  include "os/windows/ns_windows_os.h"

#  include <stdbool.h>
#  include <stdint.h>
#  include <stdio.h>
#  include <string.h>

static const struct os_vtable default_os_vtable;

///////////// Error helpers

#  define WIN_ERR_BUF 256

/// FormatMessage into buf, trimming the trailing "\r\n" it appends.
/// Falls back to the numeric code if the system has no message.
static const char *
win_strerror (const DWORD err, char *buf, const DWORD buflen)
{
  DWORD n = FormatMessageA (
      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      err,
      0,
      buf,
      buflen,
      NULL
  );

  if (n == 0) {
    snprintf (buf, buflen, "error %lu", (unsigned long)err);
    return buf;
  }

  while (n > 0 && (buf[n - 1] == '\r' || buf[n - 1] == '\n' || buf[n - 1] == ' ')) {
    buf[--n] = '\0';
  }

  return buf;
}

/// Sets [e] from a Win32 error code. [what] (a file name) may be NULL.
static err_t
win_fail (error *e, const err_t type, const char *op, const char *what, const DWORD err)
{
  char buf[WIN_ERR_BUF];
  win_strerror (err, buf, sizeof (buf));

  if (what) {
    return error_causef (e, type, "%s %s: %s", op, what, buf);
  }
  return error_causef (e, type, "%s: %s", op, buf);
}

/// Win32 I/O calls take a DWORD length
HEADER_FUNC DWORD
clamp_dword (const u64 n)
{
  return (DWORD)(n > 0xFFFFFFFFULL ? 0xFFFFFFFFUL : n);
}

///////////// Debug asserts

#  ifndef NDEBUG
static bool
handle_is_open (const HANDLE h)
{
  DWORD flags;
  return GetHandleInformation (h, &flags) != 0;
}

/// CRITICAL_SECTION::OwningThread is declared as a HANDLE but holds the
/// owning thread's id (0 when unlocked) on every shipping Windows version.
static DWORD
cs_owner (const CRITICAL_SECTION *m)
{
  return (DWORD)(uintptr_t)m->OwningThread;
}
#  endif

DEFINE_DBG_ASSERT (HANDLE, handle, h, {
  ASSERT (h);
  ASSERT (*h != WIN_SLOT_FREE);    // not free
  ASSERT (*h != WIN_SLOT_PENDING); // not pending
  ASSERT (*h != INVALID_HANDLE_VALUE);
  ASSERT (handle_is_open (*h));
})

DEFINE_DBG_ASSERT (struct windows_timer, timer, t, {
  ASSERT (t);
  ASSERT (t->frequency.QuadPart > 0);
  ASSERT (t->start.QuadPart >= 0);
})

DEFINE_DBG_ASSERT (CRITICAL_SECTION, mutex, m, { ASSERT (m); })

DEFINE_DBG_ASSERT (CONDITION_VARIABLE, cond, c, { ASSERT (c); })

DEFINE_DBG_ASSERT (struct windows_thread_frame, thread, t, {
  ASSERT (t);
  ASSERT (t->occupied == 1);
  ASSERT (t->handle != NULL);
})

/// Reads the HANDLE out of a file self (a handle slot) and checks it
HEADER_FUNC HANDLE
handle_of (void *self)
{
  PVOID volatile *slot = self;
  ASSERT (slot);
  const HANDLE h = windows_handle_load (slot);
  DBG_ASSERT (handle, &h);
  return h;
}

///////////// System OS implementations

static err_t
sys_open_file (
    struct windows_os *os,
    i_file            *dest,
    const char        *fname,
    const DWORD        access,
    const DWORD        creation,
    const char        *opname,
    error             *e
)
{
  ASSERT (os);
  ASSERT (dest);
  ASSERT (fname);

  PVOID volatile *slot = windows_handle_reserve (os, e);
  if (slot == NULL) {
    return error_trace (e);
  }

  // FILE_SHARE_DELETE so remove/unlink of an open file behaves like POSIX
  const HANDLE h = CreateFileA (
      fname,
      access,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
      NULL,
      creation,
      FILE_ATTRIBUTE_NORMAL,
      NULL
  );

  if (unlikely (h == INVALID_HANDLE_VALUE)) {
    // Capture before windows_handle_free can clobber it
    const DWORD err = GetLastError ();
    windows_handle_free (os, slot);
    return win_fail (e, ERR_IO, opname, fname, err);
  }

  windows_handle_store (slot, h);

  dest->table = &default_os_vtable;
  dest->self  = (void *)slot;

  return SUCCESS;
}

err_t
sys_open_file_rw (void *os_self, i_file *dest, const char *fname, error *e)
{
  return sys_open_file (
      os_self,
      dest,
      fname,
      GENERIC_READ | GENERIC_WRITE,
      OPEN_ALWAYS,
      "open_file_rw",
      e
  );
}

err_t
sys_open_file_r (void *os_self, i_file *dest, const char *fname, error *e)
{
  // OPEN_EXISTING: a read-only open fails on a missing file, like O_RDONLY
  return sys_open_file (os_self, dest, fname, GENERIC_READ, OPEN_EXISTING, "open_file_r", e);
}

err_t
sys_open_file_w (void *os_self, i_file *dest, const char *fname, error *e)
{
  return sys_open_file (os_self, dest, fname, GENERIC_WRITE, OPEN_ALWAYS, "open_file_w", e);
}

err_t
sys_close_file (void *os_self, void *file, error *e)
{
  struct windows_os *os   = os_self;
  PVOID volatile    *slot = file;
  ASSERT (os);

  const HANDLE h   = handle_of (file);

  // The handle is gone even if CloseHandle fails - free the slot either way
  const BOOL   ok  = CloseHandle (h);
  const DWORD  err = GetLastError ();

  windows_handle_free (os, slot);

  if (unlikely (!ok)) {
    return win_fail (e, ERR_IO, "close", NULL, err);
  }

  return SUCCESS;
}

err_t
sys_remove_quiet (void *os_self, const char *fname, error *e)
{
  (void)os_self;
  ASSERT (fname);

  if (unlikely (!DeleteFileA (fname))) {
    const DWORD err = GetLastError ();
    if (err != ERROR_FILE_NOT_FOUND && err != ERROR_PATH_NOT_FOUND) {
      return win_fail (e, ERR_IO, "remove", fname, err);
    }
  }

  return SUCCESS;
}

err_t
sys_unlink (void *os_self, const char *fname, error *e)
{
  (void)os_self;
  ASSERT (fname);

  if (unlikely (!DeleteFileA (fname))) {
    return win_fail (e, ERR_IO, "unlink", fname, GetLastError ());
  }

  return SUCCESS;
}

err_t
sys_file_exists (void *os_self, const char *fname, bool *dest, error *e)
{
  (void)os_self;
  ASSERT (fname);
  ASSERT (dest);

  const DWORD attrs = GetFileAttributesA (fname);

  if (attrs == INVALID_FILE_ATTRIBUTES) {
    const DWORD err = GetLastError ();
    if (likely (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND)) {
      *dest = false;
      return SUCCESS;
    }
    return win_fail (e, ERR_IO, "stat", fname, err);
  }

  *dest = !(attrs & FILE_ATTRIBUTE_DIRECTORY);
  return SUCCESS;
}

///////////// System file implementations

err_t
sys_fsync (void *self, error *e)
{
  const HANDLE h = handle_of (self);

  if (unlikely (!FlushFileBuffers (h))) {
    return win_fail (e, ERR_IO, "fsync", NULL, GetLastError ());
  }

  return SUCCESS;
}

i64
sys_file_size (void *self, error *e)
{
  const HANDLE  h = handle_of (self);

  LARGE_INTEGER size;

  if (unlikely (!GetFileSizeEx (h, &size))) {
    win_fail (e, ERR_IO, "file_size", NULL, GetLastError ());
    return error_trace (e);
  }

  return (i64)size.QuadPart;
}

/// Note: on a synchronous handle, ReadFile/WriteFile with an OVERLAPPED
/// offset ALSO move the file pointer (unlike pread/pwrite). Don't mix
/// positional and stream I/O on the same file and expect the stream
/// position to be untouched.
i64
sys_pread_all (void *self, void *dest, const u64 n, const u64 offset, error *e)
{
  const HANDLE h = handle_of (self);

  ASSERT (dest);
  ASSERT (n > 0);

  u8 *_dest = (u8 *)dest;
  u64 nread = 0;

  while (nread < n) {
    const u64  pos = offset + nread;
    OVERLAPPED ov  = {0};
    ov.Offset      = (DWORD)(pos & 0xFFFFFFFFULL);
    ov.OffsetHigh  = (DWORD)(pos >> 32);

    DWORD chunk    = 0;

    if (unlikely (!ReadFile (h, _dest + nread, clamp_dword (n - nread), &chunk, &ov))) {
      const DWORD err = GetLastError ();
      if (likely (err == ERROR_HANDLE_EOF)) {
        return (i64)nread; // offset at or past EOF
      }
      return win_fail (e, ERR_IO, "pread", NULL, err);
    }

    if (chunk == 0) {
      return (i64)nread; // EOF
    }

    nread += chunk;
  }

  ASSERT (nread == n);
  return (i64)nread;
}

err_t
sys_pwrite_all (void *self, const void *src, const u64 n, const u64 offset, error *e)
{
  const HANDLE h = handle_of (self);

  ASSERT (src);
  ASSERT (n > 0);

  const u8 *_src     = (const u8 *)src;
  u64       nwritten = 0;

  while (nwritten < n) {
    const u64  pos = offset + nwritten;
    OVERLAPPED ov  = {0};
    ov.Offset      = (DWORD)(pos & 0xFFFFFFFFULL);
    ov.OffsetHigh  = (DWORD)(pos >> 32);

    DWORD chunk    = 0;

    if (unlikely (!WriteFile (h, _src + nwritten, clamp_dword (n - nwritten), &chunk, &ov))) {
      return win_fail (e, ERR_IO, "pwrite", NULL, GetLastError ());
    }

    if (unlikely (chunk == 0)) {
      return error_causef (e, ERR_IO, "pwrite: wrote 0 bytes");
    }

    nwritten += chunk;
  }

  ASSERT (nwritten == n);
  return SUCCESS;
}

/// No scatter-gather for regular files on Win32 (WriteFileGather needs
/// unbuffered, page-aligned I/O) - write each buffer in turn. The caller's
/// iov is never modified.
err_t
sys_writev_all (void *self, struct bytes *iov, const int iovcnt, error *e)
{
  const HANDLE h = handle_of (self);

  ASSERT (iov);
  ASSERT (iovcnt > 0 && iovcnt <= 2);

  u64 total = 0;
  for (int i = 0; i < iovcnt; i++) {
    total += iov[i].len;
  }

  ASSERT (total > 0);

  u64 nwritten = 0;

  for (int i = 0; i < iovcnt; i++) {
    const u8 *src    = iov[i].head;
    u64       remain = iov[i].len;

    while (remain > 0) {
      DWORD chunk = 0;

      if (unlikely (!WriteFile (h, src, clamp_dword (remain), &chunk, NULL))) {
        return win_fail (e, ERR_IO, "writev", NULL, GetLastError ());
      }

      if (unlikely (chunk == 0)) {
        return error_causef (e, ERR_IO, "writev: wrote 0 bytes");
      }

      src += chunk;
      remain -= chunk;
      nwritten += chunk;
    }
  }

  ASSERT (nwritten == total);
  return SUCCESS;
}

i64
sys_read_all (void *self, void *dest, const u64 nbytes, error *e)
{
  const HANDLE h = handle_of (self);

  ASSERT (dest);
  ASSERT (nbytes > 0);

  u8 *_dest = (u8 *)dest;
  u64 nread = 0;

  while (nread < nbytes) {
    DWORD chunk = 0;

    if (unlikely (!ReadFile (h, _dest + nread, clamp_dword (nbytes - nread), &chunk, NULL))) {
      const DWORD err = GetLastError ();
      if (likely (err == ERROR_HANDLE_EOF || err == ERROR_BROKEN_PIPE)) {
        return (i64)nread; // EOF (file or closed pipe)
      }
      return win_fail (e, ERR_IO, "read", NULL, err);
    }

    if (chunk == 0) {
      return (i64)nread; // EOF
    }

    nread += chunk;
  }

  ASSERT (nread == nbytes);
  return (i64)nread;
}

err_t
sys_write_all (void *self, const void *src, const u64 nbytes, error *e)
{
  const HANDLE h = handle_of (self);

  ASSERT (src);
  ASSERT (nbytes > 0);

  const u8 *_src     = (const u8 *)src;
  u64       nwritten = 0;

  while (nwritten < nbytes) {
    DWORD chunk = 0;

    if (unlikely (!WriteFile (h, _src + nwritten, clamp_dword (nbytes - nwritten), &chunk, NULL))) {
      return win_fail (e, ERR_IO, "write", NULL, GetLastError ());
    }

    if (unlikely (chunk == 0)) {
      return error_causef (e, ERR_IO, "write: wrote 0 bytes");
    }

    nwritten += chunk;
  }

  ASSERT (nwritten == nbytes);
  return SUCCESS;
}

/// Like ftruncate: sets the size without moving the file pointer
/// (SetFilePointerEx + SetEndOfFile would leave it at [bytes]).
err_t
sys_truncate (void *self, const u64 bytes, error *e)
{
  const HANDLE          h   = handle_of (self);

  FILE_END_OF_FILE_INFO eof = {0};
  eof.EndOfFile.QuadPart    = (LONGLONG)bytes;

  if (unlikely (!SetFileInformationByHandle (h, FileEndOfFileInfo, &eof, sizeof (eof)))) {
    return win_fail (e, ERR_IO, "ftruncate", NULL, GetLastError ());
  }

  return SUCCESS;
}

/// Like posix_fallocate: grows the file to at least [bytes], never shrinks
/// it, and leaves the file pointer alone.
err_t
sys_prealloc (void *self, const u64 bytes, error *e)
{
  const HANDLE  h = handle_of (self);

  LARGE_INTEGER size;
  if (unlikely (!GetFileSizeEx (h, &size))) {
    return win_fail (e, ERR_IO, "prealloc (size)", NULL, GetLastError ());
  }

  if ((u64)size.QuadPart >= bytes) {
    return SUCCESS;
  }

  // Best effort: reserve the space first so the extend doesn't fragment.
  // Some filesystems (network shares, FAT) and Wine don't support this - the
  // EOF extension below is what actually matters, so ignore failure here.
  FILE_ALLOCATION_INFO alloc    = {0};
  alloc.AllocationSize.QuadPart = (LONGLONG)bytes;
  (void)SetFileInformationByHandle (h, FileAllocationInfo, &alloc, sizeof (alloc));

  FILE_END_OF_FILE_INFO eof = {0};
  eof.EndOfFile.QuadPart    = (LONGLONG)bytes;
  if (unlikely (!SetFileInformationByHandle (h, FileEndOfFileInfo, &eof, sizeof (eof)))) {
    return win_fail (e, ERR_IO, "prealloc", NULL, GetLastError ());
  }

  return SUCCESS;
}

i64
sys_seek (void *self, const u64 offset, const seek_t whence, error *e)
{
  const HANDLE h = handle_of (self);

  DWORD        method;
  switch (whence) {
    case I_SEEK_SET: {
      method = FILE_BEGIN;
      break;
    }
    case I_SEEK_CUR: {
      method = FILE_CURRENT;
      break;
    }
    case I_SEEK_END: {
      method = FILE_END;
      break;
    }
    default: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }

  LARGE_INTEGER li;
  LARGE_INTEGER result;
  li.QuadPart = (LONGLONG)offset;

  if (unlikely (!SetFilePointerEx (h, li, &result, method))) {
    win_fail (e, ERR_IO, "lseek", NULL, GetLastError ());
    return error_trace (e);
  }

  return (i64)result.QuadPart;
}

/////////////////////////////////////// Threads

/// The frame lives in the OS's thread table until join, so the thread can
/// read func/arg from it directly - no heap-allocated trampoline args.
static DWORD WINAPI
thread_trampoline (LPVOID param)
{
  struct windows_thread_frame *t = param;
  ASSERT (t);
  ASSERT (t->func);

  t->func (t->arg);
  return 0;
}

err_t
sys_thread_create (void *os_self, i_thread *dest, void *(*func) (void *), void *context, error *e)
{
  ASSERT (dest);
  ASSERT (os_self);
  ASSERT (func);

  struct windows_os           *os = os_self;
  struct windows_thread_frame *t  = windows_thread_alloc (os, e);
  if (t == NULL) {
    return error_trace (e);
  }

  t->func   = func;
  t->arg    = context;

  // CreateThread is a synchronization point: the new thread sees func/arg
  t->handle = CreateThread (NULL, 0, thread_trampoline, t, 0, &t->id);

  if (t->handle == NULL) {
    const DWORD err = GetLastError ();
    windows_thread_free (os, t);
    return win_fail (e, ERR_IO, "CreateThread", NULL, err);
  }

  dest->table = &default_os_vtable;
  dest->self  = t;

  return SUCCESS;
}

err_t
sys_thread_join (void *os_self, void *thread, error *e)
{
  struct windows_os           *os = os_self;
  struct windows_thread_frame *t  = thread;
  (void)e; // Unused
  ASSERT (os);
  DBG_ASSERT (thread, t);

  // EDEADLK equivalent: joining yourself waits forever
  if (t->id == GetCurrentThreadId ()) {
    i_log_error ("thread_join: deadlock - thread %lu joining itself\n", (unsigned long)t->id);
    UNREACHABLE (); // LCOV_EXCL_LINE
  }

  const DWORD r = WaitForSingleObject (t->handle, INFINITE);
  if (r != WAIT_OBJECT_0) {
    char buf[WIN_ERR_BUF];
    i_log_error (
        "thread_join: WaitForSingleObject returned %lu: %s\n",
        (unsigned long)r,
        win_strerror (GetLastError (), buf, sizeof (buf))
    );
    UNREACHABLE (); // LCOV_EXCL_LINE
  }

  const BOOL ok = CloseHandle (t->handle);
  ASSERT (ok);
  (void)ok;

  windows_thread_free (os, t);

  return SUCCESS;
}

///////////// System Mutex implementations
//
// CRITICAL_SECTION is recursive by design. The debug checks below make it
// behave like the POSIX PTHREAD_MUTEX_ERRORCHECK build: relocking from the
// owner is a deadlock, unlocking from a non-owner is a bug.

err_t
sys_mutex_create (void *os_self, i_mutex *dest, error *e)
{
  ASSERT (dest);
  ASSERT (os_self);

  struct windows_os *os    = os_self;
  CRITICAL_SECTION  *mutex = windows_mutex_alloc (os, e);
  if (mutex == NULL) {
    return error_trace (e);
  }

  // dwSpinCount=0: no spinning, go straight to a kernel wait.
  // Use a non-zero value (e.g. 4000) if profiling shows contention.
  if (unlikely (!InitializeCriticalSectionAndSpinCount (mutex, 0))) {
    const DWORD err = GetLastError ();
    windows_mutex_free (os, mutex);
    return win_fail (e, ERR_NOMEM, "mutex_init", NULL, err);
  }

  dest->table = &default_os_vtable;
  dest->self  = mutex;

  return SUCCESS;
}

void
sys_mutex_free (void *os_self, void *mutex)
{
  struct windows_os *os = os_self;
  CRITICAL_SECTION  *m  = mutex;
  ASSERT (os);
  DBG_ASSERT (mutex, m);

#  ifndef NDEBUG
  const DWORD owner = cs_owner (m);
  if (owner != 0) {
    i_log_error ("mutex_destroy: still locked by thread %lu\n", (unsigned long)owner);
    UNREACHABLE (); // LCOV_EXCL_LINE
  }
#  endif

  DeleteCriticalSection (m);
  windows_mutex_free (os, m);
}

void
sys_mutex_lock (void *mutex)
{
  CRITICAL_SECTION *m = mutex;
  DBG_ASSERT (mutex, m);

#  ifndef NDEBUG
  const DWORD tid = GetCurrentThreadId ();
  if (cs_owner (m) == tid) {
    i_log_error ("mutex_lock: deadlock - thread %lu already owns mutex\n", (unsigned long)tid);
    UNREACHABLE (); // LCOV_EXCL_LINE
  }
#  endif

  EnterCriticalSection (m);
}

void
sys_mutex_unlock (void *mutex)
{
  CRITICAL_SECTION *m = mutex;
  DBG_ASSERT (mutex, m);

#  ifndef NDEBUG
  const DWORD tid = GetCurrentThreadId ();
  if (cs_owner (m) != tid) {
    i_log_error ("mutex_unlock: not owner - thread %lu\n", (unsigned long)tid);
    UNREACHABLE (); // LCOV_EXCL_LINE
  }
#  endif

  LeaveCriticalSection (m);
}

///////////// System Condition implementations

err_t
sys_cond_create (void *os_self, i_cond *dest, error *e)
{
  ASSERT (dest);
  ASSERT (os_self);

  struct windows_os  *os   = os_self;
  CONDITION_VARIABLE *cond = windows_cond_alloc (os, e);
  if (cond == NULL) {
    return error_trace (e);
  }

  // Cannot fail
  InitializeConditionVariable (cond);

  dest->table = &default_os_vtable;
  dest->self  = cond;

  return SUCCESS;
}

void
sys_cond_free (void *os_self, void *cond)
{
  struct windows_os  *os = os_self;
  CONDITION_VARIABLE *c  = cond;
  ASSERT (os);
  DBG_ASSERT (cond, c);

  // CONDITION_VARIABLE has no destroy function
  windows_cond_free (os, c);
}

void
sys_cond_wait (void *cond, void *mutex)
{
  CONDITION_VARIABLE *c = cond;
  CRITICAL_SECTION   *m = mutex;
  DBG_ASSERT (cond, c);
  DBG_ASSERT (mutex, m);

#  ifndef NDEBUG
  // EPERM equivalent
  if (cs_owner (m) != GetCurrentThreadId ()) {
    i_log_error ("cond_wait: mutex not owned by thread\n");
    UNREACHABLE (); // LCOV_EXCL_LINE
  }
#  endif

  if (!SleepConditionVariableCS (c, m, INFINITE)) {
    char buf[WIN_ERR_BUF];
    i_log_error ("cond_wait: %s\n", win_strerror (GetLastError (), buf, sizeof (buf)));
    UNREACHABLE (); // LCOV_EXCL_LINE
  }
}

void
sys_cond_timed_wait (void *cond, void *mutex, u64 msec)
{
  CONDITION_VARIABLE *c = cond;
  CRITICAL_SECTION   *m = mutex;
  DBG_ASSERT (cond, c);
  DBG_ASSERT (mutex, m);

#  ifndef NDEBUG
  if (cs_owner (m) != GetCurrentThreadId ()) {
    i_log_error ("cond_timed_wait: mutex not owned by thread\n");
    UNREACHABLE (); // LCOV_EXCL_LINE
  }
#  endif

  // INFINITE is 0xFFFFFFFF - clamp below it so a huge timeout stays finite
  const DWORD ms = msec >= INFINITE ? INFINITE - 1 : (DWORD)msec;

  if (!SleepConditionVariableCS (c, m, ms)) {
    const DWORD err = GetLastError ();
    if (err != ERROR_TIMEOUT) {
      char buf[WIN_ERR_BUF];
      i_log_error ("cond_timed_wait: %s\n", win_strerror (err, buf, sizeof (buf)));
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
}

void
sys_cond_signal (void *cond)
{
  CONDITION_VARIABLE *c = cond;
  DBG_ASSERT (cond, c);

  WakeConditionVariable (c);
}

void
sys_cond_broadcast (void *cond)
{
  CONDITION_VARIABLE *c = cond;
  DBG_ASSERT (cond, c);

  WakeAllConditionVariable (c);
}

///////////// System time implementations
//
// QueryPerformanceCounter is the Windows monotonic clock. The frequency is
// fixed at boot, so it's read once per timer. Neither call fails on XP+.

err_t
sys_timer_create (void *os_self, i_timer *dest, error *e)
{
  ASSERT (os_self);
  ASSERT (dest);

  struct windows_os    *os = os_self;
  struct windows_timer *t  = windows_timer_alloc (os, e);
  if (t == NULL) {
    return error_trace (e);
  }

  if (!QueryPerformanceFrequency (&t->frequency) || !QueryPerformanceCounter (&t->start)) {
    const DWORD err = GetLastError ();
    windows_timer_free (os, t);
    return win_fail (e, ERR_IO, "QueryPerformanceCounter", NULL, err);
  }

  DBG_ASSERT (timer, t);

  dest->table = &default_os_vtable;
  dest->self  = t;

  return SUCCESS;
}

void
sys_timer_free (void *os_self, void *timer)
{
  struct windows_os    *os = os_self;
  struct windows_timer *t  = timer;
  ASSERT (os);
  DBG_ASSERT (timer, t);

  windows_timer_free (os, t);
}

u64
sys_timer_now_ns (void *timer)
{
  struct windows_timer *self = timer;
  DBG_ASSERT (timer, self);

  LARGE_INTEGER now;
  const BOOL    ok = QueryPerformanceCounter (&now);
  ASSERT (ok);
  (void)ok;

  const i64 elapsed = now.QuadPart - self->start.QuadPart;
  const i64 freq    = self->frequency.QuadPart;
  ASSERT (elapsed >= 0); // monotonic clock

  // Split into whole seconds + remainder so elapsed * 1e9 can't overflow.
  // rem < freq, so rem * 1e9 is safe for any frequency below ~9.2 GHz.
  const i64 sec = elapsed / freq;
  const i64 rem = elapsed % freq;

  return (u64)(sec * 1000000000LL + (rem * 1000000000LL) / freq);
}

////////////////////////////
/// Run once

// InitOnceExecuteOnce hands the callback its Parameter as a void*. Routing the
// function pointer through this struct avoids casting a function pointer to
// void*, which C doesn't guarantee.
struct once_ctx
{
  void (*fn) (void);
};

static BOOL CALLBACK
once_trampoline (PINIT_ONCE once, PVOID param, PVOID *ctx)
{
  (void)once;
  (void)ctx;

  const struct once_ctx *oc = param;
  oc->fn ();

  return TRUE;
}

void
i_once_run (i_once *once, void (*fn) (void))
{
  ASSERT (once);
  ASSERT (fn);

  struct once_ctx oc = {.fn = fn};
  const BOOL      ok = InitOnceExecuteOnce (once, once_trampoline, &oc, NULL);
  ASSERT (ok);
  (void)ok;
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
    .[27;
1 : 3ucond_timed_wait = sys_cond_timed_wait, .cond_signal = sys_cond_signal,
    .cond_broadcast = sys_cond_broadcast,

    // Timer
    .timer_create = sys_timer_create, .timer_free = sys_timer_free,
    .timer_now_ns = sys_timer_now_ns,
}
;

err_t
system_os_create (struct i_mem mem, struct i_os *dest, error *e)
{
  ASSERT (dest);

  struct windows_os *os = windows_create (mem, e);
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
  windows_free (os.self);
}

#endif // PLATFORM_WINDOWS
