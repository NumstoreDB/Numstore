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

#include <numpy/ndarraytypes.h>
#include <object.h>

// Numpy options
#define NO_IMPORT_ARRAY

#include "nscore/types.h"

#include <Python.h>
#include <c_specx.h>
#include <numpy/arrayobject.h>
#include <string.h>

// Build a complex valued struct
static PyObject *
build_complex_struct (int component_typenum)
{
  PyArray_Descr *comp   = NULL; // dtype for the single component (e.g. f32)
  PyObject      *fields = NULL; // the list of fields
  PyArray_Descr *out    = NULL; // dtype of the output complex float (a struct of re im)
  PyObject* name = NULL;
  PyObject* tup = NULL;

  // Internal type
  comp = PyArray_DescrFromType (component_typenum);
  fields = PyList_New (2);

  if (comp == NULL || fields == NULL) goto fail;

  // Names
  static const char *names[2] = {"re", "im"};
  for (int i = 0; i < 2; i++)
    {
      // Generate python string
      name = PyUnicode_FromString (names[i]);
      tup = PyTuple_New (2);
      
      if (name == NULL || tup == NULL) goto fail;

      // increase ref so that SET_ITEM doesn't set count to 0
      Py_INCREF (comp);

      // All these Steal
      PyTuple_SET_ITEM (tup, 0, name);           
      PyTuple_SET_ITEM (tup, 1, (PyObject *)comp); 
      PyList_SET_ITEM (fields, i, tup);           

      name = NULL;
      comp = NULL;
    }

  if (PyArray_DescrConverter (fields, &out) != NPY_SUCCEED) {  goto fail;}

  Py_DECREF (comp);
  Py_DECREF (fields);

  return (PyObject *)out;

fail:
  Py_XDECREF(name);
  Py_XDECREF(tup);
  Py_XDECREF (comp);
  Py_XDECREF (fields);
  return NULL;
}

static PyObject *
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
    default: PyErr_Format (PyExc_ValueError, "unknown numstore primitive: %d", (int)p); return NULL;
  }

  PyArray_Descr *d = PyArray_DescrFromType (typenum);
  if (d == NULL) { return NULL; }

  return (PyObject *)d;
}

static PyObject *
struct_to_dtype (const struct struct_t *st)
{
  ASSERT (st->len > 0);
  PyObject      *fields = NULL; // List of fields
  PyObject      *name   = NULL; // name of each field
  PyObject      *sub    = NULL; // sub type of each field
  PyObject      *tup    = NULL; // the wrapper of (name, sub)
  PyArray_Descr *out    = NULL; // The result

  fields = PyList_New (st->len);
  if (fields == NULL) goto fail;

  for (u16 i = 0; i < st->len; i++)
    {
      name = PyUnicode_FromStringAndSize (st->keys[i].data, (Py_ssize_t)st->keys[i].len);
      sub = pyns_type_to_dtype (st->types[i]);
      tup = PyTuple_New (2);

      if (name == NULL || sub == NULL || tup == NULL) { goto fail; }

      PyTuple_SET_ITEM (tup, 0, name); name = NULL;
      PyTuple_SET_ITEM (tup, 1, sub);  sub  = NULL;
      PyList_SET_ITEM (fields, i, tup); tup = NULL;
    }

  if (PyArray_DescrConverter (fields, &out) != NPY_SUCCEED) { goto fail; }

  Py_DECREF (fields);

  return (PyObject *)out;

fail:
  Py_XDECREF (name);
  Py_XDECREF (sub);
  Py_XDECREF (tup);
  Py_XDECREF (fields);
  return NULL;
}

static PyObject *
union_to_dtype (const struct union_t *un)
{
  ASSERT (un->len > 0);
  PyObject      *names    = NULL; // names 
  PyObject      *formats  = NULL;
  PyObject      *offsets  = NULL;
  PyObject      *name     = NULL;
  PyObject      *sub      = NULL;
  PyObject      *off      = NULL;
  PyObject      *itemsize = NULL;
  PyObject      *spec     = NULL;
  PyArray_Descr *out      = NULL;

  names   = PyList_New (un->len);
  formats = PyList_New (un->len);
  offsets = PyList_New (un->len);
  if (names == NULL || formats == NULL || offsets == NULL) goto fail;

  Py_ssize_t max_size = 0;
  for (u16 i = 0; i < un->len; i++)
    {
      name = PyUnicode_FromStringAndSize (un->keys[i].data, (Py_ssize_t)un->keys[i].len);
      sub = pyns_type_to_dtype (un->types[i]);
      off = PyLong_FromLong (0);

      if (name == NULL || sub == NULL || off == NULL) goto fail;

#if NPY_FEATURE_VERSION >= NPY_2_0_API_VERSION
      Py_ssize_t isize = ((PyArray_Descr *)sub)->elsize;
#else
      Py_ssize_t isize = PyDataType_ELSIZE ((PyArray_Descr *)sub);
#endif

      if (isize > max_size) { max_size = isize; }

      PyList_SET_ITEM (names,   i, name); name = NULL;
      PyList_SET_ITEM (formats, i, sub);  sub  = NULL;
      PyList_SET_ITEM (offsets, i, off);  off  = NULL;
    }

  itemsize = PyLong_FromSsize_t (max_size);
  spec = PyDict_New ();

  if (itemsize == NULL || spec == NULL) goto fail;

  if (PyDict_SetItemString (spec, "names",    names)    != 0) goto fail;
  if (PyDict_SetItemString (spec, "formats",  formats)  != 0) goto fail;
  if (PyDict_SetItemString (spec, "offsets",  offsets)  != 0) goto fail;
  if (PyDict_SetItemString (spec, "itemsize", itemsize) != 0) goto fail;

  Py_DECREF (names);    names    = NULL;
  Py_DECREF (formats);  formats  = NULL;
  Py_DECREF (offsets);  offsets  = NULL;
  Py_DECREF (itemsize); itemsize = NULL;

  if (PyArray_DescrConverter (spec, &out) != NPY_SUCCEED) goto fail;

  Py_DECREF (spec);
  return (PyObject *)out;

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

static PyObject *
sarray_to_dtype (const struct sarray_t *sa)
{
  ASSERT (sa->rank > 0);
  PyObject      *sub   = NULL;
  PyObject      *shape = NULL;
  PyObject      *d     = NULL;
  PyObject      *spec  = NULL;
  PyArray_Descr *out   = NULL;

  sub = pyns_type_to_dtype (sa->t);
  shape = PyTuple_New (sa->rank);

  if (sub == NULL || shape == NULL) goto fail;

  for (u16 i = 0; i < sa->rank; i++)
    {
      d = PyLong_FromUnsignedLong ((unsigned long)sa->dims[i]);

      if (d == NULL) goto fail;

      PyTuple_SET_ITEM (shape, i, d); d = NULL;
    }

  spec = PyTuple_New (2);
  if (spec == NULL) goto fail;

  PyTuple_SET_ITEM (spec, 0, sub);   sub   = NULL;
  PyTuple_SET_ITEM (spec, 1, shape); shape = NULL;

  if (PyArray_DescrConverter (spec, &out) != NPY_SUCCEED) goto fail;

  Py_DECREF (spec);
  return (PyObject *)out;

fail:
  Py_XDECREF (d);
  Py_XDECREF (sub);
  Py_XDECREF (shape);
  Py_XDECREF (spec);
  return NULL;
}

PyObject *
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
pyns_compile_type (PyObject *Py_UNUSED (m), PyObject *arg)
{
  const char *src = PyUnicode_AsUTF8 (arg);
  if (!src) { return NULL; }

  struct type        t;
  struct chunk_alloc temp;
  chunk_alloc_create_default (&temp);
  error e = error_create ();

  if (compile_type (&t, src, &temp, &e))
  {
    PyErr_Format (PyExc_ValueError, "Error: %.*s", e.cmlen, e.cause_msg);
    chunk_alloc_free_all (&temp);
    return NULL;
  }

  PyObject *ret = pyns_type_to_dtype (&t);
  chunk_alloc_free_all (&temp);

  return ret;
}

#if 0

static inline struct type *
mk_prim (enum prim_t p)
{
  struct type *t = calloc (1, sizeof (*t));
  t->type        = T_PRIM;
  t->p           = p;
  return t;
}

static inline struct type *
strfcstruct (u16 len, struct string *keys, struct type **types)
{
  struct type *t = calloc (1, sizeof (*t));
  t->type        = T_STRUCT;
  t->st.len      = len;
  t->st.keys     = keys;
  t->st.types    = types;
  return t;
}

static inline struct type *
mk_union (u16 len, struct string *keys, struct type **types)
{
  struct type *t = calloc (1, sizeof (*t));
  t->type        = T_UNION;
  t->un.len      = len;
  t->un.keys     = keys;
  t->un.types    = types;
  return t;
}

static inline struct type *
mk_sarray (u16 rank, u32 *dims, struct type *sub)
{
  struct type *t = calloc (1, sizeof (*t));
  t->type        = T_SARRAY;
  t->sa.rank     = rank;
  t->sa.dims     = dims;
  t->sa.t        = sub;
  return t;
}

static PyObject *
ref_dtype (const char *spec)
{
  PyObject *np = PyImport_ImportModule ("numpy");
  if (np == NULL) { return NULL; }

  PyObject *fn = PyObject_GetAttrString (np, "dtype");
  Py_DECREF (np);
  if (fn == NULL) { return NULL; }

  PyObject *s = PyUnicode_FromString (spec);
  if (s == NULL)
  {
    Py_DECREF (fn);
    return NULL;
  }

  PyObject *r = PyObject_CallOneArg (fn, s);
  Py_DECREF (s);
  Py_DECREF (fn);

  return r;
}

static int
dtype_equals_expr (PyObject *dt, const char *expr)
{
  PyObject *globals = PyDict_New ();
  if (!globals) { return -1; }

  PyObject *np = PyImport_ImportModule ("numpy");
  if (!np)
  {
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

TEST_DISABLED (numstore_to_dtype)
{
  Py_Initialize ();

  TEST_CASE ("primitive_simple")
  {
    struct
    {
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

    for (size_t i = 0; i < sizeof (cases) / sizeof (cases[0]); i++)
    {
      struct type t = {.type = T_PRIM};
      t.p           = cases[i].p;
      PyObject *dt  = pyns_type_to_dtype (&t);
      test_assert (dt);

      PyObject *ref = ref_dtype (cases[i].expected);
      test_assert (ref);

      int eq = PyObject_RichCompareBool (dt, ref, Py_EQ);
      test_assert_int_equal (eq, 1);

      Py_DECREF (dt);
      Py_DECREF (ref);
    }
  }

  TEST_CASE ("primitive_complex_as_struct")
  {
    /* cf32, ci*, cu* lack native numpy dtypes; expect struct{re,im}. */
    struct
    {
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
    for (size_t i = 0; i < sizeof (cases) / sizeof (cases[0]); i++)
    {
      struct type t = {.type = T_PRIM};
      t.p           = cases[i].p;
      PyObject *dt  = pyns_type_to_dtype (&t);

      test_assert (dt);
      int eq = dtype_equals_expr (dt, cases[i].expr);
      test_assert_int_equal (eq, 1);

      Py_DECREF (dt);
    }
  }

  TEST_CASE ("struct_simple")
  {
    /* struct { foo i32, bar f64 } */
    struct string keys[2]  = {strfcstr ("foo"), strfcstr ("bar")};
    struct type  *types[2] = {mk_prim (I32), mk_prim (F64)};
    struct type  *t        = strfcstruct (2, keys, types);

    PyObject *dt = pyns_type_to_dtype (t);
    test_assert (dt);

    int eq = dtype_equals_expr (dt, "np.dtype([('foo','i4'),('bar','f8')])");
    test_assert_int_equal (eq, 1);
    Py_DECREF (dt);

    i_free (types[0]);
    i_free (types[1]);
    i_free (t);
  }

  TEST_CASE ("struct_nested")
  {
    /* struct { foo i32, bar struct { biz u32, buz f32 } } */
    struct string inner_keys[2]  = {strfcstr ("biz"), strfcstr ("buz")};
    struct type  *inner_types[2] = {mk_prim (U32), mk_prim (F32)};
    struct type  *inner          = strfcstruct (2, inner_keys, inner_types);

    struct string outer_keys[2]  = {strfcstr ("foo"), strfcstr ("bar")};
    struct type  *outer_types[2] = {mk_prim (I32), inner};
    struct type  *outer          = strfcstruct (2, outer_keys, outer_types);

    PyObject *dt = pyns_type_to_dtype (outer);
    test_assert (dt);

    int eq = dtype_equals_expr (
        dt,
        "np.dtype([('foo','i4'),"
        "          ('bar', np.dtype([('biz','u4'),('buz','f4')]))])"
    );
    test_assert_int_equal (eq, 1);

    Py_DECREF (dt);

    free (inner_types[0]);
    free (inner_types[1]);
    free (inner);
    free (outer_types[0]);
    free (outer);
  }

  TEST_CASE ("union_simple")
  {
    /* union { a i32, b f32 } */
    struct string keys[2]  = {strfcstr ("a"), strfcstr ("b")};
    struct type  *types[2] = {mk_prim (I32), mk_prim (F32)};
    struct type  *t        = mk_union (2, keys, types);

    PyObject *dt = pyns_type_to_dtype (t);
    test_assert (dt);

    int eq = dtype_equals_expr (
        dt,
        "np.dtype({'names':['a','b'],'formats':['i4','f4'],"
        "          'offsets':[0,0],'itemsize':4})"
    );
    test_assert_int_equal (eq, 1);

    Py_DECREF (dt);

    free (types[0]);
    free (types[1]);
    free (t);
  }

  TEST_CASE ("union_mixed_sizes")
  {
    /* union { small u8, big i64 } -> itemsize should be 8 */
    struct string keys[2]  = {strfcstr ("small"), strfcstr ("big")};
    struct type  *types[2] = {mk_prim (U8), mk_prim (I64)};
    struct type  *t        = mk_union (2, keys, types);

    PyObject *dt = pyns_type_to_dtype (t);
    test_assert (dt);

    PyArray_Descr *d = (PyArray_Descr *)dt;
    test_assert_int_equal (d->elsize, 8);

    Py_DECREF (dt);

    free (types[0]);
    free (types[1]);
    free (t);
  }

  TEST_CASE ("sarray_1d")
  {
    /* [10] i32 */
    u32          dims[1] = {10};
    struct type *sub     = mk_prim (I32);
    struct type *t       = mk_sarray (1, dims, sub);

    PyObject *dt = pyns_type_to_dtype (t);
    test_assert (dt);

    int eq = dtype_equals_expr (dt, "np.dtype(('i4', (10,)))");
    test_assert_int_equal (eq, 1);

    Py_DECREF (dt);

    free (sub);
    free (t);
  }

  TEST_CASE ("sarray_2d")
  {
    /* [20][30] f64 */
    u32          dims[2] = {20, 30};
    struct type *sub     = mk_prim (F64);
    struct type *t       = mk_sarray (2, dims, sub);

    PyObject *dt = pyns_type_to_dtype (t);
    test_assert (dt);

    int eq = dtype_equals_expr (dt, "np.dtype(('f8', (20,30)))");
    test_assert_int_equal (eq, 1);

    Py_DECREF (dt);

    free (sub);
    free (t);
  }

  TEST_CASE ("spec_example")
  {
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

    PyObject *dt = pyns_type_to_dtype (outer);
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

  TEST_CASE ("null_input")
  {
    PyObject *dt = pyns_type_to_dtype (NULL);
    test_assert (dt == NULL);

    int has_err = PyErr_Occurred () != NULL;
    test_assert (!has_err);

    PyErr_Clear ();
  }

  TEST_CASE ("unknown_kind")
  {
    struct type t = {.type = (enum type_t)99};

    PyObject *dt = pyns_type_to_dtype (&t);
    test_assert (dt == NULL);

    int has_err = PyErr_Occurred () != NULL;
    test_assert (!has_err);

    PyErr_Clear ();
  }

  TEST_CASE ("unknown_primitive")
  {
    struct type t = {.type = T_PRIM};
    t.p           = (enum prim_t)99;

    PyObject *dt = pyns_type_to_dtype (&t);
    test_assert (dt == NULL);

    int has_err = PyErr_Occurred () != NULL;
    test_assert (!has_err);

    PyErr_Clear ();
  }

  TEST_CASE ("struct_with_array_field")
  {
    /* struct { xs [10] i32, y f64 } */
    u32          dims[1] = {10};
    struct type *xs_sub  = mk_prim (I32);
    struct type *xs      = mk_sarray (1, dims, xs_sub);

    struct string keys[2]  = {strfcstr ("xs"), strfcstr ("y")};
    struct type  *types[2] = {xs, mk_prim (F64)};
    struct type  *t        = strfcstruct (2, keys, types);

    PyObject *dt = pyns_type_to_dtype (t);
    test_assert (dt);

    int eq = dtype_equals_expr (dt, "np.dtype([('xs', 'i4', (10,)), ('y','f8')])");
    test_assert_int_equal (eq, 1);

    Py_DECREF (dt);

    free (xs_sub);
    free (xs);
    free (types[1]);
    free (t);
  }

  Py_Finalize ();
}
#endif
