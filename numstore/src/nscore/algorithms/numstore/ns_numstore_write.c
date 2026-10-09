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

#include "core/ns_error.h"
#include "nscore/algorithms/numstore/ns_numstore_algorithms.h"
#include "nscore/algorithms/rope/ns_rope_algorithms.h"

sb_size
numstore_write_from_name (
    struct pager *NONNULL       p,
    struct txn *NONNULL         tx,
    struct string               name,  // Name of the variable
    struct user_stride          ustr,  // Stride to write
    struct arena_alloc *NONNULL alloc, // Allocator for variable in get
    struct variable *NULLABLE   var,   // If not null - save the variable
    struct stream *NONNULL      src,   // Input stream
    error *NONNULL              e
)
{
  DBG_ASSERT (pager, p);
  DBG_ASSERT (ns_txn, tx);
  DBG_ASSERT (string, &name);
  DBG_ASSERT (user_stride, &ustr);
  DBG_ASSERT (arena_alloc, alloc);
  DBG_ASSERT (stream, src);
  DBG_ASSERT (clean_error, e);

  struct variable _var;
  if (var == NULL) {
    var = &_var;
  }
  if (numstore_get (p, tx, false, name, alloc, var, e) < 0) {
    return error_trace (e);
  }

  return numstore_write (p, tx, var, ustr, src, e);
}

sb_size
numstore_write (
    struct pager *NONNULL    p,
    struct txn *NONNULL      tx,
    struct variable *NONNULL var,
    struct user_stride       ustr,
    struct stream *NONNULL   src,
    error *NONNULL           e
)
{
  DBG_ASSERT (pager, p);
  DBG_ASSERT (ns_txn, tx);
  DBG_ASSERT (variable, var);
  DBG_ASSERT (user_stride, &ustr);
  DBG_ASSERT (stream, src);
  DBG_ASSERT (clean_error, e);

  // Resolve sizes
  t_size tsize = type_byte_size (var->dtype);
  ASSERT (var->nbytes % tsize == 0); // From valid var (assert)
  b_size        len = var->nbytes / tsize;

  // Resolve length based on the stride
  struct stride stride;
  if (stride_resolve (&stride, ustr, len, e)) {
    return error_trace (e);
  }

  return ns_rope_write (
      (struct ns_write_params){
          .p      = p,
          .src    = src,
          .tx     = tx,
          .root   = var->rpt_root,
          .size   = tsize,
          .bofst  = tsize * stride.start,
          .stride = stride.stride,
          .nelem  = stride.nelems,
      },
      e
  );
}
