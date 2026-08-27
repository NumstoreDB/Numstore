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
#  include "core/os/ns_file.h"
#  include "core/os/ns_filesystem.h"

#  include <stddef.h>
#  include <stdio.h>
#  include <string.h>

#  define WIN32_LEAN_AND_MEAN
#  include "windows.h"

/******************************************************************************
 * SECTION: Helpers
 ******************************************************************************/

static char *
win32_strerror (DWORD err, char *buf, DWORD buflen)
{
  FormatMessageA (
      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      err,
      0,
      buf,
      buflen,
      NULL
  );
  return buf;
}

#  define WIN_ERR_BUF     256
#  define WIN_ERRMSG(buf) win32_strerror (GetLastError (), buf, sizeof (buf))

static OVERLAPPED
make_overlapped (u64 offset)
{
  OVERLAPPED ov = {0};
  ov.Offset     = (DWORD)(offset & 0xFFFFFFFFULL);
  ov.OffsetHigh = (DWORD)(offset >> 32);
  return ov;
}

#  ifndef NDEBUG
static bool
handle_is_open (HANDLE h)
{
  return h != NULL && h != INVALID_HANDLE_VALUE;
}
#  endif

DEFINE_DBG_ASSERT (i_file, i_file, fp, {
  ASSERT (fp);
  ASSERT (handle_is_open (fp->handle));
})

/******************************************************************************
 * SECTION: File
 ******************************************************************************/

static err_t
win32_close (void *_fp, error *e)
{
  i_file *fp = _fp;
  DBG_ASSERT (i_file, fp);

  if (unlikely (!CloseHandle (fp->handle))) {
    char buf[WIN_ERR_BUF];
    return error_causef (e, ERR_IO, "close: %s", WIN_ERRMSG (buf));
  }

  fp->handle = INVALID_HANDLE_VALUE;
  return SUCCESS;
}

static err_t
win32_fsync (void *_fp, error *e)
{
  i_file *fp = _fp;
  DBG_ASSERT (i_file, fp);

  if (unlikely (!FlushFileBuffers (fp->handle))) {
    char buf[WIN_ERR_BUF];
    return error_causef (e, ERR_IO, "fsync: %s", WIN_ERRMSG (buf));
  }

  return SUCCESS;
}

static i64
win32_file_size (void *_fp, error *e)
{
  i_file *fp = _fp;
  DBG_ASSERT (i_file, fp);

  LARGE_INTEGER size;

  if (unlikely (!GetFileSizeEx (fp->handle, &size))) {
    char buf[WIN_ERR_BUF];
    error_causef (e, ERR_IO, "file_size: %s", WIN_ERRMSG (buf));
    return error_trace (e);
  }

  return (i64)size.QuadPart;
}

////////////////////////////////////////////////////////////
// Positional Read / Write

static i64
win32_pread_all (void *_fp, void *dest, const u64 n, const u64 offset, error *e)
{
  i_file *fp = _fp;
  DBG_ASSERT (i_file, fp);
  ASSERT (dest);
  ASSERT (n > 0);

  u8 *_dest = (u8 *)dest;
  u64 nread = 0;

  while (nread < n) {
    OVERLAPPED ov    = make_overlapped (offset + nread);
    DWORD      chunk = 0;
    DWORD      want  = (DWORD)((n - nread) > 0xFFFFFFFFULL ? 0xFFFFFFFFUL : (n - nread));

    if (unlikely (!ReadFile (fp->handle, _dest + nread, want, &chunk, &ov))) {
      DWORD err = GetLastError ();

      if (likely (err == ERROR_HANDLE_EOF)) {
        return (i64)nread;
      }

      char buf[WIN_ERR_BUF];
      win32_strerror (err, buf, sizeof (buf));

      return error_causef (e, ERR_IO, "pread: %s", buf);
    }

    if (chunk == 0) {
      return (i64)nread; // EOF
    }
    nread += chunk;
  }

  ASSERT (nread == n);
  return (i64)nread;
}

static err_t
win32_pwrite_all (void *_fp, const void *src, const u64 n, const u64 offset, error *e)
{
  i_file *fp = _fp;
  DBG_ASSERT (i_file, fp);
  ASSERT (src);
  ASSERT (n > 0);

  const u8 *_src   = (const u8 *)src;
  u64       nwrite = 0;

  while (nwrite < n) {
    OVERLAPPED ov    = make_overlapped (offset + nwrite);
    DWORD      chunk = 0;
    DWORD      want  = (DWORD)((n - nwrite) > 0xFFFFFFFFULL ? 0xFFFFFFFFUL : (n - nwrite));

    if (unlikely (!WriteFile (fp->handle, _src + nwrite, want, &chunk, &ov))) {
      char buf[WIN_ERR_BUF];
      return error_causef (e, ERR_IO, "pwrite: %s", WIN_ERRMSG (buf));
    }
    nwrite += chunk;
  }

  ASSERT (nwrite == n);
  return SUCCESS;
}

////////////////////////////////////////////////////////////
// IO Vec (no scatter-gather on Windows for regular files - loop per buffer)

static err_t
win32_writev_all (void *_fp, struct bytes *iov, const int iovcnt, error *e)
{
  i_file *fp = _fp;
  DBG_ASSERT (i_file, fp);
  ASSERT (iov);
  ASSERT (iovcnt > 0 && iovcnt <= 2);

  for (int i = 0; i < iovcnt; i++) {
    u8 *src    = (u8 *)iov[i].head;
    u64 remain = iov[i].len;
    while (remain > 0) {
      DWORD want     = (DWORD)(remain > 0xFFFFFFFFULL ? 0xFFFFFFFFUL : remain);
      DWORD nwritten = 0;
      if (unlikely (!WriteFile (fp->handle, src, want, &nwritten, NULL))) {
        char buf[WIN_ERR_BUF];
        return error_causef (e, ERR_IO, "writev: %s", WIN_ERRMSG (buf));
      }
      src += nwritten;
      remain -= nwritten;
    }
  }
  return SUCCESS;
}

////////////////////////////////////////////////////////////
// Stream Read / Write

static i64
win32_read_all (void *_fp, void *dest, const u64 nbytes, error *e)
{
  i_file *fp = _fp;
  DBG_ASSERT (i_file, fp);
  ASSERT (dest);
  ASSERT (nbytes > 0);

  u8 *_dest = (u8 *)dest;
  u64 nread = 0;

  while (nread < nbytes) {
    DWORD chunk = 0;
    DWORD want  = (DWORD)((nbytes - nread) > 0xFFFFFFFFULL ? 0xFFFFFFFFUL : (nbytes - nread));

    if (unlikely (!ReadFile (fp->handle, _dest + nread, want, &chunk, NULL))) {
      DWORD err = GetLastError ();
      if (likely (err == ERROR_HANDLE_EOF)) {
        return (i64)nread;
      }
      char buf[WIN_ERR_BUF];
      win32_strerror (err, buf, sizeof (buf));
      return error_causef (e, ERR_IO, "read: %s", buf);
    }

    if (chunk == 0) {
      return (i64)nread; // EOF
    }
    nread += chunk;
  }

  ASSERT (nread == nbytes);
  return (i64)nread;
}

static err_t
win32_write_all (void *_fp, const void *src, const u64 nbytes, error *e)
{
  i_file *fp = _fp;
  DBG_ASSERT (i_file, fp);
  ASSERT (src);
  ASSERT (nbytes > 0);

  const u8 *_src   = (const u8 *)src;
  u64       nwrite = 0;

  while (nwrite < nbytes) {
    DWORD chunk = 0;
    DWORD want  = (DWORD)((nbytes - nwrite) > 0xFFFFFFFFULL ? 0xFFFFFFFFUL : (nbytes - nwrite));

    if (unlikely (!WriteFile (fp->handle, _src + nwrite, want, &chunk, NULL))) {
      char buf[WIN_ERR_BUF];
      return error_causef (e, ERR_IO, "write: %s", WIN_ERRMSG (buf));
    }

    nwrite += chunk;
  }

  ASSERT (nwrite == nbytes);
  return SUCCESS;
}

////////////////////////////////////////////////////////////
// Other file ops

static err_t
win32_truncate (void *_fp, const u64 bytes, error *e)
{
  i_file *fp = _fp;
  DBG_ASSERT (i_file, fp);

  LARGE_INTEGER li;
  li.QuadPart = (LONGLONG)bytes;
  if (unlikely (!SetFilePointerEx (fp->handle, li, NULL, FILE_BEGIN))) {
    char buf[WIN_ERR_BUF];
    return error_causef (e, ERR_IO, "truncate (seek): %s", WIN_ERRMSG (buf));
  }

  if (unlikely (!SetEndOfFile (fp->handle))) {
    char buf[WIN_ERR_BUF];
    return error_causef (e, ERR_IO, "truncate: %s", WIN_ERRMSG (buf));
  }

  return SUCCESS;
}

static err_t
win32_fallocate (void *_fp, const u64 bytes, error *e)
{
  i_file *fp = _fp;
  DBG_ASSERT (i_file, fp);

  LARGE_INTEGER li;
  li.QuadPart = (LONGLONG)bytes;

  if (unlikely (!SetFilePointerEx (fp->handle, li, NULL, FILE_BEGIN))) {
    char buf[WIN_ERR_BUF];
    return error_causef (e, ERR_IO, "fallocate (seek): %s", WIN_ERRMSG (buf));
  }

  if (unlikely (!SetEndOfFile (fp->handle))) {
    char buf[WIN_ERR_BUF];
    return error_causef (e, ERR_IO, "fallocate: %s", WIN_ERRMSG (buf));
  }

  return SUCCESS;
}

static i64
win32_seek (void *_fp, const u64 offset, const seek_t whence, error *e)
{
  i_file *fp = _fp;
  DBG_ASSERT (i_file, fp);

  DWORD method;
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

  LARGE_INTEGER li, result;
  li.QuadPart = (LONGLONG)offset;

  if (unlikely (!SetFilePointerEx (fp->handle, li, &result, method))) {
    char buf[WIN_ERR_BUF];
    error_causef (e, ERR_IO, "seek: %s", WIN_ERRMSG (buf));
    return error_trace (e);
  }

  return (i64)result.QuadPart;
}

static struct i_file_vtable win32_file_vtable = {
    .close      = win32_close,
    .fsync      = win32_fsync,
    .file_size  = win32_file_size,
    .read_all   = win32_read_all,
    .pread_all  = win32_pread_all,
    .write_all  = win32_write_all,
    .pwrite_all = win32_pwrite_all,
    .writev_all = win32_writev_all,
    .truncate   = win32_truncate,
    .fallocate  = win32_fallocate,
    .seek       = win32_seek,
#  ifdef TESTING
    .test_data = NULL,
#  endif
};

i_file
create_default_file (HANDLE h)
{
  return (i_file){
      .handle = h,
      .table  = &win32_file_vtable,
  };
}

/******************************************************************************
 * SECTION: File System
 ******************************************************************************/
// vfs parameter unused; Win32 FS operations are stateless.

static err_t
win32_open_rw (void *vfs, i_file *dest, const char *fname, error *e)
{
  (void)vfs;
  HANDLE h = CreateFileA (
      fname,
      GENERIC_READ | GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE,
      NULL,
      OPEN_ALWAYS,
      FILE_ATTRIBUTE_NORMAL,
      NULL
  );

  if (unlikely (h == INVALID_HANDLE_VALUE)) {
    char buf[WIN_ERR_BUF];
    return error_causef (e, ERR_IO, "open_rw %s: %s", fname, WIN_ERRMSG (buf));
  }

  *dest = create_default_file (h);
  return SUCCESS;
}

static err_t
win32_open_r (void *vfs, i_file *dest, const char *fname, error *e)
{
  (void)vfs;
  HANDLE h = CreateFileA (
      fname,
      GENERIC_READ,
      FILE_SHARE_READ | FILE_SHARE_WRITE,
      NULL,
      OPEN_ALWAYS,
      FILE_ATTRIBUTE_NORMAL,
      NULL
  );

  if (unlikely (h == INVALID_HANDLE_VALUE)) {
    char buf[WIN_ERR_BUF];
    return error_causef (e, ERR_IO, "open_r %s: %s", fname, WIN_ERRMSG (buf));
  }

  *dest = create_default_file (h);
  return SUCCESS;
}

static err_t
win32_open_w (void *vfs, i_file *dest, const char *fname, error *e)
{
  (void)vfs;
  HANDLE h = CreateFileA (
      fname,
      GENERIC_WRITE,
      FILE_SHARE_WRITE | FILE_SHARE_READ,
      NULL,
      OPEN_ALWAYS,
      FILE_ATTRIBUTE_NORMAL,
      NULL
  );

  if (unlikely (h == INVALID_HANDLE_VALUE)) {
    char buf[WIN_ERR_BUF];
    return error_causef (e, ERR_IO, "open_w %s: %s", fname, WIN_ERRMSG (buf));
  }

  *dest = create_default_file (h);
  return SUCCESS;
}

static err_t
win32_remove_quiet (void *vfs, const char *fname, error *e)
{
  (void)vfs;
  if (unlikely (!DeleteFileA (fname))) {
    DWORD err = GetLastError ();
    if (err != ERROR_FILE_NOT_FOUND && err != ERROR_PATH_NOT_FOUND) {
      char buf[WIN_ERR_BUF];
      win32_strerror (err, buf, sizeof (buf));
      error_causef (e, ERR_IO, "remove: %s", buf);
      return error_trace (e);
    }
  }
  return SUCCESS;
}

static err_t
win32_unlink (void *vfs, const char *name, error *e)
{
  (void)vfs;
  if (unlikely (!DeleteFileA (name))) {
    char buf[WIN_ERR_BUF];
    error_causef (e, ERR_IO, "unlink: %s", WIN_ERRMSG (buf));
    return error_trace (e);
  }
  return SUCCESS;
}

static err_t
win32_file_exists (void *vfs, const char *fname, bool *dest, error *e)
{
  (void)vfs;
  DWORD attrs = GetFileAttributesA (fname);

  if (unlikely (attrs == INVALID_FILE_ATTRIBUTES)) {
    DWORD err = GetLastError ();

    if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) {
      *dest = false;
      return SUCCESS;
    }

    char buf[WIN_ERR_BUF];
    win32_strerror (err, buf, sizeof (buf));
    error_causef (e, ERR_IO, "stat %s: %s", fname, buf);
    return error_trace (e);
  }

  *dest = !(attrs & FILE_ATTRIBUTE_DIRECTORY);

  return SUCCESS;
}

static const struct i_file_system_vtable win32_fsvtable = {
    .open_rw      = win32_open_rw,
    .open_r       = win32_open_r,
    .open_w       = win32_open_w,
    .remove_quiet = win32_remove_quiet,
    .unlink       = win32_unlink,
    .file_exists  = win32_file_exists,
};

struct i_file_system
default_filesystem (void)
{
  return (struct i_file_system){
      .table = &win32_fsvtable,
      .data  = NULL,
  };
}

#endif
