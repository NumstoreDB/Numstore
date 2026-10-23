#include "core/os/ns_os.h"

////////////////////////////
/// Composite file helpers
///
/// Built only out of vtable primitives - they are here so the short read
/// check isn't copy pasted at every call site.

/// pread exactly [n] bytes - a short read is ERR_CORRUPT, not EOF
err_t
file_pread_all_expect (i_file f, void *dest, const u64 n, const u64 offset, error *e)
{
  const i64 ret = f.table->pread_all (f.self, dest, n, offset, e);
  if (ret < 0) {
    return error_trace (e);
  }

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

/// read exactly [nbytes] bytes - a short read is ERR_CORRUPT, not EOF
err_t
file_read_all_expect (i_file f, void *dest, const u64 nbytes, error *e)
{
  const i64 ret = f.table->read_all (f.self, dest, nbytes, e);
  if (ret < 0) {
    return error_trace (e);
  }

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
