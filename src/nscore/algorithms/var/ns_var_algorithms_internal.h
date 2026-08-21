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

#ifndef NS_VAR_ALGORITHMS_INTERNAL_H
#define NS_VAR_ALGORITHMS_INTERNAL_H

#include "core/ns_alloc.h"
#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "nscore/pager/ns_pager.h"
#include "nscore/txn_table/ns_txn_table.h"
#include "nscore/types/ns_variables.h"

err_t ns_variable_valid (struct pager *p, pgno var_root, error *e);

struct ns_read_var_page_params
{
  struct pager        *p;
  struct txn          *tx;

  page_h              *vp;    // The currently loaded variable page
  struct allocator    *alloc; // Where to allocate stuff
  struct variable     *dest;  // Output variable

  bool                 matches;
  const struct string *check;

  bool                 save_vname;
  bool                 save_type;
};

err_t ns_read_var_page (struct ns_read_var_page_params *params, error *e);

struct ns_write_var_page_params
{
  struct pager          *p;
  struct txn            *tx;
  page_h                *vp;  // The currently loaded variable page
  const struct variable *var; // The variable to write
};

err_t ns_write_var_page (struct ns_write_var_page_params *params, error *e);

struct ns_find_var_page_params
{
  struct pager     *p;
  struct txn       *tx;
  struct allocator *alloc;

  struct string     vname;
  struct variable  *dvar;

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

err_t ns_find_var_page (struct ns_find_var_page_params *pms, error *e);

#endif
