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

/// ns_pyns_var_capsule.c
///
/// Lifecycle of the VAR_CAPSULE that carries a captured numstore_var into
/// Python. Nothing in Python releases a variable - the capsule destructor is
/// the only path, and it runs when the last reference goes away.

#include "core/ns_csx_assert.h"
#include "ns_pynumstore.h"
#include "numstore/numstore.h"

#include <stdlib.h>

nsdb_var_t *
_unwrap_var (PyObject *capsule)
{
  // Ensure that the object is a capsule, or else return NULL
  if (!PyCapsule_CheckExact (capsule)) {
    PyErr_SetString (PyExc_TypeError, "expected nsvar capsule");
    return NULL;
  }

  // Get the variable pointer from the capsule
  void *ptr = PyCapsule_GetPointer (capsule, VAR_CAPSULE);
  if (ptr == NULL) {
    return NULL; // error already set by PyCapsule_GetPointer
  }

  // Return the variable
  return (nsdb_var_t *)ptr;
}

// The only place a captured variable is ever released
static void
pyns_var_capsule_destructor (PyObject *capsule)
{
  void *ptr = PyCapsule_GetPointer (capsule, VAR_CAPSULE);
  if (ptr == NULL) {
    PyErr_Clear ();
    return;
  }
  ns_var_free ((nsdb_var_t *)ptr);
}

PyObject *
pyns_var_capsule_new (nsdb_var_t *var)
{
  ASSERT (var);

  PyObject *capsule = PyCapsule_New (var, VAR_CAPSULE, pyns_var_capsule_destructor);
  if (capsule == NULL) {
    // Ownership never transferred, so the variable is still ours to release
    ns_var_free (var);
    return NULL;
  }
  return capsule;
}

PyObject *
pyns_type_string (struct type *t)
{
  if (t == NULL) {
    PyErr_SetString (PyExc_RuntimeError, "variable has no type");
    return NULL;
  }

  // Ask for the buffer size first - a struct/union type has no bound worth
  // hard coding here.
  i32 needed = type_snprintf (NULL, 0, t);
  if (needed < 0) {
    PyErr_SetString (PyExc_RuntimeError, "failed to measure variable type");
    return NULL;
  }

  char *buf = malloc ((size_t)needed + 1);
  if (buf == NULL) {
    return PyErr_NoMemory ();
  }

  i32 len = type_snprintf (buf, (u32)needed + 1, t);
  if (len < 0) {
    free (buf);
    PyErr_SetString (PyExc_RuntimeError, "failed to render variable type");
    return NULL;
  }

  PyObject *ret = PyUnicode_FromStringAndSize (buf, (Py_ssize_t)len);
  free (buf);
  return ret;
}
