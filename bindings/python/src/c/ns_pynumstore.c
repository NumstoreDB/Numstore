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

#include "core/ns_csx_assert.h"
#define PYNUMSTORE_MODULE_MAIN
#include "ns_pynumstore.h"

nsdb_t *
_unwrap_db (PyObject *capsule)
{
  // Ensure that the object is a capsule, or else return NULL
  if (!PyCapsule_CheckExact (capsule)) {
    PyErr_SetString (PyExc_TypeError, "expected nsdb capsule or None");
    return NULL;
  }

  // Get the database pointer from the capsule
  void *ptr = PyCapsule_GetPointer (capsule, DB_CAPSULE);
  if (ptr == NULL) {
    return NULL; // error already set by PyCapsule_GetPointer
  }

  // Check if the database is closed
  if (ptr == &DB_CLOSED_SENTINEL) {
    PyErr_SetString (PyExc_RuntimeError, "database is already closed");
    return NULL;
  }

  // Return the database
  return (nsdb_t *)ptr;
}

txn_t *
_unwrap_txn (PyObject *txn_capsule)
{
  // Ensure that the object is a capsule, or else return NULL
  if (!PyCapsule_CheckExact (txn_capsule)) {
    PyErr_SetString (PyExc_TypeError, "expected nstxn capsule or None");
    return NULL;
  }

  // Get the transaction pointer from the capsule
  void *ptr = PyCapsule_GetPointer (txn_capsule, TXN_CAPSULE);
  if (ptr == NULL) {
    return NULL; // error already set by PyCapsule_GetPointer
  }

  // Check if the transaction is closed
  if (ptr == &TXN_CLOSED_SENTINEL) {
    PyErr_SetString (PyExc_RuntimeError, "transaction was already committed or rolled back");
    return NULL;
  }

  // Return the transaction object
  return (txn_t *)ptr;
}

void
_nspy_release_db (PyObject *capsule)
{
  // Ensure that the object is a capsule, or else do nothing
  if (!PyCapsule_CheckExact (capsule)) {
    PyErr_SetString (PyExc_TypeError, "expected nsdb capsule or None");
    return;
  }

  // Grab the database handle
  nsdb_t *ns = _unwrap_db (capsule);
  if (ns == NULL) {
    return;
  }

  // Close the database
  if (ns_close (ns) < 0) {
    PyErr_SetString (PyExc_RuntimeError, "Failed to close database");
  }
}

void
_pyns_set_error_from_nsdb (nsdb_t *ns)
{
  // A failing call always leaves an error behind, but never trust that
  // enough to hand PyErr_SetString a NULL - it segfaults on one.
  const char *err = ns_strerror (ns);
  PyErr_SetString (PyExc_RuntimeError, err ? err : "numstore operation failed");
}

void
_pyns_set_error_from_e (PyObject *exc_type, error *e)
{
  ASSERT (e->cause_code < 0);
  // cause_msg is an inline buffer now, so an empty message - not a NULL one -
  // is what "no detail" looks like.
  PyErr_SetString (exc_type, (e->cmlen > 0) ? e->cause_msg : "numstore operation failed");
}

Py_ssize_t
elsize (PyArray_Descr *type)
{
#if NPY_FEATURE_VERSION >= NPY_2_0_API_VERSION
  return (type)->elsize;
#else
  return PyDataType_ELSIZE (type);
#endif
}

const char         DB_CAPSULE[]  = "numstore.db";
const char         TXN_CAPSULE[] = "numstore.txn";
const char         VAR_CAPSULE[] = "numstore.var";

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
        "pyns_execute(db, txn_or_none, query, data) -> int | array | var | "
        "None",
    },

    // Variables
    {
        "pyns_var_length",
        pyns_var_length,
        METH_O,
        "pyns_var_length(var) -> int",
    },
    {
        "pyns_var_tsize",
        pyns_var_tsize,
        METH_O,
        "pyns_var_tsize(var) -> int",
    },
    {
        "pyns_var_type",
        pyns_var_type,
        METH_O,
        "pyns_var_type(var) -> str",
    },
    {
        "pyns_var_name",
        pyns_var_name,
        METH_O,
        "pyns_var_name(var) -> str",
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
