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

#include "core/ns_arena_alloc.h"
#include "core/ns_error.h"
#include "core/os/ns_memory.h"
#include "ns_pynumstore.h"
#include "nscore/compiler/ns_compiler.h"
#include "numpy/ndarraytypes.h"
#include "numstore/numstore.h"

#define PYNS_DATA_CAPSULE_NAME "numstore.data"

static void
pyns_data_capsule_destructor (PyObject *capsule)
{
  void *data = PyCapsule_GetPointer (capsule, PYNS_DATA_CAPSULE_NAME);
  if (data != NULL) {
    i_free (default_mem (), data);
  }
}

static PyObject *
pyns_wrap_owned_bytes (const struct type *dtype, void *data, b_size nelems)
{
  struct dims_vec vec;
  dims_vec_create (&vec);

  // Convert to type
  PyArray_Descr *descr = pyns_type_to_dtype_flatten_sarray (dtype, nelems, &vec);
  if (descr == NULL) {
    dims_vec_free (&vec);
    i_free (default_mem (), data);
    return NULL;
  }

  // Build the numpy array
  PyObject *arr = PyArray_NewFromDescr (
      &PyArray_Type,       // Array
      descr,               // Sub type (reference stolen)
      vec.len,             // Shape length
      vec.data,            // Shape
      NULL,                // strides: C-contiguous
      data,                // The raw data
      NPY_ARRAY_WRITEABLE, // Can write to array
      NULL
  );

  dims_vec_free (&vec);
  if (arr == NULL) {
    i_free (default_mem (), data);
    return NULL;
  }

  // Own data in a capsule to attach a destructor
  PyObject *arr_capsule = PyCapsule_New (
      data,
      PYNS_DATA_CAPSULE_NAME,
      pyns_data_capsule_destructor
  );
  if (arr_capsule == NULL) {
    Py_DECREF (arr); // arr has no arr_capsule yet -> does NOT free data
    i_free (default_mem (), data);
    return NULL;
  }

  if (PyArray_SetBaseObject ((PyArrayObject *)arr, arr_capsule) < 0) {
    // Decref'ing arr runs the capsule descructor and frees data
    // in pyns_data_capsule_destructor
    Py_DECREF (arr);
    return NULL;
  }

  return arr;
}

// The API no longer has one "execute anything" entry point: a query goes to
// ns_exec, ns_get_var or ns_read_malloc depending on what it is. Compile it
// once here purely to pick the branch - the chosen call compiles it again.
static int
pyns_query_type (const char *query, enum query_type *dest)
{
  struct query q;
  error        e = error_create ();

  ALLOC_INIT (temp);
  if (compile_query (&q, query, &temp, &e) < 0) {
    ALLOC_CLOSE (temp);
    _pyns_set_error_from_e (PyExc_RuntimeError, &e);
    return -1;
  }
  *dest = q.type;
  ALLOC_CLOSE (temp);

  return 0;
}

PyObject *
pyns_execute_data_not_present (nsdb_t *db, txn_t *txn, char *query)
{
  enum query_type qt;
  if (pyns_query_type (query, &qt) < 0) {
    return NULL;
  }

  switch (qt) {
      // Readable queries - the bytes come from ns_read_malloc, and the type to
      // hand numpy comes off the variable the query touches.
    case QT_READ:
    case QT_REMOVE: {
      nsdb_var_t *var = ns_get_var (db, txn, "%s", query);
      if (var == NULL) {
        _pyns_set_error_from_nsdb (db);
        return NULL;
      }

      b_size nelems = 0;
      void  *data   = ns_read_malloc (db, txn, &nelems, "%s", query);
      if (data == NULL) {
        ns_var_free (var);
        _pyns_set_error_from_nsdb (db);
        return NULL;
      }

      // Takes ownership of data
      PyObject *arr = pyns_wrap_owned_bytes (nsdb_var_type (var), data, nelems);
      ns_var_free (var);
      return arr;
    }

      // Resolves a variable without executing anything
    case QT_GET: {
      nsdb_var_t *var = ns_get_var (db, txn, "%s", query);
      if (var == NULL) {
        _pyns_set_error_from_nsdb (db);
        return NULL;
      }
      return pyns_var_capsule_new (var);
    }

      // Runs to completion and yields no variable of its own
    case QT_CREATE:
    case QT_DELETE: {
      if (ns_exec (db, txn, "%s", query) < 0) {
        _pyns_set_error_from_nsdb (db);
        return NULL;
      }
      Py_RETURN_NONE;
    }

      // Writable queries have nothing to write without a data argument
    case QT_INSERT:
    case QT_WRITE: {
      PyErr_SetString (PyExc_ValueError, "insert/write queries require a data argument");
      return NULL;
    }

      // Console-only queries carry nothing back into Python
    case QT_EXIT:
    case QT_HELP:
    default: {
      Py_RETURN_NONE;
    }
  }
}
