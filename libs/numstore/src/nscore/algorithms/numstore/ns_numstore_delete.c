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
#include "core/ns_testing.h"
#include "nscore/algorithms/numstore/ns_numstore_algorithms.h"
#include "nscore/algorithms/var/ns_var_algorithms.h"
#include "nscore/page/ns_page_fixture.h"

err_t
numstore_delete (struct pager *p, struct txn *tx, struct string name, bool if_exists, error *e)
{
  DBG_ASSERT (pager, p);
  DBG_ASSERT (ns_txn, tx);
  DBG_ASSERT (string, &name);
  DBG_ASSERT (clean_error, e);

  err_t err = ns_var_delete (p, tx, name, e);

  // If the variable doesn't exist it's ok
  if (if_exists && err == ERR_VARIABLE_NE) {
    error_reset (e);
    goto theend;
  }

  if (err < SUCCESS) {
    goto theend;
  }

theend:
  return error_trace (e);
}

#ifndef NDEBUG
TEST (numstore_delete)
{
  TEST_CASE ("if_exists on an existing variable deletes it")
  {
    // Setup - a pager with one variable
    struct pgr_fixture f;
    test_err_t_check (pgr_fixture_create (&f, &ctx->e), SUCCESS, &ctx->e);
    test_err_t_check (numstore_init_pager (f.p, &ctx->e), SUCCESS, &ctx->e);
    pgr_begin_txn (&f.tx, f.p, &ctx->e);

    struct variable var;
    numstore_create (f.p, &f.tx, strfcstr ("var"), TU32, &ctx->alloc, &var, &ctx->e);

    // Run
    err_t ret = numstore_delete (f.p, &f.tx, strfcstr ("var"), true, &ctx->e);
    test_assert_int_equal (ret, SUCCESS);

    // The variable is gone
    test_err_t_check (
        numstore_get (f.p, &f.tx, false, strfcstr ("var"), &ctx->alloc, &var, &ctx->e),
        ERR_VARIABLE_NE,
        &ctx->e
    );

    // Teardown
    pgr_commit (f.p, &f.tx, &ctx->e);
    pgr_fixture_teardown (&f, &ctx->e);
    arena_alloc_free_all (&ctx->alloc);
  }

  TEST_CASE ("a missing variable is an error unless if_exists is set")
  {
    // Setup
    struct pgr_fixture f;
    test_err_t_check (pgr_fixture_create (&f, &ctx->e), SUCCESS, &ctx->e);
    test_err_t_check (numstore_init_pager (f.p, &ctx->e), SUCCESS, &ctx->e);
    pgr_begin_txn (&f.tx, f.p, &ctx->e);

    // Run
    test_err_t_check (
        numstore_delete (f.p, &f.tx, strfcstr ("nope"), false, &ctx->e),
        ERR_VARIABLE_NE,
        &ctx->e
    );
    test_assert_int_equal (numstore_delete (f.p, &f.tx, strfcstr ("nope"), true, &ctx->e), SUCCESS);

    // Teardown
    pgr_commit (f.p, &f.tx, &ctx->e);
    pgr_fixture_teardown (&f, &ctx->e);
  }
}
#endif
