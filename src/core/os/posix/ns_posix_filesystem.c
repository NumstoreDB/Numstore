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
#include "core/os/ns_file.h"

#include <inttypes.h>
#include <stdbool.h>
#include <sys/types.h>

#if PLATFORM_POSIX

#  include "core/ns_error.h"
#  include "core/os/ns_filesystem.h"

#  include <dirent.h>
#  include <errno.h>
#  include <fcntl.h>
#  include <limits.h>
#  include <pthread.h>
#  include <stdio.h>
#  include <string.h>
#  include <sys/stat.h>
#  include <sys/uio.h>
#  include <time.h>
#  include <unistd.h>

/******************************************************************************
 * SECTION: File System
 ******************************************************************************/

static err_t
posix_open_rw (void *vfs, i_file *dest, const char *fname, error *e)
{
  (void)vfs;
  const int fd = open (fname, O_RDWR | O_CREAT, 0644);

  if (unlikely (fd == -1)) {
    error_causef (e, ERR_IO, "open_rw %s: %s", fname, strerror (errno));
    return error_trace (e);
  }

  *dest = create_default_file (fd);

  return SUCCESS;
}

static err_t
posix_open_r (void *vfs, i_file *dest, const char *fname, error *e)
{
  (void)vfs;
  const int fd = open (fname, O_RDONLY, 0644);

  if (unlikely (fd == -1)) {
    error_causef (e, ERR_IO, "open_r %s: %s", fname, strerror (errno));
    return error_trace (e);
  }

  *dest = create_default_file (fd);

  return SUCCESS;
}

static err_t
posix_open_w (void *vfs, i_file *dest, const char *fname, error *e)
{
  (void)vfs;
  const int fd = open (fname, O_WRONLY | O_CREAT, 0644);

  if (unlikely (fd == -1)) {
    error_causef (e, ERR_IO, "open_w %s: %s", fname, strerror (errno));
    return error_trace (e);
  }

  *dest = create_default_file (fd);

  return SUCCESS;
}

static err_t
posix_remove_quiet (void *vfs, const char *fname, error *e)
{
  (void)vfs;

  if (unlikely (remove (fname) && errno != ENOENT)) {
    error_causef (e, ERR_IO, "remove: %s", strerror (errno));
    return error_trace (e);
  }

  return SUCCESS;
}

static err_t
posix_unlink (void *vfs, const char *name, error *e)
{
  (void)vfs;

  if (unlikely (unlink (name))) {
    error_causef (e, ERR_IO, "unlink: %s", strerror (errno));
    return error_trace (e);
  }

  return SUCCESS;
}

static err_t
posix_file_exists (void *vfs, const char *fname, bool *dest, error *e)
{
  (void)vfs;

  struct stat st;

  if (stat (fname, &st) != 0) {
    if (likely (errno == ENOENT)) {
      *dest = false;
      return SUCCESS;
    }
    error_causef (e, ERR_IO, "stat %s: %s", fname, strerror (errno));
    return error_trace (e);
  }

  *dest = S_ISREG (st.st_mode);
  return SUCCESS;
}

////////////////////////////////////////////////////////////
// Default file system vtable

static const struct i_file_system_vtable default_fsvtable = {
    .open_rw      = posix_open_rw,
    .open_r       = posix_open_r,
    .open_w       = posix_open_w,
    .remove_quiet = posix_remove_quiet,
    .unlink       = posix_unlink,
    .file_exists  = posix_file_exists,
};

struct i_file_system
default_filesystem (void)
{
  return (struct i_file_system){
      .table = &default_fsvtable,
      .data  = NULL,
  };
}

#endif
