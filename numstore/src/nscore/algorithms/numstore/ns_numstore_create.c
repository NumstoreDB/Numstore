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
#include "nscore/algorithms/var/ns_var_algorithms.h"

err_t
numstore_create (
    struct pager       *p,
    struct txn         *tx,
    struct string       name,
    struct type         type,
    struct arena_alloc *valloc,
    struct variable    *var,
    error              *e
)
{
  // Log the call
  i_log_debug ("CREATE (txn = %" PRtxid "): %.*s\n", tx->tid, strfmt (&vname));

  // Get or create
  struct ns_var_get_or_create_params gparams = {
      .p     = p,
      .tx    = tx,
      .vname = name,
      .type  = &type,
      .alloc = valloc,
  };
  if (ns_var_get_or_create (&gparams, e)) {
    goto theend;
  }

  if (var) {
    *var = gparams.dest;
  }

theend:
  return error_trace (e);
}
