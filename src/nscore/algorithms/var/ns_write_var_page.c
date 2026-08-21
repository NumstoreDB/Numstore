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

#include "core/ns_alloc.h"
#include "core/ns_bytes.h"
#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_serial.h"
#include "core/ns_stdtypes.h"
#include "core/ns_string.h"
#include "core/ns_utils.h"
#include "nscore/algorithms/var/ns_var_algorithms_internal.h"
#include "nscore/page/ns_page.h"
#include "nscore/page/ns_page_delegate.h"
#include "nscore/page/ns_page_h.h"
#include "nscore/page/ns_page_var_page.h"
#include "nscore/pager/ns_pager.h"
#include "nscore/types/ns_types.h"
#include "nscore/types/ns_variables.h"

#include <string.h>

static err_t
ns_write_var_page_advance (struct ns_write_var_page_params *params, error *e)
{
  page_h next = page_h_create ();

  // Create a tail page
  if (pgr_new (&next, params->p, params->tx, PG_VAR_TAIL, e)) {
    goto failed;
  }

  // Link with the existing page
  dlgtovlink (page_h_w (params->vp), page_h_w (&next));

  // Release current
  if ((pgr_release (params->p, params->vp, PG_VAR_PAGE | PG_VAR_TAIL, e))) {
    goto failed;
  }

  // And transfer next to current page
  page_h_xfer_ownership_ptr (params->vp, &next);

failed:
  return error_trace (e);
}

/*
 * Serialise a variable record into a PG_VAR_PAGE (plus any overflow tails).
 *
 * The inverse of _read_var_page.  The fixed-size fields (next, ovnext, vlen,
 * tlen, rpt_root, nbytes) are stamped into the page header first.  Then the
 * variable-length name and serialised type bytes are written sequentially
 * into the page's data area; overflow pages are allocated on demand via
 * write_var_page_advance() whenever the current page is full.
 *
 * On return, params->vp is re-wound to the original head page (the caller
 * still holds the page in write mode from before the call).
 */
err_t
ns_write_var_page (struct ns_write_var_page_params *params, error *e)
{
  ASSERT (params->vp->mode == PHM_X);
  ASSERT (page_h_type (params->vp) == PG_VAR_PAGE);
  ASSERT (params->tx);

  // Serialize dtype
  ALLOC_INIT (temp);

  // The serialized data
  u16 tlen             = type_get_serial_size (params->var->dtype);
  u8 *dtype_serialized = allocate (&temp, tlen, 1, e);

  if (dtype_serialized == NULL) {
    // Allocation failed
    return error_trace (e);
  } else {
    // Serialize type
    struct serializer srlzr = srlizr_create (dtype_serialized, tlen);
    type_serialize (&srlzr, params->var->dtype);
  }

  pgno start = page_h_pgno (params->vp);

  // Set the simple stuff
  vp_set_next (page_h_w (params->vp), PGNO_NULL);
  vp_set_ovnext (page_h_w (params->vp), PGNO_NULL);
  vp_set_vlen (page_h_w (params->vp), params->var->vname.len);
  vp_set_tlen (page_h_w (params->vp), tlen);
  vp_set_root (page_h_w (params->vp), params->var->rpt_root);
  vp_set_nbytes (page_h_w (params->vp), params->var->nbytes);

  // The bytes iterator - available bytes to write to
  struct bytes head     = dlgt_get_bytes (page_h_w (params->vp));

  // Total number of variable and type bytes written so far
  p_size       vwritten = 0;
  p_size       twritten = 0;

  // First, write the variable name
  p_size       lwritten = 0; // Local number of bytes written so far - resets on every new page
  while (vwritten < params->var->vname.len) {
    // Advance forward one new node and reset local written and head
    if (lwritten == head.len) {
      if (ns_write_var_page_advance (params, e)) {
        goto failed;
      }
      lwritten = 0;
      head     = dlgt_get_bytes (page_h_w (params->vp));
    }

    // MIN(available to write to, available to read from)
    u16 next = MIN (head.len - lwritten, params->var->vname.len - vwritten);
    ASSERT (next > 0);

    // Do the write and increment pointers
    memcpy (&head.head[lwritten], &params->var->vname.data[vwritten], next);
    vwritten += next;
    lwritten += next;
  }

  // Then, write the type (pretty much the same algorithm)
  while (twritten < tlen) {
    // Advance forward one new node and reset local written and head
    if (lwritten == head.len) {
      if (ns_write_var_page_advance (params, e)) {
        goto failed;
      }
      lwritten = 0;
      head     = dlgt_get_bytes (page_h_w (params->vp));
    }

    // MIN(available to write to, available to read from)
    u16 next = MIN (head.len - lwritten, tlen - twritten);
    ASSERT (next > 0);

    // Do the write and increment pointers
    memcpy (&head.head[lwritten], &dtype_serialized[twritten], next);
    twritten += next;
    lwritten += next;
  }

  /**
   * After we're done writing - we need to reset
   * params->vp to the starting (variable) page
   * to be considerate of the caller
   *
   * This might be an extra call that I could end up
   * removing - it's kind of bad practice
   */
  if (page_h_pgno (params->vp) != start) {
    // Release the tail (must be a tail because we're not at the starting page)
    if ((pgr_release (params->p, params->vp, PG_VAR_TAIL, e))) {
      goto theend;
    }

    // Get the starting node
    if ((pgr_get (params->vp, PG_VAR_PAGE, start, params->p, e))) {
      goto theend;
    }
  } else {
    // Release and get the page in read mode
    if (pgr_release (params->p, params->vp, PG_VAR_PAGE, e)) {
      goto theend;
    }
    if (pgr_get (params->vp, PG_VAR_PAGE, start, params->p, e)) {
      goto theend;
    }
  }

theend:
  ALLOC_CLOSE (temp);
  return error_trace (e);

failed:
  ALLOC_CLOSE (temp);
  return error_trace (e);
}
