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

#include "nscore/type_accessor.h"

#include "c_specx.h"
#include "nscore/compiler.h"
#include "nscore/subtype.h"
#include "nscore/types.h"

static bool range_ta_equal (const struct range_ta *left, const struct range_ta *right) {
  // Quick check that rank is the same
  if (left->dlen != right->dlen) { return false; }

  // Iterate through each supplied accessor
  for (u32 i = 0; i < left->dlen; ++i) {
    if (!user_stride_equal (&left->dim_accessors[i], &right->dim_accessors[i])) { return false; }
  }
  return type_accessor_equal (*left->sub_ta, *right->sub_ta);
}

bool type_accessor_equal (const struct type_accessor left, const struct type_accessor right) {
  if (left.type != right.type) { return false; }

  switch (left.type) {
    case TA_TAKE: {
      return true;
    }
    case TA_SELECT: {
      if (!string_equal (left.select.key, right.select.key)) { return false; }
      return type_accessor_equal (*left.select.sub_ta, *right.select.sub_ta);
    }
    case TA_RANGE: {
      return range_ta_equal (&left.range, &right.range);
    }
  }

  return false;
}

static struct type *ta_select_struct (
    struct type          *reftype,
    struct type_accessor *ta,
    struct chunk_alloc   *alloc,
    error                *e) {
  struct type *subtype = struct_t_resolve_key (NULL, &reftype->st, ta->select.key, e);
  if (subtype == NULL) { return NULL; }
  return ta_subtype (subtype, ta->select.sub_ta, alloc, e);
}

static struct type *ta_select_union (
    struct type          *reftype,
    struct type_accessor *ta,
    struct chunk_alloc   *alloc,
    error                *e) {
  struct type *subtype = union_t_resolve_key (&reftype->un, ta->select.key, e);
  if (subtype == NULL) { return NULL; }
  return ta_subtype (subtype, ta->select.sub_ta, alloc, e);
}

static struct type *ta_select_sarray (
    struct type          *reftype,
    struct type_accessor *ta,
    struct chunk_alloc   *alloc,
    error                *e) {
  struct type *ret = chunk_malloc (alloc, 1, sizeof *ret, e);
  if (ret == NULL) { goto failed; }

  struct sarray_builder builder;
  struct chunk_alloc    temp;
  chunk_alloc_create_default (&temp);
  sab_create (&builder, &temp, alloc);

  for (u32 i = 0; i < reftype->sa.rank; ++i) {
    if (sab_accept_dim (&builder, reftype->sa.dims[i], e)) { goto fail_chunk_alloc; }
  }

  struct type *t = ta_subtype (reftype->sa.t, ta->select.sub_ta, alloc, e);
  if (t == NULL) { goto fail_chunk_alloc; }

  if (sab_accept_type (&builder, t, e)) { goto fail_chunk_alloc; }

  ret->type = T_SARRAY;
  if (sab_build (&ret->sa, &builder, e)) { goto fail_chunk_alloc; }

  chunk_alloc_free_all (&temp);
  return ret;

fail_chunk_alloc:
  chunk_alloc_free_all (&temp);
failed:
  return NULL;
}

static struct type *ta_range_sarray (
    struct type          *reftype,
    struct type_accessor *ta,
    struct chunk_alloc   *alloc,
    error                *e) {
  struct sarray_builder builder;
  struct chunk_alloc    temp;
  chunk_alloc_create_default (&temp);
  sab_create (&builder, &temp, alloc);

  bool isarray = false;

  for (u32 i = 0; i < reftype->sa.rank; ++i) {
    if (i >= ta->range.dlen) {
      isarray = true;
      struct stride str;
      if (stride_resolve (&str, USER_STRIDE_ALL, reftype->sa.dims[i], e)) { goto fail_chunk_alloc; }
      if (sab_accept_dim (&builder, str.nelems, e)) { goto fail_chunk_alloc; }
    } else {
      isarray = isarray || ta->range.dim_accessors[i].present & COLON_PRESENT;
      struct stride str;
      if (stride_resolve (&str, ta->range.dim_accessors[i], reftype->sa.dims[i], e)) {
        goto fail_chunk_alloc;
      }
      if (ta->range.dim_accessors[i].present & COLON_PRESENT) {
        if (sab_accept_dim (&builder, str.nelems, e)) { goto fail_chunk_alloc; }
      }
    }
  }

  struct type *ret = NULL;
  struct type *t   = ta_subtype (reftype->sa.t, ta->range.sub_ta, alloc, e);
  if (t == NULL) { goto fail_chunk_alloc; }

  if (isarray) {
    if (sab_accept_type (&builder, t, e)) { goto fail_chunk_alloc; }

    ret = chunk_malloc (alloc, 1, sizeof *ret, e);
    if (ret == NULL) { return NULL; }

    ret->type = T_SARRAY;
    if (sab_build (&ret->sa, &builder, e)) { goto fail_chunk_alloc; }
  } else {
    ret = t;
  }

  chunk_alloc_free_all (&temp);
  return ret;

fail_chunk_alloc:
  chunk_alloc_free_all (&temp);
  return NULL;
}

struct type *
ta_subtype (struct type *reftype, struct type_accessor *ta, struct chunk_alloc *alloc, error *e) {
  switch (ta->type) {
    case TA_TAKE: {
      // Just copy the type over
      return reftype;
    }
    case TA_SELECT: {
      switch (reftype->type) {
        case T_STRUCT: {
          return ta_select_struct (reftype, ta, alloc, e);
        }
        case T_UNION: {
          return ta_select_union (reftype, ta, alloc, e);
        }
        case T_SARRAY: {
          return ta_select_sarray (reftype, ta, alloc, e);
        }
        case T_PRIM: {
          error_causef (
              e,
              ERR_INVALID_ARGUMENT,
              "type is not "
              "selectable");
          return NULL;
        }
      }
      UNREACHABLE ();
    }
    case TA_RANGE: {
      switch (reftype->type) {
        case T_SARRAY: {
          return ta_range_sarray (reftype, ta, alloc, e);
        }
        case T_STRUCT:
        case T_UNION:
        case T_PRIM: {
          error_causef (
              e,
              ERR_INVALID_ARGUMENT,
              "type is not "
              "rangeable");
          return NULL;
        }
      }
      UNREACHABLE ();
    }
  }

  UNREACHABLE ();
}

#ifndef NTEST

static void
test_ta_subtype_case (const char *typestr, const char *accessor, const char *expected_type) {
  error              e = error_create ();
  struct chunk_alloc alloc;
  chunk_alloc_create_default (&alloc);

  struct type    reftype;
  struct type    expected;
  struct subtype st;

  compile_type (&reftype, typestr, &alloc, &e);
  compile_type (&expected, expected_type, &alloc, &e);
  compile_subtype (&st, accessor, &alloc, &e);

  struct type *subtype = ta_subtype (&reftype, &st.ta, &alloc, &e);
  test_assert (type_equal (&expected, subtype));

  chunk_alloc_free_all (&alloc);
}

TEST (ta_subtype) {
  struct test_entry {
    const char *typestr;
    const char *accessor;
    const char *expected_type;
  } entries[] = {
      {"struct { i i32 } ", "a.i", "i32"},

      // ── simple struct field access ────────────────────────────

      {"struct { i i32 }", "a.i", "i32"},
      {"struct { x f32, y f64 }", "a.x", "f32"},
      {"struct { x f32, y f64 }", "a.y", "f64"},

      // ── nested struct ─────────────────────────────────────────
      {"struct { a struct { b i64 } }", "x.a.b", "i64"},
      {"struct { a struct { b struct { c f32 } } }", "x.a.b.c", "f32"},
      {"struct { a struct { b i32, c f64 }, d i8 }", "x.a.c", "f64"},

      // ── 1D array: single index (removes dimension) ───────────
      {"[10] i32", "a[5]", "i32"},
      {"[10] f64", "a[0]", "f64"},
      {"[10] f64", "a[9]", "f64"},

      // ── 1D array: stride (computes new dimension) ────────────
      {"[10] i32", "a[0:10:1]", "[10] i32"},
      {"[10] i32", "a[0:10:2]", "[5] i32"},
      {"[20] f32", "a[2:10:2]", "[4] f32"},
      {"[100] i64", "a[0:100:10]", "[10] i64"},
      {"[50] f64", "a[10:30:5]", "[4] f64"},
      {"[10] i32", "a[0:1:1]", "[1] i32"},
      {"[20] f32", "a[1:10:3]", "[3] f32"},

      // ── multi-dim: all singles ───────────────────────────────
      {"[10][ 20] i32", "a[5, 3]", "i32"},
      {"[2][ 3][ 4] f32", "a[0, 1, 2]", "f32"},

      // ── multi-dim: all strides ───────────────────────────────
      {"[10][ 20] i32", "a[0:10:2, 0:20:5]", "[5][ 4] i32"},
      {"[10][ 20] f64", "a[0:10:1, 0:20:1]", "[10][ 20] f64"},
      {"[6][ 8][ 10] i32", "a[0:6:2, 0:8:4, 0:10:5]", "[3][ 2][ 2] i32"},

      // ── multi-dim: mixed single + stride ─────────────────────
      {"[10][ 20] i32", "a[0:10:2, 5]", "[5] i32"},
      {"[10][ 20] i32", "a[5, 0:20:4]", "[5] i32"},
      {"[2][ 3][ 4] f32", "a[0:2:1, 1, 0:4:2]", "[2][ 2] f32"},
      {"[4][ 6][ 8] i64", "a[2, 0:6:3, 3]", "[2] i64"},
      {"[4][ 6][ 8] i64", "a[0:4:1, 2, 0:8:2]", "[4][ 4] i64"},

      // ── struct containing array ──────────────────────────────
      {"struct { data [100] f64 }", "a.data[0:50:1]", "[50] f64"},
      {"struct { data [100] f64 }", "a.data[5]", "f64"},
      {"struct { data [10][ 20] i32 }", "a.data[0:10:5, 3]", "[2] i32"},
      {"struct { m [4][ 4] f64 }", "a.m[0:4:1, 0:4:1]", "[4][ 4] f64"},

      // ── array of structs ─────────────────────────────────────
      {"[10] struct { x f32, y f32 }", "a[3].x", "f32"},
      {"[10] struct { x f32, y f32 }", "a[0:5:1].x", "[5] f32"},
      {"[10] struct { x f32, y f32 }", "a[0:10:2].y", "[5] f32"},

      // ── array of structs with nested field ───────────────────
      {"[10] struct { pos struct { x f64, y f64 } }", "a[4].pos.x", "f64"},
      {"[10] struct { pos struct { x f64, y f64 } }", "a[0:10:5].pos.y", "[2] f64"},

      // ── struct → array → struct chain ────────────────────────
      {"struct { points [100] struct { val f32 } }", "a.points[0:50:2].val", "[25] f32"},
      {"struct { points [100] struct { val f32 } }", "a.points[7].val", "f32"},

      // ── deeply nested struct + array ─────────────────────────
      {"struct { a struct { b [20] struct { c i32 } } }", "a.a.b[0:20:4].c", "[5] i32"},
      {"struct { a struct { b [20] struct { c i32 } } }", "a.a.b[10].c", "i32"},

      // ── struct field is a primitive (identity select) ────────
      {"struct { x i8 }", "a.x", "i8"},
      {"struct { x i16 }", "a.x", "i16"},
      {"struct { x i64 }", "a.x", "i64"},
      {"struct { x f32 }", "a.x", "f32"},

      // ── sibling field doesn't affect result ──────────────────
      {"struct { a i32, b f64, c [10] i8 }", "a.a", "i32"},
      {"struct { a i32, b f64, c [10] i8 }", "a.c[0:10:2]", "[5] i8"},
      {"struct { a i32, b f64, c [10] i8 }", "a.c[3]", "i8"},
  };

  for (u32 i = 0; i < arrlen (entries); ++i) {
    TEST_CASE ("%s %s %s", entries[i].typestr, entries[i].accessor, entries[i].expected_type) {
      test_ta_subtype_case (entries[i].typestr, entries[i].accessor, entries[i].expected_type);
    }
  }
}
#endif
