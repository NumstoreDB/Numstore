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
#include "nscore/nshandle.h"
#include "nscore/pager.h"
#include "smfile.h"

static err_t _nsh_commit (struct nshandle *smf, error *e) {
  if (smf->atx == NULL) {
    return error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "Can't commit transaction, not a part of an existing transaction");
  }

  WRAP (pgr_commit (smf->root->p, smf->atx, e));
  smf->atx = NULL;

  return SUCCESS;
}

int nsh_commit (struct nshandle *smf) {
  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  return _nsh_commit (smf, &smf->e);
}
