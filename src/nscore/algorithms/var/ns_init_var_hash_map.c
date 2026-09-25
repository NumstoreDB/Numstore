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

#include "core/ns_error.h"
#include "core/testing/ns_testing.h"
#include "nscore/algorithms/var/ns_var_algorithms.h"
#include "nscore/page/ns_page.h"
#include "nscore/page/ns_page_data_list.h"
#include "nscore/page/ns_page_fixture.h"
#include "nscore/page/ns_page_h.h"
#include "nscore/pager/ns_pager.h"
#include "nscore/txn_table/ns_txn_table.h"

err_t
ns_init_var_hash_map (struct pager *p, error *e)
{
  page_h hp = page_h_create ();

  if (!pgr_isnew (p)) {
    return SUCCESS;
  }

  // BEGIN TXN
  struct txn tx;
  if (pgr_begin_txn (&tx, p, e)) {
    return error_trace (e);
  }

  // Create a new variable hash page
  if (pgr_new (&hp, p, &tx, PG_VAR_HASH_PAGE, e)) {
    goto failed;
  }

  bool valid = page_h_pgno (&hp) == VHASH_PGNO;

  if (pgr_release (p, &hp, PG_VAR_HASH_PAGE, e)) {
    goto failed;
  }

  // Next page should be valid
  //   this is a weak contract
  //   but assumes the structure of the pager,
  if (!valid) {
    error_causef (e, ERR_CORRUPT, "First page should be a variable hash page");
    goto failed;
  }

  // COMMIT
  if (pgr_commit (p, &tx, e)) {
    goto failed;
  }

  return error_trace (e);

failed:
  pgr_rollback (p, &tx, 0, e);
  return error_trace (e);
}

#ifdef TESTING
TEST (ns_init_var_hash_map)
{
  TEST_CASE ("green path")
  {
    struct pgr_fixture f;
    pgr_fixture_create (&f);
    ns_init_var_hash_map (f.p, &f.e);

    page_h vhp = page_h_create ();
    test_assert_int_equal (pgr_get (&vhp, PG_VAR_HASH_PAGE, 1, f.p, &f.e), SUCCESS);

    pgr_release (f.p, &vhp, PG_VAR_HASH_PAGE, &f.e);
    pgr_fixture_teardown (&f);
  }

  TEST_CASE ("corrupt database")
  {
    struct pgr_fixture f;
    pgr_fixture_create (&f);

    // Create an invalid database by creating an upfront page
    pgr_begin_txn (&f.tx, f.p, &f.e);
    page_h corrupt_page = page_h_create ();
    pgr_new (&corrupt_page, f.p, &f.tx, PG_DATA_LIST, &f.e);
    dl_make_valid (page_h_w (&corrupt_page));
    pgr_release (f.p, &corrupt_page, PG_DATA_LIST, &f.e);
    pgr_commit (f.p, &f.tx, &f.e);

    // Initialize - this should fail
    test_err_t_check (ns_init_var_hash_map (f.p, &f.e), ERR_CORRUPT, &f.e);

    pgr_fixture_teardown (&f);
  }

  TEST_CASE ("can't init twice")
  {
    struct pgr_fixture f;
    pgr_fixture_create (&f);

    test_assert (ns_init_var_hash_map (f.p, &f.e) == SUCCESS);
    test_err_t_check (ns_init_var_hash_map (f.p, &f.e), ERR_CORRUPT, &f.e);

    pgr_fixture_teardown (&f);
  }
}
#endif
