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

#include "algorithms/nsdb/var/algorithms.h"
#include "algorithms/smfile/_smfile.h"
#include "c_specx/dev/error.h"
#include "c_specx/memory/chunk_alloc.h"
#include "pager.h"

err_t
_ns_var_get_or_create (struct _ns_var_get_or_create_params *params, error *e)
{
  // Try to get the variable
  struct _ns_var_get_params gparams = {
    .p = params->p,
    .tx = params->tx,
    .vname = params->vname,
    .alloc = params->alloc,
  };

  error_silence (e);
  err_t err = _ns_var_get (&gparams, e);
  error_unsilence (e);
  if (err == ERR_VARIABLE_NE)
    {
      // Doesn't exist - reset and create it
      e->cause_code = SUCCESS;
      e->cmlen = 0;

      // Create the variable
      struct _ns_var_create_params cparams = {
        .p = params->p,
        .tx = params->tx,
        .vname = params->vname,
      };
      if (_ns_var_create (cparams, e))
        {
          goto failed;
        }

      // Try again
      if (_ns_var_get (&gparams, e))
        {
          goto failed;
        }
    }
  else if (err < 0)
    {
      goto failed;
    }

  params->dest = gparams.dest;

failed:
  return error_trace (e);
}

#ifndef NTEST
TEST (_ns_var_get_or_create)
{
  error e = error_create ();
  struct smfile *sf = _smfile_remove_and_open ("test", &e);

  pgr_begin_txn (&sf->tx, sf->root->p, &sf->e);
  struct chunk_alloc alloc;
  chunk_alloc_create_default (&alloc);

  struct _ns_var_get_or_create_params params = {
    .p = sf->root->p,
    .tx = &sf->tx,
    .vname = strfcstr ("foobar"),
    .alloc = &alloc,
  };

  _ns_var_get_or_create (&params, &sf->e);
  chunk_alloc_free_all (&alloc);

  pgr_commit (sf->root->p, &sf->tx, &e);

  smfile_close (sf);
}
#endif
