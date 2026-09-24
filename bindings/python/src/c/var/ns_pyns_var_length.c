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
#include "numstore/numstore.h"

PyObject *
pyns_var_length (PyObject *Py_UNUSED (m), PyObject *arg)
{
  numstore_var_t *var = _unwrap_var (arg);
  if (var == NULL) {
    return NULL;
  }
  return PyLong_FromUnsignedLongLong ((unsigned long long)numstore_var_len (var));
}
