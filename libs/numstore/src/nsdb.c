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
#include "nscore/pager.h"

#include <c_specx.h>

// smfile

int
nsdb_perror (nsdb_t *ns, const char *prefix)
{ return nsh_perror ((struct nshandle *)ns, prefix); }
const char *
nsdb_strerror (nsdb_t *ns)
{ return nsh_strerror ((struct nshandle *)ns); }
int
nsdb_cleanup (const char *path)
{ return nsh_cleanup (path); }
nsdb_t *
nsdb_new_context (nsdb_t *n)
{ return (nsdb_t *)nsh_new_context ((struct nshandle *)n); }
int
nsdb_close (nsdb_t *ns)
{ return nsh_close ((struct nshandle *)ns); }
int
_nsdb_crash (nsdb_t *ns)
{ return nsh_crash ((struct nshandle *)ns); }
int
nsdb_begin (nsdb_t *_smf)
{
  int ret = nsh_begin ((struct nshandle *)_smf);
  if (ret == 0) { i_log_debug ("BEGIN TXN: %" PRtxid "\n", ((struct nshandle *)_smf)->atx->tid); }
  return ret;
}
int
nsdb_commit (nsdb_t *_smf)
{
  i_log_debug ("COMMIT: %" PRtxid "\n", ((struct nshandle *)_smf)->atx->tid);
  return nsh_commit ((struct nshandle *)_smf);
}
int
nsdb_rollback (nsdb_t *smf)
{
  i_log_debug ("ROLLBACK: %" PRtxid "\n", ((struct nshandle *)smf)->atx->tid);
  return nsh_rollback ((struct nshandle *)smf);
}
