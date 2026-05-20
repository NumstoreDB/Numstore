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

#include "nscore/compiler.h"
#include "pynumstore.h"

#include "c_specx.h"
#include "nscore/types.h"

#include <Python.h>
#include <string.h>

PyObject *ns_db_open (PyObject *Py_UNUSED (m), PyObject *arg) {
  if (!PyUnicode_Check (arg)) {
    PyErr_SetString (PyExc_TypeError, "path must be str");
    return NULL;
  }

  /* TODO: smfile_t *smf = smfile_open(PyUnicode_AsUTF8(arg)); */
  return PyCapsule_New ((void *)(1), "numstore.db", NULL);
}
