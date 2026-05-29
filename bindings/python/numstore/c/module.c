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

#define PY_SSIZE_T_CLEAN
#define PY_ARRAY_UNIQUE_SYMBOL _NUMSTORE_ARRAY_API
#define NPY_NO_DEPRECATED_API  NPY_2_0_API_VERSION

#include <numpy/arrayobject.h>
#include "pynumstore.h"


static PyMethodDef numstore_methods[] = {
    // Utils
    {
        "ns_to_np",
        pyns_compile_type,
        METH_O,
        "ns_to_np(str) -> np.dtype",
    },

    // Lifecycle
    {
        "db_open",
        pyns_db_open,
        METH_O,
        "db_open(path) -> capsule",
    },
    {
        "db_close",
        pyns_db_close,
        METH_O,
        "db_close(db) -> None",
    },

    // Transactions
    {
        "db_begin",
        pyns_db_begin,
        METH_O,
        "db_begin(db) -> capsule",
    },
    {
        "txn_commit",
        pyns_txn_commit,
        METH_O,
        "txn_commit(txn) -> None",
    },
    {
        "txn_rollback",
        pyns_txn_rollback,
        METH_O,
        "txn_rollback(txn) -> None",
    },

    // Variable management
    {
        "var_create",
        pyns_var_create,
        METH_VARARGS,
        "var_create(db, txn_or_none, name, type_str) -> None",
    },
    {
        "var_delete",
        pyns_var_delete,
        METH_VARARGS,
        "var_delete(db, txn_or_none, name) -> None",
    },
    {
        "var_len",
        pyns_var_len,
        METH_VARARGS,
        "var_len(db, txn_or_none, var) -> int",
    },

    // Main Methods
    {
        "var_read",
        pyns_var_read,
        METH_VARARGS,
        "var_read(db, txn_or_none, var, key) -> NDArray",
    },
    {
        "var_insert",
        pyns_var_insert,
        METH_VARARGS,
        "var_insert(db, txn_or_none, var, ofst, data) -> None",
    },
    {
        "var_write",
        pyns_var_write,
        METH_VARARGS,
        "var_write(db, txn_or_none, var, key, data) -> None",
    },
    {
        "var_remove",
        pyns_var_remove,
        METH_VARARGS,
        "var_remove(db, txn_or_none, var, key) -> NDArray",
    },
    {
        "var_exists",
        pyns_var_exists,
        METH_VARARGS,
        "var_exists(db, txn_or_none, var) -> bool",
    },

    // End
    {NULL, NULL, 0, NULL},
};

static PyModuleDef numstore_module = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "_numstore",
    .m_doc =
        "Thin C wrapper around smfile operations for the numstore package.",
    .m_size    = -1,
    .m_methods = numstore_methods,
};

PyMODINIT_FUNC PyInit__numstore (void);

PyMODINIT_FUNC
PyInit__numstore (void)
{
  import_array ();
  return PyModule_Create (&numstore_module);
}
