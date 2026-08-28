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

#define PYNUMSTORE_MODULE_MAIN
#include "ns_pynumstore.h"

const char         DB_CAPSULE[]  = "numstore.db";
const char         TXN_CAPSULE[] = "numstore.txn";

char               TXN_CLOSED_SENTINEL;
char               DB_CLOSED_SENTINEL;

static PyMethodDef pynumstore_methods[] = {
    // Utils
    {
        "pyns_ns_to_np",
        pyns_ns_to_np,
        METH_O,
        "pyns_ns_to_np(str) -> np.dtype",
    },

    // Lifecycle
    {
        "pyns_open",
        pyns_open,
        METH_O,
        "db_open(path) -> capsule",
    },
    {
        "pyns_close",
        pyns_close,
        METH_O,
        "db_close(db) -> None",
    },

    // Transactions
    {
        "pyns_begin",
        pyns_begin,
        METH_O,
        "db_begin(db) -> capsule",
    },
    {
        "pyns_commit",
        pyns_commit,
        METH_VARARGS,
        "txn_commit(db, txn) -> None",
    },
    {
        "pyns_rollback",
        pyns_rollback,
        METH_VARARGS,
        "txn_rollback(db, txn) -> None",
    },

    // Variable management
    {
        "pyns_execute",
        pyns_execute,
        METH_VARARGS,
        "var_create(db, txn_or_none, name, type_str) -> None",
    },

    // End
    {NULL, NULL, 0, NULL},
};

static PyModuleDef pynumstore_module = {
    .m_base    = PyModuleDef_HEAD_INIT,
    .m_name    = "_pynumstore",
    .m_doc     = "Thin C wrapper around smfile operations for the pynumstore package.",
    .m_size    = -1,
    .m_methods = pynumstore_methods,
};

PyMODINIT_FUNC PyInit__pynumstore (void);

PyMODINIT_FUNC
PyInit__pynumstore (void)
{
  import_array ();
  return PyModule_Create (&pynumstore_module);
}
