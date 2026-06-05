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

#define WIN32_LEAN_AND_MEAN
#include <c_specx.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "windows.h"

////////////////////////////////////////////////////////////
// Helpers

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

#define WIN_ERR_BUF     256
#define WIN_ERRMSG(buf) win32_strerror (GetLastError (), buf, sizeof (buf))

static OVERLAPPED
make_overlapped (u64 offset)
{
  OVERLAPPED ov = {0};
  ov.Offset     = (DWORD)(offset & 0xFFFFFFFFULL);
  ov.OffsetHigh = (DWORD)(offset >> 32);
  return ov;
}

#ifndef NDEBUG
static bool
handle_is_open (HANDLE h)
{
  return h != NULL && h != INVALID_HANDLE_VALUE;
}
#endif

DEFINE_DBG_ASSERT (i_file, i_file, fp, {
  ASSERT (fp);
  ASSERT (handle_is_open (fp->handle));
})

////////////////////////////////////////////////////////////
// File vtable — Win32 implementations

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

static i64
win32_readv_some (
    const i_file       *fp,
    const struct bytes *iov,
    const int           iovcnt,
    error              *e
)
{
  DBG_ASSERT (i_file, fp);
  ASSERT (iov);
  ASSERT (iovcnt > 0 && iovcnt <= 2);

  i64 total = 0;
  for (int i = 0; i < iovcnt; i++)
  {
    DWORD nread = 0;
    if (unlikely (
            !ReadFile (fp->handle, iov[i].head, (DWORD)iov[i].len, &nread, NULL)
        ))
    {
      DWORD err = GetLastError ();
      if (likely (err == ERROR_HANDLE_EOF))
      {
        break;
      }
      char buf[WIN_ERR_BUF];
      win32_strerror (err, buf, sizeof (buf));
      return error_causef (e, ERR_IO, "readv: %s", buf);
    }
    total += nread;
    if ((u64)nread < iov[i].len)
    {
      break; // partial / EOF
    }
  }
  return total;
}

static i64
win32_readv_all (
    const i_file *fp,
    struct bytes *iov,
    const int     iovcnt,
    error        *e
)
{
  DBG_ASSERT (i_file, fp);
  ASSERT (iov);
  ASSERT (iovcnt > 0 && iovcnt <= 2);

  i64 total = 0;
  for (int i = 0; i < iovcnt; i++)
  {
    u8 *dst    = (u8 *)iov[i].head;
    u64 remain = iov[i].len;
    while (remain > 0)
    {
      DWORD want  = (DWORD)(remain > 0xFFFFFFFFULL ? 0xFFFFFFFFUL : remain);
      DWORD nread = 0;
      if (unlikely (!ReadFile (fp->handle, dst, want, &nread, NULL)))
      {
        DWORD err = GetLastError ();
        if (likely (err == ERROR_HANDLE_EOF))
        {
          goto done;
        }
        char buf[WIN_ERR_BUF];
        win32_strerror (err, buf, sizeof (buf));
        return error_causef (e, ERR_IO, "readv: %s", buf);
      }
      if (nread == 0)
      {
        goto done;
      }
      dst += nread;
      remain -= nread;
      total += nread;
    }
  }
done:
  return total;
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
      UNREACHABLE ();
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
    .i_readv_some  = win32_readv_some,
    .i_readv_all   = win32_readv_all,
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
