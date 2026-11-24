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

#include "ns_arena_alloc.h"
#include "ns_error.h"
#include "ns_page.h"
#include "ns_page_h.h"
#include "ns_page_var_hash_page.h"
#include "ns_page_var_page.h"
#include "ns_stdtypes.h"
#include "ns_testing.h"
#include "pager/ns_pager.h"
#include "types/ns_types.h"
#include "types/ns_variables.h"
#include "var/ns_var_algorithms.h"
#include "var/ns_var_algorithms_internal.h"

static err_t
ns_consume_one_var (struct pager *p, page_h *cur, var_consumer var, void *ctx, error *e)
{
  DBG_ASSERT (pager, p);

  ALLOC_INIT (alloc);
  struct variable                v;

  // Read this one variable page
  struct ns_read_var_page_params params = {
      .p          = p,
      .tx         = NULL,

      .vp         = cur,
      .alloc      = &alloc,
      .dest       = &v,

      .matches    = false,
      .check      = NULL,

      .save_vname = true,
      .save_type  = true,
  };
  if (ns_read_var_page (&params, e) < 0) {
    goto theend;
  }

  // Execute the consumer
  if ((*var) (&v, ctx, e) < 0) {
    goto theend;
  }

theend:
  ALLOC_CLOSE (alloc);
  return error_trace (e);
}

static err_t
ns_consume_var_chain (struct pager *p, pgno start, var_consumer var, void *ctx, error *e)
{
  DBG_ASSERT (pager, p);

  page_h cur = page_h_create ();

  err_t  ret = pgr_get (&cur, PG_VAR_PAGE, start, p, e);
  if (ret < 0) {
    goto failed;
  }

  while (true) {
    // Consume this page
    if (ns_consume_one_var (p, &cur, var, ctx, e) < 0) {
      goto failed;
    }

    // Maybe finish
    pgno next = vp_get_next (page_h_ro (&cur));

    if (pgr_release (p, &cur, PG_VAR_PAGE, e) < 0) {
      goto failed;
    }

    if (next == PGNO_NULL) {
      return SUCCESS;
    }

    ret = pgr_get (&cur, PG_VAR_PAGE, next, p, e);
    if (ret < 0) {
      goto failed;
    }
  }

failed:
  pgr_cancel_if_exists (&cur);
  return error_trace (e);
}

err_t
ns_visit_variables (struct pager *p, var_consumer var, void *ctx, error *e)
{
  DBG_ASSERT (pager, p);

  page_h vhp = page_h_create ();

  // Fetch the variable hash page
  err_t  ret = pgr_get (&vhp, PG_VAR_HASH_PAGE, VHASH_PGNO, p, e);
  if (ret < 0) {
    goto failed;
  }

  // Iterate through each root
  for (p_size i = 0; i < VH_HASH_LEN; ++i) {
    pgno pg = vh_get_hash_value (page_h_ro (&vhp), i);

    if (pg != PGNO_NULL) {
      if (ns_consume_var_chain (p, pg, var, ctx, e) < 0) {
        goto failed;
      }
    }
  }

  if (pgr_release (p, &vhp, PG_VAR_HASH_PAGE, e) < 0) {
    goto failed;
  }

  return SUCCESS;

failed:
  pgr_cancel_if_exists (&vhp);
  return error_trace (e);
}
