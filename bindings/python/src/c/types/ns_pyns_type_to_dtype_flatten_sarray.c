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

#include "core/ns_csx_assert.h"
#include "ns_pynumstore.h"
#include "numpy/ndarraytypes.h"

PyArray_Descr *
pyns_type_to_dtype_flatten_sarray (const struct type *t, b_size top, struct dims_vec *vec)
{
  ASSERT (t);

  // Top length is the first dimension of the shape
  // ensure it is wrapped
  if (top > (b_size)NPY_MAX_INTP) {
    PyErr_SetString (PyExc_OverflowError, "result has too many elements for numpy");
    return NULL;
  }

  if (dims_vec_append (vec, (npy_intp)top) != 0) {
    return NULL;
  }

  // Append each top level array
  const struct type *cur = t;
  while (cur->type == T_SARRAY) {
    const struct sarray_t *sa = &cur->sa;
    ASSERT (sa->rank > 0);

    if (dims_vec_append_many (vec, sa->dims, sa->rank) != 0) {
      return NULL;
    }

    cur = sa->t;
  }

  PyArray_Descr *base = pyns_type_to_dtype (cur);
  if (base == NULL) {
    return NULL;
  }

  return base;
}
