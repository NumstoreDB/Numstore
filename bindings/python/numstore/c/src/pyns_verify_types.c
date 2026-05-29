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

int pyns_verify_types(PyArray_Descr *dtype, struct type *type)
{
  PyArray_Descr *nsdtype = (PyArray_Descr *)pyns_type_to_dtype (type);
  if (nsdtype == NULL) { return -1; }

  int eq = PyArray_EquivTypes (dtype, nsdtype);
  Py_DECREF (nsdtype);

  if (!eq)
    {
      PyErr_SetString (PyExc_ValueError, "array dtype does not match variable type");
      return -1;
    }

  return 0;
}
