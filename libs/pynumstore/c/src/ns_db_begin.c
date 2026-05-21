#include "_pynumstore.h"
#include "pynumstore.h"

#include <Python.h>
#include <string.h>

PyObject *ns_db_begin (PyObject *Py_UNUSED (m), PyObject *arg) {
  nsdb_t *db = _unwrap_db (arg);
  if (!db) { return NULL; }

  nsdb_t *ctx = nsdb_new_context (db);
  if (!ctx) {
    PyErr_SetString (PyExc_RuntimeError, "nsdb_new_context failed");
    return NULL;
  }

  if (nsdb_begin (ctx) < 0) {
    PyErr_SetString (PyExc_RuntimeError, "nsdb_begin failed");
    nsdb_close (ctx);
    return NULL;
  }

  return PyCapsule_New ((void *)ctx, TXN_CAPSULE, _txn_destructor);
}
