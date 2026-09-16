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
#include "core/ns_logging.h"
#include "nscore/algorithms/var/ns_var_algorithms.h"
#include "nscore/nsdb/ns_nsdb.h"
#include "nscore/nsdb/ns_nsdb_execute.h"
#include "nscore/types/ns_query.h"
#include "nscore/types/ns_types.h"

err_t
nsdb_get_and_print (struct nsdb *db, struct get_query *query, struct arena_alloc *alloc, error *e)
{
  struct ns_var_get_params gparams; // Get or create operation

  struct ns_txn           *tx = nsdb_begin (db, e);
  if (tx == NULL) {
    goto failed;
  }

  i_log_debug (
      "GET (txn = %" PRtxid
      ")"
      " - %.*s\n",
      tx->tid,
      strfmt (&query->name)
  );

  // GET VARIABLE
  {
    gparams = (struct ns_var_get_params){
        .p     = db->p,
        .tx    = tx,
        .vname = query->name,
        .alloc = alloc,
    };
    err_t err = ns_var_get (&gparams, e);
    if (query->if_exists && err == ERR_VARIABLE_NE) {
      e->cause_code = SUCCESS;
      e->cmlen      = 0;
      fprintf (stderr, "Variable: %.*s doesn't exist\n", strfmt (&query->name));
      goto commit;
    }
    WRAP_GOTO (err, failed_rollback);
  }

commit:
  if (nsdb_commit (db, tx, e) < 0) {
    goto failed;
  }
  return SUCCESS;

failed_rollback:
  nsdb_rollback (db, tx, e);

failed:
  return error_trace (e);
}
