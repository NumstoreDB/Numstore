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

PyObject *
pyns_var_insert (PyObject *Py_UNUSED (m), PyObject *args)
{
  PyObject   *db         = NULL;
  PyObject   *txn_or_none = NULL;
  PyObject   *ofst_obj   = NULL;
  PyObject   *data_obj   = NULL;
  const char *name       = NULL;

  // Validate parameters
  if (!PyArg_ParseTuple (args, "OOsOO", &db, &txn_or_none, &name, &ofst_obj, &data_obj)){
    goto fail;
  }

  // check that ofst_obj is an int
  if (!PyLong_Check (ofst_obj))
    {
      PyErr_SetString (PyExc_TypeError, "offset must be int");
      goto fail;
    }

  long long ofst = PyLong_AsLongLong (ofst_obj);
  if (ofst == -1 && PyErr_Occurred ()) goto fail;

  // Check that data is a numpy array
  if (!PyArray_Check (data_obj))
    {
      PyErr_SetString (PyExc_TypeError, "data must be a numpy array");
      goto fail;
    }

  PyArrayObject *arr    = (PyArrayObject *)data_obj;
  void          *buf    = PyArray_DATA (arr);
  npy_intp       nelems = PyArray_SIZE (arr);

  nsdb_t *ns = _active_ns (db, txn_or_none);
  if (!ns) goto fail;

  sb_size inserted = nsdb_insert (ns, name, buf, (sb_size)ofst, (b_size)nelems);
  if (inserted < 0)
    {
      _pyns_set_error (ns);
      goto fail;
    }

  Py_RETURN_NONE;

fail:
  return NULL;
}

