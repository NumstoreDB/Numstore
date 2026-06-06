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
#include "alloc.h"
#include "c_specx/memory.h"
#include "nscore/nshandle.h"
#include "nscore/var.h"

static nsdb_var_t *
_nsdb_get (struct nshandle *db, const char *name, error *e)
{
  nsdb_var_t              *ret = NULL;              // Return value
  struct ns_var_get_params gparams;                 // Get or create operation
  struct string            vname = strfcstr (name); // Variable name

  ret = i_malloc (1, sizeof *ret, e);
  if (ret == NULL)
  {
    return NULL;
  }

  chunk_alloc_create_default (&ret->alloc);

  // BEGIN TXN
  WRAP_GOTO (nsh_auto_begin_txn (db, e), failed);

  i_log_debug (
      "GET (txn = %" PRtxid
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
    WRAP_GOTO (err, failed_rollback);

    ret->var = gparams.dest;
  }

  // COMMIT
  WRAP_GOTO (nsh_auto_commit (db, e), failed_rollback);

  return ret;

failed_rollback:

  nsh_auto_rollback (db);

failed:
  chunk_alloc_free_all (&ret->alloc);
  i_free (ret);
  return NULL;
}

nsdb_var_t *
nsdb_get (nsdb_t *_smf, const char *name)
{
  struct nshandle *smf = (struct nshandle *)_smf;

  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  return _nsdb_get (smf, name, &smf->e);
}
