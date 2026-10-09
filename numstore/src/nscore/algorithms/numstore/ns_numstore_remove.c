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
#include "nscore/algorithms/var/ns_var_algorithms.h"

sb_size
numstore_remove_from_name (
    struct pager *NONNULL       p,
    struct txn *NONNULL         tx,
    struct string               name,
    struct user_stride          ustr,
    struct arena_alloc *NONNULL valloc,
    struct variable *NULLABLE   var,
    struct stream *NULLABLE     dest,
    error *NONNULL              e
)
{
  DBG_ASSERT (pager, p);
  DBG_ASSERT (ns_txn, tx);
  DBG_ASSERT (string, &name);
  DBG_ASSERT (user_stride, &ustr);
  DBG_ASSERT (arena_alloc, valloc);
  DBG_ASSERT_IF_NN (stream, dest);
  DBG_ASSERT (clean_error, e);

  // First, fetch the variable
  struct variable _var;
  if (var == NULL) {
    var = &_var;
  }
  if (numstore_get (p, tx, false, name, valloc, var, e) < 0) {
    return error_trace (e);
  }

  // Then, do the remove
  return numstore_remove (p, tx, var, ustr, dest, e);
}

sb_size
numstore_remove (
    struct pager *NONNULL    p,
    struct txn *NONNULL      tx,
    struct variable *NONNULL var,
    struct user_stride       ustr,
    struct stream *NULLABLE  dest,
    error *NONNULL           e
)
{
  DBG_ASSERT (pager, p);
  DBG_ASSERT (ns_txn, tx);
  DBG_ASSERT (variable, var);
  DBG_ASSERT (user_stride, &ustr);
  DBG_ASSERT_IF_NN (stream, dest);
  DBG_ASSERT (clean_error, e);

  // Resolve sizes
  t_size tsize = type_byte_size (var->dtype);
  ASSERT (var->nbytes % tsize == 0); // Variable is valid (from assert)
  b_size        len = var->nbytes / tsize;

  // Resolve length based on the stride
  struct stride stride;
  if (stride_resolve (&stride, ustr, len, e)) {
    return error_trace (e);
  }

  struct ns_remove_params rparams = {
      .p      = p,
      .dest   = dest,
      .tx     = tx,
      .root   = var->rpt_root,
      .size   = tsize,
      .bofst  = tsize * stride.start,
      .stride = stride.stride,
      .nelem  = stride.nelems,
  };
  sb_size ret = ns_remove (&rparams, e);
  if (ret < 0) {
    return error_trace (e);
  }

  // Update new sizes and root
  b_size newsize = var->nbytes + (len * tsize);
  pgno   newroot = rparams.root;

  if (ns_var_update_by_var_root (p, tx, var->var_root, newroot, newsize, e) < 0) {
    return error_trace (e);
  }

  return ret;
}
