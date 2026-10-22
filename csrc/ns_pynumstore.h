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

#include <numpy/arrayobject.h>
#include <string.h>

// Types
PyObject *pyns_ns_to_np (PyObject *m, PyObject *arg);

// Lifecycle
PyObject *pyns_open (PyObject *m, PyObject *arg);
PyObject *pyns_close (PyObject *m, PyObject *arg);

// Transaction control
PyObject *pyns_begin (PyObject *m, PyObject *arg);
PyObject *pyns_commit (PyObject *m, PyObject *args);
PyObject *pyns_rollback (PyObject *m, PyObject *args);

// Execution
PyObject *pyns_execute (PyObject *m, PyObject *args);
PyObject *pyns_plan_execute (PyObject *m, PyObject *args);

PyObject *pyns_get_var (PyObject *m, PyObject *args);
PyObject *pyns_plan_get_var (PyObject *m, PyObject *args);

PyObject *pyns_read (PyObject *m, PyObject *args);
PyObject *pyns_plan_read (PyObject *m, PyObject *args);

PyObject *pyns_malloc (PyObject *m, PyObject *args);
PyObject *pyns_plan_malloc (PyObject *m, PyObject *args);

PyObject *pyns_write (PyObject *m, PyObject *args);
PyObject *pyns_plan_write (PyObject *m, PyObject *args);

// Plans
PyObject *pyns_plan_type (PyObject *m, PyObject *arg);

// Variables
PyObject *pyns_var_name (PyObject *m, PyObject *arg);
PyObject *pyns_var_length (PyObject *m, PyObject *arg);
PyObject *pyns_var_tsize (PyObject *m, PyObject *arg);
PyObject *pyns_var_type (PyObject *m, PyObject *arg);

#endif // NS_PYMODULE_COMMON_H
