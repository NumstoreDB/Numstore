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

#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "core/os/ns_os_vtable.h"

struct i_file
{
  const struct os_vtable *table;
#if PLATFORM_WINDOWS
  HANDLE handle;
#else
  int fd;
#endif
};

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
 * @brief Composite helpers built entirely from os_vtable file primitives
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
