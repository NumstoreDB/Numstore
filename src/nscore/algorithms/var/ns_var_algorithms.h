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

#include "core/ns_alloc.h"
#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "nscore/pager/ns_pager.h"
#include "nscore/txn_table/ns_txn_table.h"
#include "nscore/types/ns_variables.h"

err_t ns_init_var_hash_map (struct pager *p, error *e);
err_t ns_valid (struct pager *p, error *e);

struct var_retrieval
{
  enum
  {
    VR_PG,
    VR_NAME,
  } type;

  union {
    struct string vname;
    pgno          root;
  };
};

struct ns_var_get_params
{
  struct pager     *p;
  struct txn       *tx;

  struct string     vname;
  struct allocator *alloc;

  struct variable   dest;
};

err_t ns_var_get (struct ns_var_get_params *params, error *e);

struct ns_var_get_or_create_params
{
  struct pager     *p;
  struct txn       *tx;

  struct string     vname;
  struct type      *type;
  struct allocator *alloc;

  struct variable   dest;
};

err_t ns_var_get_or_create (struct ns_var_get_or_create_params *params, error *e);

/******************************************************************************
 * SECTION: Creating Variables
 ******************************************************************************/

struct ns_var_create_params
{
  struct pager *p;
  struct txn   *tx;

  struct string vname;
  struct type  *type;
};

spgno ns_var_create (struct ns_var_create_params params, error *e);

/******************************************************************************
 * SECTION: Updating Variables
 ******************************************************************************/

struct ns_var_update_params
{
  struct pager        *p;
  struct txn          *tx;

  struct var_retrieval retr;

  // New values
  pgno                 newpg;
  b_size               nbytes;
};

err_t ns_var_update (struct ns_var_update_params params, error *e);

/******************************************************************************
 * SECTION: Deleting Variables
 ******************************************************************************/

struct ns_var_delete_params
{
  struct pager *p;
  struct txn   *tx;

  struct string vname;
};

err_t ns_var_delete (struct ns_var_delete_params params, error *e);

#endif
