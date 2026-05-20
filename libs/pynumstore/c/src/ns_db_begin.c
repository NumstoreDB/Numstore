#include "_pynumstore.h"
#include "nscore/compiler.h"
#include "nscore/types.h"
#include "pynumstore.h"

#include <Python.h>
#include <numpy/arrayobject.h>
#include <string.h>

/*
 * db_begin(db: capsule) -> capsule
 */
PyObject *ns_db_begin (PyObject *Py_UNUSED (m), PyObject *arg) {
  if (!_unwrap_db (arg)) { return NULL; }

  /* TODO: smfile_txn_t *txn = smfile_begin(smf); */
  return PyCapsule_New ((void *)(1), TXN_CAPSULE, NULL);
}
