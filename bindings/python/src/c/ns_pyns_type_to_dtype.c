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

/// ns_type_convert.c
///
/// Recursively converts a compiled numstore `struct type` into a numpy
/// PyArray_Descr. Only pyns_type_to_dtype is exported (declared in
/// ns_pymodule_common.h) - the primitive/struct/union/array builders below
/// are internal recursion helpers.

#include "core/ns_string.h"
#include "ns_pynumstore.h"

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
