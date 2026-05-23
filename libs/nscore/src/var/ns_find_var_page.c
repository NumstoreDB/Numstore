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
#include "nscore/errors.h"
#include "nscore/page_fixture.h"
#include "nscore/page_h.h"
#include "nscore/pager.h"
#include "nscore/pages/page.h"
#include "nscore/pages/var_hash_page.h"
#include "nscore/pages/var_page.h"
#include "nscore/types.h"
#include "nscore/var.h"
#include "nscore/variables.h"

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
    return error_causef (
        e,
        ERR_DUPLICATE_VARIABLE,
        "Variable: %.*s... already exists",
        7,
        vname.data
    );
  }
  else
  {
    return error_causef (
        e,
        ERR_DUPLICATE_VARIABLE,
        "Variable: %.*s already exists",
        vname.len,
        vname.data
    );
  }
}

static err_t
xfer_or_release (struct pager *p, page_h *dest, page_h *src, error *e)
{
  if (dest) { page_h_xfer_ownership_ptr (dest, src); }
  else
  {
    if (pgr_release (p, src, PG_VAR_HASH_PAGE | PG_VAR_PAGE, e)) { goto theend; }
  }

theend:
  return error_trace (e);
}

err_t
ns_find_var_page (struct ns_find_var_page_params *pms, error *e)
{
  page_h prev = page_h_create ();
  page_h cur  = page_h_create ();
  page_h npg  = page_h_create ();

  // If create mode - we read everything in X
  // this is a pretty bad way of doing it and
  // was a bug fix - it should be refactored
  bool writable = pms->mode == FP_CREATE;

  // Temporary allocator while scanning nodes
  struct chunk_alloc temp;
  chunk_alloc_create_default (&temp);

  // The hash bin of this variable
  pms->hpos = vh_get_hash_pos (pms->vname);

  // Fetch the variable hash page (put it in prev)
  if (pgr_get_maybe_writable (&prev, pms->tx, PG_VAR_HASH_PAGE, VHASH_PGNO, pms->p, writable, e))
  {
    goto failed;
  }
  // state:
  //    prev = VHP
  //    cur  = NONE

  // What does this bin hold
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
        if (pgr_new (&cur, pms->p, pms->tx, PG_VAR_PAGE, e)) { goto failed; }
        // state:
        //    prev = VHP
        //    cur  = VP

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
    // state:
    //    prev = VHP
    //    cur  = VP

    // Find the end of the list
    while (true)
    {
      // Check if this var page matches
      struct ns_read_var_page_params get_params = {
          .p          = pms->p,
          .tx         = pms->tx,
          .vp         = &cur,
          .alloc      = &temp,
          .dest       = pms->dvar,
          .matches    = false,
          .check      = &pms->vname,
          .save_type  = false,
          .save_vname = false,
      };
      if (ns_read_var_page (&get_params, e)) { goto failed; }

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
              if (pgr_new (&npg, pms->p, pms->tx, PG_VAR_PAGE, e)) { goto failed; }

              // cur.next = npg
              vp_set_next (page_h_w (&cur), page_h_pgno (&npg));

              // Advance
              {
                // free(prev)
                if ((pgr_release_if_exists (pms->p, &prev, PG_VAR_PAGE | PG_VAR_HASH_PAGE, e)))
                {
                  goto failed;
                }

                // prev = cur
                prev = page_h_xfer_ownership (&cur);
                cur  = page_h_xfer_ownership (&npg);
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
          if ((pgr_release (pms->p, &prev, PG_VAR_PAGE, e))) { goto failed; }

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

  if (pms->dvar && pms->alloc)
  {
    // Transfer variable name and type to persistent allocator
    pms->dvar->vname.data = chunk_alloc_move_mem (pms->alloc, pms->vname.data, pms->vname.len, e);
    pms->dvar->vname.len  = pms->vname.len;

    // Error check
    if (pms->dvar->vname.data == NULL) { goto failed; }
  }

  chunk_alloc_free_all (&temp);

  // Transfer nodes to params
  if (xfer_or_release (pms->p, pms->prev, &prev, e)) { goto failed; }
  if (xfer_or_release (pms->p, pms->cur, &cur, e)) { goto failed; }

  return SUCCESS;

failed:
  chunk_alloc_free_all (&temp);

  pgr_cancel_if_exists (pms->p, &prev);
  pgr_cancel_if_exists (pms->p, &cur);
  pgr_cancel_if_exists (pms->p, &npg);

  return error_trace (e);
}

#ifndef NTEST
TEST (ns_find_var_page)
{
  TEST_CASE ("create new variable page (prev, cur) = (vhp, vp)")
  {
    struct pgr_fixture f;
    pgr_fixture_create (&f);
    ns_init_var_hash_map (f.p, &f.e);
    struct variable    dvar; // The destination variable
    struct chunk_alloc temp;
    chunk_alloc_create_default (&temp);
    struct txn tx;
    page_h     cur  = page_h_create ();
    page_h     prev = page_h_create ();

    {
      pgr_begin_txn (&tx, f.p, &f.e);

      struct ns_find_var_page_params fparams = {
          .p     = f.p,
          .tx    = &tx,
          .alloc = &temp,
          .vname = strfcstr ("foo"),
          .dvar  = &dvar,
          .mode  = FP_CREATE,
          .cur   = &cur,
          .prev  = &prev,
      };

      ns_find_var_page (&fparams, &f.e);

      test_assert_int_equal (page_get_type (page_h_ro (&prev)), PG_VAR_HASH_PAGE);
      test_assert_int_equal (page_get_type (page_h_ro (&cur)), PG_VAR_PAGE);

      pgr_release (f.p, &cur, PG_PERMISSIVE, &f.e);
      pgr_release (f.p, &prev, PG_PERMISSIVE, &f.e);

      pgr_commit (f.p, &tx, &f.e);
    }

    // Clean up
    chunk_alloc_free_all (&temp);
    pgr_fixture_teardown (&f);
  }

  TEST_CASE ("create new page hash collision (prev, cur) = (vp, vp)")
  {
    struct pgr_fixture f;
    pgr_fixture_create (&f);
    ns_init_var_hash_map (f.p, &f.e);
    struct string      name1;
    struct string      name2;
    struct chunk_alloc alloc;
    chunk_alloc_create_default (&alloc);
    rand_varname_same_hash (&name1, &name2, &alloc, &f.e);
    struct variable dvar; // The destination variable
    struct txn      tx;
    page_h          cur  = page_h_create ();
    page_h          prev = page_h_create ();
    pgno            pg1;

    {
      pgr_begin_txn (&tx, f.p, &f.e);

      struct ns_find_var_page_params fparams = {
          .p     = f.p,
          .tx    = &tx,
          .alloc = &alloc,
          .vname = name1,
          .dvar  = &dvar,
          .mode  = FP_CREATE,
          .cur   = &cur,
          .prev  = &prev,
      };

      ns_find_var_page (&fparams, &f.e);

      test_assert_int_equal (page_get_type (page_h_ro (&prev)), PG_VAR_HASH_PAGE);
      test_assert_int_equal (page_get_type (page_h_ro (&cur)), PG_VAR_PAGE);

      pg1 = page_h_pgno (&cur);

      pgr_release (f.p, &cur, PG_PERMISSIVE, &f.e);
      pgr_release (f.p, &prev, PG_PERMISSIVE, &f.e);

      pgr_commit (f.p, &tx, &f.e);
    }

    // Create a colliding variable
    {
      pgr_begin_txn (&tx, f.p, &f.e);

      struct ns_find_var_page_params fparams = (struct ns_find_var_page_params){
          .p     = f.p,
          .tx    = &tx,
          .alloc = &alloc,
          .vname = name1,
          .dvar  = &dvar,
          .mode  = FP_CREATE,
          .cur   = &cur,
          .prev  = &prev,
      };
      ns_find_var_page (&fparams, &f.e);

      // This time, prev is the previous page
      test_assert_int_equal (page_get_type (page_h_ro (&prev)), PG_VAR_PAGE);
      test_assert_int_equal (page_get_type (page_h_ro (&cur)), PG_VAR_PAGE);
      test_assert_int_equal (pg1, page_h_pgno (&prev));

      pgr_release (f.p, &cur, PG_PERMISSIVE, &f.e);
      pgr_release (f.p, &prev, PG_PERMISSIVE, &f.e);

      pgr_commit (f.p, &tx, &f.e);
    }

    // Clean up
    chunk_alloc_free_all (&alloc);
    pgr_fixture_teardown (&f);
  }

  TEST_CASE ("Create same variable throws")
  {
    struct pgr_fixture f;
    pgr_fixture_create (&f);
    ns_init_var_hash_map (f.p, &f.e);
    struct chunk_alloc alloc;
    chunk_alloc_create_default (&alloc);
    struct variable dvar; // The destination variable
    struct txn      tx;
    page_h          cur  = page_h_create ();
    page_h          prev = page_h_create ();
    pgno            pg1;

    // Create a variable
    {
      pgr_begin_txn (&tx, f.p, &f.e);
      struct ns_var_get_or_create_params cparams = {
          .p     = f.p,
          .tx    = &tx,
          .vname = strfcstr ("foo"),
          .type  = &(struct type){.type = T_PRIM, .p = U32},
          .alloc = &alloc,
      };
      ns_var_get_or_create (&cparams, &f.e);
      pg1 = cparams.dest.var_root;
      pgr_commit (f.p, &tx, &f.e);
    }

    // Create the same variable
    {
      pgr_begin_txn (&tx, f.p, &f.e);

      struct ns_find_var_page_params fparams = (struct ns_find_var_page_params){
          .p     = f.p,
          .tx    = &tx,
          .alloc = &alloc,
          .vname = strfcstr ("foo"),
          .dvar  = &dvar,
          .mode  = FP_CREATE,
          .cur   = &cur,
          .prev  = &prev,
      };
      test_err_t_check (ns_find_var_page (&fparams, &f.e), ERR_DUPLICATE_VARIABLE, &f.e);

      pgr_commit (f.p, &tx, &f.e);
    }

    // Clean up
    chunk_alloc_free_all (&alloc);
    pgr_fixture_teardown (&f);
  }

  TEST_CASE ("Read existing variable no chain")
  {
    struct pgr_fixture f;
    pgr_fixture_create (&f);
    ns_init_var_hash_map (f.p, &f.e);
    struct chunk_alloc alloc;
    chunk_alloc_create_default (&alloc);
    struct variable dvar; // The destination variable
    struct txn      tx;
    page_h          cur  = page_h_create ();
    page_h          prev = page_h_create ();
    pgno            pg1;
    pgno            pg2;

    // Create a variable
    {
      pgr_begin_txn (&tx, f.p, &f.e);
      struct ns_var_get_or_create_params cparams = {
          .p     = f.p,
          .tx    = &tx,
          .vname = strfcstr ("foo"),
          .type  = &(struct type){.type = T_PRIM, .p = U32},
          .alloc = &alloc,
      };
      ns_var_get_or_create (&cparams, &f.e);
      pg1 = cparams.dest.var_root;
      pgr_commit (f.p, &tx, &f.e);
    }

    // Find the same variable
    {
      pgr_begin_txn (&tx, f.p, &f.e);

      struct ns_find_var_page_params fparams = {
          .p     = f.p,
          .tx    = &tx,
          .alloc = &alloc,
          .vname = strfcstr ("foo"),
          .dvar  = &dvar,
          .mode  = FP_FIND,
          .cur   = &cur,
          .prev  = &prev,
      };
      ns_find_var_page (&fparams, &f.e);

      test_assert_int_equal (page_get_type (page_h_ro (&cur)), PG_VAR_PAGE);
      test_assert_int_equal (page_get_type (page_h_ro (&prev)), PG_VAR_HASH_PAGE);
      test_assert_int_equal (pg1, page_h_pgno (&cur));

      pgr_release (f.p, &cur, PG_PERMISSIVE, &f.e);
      pgr_release (f.p, &prev, PG_PERMISSIVE, &f.e);

      pgr_commit (f.p, &tx, &f.e);
    }

    // Clean up
    chunk_alloc_free_all (&alloc);
    pgr_fixture_teardown (&f);
  }

  TEST_CASE ("Find existing page hash collision (prev, cur) = (vp, vp)")
  {
    struct pgr_fixture f;
    pgr_fixture_create (&f);
    ns_init_var_hash_map (f.p, &f.e);
    struct string      name1;
    struct string      name2;
    struct chunk_alloc alloc;
    chunk_alloc_create_default (&alloc);
    rand_varname_same_hash (&name1, &name2, &alloc, &f.e);
    struct variable dvar; // The destination variable
    struct txn      tx;
    page_h          cur  = page_h_create ();
    page_h          prev = page_h_create ();
    pgno            pg1;
    pgno            pg2;

    // Create a variable
    {
      pgr_begin_txn (&tx, f.p, &f.e);
      struct ns_var_get_or_create_params cparams = {
          .p     = f.p,
          .tx    = &tx,
          .vname = name1,
          .type  = &(struct type){.type = T_PRIM, .p = U32},
          .alloc = &alloc,
      };
      ns_var_get_or_create (&cparams, &f.e);
      pg1 = cparams.dest.var_root;
      pgr_commit (f.p, &tx, &f.e);
    }

    // Create a variable
    {
      pgr_begin_txn (&tx, f.p, &f.e);
      struct ns_var_get_or_create_params cparams = {
          .p     = f.p,
          .tx    = &tx,
          .vname = name2,
          .type  = &(struct type){.type = T_PRIM, .p = U32},
          .alloc = &alloc,
      };
      ns_var_get_or_create (&cparams, &f.e);
      pg2 = cparams.dest.var_root;
      pgr_commit (f.p, &tx, &f.e);
    }

    // Find the first variable
    {
      pgr_begin_txn (&tx, f.p, &f.e);

      struct ns_find_var_page_params fparams = {
          .p     = f.p,
          .tx    = &tx,
          .alloc = &alloc,
          .vname = name1,
          .dvar  = &dvar,
          .mode  = FP_FIND,
          .cur   = &cur,
          .prev  = &prev,
      };
      ns_find_var_page (&fparams, &f.e);
      test_assert_int_equal (page_get_type (page_h_ro (&prev)), PG_VAR_HASH_PAGE);
      test_assert_int_equal (page_get_type (page_h_ro (&cur)), PG_VAR_PAGE);
      test_assert_int_equal (pg1, page_h_pgno (&cur));

      pgr_release (f.p, &cur, PG_PERMISSIVE, &f.e);
      pgr_release (f.p, &prev, PG_PERMISSIVE, &f.e);

      pgr_commit (f.p, &tx, &f.e);
    }

    // Find the second variable
    {
      pgr_begin_txn (&tx, f.p, &f.e);

      struct ns_find_var_page_params fparams = {
          .p     = f.p,
          .tx    = &tx,
          .alloc = &alloc,
          .vname = name2,
          .dvar  = &dvar,
          .mode  = FP_FIND,
          .cur   = &cur,
          .prev  = &prev,
      };
      ns_find_var_page (&fparams, &f.e);
      test_assert_int_equal (page_get_type (page_h_ro (&prev)), PG_VAR_PAGE);
      test_assert_int_equal (page_get_type (page_h_ro (&cur)), PG_VAR_PAGE);

      test_assert_int_equal (page_h_pgno (&prev), pg1);
      test_assert_int_equal (page_h_pgno (&cur), pg2);

      pgr_release (f.p, &cur, PG_PERMISSIVE, &f.e);
      pgr_release (f.p, &prev, PG_PERMISSIVE, &f.e);

      pgr_commit (f.p, &tx, &f.e);
    }

    // Clean up
    chunk_alloc_free_all (&alloc);
    pgr_fixture_teardown (&f);
  }

  TEST_CASE ("Find existing page no hash collision (prev, cur) = (vp, vp)")
  {
    struct pgr_fixture f;
    pgr_fixture_create (&f);
    ns_init_var_hash_map (f.p, &f.e);
    struct string      name1;
    struct string      name2;
    struct chunk_alloc alloc;
    chunk_alloc_create_default (&alloc);
    rand_varname_different_hash (&name1, &name2, &alloc, &f.e);
    struct variable dvar; // The destination variable
    struct txn      tx;
    page_h          cur  = page_h_create ();
    page_h          prev = page_h_create ();
    pgno            pg1;
    pgno            pg2;

    {
      pgr_begin_txn (&tx, f.p, &f.e);
      struct ns_var_get_or_create_params cparams = {
          .p     = f.p,
          .tx    = &tx,
          .vname = name1,
          .type  = &(struct type){.type = T_PRIM, .p = U32},
          .alloc = &alloc,
      };
      ns_var_get_or_create (&cparams, &f.e);
      pg1 = cparams.dest.var_root;
      pgr_commit (f.p, &tx, &f.e);
    }

    {
      pgr_begin_txn (&tx, f.p, &f.e);
      struct ns_var_get_or_create_params cparams = {
          .p     = f.p,
          .tx    = &tx,
          .vname = name2,
          .type  = &(struct type){.type = T_PRIM, .p = U32},
          .alloc = &alloc,
      };
      ns_var_get_or_create (&cparams, &f.e);
      pg2 = cparams.dest.var_root;
      pgr_commit (f.p, &tx, &f.e);
    }

    // Find the first variable
    {
      pgr_begin_txn (&tx, f.p, &f.e);

      struct ns_find_var_page_params fparams = {
          .p     = f.p,
          .tx    = &tx,
          .alloc = &alloc,
          .vname = name1,
          .dvar  = &dvar,
          .mode  = FP_FIND,
          .cur   = &cur,
          .prev  = &prev,
      };
      ns_find_var_page (&fparams, &f.e);
      test_assert_int_equal (page_get_type (page_h_ro (&cur)), PG_VAR_PAGE);
      test_assert_int_equal (page_get_type (page_h_ro (&prev)), PG_VAR_HASH_PAGE);

      test_assert_int_equal (page_h_pgno (&cur), pg1);

      pgr_release (f.p, &cur, PG_PERMISSIVE, &f.e);
      pgr_release (f.p, &prev, PG_PERMISSIVE, &f.e);

      pgr_commit (f.p, &tx, &f.e);
    }

    // Find the second variable
    {
      pgr_begin_txn (&tx, f.p, &f.e);

      struct ns_find_var_page_params fparams = {
          .p     = f.p,
          .tx    = &tx,
          .alloc = &alloc,
          .vname = name2,
          .dvar  = &dvar,
          .mode  = FP_FIND,
          .cur   = &cur,
          .prev  = &prev,
      };
      ns_find_var_page (&fparams, &f.e);
      test_assert_int_equal (page_get_type (page_h_ro (&cur)), PG_VAR_PAGE);
      test_assert_int_equal (page_get_type (page_h_ro (&prev)), PG_VAR_HASH_PAGE);

      test_assert_int_equal (page_h_pgno (&cur), pg2);

      pgr_release (f.p, &cur, PG_PERMISSIVE, &f.e);
      pgr_release (f.p, &prev, PG_PERMISSIVE, &f.e);

      pgr_commit (f.p, &tx, &f.e);
    }

    // Clean up
    chunk_alloc_free_all (&alloc);
    pgr_fixture_teardown (&f);
  }
}
#endif
