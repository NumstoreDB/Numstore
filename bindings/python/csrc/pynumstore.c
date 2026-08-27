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

// Python
#include "core/ns_alloc.h"

#include <numpy/ndarrayobject.h>
#define PY_SSIZE_T_CLEAN
#include <Python.h>

// Numpy
#define PY_ARRAY_UNIQUE_SYMBOL _NUMSTORE_ARRAY_API
#define NPY_NO_DEPRECATED_API  NPY_2_0_API_VERSION
#include <numpy/arrayobject.h>

// System
#include <string.h>

// Numstore
#include "core/ns_csx_assert.h"
#include "nscore/compiler/ns_compiler.h"
#include "nscore/nsdb/ns_nsdb_execute.h"
#include "nscore/types/ns_types.h"
#include "numstore.h"

// Forward declarations
PyObject *pyns_ns_to_np (PyObject *Py_UNUSED (m), PyObject *arg);
PyObject *pyns_open (PyObject *Py_UNUSED (m), PyObject *arg);
PyObject *pyns_close (PyObject *Py_UNUSED (m), PyObject *arg);
PyObject *pyns_begin (PyObject *Py_UNUSED (m), PyObject *arg);
PyObject *pyns_commit (PyObject *Py_UNUSED (m), PyObject *arg);
PyObject *pyns_rollback (PyObject *Py_UNUSED (m), PyObject *arg);
PyObject *pyns_execute (PyObject *Py_UNUSED (m), PyObject *args);

static const char DB_CAPSULE[]  = "numstore.db";
static const char TXN_CAPSULE[] = "numstore.txn";

// PyCapsule_SetPointer() forbids NULL, so a committed/rolled-back txn capsule
// (or a closed db capsule, see DB_CLOSED_SENTINEL) is repointed here instead
// of being nulled out, to mark it as spent.
static char       TXN_CLOSED_SENTINEL;
static char       DB_CLOSED_SENTINEL;

#define ENSURE_CAPSULE(obj, ret)                                           \
  do {                                                                     \
    if (!PyCapsule_CheckExact (obj)) {                                     \
      PyErr_SetString (PyExc_TypeError, "expected nstxn capsule or None"); \
      return ret;                                                          \
    }                                                                      \
  }                                                                        \
  while (0)

static inline nsdb_t *
_unwrap_db (PyObject *capsule)
{
  ENSURE_CAPSULE (capsule, NULL);
  void *ptr = PyCapsule_GetPointer (capsule, DB_CAPSULE);
  if (ptr == NULL) {
    return NULL; // error already set by PyCapsule_GetPointer
  }
  if (ptr == &DB_CLOSED_SENTINEL) {
    PyErr_SetString (PyExc_RuntimeError, "database is already closed");
    return NULL;
  }
  return (nsdb_t *)ptr;
}

static inline void
_nspy_release_db (PyObject *capsule)
{
  ENSURE_CAPSULE (capsule, );
  nsdb_t *ns = _unwrap_db (capsule);
  ASSERT (ns);
  nsdb_close (ns);
}

static inline ns_txn_t *
_unwrap_txn (PyObject *txn_capsule)
{
  ENSURE_CAPSULE (txn_capsule, NULL);
  void *ptr = PyCapsule_GetPointer (txn_capsule, TXN_CAPSULE);
  if (ptr == NULL) {
    return NULL; // error already set by PyCapsule_GetPointer
  }
  if (ptr == &TXN_CLOSED_SENTINEL) {
    PyErr_SetString (PyExc_RuntimeError, "transaction was already committed or rolled back");
    return NULL;
  }
  return (ns_txn_t *)ptr;
}

// Sets a Python RuntimeError from the nsdb error string.
static inline void
_pyns_set_error (nsdb_t *ns)
{
  const char *err = nsdb_strerror (ns);
  if (err) {
    PyErr_SetString (PyExc_RuntimeError, err);
  } else {
    PyErr_SetString (PyExc_RuntimeError, "numstore operation failed");
  }
}

static inline Py_ssize_t
elsize (PyArray_Descr *type)
{
#if NPY_FEATURE_VERSION >= NPY_2_0_API_VERSION
  return (type)->elsize;
#else
  return PyDataType_ELSIZE (type);
#endif
}

// Build a complex valued struct
static PyArray_Descr *
build_complex_struct (int component_typenum)
{
  PyArray_Descr *comp   = NULL; // dtype for the single component (e.g. f32)
  PyObject      *fields = NULL; // the list of fields
  PyArray_Descr *out    = NULL;
  PyObject      *name   = NULL;
  PyObject      *tup    = NULL;

  // Internal type
  comp                  = PyArray_DescrFromType (component_typenum);
  fields                = PyList_New (2);

  if (comp == NULL || fields == NULL) {
    goto fail;
  }

  // Names
  static const char *names[2] = {"re", "im"};
  for (int i = 0; i < 2; i++) {
    // Generate python string
    name = PyUnicode_FromString (names[i]);
    tup  = PyTuple_New (2);

    if (name == NULL || tup == NULL) {
      goto fail;
    }

    // increase ref so that SET_ITEM doesn't set count to 0
    Py_INCREF (comp);

    // All these Steal
    PyTuple_SET_ITEM (tup, 0, name);
    PyTuple_SET_ITEM (tup, 1, (PyObject *)comp);
    PyList_SET_ITEM (fields, i, tup);

    name = NULL;
    tup  = NULL;
  }

  if (PyArray_DescrConverter (fields, &out) != NPY_SUCCEED) {
    goto fail;
  }

  Py_DECREF (comp);
  Py_DECREF (fields);

  return out;

fail:
  Py_XDECREF (name);
  Py_XDECREF (tup);
  Py_XDECREF (comp);
  Py_XDECREF (fields);
  return NULL;
}

/**
 * Forward Declaration for recursion
 */
static PyArray_Descr *pyns_type_to_dtype (const struct type *t);

static PyArray_Descr *
primitive_to_dtype (enum prim_t p)
{
  int typenum;
  switch (p) {
    case U8: typenum = NPY_UINT8; break;
    case U16: typenum = NPY_UINT16; break;
    case U32: typenum = NPY_UINT32; break;
    case U64: typenum = NPY_UINT64; break;
    case I8: typenum = NPY_INT8; break;
    case I16: typenum = NPY_INT16; break;
    case I32: typenum = NPY_INT32; break;
    case I64: typenum = NPY_INT64; break;
    case F16: typenum = NPY_FLOAT16; break;
    case F32: typenum = NPY_FLOAT32; break;
    case F64: typenum = NPY_FLOAT64; break;
    case F128: typenum = NPY_LONGDOUBLE; break;
    case CF64: typenum = NPY_COMPLEX64; break;
    case CF128: typenum = NPY_COMPLEX128; break;
    case CF256: typenum = NPY_CLONGDOUBLE; break;
    case CF32: return build_complex_struct (NPY_FLOAT16);
    case CI16: return build_complex_struct (NPY_INT8);
    case CI32: return build_complex_struct (NPY_INT16);
    case CI64: return build_complex_struct (NPY_INT32);
    case CI128: return build_complex_struct (NPY_INT64);
    case CU16: return build_complex_struct (NPY_UINT8);
    case CU32: return build_complex_struct (NPY_UINT16);
    case CU64: return build_complex_struct (NPY_UINT32);
    case CU128: return build_complex_struct (NPY_UINT64);
    default: PyErr_Format (PyExc_ValueError, "unknown numstore primitive: %d", (int)p); return NULL;
  }

  PyArray_Descr *d = PyArray_DescrFromType (typenum);
  if (d == NULL) {
    return NULL;
  }

  return d;
}

static PyArray_Descr *
struct_to_dtype (const struct struct_t *st)
{
  ASSERT (st->len > 0);
  PyObject      *fields = NULL; // List of fields
  PyObject      *name   = NULL; // name of each field
  PyArray_Descr *sub    = NULL; // sub type of each field
  PyObject      *tup    = NULL; // the wrapper of (name, sub)
  PyArray_Descr *out    = NULL; // The result

  fields                = PyList_New (st->len);
  if (fields == NULL) {
    goto fail;
  }

  for (u16 i = 0; i < st->len; i++) {
    name = PyUnicode_FromStringAndSize (st->keys[i].data, (Py_ssize_t)st->keys[i].len);
    sub  = pyns_type_to_dtype (st->types[i]);
    tup  = PyTuple_New (2);

    if (name == NULL || sub == NULL || tup == NULL) {
      goto fail;
    }

    PyTuple_SET_ITEM (tup, 0, name);
    name = NULL;
    PyTuple_SET_ITEM (tup, 1, (PyObject *)sub);
    sub = NULL;
    PyList_SET_ITEM (fields, i, tup);
    tup = NULL;
  }

  if (PyArray_DescrConverter (fields, &out) != NPY_SUCCEED) {
    goto fail;
  }

  Py_DECREF (fields);

  return out;

fail:
  Py_XDECREF (name);
  Py_XDECREF (sub);
  Py_XDECREF (tup);
  Py_XDECREF (fields);
  return NULL;
}

static PyArray_Descr *
union_to_dtype (const struct union_t *un)
{
  ASSERT (un->len > 0);
  PyObject      *names    = NULL; // names
  PyObject      *formats  = NULL;
  PyObject      *offsets  = NULL;
  PyObject      *name     = NULL;
  PyArray_Descr *sub      = NULL;
  PyObject      *off      = NULL;
  PyObject      *itemsize = NULL;
  PyObject      *spec     = NULL;
  PyArray_Descr *out      = NULL;

  names                   = PyList_New (un->len);
  formats                 = PyList_New (un->len);
  offsets                 = PyList_New (un->len);
  if (names == NULL || formats == NULL || offsets == NULL) {
    goto fail;
  }

  Py_ssize_t max_size = 0;
  for (u16 i = 0; i < un->len; i++) {
    name = PyUnicode_FromStringAndSize (un->keys[i].data, (Py_ssize_t)un->keys[i].len);
    sub  = pyns_type_to_dtype (un->types[i]);
    off  = PyLong_FromLong (0);

    if (name == NULL || sub == NULL || off == NULL) {
      goto fail;
    }

    Py_ssize_t isize = elsize (sub);

    if (isize > max_size) {
      max_size = isize;
    }

    PyList_SET_ITEM (names, i, name);
    name = NULL;
    PyList_SET_ITEM (formats, i, (PyObject *)sub);
    sub = NULL;
    PyList_SET_ITEM (offsets, i, off);
    off = NULL;
  }

  itemsize = PyLong_FromSsize_t (max_size);
  spec     = PyDict_New ();

  if (itemsize == NULL || spec == NULL) {
    goto fail;
  }

  if (PyDict_SetItemString (spec, "names", names) != 0) {
    goto fail;
  }
  if (PyDict_SetItemString (spec, "formats", formats) != 0) {
    goto fail;
  }
  if (PyDict_SetItemString (spec, "offsets", offsets) != 0) {
    goto fail;
  }
  if (PyDict_SetItemString (spec, "itemsize", itemsize) != 0) {
    goto fail;
  }

  Py_DECREF (names);
  names = NULL;
  Py_DECREF (formats);
  formats = NULL;
  Py_DECREF (offsets);
  offsets = NULL;
  Py_DECREF (itemsize);
  itemsize = NULL;

  if (PyArray_DescrConverter (spec, &out) != NPY_SUCCEED) {
    goto fail;
  }

  Py_DECREF (spec);
  return out;

fail:
  Py_XDECREF (name);
  Py_XDECREF (sub);
  Py_XDECREF (off);
  Py_XDECREF (names);
  Py_XDECREF (formats);
  Py_XDECREF (offsets);
  Py_XDECREF (itemsize);
  Py_XDECREF (spec);
  return NULL;
}

static PyArray_Descr *
sarray_to_dtype (const struct sarray_t *sa)
{
  ASSERT (sa->rank > 0);
  PyArray_Descr *sub   = NULL;
  PyObject      *shape = NULL;
  PyObject      *d     = NULL;
  PyObject      *spec  = NULL;
  PyArray_Descr *out   = NULL;

  sub                  = pyns_type_to_dtype (sa->t);
  shape                = PyTuple_New (sa->rank);

  if (sub == NULL || shape == NULL) {
    goto fail;
  }

  for (u16 i = 0; i < sa->rank; i++) {
    d = PyLong_FromUnsignedLong ((unsigned long)sa->dims[i]);

    if (d == NULL) {
      goto fail;
    }

    PyTuple_SET_ITEM (shape, i, d);
    d = NULL;
  }

  spec = PyTuple_New (2);
  if (spec == NULL) {
    goto fail;
  }

  PyTuple_SET_ITEM (spec, 0, (PyObject *)sub);
  sub = NULL;
  PyTuple_SET_ITEM (spec, 1, shape);
  shape = NULL;

  if (PyArray_DescrConverter (spec, &out) != NPY_SUCCEED) {
    goto fail;
  }

  Py_DECREF (spec);
  return out;

fail:
  Py_XDECREF (d);
  Py_XDECREF (sub);
  Py_XDECREF (shape);
  Py_XDECREF (spec);
  return NULL;
}

PyArray_Descr *
pyns_type_to_dtype (const struct type *t)
{
  ASSERT (t);

  switch (t->type) {
    case T_PRIM: return primitive_to_dtype (t->p);
    case T_STRUCT: return struct_to_dtype (&t->st);
    case T_UNION: return union_to_dtype (&t->un);
    case T_SARRAY: return sarray_to_dtype (&t->sa);
    default: {
      UNREACHABLE ();
    }
  }
}

PyObject *
pyns_ns_to_np (PyObject *Py_UNUSED (m), PyObject *arg)
{
  ALLOC_INIT (alloc);
  struct type    t;
  error          e   = error_create ();
  PyArray_Descr *ret = NULL;

  // Extract utf8 string from argument
  const char    *src = PyUnicode_AsUTF8 (arg);
  if (!src) {
    return NULL;
  }

  // compile the type string
  if (compile_type (&t, src, &alloc, &e)) {
    PyErr_Format (PyExc_ValueError, "Error: %.*s", e.cmlen, e.cause_msg);
    goto theend;
  }

  // Convert it to a numpy type
  ret = pyns_type_to_dtype (&t);

theend:
  ALLOC_CLOSE (alloc);
  return (PyObject *)ret;
}

PyObject *
pyns_begin (PyObject *Py_UNUSED (m), PyObject *arg)
{
  // Get the wrapped database
  nsdb_t *ns = _unwrap_db (arg);
  if (!ns) {
    return NULL;
  }

  // BEGIN TXN
  ns_txn_t *txn = nsdb_begin (ns);
  if (!txn) {
    _pyns_set_error (ns);
    return NULL;
  }

  return PyCapsule_New ((void *)txn, TXN_CAPSULE, NULL);
}

PyObject *
pyns_close (PyObject *Py_UNUSED (m), PyObject *arg)
{
  nsdb_t *ns = _unwrap_db (arg);
  if (!ns) {
    return NULL;
  }

  PyCapsule_SetDestructor (arg, NULL);

  err_t ret = nsdb_close (ns);
  if (PyCapsule_SetPointer (arg, &DB_CLOSED_SENTINEL) < 0) {
    PyErr_Clear ();
  }

  if (ret < 0) {
    PyErr_SetString (PyExc_RuntimeError, "Failed to close numstore database");
    return NULL;
  }

  Py_RETURN_NONE;
}

PyObject *
pyns_open (PyObject *Py_UNUSED (m), PyObject *arg)
{
  if (!PyUnicode_Check (arg)) {
    PyErr_SetString (PyExc_TypeError, "path must be str");
    return NULL;
  }

  const char *path = PyUnicode_AsUTF8 (arg);
  if (!path) {
    return NULL;
  }

  nsdb_t *ns = nsdb_open (path);
  if (!ns) {
    PyErr_SetString (PyExc_RuntimeError, "Failed to open numstore database");
    return NULL;
  }

  return PyCapsule_New ((void *)ns, DB_CAPSULE, _nspy_release_db);
}

PyObject *
pyns_commit (PyObject *Py_UNUSED (m), PyObject *args)
{
  PyObject *_db;
  PyObject *_txn;

  /* commit(db, txn) */
  if (!PyArg_ParseTuple (args, "OO", &_db, &_txn)) {
    return NULL;
  }

  nsdb_t *db = _unwrap_db (_db);
  if (db == NULL) {
    return NULL; /* error already set by _unwrap_db */
  }

  ns_txn_t *txn = _unwrap_txn (_txn);
  if (txn == NULL) {
    return NULL; /* error already set by _unwrap_txn */
  }

  if (nsdb_commit (db, txn) < 0) {
    _pyns_set_error (db);
    return NULL;
  }

  // txn is now invalid in memory - mark the capsule so reuse raises cleanly
  // instead of dereferencing a freed pointer
  if (PyCapsule_SetPointer (_txn, &TXN_CLOSED_SENTINEL) < 0) {
    PyErr_Clear ();
  }

  Py_RETURN_NONE;
}

PyObject *
pyns_rollback (PyObject *Py_UNUSED (m), PyObject *args)
{
  PyObject *_db;
  PyObject *_txn;

  /* rollback(db, txn) */
  if (!PyArg_ParseTuple (args, "OO", &_db, &_txn)) {
    return NULL;
  }

  nsdb_t *db = _unwrap_db (_db);
  if (db == NULL) {
    return NULL; /* error already set by _unwrap_db */
  }

  ns_txn_t *txn = _unwrap_txn (_txn);
  if (txn == NULL) {
    return NULL; /* error already set by _unwrap_txn */
  }

  if (nsdb_rollback (db, txn) < 0) {
    _pyns_set_error (db);
    return NULL;
  }

  // txn is now invalid in memory - mark the capsule so reuse raises cleanly
  // instead of dereferencing a freed pointer
  if (PyCapsule_SetPointer (_txn, &TXN_CLOSED_SENTINEL) < 0) {
    PyErr_Clear ();
  }

  Py_RETURN_NONE;
}

// Handles execute(db, txn, query, None) for read/remove queries: allocates a
// numpy array sized to the variable's current full length (a simple, always
// -safe upper bound for anything a read/remove range can produce) and
// returns it trimmed to however many elements the operation actually
// produced, instead of requiring the caller to pre-size a buffer. There's no
// way to ask the Python C API "is this return value about to be assigned or
// discarded" - a function call always returns a value regardless of what the
// caller does with it - so this is driven entirely by whether the caller
// passed a destination buffer.
//
// *handled is set to true if this query type is one we own (read/remove),
// regardless of success; the caller should fall back to its own data=NULL
// handling (which works fine for create/delete/get/exit/help) when false.
static PyObject *
pyns_execute_malloc (nsdb_t *db, ns_txn_t *txn, const char *query_str, bool *handled)
{
  *handled = false;

  ALLOC_INIT (alloc);
  error        e        = error_create ();
  PyObject    *result   = NULL;
  bool         auto_txn = false;

  struct query q;
  if (compile_query (&q, query_str, &alloc, &e)) {
    // Not handled: let the caller's normal nsdb_fexecute() path recompile
    // and fail the same way it would have without this function existing,
    // so a bad query always raises the same RuntimeError regardless of
    // whether `data` happened to be None.
    goto theend;
  }

  if (q.type != QT_READ && q.type != QT_REMOVE) {
    goto theend;
  }
  *handled = true;

  // Holding one transaction across both the size lookup below and the
  // actual read/remove closes the only race in this function: this engine
  // takes an exclusive lock for the lifetime of a transaction (auto or
  // explicit), so nothing else can grow the variable out from under a
  // buffer sized for its old, smaller length in between. If the caller
  // already had a transaction open, their lock already covers both calls.
  if (txn == NULL) {
    txn = nsdb_begin (db);
    if (txn == NULL) {
      _pyns_set_error (db);
      goto theend;
    }
    auto_txn = true;
  }

  struct string    vname = (q.type == QT_READ) ? q.read.name : q.remove.name;

  struct variable *var;
  struct get_query gq = {.name = vname, .if_exists = false};
  if (nsdb_get (db, txn, &gq, &alloc, &var) < 0) {
    _pyns_set_error (db);
    goto theend_rollback;
  }

  t_size         tsize = type_byte_size (var->dtype);
  npy_intp       nelem = (npy_intp)(tsize ? var->nbytes / tsize : 0);

  PyArray_Descr *descr = pyns_type_to_dtype (var->dtype);
  if (descr == NULL) {
    goto theend_rollback;
  }

  // Steals `descr`.
  PyObject *arr = PyArray_SimpleNewFromDescr (1, &nelem, descr);
  if (arr == NULL) {
    goto theend_rollback;
  }

  sb_size n =
      nelem == 0
          ? 0
          : nsdb_execute_on_buffer (db, txn, &q, PyArray_BYTES ((PyArrayObject *)arr), &alloc);
  if (n < 0) {
    _pyns_set_error (db);
    Py_DECREF (arr);
    goto theend_rollback;
  }

  npy_intp     new_dims[1] = {n};
  PyArray_Dims newshape    = {new_dims, 1};
  PyObject    *resize_ret  = PyArray_Resize ((PyArrayObject *)arr, &newshape, 0, NPY_ANYORDER);
  if (resize_ret == NULL) {
    Py_DECREF (arr);
    goto theend_rollback;
  }
  Py_DECREF (resize_ret);

  if (auto_txn && nsdb_commit (db, txn) < 0) {
    _pyns_set_error (db);
    Py_DECREF (arr);
    goto theend;
  }

  result = arr;
  goto theend;

theend_rollback:
  if (auto_txn) {
    nsdb_rollback (db, txn);
  }

theend:
  ALLOC_CLOSE (alloc);
  return result;
}

PyObject *
pyns_execute (PyObject *Py_UNUSED (m), PyObject *args)
{
  PyObject *_db;
  PyObject *_txn;
  char     *query;
  PyObject *data_obj;

  // Parse arguments
  //    execute(db, txn | None, query, data | None)
  if (!PyArg_ParseTuple (args, "OOsO", &_db, &_txn, &query, &data_obj)) {
    return NULL;
  }

  // The underlying data buffer of data if it's present
  PyArrayObject *contig = NULL;

  if (data_obj != Py_None) {
    // Check that it's an array
    if (!PyArray_Check (data_obj)) {
      PyErr_SetString (PyExc_TypeError, "data must be a numpy array or None");
      return NULL;
    }

    // Get the data backing the array
    contig = (PyArrayObject *)
        PyArray_FROM_OTF (data_obj, NPY_NOTYPE, NPY_ARRAY_IN_ARRAY | NPY_ARRAY_FORCECAST);
    if (contig == NULL) {
      return NULL;
    }
  }

  // Fetch active handle
  nsdb_t *db = _unwrap_db (_db);
  if (db == NULL) {
    Py_XDECREF (contig);
    return NULL;
  }

  ns_txn_t *txn = NULL;
  if (_txn != Py_None) {
    txn = _unwrap_txn (_txn);
    if (txn == NULL) {
      Py_XDECREF (contig);
      return NULL;
    }
  }

  if (contig == NULL && data_obj == Py_None) {
    bool      handled = false;
    PyObject *result  = pyns_execute_malloc (db, txn, query, &handled);
    if (handled) {
      return result;
    }
  }

  char    *bytes  = contig ? PyArray_BYTES (contig) : NULL;
  npy_intp nbytes = contig ? PyArray_NBYTES (contig) : 0;
  (void)nbytes;
  if (contig != NULL && bytes == NULL) {
    PyErr_SetString (PyExc_RuntimeError, "array has no underlying buffer");
    Py_XDECREF (contig);
    return NULL;
  }

  // Execute the query
  sb_size ret = nsdb_fexecute (db, txn, "%s", bytes, query);
  if (ret < 0) {
    _pyns_set_error (db);
    Py_XDECREF (contig);
    return NULL;
  }

  Py_XDECREF (contig);
  return PyLong_FromSsize_t ((Py_ssize_t)ret);
}

/******************************************************************************
 * SECTION: Module Code
 ******************************************************************************/

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
