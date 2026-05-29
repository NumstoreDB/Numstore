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
#include "_numstore.h"

PyObject *
pyns_var_insert (PyObject *Py_UNUSED (m), PyObject *args)
{
  PyObject   *db          = NULL;
  PyObject   *txn_or_none = NULL;
  PyObject   *ofst_obj    = NULL;
  PyObject   *data_obj    = NULL;
  const char *name        = NULL;
  nsdb_var_t *var         = NULL;


  if (!PyArg_ParseTuple (args, "OOsOO", &db, &txn_or_none, &name, &ofst_obj, &data_obj))
    goto fail;


  if (!PyLong_Check (ofst_obj))
    {
      PyErr_SetString (PyExc_TypeError, "offset must be int");
      goto fail;
    }


  long long ofst = PyLong_AsLongLong (ofst_obj);
  if (ofst == -1 && PyErr_Occurred ()) goto fail;


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

  var = nsdb_get (ns, name);
  if (var == NULL) goto fail;


  if (pyns_verify_types (PyArray_DESCR (arr), var->var.dtype) != 0) goto fail;


  sb_size inserted = nsdb_insert (ns, var, buf, (sb_size)ofst, (b_size)nelems);
  if (inserted < 0)
    {
      _pyns_set_error (ns);
      goto fail;
    }


  nsdb_free (var);


  Py_RETURN_NONE;

fail:
  nsdb_free (var);
  return NULL;
}

