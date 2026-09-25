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

#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "nscore/algorithms/var/ns_var_algorithms.h"
#include "nscore/algorithms/var/ns_var_algorithms_internal.h"
#include "nscore/page/ns_page.h"
#include "nscore/page/ns_page_h.h"
#include "nscore/page/ns_page_var_page.h"
#include "nscore/pager/ns_pager.h"

#include <stddef.h>

/*
 * Update rpt_root and nbytes on a variable page addressed by page number.
 *
 * Used when the caller already holds the variable page's pgno from an
 * earlier _find_var_page() call, avoiding a second hash-chain traversal.
 */
err_t
ns_var_update_by_var_root (
    struct pager *p,
    struct txn   *tx,
    pgno          root,
    pgno          newpg,
    b_size        nbytes,
    error        *e
)
{
  page_h cur = page_h_create ();

  if (pgr_get_writable (&cur, tx, PG_VAR_PAGE, root, p, e)) {
    goto failed;
  }

  vp_set_root (page_h_w (&cur), newpg);
  vp_set_nbytes (page_h_w (&cur), nbytes);

  if (pgr_release (p, &cur, PG_VAR_PAGE, e)) {
    goto failed;
  }

  goto failed;

failed:
  if (error_trace (e)) {
    return error_trace (e);
  }
  return SUCCESS;
}

/*
 * Update rpt_root and nbytes on a variable page addressed by variable name.
 *
 * Walks the hash chain via _find_var_page() in FP_FIND mode, then upgrades
 * the page to writable and stamps the new root pgno and byte count.
 */
err_t
ns_var_update_by_name (
    struct pager *p,
    struct txn   *tx,
    struct string name,
    pgno          newpg,
    b_size        nbytes,
    error        *e
)
{
  page_h                         cur     = page_h_create ();

  struct ns_find_var_page_params fparams = {
      .p     = p,
      .tx    = tx,

      .vname = name,
      .dvar  = NULL,
      .mode  = FP_FIND,

      .hpos  = PGNO_NULL,
      .prev  = NULL,
      .cur   = &cur,
  };

  if (ns_find_var_page (&fparams, e)) {
    goto failed;
  }

  vp_set_root (page_h_w (&cur), newpg);
  vp_set_nbytes (page_h_w (&cur), nbytes);

  if (pgr_release (p, &cur, PG_VAR_PAGE, e)) {
    goto failed;
  }

  return SUCCESS;

failed:
  pgr_cancel_if_exists (&cur);
  return error_trace (e);
}
