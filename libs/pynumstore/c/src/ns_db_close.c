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

PyObject *pyns_db_close (PyObject *Py_UNUSED (m), PyObject *arg) {
  struct ns_db_wrap *w = (struct ns_db_wrap *)PyCapsule_GetPointer (arg, DB_CAPSULE);
  if (!w || !w->ns) { return NULL; }

  int rc = nsdb_close (w->ns);
  w->ns  = NULL;
  if (rc < 0) {
    PyErr_SetString (PyExc_RuntimeError, "nsdb_close failed");
    return NULL;
  }

  Py_RETURN_NONE;
}
