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
#include <stdio.h>
#include <string.h>

PyObject *pyns_var_delete (PyObject *Py_UNUSED (m), PyObject *args) {
  PyObject *db;
  PyObject *txn_or_none;

  long long var_id, key;
  if (!PyArg_ParseTuple (args, "OOLL", &db, &txn_or_none, &var_id, &key)) { return NULL; }

  nsdb_t *handle = _resolve_handle (db, txn_or_none);
  if (!handle) { return NULL; }

  char buf[32];
  snprintf (buf, sizeof (buf), "%lld", var_id);

  sb_size rc = nsdb_remove (
      handle, buf, NULL, (sb_size)key, 1, (sb_size)key + 1, START_PRESENT | STOP_PRESENT | STEP_PRESENT);

  if (rc < 0) {
    PyErr_SetString (PyExc_RuntimeError, nsdb_strerror (handle));
    return NULL;
  }

  Py_RETURN_NONE;
}
