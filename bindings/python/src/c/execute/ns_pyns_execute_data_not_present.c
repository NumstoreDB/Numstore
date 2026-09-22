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
#include "core/os/ns_memory.h"
#include "ns_pynumstore.h"
#include "nscore/variables/ns_variables.h"
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

PyObject *
pyns_execute_data_not_present (numstore_t *db, ns_txn_t *txn, char *query)
{
  // Ask for both outputs
  struct numstore_plan plan = {
      .data    = NULL,
      .dlen    = 0,
      .options = NSDB_PLAN_OPT_ALLOCATE_DATA | NSDB_PLAN_OPT_CAPTURE_VAR,
      .var     = NULL,
  };

  // Do the execute
  sb_size nelems = numstore_fexecute (db, txn, &plan, "%s", query);
  if (nelems < 0) {
    _pyns_set_error_from_nsdb (db);
    return NULL;
  }

  if (plan.data != NULL) {
    // Query returned data - we should also have touched a variable
    ASSERT (plan.var);

    // Take ownership of plan.data
    PyObject *arr = pyns_wrap_owned_bytes (plan.var->var.dtype, plan.data, (b_size)nelems);
    numstore_var_free (plan.var);
    return arr;
  } else if (plan.var != NULL && plan.var->var.dtype != NULL) {
    // Query returned a variable
    return pyns_var_capsule_new (plan.var);
  } else {
    // Query resolved no variable (delete, exit) - the capture is an empty
    // shell that no accessor can read, so it goes no further than here
    if (plan.var != NULL) {
      numstore_var_free (plan.var);
    }
    Py_RETURN_NONE;
  }
}
