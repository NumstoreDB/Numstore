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

#ifndef NS_FILE_H
#define NS_FILE_H

#include "core/ns_bytes.h"
#include "core/ns_error.h"
#include "core/ns_stdtypes.h"

typedef enum
{
  I_SEEK_END,
  I_SEEK_CUR,
  I_SEEK_SET,
} seek_t;

struct i_file_vtable
{
  // Properties
  err_t (*close) (void *fp, error *e);
  err_t (*eof) (void *fp, error *e);
  err_t (*fsync) (void *fp, error *e);
  i64 (*file_size) (void *fp, error *e);

  // Read
  i64 (*read_all) (void *fp, void *dest, u64 nbytes, error *e);
  i64 (*pread_all) (void *fp, void *dest, u64 n, u64 offset, error *e);

  // Write
  err_t (*write_all) (void *fp, const void *src, u64 nbytes, error *e);
  err_t (*pwrite_all) (void *fp, const void *src, u64 n, u64 offset, error *e);
  err_t (*writev_all) (void *fp, struct bytes *arrs, int iovcnt, error *e);

  // Other
  err_t (*truncate) (void *fp, u64 bytes, error *e);
  err_t (*fallocate) (void *fp, u64 bytes, error *e);
  i64 (*seek) (void *fp, u64 offset, seek_t whence, error *e);
};

typedef struct
{
  const struct i_file_vtable *table;
#if PLATFORM_WINDOWS
  HANDLE handle;
#else
  int fd;
#endif
} i_file;

#if PLATFORM_WINDOWS
i_file create_default_file (HANDLE h);
#else
i_file create_default_file (int fd);
#endif

#define i_close(fp, e)                      (fp)->table->close (fp, e)
#define i_eof(fp, e)                        (fp)->table->eof (fp, e)
#define i_fsync(fp, e)                      (fp)->table->fsync (fp, e)
#define i_file_size(fp, e)                  (fp)->table->file_size (fp, e)
#define i_read_all(fp, dest, nbytes, e)     (fp)->table->read_all (fp, dest, nbytes, e)
#define i_pread_all(fp, dest, n, offset, e) (fp)->table->pread_all (fp, dest, n, offset, e)
#define i_write_all(fp, src, nbytes, e)     (fp)->table->write_all (fp, src, nbytes, e)
#define i_pwrite_all(fp, src, n, offset, e) (fp)->table->pwrite_all (fp, src, n, offset, e)
#define i_writev_all(fp, arrs, iovcnt, e)   (fp)->table->writev_all (fp, arrs, iovcnt, e)
#define i_truncate(fp, bytes, e)            (fp)->table->truncate (fp, bytes, e)
#define i_fallocate(fp, bytes, e)           (fp)->table->fallocate (fp, bytes, e)
#define i_seek(fp, offset, whence, e)       (fp)->table->seek (fp, offset, whence, e)

/*-----------------------------------------------------------------------------
 * SUBSECTION: Default Methods
 * ----------------------------------------------------------------------------
 * @brief Composite helpers built entirely from i_file_vtable primitives
 *----------------------------------------------------------------------------*/

HEADER_FUNC err_t
i_pread_all_expect (i_file *fp, void *dest, const u64 n, const u64 offset, error *e)
{
  const i64 ret = i_pread_all (fp, dest, n, offset, e);
  WRAP (ret);

  if (unlikely ((u64)ret != n)) {
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

  if (unlikely ((u64)ret != nbytes)) {
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

#endif
