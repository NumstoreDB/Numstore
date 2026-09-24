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

#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "core/os/ns_filesystem.h"
#include "core/os/ns_memory.h"
#include "nscore/algorithms/numstore/ns_numstore_algorithms.h"
#include "nscore/nsdb/ns_nsdb.h"
#include "nscore/types/ns_types.h"
#include "nscore/variables/ns_variables.h"
#include "numstore/ns_numstore_internal.h"
#include "numstore/numstore.h"

#ifdef TESTING
#  include "core/testing/ns_testing.h"
#endif

#ifdef TESTING
/**
 * A small wrapper that just runs a query,
 * pass in data and dlen. Should be insert read remove
 * or write
 */
static inline sb_size
_numstore_fexecute_simple_with_data (
    numstore_t *ns,
    ns_txn_t   *txn,
    void       *data,
    b_size      dlen,
    const char *query,
    ...
)
{
  va_list ap;
  va_start (ap, query);

  struct numstore_plan plan;
  memset (&plan, 0, sizeof (plan));
  plan.data   = data;
  plan.dlen   = dlen;
  sb_size ret = numstore_vexecute (ns, txn, &plan, query, ap);

  va_end (ap);
  return ret;
}

/**
 * A small wrapper that just runs a query that
 * returns a variable
 */
static inline err_t
_numstore_fexecute_simple_with_var (
    numstore_var_t **dest,
    numstore_t      *ns,
    ns_txn_t        *txn,
    const char      *query,
    ...
)
{
  va_list ap;
  va_start (ap, query);

  struct numstore_plan plan;
  memset (&plan, 0, sizeof (plan));
  numstore_plan_setopt (&plan, NSDB_PLAN_OPT_CAPTURE_VAR);
  sb_size ret = numstore_vexecute (ns, txn, &plan, query, ap);

  va_end (ap);

  if (ret < 0) {
    return ret;
  }
  *dest = plan.var;

  return SUCCESS;
}

#endif
