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

#include "c_specx.h"
#include "nscore/compile_config.h"
#include "nscore/page_delegate.h"
#include "nscore/page_h.h"
#include "nscore/pager.h"
#include "nscore/pages/var_page.h"
#include "nscore/types.h"
#include "nscore/var.h"

/*
 * Advance params->vp from the current PG_VAR_PAGE or PG_VAR_TAIL to the
 * next overflow page in the chain.  Used when a variable's serialised name
 * and type data spans more than one page.
 */
static err_t
ns_read_var_page_advance (struct ns_read_var_page_params *params, error *e)
{
  page_h next = page_h_create ();

  pgno npg = dlgt_get_ovnext (page_h_ro (params->vp));

  if (npg == PGNO_NULL)
  {
    error_causef (e, ERR_CORRUPT, "var page missing overflow pointer");
    goto failed;
  }

  WRAP (pgr_get_writable (&next, params->tx, PG_VAR_TAIL, npg, params->p, e));

  if ((pgr_release (params->p, params->vp, PG_VAR_PAGE | PG_VAR_TAIL, e))) { goto failed; }

  page_h_xfer_ownership_ptr (params->vp, &next);

  return SUCCESS;

failed:
  return error_trace (e);
}

err_t
ns_read_var_page (struct ns_read_var_page_params *params, error *e)
{
  ASSERT (params->vp->mode != PHM_NONE);
  ASSERT (params->tx);
  ASSERT (page_h_type (params->vp) == PG_VAR_PAGE);

  p_size        lread = 0;
  struct cbytes head;
  char         *vstr = NULL;
  u8           *tstr = NULL;

  // Save for the end to reset var page
  pgno start    = page_h_pgno (params->vp);
  bool writable = params->vp->mode == PHM_X;

  // Temporary allocator
  struct chunk_alloc temp;
  chunk_alloc_create_default (&temp);

  // Initialize the simple stuff
  u16    vlen     = vp_get_vlen (page_h_ro (params->vp));
  u16    tlen     = vp_get_tlen (page_h_ro (params->vp));
  pgno   rpt_root = vp_get_root (page_h_ro (params->vp));
  b_size nbytes   = vp_get_nbytes (page_h_ro (params->vp));
  pgno   var_root = page_h_pgno (params->vp);

  // Quick check on the length
  if (params->check)
  {
    if (vlen != params->check->len)
    {
      params->matches = false;
      goto theend;
    }
  }

  // Allocate variable name
  if (params->save_vname)
  {
    vstr = chunk_malloc (params->alloc, 1, vlen, e);
    if (vstr == NULL) { goto failed; }
  }
  else
  {
    vstr = chunk_malloc (&temp, 1, vlen, e);
    if (vstr == NULL) { goto failed; }
  }

  if (params->save_type)
  {
    tstr = chunk_malloc (&temp, 1, tlen, e);
    if (tstr == NULL) { goto failed; }
  }
  else
  {
    tstr = NULL; // We don't event touch this
  }

  // Read the variable name
  u16 rread = 0;
  head      = dlgt_get_bytes_imut (page_h_ro (params->vp));
  while (rread < vlen)
  {
    // We exhausted this page - move forward one
    if (lread == head.len)
    {
      if (ns_read_var_page_advance (params, e)) { goto failed; }

      lread = 0;
      head  = dlgt_get_bytes_imut (page_h_ro (params->vp));
    }

    // NAME READ
    u16 toread = vlen - rread;
    u16 bavail = head.len - lread;
    u16 next   = MIN (bavail, toread);

    memcpy (&vstr[rread], &head.head[lread], next);
    rread += next;
    lread += next;
  }

  // Quick termination on string data
  if (params->check)
  {
    if (memcmp (params->check->data, vstr, vlen) != 0)
    {
      params->matches = false;
      goto theend;
    }
  }

  if (params->save_type)
  {
    // Read the type bytes
    rread = 0;
    while (rread < tlen)
    {
      // We exhausted this page - move forward one
      if (lread == head.len)
      {
        // Advance forward one node and reset local and head
        if (ns_read_var_page_advance (params, e)) { goto failed; }
        lread = 0;
        head  = dlgt_get_bytes_imut (page_h_ro (params->vp));
      }

      // MIN(available in this node, available to be read left)
      u16 avail = head.len - lread;
      u16 left  = tlen - rread;
      u16 next  = MIN (avail, left);

      // Do the read
      memcpy (&tstr[rread], &head.head[lread], next);
      rread += next;
      lread += next;
    }

    // We just finished reading everything - expect us to be done
    if (dlgt_get_ovnext (page_h_ro (params->vp)) != PGNO_NULL)
    {
      error_causef (
          e,
          ERR_CORRUPT,
          "var page read complete but overflow "
          "pointer is non-null"
      );
      goto failed;
    }
  }

  params->dest->rpt_root = rpt_root;
  params->dest->nbytes   = nbytes;
  params->dest->var_root = var_root;

  if (params->save_type)
  {
    struct deserializer d     = dsrlizr_create (tstr, tlen);
    struct type        *dtype = type_deserialize (&d, params->alloc, e);
    if (dtype == NULL) { goto failed; }
    params->dest->dtype = dtype;
  }

  if (params->save_vname) { params->dest->vname = (struct string){.data = NULL, .len = 0}; }

  params->matches = true;

theend:
  // Reset back to head page
  if (page_h_pgno (params->vp) != start)
  {
    if ((pgr_release (params->p, params->vp, PG_VAR_TAIL, e))) { goto failed; }
    if ((pgr_get_maybe_writable (
            params->vp,
            params->tx,
            PG_VAR_PAGE,
            start,
            params->p,
            writable,
            e
        )))
    {
      goto failed;
    }
  }

  chunk_alloc_free_all (&temp);
  return SUCCESS;

failed:
  chunk_alloc_free_all (&temp);
  return error_trace (e);
}
