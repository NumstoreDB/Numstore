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
numstore_insert_from_name (
    struct pager       *p,
    struct txn         *tx,
    struct string       vname,  // Name of the variable
    b_size              ofst,   // Offset (in elements)
    b_size              len,    // Length of the data
    struct arena_alloc *valloc, // Where to allocate the variable
    struct variable    *var,    // If not null - save the variable
    struct stream      *src,    // Input stream
    error              *e
)
{
  WITH_OPT_VARIABLE (p, tx, vname, valloc, var, e, numstore_insert (p, tx, var, ofst, len, src, e));
}

sb_size
numstore_insert (
    struct pager    *p,
    struct txn      *tx,
    struct variable *var,
    b_size           ofst,
    b_size           len,
    struct stream   *src,
    error           *e
)
{
  // Skip len 0 inserts
  if (len == 0) {
    return 0;
  }

  // Resolve sizes
  t_size                  tsize   = type_byte_size (var->dtype);
  b_size                  bofst   = var_resolve_index (var, tsize * ofst);

  // Insert
  struct ns_insert_params iparams = {
      .p     = p,
      .src   = src,
      .tx    = tx,
      .root  = var->rpt_root,
      .bofst = bofst,
      .bytes = len * tsize,
  };
  sb_size ret = ns_insert (&iparams, e);
  if (ret != (sb_size)(len * tsize)) {
    goto failed;
  }

  if (ns_var_update_by_var_root (p, tx, var->var_root, iparams.root, var->nbytes + ret, e) < 0) {
    goto failed;
  }

  ASSERT (ret % tsize == 0);
  ret /= tsize;

  return ret;

failed:
  return error_trace (e);
}
