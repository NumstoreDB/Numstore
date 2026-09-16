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
numstore_insert (
    struct pager       *p,
    struct ns_txn      *tx,
    struct string       vname,
    b_size              len,
    b_size              ofst,
    struct arena_alloc *valloc,
    struct variable    *var,
    struct stream      *src,
    error              *e
)
{
  // Skip len 0 inserts
  if (len == 0) {
    return 0;
  }

  // Get Variable
  struct ns_var_get_params gparams = (struct ns_var_get_params){
      .p     = p,
      .tx    = tx,
      .vname = vname,
      .alloc = valloc,
  };
  if (ns_var_get (&gparams, e) < 0) {
    goto failed;
  }

  // Resolve sizes
  t_size tsize = type_byte_size (gparams.dest.dtype);
  b_size bofst = var_resolve_index (&gparams.dest, tsize * ofst);

  i_log_debug (
      "INSERT (txn = %" PRtxid
      ")"
      " - %.*s"
      " size (bytes): %" PRt_size " curlen: %" PRb_size " curlen (bytes): %" PRb_size
      " Requested: "
      " ofst: %" PRId64 " ofst (bytes): %" PRId64 " nelem: %" PRId64 " nbytes (bytes): %" PRId64
      " Granted: "
      " start: %" PRIu64 " start (bytes): %" PRIu64 " granted: %" PRIu64
      " granted (bytes): %" PRIu64 "\n",
      tx->tid,
      strfmt (&name),
      tsize,
      gparams.dest.nbytes / tsize,
      gparams.dest.nbytes,
      ofst,
      ofst * tsize,
      len,
      len * tsize,
      bofst / tsize,
      bofst,
      len,
      len * tsize
  );

  // Insert
  struct ns_insert_params iparams = {
      .p     = p,
      .src   = src,
      .tx    = tx,
      .root  = gparams.dest.rpt_root,
      .bofst = bofst,
      .bytes = len * tsize,
  };
  sb_size ret = ns_insert (&iparams, e);
  if (ret != (sb_size)(len * tsize)) {
    goto failed;
  }

  // Update Varible
  if (ns_var_update (
          (struct ns_var_update_params){
              .p  = p,
              .tx = tx,
              .retr =
                  (struct var_retrieval){
                      .type = VR_PG,
                      .root = gparams.dest.var_root,
                  },
              .newpg  = iparams.root,
              .nbytes = gparams.dest.nbytes + ret,
          },
          e
      )
      < 0) {
    goto failed;
  }

  ASSERT (ret % tsize == 0);
  ret /= tsize;

  // save the variable
  if (var) {
    *var = gparams.dest;
  }

  return ret;

failed:
  return error_trace (e);
}
