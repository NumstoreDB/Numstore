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
#include "nscore/page_fixture.h"
#include "nscore/pager.h"
#include "nscore/types.h"
#include "nscore/var.h"

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
    if (ns_var_create (cparams, e)) { goto failed; }

    // Try again
    if (ns_var_get (&gparams, e)) { goto failed; }
  }
  else if (err < 0) { goto failed; }

  if (!type_equal (params->type, gparams.dest.dtype))
  {
    error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "Trying to create variable: %.*s but variable already exists and types are different",
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

#ifndef NTEST
TEST (ns_var_get_or_create)
{
  TEST_CASE ("check types is correct")
  {
    struct pgr_fixture f;
    pgr_fixture_create (&f);
    ns_init_var_hash_map (f.p, &f.e);
    struct txn         tx;
    struct chunk_alloc alloc;
    chunk_alloc_create_default (&alloc);

    // Create
    {
      pgr_begin_txn (&tx, f.p, &f.e);

      struct ns_var_get_or_create_params params = {
          .p  = f.p,
          .tx = &tx,

          .vname = strfcstr ("foo"),
          .type  = &(struct type){.type = T_PRIM, .p = U32},
          .alloc = &alloc,
      };

      // Do it twice, both times succeed because it checks types
      test_assert (ns_var_get_or_create (&params, &f.e) == SUCCESS);
      test_assert (ns_var_get_or_create (&params, &f.e) == SUCCESS);

      pgr_commit (f.p, &tx, &f.e);
    }

    chunk_alloc_free_all (&alloc);
    pgr_fixture_teardown (&f);
  }

  TEST_CASE ("errors on different type")
  {
    struct pgr_fixture f;
    pgr_fixture_create (&f);
    ns_init_var_hash_map (f.p, &f.e);
    struct txn         tx;
    struct chunk_alloc alloc;
    chunk_alloc_create_default (&alloc);

    // Create
    {
      pgr_begin_txn (&tx, f.p, &f.e);

      struct ns_var_get_or_create_params params = {
          .p  = f.p,
          .tx = &tx,

          .vname = strfcstr ("foo"),
          .type  = &(struct type){.type = T_PRIM, .p = U32},
          .alloc = &alloc,
      };

      test_assert (ns_var_get_or_create (&params, &f.e) == SUCCESS);

      // Should fail if you pass a different type
      params.type = &(struct type){.type = T_PRIM, .p = I32};
      test_err_t_check (ns_var_get_or_create (&params, &f.e), ERR_INVALID_ARGUMENT, &f.e);

      pgr_commit (f.p, &tx, &f.e);
    }

    chunk_alloc_free_all (&alloc);
    pgr_fixture_teardown (&f);
  }

  TEST_CASE ("Big variable short type")
  {
    struct pgr_fixture f;
    pgr_fixture_create (&f);
    ns_init_var_hash_map (f.p, &f.e);

    for (u32 i = 0; i < 100; ++i)
    {
      struct txn         tx;
      struct chunk_alloc alloc;
      chunk_alloc_create_default (&alloc);

      // Long variable name
      {
        pgr_begin_txn (&tx, f.p, &f.e);

        u32 len = randu32r(PAGE_SIZE, PAGE_SIZE * 10);
        char *name = i_malloc (len, 1, &f.e);
        for (u32 k = 0; k < len - 1; ++k) { name[k] = 'a' + randu32r (0, 26); }
        name[len - 1] = '\0';

        struct ns_var_get_or_create_params params = {
            .p  = f.p,
            .tx = &tx,

            .vname = strfcstr (name),
            .type  = &(struct type){.type = T_PRIM, .p = U32},
            .alloc = &alloc,
        };

        // Do it twice, both times succeed because it checks types
        test_assert (ns_var_get_or_create (&params, &f.e) == SUCCESS);
        test_assert (ns_var_get_or_create (&params, &f.e) == SUCCESS);

        i_free (name);

        pgr_commit (f.p, &tx, &f.e);
      }

      chunk_alloc_free_all (&alloc);
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
      struct txn         tx;
      struct chunk_alloc alloc;
      chunk_alloc_create_default (&alloc);

      // Long variable name
      {
        pgr_begin_txn (&tx, f.p, &f.e);

        u32 len = randu32r(PAGE_SIZE, PAGE_SIZE * 10);
        char *name = i_malloc (len, 1, &f.e);
        for (u32 k = 0; k < len - 1; ++k) { name[k] = 'a' + randu32r (0, 26); }
        name[len - 1] = '\0';

        struct type *t           = type_random (&alloc, randu32r(0, 10), &f.e);
        i_log_info("%d/%d\n", i, 100);

        struct ns_var_get_or_create_params params = {
            .p  = f.p,
            .tx = &tx,

            .vname = strfcstr (name),
            .type  = t,
            .alloc = &alloc,
        };

        // Do it twice, both times succeed because it checks types
        test_assert (ns_var_get_or_create (&params, &f.e) == SUCCESS);
        test_assert (ns_var_get_or_create (&params, &f.e) == SUCCESS);

        i_free (name);

        pgr_commit (f.p, &tx, &f.e);
      }

      chunk_alloc_free_all (&alloc);
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

      struct chunk_alloc alloc;
      chunk_alloc_create_default (&alloc);

      // Long variable name
      {
        pgr_begin_txn (&tx, f.p, &f.e);

        struct type *t = type_random (&alloc, randu32r(0, 10), &f.e);
        i_log_info("%d/%d\n", i, 100);

        struct ns_var_get_or_create_params params = {
            .p  = f.p,
            .tx = &tx,

            .vname = strfcstr ("foo"),
            .type  = t,
            .alloc = &alloc,
        };

        // Do it twice, both times succeed because it checks types
        test_assert (ns_var_get_or_create (&params, &f.e) == SUCCESS);
        test_assert (ns_var_get_or_create (&params, &f.e) == SUCCESS);

        pgr_commit (f.p, &tx, &f.e);
      }

      chunk_alloc_free_all (&alloc);
      pgr_fixture_teardown (&f);
    }
  }
}
#endif
