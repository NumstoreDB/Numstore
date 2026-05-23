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
#include "nscore/compiler.h"
#include "nscore/nshandle.h"
#include "nscore/var.h"

static int
_nsdb_create (struct nshandle *db, const char *name, const char *type, error *e)
{
  struct chunk_alloc                 temp;    // Allocator for get operation
  struct ns_var_get_or_create_params gparams; // Get or create operation

  struct string vname = strfcstr (name); // Variable name
  chunk_alloc_create_default (&temp);

  // Compile type
  struct type dtype;
  WRAP_GOTO (compile_type (&dtype, type, &temp, e), failed);

  // BEGIN TXN
  WRAP_GOTO (nsh_auto_begin_txn (db, e), failed);

  // GET OR CREATE VARIABLE
  {
    gparams = (struct ns_var_get_or_create_params){
        .p     = db->root->p,
        .tx    = db->atx,
        .vname = vname,
        .type  = &dtype,
        .alloc = &temp,
    };
    WRAP_GOTO (ns_var_get_or_create (&gparams, e), failed_rollback);
  }

  // COMMIT
  WRAP_GOTO (nsh_auto_commit (db, e), failed_rollback);
  chunk_alloc_free_all (&temp);

  return SUCCESS;

failed_rollback:

  nsh_auto_rollback (db);

failed:
  chunk_alloc_free_all (&temp);
  return error_trace (e);
}

int
nsdb_create (nsdb_t *_smf, const char *name, const char *type)
{
  struct nshandle *smf = (struct nshandle *)_smf;

  i_log_debug ("CREATE: %s %s\n", name, type);

  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  return _nsdb_create (smf, name, type, &smf->e);
}
