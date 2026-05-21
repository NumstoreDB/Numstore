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

// This translation unit owns the numpy API table for the test binary.
// All other TUs use NO_IMPORT_ARRAY.
#define PY_SSIZE_T_CLEAN
#define PY_ARRAY_UNIQUE_SYMBOL _pynumstore_ARRAY_API
#define NPY_NO_DEPRECATED_API  NPY_2_0_API_VERSION

#include <Python.h>
#include <numpy/arrayobject.h>

int pynumstore_import_numpy (void);

int pynumstore_import_numpy (void) {
  return _import_array ();
}
