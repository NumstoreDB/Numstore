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

#include "_smfile.h"
#include "nscore/nshandle.h"
#include "nscore/page_h.h"
#include "nscore/pager.h"
#include "nscore/types.h"
#include "nscore/var.h"

#include <c_specx.h>

smfile_t *
smfile_open (const char *path)
{
  struct nshandle *ret = nsh_open (path);

  if (ret == NULL) { return NULL; }

  // Create the default variable
  if (pgr_isnew (ret->root->p))
  {
    // BEGIN TXN
    struct txn tx;
    if (pgr_begin_txn (&tx, ret->root->p, &ret->e)) { goto failed; }

    struct ns_var_create_params params = {
        .p     = ret->root->p,
        .tx    = &tx,
        .vname = strfcstr (DEFAULT_VARIABLE),
        .type  = &(struct type){.type = T_PRIM, .p = U8},
    };
    if (ns_var_create (params, &ret->e)) { goto failed; }

    // COMMIT
    if (pgr_commit (ret->root->p, &tx, &ret->e)) { goto failed; }
  }

  return (smfile_t *)ret;

failed:
  nsh_close (ret);

  return NULL;
}
