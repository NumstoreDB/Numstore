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

#ifndef NS_VAR_ALGORITHMS_H
#define NS_VAR_ALGORITHMS_H

#include "core/ns_arena_alloc.h"
#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "nscore/pager/ns_pager.h"
#include "nscore/txn_table/ns_txn_table.h"
#include "nscore/variables/ns_variables.h"

err_t ns_init_var_hash_map (struct pager *p, error *e);
err_t ns_valid (struct pager *p, error *e);

struct ns_var_get_params
{
  struct pager       *p;
  struct txn         *tx;

  struct string       vname;
  struct arena_alloc *alloc;

  struct variable     dest;
};

// create and get
err_t ns_var_get (struct ns_var_get_params *params, error *e);

struct ns_var_get_or_create_params
{
  struct pager       *p;
  struct txn         *tx;

  struct string       vname;
  struct type        *type;
  struct arena_alloc *alloc;

  struct variable     dest;
};

err_t ns_var_get_or_create (struct ns_var_get_or_create_params *params, error *e);
spgno ns_var_create (
    struct pager *p,
    struct txn   *tx,
    struct string vname,
    struct type  *type,
    error        *e
);

// Delete
err_t ns_var_delete (struct pager *p, struct txn *tx, struct string vname, error *e);

// Visit
typedef err_t (*var_consumer) (struct variable *v, void *ctx, error *e);
err_t ns_visit_variables (struct pager *p, var_consumer var, void *ctx, error *e);

// Update
err_t ns_var_update_by_var_root (
    struct pager *p,
    struct txn   *tx,
    pgno          root,
    pgno          newpg,
    b_size        nbytes,
    error        *e
);

err_t ns_var_update_by_name (
    struct pager *p,
    struct txn   *tx,
    struct string name,
    pgno          newpg,
    b_size        nbytes,
    error        *e
);

#endif
