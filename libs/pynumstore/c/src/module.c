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

#include "pynumstore.h"

#include <numpy/arrayobject.h>

static PyMethodDef numstore_methods[] = {
    // Utils
    {
        "ns_compile_type",
        ns_compile_type,
        METH_O,
        "ns_compile_type(str) -> np.dtype",
    },

    // Lifecycle
    {
        "db_open",
        ns_db_open,
        METH_O,
        "db_open(path) -> capsule",
    },
    {
        "db_close",
        ns_db_close,
        METH_O,
        "db_close(db) -> None",
    },

    // Transactions
    {
        "db_begin",
        ns_db_begin,
        METH_O,
        "db_begin(db) -> capsule",
    },
    {
        "txn_commit",
        ns_txn_commit,
        METH_O,
        "txn_commit(txn) -> None",
    },
    {
        "txn_rollback",
        ns_txn_rollback,
        METH_O,
        "txn_rollback(txn) -> None",
    },

    // Main Methods
    {
        "var_read",
        ns_var_read,
        METH_VARARGS,
        "var_read(db, txn_or_none, var_id, key) -> bytes",
    },
    {
        "var_insert",
        ns_var_insert,
        METH_VARARGS,
        "var_insert(db, txn_or_none, var_id, key, data) -> None",
    },
    {
        "var_write",
        ns_var_write,
        METH_VARARGS,
        "var_write(db, txn_or_none, var_id, key, data) -> None",
    },
    {
        "var_len",
        ns_var_len,
        METH_VARARGS,
        "var_len(db, txn_or_none, var_id) -> int",
    },
    {
        "var_delete",
        pynumstore_var_delete,
        METH_VARARGS,
        "var_delete(db, txn_or_none, var_id, key) -> None",
    },

    // End
    {NULL, NULL, 0, NULL},
};

static PyModuleDef numstore_module = {
    .m_base    = PyModuleDef_HEAD_INIT,
    .m_name    = "_pynumstore",
    .m_doc     = "Thin C wrapper around smfile operations for the numstore package.",
    .m_size    = -1,
    .m_methods = numstore_methods,
};

PyMODINIT_FUNC PyInit__pynumstore (void);

PyMODINIT_FUNC PyInit__pynumstore (void) {
  import_array ();
  return PyModule_Create (&numstore_module);
}
