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

#include <c_specx.h>
#include "nscore/compile_config.h"
#include "nscore/page_delegate.h"
#include "nscore/page_h.h"
#include "nscore/pager.h"
#include "nscore/pages/page.h"
#include "nscore/pages/var_hash_page.h"
#include "nscore/pages/var_page.h"
#include "nscore/var.h"

/*
 * Delete a variable and reclaim all its storage.
 *
 * Three steps:
 *
 *   1. _find_var_page() in FP_FIND mode locates the variable's PG_VAR_PAGE,
 *      keeping prev (the page or hash-bucket that points to cur) pinned.
 *
 *   2. ns_remove() deletes every element from the R+Tree.  After this,
 *      rparams.root is PGNO_NULL (the tree has been fully drained and freed).
 *
 *   3. Unlink cur from the hash chain.  Two sub-cases:
 *        - prev is PG_VAR_HASH_PAGE: the bucket pointer is cleared to
 *          cur->next (or PGNO_NULL if cur was the only node).
 *        - prev is PG_VAR_PAGE: prev->next is updated to skip cur.
 *      Then all PG_VAR_PAGE/PG_VAR_TAIL overflow pages chained from cur are
 *      deleted in sequence.
 */
err_t
ns_var_delete (struct ns_var_delete_params params, error *e)
{
  page_h prev   = page_h_create ();
  page_h cur    = page_h_create ();
  page_h ovnext = page_h_create ();

  struct variable var;

  struct ns_find_var_page_params fparams = {
      .p  = params.p,
      .tx = params.tx,

      .vname = params.vname,
      .dvar  = &var,
      .mode  = FP_FIND,

      .hpos = PGNO_NULL,
      .prev = &prev,
      .cur  = &cur,
  };

  if (ns_find_var_page (&fparams, e)) { goto failed; }
  pgr_upgrade (&prev, params.tx, PG_VAR_PAGE | PG_VAR_HASH_PAGE, params.p, e);
  pgr_upgrade (&cur, params.tx, PG_VAR_PAGE, params.p, e);

  struct ns_remove_params rparams = {
      .p      = params.p,
      .dest   = NULL,
      .tx     = params.tx,
      .root   = fparams.dvar->rpt_root,
      .size   = 1,
      .bofst  = 0,
      .stride = 1,
      .nelem  = fparams.dvar->nbytes,
  };

  if (ns_remove (&rparams, e)) { goto failed; }

  ASSERT (rparams.root == PGNO_NULL);

  switch (page_h_type (&prev))
  {
      // Previous is the root hash page
    case PG_VAR_HASH_PAGE:
    {
      vh_set_hash_value (page_h_w (&prev), fparams.hpos, vp_get_next (page_h_ro (&cur)));

      if (pgr_release (params.p, &prev, PG_VAR_HASH_PAGE, e)) { goto failed; }

      break;
    }

      // Otherwise, we just need to link prev->cur
    case PG_VAR_PAGE:
    {
      vp_set_next (page_h_w (&prev), vp_get_next (page_h_ro (&cur)));

      if (pgr_release (params.p, &prev, PG_VAR_PAGE, e)) { goto failed; }

      break;
    }
    default:
    {
      UNREACHABLE ();
    }
  }

  // Delete all overflow pages
  while (cur.mode != PHM_NONE)
  {
    pgno npg = dlgt_get_ovnext (page_h_ro (&cur));
    if (npg != PGNO_NULL)
    {
      if (pgr_get (&ovnext, PG_VAR_TAIL, npg, params.p, e)) { goto failed; }
    }

    if (pgr_delete_and_release (params.p, params.tx, &cur, e)) { goto failed; }

    page_h_xfer_ownership_ptr (&cur, &ovnext);
  }

  return error_trace (e);

failed:
  pgr_cancel_if_exists (params.p, &prev);
  pgr_cancel_if_exists (params.p, &cur);
  pgr_cancel_if_exists (params.p, &ovnext);

  return error_trace (e);
}
