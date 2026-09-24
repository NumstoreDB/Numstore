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

#include "nscore/algorithms/numstore/ns_numstore_algorithms.h"
#include "nscore/algorithms/rope/ns_rope_algorithms.h"

sb_size
numstore_read_from_name (
    struct pager       *p,
    struct txn         *tx,
    struct string       name,   // Name of the variable
    struct user_stride  ustr,   // Stride to read
    struct arena_alloc *valloc, // Allocator for variable in get
    struct variable    *var,    // If not null - save the variable
    struct stream      *dest,   // Output stream
    error              *e
)
{
  WITH_OPT_VARIABLE (p, tx, name, valloc, var, e, numstore_read (p, tx, var, ustr, dest, e));
}

sb_size
numstore_read (
    struct pager      *p,
    struct txn        *tx,
    struct variable   *var,
    struct user_stride ustr,
    struct stream     *dest,
    error             *e
)
{
  // Resolve sizes
  t_size tsize = type_byte_size (var->dtype);

  b_size len   = var->nbytes;

  // A consistent database has this be a multiple of tsize
  if (len % tsize != 0) {
    error_causef (e, ERR_CORRUPT, "Variable: invalid byte size");
    goto failed;
  }
  len /= tsize;

  // Resolve length based on the stride
  struct stride stride; // Resolved stride
  if (stride_resolve (&stride, ustr, len, e)) {
    goto failed;
  }

  // READ
  sb_size ret = ns_rope_read (
      (struct ns_read_params){
          .p      = p,
          .dest   = dest,
          .tx     = tx,
          .root   = var->rpt_root,
          .size   = tsize,
          .bofst  = tsize * stride.start,
          .stride = stride.stride,
          .nelem  = stride.nelems,
      },
      e
  );
  if (ret < 0) {
    goto failed;
  }

  return ret;

failed:
  return error_trace (e);
}
