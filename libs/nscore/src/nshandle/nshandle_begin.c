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

#include "nscore/lock_table.h"
#include "nscore/lt_lock.h"
#include "nscore/nshandle.h"
#include "nscore/pager.h"

#include <c_specx.h>

static err_t
_nsh_begin (struct nshandle *smf, error *e)
{
  if (smf->atx)
  {
    return error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "Can't start another transaction, already a part of an existing "
        "transaction: %" PRtxid ". Either commit or rollback first",
        smf->atx->tid
    );
  }

  WRAP (pgr_begin_txn (&smf->tx, smf->root->p, &smf->e));

  smf->is_auto_txn = 0;
  smf->atx         = &smf->tx;
  printf ("%" PRtxid "\n", smf->atx->tid);

  return SUCCESS;
}

int
nsh_begin (struct nshandle *smf)
{
  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  return _nsh_begin (smf, &smf->e);
}
