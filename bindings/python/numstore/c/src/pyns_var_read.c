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
#include "numstore.h"

PyObject *
pyns_var_read (PyObject *Py_UNUSED (m), PyObject *args)
{
  PyObject      *db          = NULL;
  PyObject      *txn_or_none = NULL;
  PyObject      *key_obj     = NULL;
  PyObject      *r_start     = NULL;
  PyObject      *r_stop      = NULL;
  PyObject      *r_step      = NULL;
  PyObject      *arr         = NULL;
  PyArray_Descr *dtype       = NULL;
  nsdb_var_t    *var         = NULL;
  void          *buf         = NULL;
  const char    *name        = NULL;

  if (!PyArg_ParseTuple (args, "OOsO", &db, &txn_or_none, &name, &key_obj))
    goto fail;

  printf("POPOPOPO\n");
  fflush(stdout);

  nsdb_t *ns = _active_ns (db, txn_or_none);
  if (!ns) goto fail;

  printf("THERE\n");
  fflush(stdout);

  long long start = -1;
  long long step = -1;
  long long stop = -1;
  int       flags;

  printf("HERE\n");
  fflush(stdout);

    if (PyLong_Check (key_obj))
    {
      start = PyLong_AsLongLong (key_obj);
      if (start == -1 && PyErr_Occurred ()) goto fail;
      step  = 1;
      stop  = start + 1;
      flags = START_PRESENT | STOP_PRESENT | STEP_PRESENT;
    }
  else
    {
      r_start = PyObject_GetAttrString (key_obj, "start");
      r_stop  = PyObject_GetAttrString (key_obj, "stop");
      r_step  = PyObject_GetAttrString (key_obj, "step");
      if (!r_start || !r_stop || !r_step) goto fail;

      flags = COLON_PRESENT;

      if (r_start != Py_None) {
        start = PyLong_AsLongLong (r_start);
        if (start == -1 && PyErr_Occurred ()) goto fail;
        flags |= START_PRESENT;
      }
      if (r_stop != Py_None) {
        stop = PyLong_AsLongLong (r_stop);
        if (stop == -1 && PyErr_Occurred ()) goto fail;
        flags |= STOP_PRESENT;
      }
      if (r_step != Py_None) {
        step = PyLong_AsLongLong (r_step);
        if (step == -1 && PyErr_Occurred ()) goto fail;
        flags |= STEP_PRESENT;
      }

      Py_DECREF (r_start); r_start = NULL;
      Py_DECREF (r_stop);  r_stop  = NULL;
      Py_DECREF (r_step);  r_step  = NULL;
    }

  printf("ASDASDASDASD\n");
  fflush(stdout);

  if (step <= 0 && STEP_PRESENT & flags)
    {
      PyErr_SetString (PyExc_ValueError, "key step must be positive");
      goto fail;
    }

  printf("SDASDASDASD\n");
  fflush(stdout);

  var = nsdb_get (ns, name);
  if (var == NULL) goto fail;

  printf("SDASDAlDASD\n");
  fflush(stdout);

  dtype = (PyArray_Descr *)pyns_type_to_dtype (var->var.dtype);
  if (dtype == NULL) goto fail;

  printf("SDiSDAlDASD\n");
  fflush(stdout);

#if NPY_FEATURE_VERSION >= NPY_2_0_API_VERSION
  npy_intp tsize = dtype->elsize;
#else
  npy_intp tsize = (npy_intp)PyDataType_ELSIZE (dtype);
#endif

  fflush(stdout);

  npy_intp nelems_max;
  if (stop <= start) { nelems_max = 0; }
  else { nelems_max = (npy_intp)((stop - start + step - 1) / step); }

  printf("DolDASD %ld %ld\n", tsize, nelems_max);

  if (nelems_max > 0)
    {
      buf = malloc ((size_t)(nelems_max * tsize));
      if (!buf) { PyErr_NoMemory (); goto fail; }
    }

  sb_size bytes_read = nsdb_read (ns, var, buf, (sb_size)start, (sb_size)step, (sb_size)stop, flags);
  if (bytes_read < 0) { _pyns_set_error (ns); goto fail; }

  npy_intp nelems_actual = (npy_intp)bytes_read;
  npy_intp dims[1]       = {nelems_actual};

  printf("SDiSDolDoSD\n\n");
  fflush(stdout);

  arr = PyArray_NewFromDescr (&PyArray_Type, dtype, 1, dims, NULL, NULL, 0, NULL);
  dtype = NULL; /* stolen by PyArray_NewFromDescr */
  if (!arr) goto fail;

  if (nelems_actual > 0)
    memcpy (PyArray_DATA ((PyArrayObject *)arr), buf, (size_t)(nelems_actual * tsize));

  free (buf);
  nsdb_free (var);

  printf("SiDSDolDoSD\n");
  fflush(stdout);

  return arr;


fail:
  Py_XDECREF (r_start);
  Py_XDECREF (r_stop);
  Py_XDECREF (r_step);
  Py_XDECREF (arr);
  Py_XDECREF (dtype);
  free (buf);
  nsdb_free (var);
  return NULL;
}
