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

#include "nscore/compiler.h"
#include "pynumstore.h"

// Numpy options
#define NO_IMPORT_ARRAY

#include "c_specx.h"
#include "nscore/types.h"

#include <Python.h>
#include <numpy/arrayobject.h>
#include <string.h>

static PyObject *ns_type_to_dtype (const struct type *t);

static PyObject *build_complex_struct (int component_typenum) {
  PyArray_Descr *comp = PyArray_DescrFromType (component_typenum);
  if (comp == NULL) { return NULL; }

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
  if (d == NULL) { return NULL; }

  return (PyObject *)d;
}

static PyObject *struct_to_dtype (const struct struct_t *st) {
  ASSERT (st->len > 0);

  // fields = [None, None, .... ]
  PyObject *fields = PyList_New (st->len);
  if (fields == NULL) { return NULL; }

  for (u16 i = 0; i < st->len; i++) {
    // name = st.name
    PyObject *name = PyUnicode_FromStringAndSize (st->keys[i].data, (Py_ssize_t)st->keys[i].len);
    if (name == NULL) {
      Py_DECREF (fields);
      return NULL;
    }

    // sub = topy(st.type)
    PyObject *sub = ns_type_to_dtype (st->types[i]);
    if (sub == NULL) {
      Py_DECREF (name);
      Py_DECREF (fields);
      return NULL;
    }

    // tup = (name, sub)
    PyObject *tup = PyTuple_New (2);
    if (tup == NULL) {
      Py_DECREF (name);
      Py_DECREF (sub);
      Py_DECREF (fields);
      return NULL;
    }
    PyTuple_SET_ITEM (tup, 0, name);
    PyTuple_SET_ITEM (tup, 1, sub);

    // fields[i] = tup
    PyList_SET_ITEM (fields, i, tup);
  }

  // out = np.dtype(fields)
  PyArray_Descr *out = NULL;
  if (PyArray_DescrConverter (fields, &out) != NPY_SUCCEED) {
    Py_DECREF (fields);
    return NULL;
  }
  Py_DECREF (fields);

  return (PyObject *)out;
}

static PyObject *union_to_dtype (const struct union_t *un) {
  ASSERT (un->len > 0);

  // names = [None, None, ...]
  // formats = [None, None, ...]
  // offsets = [None, None, ...]
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
    // name = un.name
    // sub = un.name
    // offset = 0
    PyObject *name = PyUnicode_FromStringAndSize (un->keys[i].data, (Py_ssize_t)un->keys[i].len);
    PyObject *sub  = name ? ns_type_to_dtype (un->types[i]) : NULL;
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

    // max_size = max(max_size, sizeof(sub))
    Py_ssize_t isize = PyDataType_ELSIZE (((PyArray_Descr *)sub));
    if (isize > max_size) { max_size = isize; }

    // names[i] = name
    // formats[i] = sub
    // offsets[i] = off
    PyList_SET_ITEM (names, i, name);
    PyList_SET_ITEM (formats, i, sub);
    PyList_SET_ITEM (offsets, i, off);
  }

  // itemsize = long(max_size)
  // spec = {}
  PyObject *itemsize = PyLong_FromSsize_t (max_size);
  PyObject *spec     = itemsize ? PyDict_New () : NULL;
  if (spec == NULL) {
    Py_XDECREF (itemsize);
    Py_DECREF (names);
    Py_DECREF (formats);
    Py_DECREF (offsets);
    return NULL;
  }

  // spec["formats"] = formats
  // spec["offsets"] = offsets
  // spec["itemsize"] = itemsize
  int rc = PyDict_SetItemString (spec, "names", names);
  rc     = rc | PyDict_SetItemString (spec, "formats", formats);
  rc     = rc | PyDict_SetItemString (spec, "offsets", offsets);
  rc     = rc | PyDict_SetItemString (spec, "itemsize", itemsize);
  Py_DECREF (names);
  Py_DECREF (formats);
  Py_DECREF (offsets);
  Py_DECREF (itemsize);
  if (rc != 0) {
    Py_DECREF (spec);
    return NULL;
  }

  // out = np.dtype(spec)
  PyArray_Descr *out = NULL;
  if (PyArray_DescrConverter (spec, &out) != NPY_SUCCEED) {
    Py_DECREF (spec);
    return NULL;
  }
  Py_DECREF (spec);

  return (PyObject *)out;
}

static PyObject *sarray_to_dtype (const struct sarray_t *sa) {
  ASSERT (sa->rank > 0);

  // sub = topy(sa->t)
  PyObject *sub = ns_type_to_dtype (sa->t);
  if (sub == NULL) { return NULL; }

  // shape = (sa->dims[0], sa->dims[1]...)
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

  // spec = (sub, shape)
  PyObject *spec = PyTuple_New (2);
  if (spec == NULL) {
    Py_DECREF (shape);
    Py_DECREF (sub);
    return NULL;
  }
  // Steals
  PyTuple_SET_ITEM (spec, 0, sub);
  PyTuple_SET_ITEM (spec, 1, shape);

  // out = np.dtype(spec)
  PyArray_Descr *out = NULL;
  if (PyArray_DescrConverter (spec, &out) != NPY_SUCCEED) {
    Py_DECREF (spec);
    return NULL;
  }
  Py_DECREF (spec);

  return (PyObject *)out;
}

static PyObject *ns_type_to_dtype (const struct type *t) {
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

PyObject *pyns_compile_type (PyObject *Py_UNUSED (m), PyObject *arg) {
  const char *src = PyUnicode_AsUTF8 (arg);
  if (!src) { return NULL; }

  struct type        t;
  struct chunk_alloc temp;
  chunk_alloc_create_default (&temp);
  error e = error_create ();

  if (compile_type (&t, src, &temp, &e)) {
    PyErr_Format (PyExc_ValueError, "Error: %.*s", e.cmlen, e.cause_msg);
    chunk_alloc_free_all (&temp);
    return NULL;
  }

  PyObject *ret = ns_type_to_dtype (&t);
  chunk_alloc_free_all (&temp);

  return ret;
}

#ifndef NTEST

static inline struct type *mk_prim (enum prim_t p) {
  struct type *t = calloc (1, sizeof (*t));
  t->type        = T_PRIM;
  t->p           = p;
  return t;
}

static inline struct type *strfcstruct (u16 len, struct string *keys, struct type **types) {
  struct type *t = calloc (1, sizeof (*t));
  t->type        = T_STRUCT;
  t->st.len      = len;
  t->st.keys     = keys;
  t->st.types    = types;
  return t;
}

static inline struct type *mk_union (u16 len, struct string *keys, struct type **types) {
  struct type *t = calloc (1, sizeof (*t));
  t->type        = T_UNION;
  t->un.len      = len;
  t->un.keys     = keys;
  t->un.types    = types;
  return t;
}

static inline struct type *mk_sarray (u16 rank, u32 *dims, struct type *sub) {
  struct type *t = calloc (1, sizeof (*t));
  t->type        = T_SARRAY;
  t->sa.rank     = rank;
  t->sa.dims     = dims;
  t->sa.t        = sub;
  return t;
}

static PyObject *ref_dtype (const char *spec) {
  PyObject *np = PyImport_ImportModule ("numpy");
  if (np == NULL) { return NULL; }

  PyObject *fn = PyObject_GetAttrString (np, "dtype");
  Py_DECREF (np);
  if (fn == NULL) { return NULL; }

  PyObject *s = PyUnicode_FromString (spec);
  if (s == NULL) {
    Py_DECREF (fn);
    return NULL;
  }

  PyObject *r = PyObject_CallOneArg (fn, s);
  Py_DECREF (s);
  Py_DECREF (fn);

  return r;
}

static int dtype_equals_expr (PyObject *dt, const char *expr) {
  PyObject *globals = PyDict_New ();
  if (!globals) { return -1; }

  PyObject *np = PyImport_ImportModule ("numpy");
  if (!np) {
    Py_DECREF (globals);
    return -1;
  }

  PyDict_SetItemString (globals, "np", np);
  Py_DECREF (np);

  PyObject *expected = PyRun_String (expr, Py_eval_input, globals, globals);
  Py_DECREF (globals);
  if (!expected) { return -1; }

  int eq = PyObject_RichCompareBool (dt, expected, Py_EQ);
  Py_DECREF (expected);

  return eq;
}

TEST (numstore_to_dtype) {

  TEST_CASE ("primitive_simple") {
    struct {
      enum prim_t p;
      const char *expected;
    } cases[] = {
        {U8, "u1"},
        {U16, "u2"},
        {U32, "u4"},
        {U64, "u8"},
        {I8, "i1"},
        {I16, "i2"},
        {I32, "i4"},
        {I64, "i8"},
        {F16, "f2"},
        {F32, "f4"},
        {F64, "f8"},
        {F128, "f16"},
        {CF64, "c8"},
        {CF128, "c16"},
        {CF256, "c32"},
    };

    for (size_t i = 0; i < sizeof (cases) / sizeof (cases[0]); i++) {
      struct type t = {.type = T_PRIM};
      t.p           = cases[i].p;
      PyObject *dt  = ns_type_to_dtype (&t);
      test_assert (dt);

      PyObject *ref = ref_dtype (cases[i].expected);
      test_assert (ref);

      int eq = PyObject_RichCompareBool (dt, ref, Py_EQ);
      test_assert_int_equal (eq, 1);

      Py_DECREF (dt);
      Py_DECREF (ref);
    }
  }

  TEST_CASE ("primitive_complex_as_struct") {
    /* cf32, ci*, cu* lack native numpy dtypes; expect struct{re,im}. */
    struct {
      enum prim_t p;
      const char *expr; /* Python expression producing the expected dtype */
    } cases[] = {
        {CF32, "np.dtype([('re','f2'),('im','f2')])"},
        {CI16, "np.dtype([('re','i1'),('im','i1')])"},
        {CI32, "np.dtype([('re','i2'),('im','i2')])"},
        {CI64, "np.dtype([('re','i4'),('im','i4')])"},
        {CI128, "np.dtype([('re','i8'),('im','i8')])"},
        {CU16, "np.dtype([('re','u1'),('im','u1')])"},
        {CU32, "np.dtype([('re','u2'),('im','u2')])"},
        {CU64, "np.dtype([('re','u4'),('im','u4')])"},
        {CU128, "np.dtype([('re','u8'),('im','u8')])"},
    };
    for (size_t i = 0; i < sizeof (cases) / sizeof (cases[0]); i++) {
      struct type t = {.type = T_PRIM};
      t.p           = cases[i].p;
      PyObject *dt  = ns_type_to_dtype (&t);

      test_assert (dt);
      int eq = dtype_equals_expr (dt, cases[i].expr);
      test_assert_int_equal (eq, 1);

      Py_DECREF (dt);
    }
  }

  TEST_CASE ("struct_simple") {
    /* struct { foo i32, bar f64 } */
    struct string keys[2]  = {strfcstr ("foo"), strfcstr ("bar")};
    struct type  *types[2] = {mk_prim (I32), mk_prim (F64)};
    struct type  *t        = strfcstruct (2, keys, types);

    PyObject *dt = ns_type_to_dtype (t);
    test_assert (dt);

    int eq = dtype_equals_expr (dt, "np.dtype([('foo','i4'),('bar','f8')])");
    test_assert_int_equal (eq, 1);
    Py_DECREF (dt);

    i_free (types[0]);
    i_free (types[1]);
    i_free (t);
  }

  TEST_CASE ("struct_nested") {
    /* struct { foo i32, bar struct { biz u32, buz f32 } } */
    struct string inner_keys[2]  = {strfcstr ("biz"), strfcstr ("buz")};
    struct type  *inner_types[2] = {mk_prim (U32), mk_prim (F32)};
    struct type  *inner          = strfcstruct (2, inner_keys, inner_types);

    struct string outer_keys[2]  = {strfcstr ("foo"), strfcstr ("bar")};
    struct type  *outer_types[2] = {mk_prim (I32), inner};
    struct type  *outer          = strfcstruct (2, outer_keys, outer_types);

    PyObject *dt = ns_type_to_dtype (outer);
    test_assert (dt);

    int eq = dtype_equals_expr (
        dt,
        "np.dtype([('foo','i4'),"
        "          ('bar', np.dtype([('biz','u4'),('buz','f4')]))])");
    test_assert_int_equal (eq, 1);

    Py_DECREF (dt);

    free (inner_types[0]);
    free (inner_types[1]);
    free (inner);
    free (outer_types[0]);
    free (outer);
  }

  TEST_CASE ("union_simple") {
    /* union { a i32, b f32 } */
    struct string keys[2]  = {strfcstr ("a"), strfcstr ("b")};
    struct type  *types[2] = {mk_prim (I32), mk_prim (F32)};
    struct type  *t        = mk_union (2, keys, types);

    PyObject *dt = ns_type_to_dtype (t);
    test_assert (dt);

    int eq = dtype_equals_expr (
        dt,
        "np.dtype({'names':['a','b'],'formats':['i4','f4'],"
        "          'offsets':[0,0],'itemsize':4})");
    test_assert_int_equal (eq, 1);

    Py_DECREF (dt);

    free (types[0]);
    free (types[1]);
    free (t);
  }

  TEST_CASE ("union_mixed_sizes") {
    /* union { small u8, big i64 } -> itemsize should be 8 */
    struct string keys[2]  = {strfcstr ("small"), strfcstr ("big")};
    struct type  *types[2] = {mk_prim (U8), mk_prim (I64)};
    struct type  *t        = mk_union (2, keys, types);

    PyObject *dt = ns_type_to_dtype (t);
    test_assert (dt);

    PyArray_Descr *d = (PyArray_Descr *)dt;
    test_assert_int_equal (PyDataType_ELSIZE (d), 8);

    Py_DECREF (dt);

    free (types[0]);
    free (types[1]);
    free (t);
  }

  TEST_CASE ("sarray_1d") {
    /* [10] i32 */
    u32          dims[1] = {10};
    struct type *sub     = mk_prim (I32);
    struct type *t       = mk_sarray (1, dims, sub);

    PyObject *dt = ns_type_to_dtype (t);
    test_assert (dt);

    int eq = dtype_equals_expr (dt, "np.dtype(('i4', (10,)))");
    test_assert_int_equal (eq, 1);

    Py_DECREF (dt);

    free (sub);
    free (t);
  }

  TEST_CASE ("sarray_2d") {
    /* [20][30] f64 */
    u32          dims[2] = {20, 30};
    struct type *sub     = mk_prim (F64);
    struct type *t       = mk_sarray (2, dims, sub);

    PyObject *dt = ns_type_to_dtype (t);
    test_assert (dt);

    int eq = dtype_equals_expr (dt, "np.dtype(('f8', (20,30)))");
    test_assert_int_equal (eq, 1);

    Py_DECREF (dt);

    free (sub);
    free (t);
  }

  TEST_CASE ("spec_example") {
    /* [20][30] struct { a union { i f32, b i64 }, c [10] f64 } */
    struct string union_keys[2]  = {strfcstr ("i"), strfcstr ("b")};
    struct type  *union_types[2] = {mk_prim (F32), mk_prim (I64)};
    struct type  *un             = mk_union (2, union_keys, union_types);

    u32          inner_dims[1] = {10};
    struct type *inner_arr_sub = mk_prim (F64);
    struct type *inner_arr     = mk_sarray (1, inner_dims, inner_arr_sub);

    struct string struct_keys[2]  = {strfcstr ("a"), strfcstr ("c")};
    struct type  *struct_types[2] = {un, inner_arr};
    struct type  *st              = strfcstruct (2, struct_keys, struct_types);

    u32          outer_dims[2] = {20, 30};
    struct type *outer         = mk_sarray (2, outer_dims, st);

    PyObject *dt = ns_type_to_dtype (outer);
    test_assert (dt);

    const char *expr =
        "np.dtype((np.dtype([('a', np.dtype({'names':['i','b'],"
        "                                    'formats':['f4','i8'],"
        "                                    'offsets':[0,0],"
        "                                    'itemsize':8})),"
        "                    ('c', np.dtype(('f8', (10,))))]),"
        "          (20, 30)))";
    int eq = dtype_equals_expr (dt, expr);

    test_assert_int_equal (eq, 1);
    Py_DECREF (dt);
    free (union_types[0]);
    free (union_types[1]);
    free (un);
    free (inner_arr_sub);
    free (inner_arr);
    free (st);
    free (outer);
  }

  TEST_CASE ("unknown_primitive") {
    struct type t = {.type = T_PRIM};
    t.p           = (enum prim_t)99;

    PyObject *dt = ns_type_to_dtype (&t);
    test_assert (dt == NULL);

    int has_err = PyErr_Occurred () != NULL;
    test_assert (has_err);

    PyErr_Clear ();
  }

  TEST_CASE ("struct_with_array_field") {
    /* struct { xs [10] i32, y f64 } */
    u32          dims[1] = {10};
    struct type *xs_sub  = mk_prim (I32);
    struct type *xs      = mk_sarray (1, dims, xs_sub);

    struct string keys[2]  = {strfcstr ("xs"), strfcstr ("y")};
    struct type  *types[2] = {xs, mk_prim (F64)};
    struct type  *t        = strfcstruct (2, keys, types);

    PyObject *dt = ns_type_to_dtype (t);
    test_assert (dt);

    int eq = dtype_equals_expr (dt, "np.dtype([('xs', 'i4', (10,)), ('y','f8')])");
    test_assert_int_equal (eq, 1);

    Py_DECREF (dt);

    free (xs_sub);
    free (xs);
    free (types[1]);
    free (t);
  }

}

static PyObject *make_dtype (const char *expr) {
  PyObject *globals = PyDict_New ();
  if (!globals) { return NULL; }
  PyObject *np = PyImport_ImportModule ("numpy");
  if (!np) {
    Py_DECREF (globals);
    return NULL;
  }
  PyDict_SetItemString (globals, "np", np);
  Py_DECREF (np);
  PyObject *result = PyRun_String (expr, Py_eval_input, globals, globals);
  Py_DECREF (globals);
  return result;
}

static PyObject *call_numpy_to_numstore (PyObject *dtype) {
  PyObject *mod = PyImport_ImportModule ("pynumstore");
  if (!mod) { return NULL; }
  PyObject *fn = PyObject_GetAttrString (mod, "numpy_to_numstore");
  Py_DECREF (mod);
  if (!fn) { return NULL; }
  PyObject *result = PyObject_CallOneArg (fn, dtype);
  Py_DECREF (fn);
  return result;
}

static bool n2ns_ok (PyObject *dtype, const char *expected) {
  PyObject  *result = call_numpy_to_numstore (dtype);
  if (!result) { return false; }
  const char *s  = PyUnicode_AsUTF8 (result);
  bool        ok = s && strcmp (s, expected) == 0;
  Py_DECREF (result);
  return ok;
}

TEST (numpy_to_numstore_cases) {
  /* sys.path is seeded by the test runner in main() */

  TEST_CASE ("primitives") {
    struct {
      const char *np_str;
      const char *expected;
    } cases[] = {
        {"u1", "u8"},    {"u2", "u16"},   {"u4", "u32"},   {"u8", "u64"},
        {"i1", "i8"},    {"i2", "i16"},   {"i4", "i32"},   {"i8", "i64"},
        {"f2", "f16"},   {"f4", "f32"},   {"f8", "f64"},
        {"c8", "cf64"},  {"c16", "cf128"},
    };
    for (size_t i = 0; i < sizeof (cases) / sizeof (cases[0]); i++) {
      PyObject *dt = ref_dtype (cases[i].np_str);
      test_assert (dt);
      test_assert (n2ns_ok (dt, cases[i].expected));
      Py_DECREF (dt);
    }
  }

  TEST_CASE ("byte_order_ignored") {
    PyObject *dt1 = make_dtype ("np.dtype('>i4')");
    PyObject *dt2 = make_dtype ("np.dtype('<i4')");
    test_assert (dt1 && n2ns_ok (dt1, "i32"));
    test_assert (dt2 && n2ns_ok (dt2, "i32"));
    Py_XDECREF (dt1);
    Py_XDECREF (dt2);
  }

  TEST_CASE ("accepts_string_input") {
    PyObject *s   = PyUnicode_FromString ("i4");
    test_assert (s);
    PyObject *res = call_numpy_to_numstore (s);
    test_assert (res);
    const char *out = PyUnicode_AsUTF8 (res);
    test_assert (out && strcmp (out, "i32") == 0);
    Py_DECREF (res);
    Py_DECREF (s);
  }

  TEST_CASE ("struct_simple") {
    PyObject *dt = make_dtype ("np.dtype([('foo','i4'),('bar','f8')])");
    test_assert (dt);
    test_assert (n2ns_ok (dt, "struct { foo i32, bar f64 }"));
    Py_DECREF (dt);
  }

  TEST_CASE ("struct_nested") {
    PyObject *dt = make_dtype (
        "np.dtype([('foo','i4'),('bar',[('biz','u4'),('buz','f4')])])");
    test_assert (dt);
    test_assert (n2ns_ok (dt, "struct { foo i32, bar struct { biz u32, buz f32 } }"));
    Py_DECREF (dt);
  }

  TEST_CASE ("struct_single_field") {
    PyObject *dt = make_dtype ("np.dtype([('only','i4')])");
    test_assert (dt);
    test_assert (n2ns_ok (dt, "struct { only i32 }"));
    Py_DECREF (dt);
  }

  TEST_CASE ("union_simple") {
    PyObject *dt = make_dtype (
        "np.dtype({'names':['a','b'],'formats':['i4','f4'],"
        "          'offsets':[0,0],'itemsize':4})");
    test_assert (dt);
    test_assert (n2ns_ok (dt, "union { a i32, b f32 }"));
    Py_DECREF (dt);
  }

  TEST_CASE ("union_of_struct_and_prim") {
    PyObject *dt = make_dtype (
        "np.dtype({'names':['x','y'],'formats':['i4',[('a','u4'),('b','f4')]],"
        "          'offsets':[0,0],'itemsize':8})");
    test_assert (dt);
    test_assert (n2ns_ok (dt, "union { x i32, y struct { a u32, b f32 } }"));
    Py_DECREF (dt);
  }

  TEST_CASE ("array_1d") {
    PyObject *dt = make_dtype ("np.dtype(('i4',(10,)))");
    test_assert (dt);
    test_assert (n2ns_ok (dt, "[10] i32"));
    Py_DECREF (dt);
  }

  TEST_CASE ("array_2d") {
    PyObject *dt = make_dtype ("np.dtype(('f8',(20,30)))");
    test_assert (dt);
    test_assert (n2ns_ok (dt, "[20][30] f64"));
    Py_DECREF (dt);
  }

  TEST_CASE ("array_of_struct") {
    PyObject *dt = make_dtype ("np.dtype(([('x','i4'),('y','f4')],(5,)))");
    test_assert (dt);
    test_assert (n2ns_ok (dt, "[5] struct { x i32, y f32 }"));
    Py_DECREF (dt);
  }

  TEST_CASE ("struct_with_array_field") {
    PyObject *dt = make_dtype ("np.dtype([('xs','i4',(10,)),('y','f8')])");
    test_assert (dt);
    test_assert (n2ns_ok (dt, "struct { xs [10] i32, y f64 }"));
    Py_DECREF (dt);
  }

  TEST_CASE ("spec_example") {
    PyObject *union_d = make_dtype (
        "np.dtype({'names':['i','b'],'formats':['f4','i8'],"
        "          'offsets':[0,0],'itemsize':8})");
    test_assert (union_d);
    PyObject *inner = make_dtype (
        "np.dtype([('a',np.dtype({'names':['i','b'],'formats':['f4','i8'],"
        "                         'offsets':[0,0],'itemsize':8})),"
        "          ('c',np.dtype(('f8',(10,))))])");
    test_assert (inner);
    PyObject *outer = make_dtype (
        "np.dtype((np.dtype([('a',np.dtype({'names':['i','b'],'formats':['f4','i8'],"
        "                                   'offsets':[0,0],'itemsize':8})),"
        "                    ('c',np.dtype(('f8',(10,))))]),(20,30)))");
    test_assert (outer);
    test_assert (n2ns_ok (
        outer,
        "[20][30] struct { a union { i f32, b i64 }, c [10] f64 }"));
    Py_XDECREF (union_d);
    Py_XDECREF (inner);
    Py_DECREF (outer);
  }

  TEST_CASE ("rejects_unsupported") {
    const char *bad[] = {
        "np.dtype('O')",    "np.dtype('U10')",  "np.dtype('S5')",
        "np.dtype('M8[s]')", "np.dtype('m8[s]')", "np.dtype('V8')",
        "np.dtype('?')",
    };
    for (size_t i = 0; i < sizeof (bad) / sizeof (bad[0]); i++) {
      PyObject *dt  = make_dtype (bad[i]);
      test_assert (dt);
      PyObject *res = call_numpy_to_numstore (dt);
      Py_DECREF (dt);
      test_assert (res == NULL);
      test_assert (PyErr_ExceptionMatches (PyExc_ValueError));
      PyErr_Clear ();
    }
  }

  TEST_CASE ("rejects_unsupported_inside_struct") {
    PyObject *dt = make_dtype ("np.dtype([('ok','i4'),('bad','O')])");
    test_assert (dt);
    PyObject *res = call_numpy_to_numstore (dt);
    Py_DECREF (dt);
    test_assert (res == NULL);
    test_assert (PyErr_ExceptionMatches (PyExc_ValueError));
    PyErr_Clear ();
  }
}
#endif
