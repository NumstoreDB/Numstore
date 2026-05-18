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
#include "c_specx.h"
#include "numstore/rope.h"
#include "numstore/txn.h"
#include "numstore/var.h"
#include "smfile.h"

static sb_size _smfile_psize (struct smfile *db, const char *name, error *e) {
  struct chunk_alloc temp;
  chunk_alloc_create_default (&temp);
  struct string vname = vname_or_default (name);
  b_size        ret;

  // BEGIN TXN
  if (_smfile_auto_begin_txn (db, e) < 0) { goto failed; }

  // GET OR CREATE VARIABLE
  struct ns_var_get_params gparams = {
      .p     = db->root->p,
      .tx    = db->atx,
      .vname = vname,
      .alloc = &temp,
  };
  if (ns_var_get (&gparams, e)) { goto failed; }

  ret = gparams.dest.nbytes;

  // COMMIT
  if (_smfile_auto_commit (db, e) < 0) { goto failed_rollback; }

  chunk_alloc_free_all (&temp);

  return ret;

failed_rollback:

  _smfile_auto_rollback (db);

failed:
  chunk_alloc_free_all (&temp);
  return error_trace (e);
}

sb_size smfile_psize (smfile_t *smf, const char *name) {
  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;
  return _smfile_psize (smf, name, &smf->e);
}
