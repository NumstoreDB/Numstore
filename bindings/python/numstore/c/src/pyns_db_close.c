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
pyns_db_close (PyObject *Py_UNUSED (m), PyObject *arg)
{
  nsdb_t *ns = _unwrap_db (arg);
  if (!ns)
  {
    return NULL;
  }

  PyCapsule_SetDestructor (arg, NULL);

  if (nsdb_close (ns) < 0)
  {
    PyErr_SetString (PyExc_RuntimeError, "Failed to close numstore database");
    return NULL;
  }

  Py_RETURN_NONE;
}
