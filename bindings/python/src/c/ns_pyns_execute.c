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

  // Get the data backing the array
  PyArrayObject *contig = (PyArrayObject *)
      PyArray_FROM_OTF (data_obj, NPY_NOTYPE, NPY_ARRAY_IN_ARRAY | NPY_ARRAY_FORCECAST);
  if (contig == NULL) {
    return NULL;
  }

  return contig;
}

static inline PyObject *
pyns_execute_data_present (numstore_t *db, ns_txn_t *txn, char *query, PyObject *data_obj)
{
  ASSERT (data_obj);
  ASSERT (data_obj != Py_None);

  // Return Value
  PyObject      *ret    = NULL;

  // Get the contiguous array underlying the numpy array
  PyArrayObject *contig = get_bytes_from_nparray (data_obj);
  if (contig == NULL) {
    goto theend;
  }

  // Convert numpy array to a raw byte buffer
  char    *bytes  = PyArray_BYTES (contig);
  npy_intp nbytes = PyArray_NBYTES (contig);
  if (bytes == NULL) {
    PyErr_SetString (PyExc_RuntimeError, "array has no underlying buffer");
    goto theend;
  }

  // Execute query
  if (numstore_fexecute (db, txn, bytes, nbytes, "%s", query)) {
    goto theend;
  }

  ret = PyLong_FromSsize_t ((Py_ssize_t)ret);

theend:
  Py_XDECREF (contig);
  return NULL;
}

static inline PyObject *
pyns_execute_data_not_present (numstore_t *db, ns_txn_t *txn, char *query)
{
  // Return Value
  PyObject *ret = NULL;

  b_size    dlen;
  void     *ret = numstore_fexecute_malloc (db, txn, &dlen, "%s", query);

  ret           = PyLong_FromSsize_t ((Py_ssize_t)ret);

theend:
  Py_XDECREF (contig);
  return NULL;
}

PyObject *
pyns_execute (PyObject *Py_UNUSED (m), PyObject *args)
{
  PyObject *_db;      // Database capsule
  PyObject *_txn;     // Transaction capsule
  char     *query;    // Query string
  PyObject *data_obj; // Data object
  sb_size   ret = -1;

  // pyns_execute(db: capsule, txn: capsule | None, query: str, data: array | None)
  if (!PyArg_ParseTuple (args, "OOsO", &_db, &_txn, &query, &data_obj)) {
    goto theend;
  }

  // Fetch the database
  numstore_t *db = _unwrap_db (_db);
  if (db == NULL) {
    goto theend;
  }

  // Fetch the transaction
  ns_txn_t *txn = NULL;
  if (_txn != Py_None) {
    txn = _unwrap_txn (_txn);
    if (txn == NULL) {
      goto theend;
    }
  }

  // Grab the bytes from the numpy array
  if (data_obj != Py_None) {
    return pyns_execute_data_present (db, txn, query, data_obj);
  } else {
    bool handled = false;
    ret          = pyns_execute_malloc (db, txn, query, &handled);
    if (handled) {
      goto theend;
    }
  }

  // Execute the query
  ret = numstore_fexecute (db, txn, "%s", bytes, query);
  if (ret < 0) {
    _pyns_set_error_from_nsdb (db);
    goto theend;
  }

theend:
  Py_XDECREF (contig);

  if (ret < 0) {
    return NULL;
  } else {
    return PyLong_FromSsize_t ((Py_ssize_t)ret);
  }
}
