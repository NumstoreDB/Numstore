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

#include "os.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

#include "csx_assert.h"
#include "error.h" // error
#include "serial.h"

/******************************************************************************
 * SECTION: File System
 ******************************************************************************/

#ifndef NDEBUG
static bool
fd_is_open (const int fd)
{
  return fcntl (fd, F_GETFD) != -1 || errno != EBADF;
}
#endif

DEFINE_DBG_ASSERT (i_file, i_file, fp, {
  ASSERT (fp);
  ASSERT (fd_is_open (fp->fd));
})

/******************************************************************************
 * SECTION: File System
 ******************************************************************************/

static err_t
_posix_close (i_file *fp, error *e)
{
  DBG_ASSERT (i_file, fp);

  if (unlikely (close (fp->fd)))
  {
    return error_causef (e, ERR_IO, "close: %s", strerror (errno));
  }

  return SUCCESS;
}

static err_t
posix_fsync (const i_file *fp, error *e)
{
  DBG_ASSERT (i_file, fp);

  if (unlikely (fsync (fp->fd)))
  {
    return error_causef (e, ERR_IO, "fsync: %s", strerror (errno));
  }

  return SUCCESS;
}

static i64
posix_file_size (const i_file *fp, error *e)
{
  DBG_ASSERT (i_file, fp);

  struct stat st;

  if (unlikely (fstat (fp->fd, &st) == -1))
  {
    error_causef (e, ERR_IO, "fstat: %s", strerror (errno));
    return error_trace (e);
  }

  return (i64)st.st_size;
}

static i64
posix_pread_some (
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

  const ssize_t ret = pread (fp->fd, dest, n, (off_t)offset);

  if (unlikely (ret < 0 && errno != EINTR))
  {
    return error_causef (e, ERR_IO, "pread: %s", strerror (errno));
  }

  return (i64)ret;
}

static i64
posix_pread_all (
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
    ASSERT (n > nread);
    const ssize_t _nread =
        pread (fp->fd, _dest + nread, n - nread, (off_t)(offset + nread));
    if (_nread == 0)
    {
      return (i64)nread;
    }
    if (unlikely (_nread < 0 && errno != EINTR))
    {
      return error_causef (e, ERR_IO, "pread: %s", strerror (errno));
    }
    nread += (u64)_nread;
  }

  ASSERT (nread == n);

  return (i64)nread;
}

static i64
posix_pwrite_some (
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

  const ssize_t ret = pwrite (fp->fd, src, n, (off_t)offset);

  if (unlikely (ret < 0 && errno != EINTR))
  {
    return error_causef (e, ERR_IO, "pwrite: %s", strerror (errno));
  }

  return (i64)ret;
}

static err_t
posix_pwrite_all (
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

  const u8 *_src     = (const u8 *)src;
  u64       nwritten = 0;

  while (nwritten < n)
  {
    ASSERT (n > nwritten);

    const ssize_t _nw = pwrite (
        fp->fd,
        _src + nwritten,
        n - nwritten,
        (off_t)(offset + nwritten)
    );

    if (unlikely (_nw < 0 && errno != EINTR))
    {
      return error_causef (e, ERR_IO, "pwrite: %s", strerror (errno));
    }
    nwritten += (u64)_nw;
  }

  ASSERT (nwritten == n);
  return SUCCESS;
}

////////////////////////////////////////////////////////////
// IO Vec

static i64
posix_writev_some (
    const i_file       *fp,
    const struct bytes *src,
    const int           iovcnt,
    error              *e
)
{
  DBG_ASSERT (i_file, fp);
  ASSERT (src);
  ASSERT (iovcnt > 0 && iovcnt <= 2);

  struct iovec sys_iov[2];
  for (int i = 0; i < iovcnt; i++)
  {
    sys_iov[i].iov_base = src[i].head;
    sys_iov[i].iov_len  = src[i].len;
  }

  const ssize_t ret = writev (fp->fd, sys_iov, iovcnt);
  if (unlikely (ret < 0 && errno != EINTR))
  {
    return error_causef (e, ERR_IO, "writev: %s", strerror (errno));
  }
  return (i64)ret;
}

static err_t
posix_writev_all (
    const i_file *fp,
    struct bytes *iov,
    const int     iovcnt,
    error        *e
)
{
  DBG_ASSERT (i_file, fp);
  ASSERT (iov);
  ASSERT (iovcnt > 0 && iovcnt <= 2);

  u64 total = 0;
  for (int i = 0; i < iovcnt; i++)
  {
    total += iov[i].len;
  }

  ASSERT (total > 0);

  u64           nwritten  = 0;
  struct bytes *cur       = iov;
  int           remaining = iovcnt;

  while (nwritten < total)
  {
    struct iovec sys_iov[2];
    for (int i = 0; i < remaining; i++)
    {
      sys_iov[i].iov_base = cur[i].head;
      sys_iov[i].iov_len  = cur[i].len;
    }

    const ssize_t ret = writev (fp->fd, sys_iov, remaining);

    if (unlikely (ret < 0 && errno != EINTR))
    {
      return error_causef (e, ERR_IO, "writev: %s", strerror (errno));
    }

    if (ret <= 0)
    {
      continue;
    }

    nwritten += (u64)ret;
    u64 skip = (u64)ret;
    while (skip > 0 && remaining > 0)
    {
      if (skip >= cur->len)
      {
        skip -= cur->len;
        cur++;
        remaining--;
      }
      else
      {
        cur->head = (u8 *)cur->head + skip;
        cur->len -= skip;
        skip = 0;
      }
    }
  }

  ASSERT (nwritten == total);
  return SUCCESS;
}

static i64
posix_readv_some (
    const i_file       *fp,
    const struct bytes *iov,
    const int           iovcnt,
    error              *e
)
{
  DBG_ASSERT (i_file, fp);
  ASSERT (iov);
  ASSERT (iovcnt > 0 && iovcnt <= 2);

  struct iovec sys_iov[2];
  for (int i = 0; i < iovcnt; i++)
  {
    sys_iov[i].iov_base = iov[i].head;
    sys_iov[i].iov_len  = iov[i].len;
  }

  const ssize_t ret = readv (fp->fd, sys_iov, iovcnt);

  if (unlikely (ret < 0 && errno != EINTR))
  {
    return error_causef (e, ERR_IO, "readv: %s", strerror (errno));
  }

  return (i64)ret;
}

static i64
posix_readv_all (
    const i_file *fp,
    struct bytes *iov,
    const int     iovcnt,
    error        *e
)
{
  DBG_ASSERT (i_file, fp);
  ASSERT (iov);
  ASSERT (iovcnt > 0 && iovcnt <= 2);

  u64 total = 0;
  for (int i = 0; i < iovcnt; i++)
  {
    total += iov[i].len;
  }
  ASSERT (total > 0);

  u64           nread     = 0;
  struct bytes *cur       = iov;
  int           remaining = iovcnt;

  while (nread < total)
  {
    struct iovec sys_iov[2];
    for (int i = 0; i < remaining; i++)
    {
      sys_iov[i].iov_base = cur[i].head;
      sys_iov[i].iov_len  = cur[i].len;
    }

    const ssize_t ret = readv (fp->fd, sys_iov, remaining);
    if (unlikely (ret < 0 && errno != EINTR))
    {
      return error_causef (e, ERR_IO, "readv: %s", strerror (errno));
    }
    if (ret == 0)
    {
      break;
    }
    if (ret < 0)
    {
      continue; // EINTR, already handled above — defensive
    }

    nread += (u64)ret;
    u64 skip = (u64)ret;
    while (skip > 0 && remaining > 0)
    {
      if (skip >= cur->len)
      {
        skip -= cur->len;
        cur++;
        remaining--;
      }
      else
      {
        cur->head = (u8 *)cur->head + skip;
        cur->len -= skip;
        skip = 0;
      }
    }
  }

  return (i64)nread;
}

////////////////////////////////////////////////////////////
// Stream Read / Write

static i64
posix_read_some (const i_file *fp, void *dest, const u64 nbytes, error *e)
{
  DBG_ASSERT (i_file, fp);
  ASSERT (dest);
  ASSERT (nbytes > 0);

  i_log_trace ("read %llu bytes\n", nbytes);
  const ssize_t ret = read (fp->fd, dest, nbytes);
  i_log_trace ("read returned %ld\n", ret);

  if (unlikely (ret < 0))
  {
    if (likely (errno == EINTR || errno == EWOULDBLOCK))
    {
      return 0;
    }
    return error_causef (e, ERR_IO, "read: %s", strerror (errno));
  }
  return (i64)ret;
}

static i64
posix_read_all (const i_file *fp, void *dest, const u64 nbytes, error *e)
{
  DBG_ASSERT (i_file, fp);
  ASSERT (dest);
  ASSERT (nbytes > 0);

  u8 *_dest = (u8 *)dest;
  u64 nread = 0;

  while (nread < nbytes)
  {
    ASSERT (nbytes > nread);
    const ssize_t _nread = read (fp->fd, _dest + nread, nbytes - nread);

    if (_nread == 0)
    {
      return (i64)nread;
    }

    if (unlikely (_nread < 0))
    {
      if (likely (errno == EINTR || errno == EWOULDBLOCK))
      {
        return 0;
      }
      return error_causef (e, ERR_IO, "read: %s", strerror (errno));
    }

    nread += (u64)_nread;
  }

  ASSERT (nread == nbytes);
  return (i64)nread;
}

static i64
posix_write_some (const i_file *fp, const void *src, const u64 nbytes, error *e)
{
  DBG_ASSERT (i_file, fp);
  ASSERT (src);
  ASSERT (nbytes > 0);

  i_log_trace ("write %llu bytes\n", nbytes);
  const ssize_t ret = write (fp->fd, src, nbytes);
  i_log_trace ("write returned %ld\n", ret);

  if (unlikely (ret < 0 && errno != EINTR))
  {
    return error_causef (e, ERR_IO, "write: %s", strerror (errno));
  }

  return (i64)ret;
}

static err_t
posix_write_all (const i_file *fp, const void *src, const u64 nbytes, error *e)
{
  DBG_ASSERT (i_file, fp);
  ASSERT (src);
  ASSERT (nbytes > 0);

  const u8 *_src     = (const u8 *)src;
  u64       nwritten = 0;

  while (nwritten < nbytes)
  {
    ASSERT (nbytes > nwritten);

    const ssize_t _nw = write (fp->fd, _src + nwritten, nbytes - nwritten);
    if (unlikely (_nw < 0 && errno != EINTR))
    {
      return error_causef (e, ERR_IO, "write: %s", strerror (errno));
    }

    nwritten += (u64)_nw;
  }

  ASSERT (nwritten == nbytes);
  return SUCCESS;
}

////////////////////////////////////////////////////////////
// Other file ops

static err_t
posix_truncate (const i_file *fp, const u64 bytes, error *e)
{
  DBG_ASSERT (i_file, fp);

  if (unlikely (ftruncate (fp->fd, (off_t)bytes) == -1))
  {
    return error_causef (e, ERR_IO, "ftruncate: %s", strerror (errno));
  }

  return SUCCESS;
}

static err_t
_posix_fallocate (i_file *fp, const u64 bytes, error *e)
{
  DBG_ASSERT (i_file, fp);

#if defined(__APPLE__)
  fstore_t store = {
      .fst_flags   = F_ALLOCATECONTIG,
      .fst_posmode = F_PEOFPOSMODE,
      .fst_offset  = 0,
      .fst_length  = (off_t)bytes,
  };
  if (unlikely (fcntl (fp->fd, F_PREALLOCATE, &store) == -1))
  {
    store.fst_flags = F_ALLOCATEALL;
    if (unlikely (fcntl (fp->fd, F_PREALLOCATE, &store) == -1))
    {
      return error_causef (e, ERR_IO, "F_PREALLOCATE: %s", strerror (errno));
    }
  }
  if (unlikely (ftruncate (fp->fd, (off_t)bytes) == -1))
  {
    return error_causef (e, ERR_IO, "ftruncate: %s", strerror (errno));
  }
#else
  const int ret = posix_fallocate (fp->fd, 0, (off_t)bytes);

  if (unlikely (ret != 0))
  {
    return error_causef (e, ERR_IO, "posix_fallocate: %s", strerror (ret));
  }
#endif

  return SUCCESS;
}

static i64
posix_seek (const i_file *fp, const u64 offset, const seek_t whence, error *e)
{
  DBG_ASSERT (i_file, fp);

  int w;
  switch (whence)
  {
    case I_SEEK_SET:
    {
      w = SEEK_SET;
      break;
    }
    case I_SEEK_CUR:
    {
      w = SEEK_CUR;
      break;
    }
    case I_SEEK_END:
    {
      w = SEEK_END;
      break;
    }
    default:
    {
      UNREACHABLE ();
    }
  }

  errno           = 0;
  const off_t ret = lseek (fp->fd, (off_t)offset, w);

  if (unlikely (ret == (off_t)-1))
  {
    error_causef (e, ERR_IO, "lseek: %s", strerror (errno));
    return error_trace (e);
  }

  return (i64)ret;
}

static err_t
posix_open_rw (
    i_file_system_vtable *vfs,
    i_file               *dest,
    const char           *fname,
    error                *e
)
{
  (void)vfs;
  const int fd = open (fname, O_RDWR | O_CREAT, 0644);

  if (unlikely (fd == -1))
  {
    error_causef (e, ERR_IO, "open_rw %s: %s", fname, strerror (errno));
    return error_trace (e);
  }

  *dest = (i_file){
      .fd       = fd,
      .fvtable  = &default_fvtable,
      .fsvtable = &default_fsvtable,
  };

  DBG_ASSERT (i_file, dest);

  return SUCCESS;
}

static err_t
posix_open_r (
    i_file_system_vtable *vfs,
    i_file               *dest,
    const char           *fname,
    error                *e
)
{
  (void)vfs;
  const int fd = open (fname, O_RDONLY, 0644);

  if (unlikely (fd == -1))
  {
    error_causef (e, ERR_IO, "open_r %s: %s", fname, strerror (errno));
    return error_trace (e);
  }

  *dest = (i_file){
      .fd       = fd,
      .fvtable  = &default_fvtable,
      .fsvtable = &default_fsvtable,
  };

  DBG_ASSERT (i_file, dest);

  return SUCCESS;
}

static err_t
posix_open_w (
    i_file_system_vtable *vfs,
    i_file               *dest,
    const char           *fname,
    error                *e
)
{
  (void)vfs;
  const int fd = open (fname, O_WRONLY | O_CREAT, 0644);

  if (unlikely (fd == -1))
  {
    error_causef (e, ERR_IO, "open_w %s: %s", fname, strerror (errno));
    return error_trace (e);
  }

  *dest = (i_file){
      .fd       = fd,
      .fvtable  = &default_fvtable,
      .fsvtable = &default_fsvtable,
  };

  DBG_ASSERT (i_file, dest);

  return SUCCESS;
}

static err_t
posix_remove_quiet (i_file_system_vtable *vfs, const char *fname, error *e)
{
  (void)vfs;

  if (unlikely (remove (fname) && errno != ENOENT))
  {
    error_causef (e, ERR_IO, "remove: %s", strerror (errno));
    return error_trace (e);
  }

  return SUCCESS;
}

static err_t
posix_unlink (i_file_system_vtable *vfs, const char *name, error *e)
{
  (void)vfs;

  if (unlikely (unlink (name)))
  {
    error_causef (e, ERR_IO, "unlink: %s", strerror (errno));
    return error_trace (e);
  }

  return SUCCESS;
}

static err_t
posix_mkdir (i_file_system_vtable *vfs, const char *name, error *e)
{
  (void)vfs;

  if (unlikely (mkdir (name, S_IRWXU | S_IRWXG | S_IRWXO)))
  {
    error_causef (e, ERR_IO, "mkdir: %s", strerror (errno));
    return error_trace (e);
  }

  return SUCCESS;
}

static err_t
posix_mkdir_quiet (i_file_system_vtable *vfs, const char *name, error *e)
{
  (void)vfs;

  if (unlikely (mkdir (name, S_IRWXU | S_IRWXG | S_IRWXO) == 0))
  {
    return SUCCESS;
  }

  if (unlikely (errno != EEXIST))
  {
    error_causef (e, ERR_IO, "mkdir: %s", strerror (errno));
    return error_trace (e);
  }

  struct stat st;

  if (unlikely (stat (name, &st) != 0))
  {
    error_causef (e, ERR_IO, "stat %s: %s", name, strerror (errno));
    return error_trace (e);
  }

  if (unlikely (!S_ISDIR (st.st_mode)))
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
posix_rm_rf (i_file_system_vtable *vfs, const char *path, error *e)
{
  (void)vfs;

  DIR *dir = opendir (path);

  if (unlikely (dir == NULL))
  {
    if (errno == ENOENT)
    {
      return SUCCESS;
    }
    error_causef (e, ERR_IO, "opendir %s: %s", path, strerror (errno));
    return error_trace (e);
  }

  struct dirent *entry;
  char           child[PATH_MAX];

  while ((entry = readdir (dir)) != NULL)
  {
    if (strcmp (entry->d_name, ".") == 0 || strcmp (entry->d_name, "..") == 0)
    {
      continue;
    }

    snprintf (child, sizeof child, "%s/%s", path, entry->d_name);

    if (unlikely (unlink (child) && errno != ENOENT))
    {
      closedir (dir);
      error_causef (e, ERR_IO, "unlink %s: %s", child, strerror (errno));
      return error_trace (e);
    }
  }

  closedir (dir);

  if (unlikely (rmdir (path) && errno != ENOENT))
  {
    error_causef (e, ERR_IO, "rmdir %s: %s", path, strerror (errno));
    return error_trace (e);
  }

  return SUCCESS;
}

static err_t
posix_access_rw (i_file_system_vtable *vfs, const char *fname, error *e)
{
  (void)vfs;

  if (unlikely (access (fname, F_OK | W_OK | R_OK)))
  {
    error_causef (e, ERR_IO, "access: %s", strerror (errno));
    return error_trace (e);
  }

  return SUCCESS;
}

static bool
posix_exists_rw (i_file_system_vtable *vfs, const char *fname)
{
  (void)vfs;
  return access (fname, F_OK | W_OK | R_OK) == 0;
}

static err_t
posix_touch (i_file_system_vtable *vfs, const char *fname, error *e)
{
  ASSERT (fname);

  i_file fd = {0};
  WRAP (vfs->i_open_rw (vfs, &fd, fname, e));
  WRAP (fd.fvtable->i_close (&fd, e));

  return SUCCESS;
}

static err_t
posix_dir_exists (
    i_file_system_vtable *vfs,
    const char           *fname,
    bool                 *dest,
    error                *e
)
{
  (void)vfs;

  struct stat st;

  if (unlikely (stat (fname, &st) != 0))
  {
    if (likely (errno == ENOENT))
    {
      *dest = false;
      return SUCCESS;
    }

    error_causef (e, ERR_IO, "stat %s: %s", fname, strerror (errno));
    return error_trace (e);
  }

  *dest = S_ISDIR (st.st_mode);
  return SUCCESS;
}

static err_t
posix_file_exists (
    i_file_system_vtable *vfs,
    const char           *fname,
    bool                 *dest,
    error                *e
)
{
  (void)vfs;

  struct stat st;

  if (stat (fname, &st) != 0)
  {
    if (likely (errno == ENOENT))
    {
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

struct i_file_vtable default_fvtable = {
    .i_close       = _posix_close,
    .i_fsync       = posix_fsync,
    .i_file_size   = posix_file_size,
    .i_read_some   = posix_read_some,
    .i_read_all    = posix_read_all,
    .i_pread_some  = posix_pread_some,
    .i_pread_all   = posix_pread_all,
    .i_readv_some  = posix_readv_some,
    .i_readv_all   = posix_readv_all,
    .i_write_some  = posix_write_some,
    .i_write_all   = posix_write_all,
    .i_pwrite_some = posix_pwrite_some,
    .i_pwrite_all  = posix_pwrite_all,
    .i_writev_some = posix_writev_some,
    .i_writev_all  = posix_writev_all,
    .i_truncate    = posix_truncate,
    .i_fallocate   = _posix_fallocate,
    .i_seek        = posix_seek,
};

struct i_file_system_vtable default_fsvtable = {
    .i_open_rw      = posix_open_rw,
    .i_open_r       = posix_open_r,
    .i_open_w       = posix_open_w,
    .i_remove_quiet = posix_remove_quiet,
    .i_unlink       = posix_unlink,
    .i_mkdir        = posix_mkdir,
    .i_mkdir_quiet  = posix_mkdir_quiet,
    .i_rm_rf        = posix_rm_rf,
    .i_access_rw    = posix_access_rw,
    .i_exists_rw    = posix_exists_rw,
    .i_touch        = posix_touch,
    .i_dir_exists   = posix_dir_exists,
    .i_file_exists  = posix_file_exists,
};

/******************************************************************************
 * SECTION: Threading
 ******************************************************************************/

/*-----------------------------------------------------------------------------
 * SUBSECTION: Condition Variables
 *----------------------------------------------------------------------------*/

static err_t
posix_cond_create (i_threading *t, i_cond *c, error *e)
{
  (void)t;
  ASSERT (c);

#ifndef NDEBUG
  pthread_condattr_t attr;

  // I just don't want to handle errors for debug code
  int r1 = pthread_condattr_init (&attr);
  ASSERT (r1 == 0);

  const int r2 = pthread_cond_init (&c->cond, &attr);

  r1 = pthread_condattr_destroy (&attr);
  ASSERT (r1 == 0);

  if (r2)
#else
  if (pthread_cond_init (&c->cond, NULL))
#endif
  {
    switch (errno)
    {
      case EAGAIN:
      {
        return error_causef (
            e,
            ERR_IO,
            "pthread_cond_init: %s",
            strerror (errno)
        );
      }

      case ENOMEM:
      {
        return error_causef (
            e,
            ERR_NOMEM,
            "pthread_cond_init: %s",
            strerror (errno)
        );
      }

      case EBUSY:
      {
        i_log_error (
            "cond_create: cond "
            "already initialized: "
            "%s\n",
            strerror (errno)
        );
        UNREACHABLE ();
      }

      case EINVAL:
      {
        i_log_error (
            "cond_create: invalid "
            "attributes or cond: %s\n",
            strerror (errno)
        );
        UNREACHABLE ();
      }

      default:
      {
        i_log_error (
            "cond_create: unknown "
            "error: %s\n",
            strerror (errno)
        );
        UNREACHABLE ();
      }
    }
  }

  return SUCCESS;
}

static void
posix_cond_free (i_threading *t, i_cond *c)
{
  (void)t;
  ASSERT (c);

  errno = 0;
  if (pthread_cond_destroy (&c->cond))
  {
    switch (errno)
    {
      case EBUSY:
      {
        i_log_error (
            "cond_free: cond has "
            "active waiters: %s\n",
            strerror (errno)
        );
        UNREACHABLE ();
      }

      case EINVAL:
      {
        i_log_error (
            "cond_free: invalid or "
            "uninitialized cond: %s\n",
            strerror (errno)
        );
        UNREACHABLE ();
      }

      default:
      {
        i_log_error (
            "cond_free: unknown "
            "error: %s\n",
            strerror (errno)
        );
        UNREACHABLE ();
      }
    }
  }
}

static void
posix_cond_wait (i_threading *t, i_cond *c, i_mutex *m)
{
  (void)t;
  ASSERT (c);
  ASSERT (m);

  errno = 0;
  if (pthread_cond_wait (&c->cond, &m->m))
  {
    switch (errno)
    {
      case EINVAL:
      {
        i_log_error (
            "cond_wait: invalid cond "
            "or mutex: %s\n",
            strerror (errno)
        );
        UNREACHABLE ();
      }

      case EPERM:
      {
        i_log_error (
            "cond_wait: mutex not "
            "owned by thread: %s\n",
            strerror (errno)
        );
        UNREACHABLE ();
      }

      default:
      {
        i_log_error (
            "cond_wait: unknown "
            "error: %s\n",
            strerror (errno)
        );
        UNREACHABLE ();
      }
    }
  }
}

static void
posix_cond_timed_wait (i_threading *t, i_cond *c, i_mutex *m, u64 msec)
{
  (void)t;
  ASSERT (c);
  ASSERT (m);

  struct timespec ts;
  clock_gettime (CLOCK_REALTIME, &ts);
  ts.tv_sec += msec / 1000;
  ts.tv_nsec += (msec % 1000) * 1000000LL;
  if (ts.tv_nsec >= 1000000000LL)
  {
    ts.tv_sec += 1;
    ts.tv_nsec -= 1000000000LL;
  }

  errno   = 0;
  int ret = pthread_cond_timedwait (&c->cond, &m->m, &ts);
  if (ret && ret != ETIMEDOUT)
  {
    switch (ret)
    {
      case EINVAL:
      {
        i_log_error (
            "cond_timed_wait: invalid cond, "
            "mutex, or abstime: %s\n",
            strerror (ret)
        );
        UNREACHABLE ();
      }
      case EPERM:
      {
        i_log_error (
            "cond_timed_wait: mutex not "
            "owned by thread: %s\n",
            strerror (ret)
        );
        UNREACHABLE ();
      }
      default:
      {
        i_log_error (
            "cond_timed_wait: unknown "
            "error: %s\n",
            strerror (ret)
        );
        UNREACHABLE ();
      }
    }
  }
}

static void
posix_cond_signal (i_threading *t, i_cond *c)
{
  (void)t;
  ASSERT (c);

  errno = 0;
  if (pthread_cond_signal (&c->cond))
  {
    switch (errno)
    {
      case EINVAL:
      {
        i_log_error (
            "cond_signal: invalid or "
            "uninitialized cond: %s\n",
            strerror (errno)
        );
        UNREACHABLE ();
      }

      default:
      {
        i_log_error (
            "cond_signal: unknown "
            "error: %s\n",
            strerror (errno)
        );
        UNREACHABLE ();
      }
    }
  }
}

static void
posix_cond_broadcast (i_threading *t, i_cond *c)
{
  (void)t;
  ASSERT (c);

  errno = 0;
  if (pthread_cond_broadcast (&c->cond))
  {
    switch (errno)
    {
      case EINVAL:
      {
        i_log_error (
            "cond_broadcast: invalid "
            "or uninitialized cond: "
            "%s\n",
            strerror (errno)
        );
        UNREACHABLE ();
      }

      default:
      {
        i_log_error (
            "cond_broadcast: unknown "
            "error: %s\n",
            strerror (errno)
        );
        UNREACHABLE ();
      }
    }
  }
}

/*-----------------------------------------------------------------------------
 * SUBSECTION: Mutex
 *----------------------------------------------------------------------------*/

struct i_mutex_s
{
  pthread_mutex_t mutex;
};

static err_t
posix_mutex_create (i_threading *t, i_mutex *dest, error *e)
{
  (void)t;
  errno = 0;
#ifndef NDEBUG
  pthread_mutexattr_t attr;

  // I just don't want to handle errors for debug code
  int r1 = pthread_mutexattr_init (&attr);
  ASSERT (!r1);

  r1 = pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_ERRORCHECK);
  ASSERT (!r1);

  const int r2 = pthread_mutex_init (&dest->m, NULL);

  r1 = pthread_mutexattr_destroy (&attr);
  ASSERT (!r1);
  if (r2)
#else
  if (pthread_mutex_init (&dest->m, NULL))
#endif
  {
    switch (errno)
    {
      case EAGAIN:
      {
        return error_causef (e, ERR_IO, "mutex_init: %s", strerror (errno));
      }
      case ENOMEM:
      {
        return error_causef (e, ERR_NOMEM, "mutex_init: %s", strerror (errno));
      }
      case EPERM:
      {
        i_log_error (
            "mutex_init: insufficient "
            "permissions: %s\n",
            strerror (errno)
        );
        UNREACHABLE ();
      }
      default:
      {
        UNREACHABLE ();
      }
    }
  }

  return SUCCESS;
}

static void
posix_mutex_free (i_threading *t, i_mutex *m)
{
  (void)t;
  ASSERT (m);

  errno = 0;

  if (pthread_mutex_destroy (&m->m))
  {
    switch (errno)
    {
      case EBUSY:
      {
        i_log_error (
            "mutex_destroy: still "
            "locked: %s\n",
            strerror (errno)
        );
        UNREACHABLE ();
      }
      case EINVAL:
      {
        i_log_error (
            "mutex_destroy: "
            "invalid: %s\n",
            strerror (errno)
        );
        UNREACHABLE ();
      }
      default:
      {
        UNREACHABLE ();
      }
    }
  }
}

static void
posix_mutex_lock (i_threading *t, i_mutex *m)
{
  (void)t;
  ASSERT (m);

  int ret = pthread_mutex_lock (&m->m);
  if (ret)
  {
    switch (ret)
    {
      case EINVAL:
      {
        i_log_error (
            "mutex_lock: "
            "invalid: %s\n",
            strerror (ret)
        );
        UNREACHABLE ();
      }
      case EAGAIN:
      {
        i_log_error (
            "mutex_lock: recursive "
            "lock: %s\n",
            strerror (ret)
        );
        UNREACHABLE ();
      }
      case EDEADLK:
      {
        i_log_error (
            "mutex_lock: "
            "deadlock: %s\n",
            strerror (ret)
        );
        UNREACHABLE ();
      }
      default:
      {
        i_log_error ("mutex_lock: %s\n", strerror (ret));
        UNREACHABLE ();
      }
    }
  }
}

static bool
posix_mutex_try_lock (i_threading *t, i_mutex *m)
{
  (void)t;
  ASSERT (m);

  errno = 0;
  if (pthread_mutex_trylock (&m->m))
  {
    switch (errno)
    {
      case EINVAL:
      {
        i_log_error (
            "mutex_lock: "
            "invalid: %s\n",
            strerror (errno)
        );
        UNREACHABLE ();
      }
      case EAGAIN:
      {
        i_log_error (
            "mutex_lock: recursive "
            "lock: %s\n",
            strerror (errno)
        );
        UNREACHABLE ();
      }
      case EDEADLK:
      {
        i_log_error (
            "mutex_lock: "
            "deadlock: %s\n",
            strerror (errno)
        );
        UNREACHABLE ();
      }
      case EBUSY:
      {
        return false;
      }
      default:
      {
        i_log_error ("mutex_lock: %s\n", strerror (errno));
        UNREACHABLE ();
      }
    }
  }
  return true;
}

static void
posix_mutex_unlock (i_threading *t, i_mutex *m)
{
  (void)t;
  ASSERT (m);

  errno = 0;
  if (pthread_mutex_unlock (&m->m))
  {
    switch (errno)
    {
      case EINVAL:
      {
        i_log_error (
            "mutex_unlock: "
            "invalid: %s\n",
            strerror (errno)
        );
        UNREACHABLE ();
      }
      case EAGAIN:
      {
        i_log_error (
            "mutex_unlock: recursive "
            "lock: %s\n",
            strerror (errno)
        );
        UNREACHABLE ();
      }
      case EPERM:
      {
        i_log_error (
            "mutex_unlock: "
            "not owner: %s\n",
            strerror (errno)
        );
        UNREACHABLE ();
      }
      default:
      {
        i_log_error ("mutex_unlock: %s\n", strerror (errno));
        UNREACHABLE ();
      }
    }
  }
}

/*-----------------------------------------------------------------------------
 * SUBSECTION: Thread
 *----------------------------------------------------------------------------*/

static err_t
posix_thread_create (
    i_threading *t,
    i_thread    *dest,
    void *(*func) (void *),
    void  *context,
    error *e
)
{
  (void)t;
  ASSERT (dest);

#ifndef NDEBUG
  pthread_attr_t attr;
  int            r1 = pthread_attr_init (&attr);
  ASSERT (!r1);

  // Examples:
  // pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  // pthread_attr_setstacksize(&attr, 1 << 20);
  // pthread_attr_setguardsize(&attr, 4096);
  // pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
  // pthread_attr_setschedpolicy(&attr, SCHED_OTHER);
  // pthread_attr_setschedparam(&attr, &sched_param);

  const int r2 = pthread_create (&dest->thread, &attr, func, context);

  r1 = pthread_attr_destroy (&attr);
  ASSERT (!r1);
  if (r2)
#else
  if (pthread_create (&dest->thread, NULL, func, context))
#endif
  {
    switch (errno)
    {
      case EAGAIN:
      {
        return error_causef (e, ERR_IO, "pthread_create: %s", strerror (errno));
      }
      case EINVAL:
      {
        i_log_error (
            "pthread_create: invalid "
            "attributes: %s\n",
            strerror (errno)
        );
        UNREACHABLE ();
      }
      case EPERM:
      {
        i_log_error (
            "pthread_create: "
            "insufficient "
            "permissions: %s\n",
            strerror (errno)
        );
        UNREACHABLE ();
      }
      default:
      {
        UNREACHABLE ();
      }
    }
  }

  return SUCCESS;
}

static err_t
posix_thread_join (i_threading *t, i_thread *th, error *e)
{
  (void)t;
  ASSERT (th);

  const int r = pthread_join (th->thread, NULL);

  if (r != 0)
  {
    switch (r)
    {
      case EDEADLK:
      {
        i_log_error (
            "pthread_join: "
            "deadlock: %s\n",
            strerror (r)
        );
        UNREACHABLE ();
      }
      case EINVAL:
      {
        i_log_error (
            "pthread_join: not "
            "joinable: %s\n",
            strerror (r)
        );
        UNREACHABLE ();
      }
      case ESRCH:
      {
        i_log_error (
            "pthread_join: no such "
            "thread: %s\n",
            strerror (r)
        );
        UNREACHABLE ();
      }
      default:
      {
        UNREACHABLE ();
      }
    }
  }

  return SUCCESS;
}

/*-----------------------------------------------------------------------------
 * SUBSECTION: Abstraction
 *----------------------------------------------------------------------------*/

struct i_threading default_threading = {
    .i_thread_create = posix_thread_create,
    .i_thread_join   = posix_thread_join,

    .i_mutex_create   = posix_mutex_create,
    .i_mutex_free     = posix_mutex_free,
    .i_mutex_lock     = posix_mutex_lock,
    .i_mutex_try_lock = posix_mutex_try_lock,
    .i_mutex_unlock   = posix_mutex_unlock,

    .i_cond_create     = posix_cond_create,
    .i_cond_free       = posix_cond_free,
    .i_cond_wait       = posix_cond_wait,
    .i_cond_timed_wait = posix_cond_timed_wait,
    .i_cond_signal     = posix_cond_signal,
    .i_cond_broadcast  = posix_cond_broadcast,
};

/******************************************************************************
 * SECTION: Timing
 ******************************************************************************/

err_t
i_timer_create (i_timer *timer, error *e)
{
  ASSERT (timer);

  if (clock_gettime (CLOCK_MONOTONIC, &timer->start) != 0)
  {
    return error_causef (e, ERR_IO, "clock_gettime: %s", strerror (errno));
  }

  return SUCCESS;
}

void
i_timer_free (i_timer *timer)
{
  ASSERT (timer);
}

u64
i_timer_now_ns (i_timer *timer)
{
  ASSERT (timer);

  struct timespec now;
  clock_gettime (CLOCK_MONOTONIC, &now);

  // Calculate elapsed time in nanoseconds
  const i64 sec_diff  = (i64)now.tv_sec - (i64)timer->start.tv_sec;
  const i64 nsec_diff = (i64)now.tv_nsec - (i64)timer->start.tv_nsec;

  return (u64)(sec_diff * 1000000000LL + nsec_diff);
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
  return (f64)i_timer_now_ns (timer) / 1000000000.0;
}

void
i_sleep_us (const u64 us)
{
  const struct timespec ts = {
      .tv_sec  = (time_t)(us / 1000000ULL),
      .tv_nsec = (long)((us % 1000000ULL) * 1000ULL),
  };
  nanosleep (&ts, NULL);
}
