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

#include <c_specx.h>
#include "nscore/types.h"

#include <Python.h>
#include <string.h>

// PyObject *Py_UNUSED (m)
//    The _pynumstore module - not used
//
// PyObject *arg
//    One Python Object
//
// PyObject *args
//    Multiple Python Object

// Utils
PyObject *pyns_compile_type (PyObject *Py_UNUSED (m), PyObject *arg);

// Lifecycle
PyObject *pyns_db_open (PyObject *Py_UNUSED (m), PyObject *arg);
PyObject *pyns_db_close (PyObject *Py_UNUSED (m), PyObject *arg);

// Transactions
PyObject *pyns_db_begin (PyObject *Py_UNUSED (m), PyObject *arg);
PyObject *pyns_txn_commit (PyObject *Py_UNUSED (m), PyObject *arg);
PyObject *pyns_txn_rollback (PyObject *Py_UNUSED (m), PyObject *arg);

// Main Methods
PyObject *pyns_var_read (PyObject *Py_UNUSED (m), PyObject *args);
PyObject *pyns_var_insert (PyObject *Py_UNUSED (m), PyObject *args);
PyObject *pyns_var_write (PyObject *Py_UNUSED (m), PyObject *args);
PyObject *pyns_var_len (PyObject *Py_UNUSED (m), PyObject *args);
PyObject *pyns_var_delete (PyObject *Py_UNUSED (m), PyObject *args);
