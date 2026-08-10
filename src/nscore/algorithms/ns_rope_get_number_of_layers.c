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

#include <stdbool.h>

#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "nscore/algorithms/ns_rope_algorithms.h"
#include "nscore/ns_page_h.h"
#include "nscore/page/ns_page.h"
#include "nscore/page/ns_page_inner_node.h"
#include "nscore/pager/ns_pager.h"

i32
ns_get_number_of_layers (struct pager *p, pgno root, error *e)
{
  if (root == PGNO_NULL)
  {
    return 0;
  }

  page_h cur  = page_h_create ();
  i32    ret  = 0;
  pgno   next = root;

  while (true)
  {
    if (pgr_get (&cur, PG_INNER_NODE | PG_DATA_LIST, next, p, e))
    {
      goto failed;
    }

    ret++;

    if (page_h_type (&cur) == PG_DATA_LIST)
    {
      if (pgr_release (p, &cur, PG_DATA_LIST, e))
      {
        goto failed;
      }
      return ret;
    }
    else if (page_h_type (&cur) == PG_INNER_NODE)
    {
      next = in_get_leaf (page_h_ro (&cur), 0);
      if (pgr_release (p, &cur, PG_DATA_LIST, e))
      {
        goto failed;
      }
    }
  }

  return ret;

failed:
  pgr_cancel_if_exists (p, &cur);
  return error_trace (e);
}

/******************************************************************************
 * SECTION: Utils
 ******************************************************************************/

/**
i32
ns_get_length_to_the_right_of (struct pager *p, pgno pg, error *e)
{
  if (pg == PGNO_NULL)
  {
    return 0;
  }

  page_h cur = page_h_create ();
  i32    ret = 1;

  if (pgr_get (&cur, PG_INNER_NODE | PG_DATA_LIST, pg, p, e))
  {
    goto failed;
  }

  // Scan Right
  while (true)
  {
    pgno next = dlgt_get_next (page_h_ro (&cur));
    if (pgr_release (p, &cur, PG_INNER_NODE | PG_DATA_LIST, e))
    {
      goto failed;
    }

    if (next == PGNO_NULL)
    {
      break;
    }
    else
    {
      ret++;

      if (pgr_get (&cur, PG_INNER_NODE | PG_DATA_LIST, next, p, e))
      {
        goto failed;
      }
    }
  }
  return ret;

failed:
  pgr_cancel_if_exists (p, &cur);
  return error_trace (e);
}

i32
ns_get_length_to_the_left_of (struct pager *p, pgno pg, error *e)
{
  if (pg == PGNO_NULL)
  {
    return 0;
  }

  page_h cur = page_h_create ();
  i32    ret = 1;

  if (pgr_get (&cur, PG_INNER_NODE | PG_DATA_LIST, pg, p, e))
  {
    goto failed;
  }

  // Scan left
  while (true)
  {
    pgno next = dlgt_get_prev (page_h_ro (&cur));
    if (pgr_release (p, &cur, PG_INNER_NODE | PG_DATA_LIST, e))
    {
      goto failed;
    }

    if (next == PGNO_NULL)
    {
      break;
    }
    else
    {
      ret++;

      if (pgr_get (&cur, PG_INNER_NODE | PG_DATA_LIST, next, p, e))
      {
        goto failed;
      }
    }
  }
  return ret;

failed:
  pgr_cancel_if_exists (p, &cur);
  return error_trace (e);
}

i32
ns_get_length_of_layer_that_contains_node (struct pager *p, pgno pg, error *e)
{
  if (pg == PGNO_NULL)
  {
    return 0;
  }

  i32 left = ns_get_length_to_the_left_of (p, pg, e);
  if (left < 0)
  {
    return left;
  }

  i32 right = ns_get_length_to_the_left_of (p, pg, e);
  if (right < 0)
  {
    return right;
  }

  return left + right + 1;
}
*/
