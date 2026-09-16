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
#include "nscore/algorithms/var/ns_var_algorithms.h"

sb_size
numstore_remove (
    struct pager       *p,
    struct ns_txn      *tx,
    struct string       name,  // Name of the variable
    struct user_stride  ustr,  // Stride to remove
    struct arena_alloc *alloc, // Allocator for variable in get
    struct variable    *var,   // If not null - save the variable
    struct stream      *dest,  // Output stream (can be null)
    error              *e
)
{
  // GET VARIABLE
  struct ns_var_get_params gparams = {
      .p     = p,
      .tx    = tx,
      .vname = name,
      .alloc = alloc,
  };

  if (ns_var_get (&gparams, e)) {
    goto failed;
  }

  // Resolve sizes
  t_size tsize = type_byte_size (gparams.dest.dtype);

  // Total size in bytes of the variable
  b_size len   = gparams.dest.nbytes;

  // A consistent database has this be a multiple of tsize
  if (len % tsize != 0) {
    error_causef (e, ERR_CORRUPT, "Variable: %.*s has invalid byte size", strfmt (&name));
    goto failed;
  }
  len /= tsize;

  // Resolve length based on the stride
  struct stride stride;
  if (stride_resolve (&stride, ustr, len, e)) {
    goto failed;
  }

  i_log_debug (
      "REMOVE (txn = %" PRtxid
      ")"
      " - %.*s"
      " size (bytes): %" PRt_size " curlen: %" PRb_size " curlen (bytes): %" PRb_size
      " Requested: "
      " start: %" PRId64 " stride: %" PRId64 " stop: %" PRId64 " start (bytes): %" PRId64
      " stride (bytes): %" PRId64 " stop (bytes): %" PRId64
      " Granted: "
      " start: %" PRIu64 " stride: %" PRIu64 " nelems: %" PRIu64 " start (bytes): %" PRIu64
      " stride (bytes): %" PRIu64 " nelems (bytes): %" PRIu64 "\n",
      tx->tid,
      strfmt (&name),
      tsize,
      len,
      gparams.dest.nbytes,
      ustr.present & START_PRESENT ? ustr.start : 0,
      ustr.present & STEP_PRESENT ? ustr.step : 0,
      ustr.present & STOP_PRESENT ? ustr.stop : 0,
      ustr.present & START_PRESENT ? tsize * ustr.start : 0,
      ustr.present & STEP_PRESENT ? tsize * ustr.step : 0,
      ustr.present & STOP_PRESENT ? tsize * ustr.stop : 0,
      stride.start,
      stride.stride,
      stride.nelems,
      tsize * stride.start,
      tsize * stride.stride,
      tsize * stride.nelems
  );

  // REMOVE
  struct ns_remove_params rparams = {
      .p      = p,
      .dest   = dest,
      .tx     = tx,
      .root   = gparams.dest.rpt_root,
      .size   = tsize,
      .bofst  = tsize * stride.start,
      .stride = stride.stride,
      .nelem  = stride.nelems,
  };
  sb_size ret = ns_remove (&rparams, e);
  if (ret < 0) {
    goto failed;
  }

  // UPDATE VARIABLE
  if (ns_var_update (
          (struct ns_var_update_params){
              .p      = p,
              .tx     = tx,
              .retr   = (struct var_retrieval){.type = VR_PG, .root = gparams.dest.var_root},
              .newpg  = rparams.root,
              .nbytes = gparams.dest.nbytes - (ret * tsize),
          },
          e
      )
      < 0) {
    goto failed;
  }

  if (var) {
    *var = gparams.dest;
  }

  return ret;

failed:
  return error_trace (e);
}
