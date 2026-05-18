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

#include "nscore/page_h.h"
#include "nscore/var.h"

err_t ns_init_var_hash_map (struct pager *p, error *e) {
  page_h hp = page_h_create ();

  // BEGIN TXN
  struct txn tx;
  if (pgr_begin_txn (&tx, p, e)) { goto failed; }

  // Upfront initialization
  if (pgr_isnew (p)) {
    // Create a new variable hash page
    if (pgr_new (&hp, p, &tx, PG_VAR_HASH_PAGE, e)) { goto failed; }

    // Next page should be valid
    //   this is a weak contract
    //   but assumes the structure of the pager,
    //   it's good enough but might need to change
    ASSERT (page_h_pgno (&hp) == VHASH_PGNO);

    if (pgr_release (p, &hp, PG_VAR_HASH_PAGE, e)) { goto failed; }
  }

  // COMMIT
  if (pgr_commit (p, &tx, e)) { goto failed; }

failed:
  return error_trace (e);
}
