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

#include "cpynumstore.h"

static const char DB_CAPSULE[]  = "numstore.db";
static const char TXN_CAPSULE[] = "numstore.txn";

#define FAKE_PTR ((void *)1)

/* ------------------------------------------------------------------ */
/* Capsule helpers                                                     */
/* ------------------------------------------------------------------ */

static smfile_t *_unwrap_db (PyObject *db) {
  smfile_t *p = (smfile_t *)PyCapsule_GetPointer (db, DB_CAPSULE);
  /* PyCapsule_GetPointer sets TypeError on mismatch */
  return p;
}

/*
 * Returns the txn pointer, or NULL with no error set when txn_or_none
 * is None (caller should treat NULL-without-error as "no transaction").
 */
static smfile_txn_t *_unwrap_txn (PyObject *txn_or_none) {
  if (txn_or_none == Py_None) { return NULL; }
  smfile_txn_t *p = (smfile_txn_t *)PyCapsule_GetPointer (txn_or_none, TXN_CAPSULE);
  return p; /* NULL + error set on mismatch */
}

/* ------------------------------------------------------------------ */
/* Database lifecycle                                                  */
/* ------------------------------------------------------------------ */

/*
 * db_open(path: str) -> capsule
 */
static PyObject *ns_db_open (PyObject *Py_UNUSED (m), PyObject *arg) {
  if (!PyUnicode_Check (arg)) {
    PyErr_SetString (PyExc_TypeError, "path must be str");
    return NULL;
  }
  /* TODO: smfile_t *smf = smfile_open(PyUnicode_AsUTF8(arg)); */
  return PyCapsule_New (FAKE_PTR, DB_CAPSULE, NULL);
}

/*
 * db_close(db: capsule) -> None
 */
static PyObject *ns_db_close (PyObject *Py_UNUSED (m), PyObject *arg) {
  if (!_unwrap_db (arg)) { return NULL; }
  /* TODO: smfile_close(smf); */
  Py_RETURN_NONE;
}

/* ------------------------------------------------------------------ */
/* Transaction lifecycle                                               */
/* ------------------------------------------------------------------ */

/*
 * db_begin(db: capsule) -> capsule
 */
static PyObject *ns_db_begin (PyObject *Py_UNUSED (m), PyObject *arg) {
  if (!_unwrap_db (arg)) { return NULL; }
  /* TODO: smfile_txn_t *txn = smfile_begin(smf); */
  return PyCapsule_New (FAKE_PTR, TXN_CAPSULE, NULL);
}

/*
 * txn_commit(txn: capsule) -> None
 */
static PyObject *ns_txn_commit (PyObject *Py_UNUSED (m), PyObject *arg) {
  if (!PyCapsule_GetPointer (arg, TXN_CAPSULE)) { return NULL; }
  /* TODO: smfile_commit(txn); */
  Py_RETURN_NONE;
}

/*
 * txn_rollback(txn: capsule) -> None
 */
static PyObject *ns_txn_rollback (PyObject *Py_UNUSED (m), PyObject *arg) {
  if (!PyCapsule_GetPointer (arg, TXN_CAPSULE)) { return NULL; }
  /* TODO: smfile_rollback(txn); */
  Py_RETURN_NONE;
}

/* ------------------------------------------------------------------ */
/* Variable I/O                                                        */
/*                                                                     */
/* All functions take (db, txn_or_none, var_id, ...).                 */
/* txn_or_none is either the active transaction capsule or None.      */
/* ------------------------------------------------------------------ */

/*
 * var_read(db, txn_or_none, var_id: int, key: int) -> bytes
 */
static PyObject *ns_var_read (PyObject *Py_UNUSED (m), PyObject *args) {
  PyObject *db, *txn_or_none;
  long long var_id, key;
  if (!PyArg_ParseTuple (args, "OOLL", &db, &txn_or_none, &var_id, &key)) { return NULL; }
  smfile_t *smf = _unwrap_db (db);
  if (!smf) { return NULL; }
  smfile_txn_t *txn = _unwrap_txn (txn_or_none);
  if (!txn && PyErr_Occurred ()) { return NULL; }
  /* TODO: smfile_read(smf, txn, var_id, key, &buf, &len); */
  static const char zeros[8] = {0};
  return PyBytes_FromStringAndSize (zeros, sizeof zeros);
}

/*
 * var_insert(db, txn_or_none, var_id: int, key: int, data: bytes) -> None
 *
 * Create a new entry at (var_id, key).
 */
static PyObject *ns_var_insert (PyObject *Py_UNUSED (m), PyObject *args) {
  PyObject *db, *txn_or_none;
  long long var_id, key;
  Py_buffer data;
  if (!PyArg_ParseTuple (args, "OOLLy*", &db, &txn_or_none, &var_id, &key, &data)) { return NULL; }
  smfile_t *smf = _unwrap_db (db);
  if (!smf) {
    PyBuffer_Release (&data);
    return NULL;
  }
  smfile_txn_t *txn = _unwrap_txn (txn_or_none);
  if (!txn && PyErr_Occurred ()) {
    PyBuffer_Release (&data);
    return NULL;
  }
  /* TODO: smfile_insert(smf, txn, var_id, key, data.buf, data.len); */
  PyBuffer_Release (&data);
  Py_RETURN_NONE;
}

/*
 * var_write(db, txn_or_none, var_id: int, key: int, data: bytes) -> None
 *
 * Overwrite an existing entry at (var_id, key).
 */
static PyObject *ns_var_write (PyObject *Py_UNUSED (m), PyObject *args) {
  PyObject *db, *txn_or_none;
  long long var_id, key;
  Py_buffer data;
  if (!PyArg_ParseTuple (args, "OOLLy*", &db, &txn_or_none, &var_id, &key, &data)) { return NULL; }
  smfile_t *smf = _unwrap_db (db);
  if (!smf) {
    PyBuffer_Release (&data);
    return NULL;
  }
  smfile_txn_t *txn = _unwrap_txn (txn_or_none);
  if (!txn && PyErr_Occurred ()) {
    PyBuffer_Release (&data);
    return NULL;
  }
  /* TODO: smfile_write(smf, txn, var_id, key, data.buf, data.len); */
  PyBuffer_Release (&data);
  Py_RETURN_NONE;
}

/*
 * var_len(db, txn_or_none, var_id: int) -> int
 */
static PyObject *ns_var_len (PyObject *Py_UNUSED (m), PyObject *args) {
  PyObject *db, *txn_or_none;
  long long var_id;
  if (!PyArg_ParseTuple (args, "OOL", &db, &txn_or_none, &var_id)) { return NULL; }
  smfile_t *smf = _unwrap_db (db);
  if (!smf) { return NULL; }
  smfile_txn_t *txn = _unwrap_txn (txn_or_none);
  if (!txn && PyErr_Occurred ()) { return NULL; }
  /* TODO: return PyLong_FromSsize_t(smfile_len(smf, txn, var_id)); */
  return PyLong_FromLong (16);
}

/*
 * var_delete(db, txn_or_none, var_id: int, key: int) -> None
 */
static PyObject *ns_var_delete (PyObject *Py_UNUSED (m), PyObject *args) {
  PyObject *db, *txn_or_none;
  long long var_id, key;
  if (!PyArg_ParseTuple (args, "OOLL", &db, &txn_or_none, &var_id, &key)) { return NULL; }
  smfile_t *smf = _unwrap_db (db);
  if (!smf) { return NULL; }
  smfile_txn_t *txn = _unwrap_txn (txn_or_none);
  if (!txn && PyErr_Occurred ()) { return NULL; }
  /* TODO: smfile_remove(smf, txn, var_id, key); */
  Py_RETURN_NONE;
}

/* ------------------------------------------------------------------ */
/* Module                                                              */
/* ------------------------------------------------------------------ */

static PyMethodDef ns_methods[] = {
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
        ns_var_delete,
        METH_VARARGS,
        "var_delete(db, txn_or_none, var_id, key) -> None",
    },

    {NULL, NULL, 0, NULL},
};

static PyModuleDef numstore_moduledef = {
    .m_base    = PyModuleDef_HEAD_INIT,
    .m_name    = "_numstore",
    .m_doc     = "Thin C wrapper around smfile operations for the numstore package.",
    .m_size    = -1,
    .m_methods = ns_methods,
};

PyMODINIT_FUNC PyInit__numstore (void) {
  import_array (); /* initialise NumPy C-API; returns NULL on failure */

  PyObject *m = PyModule_Create (&numstore_moduledef);
  if (!m) { return NULL; }

  /* Register dtype serialisation helpers from numstore_dtype.c */
  for (PyMethodDef *def = ns_dtype_methods; def->ml_name; def++) {
    PyObject *fn = PyCFunction_NewEx (def, NULL, NULL);
    if (!fn || PyModule_AddObject (m, def->ml_name, fn) < 0) {
      Py_XDECREF (fn);
      Py_DECREF (m);
      return NULL;
    }
  }

  return m;
}
