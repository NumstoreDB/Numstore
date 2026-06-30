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

#include "platform.h"

#if PLATFORM_WINDOWS

#  include <stddef.h>
#  include <stdio.h>
#  include <string.h>

#  include "csx_assert.h"
#  include "error.h" // error
#  include "os.h"
#  include "serial.h"

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
 * SECTION: File System
 ******************************************************************************/

static err_t
win32_close (i_file *fp, error *e)
{
  DBG_ASSERT (i_file, fp);

  if (unlikely (!CloseHandle (fp->handle)))
  {
    char buf[WIN_ERR_BUF];
    return error_causef (e, ERR_IO, "close: %s", WIN_ERRMSG (buf));
  }

  fp->handle = INVALID_HANDLE_VALUE;
  return SUCCESS;
}

static err_t
win32_fsync (const i_file *fp, error *e)
{
  DBG_ASSERT (i_file, fp);

  if (unlikely (!FlushFileBuffers (fp->handle)))
  {
    char buf[WIN_ERR_BUF];
    return error_causef (e, ERR_IO, "fsync: %s", WIN_ERRMSG (buf));
  }

  return SUCCESS;
}

static i64
win32_file_size (const i_file *fp, error *e)
{
  DBG_ASSERT (i_file, fp);

  LARGE_INTEGER size;

  if (unlikely (!GetFileSizeEx (fp->handle, &size)))
  {
    char buf[WIN_ERR_BUF];
    error_causef (e, ERR_IO, "file_size: %s", WIN_ERRMSG (buf));
    return error_trace (e);
  }

  return (i64)size.QuadPart;
}

////////////////////////////////////////////////////////////
// Positional Read / Write

static i64
win32_pread_some (
    const i_file *fp,
    void         *dest,
    const u64     n,
    const u64     offset,
    error        *e
)
{
  DBG_ASSERT (i_file, fp);
  ASSERT (dest);
  ASSERT (n > 0);

  OVERLAPPED ov    = make_overlapped (offset);
  DWORD      nread = 0;

  if (unlikely (!ReadFile (fp->handle, dest, (DWORD)n, &nread, &ov)))
  {
    DWORD err = GetLastError ();

    if (likely (err == ERROR_HANDLE_EOF))
    {
      return 0;
    }

    char buf[WIN_ERR_BUF];
    win32_strerror (err, buf, sizeof (buf));

    return error_causef (e, ERR_IO, "pread: %s", buf);
  }
  return (i64)nread;
}

static i64
win32_pread_all (
    const i_file *fp,
    void         *dest,
    const u64     n,
    const u64     offset,
    error        *e
)
{
  DBG_ASSERT (i_file, fp);
  ASSERT (dest);
  ASSERT (n > 0);

  u8 *_dest = (u8 *)dest;
  u64 nread = 0;

  while (nread < n)
  {
    OVERLAPPED ov    = make_overlapped (offset + nread);
    DWORD      chunk = 0;
    DWORD      want =
        (DWORD)((n - nread) > 0xFFFFFFFFULL ? 0xFFFFFFFFUL : (n - nread));

    if (unlikely (!ReadFile (fp->handle, _dest + nread, want, &chunk, &ov)))
    {
      DWORD err = GetLastError ();

      if (likely (err == ERROR_HANDLE_EOF))
      {
        return (i64)nread;
      }

      char buf[WIN_ERR_BUF];
      win32_strerror (err, buf, sizeof (buf));

      return error_causef (e, ERR_IO, "pread: %s", buf);
    }

    if (chunk == 0)
    {
      return (i64)nread; // EOF
    }
    nread += chunk;
  }

  ASSERT (nread == n);
  return (i64)nread;
}

static i64
win32_pwrite_some (
    const i_file *fp,
    const void   *src,
    const u64     n,
    const u64     offset,
    error        *e
)
{
  DBG_ASSERT (i_file, fp);
  ASSERT (src);
  ASSERT (n > 0);

  OVERLAPPED ov       = make_overlapped (offset);
  DWORD      nwritten = 0;

  if (unlikely (!WriteFile (fp->handle, src, (DWORD)n, &nwritten, &ov)))
  {
    char buf[WIN_ERR_BUF];
    return error_causef (e, ERR_IO, "pwrite: %s", WIN_ERRMSG (buf));
  }

  return (i64)nwritten;
}

static err_t
win32_pwrite_all (
    const i_file *fp,
    const void   *src,
    const u64     n,
    const u64     offset,
    error        *e
)
{
  DBG_ASSERT (i_file, fp);
  ASSERT (src);
  ASSERT (n > 0);

  const u8 *_src   = (const u8 *)src;
  u64       nwrite = 0;

  while (nwrite < n)
  {
    OVERLAPPED ov    = make_overlapped (offset + nwrite);
    DWORD      chunk = 0;
    DWORD      want =
        (DWORD)((n - nwrite) > 0xFFFFFFFFULL ? 0xFFFFFFFFUL : (n - nwrite));

    if (unlikely (!WriteFile (fp->handle, _src + nwrite, want, &chunk, &ov)))
    {
      char buf[WIN_ERR_BUF];
      return error_causef (e, ERR_IO, "pwrite: %s", WIN_ERRMSG (buf));
    }
    nwrite += chunk;
  }

  ASSERT (nwrite == n);
  return SUCCESS;
}

////////////////////////////////////////////////////////////
// IO Vec (no scatter-gather on Windows for regular files — loop per buffer)

static i64
win32_writev_some (
    const i_file       *fp,
    const struct bytes *src,
    const int           iovcnt,
    error              *e
)
{
  DBG_ASSERT (i_file, fp);
  ASSERT (src);
  ASSERT (iovcnt > 0 && iovcnt <= 2);

  i64 total = 0;
  for (int i = 0; i < iovcnt; i++)
  {
    DWORD nwritten = 0;
    if (unlikely (!WriteFile (
            fp->handle,
            src[i].head,
            (DWORD)src[i].len,
            &nwritten,
            NULL
        )))
    {
      char buf[WIN_ERR_BUF];
      return error_causef (e, ERR_IO, "writev: %s", WIN_ERRMSG (buf));
    }

    total += nwritten;
    if ((u64)nwritten < src[i].len)
    {
      break; // partial
    }
  }
  return total;
}

static err_t
win32_writev_all (
    const i_file *fp,
    struct bytes *iov,
    const int     iovcnt,
    error        *e
)
{
  DBG_ASSERT (i_file, fp);
  ASSERT (iov);
  ASSERT (iovcnt > 0 && iovcnt <= 2);

  for (int i = 0; i < iovcnt; i++)
  {
    u8 *src    = (u8 *)iov[i].head;
    u64 remain = iov[i].len;
    while (remain > 0)
    {
      DWORD want     = (DWORD)(remain > 0xFFFFFFFFULL ? 0xFFFFFFFFUL : remain);
      DWORD nwritten = 0;
      if (unlikely (!WriteFile (fp->handle, src, want, &nwritten, NULL)))
      {
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
win32_read_some (const i_file *fp, void *dest, const u64 nbytes, error *e)
{
  DBG_ASSERT (i_file, fp);
  ASSERT (dest);
  ASSERT (nbytes > 0);

  DWORD nread = 0;
  if (unlikely (!ReadFile (fp->handle, dest, (DWORD)nbytes, &nread, NULL)))
  {
    DWORD err = GetLastError ();
    if (likely (err == ERROR_HANDLE_EOF))
    {
      return 0;
    }
    char buf[WIN_ERR_BUF];
    win32_strerror (err, buf, sizeof (buf));
    return error_causef (e, ERR_IO, "read: %s", buf);
  }
  return (i64)nread;
}

static i64
win32_read_all (const i_file *fp, void *dest, const u64 nbytes, error *e)
{
  DBG_ASSERT (i_file, fp);
  ASSERT (dest);
  ASSERT (nbytes > 0);

  u8 *_dest = (u8 *)dest;
  u64 nread = 0;

  while (nread < nbytes)
  {
    DWORD chunk = 0;
    DWORD want  = (DWORD)((nbytes - nread) > 0xFFFFFFFFULL ? 0xFFFFFFFFUL
                                                           : (nbytes - nread));

    if (unlikely (!ReadFile (fp->handle, _dest + nread, want, &chunk, NULL)))
    {
      DWORD err = GetLastError ();
      if (likely (err == ERROR_HANDLE_EOF))
      {
        return (i64)nread;
      }
      char buf[WIN_ERR_BUF];
      win32_strerror (err, buf, sizeof (buf));
      return error_causef (e, ERR_IO, "read: %s", buf);
    }

    if (chunk == 0)
    {
      return (i64)nread; // EOF
    }
    nread += chunk;
  }

  ASSERT (nread == nbytes);
  return (i64)nread;
}

static i64
win32_write_some (const i_file *fp, const void *src, const u64 nbytes, error *e)
{
  DBG_ASSERT (i_file, fp);
  ASSERT (src);
  ASSERT (nbytes > 0);

  DWORD nwritten = 0;
  if (unlikely (!WriteFile (fp->handle, src, (DWORD)nbytes, &nwritten, NULL)))
  {
    char buf[WIN_ERR_BUF];
    return error_causef (e, ERR_IO, "write: %s", WIN_ERRMSG (buf));
  }
  return (i64)nwritten;
}

static err_t
win32_write_all (const i_file *fp, const void *src, const u64 nbytes, error *e)
{
  DBG_ASSERT (i_file, fp);
  ASSERT (src);
  ASSERT (nbytes > 0);

  const u8 *_src   = (const u8 *)src;
  u64       nwrite = 0;

  while (nwrite < nbytes)
  {
    DWORD chunk = 0;
    DWORD want = (DWORD)((nbytes - nwrite) > 0xFFFFFFFFULL ? 0xFFFFFFFFUL
                                                           : (nbytes - nwrite));

    if (unlikely (!WriteFile (fp->handle, _src + nwrite, want, &chunk, NULL)))
    {
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
win32_truncate (const i_file *fp, const u64 bytes, error *e)
{
  DBG_ASSERT (i_file, fp);

  LARGE_INTEGER li;
  li.QuadPart = (LONGLONG)bytes;
  if (unlikely (!SetFilePointerEx (fp->handle, li, NULL, FILE_BEGIN)))
  {
    char buf[WIN_ERR_BUF];
    return error_causef (e, ERR_IO, "truncate (seek): %s", WIN_ERRMSG (buf));
  }

  if (unlikely (!SetEndOfFile (fp->handle)))
  {
    char buf[WIN_ERR_BUF];
    return error_causef (e, ERR_IO, "truncate: %s", WIN_ERRMSG (buf));
  }

  return SUCCESS;
}

static err_t
win32_fallocate (i_file *fp, const u64 bytes, error *e)
{
  DBG_ASSERT (i_file, fp);

  LARGE_INTEGER li;
  li.QuadPart = (LONGLONG)bytes;

  if (unlikely (!SetFilePointerEx (fp->handle, li, NULL, FILE_BEGIN)))
  {
    char buf[WIN_ERR_BUF];
    return error_causef (e, ERR_IO, "fallocate (seek): %s", WIN_ERRMSG (buf));
  }

  if (unlikely (!SetEndOfFile (fp->handle)))
  {
    char buf[WIN_ERR_BUF];
    return error_causef (e, ERR_IO, "fallocate: %s", WIN_ERRMSG (buf));
  }

  return SUCCESS;
}

static i64
win32_seek (const i_file *fp, const u64 offset, const seek_t whence, error *e)
{
  DBG_ASSERT (i_file, fp);

  DWORD method;
  switch (whence)
  {
    case I_SEEK_SET:
    {
      method = FILE_BEGIN;
      break;
    }
    case I_SEEK_CUR:
    {
      method = FILE_CURRENT;
      break;
    }
    case I_SEEK_END:
    {
      method = FILE_END;
      break;
    }
    default:
    {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }

  LARGE_INTEGER li, result;
  li.QuadPart = (LONGLONG)offset;

  if (unlikely (!SetFilePointerEx (fp->handle, li, &result, method)))
  {
    char buf[WIN_ERR_BUF];
    error_causef (e, ERR_IO, "seek: %s", WIN_ERRMSG (buf));
    return error_trace (e);
  }

  return (i64)result.QuadPart;
}

////////////////////////////////////////////////////////////
// File system vtable — Win32 implementations
// vfs parameter unused; Win32 FS operations are stateless.

static err_t
win32_open_rw (
    i_file_system_vtable *vfs,
    i_file               *dest,
    const char           *fname,
    error                *e
)
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

  if (unlikely (h == INVALID_HANDLE_VALUE))
  {
    char buf[WIN_ERR_BUF];
    return error_causef (e, ERR_IO, "open_rw %s: %s", fname, WIN_ERRMSG (buf));
  }

  *dest = (i_file){
      .handle   = h,
      .fvtable  = &default_fvtable,
      .fsvtable = &default_fsvtable,
  };
  DBG_ASSERT (i_file, dest);

  return SUCCESS;
}

static err_t
win32_open_r (
    i_file_system_vtable *vfs,
    i_file               *dest,
    const char           *fname,
    error                *e
)
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

  if (unlikely (h == INVALID_HANDLE_VALUE))
  {
    char buf[WIN_ERR_BUF];
    return error_causef (e, ERR_IO, "open_r %s: %s", fname, WIN_ERRMSG (buf));
  }

  *dest = (i_file){
      .handle   = h,
      .fvtable  = &default_fvtable,
      .fsvtable = &default_fsvtable,
  };
  DBG_ASSERT (i_file, dest);

  return SUCCESS;
}

static err_t
win32_open_w (
    i_file_system_vtable *vfs,
    i_file               *dest,
    const char           *fname,
    error                *e
)
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

  if (unlikely (h == INVALID_HANDLE_VALUE))
  {
    char buf[WIN_ERR_BUF];
    return error_causef (e, ERR_IO, "open_w %s: %s", fname, WIN_ERRMSG (buf));
  }

  *dest = (i_file){
      .handle   = h,
      .fvtable  = &default_fvtable,
      .fsvtable = &default_fsvtable,
  };
  DBG_ASSERT (i_file, dest);

  return SUCCESS;
}

static err_t
win32_remove_quiet (i_file_system_vtable *vfs, const char *fname, error *e)
{
  (void)vfs;
  if (unlikely (!DeleteFileA (fname)))
  {
    DWORD err = GetLastError ();
    if (err != ERROR_FILE_NOT_FOUND && err != ERROR_PATH_NOT_FOUND)
    {
      char buf[WIN_ERR_BUF];
      win32_strerror (err, buf, sizeof (buf));
      error_causef (e, ERR_IO, "remove: %s", buf);
      return error_trace (e);
    }
  }
  return SUCCESS;
}

static err_t
win32_unlink (i_file_system_vtable *vfs, const char *name, error *e)
{
  (void)vfs;
  if (unlikely (!DeleteFileA (name)))
  {
    char buf[WIN_ERR_BUF];
    error_causef (e, ERR_IO, "unlink: %s", WIN_ERRMSG (buf));
    return error_trace (e);
  }
  return SUCCESS;
}

static err_t
win32_mkdir (i_file_system_vtable *vfs, const char *name, error *e)
{
  (void)vfs;
  if (unlikely (!CreateDirectoryA (name, NULL)))
  {
    char buf[WIN_ERR_BUF];
    error_causef (e, ERR_IO, "mkdir: %s", WIN_ERRMSG (buf));
    return error_trace (e);
  }
  return SUCCESS;
}

static err_t
win32_mkdir_quiet (i_file_system_vtable *vfs, const char *name, error *e)
{
  (void)vfs;
  if (CreateDirectoryA (name, NULL))
  {
    return SUCCESS;
  }

  DWORD err = GetLastError ();
  if (unlikely (err != ERROR_ALREADY_EXISTS))
  {
    char buf[WIN_ERR_BUF];
    win32_strerror (err, buf, sizeof (buf));
    error_causef (e, ERR_IO, "mkdir: %s", buf);
    return error_trace (e);
  }

  DWORD attrs = GetFileAttributesA (name);
  if (unlikely (attrs == INVALID_FILE_ATTRIBUTES))
  {
    char buf[WIN_ERR_BUF];
    error_causef (e, ERR_IO, "mkdir_quiet (stat): %s", WIN_ERRMSG (buf));
    return error_trace (e);
  }

  if (unlikely (!(attrs & FILE_ATTRIBUTE_DIRECTORY)))
  {
    error_causef (
        e,
        ERR_IO,
        "mkdir_quiet: %s exists but is not a directory",
        name
    );
    return error_trace (e);
  }

  return SUCCESS;
}

static err_t
win32_rm_rf (i_file_system_vtable *vfs, const char *path, error *e)
{
  (void)vfs;

  char pattern[MAX_PATH];
  snprintf (pattern, sizeof (pattern), "%s\\*", path);

  WIN32_FIND_DATAA fd;
  HANDLE           hFind = FindFirstFileA (pattern, &fd);

  if (unlikely (hFind == INVALID_HANDLE_VALUE))
  {
    DWORD err = GetLastError ();
    if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND)
    {
      return SUCCESS;
    }
    char buf[WIN_ERR_BUF];
    win32_strerror (err, buf, sizeof (buf));
    error_causef (e, ERR_IO, "opendir %s: %s", path, buf);
    return error_trace (e);
  }

  do
  {
    if (strcmp (fd.cFileName, ".") == 0 || strcmp (fd.cFileName, "..") == 0)
    {
      continue;
    }
    char child[MAX_PATH];
    snprintf (child, sizeof (child), "%s\\%s", path, fd.cFileName);
    if (unlikely (!DeleteFileA (child)))
    {
      DWORD err = GetLastError ();
      if (err != ERROR_FILE_NOT_FOUND)
      {
        FindClose (hFind);
        char buf[WIN_ERR_BUF];
        win32_strerror (err, buf, sizeof (buf));
        error_causef (e, ERR_IO, "unlink %s: %s", child, buf);
        return error_trace (e);
      }
    }
  }
  while (FindNextFileA (hFind, &fd));

  FindClose (hFind);

  if (unlikely (!RemoveDirectoryA (path)))
  {
    DWORD err = GetLastError ();
    if (err != ERROR_FILE_NOT_FOUND && err != ERROR_PATH_NOT_FOUND)
    {
      char buf[WIN_ERR_BUF];
      win32_strerror (err, buf, sizeof (buf));
      error_causef (e, ERR_IO, "rmdir %s: %s", path, buf);
      return error_trace (e);
    }
  }

  return SUCCESS;
}

static err_t
win32_access_rw (i_file_system_vtable *vfs, const char *fname, error *e)
{
  (void)vfs;
  if (unlikely (GetFileAttributesA (fname) == INVALID_FILE_ATTRIBUTES))
  {
    char buf[WIN_ERR_BUF];
    error_causef (e, ERR_IO, "access: %s", WIN_ERRMSG (buf));
    return error_trace (e);
  }
  return SUCCESS;
}

static bool
win32_exists_rw (i_file_system_vtable *vfs, const char *fname)
{
  (void)vfs;
  return GetFileAttributesA (fname) != INVALID_FILE_ATTRIBUTES;
}

static err_t
win32_touch (i_file_system_vtable *vfs, const char *fname, error *e)
{
  ASSERT (fname);
  i_file fd = {0};
  WRAP (vfs->i_open_rw (vfs, &fd, fname, e));
  WRAP (fd.fvtable->i_close (&fd, e));
  return SUCCESS;
}

static err_t
win32_dir_exists (
    i_file_system_vtable *vfs,
    const char           *fname,
    bool                 *dest,
    error                *e
)
{
  (void)vfs;
  DWORD attrs = GetFileAttributesA (fname);

  if (unlikely (attrs == INVALID_FILE_ATTRIBUTES))
  {
    DWORD err = GetLastError ();

    if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND)
    {
      *dest = false;
      return SUCCESS;
    }

    char buf[WIN_ERR_BUF];
    win32_strerror (err, buf, sizeof (buf));
    error_causef (e, ERR_IO, "stat %s: %s", fname, buf);
    return error_trace (e);
  }
  *dest = (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
  return SUCCESS;
}

static err_t
win32_file_exists (
    i_file_system_vtable *vfs,
    const char           *fname,
    bool                 *dest,
    error                *e
)
{
  (void)vfs;
  DWORD attrs = GetFileAttributesA (fname);

  if (unlikely (attrs == INVALID_FILE_ATTRIBUTES))
  {
    DWORD err = GetLastError ();

    if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND)
    {
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

////////////////////////////////////////////////////////////
// Default file system vtable

struct i_file_vtable default_fvtable = {
    .i_close       = win32_close,
    .i_fsync       = win32_fsync,
    .i_file_size   = win32_file_size,
    .i_read_some   = win32_read_some,
    .i_read_all    = win32_read_all,
    .i_pread_some  = win32_pread_some,
    .i_pread_all   = win32_pread_all,
    .i_write_some  = win32_write_some,
    .i_write_all   = win32_write_all,
    .i_pwrite_some = win32_pwrite_some,
    .i_pwrite_all  = win32_pwrite_all,
    .i_writev_some = win32_writev_some,
    .i_writev_all  = win32_writev_all,
    .i_truncate    = win32_truncate,
    .i_fallocate   = win32_fallocate,
    .i_seek        = win32_seek,
};

struct i_file_system_vtable default_fsvtable = {
    .i_open_rw      = win32_open_rw,
    .i_open_r       = win32_open_r,
    .i_open_w       = win32_open_w,
    .i_remove_quiet = win32_remove_quiet,
    .i_unlink       = win32_unlink,
    .i_mkdir        = win32_mkdir,
    .i_mkdir_quiet  = win32_mkdir_quiet,
    .i_rm_rf        = win32_rm_rf,
    .i_access_rw    = win32_access_rw,
    .i_exists_rw    = win32_exists_rw,
    .i_touch        = win32_touch,
    .i_dir_exists   = win32_dir_exists,
    .i_file_exists  = win32_file_exists,
};

/******************************************************************************
 * SECTION: Threading
 ******************************************************************************/

/*-----------------------------------------------------------------------------
 * SUBSECTION: Condition Variable
 *----------------------------------------------------------------------------*/

static err_t
win32_cond_create (i_threading *t, i_cond *c, error *e)
{
  (void)t;
  ASSERT (c);
  (void)e;
  InitializeConditionVariable (&c->cond);
  return SUCCESS;
}

static void
win32_cond_free (i_threading *t, i_cond *c)
{
  (void)t;
  ASSERT (c);
  // No-op: CONDITION_VARIABLE has no destroy function.
}

static void
win32_cond_wait (i_threading *t, i_cond *c, i_mutex *m)
{
  (void)t;
  ASSERT (c);
  ASSERT (m);

  if (!SleepConditionVariableCS (&c->cond, &m->m, INFINITE))
  {
    i_log_error (
        "cond_wait: SleepConditionVariableCS failed: %lu\n",
        GetLastError ()
    );
    UNREACHABLE (); // LCOV_EXCL_LINE
  }
}

static void
win32_cond_timed_wait (i_threading *t, i_cond *c, i_mutex *m, u64 msec)
{
  (void)t;
  ASSERT (c);
  ASSERT (m);
  if (!SleepConditionVariableCS (&c->cond, &m->m, (DWORD)msec))
  {
    DWORD err = GetLastError ();
    if (err != ERROR_TIMEOUT)
    {
      i_log_error (
          "cond_timed_wait: SleepConditionVariableCS failed: %lu\n",
          err
      );
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
}

static void
win32_cond_signal (i_threading *t, i_cond *c)
{
  (void)t;
  ASSERT (c);
  WakeConditionVariable (&c->cond);
}

static void
win32_cond_broadcast (i_threading *t, i_cond *c)
{
  (void)t;
  ASSERT (c);
  WakeAllConditionVariable (&c->cond);
}

/*-----------------------------------------------------------------------------
 * SUBSECTION: Mutex
 *----------------------------------------------------------------------------*/

#  ifndef NDEBUG
static DWORD
cs_owner (i_mutex *m)
{
  // OwningThread is documented as a HANDLE but is actually the
  // thread ID cast to a HANDLE on all shipping Windows versions.
  return (DWORD)(uintptr_t)((CRITICAL_SECTION *)&m->m)->OwningThread;
}
#  endif

static err_t
win32_mutex_create (i_threading *t, i_mutex *dest, error *e)
{
  (void)t;
  ASSERT (dest);
  (void)e;
  // dwSpinCount=0: no spinning, go straight to kernel wait.
  // Use a non-zero value (e.g. 4000) if profiling shows contention.
  InitializeCriticalSectionAndSpinCount (&dest->m, 0);
  return SUCCESS;
}

static void
win32_mutex_free (i_threading *t, i_mutex *m)
{
  (void)t;
  ASSERT (m);
#  ifndef NDEBUG
  DWORD owner = cs_owner (m);
  if (owner != 0)
  {
    i_log_error ("mutex_destroy: still locked by thread %lu\n", owner);
    UNREACHABLE (); // LCOV_EXCL_LINE
  }
#  endif
  DeleteCriticalSection (&m->m);
}

static void
win32_mutex_lock (i_threading *t, i_mutex *m)
{
  (void)t;
  ASSERT (m);
#  ifndef NDEBUG
  DWORD tid = GetCurrentThreadId ();
  if (cs_owner (m) == tid)
  {
    i_log_error ("mutex_lock: deadlock — thread %lu already owns mutex\n", tid);
    UNREACHABLE (); // LCOV_EXCL_LINE
  }
#  endif
  EnterCriticalSection (&m->m);
}

static void
win32_mutex_unlock (i_threading *t, i_mutex *m)
{
  (void)t;
  ASSERT (m);
#  ifndef NDEBUG
  DWORD tid = GetCurrentThreadId ();
  if (cs_owner (m) != tid)
  {
    i_log_error ("mutex_unlock: thread %lu does not own mutex\n", tid);
    UNREACHABLE (); // LCOV_EXCL_LINE
  }
#  endif
  LeaveCriticalSection (&m->m);
}

/*-----------------------------------------------------------------------------
 * SUBSECTION: Threading
 *----------------------------------------------------------------------------*/

typedef struct
{
  void *(*func) (void *);
  void *arg;
} thread_trampoline_args;

static DWORD WINAPI
thread_trampoline (LPVOID param)
{
  thread_trampoline_args *args = (thread_trampoline_args *)param;
  void *(*func) (void *)       = args->func;
  void *arg                    = args->arg;
  HeapFree (GetProcessHeap (), 0, args);
  func (arg);
  return 0;
}

static err_t
win32_thread_create (
    i_threading *t,
    i_thread    *dest,
    void *(*func) (void *),
    void  *context,
    error *e
)
{
  (void)t;
  ASSERT (dest);

  thread_trampoline_args *args = HeapAlloc (GetProcessHeap (), 0, sizeof *args);
  if (!args)
  {
    return error_causef (e, ERR_NOMEM, "thread_create: HeapAlloc failed");
  }

  args->func = func;
  args->arg  = context;

  dest->handle = CreateThread (NULL, 0, thread_trampoline, args, 0, &dest->id);
  if (!dest->handle)
  {
    HeapFree (GetProcessHeap (), 0, args);
    char buf[256];
    FormatMessageA (
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError (),
        0,
        buf,
        sizeof (buf),
        NULL
    );
    return error_causef (e, ERR_IO, "CreateThread: %s", buf);
  }

  return SUCCESS;
}

static err_t
win32_thread_join (i_threading *t, i_thread *th, error *e)
{
  (void)t;
  ASSERT (th);
  ASSERT (th->handle);

  DWORD ret = WaitForSingleObject (th->handle, INFINITE);
  if (ret != WAIT_OBJECT_0)
  {
    char buf[256];
    FormatMessageA (
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError (),
        0,
        buf,
        sizeof (buf),
        NULL
    );
    return error_causef (e, ERR_IO, "thread_join: %s", buf);
  }

  CloseHandle (th->handle);
  th->handle = NULL;
  return SUCCESS;
}

////////////////////////////////////////////////////////////
// Default threading vtable

struct i_threading default_threading = {
    .i_thread_create = win32_thread_create,
    .i_thread_join   = win32_thread_join,

    .i_mutex_create = win32_mutex_create,
    .i_mutex_free   = win32_mutex_free,
    .i_mutex_lock   = win32_mutex_lock,
    .i_mutex_unlock = win32_mutex_unlock,

    .i_cond_create     = win32_cond_create,
    .i_cond_free       = win32_cond_free,
    .i_cond_wait       = win32_cond_wait,
    .i_cond_timed_wait = win32_cond_timed_wait,
    .i_cond_signal     = win32_cond_signal,
    .i_cond_broadcast  = win32_cond_broadcast,
};

////////////////////////////////////////////////////////////
// Timing
//
// QueryPerformanceCounter is the Windows monotonic clock.
// QueryPerformanceFrequency returns ticks/sec and is fixed
// at boot — safe to query once per timer.

err_t
i_timer_create (i_timer *timer, error *e)
{
  ASSERT (timer);

  if (!QueryPerformanceFrequency (&timer->frequency))
  {
    char buf[256];
    FormatMessageA (
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError (),
        0,
        buf,
        sizeof (buf),
        NULL
    );
    return error_causef (e, ERR_IO, "QueryPerformanceFrequency: %s", buf);
  }

  if (!QueryPerformanceCounter (&timer->start))
  {
    char buf[256];
    FormatMessageA (
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError (),
        0,
        buf,
        sizeof (buf),
        NULL
    );
    return error_causef (e, ERR_IO, "QueryPerformanceCounter: %s", buf);
  }

  return SUCCESS;
}

void
i_timer_free (i_timer *timer)
{
  ASSERT (timer);
  // No cleanup needed.
}

u64
i_timer_now_ns (i_timer *timer)
{
  ASSERT (timer);

  LARGE_INTEGER now;
  QueryPerformanceCounter (&now);

  LONGLONG elapsed = now.QuadPart - timer->start.QuadPart;

  // elapsed * 1e9 / frequency — multiply before divide to preserve precision.
  // Risk of overflow: elapsed * 1e9 overflows LONGLONG at ~9.2 seconds if
  // frequency is 1 GHz. Most systems are 10–100 MHz, giving ~92–920 seconds
  // before overflow. For long-running timers use the 128-bit path below.
  //
  // Safe path: scale to ns using (elapsed / freq) * 1e9 + remainder.
  LONGLONG freq = timer->frequency.QuadPart;
  LONGLONG sec  = elapsed / freq;
  LONGLONG rem  = elapsed % freq;
  return (u64)(sec * 1000000000LL + (rem * 1000000000LL) / freq);
}

u64
i_timer_now_us (i_timer *timer)
{
  return i_timer_now_ns (timer) / 1000ULL;
}

u64
i_timer_now_ms (i_timer *timer)
{
  return i_timer_now_ns (timer) / 1000000ULL;
}

f64
i_timer_now_s (i_timer *timer)
{
  ASSERT (timer);

  LARGE_INTEGER now;
  QueryPerformanceCounter (&now);

  LONGLONG elapsed = now.QuadPart - timer->start.QuadPart;
  return (f64)elapsed / (f64)timer->frequency.QuadPart;
}

void
i_sleep_us (u64 us)
{
  // Sleep() takes milliseconds; round up to avoid sleeping too short.
  DWORD ms = (DWORD)((us + 999ULL) / 1000ULL);
  Sleep (ms);
}

#else

typedef int make_compiler_happy;

#endif
