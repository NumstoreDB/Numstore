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

#include <Python.h>

PyObject *
pyns_db_begin (PyObject *Py_UNUSED (m), PyObject *arg)
{
  nsdb_t *ns = _unwrap_db (arg);
  if (!ns) { return NULL; }

  // BEGIN TXN
  if (nsdb_begin (ns) < 0)
  {
    _pyns_set_error (ns);
    return NULL;
  }

  return PyCapsule_New ((void *)ns, TXN_CAPSULE, NULL);
}
