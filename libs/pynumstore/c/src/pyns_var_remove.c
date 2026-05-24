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
#include <stdlib.h>

// var_remove(db, txn_or_none, name: str, key: int|range) -> NDArray
//
// Removes and returns the elements at key, shifting remaining elements left.
PyObject *
pyns_var_remove (PyObject *Py_UNUSED (m), PyObject *args)
{
  PyObject   *db;
  PyObject   *txn_or_none;
  const char *name;
  PyObject   *key_obj;

  if (!PyArg_ParseTuple (args, "OOsO", &db, &txn_or_none, &name, &key_obj)) { return NULL; }

  nsdb_t *ns = _active_ns (db, txn_or_none);
  if (!ns) { return NULL; }

  // Determine key range
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

  if (step <= 0)
  {
    PyErr_SetString (PyExc_ValueError, "key step must be positive");
    return NULL;
  }

  // Get variable dtype to allocate the capture buffer
  char *type_str = nsdb_type_str (ns, name);
  if (!type_str)
  {
    _pyns_set_error (ns);
    return NULL;
  }

  PyObject *type_str_py = PyUnicode_FromString (type_str);
  free (type_str);
  if (!type_str_py) { return NULL; }

  PyObject *dtype_obj = pyns_compile_type (NULL, type_str_py);
  Py_DECREF (type_str_py);
  if (!dtype_obj) { return NULL; }

  PyArray_Descr *dtype = (PyArray_Descr *)dtype_obj;
  npy_intp       tsize = (npy_intp)PyDataType_ELSIZE (dtype);

  // Compute maximum element count
  npy_intp nelems_max;
  if (stop <= start) { nelems_max = 0; }
  else { nelems_max = (npy_intp)((stop - start + step - 1) / step); }

  // Allocate capture buffer
  void *buf = NULL;
  if (nelems_max > 0)
  {
    buf = malloc ((size_t)(nelems_max * tsize));
    if (!buf)
    {
      Py_DECREF (dtype_obj);
      PyErr_NoMemory ();
      return NULL;
    }
  }

  sb_size bytes_removed =
      nsdb_remove (ns, name, buf, (sb_size)start, (sb_size)step, (sb_size)stop, flags);

  if (bytes_removed < 0)
  {
    free (buf);
    Py_DECREF (dtype_obj);
    _pyns_set_error (ns);
    return NULL;
  }

  // nsdb_remove returns element count (not bytes)
  npy_intp nelems_actual = (npy_intp)bytes_removed;
  npy_intp dims[1]       = { nelems_actual };

  Py_INCREF (dtype_obj);
  PyObject *arr = PyArray_NewFromDescr (&PyArray_Type, dtype, 1, dims, NULL, NULL, 0, NULL);
  Py_DECREF (dtype_obj);

  if (!arr)
  {
    free (buf);
    return NULL;
  }

  if (nelems_actual > 0)
  {
    memcpy (PyArray_DATA ((PyArrayObject *)arr), buf, (size_t)(nelems_actual * tsize));
  }

  free (buf);
  return arr;
}
