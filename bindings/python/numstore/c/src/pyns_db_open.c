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

PyObject *
pyns_db_open (PyObject *Py_UNUSED (m), PyObject *arg)
{
  if (!PyUnicode_Check (arg))
  {
    PyErr_SetString (PyExc_TypeError, "path must be str");
    return NULL;
  }

  const char *path = PyUnicode_AsUTF8 (arg);
  if (!path) { return NULL; }

  nsdb_t *ns = nsdb_open (path);
  if (!ns)
  {
    PyErr_SetString (PyExc_RuntimeError, "Failed to open numstore database");
    return NULL;
  }

  return PyCapsule_New ((void *)ns, DB_CAPSULE, _nspy_release_db);
}
