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
#include "nscore/algorithms/numstore/ns_numstore_algorithms.h"
#include "nscore/nsdb/ns_nsdb_execute.h"

sb_size
nsdb_execute_malloc (
    struct nsdb        *ns,
    struct ns_txn      *tx,
    struct query       *q,
    struct variable    *var,
    struct arena_alloc *valloc,
    void              **data,
    error              *e
)
{
  ASSERT (tx);
  ASSERT (data);

  switch (q->type) {
    case QT_READ: {
      b_size len;
      *data = numstore_read_malloc (
          ns->p,
          tx,
          q->read.name,
          q->read.ustr,
          valloc,
          var,
          &len,
          ns->mem,
          e
      );
      if (*data == NULL) {
        return error_trace (e);
      }
      return len;
    }
    case QT_REMOVE: {
      b_size len;
      *data = numstore_remove_malloc (
          ns->p,
          tx,
          q->remove.name,
          q->remove.ustr,
          valloc,
          var,
          &len,
          ns->mem,
          e
      );
      if (*data == NULL) {
        return error_trace (e);
      }
      return len;
    }
    case QT_CREATE: {
      return numstore_create (ns->p, tx, q->create.name, q->create.type, valloc, var, e);
    }
    case QT_DELETE: {
      return numstore_delete (ns->p, tx, q->delete.name, q->delete.if_exists, e);
    }
    case QT_GET: {
      return numstore_get (ns->p, tx, q->get.if_exists, q->get.name, valloc, var, e);
    }
    case QT_EXIT:
    case QT_HELP: {
      // Nothing to do - maybe throw
      return SUCCESS;
    }
    case QT_INSERT:
    case QT_WRITE: {
      return error_causef (e, ERR_INVALID_ARGUMENT, "Must provide data for insert/write");
    }
  }
}
