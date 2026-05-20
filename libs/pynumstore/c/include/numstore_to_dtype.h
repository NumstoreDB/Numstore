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

/* numstore_to_dtype.h
 *
 * Convert a numstore `struct type` directly into a numpy dtype PyObject.
 * No string parsing is performed -- the dtype is constructed by passing
 * Python list/dict/tuple specs to PyArray_DescrConverter.
 */
#ifndef NUMSTORE_TO_DTYPE_H
#define NUMSTORE_TO_DTYPE_H

#define PY_SSIZE_T_CLEAN
#include "numstore_type.h"

#include <Python.h>

/* Returns a new reference to a numpy.dtype object, or NULL with a Python
 * exception set on failure.
 *
 * Primitives without a native numpy dtype (cf32, ci16..ci128, cu16..cu128)
 * are encoded as structured dtypes with `re` and `im` fields.
 *
 * The caller must have called import_array() (or its equivalent macro) in
 * the translation unit that holds the numpy API table before calling this.
 */
PyObject *numstore_type_to_dtype (const struct type *t);

#endif /* NUMSTORE_TO_DTYPE_H */
