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

#include "_numstore.h"
#include "c_specx/chunk_alloc.h"
#include "c_specx/error.h"
#include "c_specx/memory.h"
#include "nscore/errors.h"
#include "nscore/nshandle.h"
#include "nscore/var.h"

static int
_nsdb_get_if_exists (
    struct nshandle *db,
    const char      *name,
    nsdb_var_t     **dest,
    error           *e
)
{
  nsdb_var_t              *ret = NULL;              // Return value
  struct ns_var_get_params gparams;                 // Get or create operation
  struct string            vname = strfcstr (name); // Variable name

  ret = i_malloc (1, sizeof *ret, e);
  if (ret == NULL)
  {
    return error_trace (e);
  }

  chunk_alloc_create_default (&ret->alloc);

  // BEGIN TXN
  WRAP_GOTO (nsh_auto_begin_txn (db, e), failed);

  i_log_debug (
      "GET IF EXISTS (txn = %" PRtxid
      ")"
      " - %s\n",
      db->atx->tid,
      name
  );

  // GET VARIABLE
  {
    gparams = (struct ns_var_get_params){
        .p     = db->root->p,
        .tx    = db->atx,
        .vname = vname,
        .alloc = &ret->alloc,
    };
    err_t err = ns_var_get (&gparams, e);
    if (err == ERR_VARIABLE_NE)
    {
      e->cause_code = SUCCESS;
      e->cmlen      = 0;
      i_free (ret);
      *dest = NULL;
      goto commit;
    }
    WRAP_GOTO (err, failed_rollback);

    ret->var = gparams.dest;
    *dest    = ret;
  }

commit:
  // COMMIT
  WRAP_GOTO (nsh_auto_commit (db, e), failed_rollback);

  return SUCCESS;

failed_rollback:

  nsh_auto_rollback (db);

failed:
  chunk_alloc_free_all (&ret->alloc);
  i_free (ret);
  return error_trace (e);
}

int
nsdb_get_if_exists (nsdb_t *_smf, nsdb_var_t **dest, const char *name)
{
  struct nshandle *smf = (struct nshandle *)_smf;

  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  return _nsdb_get_if_exists (smf, name, dest, &smf->e);
}
