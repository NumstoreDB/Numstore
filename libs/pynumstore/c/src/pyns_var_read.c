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
#include "nscore/nshandle.h"
#include "nscore/rope.h"
#include "nscore/types.h"
#include "pynumstore.h"

#include <Python.h>
#include <c_specx.h>
#include <string.h>

PyObject *
pyns_var_read (PyObject *Py_UNUSED (m), PyObject *args)
{
  PyObject *db;
  PyObject *txn_or_none;

  long long var_id, key;
  if (!PyArg_ParseTuple (args, "OOLL", &db, &txn_or_none, &var_id, &key)) { return NULL; }

  struct nshandle *smf = _unwrap_db (db);
  if (!smf) { return NULL; }

  struct txn *txn = _unwrap_txn (txn_or_none);
  if (!txn && PyErr_Occurred ()) { return NULL; }

  /* TODO: smfile_read(smf, txn, var_id, key, &buf, &len); */
  static const char zeros[8] = {0};
  return PyBytes_FromStringAndSize (zeros, sizeof zeros);
}
