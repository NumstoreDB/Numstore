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

#include "pynumstore.h"
#include "numstore.h"

#include <stdlib.h>

static const char DB_CAPSULE[]  = "numstore.db";
static const char TXN_CAPSULE[] = "numstore.txn";

struct ns_db_wrap {
  nsdb_t *ns;
};

struct ns_txn_wrap {
  nsdb_t *ns;
};

HEADER_FUNC void _db_destructor (PyObject *cap) {
  struct ns_db_wrap *w = (struct ns_db_wrap *)PyCapsule_GetPointer (cap, DB_CAPSULE);
  if (w) {
    if (w->ns) { nsdb_close (w->ns); }
    free (w);
  }
}

HEADER_FUNC void _txn_destructor (PyObject *cap) {
  struct ns_txn_wrap *w = (struct ns_txn_wrap *)PyCapsule_GetPointer (cap, TXN_CAPSULE);
  if (w) {
    if (w->ns) {
      nsdb_rollback (w->ns);
      nsdb_close (w->ns);
    }
    free (w);
  }
}

HEADER_FUNC nsdb_t *_unwrap_db (PyObject *db) {
  struct ns_db_wrap *w = (struct ns_db_wrap *)PyCapsule_GetPointer (db, DB_CAPSULE);
  if (!w) { return NULL; }
  return w->ns;
}

HEADER_FUNC nsdb_t *_unwrap_txn (PyObject *txn_or_none) {
  if (txn_or_none == Py_None) { return NULL; }
  struct ns_txn_wrap *w = (struct ns_txn_wrap *)PyCapsule_GetPointer (txn_or_none, TXN_CAPSULE);
  if (!w) { return NULL; }
  return w->ns;
}

HEADER_FUNC nsdb_t *_resolve_handle (PyObject *db, PyObject *txn_or_none) {
  if (txn_or_none != Py_None) {
    return _unwrap_txn (txn_or_none);
  }
  return _unwrap_db (db);
}
