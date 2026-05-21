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

PyObject *ns_var_write (PyObject *Py_UNUSED (m), PyObject *args) {
  PyObject *db;
  PyObject *txn_or_none;

  long long var_id, key;
  Py_buffer data;
  if (!PyArg_ParseTuple (args, "OOLLy*", &db, &txn_or_none, &var_id, &key, &data)) { return NULL; }

  nsdb_t *handle = _resolve_handle (db, txn_or_none);
  if (!handle) {
    PyBuffer_Release (&data);
    return NULL;
  }

  char buf[32];
  snprintf (buf, sizeof (buf), "%lld", var_id);

  sb_size rc = nsdb_write (
      handle, buf, data.buf, (sb_size)key, 1, (sb_size)key + 1, START_PRESENT | STOP_PRESENT | STEP_PRESENT);
  PyBuffer_Release (&data);

  if (rc < 0) {
    PyErr_SetString (PyExc_RuntimeError, nsdb_strerror (handle));
    return NULL;
  }

  Py_RETURN_NONE;
}
