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

#pragma once

#include "c_specx.h"
#include "node_updates.h"
#include "nscore/page_h.h"
#include "nscore/pager.h"
#include "nscore/rope.h"
#include "nscore/variables.h"

////////////////////////////////////////////////////////////
// Initialization

err_t ns_init_var_hash_map (struct pager *p, error *e);

////////////////////////////////////////////////////////////
// Validation

err_t ns_valid (struct pager *p, error *e);
err_t ns_variable_valid (struct pager *p, pgno var_root, error *e);

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

////////////////////////////////////////////////////////////
// Fetching Variables

struct ns_var_get_params
{
  struct pager *p;
  struct txn   *tx;

  struct string       vname;
  struct chunk_alloc *alloc;

  struct variable dest;
};
err_t ns_var_get (struct ns_var_get_params *params, error *e);

struct ns_read_var_page_params
{
  struct pager *p;
  struct txn   *tx;

  page_h             *vp;    // The currently loaded variable page
  struct chunk_alloc *alloc; // Where to allocate stuff
  struct variable    *dest;  // Output variable

  bool                 matches;
  const struct string *check;
};
err_t ns_read_var_page (struct ns_read_var_page_params *params, error *e);

struct ns_var_get_or_create_params
{
  struct pager *p;
  struct txn   *tx;

  struct string       vname;
  struct type        *type;
  struct chunk_alloc *alloc;

  struct variable dest;
};

err_t ns_var_get_or_create (struct ns_var_get_or_create_params *params, error *e);

////////////////////////////////////////////////////////////
// Creating Variables

struct ns_var_create_params
{
  struct pager *p;
  struct txn   *tx;

  struct string vname;
  struct type  *type;
};

spgno ns_var_create (struct ns_var_create_params params, error *e);

////////////////////////////////////////////////////////////
// Updating Variables

struct ns_var_update_params
{
  struct pager *p;
  struct txn   *tx;

  struct var_retrieval retr;

  // New values
  pgno   newpg;
  b_size nbytes;
};

err_t ns_var_update (struct ns_var_update_params params, error *e);

////////////////////////////////////////////////////////////
// Updating Variables

struct ns_var_delete_params
{
  struct pager *p;
  struct txn   *tx;

  struct string vname;
};

err_t ns_var_delete (struct ns_var_delete_params params, error *e);

struct ns_write_var_page_params
{
  struct pager          *p;
  struct txn            *tx;
  page_h                *vp;  // The currently loaded variable page
  const struct variable *var; // The variable to write
};

err_t ns_write_var_page (struct ns_write_var_page_params *params, error *e);

/**
 * Simply finds or creates the new page where vname should exist.
 * If this is a create operation - then the page is found and
 * the neighbor links are written, but the page itself is not completed
 * thats where you need to call ns_write_var_page
 */
struct ns_find_var_page_params
{
  struct pager       *p;
  struct txn         *tx;
  struct chunk_alloc *alloc;

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

err_t ns_find_var_page (struct ns_find_var_page_params *params, error *e);
