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
#include "nscore/nshandle.h"
#include "nscore/var.h"

#include <c_specx.h>

static char *
_nsdb_type_str (struct nshandle *db, const char *name, error *e)
{
  struct chunk_alloc       temp;
  struct ns_var_get_params gparams;
  char                    *ret  = NULL;
  struct string            vname = strfcstr (name);

  chunk_alloc_create_default (&temp);

  WRAP_GOTO (nsh_auto_begin_txn (db, e), failed);

  gparams = (struct ns_var_get_params){
      .p     = db->root->p,
      .tx    = db->atx,
      .vname = vname,
      .alloc = &temp,
  };

  err_t err = ns_var_get (&gparams, e);
  if (err == ERR_VARIABLE_NE)
  {
    e->cause_code = SUCCESS;
    e->cmlen      = 0;
    goto commit;
  }
  WRAP_GOTO (err, failed_rollback);

  ret = type_tostr (gparams.dest.dtype);

commit:
  WRAP_GOTO (nsh_auto_commit (db, e), failed_rollback);
  chunk_alloc_free_all (&temp);
  return ret;

failed_rollback:
  nsh_auto_rollback (db);

failed:
  chunk_alloc_free_all (&temp);
  return NULL;
}

char *
nsdb_type_str (nsdb_t *_smf, const char *name)
{
  struct nshandle *smf = (struct nshandle *)_smf;

  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  return _nsdb_type_str (smf, name, &smf->e);
}
