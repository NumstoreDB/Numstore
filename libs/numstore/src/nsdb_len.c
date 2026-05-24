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
#include "nscore/rope.h"
#include "nscore/txn.h"
#include "nscore/var.h"

#include <c_specx.h>

static sb_size
_nsdb_len (struct nshandle *db, const char *name, error *e)
{
  struct chunk_alloc temp;
  chunk_alloc_create_default (&temp);
  struct string            vname = strfcstr (name);
  struct ns_var_get_params gparams;
  b_size                   len;
  t_size                   tsize;

  // BEGIN TXN
  if (nsh_auto_begin_txn (db, e) < 0) { goto failed; }

  i_log_debug ("LEN (txn = %" PRtxid "): %s\n", db->atx->tid, name);

  // GET OR CREATE VARIABLE
  {
    gparams = (struct ns_var_get_params){
        .p     = db->root->p,
        .tx    = db->atx,
        .vname = vname,
        .alloc = &temp,
    };
    if (ns_var_get (&gparams, e)) { goto failed_rollback; }
  }

  // Resolve length
  {
    tsize = type_byte_size (gparams.dest.dtype);
    len   = gparams.dest.nbytes;

    if (len % tsize != 0)
    {
      error_causef (e, ERR_CORRUPT, "Variable: %s has invalid byte size", name);
      goto failed_rollback;
    }

    len /= tsize;
  }

  // COMMIT
  if (nsh_auto_commit (db, e) < 0) { goto failed_rollback; }

  chunk_alloc_free_all (&temp);

  return len;

failed_rollback:

  nsh_auto_rollback (db);

failed:
  chunk_alloc_free_all (&temp);
  return error_trace (e);
}

sb_size
nsdb_len (nsdb_t *_smf, const char *name)
{
  struct nshandle *smf = (struct nshandle *)_smf;

  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  return _nsdb_len (smf, name, &smf->e);
}
