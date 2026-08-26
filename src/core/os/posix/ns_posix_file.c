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

#include "core/ns_bytes.h"
#include "core/ns_platform.h"
#include "core/ns_stdtypes.h"
#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/os/ns_file.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

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
_posix_close (void *_fp, error *e)
{
  i_file *fp = _fp;

  // I_FILE_FAULT (fp, close_fail_prob, e);

  DBG_ASSERT (i_file, fp);

  if (unlikely (close (fp->fd))) {
    return error_causef (e, ERR_IO, "close: %s", strerror (errno));
  }

  return SUCCESS;
}

static err_t
posix_fsync (void *_fp, error *e)
{
  i_file *fp = _fp;

  // I_FILE_FAULT (fp, fsync_fail_prob, e);

  DBG_ASSERT (i_file, fp);

  if (unlikely (fsync (fp->fd))) {
    return error_causef (e, ERR_IO, "fsync: %s", strerror (errno));
  }

  return SUCCESS;
}

static i64
posix_file_size (void *_fp, error *e)
{
  i_file *fp = _fp;

  // I_FILE_FAULT (fp, file_size_fail_prob, e);

  DBG_ASSERT (i_file, fp);

  struct stat st;

  if (unlikely (fstat (fp->fd, &st) == -1)) {
    error_causef (e, ERR_IO, "fstat: %s", strerror (errno));
    return error_trace (e);
  }

  return (i64)st.st_size;
}

static i64
posix_pread_all (void *_fp, void *dest, const u64 n, const u64 offset, error *e)
{
  i_file *fp = _fp;

  // I_FILE_FAULT (fp, pread_once_fail_prob, e);

  DBG_ASSERT (i_file, fp);
  ASSERT (dest);
  ASSERT (n > 0);

  u8 *_dest = (u8 *)dest;
  u64 nread = 0;

  while (nread < n) {
    ASSERT (n > nread);

    u64           toread = n - nread;
    // I_FILE_FAULT (fp, // pread_once_fail_prob, e);
    // I_FILE_CONDITION_AMOUNT (fp, // pread_once_some_prob, toread);

    const ssize_t _nread = pread (fp->fd, _dest + nread, toread, (off_t)(offset + nread));
    if (_nread == 0) {
      return (i64)nread;
    }
    if (unlikely (_nread < 0 && errno != EINTR)) {
      return error_causef (e, ERR_IO, "pread: %s", strerror (errno));
    }
    nread += (u64)_nread;
  }

  ASSERT (nread == n);

  return (i64)nread;
}

static err_t
posix_pwrite_all (void *_fp, const void *src, const u64 n, const u64 offset, error *e)
{
  i_file *fp = _fp;

  DBG_ASSERT (i_file, fp);
  ASSERT (src);
  ASSERT (n > 0);

  const u8 *_src     = (const u8 *)src;
  u64       nwritten = 0;

  while (nwritten < n) {
    ASSERT (n > nwritten);

    u64           towrite = n - nwritten;

    // Mayb fault on write
    // I_FILE_FAULT (fp, // pwrite_once_fail_prob, e);

    // Or maybe only write some data
    // I_FILE_CONDITION_AMOUNT (fp, // pwrite_once_some_prob, towrite);

    const ssize_t _nw = pwrite (fp->fd, _src + nwritten, n - nwritten, (off_t)(offset + nwritten));

    if (unlikely (_nw < 0 && errno != EINTR)) {
      return error_causef (e, ERR_IO, "pwrite: %s", strerror (errno));
    }
    nwritten += (u64)_nw;
  }

  ASSERT (nwritten == n);
  return SUCCESS;
}

////////////////////////////////////////////////////////////
// IO Vec

static err_t
posix_writev_all (void *_fp, struct bytes *iov, const int iovcnt, error *e)
{
  i_file *fp = _fp;

  // I_FILE_FAULT (fp, writev_all_fail_prob, e);

  DBG_ASSERT (i_file, fp);
  ASSERT (iov);
  ASSERT (iovcnt > 0 && iovcnt <= 2);

  u64 total = 0;
  for (int i = 0; i < iovcnt; i++) {
    total += iov[i].len;
  }

  ASSERT (total > 0);

  u64           nwritten  = 0;
  struct bytes *cur       = iov;
  int           remaining = iovcnt;

  while (nwritten < total) {
    struct iovec sys_iov[2];
    for (int i = 0; i < remaining; i++) {
      sys_iov[i].iov_base = cur[i].head;
      sys_iov[i].iov_len  = cur[i].len;
    }

    const ssize_t ret = writev (fp->fd, sys_iov, remaining);

    if (unlikely (ret < 0 && errno != EINTR)) {
      return error_causef (e, ERR_IO, "writev: %s", strerror (errno));
    }

    if (ret <= 0) {
      continue;
    }

    nwritten += (u64)ret;
    u64 skip = (u64)ret;
    while (skip > 0 && remaining > 0) {
      if (skip >= cur->len) {
        skip -= cur->len;
        cur++;
        remaining--;
      } else {
        cur->head = (u8 *)cur->head + skip;
        cur->len -= skip;
        skip = 0;
      }
    }
  }

  ASSERT (nwritten == total);
  return SUCCESS;
}

////////////////////////////////////////////////////////////
// Stream Read / Write

static i64
posix_read_all (void *_fp, void *dest, const u64 nbytes, error *e)
{
  i_file *fp = _fp;

  DBG_ASSERT (i_file, fp);
  ASSERT (dest);
  ASSERT (nbytes > 0);

  u8 *_dest = (u8 *)dest;
  u64 nread = 0;

  while (nread < nbytes) {
    ASSERT (nbytes > nread);

    u64           toread = nbytes - nread;
    // I_FILE_FAULT (fp, read_once_fail_prob, e);
    // I_FILE_CONDITION_AMOUNT (fp, read_once_some_prob, toread);

    const ssize_t _nread = read (fp->fd, _dest + nread, toread);

    if (_nread == 0) {
      return (i64)nread;
    }

    if (unlikely (_nread < 0)) {
      if (likely (errno == EINTR || errno == EWOULDBLOCK)) {
        return 0;
      }
      return error_causef (e, ERR_IO, "read: %s", strerror (errno));
    }

    nread += (u64)_nread;
  }

  ASSERT (nread == nbytes);
  return (i64)nread;
}

static err_t
posix_write_all (void *_fp, const void *src, const u64 nbytes, error *e)
{
  i_file *fp = _fp;

  DBG_ASSERT (i_file, fp);
  ASSERT (src);
  ASSERT (nbytes > 0);

  const u8 *_src     = (const u8 *)src;
  u64       nwritten = 0;

  while (nwritten < nbytes) {
    ASSERT (nbytes > nwritten);

    u64           towrite = nbytes - nwritten;
    // I_FILE_FAULT (fp, write_once_fail_prob, e);
    // I_FILE_CONDITION_AMOUNT (fp, write_once_some_prob, towrite);

    const ssize_t _nw     = write (fp->fd, _src + nwritten, towrite);
    if (unlikely (_nw < 0 && errno != EINTR)) {
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
posix_truncate (void *_fp, const u64 bytes, error *e)
{
  i_file *fp = _fp;

  // I_FILE_FAULT (fp, truncate_fail_prob, e);

  DBG_ASSERT (i_file, fp);

  if (unlikely (ftruncate (fp->fd, (off_t)bytes) == -1)) {
    return error_causef (e, ERR_IO, "ftruncate: %s", strerror (errno));
  }

  return SUCCESS;
}

static err_t
_posix_fallocate (void *_fp, const u64 bytes, error *e)
{
  i_file *fp = _fp;

  // I_FILE_FAULT (fp, fallocate_fail_prob, e);

  DBG_ASSERT (i_file, fp);

#if defined(__APPLE__)
  fstore_t store = {
      .fst_flags   = F_ALLOCATECONTIG,
      .fst_posmode = F_PEOFPOSMODE,
      .fst_offset  = 0,
      .fst_length  = (off_t)bytes,
  };
  if (unlikely (fcntl (fp->fd, F_PREALLOCATE, &store) == -1)) {
    store.fst_flags = F_ALLOCATEALL;
    if (unlikely (fcntl (fp->fd, F_PREALLOCATE, &store) == -1)) {
      return error_causef (e, ERR_IO, "F_PREALLOCATE: %s", strerror (errno));
    }
  }
  if (unlikely (ftruncate (fp->fd, (off_t)bytes) == -1)) {
    return error_causef (e, ERR_IO, "ftruncate: %s", strerror (errno));
  }
#else
  const int ret = posix_fallocate (fp->fd, 0, (off_t)bytes);

  if (unlikely (ret != 0)) {
    return error_causef (e, ERR_IO, "posix_fallocate: %s", strerror (ret));
  }
#endif

  return SUCCESS;
}

static i64
posix_seek (void *_fp, const u64 offset, const seek_t whence, error *e)
{
  i_file *fp = _fp;

  // I_FILE_FAULT (fp, seek_fail_prob, e);

  DBG_ASSERT (i_file, fp);

  int w;
  switch (whence) {
    case I_SEEK_SET: {
      w = SEEK_SET;
      break;
    }
    case I_SEEK_CUR: {
      w = SEEK_CUR;
      break;
    }
    case I_SEEK_END: {
      w = SEEK_END;
      break;
    }
    default: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }

  errno           = 0;
  const off_t ret = lseek (fp->fd, (off_t)offset, w);

  if (unlikely (ret == (off_t)-1)) {
    error_causef (e, ERR_IO, "lseek: %s", strerror (errno));
    return error_trace (e);
  }

  return (i64)ret;
}

static struct i_file_vtable posix_file_vtable = {
    .close      = _posix_close,
    .fsync      = posix_fsync,
    .file_size  = posix_file_size,
    .read_all   = posix_read_all,
    .pread_all  = posix_pread_all,
    .write_all  = posix_write_all,
    .pwrite_all = posix_pwrite_all,
    .writev_all = posix_writev_all,
    .truncate   = posix_truncate,
    .fallocate  = _posix_fallocate,
    .seek       = posix_seek,
#ifdef TESTING
    .test_data = NULL,
#endif
};

i_file
create_default_file (int fd)
{
  return (i_file){
      .fd    = fd,
      .table = &posix_file_vtable,
  };
}

/**
void register_test_file_data(struct test_file_data *data) {
  posix_file_vtable.test_data = data;
}
*/
