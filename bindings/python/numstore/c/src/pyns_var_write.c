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
pyns_var_write (PyObject *Py_UNUSED (m), PyObject *args)
{
  PyObject   *db          = NULL;
  PyObject   *txn_or_none = NULL;
  PyObject   *key_obj     = NULL;
  PyObject   *data_obj    = NULL;
  PyObject   *r_start     = NULL;
  PyObject   *r_stop      = NULL;
  PyObject   *r_step      = NULL;
  nsdb_var_t *var         = NULL;
  const char *name        = NULL;

  if (!PyArg_ParseTuple (args, "OOsOO", &db, &txn_or_none, &name, &key_obj, &data_obj))
    goto fail;

  if (!PyArray_Check (data_obj))
    {
      PyErr_SetString (PyExc_TypeError, "data must be a numpy array");
      goto fail;
    }

  PyArrayObject *arr = (PyArrayObject *)data_obj;
  void          *buf = PyArray_DATA (arr);

  long long start = 0;
  long long step  = 1;
  long long stop  = 0;
  int       flags = 0;

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

      if (r_start != Py_None)
        {
          start = PyLong_AsLongLong (r_start);
          if (start == -1 && PyErr_Occurred ()) goto fail;
          flags |= START_PRESENT;
        }
      if (r_stop != Py_None)
        {
          stop = PyLong_AsLongLong (r_stop);
          if (stop == -1 && PyErr_Occurred ()) goto fail;
          flags |= STOP_PRESENT;
        }
      if (r_step != Py_None)
        {
          step = PyLong_AsLongLong (r_step);
          if (step == -1 && PyErr_Occurred ()) goto fail;
          flags |= STEP_PRESENT;
        }

      Py_DECREF (r_start); r_start = NULL;
      Py_DECREF (r_stop);  r_stop  = NULL;
      Py_DECREF (r_step);  r_step  = NULL;
    }

  nsdb_t *ns = _active_ns (db, txn_or_none);
  if (!ns) goto fail;

  if (!(flags & START_PRESENT))
    {
      start  = 0;
      flags |= START_PRESENT;
    }
  if (!(flags & STEP_PRESENT))
    {
      step   = 1;
      flags |= STEP_PRESENT;
    }
  if (!(flags & STOP_PRESENT))
    {
      sb_size len = nsdb_len (ns, name);
      if (len < 0) { _pyns_set_error (ns); goto fail; }
      stop   = (long long)len;
      flags |= STOP_PRESENT;
    }

  if (step <= 0)
    {
      PyErr_SetString (PyExc_ValueError, "key step must be positive");
      goto fail;
    }

  var = nsdb_get (ns, name);
  if (var == NULL) goto fail;

  if (pyns_verify_types (PyArray_DESCR (arr), var->var.dtype) != 0) goto fail;

  sb_size written = nsdb_write (ns, var, buf, (sb_size)start, (sb_size)step, (sb_size)stop, flags);
  if (written < 0) { _pyns_set_error (ns); goto fail; }

  nsdb_free (var);
  Py_RETURN_NONE;

fail:
  Py_XDECREF (r_start);
  Py_XDECREF (r_stop);
  Py_XDECREF (r_step);
  nsdb_free (var);
  return NULL;
}
