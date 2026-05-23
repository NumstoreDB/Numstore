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
#include "nscore/errors.h"
#include "nscore/nshandle.h"
#include "nscore/var.h"

static err_t
_nsh_delete (struct nshandle *db, const char *vname, error *e)
{
  struct txn auto_txn;

  // BEGIN TXN
  WRAP_GOTO (nsh_auto_begin_txn (db, e), failed);

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
    WRAP_GOTO (err, failed_rollback);
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
nsh_delete (struct nshandle *ns, const char *vname)
{
  ns->e.cause_code = SUCCESS;
  ns->e.cmlen      = 0;
  return _nsh_delete (ns, vname, &ns->e);
}
