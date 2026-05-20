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

/* test_numstore_to_dtype.c
 *
 * Embedded-Python test driver for numstore_type_to_dtype.
 * Each `test_<name>` function constructs a `struct type` in C, runs the
 * conversion, and compares the result to a reference numpy dtype.
 *
 * Build: see Makefile.
 */
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#define NPY_NO_DEPRECATED_API  NPY_1_7_API_VERSION
#define PY_ARRAY_UNIQUE_SYMBOL numstore_ARRAY_API
#include "numstore_to_dtype.h"

#include <numpy/arrayobject.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_pass = 0;
static int g_fail = 0;

#define CHECK(cond, msg)                     \
  do {                                       \
    if (cond) {                              \
      g_pass++;                              \
      printf ("    ok    %s\n", (msg));      \
    } else {                                 \
      g_fail++;                              \
      printf ("    FAIL  %s\n", (msg));      \
      if (PyErr_Occurred ()) PyErr_Print (); \
    }                                        \
  } while (0)

/* --- helpers ------------------------------------------------------------- */

/* Build a `struct string` from a NUL-terminated C string literal. */
static struct string mk_str (const char *s) {
  struct string out = {(u32)strlen (s), s};
  return out;
}

/* Build a primitive on the heap. Tests free with free_type(). */
static struct type *mk_prim (enum prim_t p) {
  struct type *t = calloc (1, sizeof (*t));
  t->type        = T_PRIM;
  t->p           = p;
  return t;
}

static struct type *mk_struct (u16 len, struct string *keys, struct type **types) {
  struct type *t = calloc (1, sizeof (*t));
  t->type        = T_STRUCT;
  t->st.len      = len;
  t->st.keys     = keys;
  t->st.types    = types;
  return t;
}

static struct type *mk_union (u16 len, struct string *keys, struct type **types) {
  struct type *t = calloc (1, sizeof (*t));
  t->type        = T_UNION;
  t->un.len      = len;
  t->un.keys     = keys;
  t->un.types    = types;
  return t;
}

static struct type *mk_sarray (u16 rank, u32 *dims, struct type *sub) {
  struct type *t = calloc (1, sizeof (*t));
  t->type        = T_SARRAY;
  t->sa.rank     = rank;
  t->sa.dims     = dims;
  t->sa.t        = sub;
  return t;
}

/* Build a reference numpy.dtype from a string literal (only used in tests).
 * Returns a new reference. */
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

/* Compare a dtype object to a reference built by evaluating a Python expr. */
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

/* --- tests --------------------------------------------------------------- */

static void test_primitive_simple (void) {
  printf ("  test_primitive_simple\n");
  struct {
    enum prim_t p;
    const char *expected; /* numpy dtype string */
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
    PyObject *dt  = numstore_type_to_dtype (&t);
    char      msg[64];
    snprintf (msg, sizeof (msg), "prim p=%d -> dtype('%s')", (int)cases[i].p, cases[i].expected);
    if (!dt) {
      CHECK (0, msg);
      continue;
    }
    PyObject *ref = ref_dtype (cases[i].expected);
    if (!ref) {
      Py_DECREF (dt);
      CHECK (0, msg);
      continue;
    }
    int eq = PyObject_RichCompareBool (dt, ref, Py_EQ);
    CHECK (eq == 1, msg);
    Py_DECREF (dt);
    Py_DECREF (ref);
  }
}

static void test_primitive_complex_as_struct (void) {
  printf ("  test_primitive_complex_as_struct\n");
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
    PyObject *dt  = numstore_type_to_dtype (&t);
    char      msg[96];
    snprintf (msg, sizeof (msg), "prim p=%d -> %s", (int)cases[i].p, cases[i].expr);
    if (!dt) {
      CHECK (0, msg);
      continue;
    }
    int eq = dtype_equals_expr (dt, cases[i].expr);
    CHECK (eq == 1, msg);
    Py_DECREF (dt);
  }
}

static void test_struct_simple (void) {
  printf ("  test_struct_simple\n");
  /* struct { foo i32, bar f64 } */
  struct string keys[2]  = {mk_str ("foo"), mk_str ("bar")};
  struct type  *types[2] = {mk_prim (I32), mk_prim (F64)};
  struct type  *t        = mk_struct (2, keys, types);

  PyObject *dt = numstore_type_to_dtype (t);
  CHECK (dt != NULL, "struct {foo i32, bar f64} converts");
  if (dt) {
    int eq = dtype_equals_expr (dt, "np.dtype([('foo','i4'),('bar','f8')])");
    CHECK (eq == 1, "struct {foo i32, bar f64} matches reference");
    Py_DECREF (dt);
  }
  free (types[0]);
  free (types[1]);
  free (t);
}

static void test_struct_nested (void) {
  printf ("  test_struct_nested\n");
  /* struct { foo i32, bar struct { biz u32, buz f32 } } */
  struct string inner_keys[2]  = {mk_str ("biz"), mk_str ("buz")};
  struct type  *inner_types[2] = {mk_prim (U32), mk_prim (F32)};
  struct type  *inner          = mk_struct (2, inner_keys, inner_types);

  struct string outer_keys[2]  = {mk_str ("foo"), mk_str ("bar")};
  struct type  *outer_types[2] = {mk_prim (I32), inner};
  struct type  *outer          = mk_struct (2, outer_keys, outer_types);

  PyObject *dt = numstore_type_to_dtype (outer);
  CHECK (dt != NULL, "nested struct converts");
  if (dt) {
    int eq = dtype_equals_expr (
        dt,
        "np.dtype([('foo','i4'),"
        "          ('bar', np.dtype([('biz','u4'),('buz','f4')]))])");
    CHECK (eq == 1, "nested struct matches reference");
    Py_DECREF (dt);
  }
  free (inner_types[0]);
  free (inner_types[1]);
  free (inner);
  free (outer_types[0]);
  free (outer);
}

static void test_union_simple (void) {
  printf ("  test_union_simple\n");
  /* union { a i32, b f32 } */
  struct string keys[2]  = {mk_str ("a"), mk_str ("b")};
  struct type  *types[2] = {mk_prim (I32), mk_prim (F32)};
  struct type  *t        = mk_union (2, keys, types);

  PyObject *dt = numstore_type_to_dtype (t);
  CHECK (dt != NULL, "union {a i32, b f32} converts");
  if (dt) {
    int eq = dtype_equals_expr (
        dt,
        "np.dtype({'names':['a','b'],'formats':['i4','f4'],"
        "          'offsets':[0,0],'itemsize':4})");
    CHECK (eq == 1, "union {a i32, b f32} matches reference");
    Py_DECREF (dt);
  }
  free (types[0]);
  free (types[1]);
  free (t);
}

static void test_union_mixed_sizes (void) {
  printf ("  test_union_mixed_sizes\n");
  /* union { small u8, big i64 } -> itemsize should be 8 */
  struct string keys[2]  = {mk_str ("small"), mk_str ("big")};
  struct type  *types[2] = {mk_prim (U8), mk_prim (I64)};
  struct type  *t        = mk_union (2, keys, types);

  PyObject *dt = numstore_type_to_dtype (t);
  CHECK (dt != NULL, "union with mixed sizes converts");
  if (dt) {
    PyArray_Descr *d = (PyArray_Descr *)dt;
    CHECK (PyDataType_ELSIZE (d) == 8, "union itemsize is max(field sizes) = 8");
    Py_DECREF (dt);
  }
  free (types[0]);
  free (types[1]);
  free (t);
}

static void test_sarray_1d (void) {
  printf ("  test_sarray_1d\n");
  /* [10] i32 */
  u32          dims[1] = {10};
  struct type *sub     = mk_prim (I32);
  struct type *t       = mk_sarray (1, dims, sub);

  PyObject *dt = numstore_type_to_dtype (t);
  CHECK (dt != NULL, "[10] i32 converts");
  if (dt) {
    int eq = dtype_equals_expr (dt, "np.dtype(('i4', (10,)))");
    CHECK (eq == 1, "[10] i32 matches reference");
    Py_DECREF (dt);
  }
  free (sub);
  free (t);
}

static void test_sarray_2d (void) {
  printf ("  test_sarray_2d\n");
  /* [20][30] f64 */
  u32          dims[2] = {20, 30};
  struct type *sub     = mk_prim (F64);
  struct type *t       = mk_sarray (2, dims, sub);

  PyObject *dt = numstore_type_to_dtype (t);
  CHECK (dt != NULL, "[20][30] f64 converts");
  if (dt) {
    int eq = dtype_equals_expr (dt, "np.dtype(('f8', (20,30)))");
    CHECK (eq == 1, "[20][30] f64 matches reference");
    Py_DECREF (dt);
  }
  free (sub);
  free (t);
}

static void test_spec_example (void) {
  printf ("  test_spec_example\n");
  /* [20][30] struct { a union { i f32, b i64 }, c [10] f64 } */
  struct string union_keys[2]  = {mk_str ("i"), mk_str ("b")};
  struct type  *union_types[2] = {mk_prim (F32), mk_prim (I64)};
  struct type  *un             = mk_union (2, union_keys, union_types);

  u32          inner_dims[1] = {10};
  struct type *inner_arr_sub = mk_prim (F64);
  struct type *inner_arr     = mk_sarray (1, inner_dims, inner_arr_sub);

  struct string struct_keys[2]  = {mk_str ("a"), mk_str ("c")};
  struct type  *struct_types[2] = {un, inner_arr};
  struct type  *st              = mk_struct (2, struct_keys, struct_types);

  u32          outer_dims[2] = {20, 30};
  struct type *outer         = mk_sarray (2, outer_dims, st);

  PyObject *dt = numstore_type_to_dtype (outer);
  CHECK (dt != NULL, "spec example converts");
  if (dt) {
    const char *expr =
        "np.dtype((np.dtype([('a', np.dtype({'names':['i','b'],"
        "                                    'formats':['f4','i8'],"
        "                                    'offsets':[0,0],"
        "                                    'itemsize':8})),"
        "                    ('c', np.dtype(('f8', (10,))))]),"
        "          (20, 30)))";
    int eq = dtype_equals_expr (dt, expr);
    CHECK (eq == 1, "spec example matches reference");
    Py_DECREF (dt);
  }
  free (union_types[0]);
  free (union_types[1]);
  free (un);
  free (inner_arr_sub);
  free (inner_arr);
  free (st);
  free (outer);
}

static void test_null_input (void) {
  printf ("  test_null_input\n");
  PyObject *dt = numstore_type_to_dtype (NULL);
  CHECK (dt == NULL, "NULL input returns NULL");
  int has_err = PyErr_Occurred () != NULL;
  CHECK (has_err, "NULL input sets an exception");
  PyErr_Clear ();
}

static void test_unknown_kind (void) {
  printf ("  test_unknown_kind\n");
  struct type t  = {.type = (enum type_t)99};
  PyObject   *dt = numstore_type_to_dtype (&t);
  CHECK (dt == NULL, "unknown type kind returns NULL");
  int has_err = PyErr_Occurred () != NULL;
  CHECK (has_err, "unknown type kind sets an exception");
  PyErr_Clear ();
}

static void test_unknown_primitive (void) {
  printf ("  test_unknown_primitive\n");
  struct type t = {.type = T_PRIM};
  t.p           = (enum prim_t)99;
  PyObject *dt  = numstore_type_to_dtype (&t);
  CHECK (dt == NULL, "unknown primitive returns NULL");
  int has_err = PyErr_Occurred () != NULL;
  CHECK (has_err, "unknown primitive sets an exception");
  PyErr_Clear ();
}

static void test_struct_with_array_field (void) {
  printf ("  test_struct_with_array_field\n");
  /* struct { xs [10] i32, y f64 } */
  u32          dims[1] = {10};
  struct type *xs_sub  = mk_prim (I32);
  struct type *xs      = mk_sarray (1, dims, xs_sub);

  struct string keys[2]  = {mk_str ("xs"), mk_str ("y")};
  struct type  *types[2] = {xs, mk_prim (F64)};
  struct type  *t        = mk_struct (2, keys, types);

  PyObject *dt = numstore_type_to_dtype (t);
  CHECK (dt != NULL, "struct with array field converts");
  if (dt) {
    int eq = dtype_equals_expr (dt, "np.dtype([('xs', 'i4', (10,)), ('y','f8')])");
    CHECK (eq == 1, "struct with array field matches reference");
    Py_DECREF (dt);
  }
  free (xs_sub);
  free (xs);
  free (types[1]);
  free (t);
}

/* --- driver -------------------------------------------------------------- */

static int do_import_numpy (void) {
  import_array1 (-1);
  return 0;
}

int main (void) {
  Py_Initialize ();
  if (do_import_numpy () < 0) {
    fprintf (stderr, "failed to import numpy\n");
    if (PyErr_Occurred ()) { PyErr_Print (); }
    Py_Finalize ();
    return 1;
  }

  printf ("numstore_type_to_dtype tests:\n");
  test_primitive_simple ();
  test_primitive_complex_as_struct ();
  test_struct_simple ();
  test_struct_nested ();
  test_struct_with_array_field ();
  test_union_simple ();
  test_union_mixed_sizes ();
  test_sarray_1d ();
  test_sarray_2d ();
  test_spec_example ();
  test_null_input ();
  test_unknown_kind ();
  test_unknown_primitive ();

  printf ("\n%d passed, %d failed\n", g_pass, g_fail);

  Py_Finalize ();
  return g_fail == 0 ? 0 : 1;
}
