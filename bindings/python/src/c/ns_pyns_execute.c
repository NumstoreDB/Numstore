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
#include "nscore/compiler/ns_compiler.h"
#include "nscore/nsdb/ns_nsdb.h"
#include "nscore/nsdb/ns_nsdb_execute.h"

static PyObject *
pyns_execute_malloc (nsdb_t *db, ns_txn_t *txn, const char *query_str, bool *handled)
{
  *handled = false;

  ALLOC_INIT (alloc);
  error        e        = error_create ();
  PyObject    *result   = NULL;
  bool         auto_txn = false;

  struct query q;
  if (compile_query (&q, query_str, &alloc, &e)) {
    goto theend;
  }

  if (q.type != QT_READ && q.type != QT_REMOVE) {
    goto theend;
  }
  *handled = true;

  if (txn == NULL) {
    txn = nsdb_begin (db);
    if (txn == NULL) {
      _pyns_set_error (db);
      goto theend;
    }
    auto_txn = true;
  }

  struct string    vname = (q.type == QT_READ) ? q.read.name : q.remove.name;

  struct variable *var;
  struct get_query gq = {.name = vname, .if_exists = false};
  if (nsdb_get (db, txn, &gq, &alloc, &var) < 0) {
    _pyns_set_error (db);
    goto theend_rollback;
  }

  t_size         tsize = type_byte_size (var->dtype);
  npy_intp       nelem = (npy_intp)(tsize ? var->nbytes / tsize : 0);

  PyArray_Descr *descr = pyns_type_to_dtype (var->dtype);
  if (descr == NULL) {
    goto theend_rollback;
  }

  // Steals `descr`.
  PyObject *arr = PyArray_SimpleNewFromDescr (1, &nelem, descr);
  if (arr == NULL) {
    goto theend_rollback;
  }

  sb_size
      n = nelem == 0
              ? 0
              : nsdb_execute_on_buffer (db, txn, &q, PyArray_BYTES ((PyArrayObject *)arr), &alloc);
  if (n < 0) {
    _pyns_set_error (db);
    Py_DECREF (arr);
    goto theend_rollback;
  }

  npy_intp     new_dims[1] = {n};
  PyArray_Dims newshape    = {new_dims, 1};
  PyObject    *resize_ret  = PyArray_Resize ((PyArrayObject *)arr, &newshape, 0, NPY_ANYORDER);
  if (resize_ret == NULL) {
    Py_DECREF (arr);
    goto theend_rollback;
  }
  Py_DECREF (resize_ret);

  if (auto_txn && nsdb_commit (db, txn) < 0) {
    _pyns_set_error (db);
    Py_DECREF (arr);
    goto theend;
  }

  result = arr;
  goto theend;

theend_rollback:
  if (auto_txn) {
    nsdb_rollback (db, txn);
  }

theend:
  ALLOC_CLOSE (alloc);
  return result;
}

PyObject *
pyns_execute (PyObject *Py_UNUSED (m), PyObject *args)
{
  PyObject *_db;
  PyObject *_txn;
  char     *query;
  PyObject *data_obj;

  // Parse arguments
  //    execute(db, txn | None, query, data | None)
  if (!PyArg_ParseTuple (args, "OOsO", &_db, &_txn, &query, &data_obj)) {
    return NULL;
  }

  // The underlying data buffer of data if it's present
  PyArrayObject *contig = NULL;

  if (data_obj != Py_None) {
    // Check that it's an array
    if (!PyArray_Check (data_obj)) {
      PyErr_SetString (PyExc_TypeError, "data must be a numpy array or None");
      return NULL;
    }

    // Get the data backing the array
    contig = (PyArrayObject *)
        PyArray_FROM_OTF (data_obj, NPY_NOTYPE, NPY_ARRAY_IN_ARRAY | NPY_ARRAY_FORCECAST);
    if (contig == NULL) {
      return NULL;
    }
  }

  // Fetch active handle
  nsdb_t *db = _unwrap_db (_db);
  if (db == NULL) {
    Py_XDECREF (contig);
    return NULL;
  }

  ns_txn_t *txn = NULL;
  if (_txn != Py_None) {
    txn = _unwrap_txn (_txn);
    if (txn == NULL) {
      Py_XDECREF (contig);
      return NULL;
    }
  }

  if (contig == NULL && data_obj == Py_None) {
    bool      handled = false;
    PyObject *result  = pyns_execute_malloc (db, txn, query, &handled);
    if (handled) {
      return result;
    }
  }

  char    *bytes  = contig ? PyArray_BYTES (contig) : NULL;
  npy_intp nbytes = contig ? PyArray_NBYTES (contig) : 0;
  (void)nbytes;
  if (contig != NULL && bytes == NULL) {
    PyErr_SetString (PyExc_RuntimeError, "array has no underlying buffer");
    Py_XDECREF (contig);
    return NULL;
  }

  // Execute the query
  sb_size ret = nsdb_fexecute (db, txn, "%s", bytes, query);
  if (ret < 0) {
    _pyns_set_error (db);
    Py_XDECREF (contig);
    return NULL;
  }

  Py_XDECREF (contig);
  return PyLong_FromSsize_t ((Py_ssize_t)ret);
}
