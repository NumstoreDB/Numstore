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

/// pyns_open.c
///
/// pyns_open(path) -> capsule
/// Opens a numstore database and wraps the handle in a DB_CAPSULE, whose
/// destructor (_nspy_release_db) closes it if the Python object is garbage
/// collected without an explicit db.close().

#include "ns_pynumstore.h"

PyObject *
pyns_open (PyObject *Py_UNUSED (m), PyObject *arg)
{
  // Check that the object is a string
  if (!PyUnicode_Check (arg)) {
    PyErr_SetString (PyExc_TypeError, "path must be str");
    return NULL;
  }

  // Get the path as a utf8 string
  const char *path = PyUnicode_AsUTF8 (arg);
  if (!path) {
    return NULL;
  }

  // Open the database
  nsdb_t *ns = ns_open (path);
  if (!ns) {
    PyErr_SetString (PyExc_RuntimeError, "Failed to open numstore database");
    return NULL;
  }

  // Wrap it in a capsule
  return PyCapsule_New ((void *)ns, DB_CAPSULE, _nspy_release_db);
}
