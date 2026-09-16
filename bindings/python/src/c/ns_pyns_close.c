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

#include "ns_pynumstore.h"

PyObject *
pyns_close (PyObject *Py_UNUSED (m), PyObject *arg)
{
  // Unwrap the database capsule
  numstore_t *ns = _unwrap_db (arg);
  if (!ns) {
    return NULL;
  }

  // Remove the automatic destructor so that the database closes
  PyCapsule_SetDestructor (arg, NULL);

  // Close the database manually and set the capsule to closed
  err_t ret = numstore_close (ns);
  if (PyCapsule_SetPointer (arg, &DB_CLOSED_SENTINEL) < 0) {
    PyErr_Clear ();
  }

  // Check close return status
  if (ret < 0) {
    PyErr_SetString (PyExc_RuntimeError, "Failed to close numstore database");
    return NULL;
  }

  Py_RETURN_NONE;
}
