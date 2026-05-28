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

#pragma once

// Use Py_ssize_t instead of int
#define PY_SSIZE_T_CLEAN
#define PY_ARRAY_UNIQUE_SYMBOL _pynumstore_ARRAY_API
#define NPY_NO_DEPRECATED_API  NPY_2_0_API_VERSION

#include "nscore/types.h"

#include <Python.h>
#include <c_specx.h>
#include <string.h>

/**
 * parse a numstore type string into a numpy dtype
 *
 * METH_O: receives a single str argument.
 *
 * Raises:
 *   TypeError  - failed to compile type or invalid type passed for [arg]
 */
PyObject *pyns_compile_type(PyObject *Py_UNUSED(m), PyObject *arg);

/**
 * convert a numstore type into a numpy dtype
 *
 * @param t  Pointer to an numstore type struct.  Must be a valid pointer
 *           previously obtained from the numstore backend
 *
 * Returns:
 *   New reference to a numpy dtype object.
 *
 * Raises:
 *   Nothing under normal use - the internal type is always valid.
 */
PyObject *pyns_type_to_dtype(const struct type *t);

/**
 * open or create a numstore database.
 *
 * METH_O: receives a single str argument (filesystem path).
 *
 * If a database file exists at `path` it is opened; otherwise a new one is
 * created.  The returned capsule is the database handle and must be closed
 * with pyns_db_close when no longer needed.
 *
 * Returns:
 *   New reference to an opaque Database capsule.
 *
 * Raises:
 *   RuntimeError      - Something went wrong opening the database
 */
PyObject *pyns_db_open(PyObject *Py_UNUSED(m), PyObject *arg);

/**
 * close a database and release all resources.
 *
 * METH_O: receives the Database capsule returned by pyns_db_open.
 *
 * After this call the handle is invalid.  Any Variable objects that were
 * obtained from this database must not be used afterwards.
 *
 * Returns:
 *   Py_None on success.
 *
 * Raises:
 *   RuntimeError - one or more transactions are still open.  Commit or
 *                  roll back all transactions before closing.
 */
PyObject *pyns_db_close(PyObject *Py_UNUSED(m), PyObject *arg);

/**
 * pyns_db_begin - begin a new read-write transaction.
 *
 * METH_O: receives the Database capsule.
 *
 * All writes made through the returned Transaction handle are invisible to
 * other readers until pyns_txn_commit is called.  Only one transaction per
 * database handle is supported at a time.
 *
 * Returns:
 *   New reference to an opaque Transaction capsule.
 *
 * Raises:
 *   RuntimeError - the database is already closed.
 */
PyObject *pyns_db_begin(PyObject *Py_UNUSED(m), PyObject *arg);

/**
 * pyns_txn_commit - commit a transaction, making all writes durable.
 *
 * METH_O: receives the Transaction capsule.
 *
 * The handle is invalidated after this call and must not be reused.
 *
 * Returns:
 *   Py_None on success.
 *
 * Raises:
 *   RuntimeError - the transaction has already been committed or rolled back.
 */
PyObject *pyns_txn_commit(PyObject *Py_UNUSED(m), PyObject *arg);

/**
 * pyns_txn_rollback - discard all writes made within a transaction.
 *
 * METH_O: receives the Transaction capsule.
 *
 * The handle is invalidated after this call and must not be reused.
 *
 * Returns:
 *   Py_None on success.
 *
 * Raises:
 *   RuntimeError - the transaction has already been committed or rolled back.
 */
PyObject *pyns_txn_rollback(PyObject *Py_UNUSED(m), PyObject *arg);


/* -------------------------------------------------------------------------
 * Variable management
 *
 * All variable functions take (db, txn, var) as their first three arguments:
 *
 *   db   - Database capsule (required).
 *   txn  - Transaction capsule, or Py_None for auto-commit / snapshot reads.
 *   var  - str: the variable name.
 * ------------------------------------------------------------------------- */

/**
 * pyns_var_create - create a new, empty variable.
 *
 * METH_VARARGS: (Database db, Txn txn, str var, str|dtype dtype)
 *
 * `dtype` may be a numstore short-hand ("f32"), a NumPy-style name
 * ("float32"), or a numpy.dtype instance.  Internally calls compile_type
 * to normalise the descriptor before storing it.
 *
 * Returns:
 *   Py_None on success.
 *
 * Raises:
 *   KeyError  - a variable called `var` already exists in this database.
 *   TypeError - `dtype` is not a str / numpy.dtype, or is not a recognised
 *               type descriptor (propagated from compile_type).
 */
PyObject *pyns_var_create(PyObject *Py_UNUSED(m), PyObject *args);

/**
 * pyns_var_delete - delete a variable and all of its stored data.
 *
 * METH_VARARGS: (Database db, Txn txn, str var)
 *
 * Returns:
 *   Py_None on success.
 *
 * Raises:
 *   KeyError - no variable called `var` exists.
 */
PyObject *pyns_var_delete(PyObject *Py_UNUSED(m), PyObject *args);

/**
 * pyns_var_len - return the number of elements stored in a variable.
 *
 * METH_VARARGS: (Database db, Txn txn, str var)
 *
 * Returns:
 *   Python int >= 0.
 *
 * Raises:
 *   KeyError - no variable called `var` exists.
 */
PyObject *pyns_var_len(PyObject *Py_UNUSED(m), PyObject *args);


/* -------------------------------------------------------------------------
 * Element-level operations
 *
 * All element functions take (db, txn, var, ...) as their leading arguments.
 *
 * key argument semantics
 * ----------------------
 *   int   - addresses a single element; negative indices are NOT supported
 *            (the Python layer resolves negatives before calling into C).
 *   range - a pre-resolved contiguous range of element indices produced by
 *            slice.indices(); step must be 1 (non-unit steps are rejected).
 * ------------------------------------------------------------------------- */

/**
 * pyns_var_read - read one or more elements from a variable.
 *
 * METH_VARARGS: (Database db, Txn txn, str var, int|range key)
 *
 * key: int   → returns a single element: a Python scalar for primitive dtypes,
 *              or a numpy ndarray for multi-dimensional element shapes.
 * key: range → returns a numpy ndarray of len(key) elements, contiguous in
 *              C order, with a dtype matching the variable.
 *
 * Returns:
 *   New reference: scalar or ndarray (see above).
 *
 * Raises:
 *   KeyError   - `var` does not exist.
 *   IndexError - `key` (or any index within a range) is out of bounds.
 *   ValueError - a range with step != 1 is supplied.
 */
PyObject *pyns_var_read(PyObject *Py_UNUSED(m), PyObject *args);

/**
 * pyns_var_insert - insert data into a variable, shifting elements right.
 *
 * METH_VARARGS: (Database db, Txn txn, str var, int ofst, ndarray data)
 *
 * Inserts all elements of `data` starting at index `ofst`.  Elements
 * previously at [ofst, len) are shifted right by len(data).  To append,
 * pass ofst == var_len(db, txn, var).
 *
 * `data` must be a C-contiguous numpy ndarray whose dtype is compatible with
 * the variable's stored dtype (exact match or safely castable).
 *
 * Returns:
 *   Py_None on success.
 *
 * Raises:
 *   KeyError   - `var` does not exist.
 *   IndexError - ofst < 0 or ofst > len(var).
 *   TypeError  - `data` dtype is incompatible with the variable's dtype.
 */
PyObject *pyns_var_insert(PyObject *Py_UNUSED(m), PyObject *args);

/**
 * pyns_var_write - overwrite elements in a variable.
 *
 * METH_VARARGS: (Database db, Txn txn, str var, int|range key, Element|ndarray data)
 *
 * key: int   → `data` is one element (scalar or sub-array matching the
 *              variable's element shape).
 * key: range → `data` is a numpy ndarray of len(key) elements; its dtype
 *              must be compatible with the variable's dtype.
 *
 * Does not change the length of `var`; only replaces existing elements.
 *
 * Returns:
 *   Py_None on success.
 *
 * Raises:
 *   KeyError   - `var` does not exist.
 *   IndexError - `key` (or any index within a range) is out of bounds.
 *   TypeError  - `data` dtype is incompatible with the variable's dtype.
 *   ValueError - a range with step != 1 is supplied.
 */
PyObject *pyns_var_write(PyObject *Py_UNUSED(m), PyObject *args);

/**
 * pyns_var_remove - remove elements from a variable and return them.
 *
 * METH_VARARGS: (Database db, Txn txn, str var, int|range key)
 *
 * Removes the element(s) at `key`, shifts the remainder left, and returns
 * the removed data.
 *
 * key: int   → removes and returns one element (scalar or sub-array).
 * key: range → removes and returns a numpy ndarray of len(key) elements.
 *
 * Returns:
 *   New reference: scalar or ndarray containing the removed elements.
 *
 * Raises:
 *   KeyError   - `var` does not exist.
 *   IndexError - `key` (or any index within a range) is out of bounds.
 *   ValueError - a range with step != 1 is supplied.
 */
PyObject *pyns_var_remove(PyObject *Py_UNUSED(m), PyObject *args);


