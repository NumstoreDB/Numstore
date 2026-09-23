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
#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "core/os/ns_memory.h"
#include "core/testing/ns_testing.h"
#include "nscore/compiler/ns_compiler.h"
#include "nscore/nsdb/ns_nsdb.h"
#include "nscore/nsdb/ns_nsdb_execute.h"
#include "nscore/types/ns_query.h"
#include "numstore/numstore.h"

struct numstore_plan
{
  struct numstore_var *var;
  struct i_mem         mem;
  struct arena_alloc   alloc;
  struct query         q;
};

struct numstore_plan *
numstore_plan_vcreate (struct i_mem mem, const char *query, va_list args, error *e)
{
  struct numstore_plan *plan = i_malloc (mem, 1, sizeof *plan, e);
  if (plan == NULL) {
    return NULL;
  }

  plan->var = NULL;
  plan->mem = mem;
  arena_alloc_create_default (&plan->alloc);

  // Compile the query
  sb_size ret = ns_query_fcompile (&plan->alloc, query, args, &plan->q, e);
  if (ret < 0) {
    arena_alloc_free_all (&plan->alloc);
    i_free (mem, plan);
    return NULL;
  }

  return plan;
}

int
numstore_prepare (numstore_t *ns, numstore_plan **out, const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  *out = numstore_plan_vcreate (ns->db->mem, fmt, ap, &ns->e);
  va_end (ap);

  if (*out == NULL) {
    return error_trace (&ns->e);
  }

  return SUCCESS;
}
