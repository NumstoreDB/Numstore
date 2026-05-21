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

#include "_pynumstore.h"
#include "pynumstore.h"

#include <Python.h>
#include <string.h>

PyObject *pyns_var_write (PyObject *Py_UNUSED (m), PyObject *args) {
  PyObject *db;
  PyObject *txn_or_none;

  long long var_id, key;
  Py_buffer data;
  if (!PyArg_ParseTuple (args, "OOLLy*", &db, &txn_or_none, &var_id, &key, &data)) { return NULL; }

  struct nshandle *smf = _unwrap_db (db);
  if (!smf) {
    PyBuffer_Release (&data);
    return NULL;
  }

  struct txn *txn = _unwrap_txn (txn_or_none);
  if (!txn && PyErr_Occurred ()) {
    PyBuffer_Release (&data);
    return NULL;
  }

  /* TODO: smfile_write(smf, txn, var_id, key, data.buf, data.len); */
  PyBuffer_Release (&data);
  Py_RETURN_NONE;
}
