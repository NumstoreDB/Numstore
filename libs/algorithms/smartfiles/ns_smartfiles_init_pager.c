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

#include "ns_error.h"
#include "ns_string.h"
#include "pager/ns_page_fixture.h"
#include "pager/ns_pager.h"
#include "smartfiles/ns_smartfiles_algorithms.h"
#include "types/ns_types.h"
#include "types/ns_variables.h"
#include "var/ns_var_algorithms.h"

err_t
smartfiles_init_pager (struct pager *p, error *e)
{
  DBG_ASSERT (pager, p);

  if (!pgr_isnew (p)) {
    return SUCCESS;
  }

  // Initialize the upfront hash page
  if (ns_init_var_hash_map (p, e) < 0) {
    return error_trace (e);
  }

  // Create the default variable
  struct txn tx;
  if (pgr_begin_txn (&tx, p, e) < 0) {
    return error_trace (e);
  }

  struct string name = strfcstr (DEFAULT_VARIABLE);
  struct type   type = (struct type){.type = T_PRIM, .p = U8};
  if (ns_var_create (p, &tx, name, &type, e) < 0) {
    return error_trace (e);
  }

  if (pgr_commit (p, &tx, e) < 0) {
    return error_trace (e);
  }

  return SUCCESS;
}
