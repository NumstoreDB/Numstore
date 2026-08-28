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

#ifndef NS_PYMODULE_COMMON_H
#define NS_PYMODULE_COMMON_H

#include "core/ns_alloc.h"

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#define PY_ARRAY_UNIQUE_SYMBOL _NUMSTORE_ARRAY_API
#define NPY_NO_DEPRECATED_API  NPY_2_0_API_VERSION
#ifndef PYNUMSTORE_MODULE_MAIN
#  define NO_IMPORT_ARRAY
#endif
#include "core/ns_csx_assert.h"
#include "nscore/compiler/ns_compiler.h"
#include "nscore/nsdb/ns_nsdb_execute.h"
#include "nscore/types/ns_types.h"
#include "numstore/numstore.h"

#include <numpy/arrayobject.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Capsule names and sentinels (defined once, in ns_pymodule_common.c)
// ---------------------------------------------------------------------------

extern const char DB_CAPSULE[];
extern const char TXN_CAPSULE[];

// PyCapsule_SetPointer() forbids NULL, so a committed/rolled-back txn capsule
// (or a closed db capsule, see DB_CLOSED_SENTINEL) is repointed here instead
// of being nulled out, to mark it as spent.
extern char       TXN_CLOSED_SENTINEL;
extern char       DB_CLOSED_SENTINEL;

#define ENSURE_CAPSULE(obj, ret)                                           \
  do {                                                                     \
    if (!PyCapsule_CheckExact (obj)) {                                     \
      PyErr_SetString (PyExc_TypeError, "expected nstxn capsule or None"); \
      return ret;                                                          \
    }                                                                      \
  }                                                                        \
  while (0)

static inline nsdb_t *
_unwrap_db (PyObject *capsule)
{
  ENSURE_CAPSULE (capsule, NULL);
  void *ptr = PyCapsule_GetPointer (capsule, DB_CAPSULE);
  if (ptr == NULL) {
    return NULL; // error already set by PyCapsule_GetPointer
  }
  if (ptr == &DB_CLOSED_SENTINEL) {
    PyErr_SetString (PyExc_RuntimeError, "database is already closed");
    return NULL;
  }
  return (nsdb_t *)ptr;
}

static inline void
_nspy_release_db (PyObject *capsule)
{
  ENSURE_CAPSULE (capsule, );
  nsdb_t *ns = _unwrap_db (capsule);
  ASSERT (ns);
  nsdb_close (ns);
}

static inline ns_txn_t *
_unwrap_txn (PyObject *txn_capsule)
{
  ENSURE_CAPSULE (txn_capsule, NULL);
  void *ptr = PyCapsule_GetPointer (txn_capsule, TXN_CAPSULE);
  if (ptr == NULL) {
    return NULL; // error already set by PyCapsule_GetPointer
  }
  if (ptr == &TXN_CLOSED_SENTINEL) {
    PyErr_SetString (PyExc_RuntimeError, "transaction was already committed or rolled back");
    return NULL;
  }
  return (ns_txn_t *)ptr;
}

// Sets a Python RuntimeError from the nsdb error string.
static inline void
_pyns_set_error (nsdb_t *ns)
{
  const char *err = nsdb_strerror (ns);
  if (err) {
    PyErr_SetString (PyExc_RuntimeError, err);
  } else {
    PyErr_SetString (PyExc_RuntimeError, "numstore operation failed");
  }
}

static inline Py_ssize_t
elsize (PyArray_Descr *type)
{
#if NPY_FEATURE_VERSION >= NPY_2_0_API_VERSION
  return (type)->elsize;
#else
  return PyDataType_ELSIZE (type);
#endif
}

// ---------------------------------------------------------------------------
// Recursive numstore type -> numpy dtype conversion (ns_type_convert.c)
// ---------------------------------------------------------------------------

PyArray_Descr *pyns_type_to_dtype (const struct type *t);

// ---------------------------------------------------------------------------
// Python-exposed entry points, one per file
// ---------------------------------------------------------------------------

PyObject *pyns_ns_to_np (PyObject *m, PyObject *arg);
PyObject *pyns_open (PyObject *m, PyObject *arg);
PyObject *pyns_close (PyObject *m, PyObject *arg);
PyObject *pyns_begin (PyObject *m, PyObject *arg);
PyObject *pyns_commit (PyObject *m, PyObject *args);
PyObject *pyns_rollback (PyObject *m, PyObject *args);
PyObject *pyns_execute (PyObject *m, PyObject *args);

#endif // NS_PYMODULE_COMMON_H
