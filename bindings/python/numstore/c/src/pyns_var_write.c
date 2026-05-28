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

// var_write(db, txn_or_none, name: str, key: int|range, data: NDArray) -> None
PyObject *
pyns_var_write (PyObject *Py_UNUSED (m), PyObject *args)
{
  PyObject   *db;
  PyObject   *txn_or_none;
  const char *name;
  PyObject   *key_obj;
  PyObject   *data_obj;

  if (!PyArg_ParseTuple (args, "OOsOO", &db, &txn_or_none, &name, &key_obj, &data_obj))
  {
    return NULL;
  }

  if (!PyArray_Check (data_obj))
  {
    PyErr_SetString (PyExc_TypeError, "data must be a numpy array");
    return NULL;
  }

  PyArrayObject *arr = (PyArrayObject *)data_obj;
  void          *buf = PyArray_DATA (arr);

  long long start, step, stop;
  int       flags;

  if (PyLong_Check (key_obj))
  {
    start = PyLong_AsLongLong (key_obj);
    if (start == -1 && PyErr_Occurred ()) { return NULL; }
    step  = 1;
    stop  = start + 1;
    flags = START_PRESENT | STOP_PRESENT | STEP_PRESENT;
  }
  else
  {
    PyObject *r_start = PyObject_GetAttrString (key_obj, "start");
    PyObject *r_stop  = PyObject_GetAttrString (key_obj, "stop");
    PyObject *r_step  = PyObject_GetAttrString (key_obj, "step");
    if (!r_start || !r_stop || !r_step)
    {
      Py_XDECREF (r_start);
      Py_XDECREF (r_stop);
      Py_XDECREF (r_step);
      return NULL;
    }
    start = PyLong_AsLongLong (r_start);
    stop  = PyLong_AsLongLong (r_stop);
    step  = PyLong_AsLongLong (r_step);
    Py_DECREF (r_start);
    Py_DECREF (r_stop);
    Py_DECREF (r_step);
    if ((start == -1 || stop == -1 || step == -1) && PyErr_Occurred ()) { return NULL; }
    flags = START_PRESENT | STOP_PRESENT | STEP_PRESENT;
  }

  nsdb_t *ns = _active_ns (db, txn_or_none);
  if (!ns) { return NULL; }

  sb_size written = nsdb_write (ns, name, buf, (sb_size)start, (sb_size)step, (sb_size)stop, flags);
  if (written < 0)
  {
    _pyns_set_error (ns);
    return NULL;
  }

  Py_RETURN_NONE;
}
