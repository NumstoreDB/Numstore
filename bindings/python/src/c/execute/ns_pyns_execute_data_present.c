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
#include "numstore/numstore.h"

static inline PyArrayObject *
get_bytes_from_nparray (PyObject *data_obj)
{
  // Check that it's a numpy array
  if (!PyArray_Check (data_obj)) {
    PyErr_SetString (PyExc_TypeError, "data must be a numpy array or None");
    return NULL;
  }

  // Get a C-contiguous, aligned view (or copy) of the array
  return (
      PyArrayObject *
  )PyArray_FROM_OTF (data_obj, NPY_NOTYPE, NPY_ARRAY_IN_ARRAY | NPY_ARRAY_FORCECAST);
}

PyObject *
pyns_execute_data_present (nsdb_t *db, txn_t *txn, char *query, PyObject *data_obj)
{
  ASSERT (data_obj);
  ASSERT (data_obj != Py_None);

  // Get the contiguous array underlying the numpy array
  PyArrayObject *contig = get_bytes_from_nparray (data_obj);
  if (contig == NULL) {
    return NULL;
  }

  // Convert numpy array to a raw byte buffer
  char    *bytes  = PyArray_BYTES (contig);
  npy_intp nbytes = PyArray_NBYTES (contig);
  if (bytes == NULL) {
    PyErr_SetString (PyExc_RuntimeError, "array has no underlying buffer");
    Py_DECREF (contig);
    return NULL;
  }

  // Data in hand means an insert/write - ns_write rejects anything else
  sb_size ret = ns_write (db, txn, bytes, (b_size)nbytes, "%s", query);
  Py_DECREF (contig);

  if (ret < 0) {
    _pyns_set_error_from_nsdb (db);
    return NULL;
  }

  return PyLong_FromSsize_t ((Py_ssize_t)ret);
}
