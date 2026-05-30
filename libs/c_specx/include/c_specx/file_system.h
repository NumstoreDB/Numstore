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

#ifndef C_SPECX_FILE_SYSTEM_H
#define C_SPECX_FILE_SYSTEM_H

#include <c_specx/bytes.h>
#include <c_specx/error.h>
#include <c_specx/platform.h>
#include <c_specx/stdtypes.h>

////////////////////////////////////////////////////////////
// FILE_SYSTEM

typedef struct i_file_vtable        i_file_vtable;
typedef struct i_file_system_vtable i_file_system_vtable;
typedef struct i_file               i_file;

typedef enum
{
  I_SEEK_END,
  I_SEEK_CUR,
  I_SEEK_SET,
} seek_t;

struct i_file_vtable
{
  ////////////////////////////////////////////////////////////
  // Properties
  err_t (*i_close) (i_file *fp, error *e);
  err_t (*i_eof) (i_file *fp, error *e);
  err_t (*i_fsync) (const i_file *fp, error *e);
  i64 (*i_file_size) (const i_file *fp, error *e);
  err_t (*i_flock) (const i_file *fp, bool *already_locked, error *e);
  err_t (*i_funlock) (const i_file *fp, error *e);

  ////////////////////////////////////////////////////////////
  // Read
  i64 (*i_read_some) (const i_file *fp, void *dest, u64 nbytes, error *e);
  i64 (*i_read_all) (const i_file *fp, void *dest, u64 nbytes, error *e);
  i64 (*i_pread_some) (
      const i_file *fp,
      void         *dest,
      u64           n,
      u64           offset,
      error        *e
  );
  i64 (*i_pread_all) (
      const i_file *fp,
      void         *dest,
      u64           n,
      u64           offset,
      error        *e
  );
  i64 (*i_readv_some) (
      const i_file       *fp,
      const struct bytes *arrs,
      int                 iovcnt,
      error              *e
  );
  i64 (*i_readv_all) (
      const i_file *fp,
      struct bytes *arrs,
      int           iovcnt,
      error        *e
  );

  ////////////////////////////////////////////////////////////
  // Write
  i64 (*i_write_some) (const i_file *fp, const void *src, u64 nbytes, error *e);
  err_t (*i_write_all) (
      const i_file *fp,
      const void   *src,
      u64           nbytes,
      error        *e
  );
  i64 (*i_pwrite_some) (
      const i_file *fp,
      const void   *src,
      u64           n,
      u64           offset,
      error        *e
  );
  err_t (*i_pwrite_all) (
      const i_file *fp,
      const void   *src,
      u64           n,
      u64           offset,
      error        *e
  );
  i64 (*i_writev_some) (
      const i_file       *fp,
      const struct bytes *arrs,
      int                 iovcnt,
      error              *e
  );
  err_t (*i_writev_all) (
      const i_file *fp,
      struct bytes *arrs,
      int           iovcnt,
      error        *e
  );

  ////////////////////////////////////////////////////////////
  // Other
  err_t (*i_truncate) (const i_file *fp, u64 bytes, error *e);
  err_t (*i_fallocate) (i_file *fp, u64 bytes, error *e);
  i64 (*i_seek) (const i_file *fp, u64 offset, seek_t whence, error *e);
};

struct i_file_system_vtable
{
  ////////////////////////////////////////////////////////////
  // Open
  err_t (*i_open_rw) (
      i_file_system_vtable *vfs,
      i_file               *dest,
      const char           *fname,
      error                *e
  );
  err_t (*i_open_r) (
      i_file_system_vtable *vfs,
      i_file               *dest,
      const char           *fname,
      error                *e
  );
  err_t (*i_open_w) (
      i_file_system_vtable *vfs,
      i_file               *dest,
      const char           *fname,
      error                *e
  );

  ////////////////////////////////////////////////////////////
  // Others
  err_t (*i_remove_quiet) (
      i_file_system_vtable *vfs,
      const char           *fname,
      error                *e
  );
  err_t (*i_unlink) (i_file_system_vtable *vfs, const char *name, error *e);
  err_t (*i_mkdir) (i_file_system_vtable *vfs, const char *name, error *e);
  err_t (*i_mkdir_quiet) (
      i_file_system_vtable *vfs,
      const char           *name,
      error                *e
  );
  err_t (*i_rm_rf) (i_file_system_vtable *vfs, const char *path, error *e);

  ////////////////////////////////////////////////////////////
  // Wrappers
  err_t (*i_access_rw) (i_file_system_vtable *vfs, const char *fname, error *e);
  bool (*i_exists_rw) (i_file_system_vtable *vfs, const char *fname);
  err_t (*i_touch) (i_file_system_vtable *vfs, const char *fname, error *e);
  err_t (*i_dir_exists) (
      i_file_system_vtable *vfs,
      const char           *fname,
      bool                 *dest,
      error                *e
  );
  err_t (*i_file_exists) (
      i_file_system_vtable *vfs,
      const char           *fname,
      bool                 *dest,
      error                *e
  );
};

struct i_file
{
#if PLATFORM_WINDOWS
  HANDLE handle;
#else
  int fd;
#endif
  i_file_vtable        *fvtable;
  i_file_system_vtable *fsvtable;
};

extern struct i_file_vtable        default_fvtable;
extern struct i_file_system_vtable default_fsvtable;

////////////////////////////////////////////////////////////
// Open / Close

HEADER_FUNC err_t
i_open_rw (i_file *dest, const char *fname, error *e)
{
  return default_fsvtable.i_open_rw (&default_fsvtable, dest, fname, e);
}
HEADER_FUNC err_t
i_open_r (i_file *dest, const char *fname, error *e)
{
  return default_fsvtable.i_open_r (&default_fsvtable, dest, fname, e);
}
HEADER_FUNC err_t
i_open_w (i_file *dest, const char *fname, error *e)
{
  return default_fsvtable.i_open_w (&default_fsvtable, dest, fname, e);
}
HEADER_FUNC err_t
i_close (i_file *fp, error *e)
{
  return fp->fvtable->i_close (fp, e);
}
HEADER_FUNC err_t
i_eof (i_file *fp, error *e)
{
  return fp->fvtable->i_eof (fp, e);
}
HEADER_FUNC err_t
i_fsync (const i_file *fp, error *e)
{
  return fp->fvtable->i_fsync (fp, e);
}

////////////////////////////////////////////////////////////
// Positional Read / Write

HEADER_FUNC i64
i_pread_some (const i_file *fp, void *dest, u64 n, u64 offset, error *e)
{
  return fp->fvtable->i_pread_some (fp, dest, n, offset, e);
}
HEADER_FUNC i64
i_pread_all (const i_file *fp, void *dest, u64 n, u64 offset, error *e)
{
  return fp->fvtable->i_pread_all (fp, dest, n, offset, e);
}

HEADER_FUNC i64
i_pwrite_some (const i_file *fp, const void *src, u64 n, u64 offset, error *e)
{
  return fp->fvtable->i_pwrite_some (fp, src, n, offset, e);
}
HEADER_FUNC err_t
i_pwrite_all (const i_file *fp, const void *src, u64 n, u64 offset, error *e)
{
  return fp->fvtable->i_pwrite_all (fp, src, n, offset, e);
}

////////////////////////////////////////////////////////////
// IO Vec

HEADER_FUNC i64
i_writev_some (const i_file *fp, const struct bytes *arrs, int iovcnt, error *e)
{
  return fp->fvtable->i_writev_some (fp, arrs, iovcnt, e);
}
HEADER_FUNC err_t
i_writev_all (const i_file *fp, struct bytes *arrs, int iovcnt, error *e)
{
  return fp->fvtable->i_writev_all (fp, arrs, iovcnt, e);
}
HEADER_FUNC i64
i_readv_some (const i_file *fp, const struct bytes *arrs, int iovcnt, error *e)
{
  return fp->fvtable->i_readv_some (fp, arrs, iovcnt, e);
}
HEADER_FUNC i64
i_readv_all (const i_file *fp, struct bytes *arrs, int iovcnt, error *e)
{
  return fp->fvtable->i_readv_all (fp, arrs, iovcnt, e);
}

////////////////////////////////////////////////////////////
// Stream Read / Write

HEADER_FUNC i64
i_read_some (const i_file *fp, void *dest, u64 nbytes, error *e)
{
  return fp->fvtable->i_read_some (fp, dest, nbytes, e);
}
HEADER_FUNC i64
i_read_all (const i_file *fp, void *dest, u64 nbytes, error *e)
{
  return fp->fvtable->i_read_all (fp, dest, nbytes, e);
}
HEADER_FUNC i64
i_write_some (const i_file *fp, const void *src, u64 nbytes, error *e)
{
  return fp->fvtable->i_write_some (fp, src, nbytes, e);
}
HEADER_FUNC err_t
i_write_all (const i_file *fp, const void *src, u64 nbytes, error *e)
{
  return fp->fvtable->i_write_all (fp, src, nbytes, e);
}

////////////////////////////////////////////////////////////
// Others

HEADER_FUNC err_t
i_truncate (const i_file *fp, u64 bytes, error *e)
{
  return fp->fvtable->i_truncate (fp, bytes, e);
}
HEADER_FUNC err_t
i_fallocate (i_file *fp, u64 bytes, error *e)
{
  return fp->fvtable->i_fallocate (fp, bytes, e);
}
HEADER_FUNC i64
i_file_size (const i_file *fp, error *e)
{
  return fp->fvtable->i_file_size (fp, e);
}
HEADER_FUNC err_t
i_flock (const i_file *fp, bool *already_locked, error *e)
{
  return fp->fvtable->i_flock (fp, already_locked, e);
}
HEADER_FUNC err_t
i_funlock (const i_file *fp, error *e)
{
  return fp->fvtable->i_funlock (fp, e);
}
HEADER_FUNC i64
i_seek (const i_file *fp, u64 offset, seek_t whence, error *e)
{
  return fp->fvtable->i_seek (fp, offset, whence, e);
}
HEADER_FUNC err_t
i_remove_quiet (const char *fname, error *e)
{
  return default_fsvtable.i_remove_quiet (&default_fsvtable, fname, e);
}
HEADER_FUNC err_t
i_unlink (const char *name, error *e)
{
  return default_fsvtable.i_unlink (&default_fsvtable, name, e);
}
HEADER_FUNC err_t
i_mkdir (const char *name, error *e)
{
  return default_fsvtable.i_mkdir (&default_fsvtable, name, e);
}
HEADER_FUNC err_t
i_mkdir_quiet (const char *name, error *e)
{
  return default_fsvtable.i_mkdir_quiet (&default_fsvtable, name, e);
}
HEADER_FUNC err_t
i_rm_rf (const char *path, error *e)
{
  return default_fsvtable.i_rm_rf (&default_fsvtable, path, e);
}

////////////////////////////////////////////////////////////
// Wrappers

HEADER_FUNC err_t
i_access_rw (const char *fname, error *e)
{
  return default_fsvtable.i_access_rw (&default_fsvtable, fname, e);
}
HEADER_FUNC bool
i_exists_rw (const char *fname)
{
  return default_fsvtable.i_exists_rw (&default_fsvtable, fname);
}
HEADER_FUNC err_t
i_touch (const char *fname, error *e)
{
  return default_fsvtable.i_touch (&default_fsvtable, fname, e);
}
HEADER_FUNC err_t
i_dir_exists (const char *fname, bool *dest, error *e)
{
  return default_fsvtable.i_dir_exists (&default_fsvtable, fname, dest, e);
}
HEADER_FUNC err_t
i_file_exists (const char *fname, bool *dest, error *e)
{
  return default_fsvtable.i_file_exists (&default_fsvtable, fname, dest, e);
}

////////////////////////////////////////////////////////////
// Helpers

HEADER_FUNC err_t
i_pread_all_expect (
    i_file   *fp,
    void     *dest,
    const u64 n,
    const u64 offset,
    error    *e
)
{
  const i64 ret = i_pread_all (fp, dest, n, offset, e);
  WRAP (ret);

  if (unlikely ((u64)ret != n))
  {
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

  if (unlikely ((u64)ret != nbytes))
  {
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

#endif // C_SPECX_FILE_SYSTEM_H
