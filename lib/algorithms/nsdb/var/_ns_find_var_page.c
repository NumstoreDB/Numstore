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

#include "algorithms/nsdb/var/algorithms.h"
#include "algorithms/smfile/_smfile.h"
#include "c_specx.h"
#include "c_specx/dev/error.h"
#include "c_specx/intf/os/file_system.h"
#include "c_specx/memory/chunk_alloc.h"
#include "pager.h"
#include "pager/page_fixture.h"
#include "pager/page_h.h"
#include "pages/page.h"
#include "pages/var_hash_page.h"
#include "pages/var_page.h"
#include "smfile.h"

static err_t
err_var_doesnt_exist (const struct string vname, error *e)
{
  if (vname.len > 10)
    {
      return error_causef (e, ERR_VARIABLE_NE, "Variable: %.*s... doesn't exist", 7, vname.data);
    }
  else
    {
      return error_causef (e, ERR_VARIABLE_NE, "Variable: %.*s doesn't exist", vname.len, vname.data);
    }
}

static err_t
err_var_already_exists (const struct string vname, error *e)
{
  if (vname.len > 10)
    {
      return error_causef (e, ERR_DUPLICATE_VARIABLE,
                           "Variable: %.*s... already exists", 7, vname.data);
    }
  else
    {
      return error_causef (e, ERR_DUPLICATE_VARIABLE,
                           "Variable: %.*s already exists", vname.len,
                           vname.data);
    }
}

static err_t
xfer_or_release (struct pager *p, page_h *dest, page_h *src, error *e)
{
  if (dest)
    {
      page_h_xfer_ownership_ptr (dest, src);
    }
  else
    {
      if (pgr_release (p, src, PG_VAR_HASH_PAGE | PG_VAR_PAGE, e))
        {
          goto theend;
        }
    }

theend:
  return error_trace (e);
}

err_t
_ns_find_var_page (struct _ns_find_var_page_params *pms, error *e)
{
  page_h prev = page_h_create ();
  page_h cur = page_h_create ();
  page_h npg = page_h_create ();

  // Temporary allocator while scanning nodes
  struct chunk_alloc temp;
  chunk_alloc_create_default (&temp);

  // Fetch the root node
  pms->hpos = vh_get_hash_pos (pms->vname);
  bool writable = pms->mode == FP_CREATE;

  if (pgr_get_maybe_writable (&prev, pms->tx, PG_VAR_HASH_PAGE, VHASH_PGNO, pms->p, writable, e))
    {
      goto failed;
    }
  pgno head = vh_get_hash_value (page_h_ro (&prev), pms->hpos);

  // Hash Chain Doesn't Exist
  if (head == PGNO_NULL)
    {
      switch (pms->mode)
        {
        case FP_FIND:
          {
            err_var_doesnt_exist (pms->vname, e);
            goto failed;
          }
        case FP_CREATE:
          {
            // Create a new variable page
            if (pgr_new (&cur, pms->p, pms->tx, PG_VAR_PAGE, e))
              {
                goto failed;
              }

            // vhp[pos] = new page
            vh_set_hash_value (page_h_w (&prev), pms->hpos, page_h_pgno (&cur));
            goto foundit;
          }
        }
    }

  // That hash chain exists
  else
    {
      // Fetch start hash chain
      if (pgr_get_maybe_writable (&cur, pms->tx, PG_VAR_PAGE, head, pms->p, writable, e))
        {
          goto failed;
        }

      // Find the end of the list
      while (true)
        {
          // Check if this var page matches
          struct _ns_read_var_page_params get_params = {
            .p = pms->p,
            .tx = pms->tx,
            .vp = &cur,
            .alloc = &temp,
            .dest = pms->dvar,
            .matches = false,
            .check = &pms->vname,
          };
          if (_ns_read_var_page (&get_params, e))
            {
              goto failed;
            }

          // Found it
          if (get_params.matches)
            {
              switch (pms->mode)
                {
                case FP_CREATE:
                  {
                    err_var_already_exists (pms->vname, e);
                    goto failed;
                  }
                case FP_FIND:
                  {
                    goto foundit;
                  }
                }
            }

          // Need to advance
          else
            {
              chunk_alloc_reset_all (&temp);

              // Continue
              pgno next = vp_get_next (page_h_ro (&cur));

              // End of the chain -
              // Doesn't exist
              if (next == PGNO_NULL)
                {
                  switch (pms->mode)
                    {
                    case FP_CREATE:
                      {
                        // Create the next page
                        if (pgr_new (&npg, pms->p, pms->tx,
                                     PG_VAR_PAGE, e))
                          {
                            goto failed;
                          }

                        // cur.next = npg
                        vp_set_next (page_h_w (&cur), page_h_pgno (&npg));

                        // Advance
                        {
                          // free(prev)
                          if ((pgr_release_if_exists (pms->p, &prev,
                                                      PG_VAR_PAGE, e)))
                            {
                              goto failed;
                            }

                          // prev = cur
                          prev = page_h_xfer_ownership (&cur);
                          cur = page_h_xfer_ownership (&npg);
                        }

                        // Finish
                        goto foundit;
                      }
                    case FP_FIND:
                      {
                        err_var_doesnt_exist (pms->vname, e);
                        goto failed;
                      }
                    }
                }

              // Normal Advance
              else
                {
                  // free(prev)
                  if ((pgr_release (pms->p, &prev, PG_VAR_PAGE, e)))
                    {
                      goto failed;
                    }

                  // prev = cur
                  prev = page_h_xfer_ownership (&cur);

                  // cur = cur->next
                  if ((pgr_get_maybe_writable (&cur, pms->tx, PG_VAR_PAGE, next, pms->p, writable, e)))
                    {
                      goto failed;
                    }
                }
            }
        }
    }

foundit:

  // Transfer stuff to alloc
  if (pms->dvar && pms->alloc)
    {
      pms->dvar->vname.data = chunk_alloc_move_mem (
          pms->alloc,
          pms->vname.data,
          pms->vname.len,
          e);

      if (pms->dvar->vname.data == NULL)
        {
          goto failed;
        }
      pms->dvar->vname.len = pms->vname.len;
    }

  chunk_alloc_free_all (&temp);

  // Transfer nodes to params
  if (xfer_or_release (pms->p, pms->prev, &prev, e))
    {
      goto failed;
    }
  if (xfer_or_release (pms->p, pms->cur, &cur, e))
    {
      goto failed;
    }

  return SUCCESS;

failed:
  chunk_alloc_free_all (&temp);

  pgr_cancel_if_exists (pms->p, &prev);
  pgr_cancel_if_exists (pms->p, &cur);
  pgr_cancel_if_exists (pms->p, &npg);

  return error_trace (e);
}

#ifndef NTEST
TEST (_ns_find_var_page)
{
  error e = error_create ();
  i_remove_quiet ("test", &e);
  pgr_delete_single_file ("test", &e);
  struct smfile *sf = smfile_open ("test");

  struct txn tx;
  pgr_begin_txn (&tx, sf->root->p, &sf->e);
  struct chunk_alloc alloc;
  chunk_alloc_create_default (&alloc);
  struct variable var;
  page_h prev = page_h_create ();
  page_h cur = page_h_create ();

  struct _ns_find_var_page_params params = {
    .p = sf->root->p,
    .tx = &tx,
    .alloc = &alloc,
    .vname = strfcstr ("foobar"),
    .dvar = &var,
    .mode = FP_CREATE,
    .hpos = 0,
    .prev = &prev,
    .cur = &cur,
  };

  _ns_find_var_page (&params, &sf->e);

  chunk_alloc_free_all (&alloc);

  pgr_unfix (sf->root->p, &prev, PG_PERMISSIVE);
  pgr_unfix (sf->root->p, &cur, PG_PERMISSIVE);

  pgr_commit (sf->root->p, &tx, &e);

  smfile_close (sf);
}
#endif
