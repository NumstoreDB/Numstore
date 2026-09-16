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

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#define PY_ARRAY_UNIQUE_SYMBOL _NUMSTORE_ARRAY_API
#define NPY_NO_DEPRECATED_API  NPY_2_0_API_VERSION
#ifndef PYNUMSTORE_MODULE_MAIN
#  define NO_IMPORT_ARRAY
#endif
#include "nscore/types/ns_types.h"
#include "numstore/numstore.h"

#include <numpy/arrayobject.h>
#include <string.h>

// Names of the capsule which stores the name
// for the transaction and database handle
extern const char DB_CAPSULE[];
extern const char TXN_CAPSULE[];

// When a database or transaction is closed - it's value is set to
// one of these sentinels.
extern char       TXN_CLOSED_SENTINEL;
extern char       DB_CLOSED_SENTINEL;

////////////// Private methods

// Get numstore objects from python capsules
numstore_t *_unwrap_db (PyObject *capsule);
ns_txn_t *_unwrap_txn (PyObject *txn_capsule);

// Release a database
void _nspy_release_db (PyObject *capsule);

// Set the error string from numstore error
void _pyns_set_error_from_e (error *e);
void _pyns_set_error_from_nsdb (numstore_t *e);

// Numpy compatible elsize
Py_ssize_t elsize (PyArray_Descr *type);

// Convert a numstore type to a numpy type
PyArray_Descr *pyns_type_to_dtype (const struct type *t);

////////////// Main Methods

// pyns_ns_to_np(type: str) -> np.dtype
PyObject *pyns_ns_to_np (PyObject *m, PyObject *arg);

// pyns_open(path: str) -> capsule
PyObject *pyns_open (PyObject *m, PyObject *arg);

// pyns_close(db: capsule) -> None
PyObject *pyns_close (PyObject *m, PyObject *arg);

// pyns_begin(db: capsule) -> capsule
PyObject *pyns_begin (PyObject *m, PyObject *arg);

// pyns_commit(db: capsule, txn: capsule) -> None
PyObject *pyns_commit (PyObject *m, PyObject *args);

// pyns_rollback(db: capsule, txn: capsule) -> None
PyObject *pyns_rollback (PyObject *m, PyObject *args);

// pyns_rollback(db: capsule, txn: capsule) -> None
PyObject *pyns_execute (PyObject *m, PyObject *args);

#endif // NS_PYMODULE_COMMON_H
