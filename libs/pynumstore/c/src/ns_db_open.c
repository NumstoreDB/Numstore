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

PyObject *ns_db_open (PyObject *Py_UNUSED (m), PyObject *arg) {
  if (!PyUnicode_Check (arg)) {
    PyErr_SetString (PyExc_TypeError, "path must be str");
    return NULL;
  }

  nsdb_t *ns = nsdb_open (PyUnicode_AsUTF8 (arg));
  if (!ns) {
    PyErr_SetString (PyExc_RuntimeError, "nsdb_open failed");
    return NULL;
  }

  struct ns_db_wrap *w = malloc (sizeof (struct ns_db_wrap));
  if (!w) {
    nsdb_close (ns);
    PyErr_NoMemory ();
    return NULL;
  }
  w->ns = ns;

  return PyCapsule_New ((void *)w, DB_CAPSULE, _db_destructor);
}
