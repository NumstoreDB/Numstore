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

#include "nscore/nshandle.h"
#include "nscore/pager.h"

#include <c_specx.h>

static err_t
_nsh_close (struct nshandle *n, error *e)
{
  struct nshandle_root *root = n->root;
  nsh_root_release (root, n);
  if (root->count == 0) { return nsh_root_close (root, &root->e); }
  return SUCCESS;
}

int
nsh_close (struct nshandle *ns)
{ return _nsh_close (ns, &ns->e); }
