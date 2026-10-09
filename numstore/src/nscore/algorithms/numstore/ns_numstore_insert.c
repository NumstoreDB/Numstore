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

#include "core/ns_arena_alloc.h"
#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_numerics.h"
#include "core/ns_stream.h"
#include "core/ns_stride.h"
#include "core/ns_testing.h"
#include "nscore/algorithms/numstore/ns_numstore_algorithms.h"
#include "nscore/algorithms/rope/ns_rope_algorithms.h"
#include "nscore/algorithms/var/ns_var_algorithms.h"
#include "nscore/page/ns_page_fixture.h"
#include "nscore/pager/ns_pager.h"
#include "nscore/types/ns_types.h"
#include "nscore/variables/ns_variables.h"

sb_size
numstore_insert_from_name (
    struct pager *NONNULL       p,
    struct txn *NONNULL         tx,
    struct string               vname,
    b_size                      ofst,
    b_size                      len,
    struct arena_alloc *NONNULL valloc,
    struct variable *NULLABLE   var,
    struct stream *NONNULL      src,
    error *NONNULL              e
)
{
  DBG_ASSERT (pager, p);
  DBG_ASSERT (ns_txn, tx);
  DBG_ASSERT (string, &vname);
  DBG_ASSERT (arena_alloc, valloc);
  DBG_ASSERT (stream, src);
  DBG_ASSERT (clean_error, e);

  // First, fetch the variable
  struct variable _var;
  if (var == NULL) {
    var = &_var;
  }
  if (numstore_get (p, tx, false, vname, valloc, var, e) < 0) {
    return error_trace (e);
  }

  // Then do the insert
  return numstore_insert (p, tx, var, ofst, len, src, e);
}

sb_size
numstore_insert (
    struct pager *NONNULL    p,
    struct txn *NONNULL      tx,
    struct variable *NONNULL var,
    b_size                   ofst,
    b_size                   len,
    struct stream *NONNULL   src,
    error *NONNULL           e
)
{
  DBG_ASSERT (pager, p);
  DBG_ASSERT (ns_txn, tx);
  DBG_ASSERT (variable, var);
  DBG_ASSERT (stream, src);
  DBG_ASSERT (clean_error, e);

  // Skip len 0 inserts
  if (len == 0) {
    return SUCCESS;
  }

  // Resolve sizes
  t_size                  tsize   = type_byte_size (var->dtype);
  b_size                  bofst   = var_resolve_index (var, tsize * ofst);

  // Do insert
  struct ns_insert_params iparams = {
      .p     = p,
      .src   = src,
      .tx    = tx,
      .root  = var->rpt_root,
      .bofst = bofst,
      .bytes = len * tsize,
  };
  sb_size ret = ns_insert (&iparams, e);
  if (ret < 0) {
    return error_trace (e);
  }

  // Update new sizes and root
  b_size newsize = var->nbytes + (len * tsize);
  pgno   newroot = iparams.root;

  if (ns_var_update_by_var_root (p, tx, var->var_root, newroot, newsize, e) < 0) {
    return error_trace (e);
  }

  // Update the user supplied variable
  var->nbytes += (len * tsize);
  var->rpt_root = iparams.root;

  ASSERT (ret % tsize == 0);

  return ret / tsize;
}

#ifndef NDEBUG
TEST (numstore_insert_from_var)
{
  struct pgr_fixture f;
  pgr_fixture_create (&f);
  numstore_init_pager (f.p, &f.e);
  pgr_begin_txn (&f.tx, f.p, &f.e);

  ALLOC_INIT (alloc);
  struct variable var;
  numstore_create (f.p, &f.tx, strfcstr ("var"), TU32, &alloc, &var, &f.e);

  TEST_CASE ("variable is present - capture variable")
  {
    // Void it so that we re fetch it
    struct variable var_test;

    // Initialize data
    u32             data[10];
    arr_range (data);
    istream_create_from_array (stream, data);

    // Do insert
    err_t ret = numstore_insert_from_name (
        f.p,
        &f.tx,
        strfcstr ("var"),
        0,
        10,
        &alloc,    // IMPORTANT - not null
        &var_test, // IMPORTANT - not null
        &stream,
        &f.e
    );

    // Check results
    test_assert_int_equal (ret, 10);
    test_assert (variable_equal (
        &(struct variable){
            .nbytes   = 10 * sizeof (u32),
            .var_root = var.var_root,
            .vname    = var.vname,
            .dtype    = var.dtype,
            .rpt_root = var_test.rpt_root, // Might have changed
        },
        &var_test
    ));

    // Read and validate the data
    u32 out_data[10];
    u32 expected[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    arr_range (out_data);
    ostream_create_from_array (out, out_data);
    sb_size read = numstore_read (f.p, &f.tx, &var_test, ustride0 (0), &out, &f.e);
    test_assert_equal (read, 10);
    test_assert_memequal (out_data, expected, sizeof (expected));
  }

  TEST_CASE ("variable is not present - capture variable")
  {
    // Initialize data
    u32 data[10];
    arr_range (data);
    istream_create_from_array (stream, data);

    // Do insert
    err_t ret = numstore_insert_from_name (
        f.p,
        &f.tx,
        strfcstr ("var"),
        0,
        10,
        &alloc,
        NULL, // IMPORTANT - null
        &stream,
        &f.e
    );

    struct variable var_test;
    ret = numstore_get (f.p, &f.tx, false, strfcstr ("var"), &alloc, &var_test, &f.e);

    // Check results
    test_assert_int_equal (ret, SUCCESS);
    test_assert (variable_equal (
        &(struct variable){
            .nbytes   = 20 * sizeof (u32),
            .var_root = var.var_root,
            .vname    = var.vname,
            .dtype    = var.dtype,
            .rpt_root = var_test.rpt_root, // Might have changed
        },
        &var_test
    ));

    // Read and validate the data
    u32 expected[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    u32 out_data[20];
    arr_range (out_data);
    ostream_create_from_array (out, out_data);
    sb_size read = numstore_read (f.p, &f.tx, &var_test, ustride0 (0), &out, &f.e);
    test_assert_equal (read, 20);
    test_assert_memequal (out_data, expected, sizeof (expected));
  }

  pgr_commit (f.p, &f.tx, &f.e);
  pgr_fixture_teardown (&f);
  ALLOC_CLOSE (alloc);
}

TEST (numstore_insert)
{}
#endif
