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

/* numstore_to_dtype.c
 *
 * Implementation: turns a numstore `struct type` into a numpy dtype object
 * by building Python list/dict/tuple specs and handing them to numpy's
 * PyArray_DescrConverter -- no strings involved.
 */
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#define NPY_NO_DEPRECATED_API  NPY_1_7_API_VERSION
#define PY_ARRAY_UNIQUE_SYMBOL numstore_ARRAY_API
#define NO_IMPORT_ARRAY        /* main translation unit calls import_array() */
#include "numstore_to_dtype.h"

#include <numpy/arrayobject.h>

static PyObject *primitive_to_dtype (enum prim_t p);
static PyObject *struct_to_dtype (const struct struct_t *st);
static PyObject *union_to_dtype (const struct union_t *un);
static PyObject *sarray_to_dtype (const struct sarray_t *sa);
static PyObject *build_complex_struct (int component_typenum);

PyObject *numstore_type_to_dtype (const struct type *t) {
  if (t == NULL) {
    PyErr_SetString (PyExc_ValueError, "numstore_type_to_dtype: null type");
    return NULL;
  }
  switch (t->type) {
    case T_PRIM: return primitive_to_dtype (t->p);
    case T_STRUCT: return struct_to_dtype (&t->st);
    case T_UNION: return union_to_dtype (&t->un);
    case T_SARRAY: return sarray_to_dtype (&t->sa);
    default:
      PyErr_Format (PyExc_ValueError, "numstore_type_to_dtype: unknown type kind %d", (int)t->type);
      return NULL;
  }
}

/* --- primitives ---------------------------------------------------------- */

static PyObject *primitive_to_dtype (enum prim_t p) {
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
    /* No native numpy dtype for these; build struct {re, im}. */
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
  if (d == NULL) return NULL;
  /* Validate that the platform actually supports this dtype at the
   * expected size (e.g. f128 / cf256 are 8/16 bytes on Windows). */
  return (PyObject *)d;
}

static PyObject *build_complex_struct (int component_typenum) {
  PyArray_Descr *comp = PyArray_DescrFromType (component_typenum);
  if (comp == NULL) return NULL;

  PyObject *fields = PyList_New (2);
  if (fields == NULL) {
    Py_DECREF (comp);
    return NULL;
  }

  static const char *names[2] = {"re", "im"};
  for (int i = 0; i < 2; i++) {
    PyObject *name = PyUnicode_FromString (names[i]);
    if (name == NULL) {
      Py_DECREF (comp);
      Py_DECREF (fields);
      return NULL;
    }
    PyObject *tup = PyTuple_New (2);
    if (tup == NULL) {
      Py_DECREF (name);
      Py_DECREF (comp);
      Py_DECREF (fields);
      return NULL;
    }
    PyTuple_SET_ITEM (tup, 0, name); /* steals name */
    Py_INCREF (comp);
    PyTuple_SET_ITEM (tup, 1, (PyObject *)comp); /* steals one ref */
    PyList_SET_ITEM (fields, i, tup);            /* steals tup */
  }
  Py_DECREF (comp); /* release our original ref */

  PyArray_Descr *out = NULL;
  if (PyArray_DescrConverter (fields, &out) != NPY_SUCCEED) {
    Py_DECREF (fields);
    return NULL;
  }
  Py_DECREF (fields);
  return (PyObject *)out;
}

/* --- struct -------------------------------------------------------------- */

static PyObject *struct_to_dtype (const struct struct_t *st) {
  if (st->len == 0) {
    PyErr_SetString (PyExc_ValueError, "struct must have at least one field");
    return NULL;
  }

  PyObject *fields = PyList_New (st->len);
  if (fields == NULL) return NULL;

  for (u16 i = 0; i < st->len; i++) {
    PyObject *name = PyUnicode_FromStringAndSize (st->keys[i].data, (Py_ssize_t)st->keys[i].len);
    if (name == NULL) {
      Py_DECREF (fields);
      return NULL;
    }

    PyObject *sub = numstore_type_to_dtype (st->types[i]);
    if (sub == NULL) {
      Py_DECREF (name);
      Py_DECREF (fields);
      return NULL;
    }

    PyObject *tup = PyTuple_New (2);
    if (tup == NULL) {
      Py_DECREF (name);
      Py_DECREF (sub);
      Py_DECREF (fields);
      return NULL;
    }
    PyTuple_SET_ITEM (tup, 0, name);  /* steals */
    PyTuple_SET_ITEM (tup, 1, sub);   /* steals */
    PyList_SET_ITEM (fields, i, tup); /* steals */
  }

  PyArray_Descr *out = NULL;
  if (PyArray_DescrConverter (fields, &out) != NPY_SUCCEED) {
    Py_DECREF (fields);
    return NULL;
  }
  Py_DECREF (fields);
  return (PyObject *)out;
}

/* --- union --------------------------------------------------------------- */

static PyObject *union_to_dtype (const struct union_t *un) {
  if (un->len == 0) {
    PyErr_SetString (PyExc_ValueError, "union must have at least one field");
    return NULL;
  }

  PyObject *names   = PyList_New (un->len);
  PyObject *formats = PyList_New (un->len);
  PyObject *offsets = PyList_New (un->len);
  if (names == NULL || formats == NULL || offsets == NULL) {
    Py_XDECREF (names);
    Py_XDECREF (formats);
    Py_XDECREF (offsets);
    return NULL;
  }

  Py_ssize_t max_size = 0;
  for (u16 i = 0; i < un->len; i++) {
    PyObject *name = PyUnicode_FromStringAndSize (un->keys[i].data, (Py_ssize_t)un->keys[i].len);
    PyObject *sub  = name ? numstore_type_to_dtype (un->types[i]) : NULL;
    PyObject *off  = sub ? PyLong_FromLong (0) : NULL;
    if (off == NULL) {
      Py_XDECREF (name);
      Py_XDECREF (sub);
      Py_XDECREF (off);
      Py_DECREF (names);
      Py_DECREF (formats);
      Py_DECREF (offsets);
      return NULL;
    }
    Py_ssize_t isize = PyDataType_ELSIZE ((PyArray_Descr *)sub);
    if (isize > max_size) max_size = isize;
    PyList_SET_ITEM (names, i, name);
    PyList_SET_ITEM (formats, i, sub);
    PyList_SET_ITEM (offsets, i, off);
  }

  PyObject *itemsize = PyLong_FromSsize_t (max_size);
  PyObject *spec     = itemsize ? PyDict_New () : NULL;
  if (spec == NULL) {
    Py_XDECREF (itemsize);
    Py_DECREF (names);
    Py_DECREF (formats);
    Py_DECREF (offsets);
    return NULL;
  }
  /* PyDict_SetItemString does NOT steal references. */
  int rc =
      (PyDict_SetItemString (spec, "names", names) | PyDict_SetItemString (spec, "formats", formats)
       | PyDict_SetItemString (spec, "offsets", offsets)
       | PyDict_SetItemString (spec, "itemsize", itemsize));
  Py_DECREF (names);
  Py_DECREF (formats);
  Py_DECREF (offsets);
  Py_DECREF (itemsize);
  if (rc != 0) {
    Py_DECREF (spec);
    return NULL;
  }

  PyArray_Descr *out = NULL;
  if (PyArray_DescrConverter (spec, &out) != NPY_SUCCEED) {
    Py_DECREF (spec);
    return NULL;
  }
  Py_DECREF (spec);
  return (PyObject *)out;
}

/* --- strict array -------------------------------------------------------- */

static PyObject *sarray_to_dtype (const struct sarray_t *sa) {
  if (sa->rank == 0) {
    PyErr_SetString (PyExc_ValueError, "strict array must have rank >= 1");
    return NULL;
  }

  PyObject *sub = numstore_type_to_dtype (sa->t);
  if (sub == NULL) return NULL;

  PyObject *shape = PyTuple_New (sa->rank);
  if (shape == NULL) {
    Py_DECREF (sub);
    return NULL;
  }
  for (u16 i = 0; i < sa->rank; i++) {
    PyObject *d = PyLong_FromUnsignedLong ((unsigned long)sa->dims[i]);
    if (d == NULL) {
      Py_DECREF (shape);
      Py_DECREF (sub);
      return NULL;
    }
    PyTuple_SET_ITEM (shape, i, d);
  }

  PyObject *spec = PyTuple_New (2);
  if (spec == NULL) {
    Py_DECREF (shape);
    Py_DECREF (sub);
    return NULL;
  }
  PyTuple_SET_ITEM (spec, 0, sub);   /* steals */
  PyTuple_SET_ITEM (spec, 1, shape); /* steals */

  PyArray_Descr *out = NULL;
  if (PyArray_DescrConverter (spec, &out) != NPY_SUCCEED) {
    Py_DECREF (spec);
    return NULL;
  }
  Py_DECREF (spec);
  return (PyObject *)out;
}
