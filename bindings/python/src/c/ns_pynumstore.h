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
#include "nscore/nsdb/ns_nsdb.h"
#include "nscore/types/ns_types.h"
#include "numstore/numstore.h"

#include <numpy/arrayobject.h>
#include <string.h>

// Names of the capsule which stores the name
// for the transaction and database handle
extern const char DB_CAPSULE[];
extern const char TXN_CAPSULE[];
extern const char VAR_CAPSULE[];

struct dims_vec
{
  npy_intp *data;
  int       len;
  int       cap;
};

// When a database or transaction is closed
// - it's value is set to one of these sentinels.
extern char TXN_CLOSED_SENTINEL;
extern char DB_CLOSED_SENTINEL;

////////////// Private methods

// Get numstore objects from python capsules
nsdb_t *_unwrap_db (PyObject *capsule);
txn_t *_unwrap_txn (PyObject *txn_capsule);
nsdb_var_t *_unwrap_var (PyObject *var_capsule);

PyObject *pyns_var_capsule_new (nsdb_var_t *var);
void _nspy_release_db (PyObject *capsule);
void _pyns_set_error_from_e (PyObject *exc_type, error *e);
void _pyns_set_error_from_nsdb (nsdb_t *e);

// Numpy compatible elsize
Py_ssize_t elsize (PyArray_Descr *type);

PyArray_Descr *pyns_type_to_dtype (const struct type *t);
PyArray_Descr *pyns_type_to_dtype_flatten_sarray (
    const struct type *t,
    b_size             top,
    struct dims_vec   *vec
);

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
PyObject *pyns_execute_data_present (nsdb_t *db, txn_t *txn, char *query, PyObject *data_obj);
PyObject *pyns_execute_data_not_present (nsdb_t *db, txn_t *txn, char *query);

// Variables
//
// pyns_var_*(var: capsule) -> int | str
PyObject *pyns_var_length (PyObject *m, PyObject *arg);
PyObject *pyns_var_tsize (PyObject *m, PyObject *arg);
PyObject *pyns_var_type (PyObject *m, PyObject *arg);
PyObject *pyns_var_name (PyObject *m, PyObject *arg);

// Render a numstore type into a Python str. A variable now hands its type back
// as a `struct type *`, so the sizing pass goes through type_snprintf rather
// than through a size-then-fill accessor on the variable itself.
PyObject *pyns_type_string (struct type *t);

// Dims - just a list of dimensions to construct a shape
void dims_vec_create (struct dims_vec *v);
int dims_vec_append (struct dims_vec *v, npy_intp dim);
int dims_vec_append_many (struct dims_vec *v, const u32 *dims, u32 count);
void dims_vec_free (struct dims_vec *v);

#endif // NS_PYMODULE_COMMON_H
