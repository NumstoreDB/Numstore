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

#include "nscore/variables.h"

#include "c_specx.h"
#include "nscore/types.h"

bool
variable_equal (const struct variable *left, const struct variable *right)
{
  if (!string_equal (left->vname, right->vname)) { return false; }
  if (!type_equal (left->dtype, right->dtype)) { return false; }
  if (left->var_root != right->var_root) { return false; }
  if (left->rpt_root != right->rpt_root) { return false; }
  if (left->nbytes != right->nbytes) { return false; }

  return true;
}

err_t
validate_vname (struct string vname, error *e)
{
  if (vname.len == 0) { return error_causef (e, ERR_INVALID_ARGUMENT, "variable name is empty"); }

  if (vname.len >= 4096)
  {
    return error_causef (e, ERR_INVALID_ARGUMENT, "variable name exceeds 4096 chars");
  }

  for (u32 i = 0; i < vname.len; ++i)
  {
    char c = vname.data[i];
    if (!is_alpha_num_generous (c))
    {
      return error_causef (
          e,
          ERR_INVALID_ARGUMENT,
          "variable name '%.*s' contains "
          "invalid characters",
          vname.len,
          vname.data
      );
    }
  }

  return SUCCESS;
}

void
variable_free (struct variable *v)
{
  type_free (v->dtype);
  v->dtype = NULL;

  i_free ((void *)v->vname.data);
  v->vname.data = NULL;
  v->vname.len  = 0;
  v->nbytes     = 0;
  v->rpt_root   = PGNO_NULL;
  v->var_root   = PGNO_NULL;
}

struct variable *
variable_malloc_copy (struct variable *v, struct malloc_plan *plan)
{
  bool active = plan->mode == MP_ALLOCING;

  struct variable *ret = malloc_plan_memcpy (plan, v, sizeof (struct variable));

  if (active) { ret->dtype = type_malloc_copy (ret->dtype, plan); }

  if (active) { ret->vname.data = malloc_plan_memcpy (plan, ret->vname.data, ret->vname.len); }

  return ret;
}
