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

#define NO_IMPORT_ARRAY
#include "_pynumstore.h"
#include "pynumstore.h"

#include <Python.h>
#include <numpy/arrayobject.h>

// var_insert(db, txn_or_none, name: str, ofst: int, data: NDArray) -> None
//
// Inserts len(data) elements from data into the variable at element offset ofst.
PyObject *
pyns_var_insert (PyObject *Py_UNUSED (m), PyObject *args)
{
  PyObject   *db;
  PyObject   *txn_or_none;
  const char *name;
  PyObject   *ofst_obj;
  PyObject   *data_obj;

  if (!PyArg_ParseTuple (args, "OOsOO", &db, &txn_or_none, &name, &ofst_obj, &data_obj))
  {
    return NULL;
  }

  if (!PyLong_Check (ofst_obj))
  {
    PyErr_SetString (PyExc_TypeError, "offset must be int");
    return NULL;
  }

  long long ofst = PyLong_AsLongLong (ofst_obj);
  if (ofst == -1 && PyErr_Occurred ()) { return NULL; }

  if (!PyArray_Check (data_obj))
  {
    PyErr_SetString (PyExc_TypeError, "data must be a numpy array");
    return NULL;
  }

  PyArrayObject *arr    = (PyArrayObject *)data_obj;
  void          *buf    = PyArray_DATA (arr);
  npy_intp       nelems = PyArray_SIZE (arr);

  nsdb_t *ns = _active_ns (db, txn_or_none);
  if (!ns) { return NULL; }

  sb_size inserted = nsdb_insert (ns, name, buf, (sb_size)ofst, (b_size)nelems);
  if (inserted < 0)
  {
    _pyns_set_error (ns);
    return NULL;
  }

  Py_RETURN_NONE;
}
