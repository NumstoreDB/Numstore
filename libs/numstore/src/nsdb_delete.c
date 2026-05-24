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

#include "nscore/errors.h"
#include "nscore/nshandle.h"
#include "nscore/var.h"
#include "numstore.h"

#include <c_specx.h>

static err_t
_nsdb_delete (struct nshandle *db, const char *vname, error *e)
{
  struct txn auto_txn;

  // BEGIN TXN
  WRAP_GOTO (nsh_auto_begin_txn (db, e), failed);

  i_log_debug ("DELETE (txn = %" PRtxid "): %s\n", db->atx->tid, vname);

  struct string vnamestr = strfcstr (vname);
  {
    // DELETE
    struct ns_var_delete_params params = {
        .p     = db->root->p,
        .tx    = db->atx,
        .vname = strfcstr (vname),
    };
    err_t err = ns_var_delete (params, e);
    if (err == ERR_VARIABLE_NE)
    {
      // It's ok - just return the error
      goto commit;
    }
    if (err < SUCCESS) { goto failed_rollback; }
  }

commit:

  if (nsh_auto_commit (db, e)) { goto failed_rollback; }
  return error_trace (e);

failed_rollback:

  nsh_auto_rollback (db);

failed:
  return error_trace (e);
}

int
nsdb_delete (nsdb_t *_smf, const char *vname)
{
  struct nshandle *smf = (struct nshandle *)_smf;

  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  return _nsdb_delete (smf, vname, &smf->e);
}
