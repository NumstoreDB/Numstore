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
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
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
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
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
      FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE,
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

#endif // PLATFORM_WINDOWS
