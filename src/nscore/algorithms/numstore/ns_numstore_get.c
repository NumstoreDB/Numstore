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
#include "nscore/algorithms/var/ns_var_algorithms.h"
#include "nscore/variables/ns_variables.h"

err_t
numstore_get (
    struct pager       *p,
    struct ns_txn      *tx,
    bool                if_exists,
    struct string       name,   // Name of the variable
    struct arena_alloc *valloc, // Allocator for variable in get
    struct variable    *var,    // If not null - save the variable
    error              *e
)
{
  i_log_debug ("GET (txn = %" PRtxid ") - %.*s\n", tx->tid, strfmt (&name));

  // Get Variable
  struct ns_var_get_params gparams = {
      .p     = p,
      .tx    = tx,
      .vname = name,
      .alloc = valloc,
  };

  err_t err = ns_var_get (&gparams, e);

  // If the variable doesn't exist, it's ok
  if (if_exists && err == ERR_VARIABLE_NE) {
    error_reset (e);
    var->dtype = NULL;
    *var       = (struct variable){0};
    goto theend;
  }

  if (err < 0) {
    goto theend;
  }

  if (var) {
    *var = gparams.dest;
  }

theend:
  return error_trace (e);
}
