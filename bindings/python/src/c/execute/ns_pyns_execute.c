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
#include "numstore/numstore.h"

PyObject *
pyns_execute (PyObject *Py_UNUSED (m), PyObject *args)
{
  PyObject *_db;      // Database capsule
  PyObject *_txn;     // Transaction capsule
  char     *query;    // Query string
  PyObject *data_obj; // Data object

  // pyns_execute(db: capsule, txn: capsule | None, query: str, data: array | None)
  if (!PyArg_ParseTuple (args, "OOsO", &_db, &_txn, &query, &data_obj)) {
    return NULL;
  }

  // Fetch the database
  numstore_t *db = _unwrap_db (_db);
  if (db == NULL) {
    return NULL;
  }

  // Fetch the transaction
  ns_txn_t *txn = NULL;
  if (_txn != Py_None) {
    txn = _unwrap_txn (_txn);
    if (txn == NULL) {
      return NULL;
    }
  }

  if (data_obj != Py_None) {
    return pyns_execute_data_present (db, txn, query, data_obj);
  } else {
    return pyns_execute_data_not_present (db, txn, query);
  }
}
