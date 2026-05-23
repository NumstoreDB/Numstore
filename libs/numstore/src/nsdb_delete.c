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

int
nsdb_delete (nsdb_t *_smf, const char *name)
{
  struct nshandle *smf = (struct nshandle *)_smf;

  i_log_debug ("DELETE: %s\n", name);

  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  return nsh_delete (smf, name);
}
