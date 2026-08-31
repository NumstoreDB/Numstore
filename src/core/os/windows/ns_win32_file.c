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
#  include "core/os/test/ns_dst.h"

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

err_t
impl_close (void *_fp, error *e)
{
  i_file *fp = _fp;
  I_FILE_FAULT (fp, file.close_fail_prob, e);
  DBG_ASSERT (i_file, fp);

  if (unlikely (!CloseHandle (fp->handle))) {
    char buf[WIN_ERR_BUF];
    return error_causef (e, ERR_IO, "close: %s", WIN_ERRMSG (buf));
  }

  fp->handle = INVALID_HANDLE_VALUE;
  return SUCCESS;
}

err_t
impl_fsync (void *_fp, error *e)
{
  i_file *fp = _fp;
  I_FILE_FAULT (fp, file.fsync_fail_prob, e);
  DBG_ASSERT (i_file, fp);

  if (unlikely (!FlushFileBuffers (fp->handle))) {
    char buf[WIN_ERR_BUF];
    return error_causef (e, ERR_IO, "fsync: %s", WIN_ERRMSG (buf));
  }

  return SUCCESS;
}

i64
impl_file_size (void *_fp, error *e)
{
  i_file *fp = _fp;
  I_FILE_FAULT (fp, file.file_size_fail_prob, e);
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

i64
impl_pread_all (void *_fp, void *dest, const u64 n, const u64 offset, error *e)
{
  i_file *fp = _fp;
  DBG_ASSERT (i_file, fp);
  ASSERT (dest);
  ASSERT (n > 0);

  u8 *_dest = (u8 *)dest;
  u64 nread = 0;

  while (nread < n) {
    u64 toread = n - nread;
    I_FILE_FAULT (fp, file.pread_fail_prob, e);
    I_FILE_CONDITION_AMOUNT (fp, file.pread_short_prob, toread);

    OVERLAPPED ov    = make_overlapped (offset + nread);
    DWORD      chunk = 0;
    DWORD      want  = (DWORD)(toread > 0xFFFFFFFFULL ? 0xFFFFFFFFUL : toread);

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

err_t
impl_pwrite_all (void *_fp, const void *src, const u64 n, const u64 offset, error *e)
{
  i_file *fp = _fp;
  DBG_ASSERT (i_file, fp);
  ASSERT (src);
  ASSERT (n > 0);

  const u8 *_src   = (const u8 *)src;
  u64       nwrite = 0;

  while (nwrite < n) {
    u64 towrite = n - nwrite;
    I_FILE_FAULT (fp, file.pwrite_fail_prob, e);
    I_FILE_CONDITION_AMOUNT (fp, file.pwrite_short_prob, towrite);

    OVERLAPPED ov    = make_overlapped (offset + nwrite);
    DWORD      chunk = 0;
    DWORD      want  = (DWORD)(towrite > 0xFFFFFFFFULL ? 0xFFFFFFFFUL : towrite);

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

err_t
impl_writev_all (void *_fp, struct bytes *iov, const int iovcnt, error *e)
{
  i_file *fp = _fp;
  I_FILE_FAULT (fp, file.writev_fail_prob, e);
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

i64
impl_read_all (void *_fp, void *dest, const u64 nbytes, error *e)
{
  i_file *fp = _fp;
  DBG_ASSERT (i_file, fp);
  ASSERT (dest);
  ASSERT (nbytes > 0);

  u8 *_dest = (u8 *)dest;
  u64 nread = 0;

  while (nread < nbytes) {
    u64 toread = nbytes - nread;
    I_FILE_FAULT (fp, file.read_fail_prob, e);
    I_FILE_CONDITION_AMOUNT (fp, file.read_short_prob, toread);

    DWORD chunk = 0;
    DWORD want  = (DWORD)(toread > 0xFFFFFFFFULL ? 0xFFFFFFFFUL : toread);

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

err_t
impl_write_all (void *_fp, const void *src, const u64 nbytes, error *e)
{
  i_file *fp = _fp;
  DBG_ASSERT (i_file, fp);
  ASSERT (src);
  ASSERT (nbytes > 0);

  const u8 *_src   = (const u8 *)src;
  u64       nwrite = 0;

  while (nwrite < nbytes) {
    u64 towrite = nbytes - nwrite;
    I_FILE_FAULT (fp, file.write_fail_prob, e);
    I_FILE_CONDITION_AMOUNT (fp, file.write_short_prob, towrite);

    DWORD chunk = 0;
    DWORD want  = (DWORD)(towrite > 0xFFFFFFFFULL ? 0xFFFFFFFFUL : towrite);

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

err_t
impl_truncate (void *_fp, const u64 bytes, error *e)
{
  i_file *fp = _fp;
  I_FILE_FAULT (fp, file.truncate_fail_prob, e);
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

err_t
impl_fallocate (void *_fp, const u64 bytes, error *e)
{
  i_file *fp = _fp;
  I_FILE_FAULT (fp, file.fallocate_fail_prob, e);
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

i64
impl_seek (void *_fp, const u64 offset, const seek_t whence, error *e)
{
  i_file *fp = _fp;
  I_FILE_FAULT (fp, file.seek_fail_prob, e);
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

i_file
create_default_file (HANDLE h)
{
  return (i_file){
      .handle = h,
      .table  = &default_os_vtable,
  };
}

#endif // PLATFORM_WINDOWS
