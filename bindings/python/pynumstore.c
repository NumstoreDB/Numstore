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
#include <numpy/ndarrayobject.h>

#include "core/ns_alloc.h"
#define PY_SSIZE_T_CLEAN
#include <Python.h>

// Numpy
#define PY_ARRAY_UNIQUE_SYMBOL _NUMSTORE_ARRAY_API
#define NPY_NO_DEPRECATED_API  NPY_2_0_API_VERSION
#include <numpy/arrayobject.h>

// System
#include <string.h>

// Numstore
#include "nscore/compiler/ns_compiler.h"
#include "numstore.h"
#include "nscore/types/ns_types.h"
#include "core/ns_csx_assert.h"

/******************************************************************************
 * SECTION: Forward Declaration
 ******************************************************************************/

PyObject *pyns_ns_to_np (PyObject *Py_UNUSED (m), PyObject *arg);
PyObject *pyns_open (PyObject *Py_UNUSED (m), PyObject *arg);
PyObject *pyns_close (PyObject *Py_UNUSED (m), PyObject *arg);
PyObject *pyns_begin (PyObject *Py_UNUSED (m), PyObject *arg);
PyObject *pyns_commit (PyObject *Py_UNUSED (m), PyObject *arg);
PyObject *pyns_rollback (PyObject *Py_UNUSED (m), PyObject *arg);
PyObject *pyns_execute (PyObject *Py_UNUSED (m), PyObject *args);

static const char DB_CAPSULE[]  = "numstore.db";
static const char TXN_CAPSULE[] = "numstore.txn";

/******************************************************************************
 * SECTION: Utils
 ******************************************************************************/

static inline PyObject* 
_verify_capsule(PyObject* obj) {
  if (!PyCapsule_CheckExact(obj)) {
      PyErr_SetString(PyExc_TypeError, "expected nstxn capsule or None");
      return NULL;
  }
  return obj;
}

// Get the underlying database object from the capsule
static inline nsdb_t *
_unwrap_db (PyObject *capsule)
{
  return (nsdb_t *)PyCapsule_GetPointer (capsule, DB_CAPSULE);
}

// Release a database - stored
static inline void
_nspy_release_db (PyObject *capsule)
{
  nsdb_t *ns = _unwrap_db(capsule);
  ASSERT (ns);
  nsdb_close (ns);
}

// Returns nsdb_t * from txn capsule, or NULL (without setting error) if None.
static inline nsdb_t *
_unwrap_txn (PyObject *txn_capsule)
{
  if (txn_capsule == Py_None)
  {
    return NULL;
  }
  return (nsdb_t *)PyCapsule_GetPointer (txn_capsule, TXN_CAPSULE);
}

// Returns the active nsdb_t *: from txn if present, otherwise from db.
static inline nsdb_t *
_active_ns (PyObject *db_capsule, PyObject *txn_capsule)
{
  if (txn_capsule != Py_None)
  {
    return (nsdb_t *)PyCapsule_GetPointer (txn_capsule, TXN_CAPSULE);
  }
  return (nsdb_t *)PyCapsule_GetPointer (db_capsule, DB_CAPSULE);
}

// Sets a Python RuntimeError from the nsdb error string.
static inline void
_pyns_set_error (nsdb_t *ns)
{
  const char *err = nsdb_strerror (ns);
  if (err)
  {
    PyErr_SetString (PyExc_RuntimeError, err);
  }
  else
  {
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

/******************************************************************************
 * SECTION: Utils
 ******************************************************************************/

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
  comp   = PyArray_DescrFromType (component_typenum);
  fields = PyList_New (2);

  if (comp == NULL || fields == NULL)
  {
    goto fail;
  }

  // Names
  static const char *names[2] = {"re", "im"};
  for (int i = 0; i < 2; i++)
  {
    // Generate python string
    name = PyUnicode_FromString (names[i]);
    tup  = PyTuple_New (2);

    if (name == NULL || tup == NULL)
    {
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

  if (PyArray_DescrConverter (fields, &out) != NPY_SUCCEED)
  {
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
  switch (p)
  {
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
    default:
      PyErr_Format (PyExc_ValueError, "unknown numstore primitive: %d", (int)p);
      return NULL;
  }

  PyArray_Descr *d = PyArray_DescrFromType (typenum);
  if (d == NULL)
  {
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

  fields = PyList_New (st->len);
  if (fields == NULL)
  {
    goto fail;
  }

  for (u16 i = 0; i < st->len; i++)
  {
    name = PyUnicode_FromStringAndSize (
        st->keys[i].data,
        (Py_ssize_t)st->keys[i].len
    );
    sub = pyns_type_to_dtype (st->types[i]);
    tup = PyTuple_New (2);

    if (name == NULL || sub == NULL || tup == NULL)
    {
      goto fail;
    }

    PyTuple_SET_ITEM (tup, 0, name);
    name = NULL;
    PyTuple_SET_ITEM (tup, 1, (PyObject *)sub);
    sub = NULL;
    PyList_SET_ITEM (fields, i, tup);
    tup = NULL;
  }

  if (PyArray_DescrConverter (fields, &out) != NPY_SUCCEED)
  {
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

  names   = PyList_New (un->len);
  formats = PyList_New (un->len);
  offsets = PyList_New (un->len);
  if (names == NULL || formats == NULL || offsets == NULL)
  {
    goto fail;
  }

  Py_ssize_t max_size = 0;
  for (u16 i = 0; i < un->len; i++)
  {
    name = PyUnicode_FromStringAndSize (
        un->keys[i].data,
        (Py_ssize_t)un->keys[i].len
    );
    sub = pyns_type_to_dtype (un->types[i]);
    off = PyLong_FromLong (0);

    if (name == NULL || sub == NULL || off == NULL)
    {
      goto fail;
    }

    Py_ssize_t isize = elsize (sub);

    if (isize > max_size)
    {
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

  if (itemsize == NULL || spec == NULL)
  {
    goto fail;
  }

  if (PyDict_SetItemString (spec, "names", names) != 0)
  {
    goto fail;
  }
  if (PyDict_SetItemString (spec, "formats", formats) != 0)
  {
    goto fail;
  }
  if (PyDict_SetItemString (spec, "offsets", offsets) != 0)
  {
    goto fail;
  }
  if (PyDict_SetItemString (spec, "itemsize", itemsize) != 0)
  {
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

  if (PyArray_DescrConverter (spec, &out) != NPY_SUCCEED)
  {
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

  sub   = pyns_type_to_dtype (sa->t);
  shape = PyTuple_New (sa->rank);

  if (sub == NULL || shape == NULL)
  {
    goto fail;
  }

  for (u16 i = 0; i < sa->rank; i++)
  {
    d = PyLong_FromUnsignedLong ((unsigned long)sa->dims[i]);

    if (d == NULL)
    {
      goto fail;
    }

    PyTuple_SET_ITEM (shape, i, d);
    d = NULL;
  }

  spec = PyTuple_New (2);
  if (spec == NULL)
  {
    goto fail;
  }

  PyTuple_SET_ITEM (spec, 0, (PyObject *)sub);
  sub = NULL;
  PyTuple_SET_ITEM (spec, 1, shape);
  shape = NULL;

  if (PyArray_DescrConverter (spec, &out) != NPY_SUCCEED)
  {
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

  switch (t->type)
  {
    case T_PRIM: return primitive_to_dtype (t->p);
    case T_STRUCT: return struct_to_dtype (&t->st);
    case T_UNION: return union_to_dtype (&t->un);
    case T_SARRAY: return sarray_to_dtype (&t->sa);
    default:
    {
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
  const char *src = PyUnicode_AsUTF8 (arg);
  if (!src)
  {
    return NULL;
  }

  // compile the type string
  if (compile_type (&t, src, &alloc, &e))
  {
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
  if (!ns)
  {
    return NULL;
  }

  // BEGIN TXN
  if (nsdb_begin (ns) < 0)
  {
    _pyns_set_error (ns);
    return NULL;
  }

  return PyCapsule_New ((void *)ns, TXN_CAPSULE, NULL);
}

PyObject *
pyns_close (PyObject *Py_UNUSED (m), PyObject *arg)
{
  nsdb_t *ns = _unwrap_db (arg);
  if (!ns)
  {
    return NULL;
  }

  PyCapsule_SetDestructor (arg, NULL);

  if (nsdb_close (ns) < 0)
  {
    PyErr_SetString (PyExc_RuntimeError, "Failed to close numstore database");
    return NULL;
  }

  Py_RETURN_NONE;
}

PyObject *
pyns_open (PyObject *Py_UNUSED (m), PyObject *arg)
{
  if (!PyUnicode_Check (arg))
  {
    PyErr_SetString (PyExc_TypeError, "path must be str");
    return NULL;
  }

  const char *path = PyUnicode_AsUTF8 (arg);
  if (!path)
  {
    return NULL;
  }

  nsdb_t *ns = nsdb_open (path);
  if (!ns)
  {
    PyErr_SetString (PyExc_RuntimeError, "Failed to open numstore database");
    return NULL;
  }

  return PyCapsule_New ((void *)ns, DB_CAPSULE, _nspy_release_db);
}

PyObject *
pyns_commit (PyObject *Py_UNUSED (m), PyObject *arg)
{
  nsdb_t *ns = (nsdb_t *)PyCapsule_GetPointer (arg, TXN_CAPSULE);
  if (!ns)
  {
    return NULL;
  }

  // COMMIT
  if (nsdb_commit (ns) < 0)
  {
    _pyns_set_error (ns);
    return NULL;
  }

  Py_RETURN_NONE;
}

PyObject *
pyns_rollback (PyObject *Py_UNUSED (m), PyObject *arg)
{
  nsdb_t *ns = (nsdb_t *)PyCapsule_GetPointer (arg, TXN_CAPSULE);
  if (!ns)
  {
    return NULL;
  }

  // ROLLBACK
  if (nsdb_rollback (ns) < 0)
  {
    _pyns_set_error (ns);
    return NULL;
  }

  Py_RETURN_NONE;
}

PyObject *
pyns_execute (PyObject *Py_UNUSED (m), PyObject *args)
{
  PyObject      *db;
  PyObject      *txn;
  char          *query;
  PyObject      *data_obj = Py_None;
  PyArrayObject *contig   = NULL;
  PyObject      *result   = NULL;

  // Parse arguments
  //    execute(db, txn, query, data | None)
  if (!PyArg_ParseTuple (args, "OOs|O", &db, &txn, &query, &data_obj))
  {
    return NULL;
  }

  if (data_obj != Py_None)
  {
    if (!PyArray_Check (data_obj))
    {
      PyErr_SetString (PyExc_TypeError, "data must be a numpy array or None");
      return NULL;
    }

    contig = (PyArrayObject *)PyArray_FROM_OTF (
        data_obj,
        NPY_NOTYPE,
        NPY_ARRAY_IN_ARRAY | NPY_ARRAY_FORCECAST
    );

    if (contig == NULL)
    {
      return NULL;
    }
  }

  // Fetch active handle
  nsdb_t *ns = _active_ns (db, txn);
  if (ns == NULL)
  {
    PyErr_SetString (PyExc_RuntimeError, "no active namespace for db/txn");
    Py_XDECREF (contig);
    return NULL;
  }

  // Get bytes
  char    *bytes  = contig ? PyArray_BYTES (contig) : NULL;
  npy_intp nbytes = contig ? PyArray_NBYTES (contig) : 0;
  if (contig != NULL && bytes == NULL)
  {
    PyErr_SetString (PyExc_RuntimeError, "array has no underlying buffer");
    Py_XDECREF (contig);
    return NULL;
  }

  // Execute the query
  sb_size ret = nsdb_fexecute (ns, "%s", bytes, query);
  if (ret < 0)
  {
    _pyns_set_error (ns);
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
        METH_O,
        "txn_commit(txn) -> None",
    },
    {
        "pyns_rollback",
        pyns_rollback,
        METH_O,
        "txn_rollback(txn) -> None",
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
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "_pynumstore",
    .m_doc =
        "Thin C wrapper around smfile operations for the pynumstore package.",
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
