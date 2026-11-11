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

// The tests of ns_visit_variables (libs/algorithms/var). They need a whole
// database to visit, and ns_nsdb sits above ns_algorithms, so they live here

#include "ns_error.h"
#include "ns_nsdb.h"
#include "ns_stdtypes.h"
#include "ns_testing.h"
#include "types/ns_types.h"
#include "types/ns_variables.h"
#include "var/ns_var_algorithms.h"
#include "var/ns_var_algorithms_internal.h"

#ifndef NDEBUG

struct visit_ctx
{
  u32    count;
  b_size total_nelems;
  t_size total_size;
  u32    varname_sum;
  u32    fail_on;
};

static err_t
test_visit_func (struct variable *var, void *ctx, error *e)
{
  if (var->vname.len != 4) {
    return error_causef (e, ERR_INVALID_ARGUMENT, "Failed test");
  }
  u32               v    = var->vname.data[3] - '0';

  struct visit_ctx *_ctx = ctx;
  _ctx->count += 1;
  _ctx->total_nelems += var->nbytes / type_byte_size (var->dtype);
  _ctx->total_size += type_byte_size (var->dtype);
  _ctx->varname_sum += v;

  if (v == _ctx->fail_on) {
    return error_causef (e, ERR_INVALID_ARGUMENT, "Intentional fail");
  }

  return SUCCESS;
}

TEST (ns_visit_variables)
{
  nsdb_cleanup ("test", ctx->os, &ctx->e);
  struct nsdb *db = nsdb_open ("test", ctx->mem, ctx->os, &ctx->e);
  nsdb_init_numstore (db, &ctx->e);

  TEST_CASE ("no variables")
  {
    struct visit_ctx vctx = {
        .count        = 0,
        .total_nelems = 0,
        .total_size   = 0,
        .varname_sum  = 0,
        .fail_on      = -1,
    };
    ns_visit_variables (db->p, test_visit_func, &vctx, &ctx->e);

    test_assert_int_equal (vctx.count, 0);
    test_assert_type_equal (vctx.total_nelems, 0, b_size, PRb_size);
    test_assert_int_equal (vctx.total_size, 0);
    test_assert_int_equal (vctx.varname_sum, 0);
  }

  TEST_CASE ("multiple variables")
  {
    struct nsdb_txn *tx = nsdb_begin (db, &ctx->e);

    nsdb_execute (db, tx, "create var0 u32");
    nsdb_execute (db, tx, "create var1 u32");
    nsdb_execute (db, tx, "create var2 u32");
    nsdb_execute (db, tx, "create var3 u32");
    nsdb_execute (db, tx, "create var4 u32");
    nsdb_execute (db, tx, "create var5 u32");
    nsdb_execute (db, tx, "create var6 u32");
    nsdb_execute (db, tx, "create var7 u32");
    nsdb_execute (db, tx, "create var8 u32");
    nsdb_execute (db, tx, "create var9 u32");

    nsdb_commit (db, tx);

    struct visit_ctx vctx = {
        .count        = 0,
        .total_nelems = 0,
        .total_size   = 0,
        .varname_sum  = 0,
        .fail_on      = -1,
    };
    ns_visit_variables (db->p, test_visit_func, &vctx, &ctx->e);

    test_assert_int_equal (vctx.count, 10);
    test_assert_type_equal (vctx.total_nelems, 0, b_size, PRb_size);
    test_assert_int_equal (vctx.total_size, 10 * sizeof (u32));
    test_assert_int_equal (vctx.varname_sum, 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9);
  }

  nsdb_close (db, &ctx->e);
}
#endif
