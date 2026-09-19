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

#include <stdlib.h>

#define PYNS_DATA_CAPSULE_NAME "numstore.data"

static void
pyns_data_capsule_destructor (PyObject *capsule)
{
  void *data = PyCapsule_GetPointer (capsule, PYNS_DATA_CAPSULE_NAME);
  free (data);
}

/// Wrap a numstore-allocated buffer in a 1-D uint8 numpy array without
/// copying. The array takes ownership: the buffer is freed when the array
/// (and every view of it) is garbage collected. On failure the buffer is
/// freed and NULL is returned with an exception set.
static inline PyObject *
pyns_wrap_owned_bytes (void *data, b_size dlen)
{
  // Nothing returned -> empty array
  if (data == NULL || dlen == 0) {
    free (data);
    npy_intp dims[1] = {0};
    return PyArray_SimpleNew (1, dims, NPY_UINT8);
  }

  if ((unsigned long long)dlen > (unsigned long long)NPY_MAX_INTP) {
    free (data);
    PyErr_SetString (PyExc_OverflowError, "result too large for a numpy array");
    return NULL;
  }

  npy_intp  dims[1] = {(npy_intp)dlen};
  PyObject *arr     = PyArray_SimpleNewFromData (1, dims, NPY_UINT8, data);
  if (arr == NULL) {
    free (data);
    return NULL;
  }

  // The capsule owns the buffer; the array keeps the capsule alive
  PyObject *base = PyCapsule_New (data, PYNS_DATA_CAPSULE_NAME, pyns_data_capsule_destructor);
  if (base == NULL) {
    Py_DECREF (arr);
    free (data);
    return NULL;
  }

  // Steals the reference to base even on failure, so the capsule
  // destructor frees data in that case
  if (PyArray_SetBaseObject ((PyArrayObject *)arr, base) < 0) {
    Py_DECREF (arr);
    return NULL;
  }

  return arr;
}

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

static inline PyObject *
pyns_execute_data_present (numstore_t *db, ns_txn_t *txn, char *query, PyObject *data_obj)
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

  // Execute query
  struct numstore_plan plan = {0};
  plan.data                 = bytes;
  plan.dlen                 = nbytes;
  sb_size ret               = numstore_fexecute (db, txn, &plan, "%s", query);

  // Done with the buffer either way (was leaked on success before)
  Py_DECREF (contig);

  if (ret < 0) {
    _pyns_set_error_from_nsdb (db);
    return NULL;
  }

  return PyLong_FromSsize_t ((Py_ssize_t)ret);
}

static inline PyObject *
pyns_execute_data_not_present (numstore_t *db, ns_txn_t *txn, char *query)
{
  // Create the plan and let numstore allocate the output buffer
  struct numstore_plan plan = {0};
  if (numstore_plan_setopt (&plan, NSDB_PLAN_OPT_ALLOCATE_DATA) < 0) {
    _pyns_set_error_from_nsdb (db);
    return NULL;
  }

  sb_size ret = numstore_fexecute (db, txn, &plan, "%s", query);
  if (ret < 0) {
    _pyns_set_error_from_nsdb (db);
    return NULL;
  }

  return pyns_wrap_owned_bytes (plan.data, plan.dlen);
}

PyObject *
pyns_execute (PyObject *Py_UNUSED (m), PyObject *args)
{
  PyObject *_db;      // Database capsule
  PyObject *_txn;     // Transaction capsule
  char     *query;    // Query string
  PyObject *data_obj; // Data object

  // pyns_execute(db: capsule, txn: capsule | None, query: str, data: array | None)
  if (!PyArg_ParseTuple (args, "OOsO", &_db, &_txn, &query, &data_obj)) {
    return NULL;
  }

  // Fetch the database
  numstore_t *db = _unwrap_db (_db);
  if (db == NULL) {
    return NULL;
  }

  // Fetch the transaction
  ns_txn_t *txn = NULL;
  if (_txn != Py_None) {
    txn = _unwrap_txn (_txn);
    if (txn == NULL) {
      return NULL;
    }
  }

  if (data_obj != Py_None) {
    return pyns_execute_data_present (db, txn, query, data_obj);
  } else {
    return pyns_execute_data_not_present (db, txn, query);
  }
}
