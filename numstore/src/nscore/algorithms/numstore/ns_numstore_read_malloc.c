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

#include "core/os/ns_malloc.h"
#include "nscore/algorithms/numstore/ns_numstore_algorithms.h"
#include "nscore/algorithms/rope/ns_rope_algorithms.h"

err_t
numstore_read_malloc_from_name (
    struct pager       *p,
    struct txn         *tx,
    struct string       name,
    struct user_stride  ustr,
    struct arena_alloc *valloc,
    struct variable    *var,
    void              **dest,
    b_size             *dlen,
    struct i_mem        mem,
    error              *e
)
{
  WITH_OPT_VARIABLE (
      p,
      tx,
      name,
      valloc,
      var,
      e,
      numstore_read_malloc (p, tx, var, ustr, dest, dlen, mem, e)
  );
}

err_t
numstore_read_malloc (
    struct pager      *p,
    struct txn        *tx,
    struct variable   *var,
    struct user_stride ustr,
    void             **dest,
    b_size            *dlen,
    struct i_mem       mem,
    error             *e
)
{
  ASSERT (dest);

  // Resolve sizes
  t_size tsize = type_byte_size (var->dtype);

  b_size len   = var->nbytes;

  // A consistent database has this be a multiple of tsize
  if (len % tsize != 0) {
    error_causef (e, ERR_CORRUPT, "Variable: has invalid byte size");
    goto failed;
  }
  len /= tsize;

  // Resolve length based on the stride
  struct stride stride; // Resolved stride
  if (stride_resolve (&stride, ustr, len, e)) {
    goto failed;
  }

  if (stride.nelems == 0) {
    *dlen = 0;
    *dest = NULL;
    return SUCCESS;
  }

  void *buffer = mem.table->malloc (mem.self, stride.nelems, tsize, e);
  if (buffer == NULL) {
    goto failed;
  }

  struct stream          stream;
  struct stream_obuf_ctx octx;
  stream_obuf_init (&stream, &octx, buffer, stride.nelems * tsize);

  // READ
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
    goto failed;
  }

  if (dlen) {
    *dlen = ret * tsize;
  }

  *dest = buffer;

  return SUCCESS;

failed:
  return error_trace (e);
}
