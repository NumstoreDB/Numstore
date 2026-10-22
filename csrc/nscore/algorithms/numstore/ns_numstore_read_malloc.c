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
#include "core/ns_stream.h"
#include "core/os/ns_malloc.h"
#include "nscore/algorithms/numstore/ns_numstore_algorithms.h"
#include "nscore/algorithms/rope/ns_rope_algorithms.h"

err_t
numstore_read_malloc_from_name (
    struct pager *NONNULL       p,
    struct txn *NONNULL         tx,
    struct string               name,
    struct user_stride          ustr,
    struct arena_alloc *NONNULL valloc,
    struct variable *NULLABLE   var,
    void *NONNULL *NULLABLE     dest,
    b_size *NONNULL             dlen,
    struct i_mem                mem,
    error *NONNULL              e
)
{
  DBG_ASSERT (pager, p);
  DBG_ASSERT (ns_txn, tx);
  DBG_ASSERT (string, &name);
  DBG_ASSERT (user_stride, &ustr);
  DBG_ASSERT (arena_alloc, valloc);
  ASSERT (dest);
  ASSERT (dlen);
  DBG_ASSERT (i_mem, &mem);
  DBG_ASSERT (clean_error, e);

  // First, fetch the variable
  struct variable _var;
  if (var == NULL) {
    var = &_var;
  }
  if (numstore_get (p, tx, false, name, valloc, var, e) < 0) {
    return error_trace (e);
  }

  return numstore_read_malloc (p, tx, var, ustr, dest, dlen, mem, e);
}

err_t
numstore_read_malloc (
    struct pager *NONNULL    p,
    struct txn *NONNULL      tx,
    struct variable *NONNULL var,
    struct user_stride       ustr,
    void *NONNULL *NULLABLE  dest,
    b_size *NONNULL          dlen,
    struct i_mem             mem,
    error *NONNULL           e
)
{
  DBG_ASSERT (pager, p);
  DBG_ASSERT (ns_txn, tx);
  DBG_ASSERT (variable, var);
  DBG_ASSERT (user_stride, &ustr);
  ASSERT (dest);
  ASSERT (dlen);
  DBG_ASSERT (i_mem, &mem);
  DBG_ASSERT (clean_error, e);

  // Resolve sizes
  t_size tsize = type_byte_size (var->dtype);
  ASSERT (var->nbytes % tsize == 0); // Variable is valid (from assert)
  b_size        len = var->nbytes / tsize;

  // Resolve length based on the stride
  struct stride stride; // Resolved stride
  if (stride_resolve (&stride, ustr, len, e)) {
    return error_trace (e);
  }

  // Quit early if nothing to read
  if (stride.nelems == 0) {
    *dlen = 0;
    *dest = NULL;
    return SUCCESS;
  }

  // Do the allocation
  void *buffer = mem.table->malloc (mem.self, stride.nelems, tsize, e);
  if (buffer == NULL) {
    return error_trace (e);
  }

  // Create output stream
  ostream_create_from (stream, buffer, stride.nelems * tsize);

  // Do the read
  sb_size ret = ns_rope_read (
      (struct ns_read_params){
          .p      = p,
          .dest   = &stream,
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
    mem.table->free (mem.self, buffer);
    return error_trace (e);
  }

  // Set return values
  *dlen = ret * tsize;
  *dest = buffer;

  return SUCCESS;
}
