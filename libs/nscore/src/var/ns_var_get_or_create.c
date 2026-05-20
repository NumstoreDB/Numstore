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

#include "c_specx.h"
#include "nscore/pager.h"
#include "nscore/types.h"
#include "nscore/var.h"

err_t ns_var_get_or_create (struct ns_var_get_or_create_params *params, error *e) {
  // Try to get the variable
  struct ns_var_get_params gparams = {
      .p     = params->p,
      .tx    = params->tx,
      .vname = params->vname,
      .alloc = params->alloc,
      // Result = dest
  };

  error_silence (e);
  err_t err = ns_var_get (&gparams, e);
  error_unsilence (e);
  if (err == ERR_VARIABLE_NE) {
    // Doesn't exist - reset and create it
    e->cause_code = SUCCESS;
    e->cmlen      = 0;

    // Create the variable
    struct ns_var_create_params cparams = {
        .p     = params->p,
        .tx    = params->tx,
        .vname = params->vname,
        .type  = params->type,
    };
    if (ns_var_create (cparams, e)) { goto failed; }

    // Try again
    if (ns_var_get (&gparams, e)) { goto failed; }

  } else if (err < 0) {
    goto failed;
  }

  if (!type_equal (params->type, gparams.dest.dtype)) {
    error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "Trying to create variable: %.*s but variable already exists and types are different",
        params->vname.len,
        params->vname.data);
    goto failed;
  }

  params->dest = gparams.dest;

failed:
  return error_trace (e);
}
