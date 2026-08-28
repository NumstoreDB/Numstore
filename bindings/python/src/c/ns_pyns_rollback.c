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

#include "ns_pynumstore.h"

PyObject *
pyns_rollback (PyObject *Py_UNUSED (m), PyObject *args)
{
  PyObject *_db;
  PyObject *_txn;

  /* rollback(db, txn) */
  if (!PyArg_ParseTuple (args, "OO", &_db, &_txn)) {
    return NULL;
  }

  nsdb_t *db = _unwrap_db (_db);
  if (db == NULL) {
    return NULL; /* error already set by _unwrap_db */
  }

  ns_txn_t *txn = _unwrap_txn (_txn);
  if (txn == NULL) {
    return NULL; /* error already set by _unwrap_txn */
  }

  if (nsdb_rollback (db, txn) < 0) {
    _pyns_set_error (db);
    return NULL;
  }

  // txn is now invalid in memory - mark the capsule so reuse raises cleanly
  // instead of dereferencing a freed pointer
  if (PyCapsule_SetPointer (_txn, &TXN_CLOSED_SENTINEL) < 0) {
    PyErr_Clear ();
  }

  Py_RETURN_NONE;
}
