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

#include "core/os/ns_memory.h"
#include "nscore/algorithms/numstore/ns_numstore_algorithms.h"
#include "nscore/algorithms/rope/ns_rope_algorithms.h"
#include "nscore/algorithms/var/ns_var_algorithms.h"

void *
numstore_read_malloc (
    struct pager       *p,
    struct ns_txn      *tx,
    struct string       name,
    struct user_stride  ustr,
    struct arena_alloc *valloc,
    struct variable    *var,
    b_size             *dlen,
    struct i_mem        mem,
    error              *e
)
{
  // Get variable
  struct ns_var_get_params gparams = {
      .p     = p,
      .tx    = tx,
      .vname = name,
      .alloc = valloc,
  };
  if (ns_var_get (&gparams, e) < 0) {
    goto failed;
  }

  // Resolve sizes
  t_size tsize = type_byte_size (gparams.dest.dtype);

  b_size len   = gparams.dest.nbytes;

  // A consistent database has this be a multiple of tsize
  if (len % tsize != 0) {
    error_causef (e, ERR_CORRUPT, "Variable: %.*s has invalid byte size", strfmt (&name));
    goto failed;
  }
  len /= tsize;

  // Resolve length based on the stride
  struct stride stride; // Resolved stride
  if (stride_resolve (&stride, ustr, len, e)) {
    goto failed;
  }

  void *buffer = i_malloc (mem, stride.nelems, tsize, e);
  if (buffer == NULL) {
    goto failed;
  }

  struct stream          stream;
  struct stream_obuf_ctx octx;
  stream_obuf_init (&stream, &octx, buffer, stride.nelems * tsize);

  // READ
  sb_size ret = ns_read (
      (struct ns_read_params){
          .p      = p,
          .dest   = &stream,
          .tx     = tx,
          .root   = gparams.dest.rpt_root,
          .size   = tsize,
          .bofst  = tsize * stride.start,
          .stride = stride.stride,
          .nelem  = stride.nelems,
      },
      e
  );
  if (ret < 0) {
    i_free (mem, buffer);
    goto failed;
  }

  // Maybe save the variable
  if (var) {
    *var = gparams.dest;
  }

  if (dlen) {
    *dlen = ret;
  }

  return buffer;

failed:
  return NULL;
}
