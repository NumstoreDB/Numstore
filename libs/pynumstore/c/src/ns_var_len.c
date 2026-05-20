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
#include "nscore/compiler.h"
#include "pynumstore.h"

#include "c_specx.h"
#include "nscore/types.h"

#include <Python.h>
#include <string.h>

PyObject *ns_var_len (PyObject *Py_UNUSED (m), PyObject *args) {
  PyObject *db;
  PyObject *txn_or_none;

  long long var_id;
  if (!PyArg_ParseTuple (args, "OOL", &db, &txn_or_none, &var_id)) { return NULL; }

  struct nshandle *smf = _unwrap_db (db);
  if (!smf) { return NULL; }

  struct txn *txn = _unwrap_txn (txn_or_none);
  if (!txn && PyErr_Occurred ()) { return NULL; }

  /* TODO: return PyLong_FromSsize_t(smfile_len(smf, txn, var_id)); */
  return PyLong_FromLong (16);
}
