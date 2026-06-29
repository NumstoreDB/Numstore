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

#include "var_algorithms.h"

#include "alloc.h"
#include "error.h"
#include "numstore.h"
#include "page.h"
#include "page_h.h"
#include "pager.h"
#include "rope_algorithms.h"
#include "types.h"
#include "variables.h"

#ifdef TESTING
#  include "testing/page_fixture.h"
#  include "testing/testing.h"
#endif

/******************************************************************************
 * SECTION: ns_init_var_hash_map
 ******************************************************************************/

err_t
ns_init_var_hash_map (struct pager *p, error *e)
{
  page_h hp = page_h_create ();

  // BEGIN TXN
  struct txn tx;
  if (pgr_begin_txn (&tx, p, e))
  {
    goto failed;
  }

  // Upfront initialization
  if (pgr_isnew (p))
  {
    // Create a new variable hash page
    if (pgr_new (&hp, p, &tx, PG_VAR_HASH_PAGE, e))
    {
      goto failed;
    }

    // Next page should be valid
    //   this is a weak contract
    //   but assumes the structure of the pager,
    //   it's good enough but might need to change
    ASSERT (page_h_pgno (&hp) == VHASH_PGNO);

    if (pgr_release (p, &hp, PG_VAR_HASH_PAGE, e))
    {
      goto failed;
    }
  }

  // COMMIT
  if (pgr_commit (p, &tx, e))
  {
    goto failed;
  }

failed:
  return error_trace (e);
}

#ifdef TESTING
TEST (ns_init_var_hash_map)
{
  struct pgr_fixture f;
  pgr_fixture_create (&f);
  ns_init_var_hash_map (f.p, &f.e);

  page_h vhp = page_h_create ();
  test_assert_int_equal (
      pgr_get (&vhp, PG_VAR_HASH_PAGE, 1, f.p, &f.e),
      SUCCESS
  );

  pgr_release (f.p, &vhp, PG_VAR_HASH_PAGE, &f.e);
  pgr_fixture_teardown (&f);
}
#endif

/******************************************************************************
 * SECTION: ns_find_var_page
 ******************************************************************************/
/**
 * Simply finds or creates the new page where vname should exist.
 * If this is a create operation - then the page is found and
 * the neighbor links are written, but the page itself is not completed
 * thats where you need to call ns_write_var_page
 */

/**
 * @brief Generates an error with correct format string for variable doesn't
 * exist
 *
 * @param vname The name of the variable
 * @param e Error handle
 * @return error code
 */
static err_t
err_var_doesnt_exist (const struct string vname, error *e)
{
  if (vname.len > 10)
  {
    return error_causef (
        e,
        ERR_VARIABLE_NE,
        "Variable: %.*s... doesn't exist",
        7,
        vname.data
    );
  }
  else
  {
    return error_causef (
        e,
        ERR_VARIABLE_NE,
        "Variable: %.*s doesn't exist",
        vname.len,
        vname.data
    );
  }
}

/**
 * @brief Generates an error with correct format string for variable already
 * exists
 *
 * @param vname The name of the variable
 * @param e Error handle
 * @return error code
 */
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

struct ns_find_var_page_params
{
  struct pager     *p;
  struct txn       *tx;
  struct allocator *alloc;

  struct string    vname;
  struct variable *dvar;
  enum
  {
    FP_CREATE,
    FP_FIND,
  } mode;

  // You don't need to set these
  pgno    hpos;
  page_h *prev;
  page_h *cur;
};

static err_t
ns_find_var_page (struct ns_find_var_page_params *pms, error *e)
{
  page_h prev = page_h_create ();
  page_h cur  = page_h_create ();
  page_h npg  = page_h_create ();

  // If create mode - we need to read everything in X
  bool writable = pms->mode == FP_CREATE;

  // Temporary allocator while scanning nodes
  ALLOC_INIT (temp);

  // The hash bin of this variable
  pms->hpos = vh_get_hash_pos (pms->vname);

  // Fetch the variable hash page (put it in prev)
  if (FAULT (
          pgr_get_maybe_writable (
              &prev,
              pms->tx,
              PG_VAR_HASH_PAGE,
              VHASH_PGNO,
              pms->p,
              writable,
              e
          ),
          "ns_find_var_page:1"
      ))
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
        if (FAULT (
                pgr_new (&cur, pms->p, pms->tx, PG_VAR_PAGE, e),
                "ns_find_var_page:2"
            ))
        {
          goto failed;
        }
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
    if (FAULT (
            pgr_get_maybe_writable (
                &cur,
                pms->tx,
                PG_VAR_PAGE,
                head,
                pms->p,
                writable,
                e
            ),
            "ns_find_var_page:3"
        ))
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
      if (FAULT (ns_read_var_page (&get_params, e), "ns_find_var_page:4"))
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
        ALLOC_RESET (temp);

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
              if (pgr_new (&npg, pms->p, pms->tx, PG_VAR_PAGE, e))
              {
                goto failed;
              }

              // cur.next = npg
              vp_set_next (page_h_w (&cur), page_h_pgno (&npg));

              // Advance
              {
                // free(prev)
                if ((pgr_release_if_exists (
                        pms->p,
                        &prev,
                        PG_VAR_PAGE | PG_VAR_HASH_PAGE,
                        e
                    )))
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
          if ((pgr_release (pms->p, &prev, PG_VAR_HASH_PAGE | PG_VAR_PAGE, e)))
          {
            goto failed;
          }

          // prev = cur
          prev = page_h_xfer_ownership (&cur);

          // cur = cur->next
          if ((pgr_get_maybe_writable (
                  &cur,
                  pms->tx,
                  PG_VAR_PAGE,
                  next,
                  pms->p,
                  writable,
                  e
              )))
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
    pms->dvar->vname.data =
        allocator_copy (pms->alloc, pms->vname.data, pms->vname.len, e);
    pms->dvar->vname.len = pms->vname.len;

    // Error check
    if (pms->dvar->vname.data == NULL)
    {
      goto failed;
    }
  }

  // Transfer nodes to params
  if (page_h_type (&prev) == PG_VAR_TAIL)
  {
    panic ("HERE");
  }
  if (xfer_or_release (pms->p, pms->prev, &prev, e))
  {
    goto failed;
  }
  if (xfer_or_release (pms->p, pms->cur, &cur, e))
  {
    goto failed;
  }

  ALLOC_CLOSE (temp);

  return SUCCESS;

failed:
  ALLOC_CLOSE (temp);

  pgr_cancel_if_exists (pms->p, &prev);
  pgr_cancel_if_exists (pms->p, &cur);
  pgr_cancel_if_exists (pms->p, &npg);

  return error_trace (e);
}

#ifdef TESTING
TEST (ns_find_var_page)
{
  struct pgr_fixture f;
  struct variable    dvar;
  page_h             cur  = page_h_create ();
  page_h             prev = page_h_create ();

  TEST_CASE ("Fault 1")
  {
    pgr_fixture_create_with_var_hash_map (&f);

    {
      pgr_begin_txn (&f.tx, f.p, &f.e);

      struct ns_find_var_page_params fparams = {
          .p     = f.p,
          .tx    = &f.tx,
          .alloc = &f.alloc,
          .vname = strfcstr ("foo"),
          .dvar  = &dvar,
          .mode  = FP_CREATE, // can be either in this test
          .cur   = &cur,
          .prev  = &prev,
      };

      fault_reset_all ();
      fault_set ("ns_find_var_page:1");

      ns_find_var_page (&fparams, &f.e);
      f.e.cause_code = 0;
      f.e.cmlen      = 0;

      test_assert_int_equal (prev.mode, PHM_NONE);
      test_assert_int_equal (cur.mode, PHM_NONE);

      pgr_commit (f.p, &f.tx, &f.e);
    }

    pgr_fixture_teardown (&f);
    fault_reset_all ();
  }

  TEST_CASE ("Fault 2")
  {
    pgr_fixture_create_with_var_hash_map (&f);

    {
      pgr_begin_txn (&f.tx, f.p, &f.e);

      struct ns_find_var_page_params fparams = {
          .p     = f.p,
          .tx    = &f.tx,
          .alloc = &f.alloc,
          .vname = strfcstr ("foo"),
          .dvar  = &dvar,
          .mode  = FP_CREATE,
          .cur   = &cur,
          .prev  = &prev,
      };

      fault_set ("ns_find_var_page:2");

      ns_find_var_page (&fparams, &f.e);
      f.e.cause_code = 0;
      f.e.cmlen      = 0;

      test_assert_int_equal (prev.mode, PHM_NONE);
      test_assert_int_equal (cur.mode, PHM_NONE);

      pgr_commit (f.p, &f.tx, &f.e);
    }

    pgr_fixture_teardown (&f);
    fault_reset_all ();
  }

  TEST_CASE ("Fault 3")
  {
    pgr_fixture_create_with_var_hash_map (&f);

    pgno pg1;

    // Create a variable
    {
      pgr_begin_txn (&f.tx, f.p, &f.e);
      struct ns_var_get_or_create_params cparams = {
          .p     = f.p,
          .tx    = &f.tx,
          .vname = strfcstr ("foo"),
          .type  = &(struct type){.type = T_PRIM, .p = U32},
          .alloc = &f.alloc,
      };
      ns_var_get_or_create (&cparams, &f.e);
      pgr_commit (f.p, &f.tx, &f.e);
    }

    // Get the same variable
    {
      pgr_begin_txn (&f.tx, f.p, &f.e);

      struct ns_find_var_page_params fparams = (struct ns_find_var_page_params){
          .p     = f.p,
          .tx    = &f.tx,
          .alloc = &f.alloc,
          .vname = strfcstr ("foo"),
          .dvar  = &dvar,
          .mode  = FP_FIND,
          .cur   = &cur,
          .prev  = &prev,
      };

      fault_reset_all ();
      fault_set ("ns_find_var_page:3");

      test_assert (ns_find_var_page (&fparams, &f.e) != SUCCESS);

      f.e.cause_code = 0;
      f.e.cmlen      = 0;

      test_assert_int_equal (prev.mode, PHM_NONE);
      test_assert_int_equal (cur.mode, PHM_NONE);

      pgr_commit (f.p, &f.tx, &f.e);
    }

    pgr_fixture_teardown (&f);
    fault_reset_all ();
  }

  TEST_CASE ("Fault 4")
  {
    pgr_fixture_create_with_var_hash_map (&f);
    pgno pg1;

    // Create a variable
    {
      pgr_begin_txn (&f.tx, f.p, &f.e);
      struct ns_var_get_or_create_params cparams = {
          .p     = f.p,
          .tx    = &f.tx,
          .vname = strfcstr ("foo"),
          .type  = &(struct type){.type = T_PRIM, .p = U32},
          .alloc = &f.alloc,
      };
      ns_var_get_or_create (&cparams, &f.e);
      pg1 = cparams.dest.var_root;
      pgr_commit (f.p, &f.tx, &f.e);
    }

    // Get the same variable
    {
      pgr_begin_txn (&f.tx, f.p, &f.e);

      struct ns_find_var_page_params fparams = (struct ns_find_var_page_params){
          .p     = f.p,
          .tx    = &f.tx,
          .alloc = &f.alloc,
          .vname = strfcstr ("foo"),
          .dvar  = &dvar,
          .mode  = FP_FIND,
          .cur   = &cur,
          .prev  = &prev,
      };

      fault_set ("ns_find_var_page:4");

      ns_find_var_page (&fparams, &f.e);
      f.e.cause_code = 0;
      f.e.cmlen      = 0;

      test_assert_int_equal (prev.mode, PHM_NONE);
      test_assert_int_equal (cur.mode, PHM_NONE);

      pgr_commit (f.p, &f.tx, &f.e);
    }

    pgr_fixture_teardown (&f);
    fault_reset_all ();
  }

  TEST_CASE ("create new variable page (prev, cur) = (vhp, vp)")
  {
    pgr_fixture_create_with_var_hash_map (&f);

    {
      pgr_begin_txn (&f.tx, f.p, &f.e);

      struct ns_find_var_page_params fparams = {
          .p     = f.p,
          .tx    = &f.tx,
          .alloc = &f.alloc,
          .vname = strfcstr ("foo"),
          .dvar  = &dvar,
          .mode  = FP_CREATE,
          .cur   = &cur,
          .prev  = &prev,
      };

      ns_find_var_page (&fparams, &f.e);

      test_assert_int_equal (
          page_get_type (page_h_ro (&prev)),
          PG_VAR_HASH_PAGE
      );
      test_assert_int_equal (page_get_type (page_h_ro (&cur)), PG_VAR_PAGE);

      pgr_release (f.p, &cur, PG_PERMISSIVE, &f.e);
      pgr_release (f.p, &prev, PG_PERMISSIVE, &f.e);

      pgr_commit (f.p, &f.tx, &f.e);
    }

    pgr_fixture_teardown (&f);
  }

  TEST_CASE ("create new page hash collision (prev, cur) = (vp, vp)")
  {
    pgr_fixture_create_with_var_hash_map (&f);
    struct string name1;
    struct string name2;
    rand_varname_same_hash (&name1, &name2, &f.alloc, &f.e);
    pgno pg1;

    {
      pgr_begin_txn (&f.tx, f.p, &f.e);

      struct ns_find_var_page_params fparams = {
          .p     = f.p,
          .tx    = &f.tx,
          .alloc = &f.alloc,
          .vname = name1,
          .dvar  = &dvar,
          .mode  = FP_CREATE,
          .cur   = &cur,
          .prev  = &prev,
      };

      ns_find_var_page (&fparams, &f.e);

      test_assert_int_equal (
          page_get_type (page_h_ro (&prev)),
          PG_VAR_HASH_PAGE
      );
      test_assert_int_equal (page_get_type (page_h_ro (&cur)), PG_VAR_PAGE);

      pg1 = page_h_pgno (&cur);

      pgr_release (f.p, &cur, PG_PERMISSIVE, &f.e);
      pgr_release (f.p, &prev, PG_PERMISSIVE, &f.e);

      pgr_commit (f.p, &f.tx, &f.e);
    }

    // Create a colliding variable
    {
      pgr_begin_txn (&f.tx, f.p, &f.e);

      struct ns_find_var_page_params fparams = (struct ns_find_var_page_params){
          .p     = f.p,
          .tx    = &f.tx,
          .alloc = &f.alloc,
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

      pgr_commit (f.p, &f.tx, &f.e);
    }

    pgr_fixture_teardown (&f);
  }

  TEST_CASE ("Create same variable throws")
  {
    pgr_fixture_create_with_var_hash_map (&f);
    pgno pg1;

    // Create a variable
    {
      pgr_begin_txn (&f.tx, f.p, &f.e);
      struct ns_var_get_or_create_params cparams = {
          .p     = f.p,
          .tx    = &f.tx,
          .vname = strfcstr ("foo"),
          .type  = &(struct type){.type = T_PRIM, .p = U32},
          .alloc = &f.alloc,
      };
      ns_var_get_or_create (&cparams, &f.e);
      pg1 = cparams.dest.var_root;
      pgr_commit (f.p, &f.tx, &f.e);
    }

    // Create the same variable
    {
      pgr_begin_txn (&f.tx, f.p, &f.e);

      struct ns_find_var_page_params fparams = (struct ns_find_var_page_params){
          .p     = f.p,
          .tx    = &f.tx,
          .alloc = &f.alloc,
          .vname = strfcstr ("foo"),
          .dvar  = &dvar,
          .mode  = FP_CREATE,
          .cur   = &cur,
          .prev  = &prev,
      };
      test_err_t_check (
          ns_find_var_page (&fparams, &f.e),
          ERR_DUPLICATE_VARIABLE,
          &f.e
      );

      pgr_commit (f.p, &f.tx, &f.e);
    }

    pgr_fixture_teardown (&f);
  }

  TEST_CASE ("Read existing variable no chain")
  {
    pgr_fixture_create_with_var_hash_map (&f);
    pgno pg1;

    // Create a variable
    {
      pgr_begin_txn (&f.tx, f.p, &f.e);
      struct ns_var_get_or_create_params cparams = {
          .p     = f.p,
          .tx    = &f.tx,
          .vname = strfcstr ("foo"),
          .type  = &(struct type){.type = T_PRIM, .p = U32},
          .alloc = &f.alloc,
      };
      ns_var_get_or_create (&cparams, &f.e);
      pg1 = cparams.dest.var_root;
      pgr_commit (f.p, &f.tx, &f.e);
    }

    // Find the same variable
    {
      pgr_begin_txn (&f.tx, f.p, &f.e);

      struct ns_find_var_page_params fparams = {
          .p     = f.p,
          .tx    = &f.tx,
          .alloc = &f.alloc,
          .vname = strfcstr ("foo"),
          .dvar  = &dvar,
          .mode  = FP_FIND,
          .cur   = &cur,
          .prev  = &prev,
      };
      ns_find_var_page (&fparams, &f.e);

      test_assert_int_equal (page_get_type (page_h_ro (&cur)), PG_VAR_PAGE);
      test_assert_int_equal (
          page_get_type (page_h_ro (&prev)),
          PG_VAR_HASH_PAGE
      );
      test_assert_int_equal (pg1, page_h_pgno (&cur));

      pgr_release (f.p, &cur, PG_PERMISSIVE, &f.e);
      pgr_release (f.p, &prev, PG_PERMISSIVE, &f.e);

      pgr_commit (f.p, &f.tx, &f.e);
    }

    pgr_fixture_teardown (&f);
  }

  TEST_CASE ("Find existing page hash collision (prev, cur) = (vp, vp)")
  {
    pgr_fixture_create_with_var_hash_map (&f);
    struct string name1;
    struct string name2;
    rand_varname_same_hash (&name1, &name2, &f.alloc, &f.e);
    pgno pg1;
    pgno pg2;

    // Create a variable
    {
      pgr_begin_txn (&f.tx, f.p, &f.e);
      struct ns_var_get_or_create_params cparams = {
          .p     = f.p,
          .tx    = &f.tx,
          .vname = name1,
          .type  = &(struct type){.type = T_PRIM, .p = U32},
          .alloc = &f.alloc,
      };
      ns_var_get_or_create (&cparams, &f.e);
      pg1 = cparams.dest.var_root;
      pgr_commit (f.p, &f.tx, &f.e);
    }

    // Create a variable
    {
      pgr_begin_txn (&f.tx, f.p, &f.e);
      struct ns_var_get_or_create_params cparams = {
          .p     = f.p,
          .tx    = &f.tx,
          .vname = name2,
          .type  = &(struct type){.type = T_PRIM, .p = U32},
          .alloc = &f.alloc,
      };
      ns_var_get_or_create (&cparams, &f.e);
      pg2 = cparams.dest.var_root;
      pgr_commit (f.p, &f.tx, &f.e);
    }

    // Find the first variable
    {
      pgr_begin_txn (&f.tx, f.p, &f.e);

      struct ns_find_var_page_params fparams = {
          .p     = f.p,
          .tx    = &f.tx,
          .alloc = &f.alloc,
          .vname = name1,
          .dvar  = &dvar,
          .mode  = FP_FIND,
          .cur   = &cur,
          .prev  = &prev,
      };
      ns_find_var_page (&fparams, &f.e);
      test_assert_int_equal (
          page_get_type (page_h_ro (&prev)),
          PG_VAR_HASH_PAGE
      );
      test_assert_int_equal (page_get_type (page_h_ro (&cur)), PG_VAR_PAGE);
      test_assert_int_equal (pg1, page_h_pgno (&cur));

      pgr_release (f.p, &cur, PG_PERMISSIVE, &f.e);
      pgr_release (f.p, &prev, PG_PERMISSIVE, &f.e);

      pgr_commit (f.p, &f.tx, &f.e);
    }

    // Find the second variable
    {
      pgr_begin_txn (&f.tx, f.p, &f.e);

      struct ns_find_var_page_params fparams = {
          .p     = f.p,
          .tx    = &f.tx,
          .alloc = &f.alloc,
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

      pgr_commit (f.p, &f.tx, &f.e);
    }

    pgr_fixture_teardown (&f);
  }

  TEST_CASE ("Find existing page no hash collision (prev, cur) = (vp, vp)")
  {
    pgr_fixture_create_with_var_hash_map (&f);
    struct string name1;
    struct string name2;
    rand_varname_different_hash (&name1, &name2, &f.alloc, &f.e);
    pgno pg1;
    pgno pg2;

    {
      pgr_begin_txn (&f.tx, f.p, &f.e);
      struct ns_var_get_or_create_params cparams = {
          .p     = f.p,
          .tx    = &f.tx,
          .vname = name1,
          .type  = &(struct type){.type = T_PRIM, .p = U32},
          .alloc = &f.alloc,
      };
      ns_var_get_or_create (&cparams, &f.e);
      pg1 = cparams.dest.var_root;
      pgr_commit (f.p, &f.tx, &f.e);
    }

    {
      pgr_begin_txn (&f.tx, f.p, &f.e);
      struct ns_var_get_or_create_params cparams = {
          .p     = f.p,
          .tx    = &f.tx,
          .vname = name2,
          .type  = &(struct type){.type = T_PRIM, .p = U32},
          .alloc = &f.alloc,
      };
      ns_var_get_or_create (&cparams, &f.e);
      pg2 = cparams.dest.var_root;
      pgr_commit (f.p, &f.tx, &f.e);
    }

    // Find the first variable
    {
      pgr_begin_txn (&f.tx, f.p, &f.e);

      struct ns_find_var_page_params fparams = {
          .p     = f.p,
          .tx    = &f.tx,
          .alloc = &f.alloc,
          .vname = name1,
          .dvar  = &dvar,
          .mode  = FP_FIND,
          .cur   = &cur,
          .prev  = &prev,
      };
      ns_find_var_page (&fparams, &f.e);
      test_assert_int_equal (page_get_type (page_h_ro (&cur)), PG_VAR_PAGE);
      test_assert_int_equal (
          page_get_type (page_h_ro (&prev)),
          PG_VAR_HASH_PAGE
      );

      test_assert_int_equal (page_h_pgno (&cur), pg1);

      pgr_release (f.p, &cur, PG_PERMISSIVE, &f.e);
      pgr_release (f.p, &prev, PG_PERMISSIVE, &f.e);

      pgr_commit (f.p, &f.tx, &f.e);
    }

    // Find the second variable
    {
      pgr_begin_txn (&f.tx, f.p, &f.e);

      struct ns_find_var_page_params fparams = {
          .p     = f.p,
          .tx    = &f.tx,
          .alloc = &f.alloc,
          .vname = name2,
          .dvar  = &dvar,
          .mode  = FP_FIND,
          .cur   = &cur,
          .prev  = &prev,
      };
      ns_find_var_page (&fparams, &f.e);
      test_assert_int_equal (page_get_type (page_h_ro (&cur)), PG_VAR_PAGE);
      test_assert_int_equal (
          page_get_type (page_h_ro (&prev)),
          PG_VAR_HASH_PAGE
      );

      test_assert_int_equal (page_h_pgno (&cur), pg2);

      pgr_release (f.p, &cur, PG_PERMISSIVE, &f.e);
      pgr_release (f.p, &prev, PG_PERMISSIVE, &f.e);

      pgr_commit (f.p, &f.tx, &f.e);
    }

    pgr_fixture_teardown (&f);
  }
}
#endif

/******************************************************************************
 * SECTION: ns_read_var_page
 * ----------------------------------------------------------------------------
 * @brief
 *
 *
 ******************************************************************************/

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

  if ((pgr_release (params->p, params->vp, PG_VAR_PAGE | PG_VAR_TAIL, e)))
  {
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
  ALLOC_INIT (temp);

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
    vstr = allocate (params->alloc, 1, vlen, e);
    if (vstr == NULL)
    {
      goto failed;
    }
  }
  else
  {
    vstr = allocate (&temp, 1, vlen, e);
    if (vstr == NULL)
    {
      goto failed;
    }
  }

  if (params->save_type)
  {
    tstr = allocate (&temp, 1, tlen, e);
    if (tstr == NULL)
    {
      goto failed;
    }
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
      if (ns_read_var_page_advance (params, e))
      {
        goto failed;
      }

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
        if (ns_read_var_page_advance (params, e))
        {
          goto failed;
        }
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
    if (dtype == NULL)
    {
      goto failed;
    }
    params->dest->dtype = dtype;
  }

  if (params->save_vname)
  {
    params->dest->vname = (struct string){.data = NULL, .len = 0};
  }

  params->matches = true;

theend:
  // Reset back to head page
  if (page_h_pgno (params->vp) != start)
  {
    if ((pgr_release (params->p, params->vp, PG_VAR_TAIL, e)))
    {
      goto failed;
    }
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

  ALLOC_CLOSE (temp);
  return SUCCESS;

failed:
  ALLOC_CLOSE (temp);
  return error_trace (e);
}

/******************************************************************************
 * SECTION: ns_var_create
 * ----------------------------------------------------------------------------
 * @brief
 *
 *
 ******************************************************************************/

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
ns_var_create (const struct ns_var_create_params params, error *e)
{
  page_h cur = page_h_create ();

  struct ns_find_var_page_params fparams = {
      .tx    = params.tx,
      .p     = params.p,
      .alloc = NULL,

      .vname = params.vname,
      .dvar  = NULL,
      .mode  = FP_CREATE,

      .hpos = PGNO_NULL,
      .prev = NULL,
      .cur  = &cur,
  };

  if (ns_find_var_page (&fparams, e))
  {
    goto failed;
  }

  struct variable var = {
      .vname    = params.vname,
      .dtype    = params.type,
      .nbytes   = 0,
      .rpt_root = PGNO_NULL,
  };

  struct ns_write_var_page_params write_params = {
      .p  = params.p,
      .tx = params.tx,

      .vp  = &cur,
      .var = &var,
  };

  if (ns_write_var_page (&write_params, e))
  {
    goto failed;
  }

  if ((pgr_release (params.p, &cur, PG_VAR_PAGE, e)))
  {
    goto failed;
  }

  return SUCCESS;

failed:

  pgr_cancel_if_exists (params.p, &cur);

  return error_trace (e);
}

/******************************************************************************
 * SECTION: ns_var_delete
 * ----------------------------------------------------------------------------
 * @brief
 *
 *
 ******************************************************************************/

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

  if (ns_find_var_page (&fparams, e))
  {
    goto failed;
  }
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

  if (ns_remove (&rparams, e))
  {
    goto failed;
  }

  ASSERT (rparams.root == PGNO_NULL);

  switch (page_h_type (&prev))
  {
      // Previous is the root hash page
    case PG_VAR_HASH_PAGE:
    {
      vh_set_hash_value (
          page_h_w (&prev),
          fparams.hpos,
          vp_get_next (page_h_ro (&cur))
      );

      if (pgr_release (params.p, &prev, PG_VAR_HASH_PAGE, e))
      {
        goto failed;
      }

      break;
    }

      // Otherwise, we just need to link prev->cur
    case PG_VAR_PAGE:
    {
      vp_set_next (page_h_w (&prev), vp_get_next (page_h_ro (&cur)));

      if (pgr_release (params.p, &prev, PG_VAR_PAGE, e))
      {
        goto failed;
      }

      break;
    }
    default:
    {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }

  // Delete all overflow pages
  while (cur.mode != PHM_NONE)
  {
    pgno npg = dlgt_get_ovnext (page_h_ro (&cur));
    if (npg != PGNO_NULL)
    {
      if (pgr_get (&ovnext, PG_VAR_TAIL, npg, params.p, e))
      {
        goto failed;
      }
    }

    if (pgr_delete_and_release (params.p, params.tx, &cur, e))
    {
      goto failed;
    }

    page_h_xfer_ownership_ptr (&cur, &ovnext);
  }

  return error_trace (e);

failed:
  pgr_cancel_if_exists (params.p, &prev);
  pgr_cancel_if_exists (params.p, &cur);
  pgr_cancel_if_exists (params.p, &ovnext);

  return error_trace (e);
}

/******************************************************************************
 * SECTION: ns_var_get
 * ----------------------------------------------------------------------------
 * @brief
 *
 *
 ******************************************************************************/

err_t
ns_var_get (struct ns_var_get_params *params, error *e)
{
  page_h cur = page_h_create ();

  // Find variable first
  struct ns_find_var_page_params fparams = {
      .p     = params->p,
      .tx    = params->tx,
      .alloc = params->alloc,

      .vname = params->vname,
      .dvar  = &params->dest, // Dest
      .mode  = FP_FIND,

      .prev = NULL,
      .cur  = &cur,
  };

  if (ns_find_var_page (&fparams, e))
  {
    goto theend;
  }

  // Read the variable here
  struct ns_read_var_page_params rparams = {
      .p  = params->p,
      .tx = params->tx,

      .vp    = &cur,
      .alloc = params->alloc,

      .matches = true,
      .check   = NULL,

      .dest = &params->dest,

      .save_vname = false,
      .save_type  = true,
  };
  if (ns_read_var_page (&rparams, e))
  {
    goto theend;
  }

  if (pgr_release (params->p, &cur, PG_VAR_PAGE, e))
  {
    goto theend;
  }

theend:
  pgr_cancel_if_exists (params->p, &cur);

  return error_trace (e);
}

/******************************************************************************
 * SECTION: ns_var_get_or_create
 * ----------------------------------------------------------------------------
 * @brief
 *
 *
 ******************************************************************************/

err_t
ns_var_get_or_create (struct ns_var_get_or_create_params *params, error *e)
{
  // Try to get the variable
  struct ns_var_get_params gparams = {
      .p     = params->p,
      .tx    = params->tx,
      .vname = params->vname,
      .alloc = params->alloc,
      // Result = dest
  };

  error_silence (e);
  err_t err = ns_var_get (&gparams, e);
  error_unsilence (e);

  // Variable doesn't exist - so create it
  if (err == ERR_VARIABLE_NE)
  {
    e->cause_code = SUCCESS;
    e->cmlen      = 0;

    // Create the variable
    struct ns_var_create_params cparams = {
        .p     = params->p,
        .tx    = params->tx,
        .vname = params->vname,
        .type  = params->type,
    };
    if (ns_var_create (cparams, e))
    {
      goto failed;
    }

    // Try again
    if (ns_var_get (&gparams, e))
    {
      goto failed;
    }
  }
  else if (err < 0)
  {
    goto failed;
  }

  if (!type_equal (params->type, gparams.dest.dtype))
  {
    error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "Trying to create variable: %.*s but variable already exists and types "
        "are different",
        params->vname.len,
        params->vname.data
    );
    goto failed;
  }

  params->dest = gparams.dest;

  return SUCCESS;

failed:
  return error_trace (e);
}

#ifdef TESTING
TEST (ns_var_get_or_create)
{
  TEST_CASE ("check types is correct")
  {
    struct pgr_fixture f;
    pgr_fixture_create (&f);
    ns_init_var_hash_map (f.p, &f.e);
    struct txn tx;

    // Create
    {
      pgr_begin_txn (&tx, f.p, &f.e);

      struct ns_var_get_or_create_params params = {
          .p  = f.p,
          .tx = &tx,

          .vname = strfcstr ("foo"),
          .type  = &(struct type){.type = T_PRIM, .p = U32},
          .alloc = &f.alloc,
      };

      // Do it twice, both times succeed because it checks types
      test_assert (ns_var_get_or_create (&params, &f.e) == SUCCESS);
      test_assert (ns_var_get_or_create (&params, &f.e) == SUCCESS);

      pgr_commit (f.p, &tx, &f.e);
    }

    pgr_fixture_teardown (&f);
  }

  TEST_CASE ("errors on different type")
  {
    struct pgr_fixture f;
    pgr_fixture_create (&f);
    ns_init_var_hash_map (f.p, &f.e);
    struct txn tx;

    // Create
    {
      pgr_begin_txn (&tx, f.p, &f.e);

      struct ns_var_get_or_create_params params = {
          .p  = f.p,
          .tx = &tx,

          .vname = strfcstr ("foo"),
          .type  = &(struct type){.type = T_PRIM, .p = U32},
          .alloc = &f.alloc,
      };

      test_assert (ns_var_get_or_create (&params, &f.e) == SUCCESS);

      // Should fail if you pass a different type
      params.type = &(struct type){.type = T_PRIM, .p = I32};
      test_err_t_check (
          ns_var_get_or_create (&params, &f.e),
          ERR_INVALID_ARGUMENT,
          &f.e
      );

      pgr_commit (f.p, &tx, &f.e);
    }

    pgr_fixture_teardown (&f);
  }

  TEST_CASE ("Big variable short type")
  {
    struct pgr_fixture f;
    pgr_fixture_create (&f);
    ns_init_var_hash_map (f.p, &f.e);

    for (u32 i = 0; i < 100; ++i)
    {
      struct txn tx;

      // Long variable name
      {
        pgr_begin_txn (&tx, f.p, &f.e);

        u32   len  = randu32r (NS_PAGE_SIZE, NS_PAGE_SIZE * 10);
        char *name = i_malloc (len, 1, &f.e);
        for (u32 k = 0; k < len - 1; ++k)
        {
          name[k] = 'a' + randu32r (0, 26);
        }
        name[len - 1] = '\0';

        struct ns_var_get_or_create_params params = {
            .p  = f.p,
            .tx = &tx,

            .vname = strfcstr (name),
            .type  = &(struct type){.type = T_PRIM, .p = U32},
            .alloc = &f.alloc,
        };

        // Do it twice, both times succeed because it checks types
        test_assert (ns_var_get_or_create (&params, &f.e) == SUCCESS);
        test_assert (ns_var_get_or_create (&params, &f.e) == SUCCESS);

        i_free (name);

        pgr_commit (f.p, &tx, &f.e);
      }
    }

    pgr_fixture_teardown (&f);
  }

  TEST_CASE ("Big variable big type")
  {
    struct pgr_fixture f;
    pgr_fixture_create (&f);
    ns_init_var_hash_map (f.p, &f.e);

    for (u32 i = 0; i < 100; ++i)
    {
      struct txn tx;

      // Long variable name
      {
        pgr_begin_txn (&tx, f.p, &f.e);

        // Generate a random name that spans 1 - 10 pages
        char *name;
        {
          u32 len = randu32r (NS_PAGE_SIZE, NS_PAGE_SIZE * 10);
          name    = i_malloc (len, 1, &f.e);
          for (u32 k = 0; k < len - 1; ++k)
          {
            name[k] = 'a' + randu32r (0, 26);
          }
          name[len - 1] = '\0';
        }

        // Generate a random type that goes 0 - 10 layers deep
        struct type *t = type_random (&f.alloc, randu32r (0, 5), &f.e);
        i_log_info ("%d/%d\n", i, 100);

        struct ns_var_get_or_create_params params = {
            .p  = f.p,
            .tx = &tx,

            .vname = strfcstr (name),
            .type  = t,
            .alloc = &f.alloc,
        };

        // Do it twice, both times succeed because it checks types
        test_assert (ns_var_get_or_create (&params, &f.e) == SUCCESS);
        test_assert (ns_var_get_or_create (&params, &f.e) == SUCCESS);

        i_free (name);

        pgr_commit (f.p, &tx, &f.e);
      }
    }

    pgr_fixture_teardown (&f);
  }

  TEST_CASE ("Big type short variable name")
  {
    for (int i = 0; i < 100; ++i)
    {
      struct pgr_fixture f;
      pgr_fixture_create (&f);
      ns_init_var_hash_map (f.p, &f.e);
      struct txn tx;

      // Long variable name
      {
        pgr_begin_txn (&tx, f.p, &f.e);

        struct type *t = type_random (&f.alloc, randu32r (0, 10), &f.e);
        i_log_info ("%d/%d\n", i, 100);

        struct ns_var_get_or_create_params params = {
            .p  = f.p,
            .tx = &tx,

            .vname = strfcstr ("foo"),
            .type  = t,
            .alloc = &f.alloc,
        };

        // Do it twice, both times succeed because it checks types
        test_assert (ns_var_get_or_create (&params, &f.e) == SUCCESS);
        test_assert (ns_var_get_or_create (&params, &f.e) == SUCCESS);

        pgr_commit (f.p, &tx, &f.e);
      }

      pgr_fixture_teardown (&f);
    }
  }
}
#endif

/******************************************************************************
 * SECTION: ns_var_update
 * ----------------------------------------------------------------------------
 * @brief
 *
 *
 ******************************************************************************/

/*
 * Update rpt_root and nbytes on a variable page addressed by page number.
 *
 * Used when the caller already holds the variable page's pgno from an
 * earlier _find_var_page() call, avoiding a second hash-chain traversal.
 */
static err_t
ns_update_by_id (struct ns_var_update_params params, error *e)
{
  page_h cur = page_h_create ();

  if (pgr_get_writable (
          &cur,
          params.tx,
          PG_VAR_PAGE,
          params.retr.root,
          params.p,
          e
      ))
  {
    goto failed;
  }

  vp_set_root (page_h_w (&cur), params.newpg);
  vp_set_nbytes (page_h_w (&cur), params.nbytes);

  if (pgr_release (params.p, &cur, PG_VAR_PAGE, e))
  {
    goto failed;
  }

  goto failed;

failed:
  if (error_trace (e))
  {
    return error_trace (e);
  }
  else
  {
    return SUCCESS;
  }
}

/*
 * Update rpt_root and nbytes on a variable page addressed by variable name.
 *
 * Walks the hash chain via _find_var_page() in FP_FIND mode, then upgrades
 * the page to writable and stamps the new root pgno and byte count.
 */
static err_t
ns_update_by_name (struct ns_var_update_params params, error *e)
{
  page_h cur = page_h_create ();

  struct ns_find_var_page_params fparams = {
      .p  = params.p,
      .tx = params.tx,

      .vname = params.retr.vname,
      .dvar  = NULL,
      .mode  = FP_FIND,

      .hpos = PGNO_NULL,
      .prev = NULL,
      .cur  = &cur,
  };

  if (ns_find_var_page (&fparams, e))
  {
    goto failed;
  }

  vp_set_root (page_h_w (&cur), params.newpg);
  vp_set_nbytes (page_h_w (&cur), params.nbytes);

  if (pgr_release (params.p, &cur, PG_VAR_PAGE, e))
  {
    goto failed;
  }

  return SUCCESS;

failed:
  pgr_cancel_if_exists (params.p, &cur);
  return error_trace (e);
}

err_t
ns_var_update (struct ns_var_update_params params, error *e)
{
  switch (params.retr.type)
  {
    case VR_NAME:
    {
      return ns_update_by_name (params, e);
    }
    case VR_PG:
    {
      return ns_update_by_id (params, e);
    }
  }
  UNREACHABLE (); // LCOV_EXCL_LINE
}

/******************************************************************************
 * SECTION: ns_write_var_page
 * ----------------------------------------------------------------------------
 * @brief
 *
 *
 ******************************************************************************/

static err_t
ns_write_var_page_advance (struct ns_write_var_page_params *params, error *e)
{
  page_h next = page_h_create ();

  // Create a tail page
  if (pgr_new (&next, params->p, params->tx, PG_VAR_TAIL, e))
  {
    goto failed;
  }

  // Link with the existing page
  dlgtovlink (page_h_w (params->vp), page_h_w (&next));

  // Release current
  if ((pgr_release (params->p, params->vp, PG_VAR_PAGE | PG_VAR_TAIL, e)))
  {
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

  if (dtype_serialized == NULL)
  {
    // Allocation failed
    return error_trace (e);
  }
  else
  {
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
  struct bytes head = dlgt_get_bytes (page_h_w (params->vp));

  // Total number of variable and type bytes written so far
  p_size vwritten = 0;
  p_size twritten = 0;

  // First, write the variable name
  p_size lwritten =
      0; // Local number of bytes written so far - resets on every new page
  while (vwritten < params->var->vname.len)
  {
    // Advance forward one new node and reset local written and head
    if (lwritten == head.len)
    {
      if (ns_write_var_page_advance (params, e))
      {
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
  while (twritten < tlen)
  {
    // Advance forward one new node and reset local written and head
    if (lwritten == head.len)
    {
      if (ns_write_var_page_advance (params, e))
      {
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
  if (page_h_pgno (params->vp) != start)
  {
    // Release the tail (must be a tail because we're not at the starting page)
    if ((pgr_release (params->p, params->vp, PG_VAR_TAIL, e)))
    {
      goto theend;
    }

    // Get the starting node
    if ((pgr_get (params->vp, PG_VAR_PAGE, start, params->p, e)))
    {
      goto theend;
    }
  }
  else
  {
    // Release and get the page in read mode
    if (pgr_release (params->p, params->vp, PG_VAR_PAGE, e))
    {
      goto theend;
    }
    if (pgr_get (params->vp, PG_VAR_PAGE, start, params->p, e))
    {
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
