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

// var_create(db, txn_or_none, name: str, type_str: str) -> None
PyObject *
pyns_var_create (PyObject *Py_UNUSED (m), PyObject *args)
{
  PyObject   *db;
  PyObject   *txn_or_none;
  const char *name;
  const char *type_str;

  if (!PyArg_ParseTuple (args, "OOss", &db, &txn_or_none, &name, &type_str))
  {
    return NULL;
  }

  nsdb_t *ns = _active_ns (db, txn_or_none);
  if (!ns)
  {
    return NULL;
  }

  if (nsdb_create (ns, name, type_str) < 0)
  {
    _pyns_set_error (ns);
    return NULL;
  }

  Py_RETURN_NONE;
}
