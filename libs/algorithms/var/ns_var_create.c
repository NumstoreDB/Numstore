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
#include "ns_page.h"
#include "ns_page_h.h"
#include "ns_stdtypes.h"
#include "pager/ns_pager.h"
#include "types/ns_variables.h"
#include "var/ns_var_algorithms.h"
#include "var/ns_var_algorithms_internal.h"

#include <stddef.h>

/*
 * Create a new variable record in the variable hash table.
 *
 * Uses ns_find_var_page() in FP_CREATE mode to allocate a fresh PG_VAR_PAGE
 * in the correct hash-chain bucket.  The page is then initialised with the
 * variable's name, type, and zero nbytes/rpt_root (no data yet).
 *
 * Returns ERR_DUPLICATE_VARIABLE if a variable with this name already exists.
 */
spgno
ns_var_create (struct pager *p, struct txn *tx, struct string vname, struct type *type, error *e)
{
  DBG_ASSERT (pager, p);
  DBG_ASSERT (ns_txn, tx);

  page_h                         cur     = page_h_create ();

  struct ns_find_var_page_params fparams = {
      .tx    = tx,
      .p     = p,
      .alloc = NULL,

      .vname = vname,
      .dvar  = NULL,
      .mode  = FP_CREATE,

      .hpos  = 0,
      .prev  = NULL,
      .cur   = &cur,
  };

  if (ns_find_var_page (&fparams, e) < 0) {
    goto failed;
  }

  struct variable var = {
      .vname    = vname,
      .dtype    = type,
      .nbytes   = 0,
      .rpt_root = PGNO_NULL,
  };

  struct ns_write_var_page_params write_params = {
      .p   = p,
      .tx  = tx,

      .vp  = &cur,
      .var = &var,
  };

  if (ns_write_var_page (&write_params, e) < 0) {
    goto failed;
  }

  if (pgr_release (p, &cur, PG_VAR_PAGE, e) < 0) {
    goto failed;
  }

  return SUCCESS;

failed:

  pgr_cancel_if_exists (&cur);

  return error_trace (e);
}
