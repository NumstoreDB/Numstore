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

#include "ns_arena_alloc.h"
#include "ns_bytes.h"
#include "ns_csx_assert.h"
#include "ns_error.h"
#include "ns_page.h"
#include "ns_page_delegate.h"
#include "ns_page_h.h"
#include "ns_page_var_page.h"
#include "ns_serial.h"
#include "ns_stdtypes.h"
#include "ns_string.h"
#include "ns_testing.h"
#include "ns_utils.h"
#include "os/ns_malloc.h"
#include "pager/ns_pager.h"
#include "types/ns_types.h"
#include "types/ns_variables.h"
#include "var/ns_var_algorithms_internal.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/*
 * Advance params->vp from the current PG_VAR_PAGE or PG_VAR_TAIL to the
 * next overflow page in the chain.  Used when a variable's serialised name
 * and type data spans more than one page.
 */
static err_t
ns_read_var_page_advance (struct ns_read_var_page_params *params, error *e)
{
  page_h next = page_h_create ();

  pgno   npg  = dlgt_get_ovnext (page_h_ro (params->vp));

  if (npg == PGNO_NULL) {
    error_causef (e, ERR_CORRUPT, "var page missing overflow pointer");
    goto failed;
  }

  WRAP (pgr_get_writable (&next, params->tx, PG_VAR_TAIL, npg, params->p, e));

  if (pgr_release (params->p, params->vp, PG_VAR_PAGE | PG_VAR_TAIL, e) < 0) {
    goto failed;
  }

  page_h_xfer_ownership_ptr (params->vp, &next);

  return SUCCESS;

failed:
  return error_trace (e);
}

err_t
ns_read_var_page (struct ns_read_var_page_params *params, error *e)
{
  ASSERT (params->vp->mode != PHM_NONE);
  ASSERT (page_h_type (params->vp) == PG_VAR_PAGE);

  // Save for the end to reset var page
  pgno start    = page_h_pgno (params->vp);
  bool writable = params->vp->mode == PHM_X;
  ASSERT (!writable || params->tx);

  // Temporary allocator
  ALLOC_INIT (temp);

  // Initialize the simple stuff
  u16    vlen     = vp_get_vlen (page_h_ro (params->vp));
  u16    tlen     = vp_get_tlen (page_h_ro (params->vp));
  pgno   rpt_root = vp_get_root (page_h_ro (params->vp));
  b_size nbytes   = vp_get_nbytes (page_h_ro (params->vp));
  pgno   var_root = page_h_pgno (params->vp);

  if (params->dest != NULL) {
    params->dest->rpt_root = rpt_root;
    params->dest->nbytes   = nbytes;
    params->dest->var_root = var_root;
  }

  // Quick check on the length
  if (params->check != NULL && vlen != params->check->len) {
    params->matches = false;
    goto theend;
  }

  // This is the "scanning string" if save_vname is true, it'll
  // be memcpy'ed at the end to the persistent params->alloc
  // It is one extra memcpy - but if you allocate this on
  // persistent, then the all variable names in the chain
  // will live on the passed memory, but it'd be a tiny
  // bit faster, but not worth the speed up
  //
  // TODO: an arena_malloc_commit that lets the arena roll back would remove
  // this extra copy
  char *vstr = arena_malloc (&temp, 1, vlen, e);
  if (vstr == NULL) {
    goto failed;
  }

  // This is the type string, it's only ever lives on
  // the temporary allocator and it's only ever used
  // if save type is true
  u8 *tstr = NULL;
  if (params->save_type) {
    tstr = arena_malloc (&temp, 1, tlen, e);
    if (tstr == NULL) {
      goto failed;
    }
  }

  // Read the variable name
  u16           total_read = 0; // Total read for the whole variable name
  p_size        local_read = 0; // Just the number of bytes read on each page
  struct cbytes head       = dlgt_get_bytes_imut (page_h_ro (params->vp));
  while (total_read < vlen) {
    // We exhausted this page - move forward one
    if (local_read == head.len) {
      if (ns_read_var_page_advance (params, e) < 0) {
        goto failed;
      }

      local_read = 0;
      head       = dlgt_get_bytes_imut (page_h_ro (params->vp));
    }

    // Do the read of the name
    u16 toread = vlen - total_read;
    u16 bavail = head.len - local_read;
    u16 next   = MIN (bavail, toread);

    memcpy (&vstr[total_read], &head.head[local_read], next);
    total_read += next;
    local_read += next;
  }

  // Quick termination on string data
  if (params->check != NULL && (memcmp (params->check->data, vstr, vlen) != 0)) {
    params->matches = false;
    goto theend;
  }

  // Only execute this next block if we want to save the
  // type. This is because the type is tail data
  if (params->save_type) {
    // Read the type bytes
    total_read = 0;
    while (total_read < tlen) {
      // We exhausted this page - move forward one
      if (local_read == head.len) {
        // Advance forward one node and reset local and head
        if (ns_read_var_page_advance (params, e) < 0) {
          goto failed;
        }
        local_read = 0;
        head       = dlgt_get_bytes_imut (page_h_ro (params->vp));
      }

      // MIN(available in this node, available to be read left)
      u16 avail = head.len - local_read;
      u16 left  = tlen - total_read;
      u16 next  = MIN (avail, left);

      // Do the read
      memcpy (&tstr[total_read], &head.head[local_read], next);
      total_read += next;
      local_read += next;
    }

    // This check ensures that this page is truly the last
    // page of the chain. The last page should point next
    // to NULL, if it doesn't then it's an invalid variable
    if (dlgt_get_ovnext (page_h_ro (params->vp)) != PGNO_NULL) {
      error_causef (
          e,
          ERR_CORRUPT,
          "var page read complete but overflow "
          "pointer is non-null"
      );
      goto failed;
    }
  }

  // Set the values

  if (params->save_type) {
    // Copy the type onto the persistent allocator
    struct deserializer d     = dsrlizr_create (tstr, tlen);
    struct type        *dtype = type_deserialize (&d, params->alloc, e);
    if (dtype == NULL) {
      goto failed;
    }
    params->dest->dtype = dtype;
  }

  if (params->save_vname) {
    char *vstr_permanent = arena_malloc (params->alloc, vlen, 1, e);
    if (vstr_permanent == NULL) {
      goto failed;
    }
    memcpy (vstr_permanent, vstr, vlen);
    params->dest->vname = (struct string){.data = vstr_permanent, .len = vlen};
  }

  params->matches = true;

theend:
  // Reset back to head page
  if (page_h_pgno (params->vp) != start) {
    if (pgr_release (params->p, params->vp, PG_VAR_TAIL, e) < 0) {
      goto failed;
    }
    if (pgr_get_maybe_writable (params->vp, params->tx, PG_VAR_PAGE, start, params->p, writable, e)
        < 0) {
      goto failed;
    }
  }

  ALLOC_CLOSE (temp);
  return SUCCESS;

failed:
  ALLOC_CLOSE (temp);
  return error_trace (e);
}
