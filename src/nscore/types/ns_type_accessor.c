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

#include "core/ns_alloc.h"
#include "core/ns_error.h"
#include "core/ns_string.h"
#include "core/testing/ns_testing.h"
#include "nscore/compiler/ns_compiler.h"
#include "nscore/ns_variables.h"
#include "nscore/types/ns_sarray_t.h"
#include "nscore/types/ns_struct_t.h"
#include "nscore/types/ns_types.h"
#include "nscore/types/ns_union_t.h"

/******************************************************************************
 * SECTION: Type Accessor
 ******************************************************************************/

static bool
range_ta_equal (const struct range_ta *left, const struct range_ta *right)
{
  // Quick check that rank is the same
  if (left->dlen != right->dlen)
  {
    return false;
  }

  // Iterate through each supplied accessor
  for (u32 i = 0; i < left->dlen; ++i)
  {
    if (!user_stride_equal (&left->dim_accessors[i], &right->dim_accessors[i]))
    {
      return false;
    }
  }
  return type_accessor_equal (*left->sub_ta, *right->sub_ta);
}

bool
type_accessor_equal (const struct type_accessor left, const struct type_accessor right)
{
  if (left.type != right.type)
  {
    return false;
  }

  switch (left.type)
  {
    case TA_TAKE:
    {
      return true;
    }
    case TA_SELECT:
    {
      if (!string_equal (left.select.key, right.select.key))
      {
        return false;
      }
      return type_accessor_equal (*left.select.sub_ta, *right.select.sub_ta);
    }
    case TA_RANGE:
    {
      return range_ta_equal (&left.range, &right.range);
    }
  }

  return false;
}

static struct type *
ta_select_struct (struct type *ref, struct type_accessor *ta, struct allocator *alloc, error *e)
{
  struct type *sub = struct_t_resolve_key (NULL, &ref->st, ta->select.key);
  if (sub == NULL)
  {
    error_causef (e, ERR_INTERP, "Failed to find sub key in struct");
    return NULL;
  }
  return ta_subtype (sub, ta->select.sub_ta, alloc, e);
}

static struct type *
ta_select_union (struct type *reftype, struct type_accessor *ta, struct allocator *alloc, error *e)
{
  struct type *subtype = union_t_resolve_key (&reftype->un, ta->select.key);
  if (subtype == NULL)
  {
    error_causef (e, ERR_INTERP, "Failed to resolve subtype");
    return NULL;
  }
  return ta_subtype (subtype, ta->select.sub_ta, alloc, e);
}

static struct type *
ta_select_sarray (struct type *reftype, struct type_accessor *ta, struct allocator *alloc, error *e)
{
  BUILDER_INIT (b, alloc);

  struct type *ret = allocate (alloc, 1, sizeof *ret, e);
  if (ret == NULL)
  {
    goto failed;
  }

  struct sarray_builder builder = sab_create (&b);

  for (u32 i = 0; i < reftype->sa.rank; ++i)
  {
    if (sab_accept_dim (&builder, reftype->sa.dims[i], e))
    {
      goto failed;
    }
  }

  struct type *t = ta_subtype (reftype->sa.t, ta, alloc, e);
  if (t == NULL)
  {
    goto failed;
  }

  if (sab_accept_type (&builder, t, e))
  {
    goto failed;
  }

  ret->type = T_SARRAY;
  if (sab_build (&ret->sa, &builder, e))
  {
    goto failed;
  }

  BUILDER_CLOSE (b);
  return ret;

failed:
  BUILDER_CLOSE (b);
  return NULL;
}

static struct type *
ta_range_sarray (struct type *reftype, struct type_accessor *ta, struct allocator *alloc, error *e)
{
  BUILDER_INIT (b, alloc);
  struct sarray_builder builder = sab_create (&b);

  bool isarray = false;

  for (u32 i = 0; i < reftype->sa.rank; ++i)
  {
    if (i >= ta->range.dlen)
    {
      isarray = true;
      struct stride str;
      if (stride_resolve (&str, USER_STRIDE_ALL, reftype->sa.dims[i], e))
      {
        goto failure;
      }
      if (sab_accept_dim (&builder, str.nelems, e))
      {
        goto failure;
      }
    }
    else
    {
      isarray = isarray || ta->range.dim_accessors[i].present & COLON_PRESENT;
      struct stride str;
      if (stride_resolve (&str, ta->range.dim_accessors[i], reftype->sa.dims[i], e))
      {
        goto failure;
      }
      if (ta->range.dim_accessors[i].present & COLON_PRESENT)
      {
        if (sab_accept_dim (&builder, str.nelems, e))
        {
          goto failure;
        }
      }
    }
  }

  struct type *ret = NULL;
  struct type *t   = ta_subtype (reftype->sa.t, ta->range.sub_ta, alloc, e);
  if (t == NULL)
  {
    goto failure;
  }

  if (isarray)
  {
    if (sab_accept_type (&builder, t, e))
    {
      goto failure;
    }

    ret = allocate (alloc, 1, sizeof *ret, e);
    if (ret == NULL)
    {
      return NULL;
    }

    ret->type = T_SARRAY;
    if (sab_build (&ret->sa, &builder, e))
    {
      goto failure;
    }
  }
  else
  {
    ret = t;
  }

  BUILDER_CLOSE (b);
  return ret;

failure:
  BUILDER_CLOSE (b);
  return NULL;
}

struct type *
ta_subtype (struct type *reftype, struct type_accessor *ta, struct allocator *alloc, error *e)
{
  switch (ta->type)
  {
    case TA_TAKE:
    {
      // Just copy the type over
      return reftype;
    }
    case TA_SELECT:
    {
      switch (reftype->type)
      {
        case T_STRUCT:
        {
          return ta_select_struct (reftype, ta, alloc, e);
        }
        case T_UNION:
        {
          return ta_select_union (reftype, ta, alloc, e);
        }
        case T_SARRAY:
        {
          return ta_select_sarray (reftype, ta, alloc, e);
        }
        case T_PRIM:
        {
          error_causef (
              e,
              ERR_INVALID_ARGUMENT,
              "type is not "
              "selectable"
          );
          return NULL;
        }
      }
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case TA_RANGE:
    {
      switch (reftype->type)
      {
        case T_SARRAY:
        {
          return ta_range_sarray (reftype, ta, alloc, e);
        }
        case T_STRUCT:
        case T_UNION:
        case T_PRIM:
        {
          error_causef (
              e,
              ERR_INVALID_ARGUMENT,
              "type is not "
              "rangeable"
          );
          return NULL;
        }
      }
      UNREACHABLE (); // LCOV_EXCL_LINE
      return 0;       // LCOV_EXCL_LINE
    }
  }

  UNREACHABLE (); // LCOV_EXCL_LINE
}

#ifdef TESTING

static void
test_ta_subtype_case (const char *typestr, const char *accessor, const char *expected_type)
{
  ALLOC_INIT (alloc);

  error e = error_create ();

  struct type    reftype;
  struct type    expected;
  struct subtype st;

  compile_type (&reftype, typestr, &alloc, &e);
  compile_type (&expected, expected_type, &alloc, &e);
  compile_subtype (&st, accessor, &alloc, &e);

  struct type *subtype = ta_subtype (&reftype, &st.ta, &alloc, &e);
  i_log_type (subtype, &e);
  i_log_type (&expected, &e);
  test_assert (type_equal (&expected, subtype));

  ALLOC_CLOSE (alloc);
}

TEST (ta_subtype)
{
  // Edge-focused cases. The interesting rules being exercised:
  //   * field select on an aggregate picks the field's type
  //   * field select on an ARRAY of aggregates BROADCASTS: the field type
  //     is re-wrapped in the outer array dimensions (a.i on [10] struct{..}
  //     -> [10] <fieldtype>)
  //   * a single index removes one dimension
  //   * a stride s:e:st resizes a dimension to ceil((e-s)/st)
  // Union cases are kept only as parity spot-checks, not full mirrors.
  struct test_entry
  {
    const char *typestr;
    const char *accessor;
    const char *expected_type;
  } entries[] = {
      // field select: baseline + non-first field (offset)
      {"struct { i i32 }", "a.i", "i32"},
      {"struct { x u32, y f64 }", "a.y", "f64"}, // pick 2nd field
      {"union { x f32, y f64 }", "a.y", "f64"},  // union parity

      // nested aggregates: depth + sibling offset
      {"struct { a struct { b struct { c f32 } } }", "x.a.b.c", "f32"},
      {"struct { a struct { b i32, c f64 }, d i8 }", "x.a.c", "f64"},
      {"union { a union { b i32, c f64 }, d i8 }", "x.a.c", "f64"}, // union parity

      // 1D single index: dimension removal at the boundaries
      {"[10] i32", "a[0]", "i32"}, // first
      {"[10] u32", "a[9]", "u32"}, // last

      // 1D stride: new size = ceil((stop - start) / step)
      {"[10] i32", "a[0:10:1]", "[10] i32"}, // identity (step 1)
      {"[10] i32", "a[0:10:3]", "[4] i32"},  // ceil edge: 0,3,6,9
      {"[20] f32", "a[2:10:2]", "[4] f32"},  // nonzero start
      {"[10] i32", "a[0:1:1]", "[1] i32"},   // degenerate single

      // multi-dim: full collapse / full stride / mixed removal
      {"[2][ 3][ 4] f32", "a[0, 1, 2]", "f32"},                           // all singles -> scalar
      {"[6][ 8][ 10] i32", "a[0:6:2, 0:8:4, 0:10:5]", "[3][ 2][ 2] i32"}, // all strides
      {"[2][ 3][ 4] f32", "a[0:2:1, 1, 0:4:2]", "[2][ 2] f32"},           // middle dim removed
      {"[4][ 6][ 8] i64", "a[2, 0:6:3, 3]", "[2] i64"},                   // lead+trail removed

      // struct/union containing an array
      {"struct { data [100] f64 }", "a.data[5]", "f64"},                 // field -> single
      {"struct { data [10][ 20] i32 }", "a.data[0:10:5, 3]", "[2] i32"}, // field -> mixed multidim
      {"union { data [10][ 20] i32 }", "a.data[0:10:5, 3]", "[2] i32"},  // union parity

      // array sub-access: field select BROADCASTS over array dims
      {"[10] struct { i i32 }", "a.i", "[10] i32"},         // the canonical case
      {"[10] struct { x f32, y f64 }", "a.y", "[10] f64"},  // broadcast + field offset
      {"[3][ 4] struct { v f32 }", "a.v", "[3][ 4] f32"},   // broadcast over 2 dims
      {"[5] struct { d [10] i32 }", "a.d", "[5][ 10] i32"}, // broadcast field is itself an array
      {"[6] struct { p struct { q i16 } }", "a.p.q", "[6] i16"}, // broadcast through nested field
      {"[10] union { x f32, y f32 }", "a.y", "[10] f32"},        // union parity

      // index/stride on array-of-aggregate, THEN field
      {"[10] struct { x f32, y f32 }", "a[3].x", "f32"},          // single index -> scalar field
      {"[10] struct { x f32, y f32 }", "a[0:10:2].y", "[5] f32"}, // stride -> broadcast field
      {"[10] union { x f32, y f32 }", "a[0:10:2].y", "[5] f32"},  // union parity

      // struct -> array -> struct chains
      {"struct { points [100] struct { val f32 } }", "a.points[7].val", "f32"}, // chain -> scalar
      {"struct { points [100] struct { val f32 } }",
       "a.points[0:50:2].val",
       "[25] f32"}, // chain -> broadcast

      // deep nest + array stride broadcast
      {"struct { a struct { b [20] struct { c i32 } } }", "a.a.b[0:20:4].c", "[5] i32"},

      // sibling fields don't shift the array-field result
      {"struct { a i32, b f64, c [10] i8 }", "a.c[0:10:2]", "[5] i8"},
  };

  for (u32 i = 0; i < arrlen (entries); ++i)
  {
    TEST_CASE ("%s :: %s :: %s", entries[i].typestr, entries[i].accessor, entries[i].expected_type)
    {
      test_ta_subtype_case (entries[i].typestr, entries[i].accessor, entries[i].expected_type);
    }
  }
}

#endif

/////////////////////////////////////////////////////////////////////
////// Builder

bool
user_stride_equal (const struct user_stride *left, const struct user_stride *right)
{
  return left->start == right->start && left->step == right->step && left->stop == right->stop
         && left->present == right->present;
}

DEFINE_DBG_ASSERT (struct range_builder, range_builder, s, { ASSERT (s); })

struct range_builder
rb_create (struct builder *b)
{
  return (struct range_builder){
      .head = NULL,
      .len  = 0,
      .b    = b,
  };
}

err_t
rb_accept_stride (struct range_builder *rb, struct user_stride stride, error *e)
{
  DBG_ASSERT (range_builder, rb);

  struct rb_llnode *node = builder_malloc_temp (rb->b, 1, sizeof *node, e);
  if (!node)
  {
    return error_trace (e);
  }

  llnode_init (&node->link);
  node->stride = stride;

  if (!rb->head)
  {
    rb->head = &node->link;
  }
  else
  {
    list_append (&rb->head, &node->link);
  }

  rb->len++;
  return SUCCESS;
}

err_t
rb_build (struct range_ta *dest, struct range_builder *rb, error *e)
{
  DBG_ASSERT (range_builder, rb);

  if (rb->len == 0)
  {
    error_causef (e, ERR_INTERP, "range: no dimensions");
    goto theend;
  }

  struct user_stride *dims = builder_malloc_persist (rb->b, rb->len, sizeof *dims, e);
  if (!dims)
  {
    goto theend;
  }

  u32 i = 0;
  for (struct llnode *it = rb->head; it; it = it->next)
  {
    struct rb_llnode *rn = container_of (it, struct rb_llnode, link);
    dims[i]              = rn->stride;
    i++;
  }

  dest->dim_accessors = dims;
  dest->dlen          = rb->len;

theend:
  return error_trace (e);
}

DEFINE_DBG_ASSERT (struct type_accessor_builder, type_accessor_builder, s, { ASSERT (s); })

static struct type_accessor *
tab_alloc (struct type_accessor_builder *builder, error *e)
{
  if (builder->head == NULL)
  {
    return &builder->ret;
  }

  struct type_accessor *ta = builder_malloc_persist (builder->b, 1, sizeof *ta, e);
  return ta;
}

static void
tab_link (struct type_accessor_builder *builder, struct type_accessor *ta)
{
  if (!builder->head)
  {
    builder->head = ta;
  }
  else
  {
    if (builder->tail->type == TA_SELECT)
    {
      builder->tail->select.sub_ta = ta;
    }
    else if (builder->tail->type == TA_RANGE)
    {
      builder->tail->range.sub_ta = ta;
    }
  }
  builder->tail = ta;
}

static err_t
tab_flush_range (struct type_accessor_builder *builder, error *e)
{
  if (!builder->in_range)
  {
    return SUCCESS;
  }

  struct type_accessor *ta = tab_alloc (builder, e);
  if (!ta)
  {
    return error_trace (e);
  }

  ta->type         = TA_RANGE;
  ta->range.sub_ta = NULL;

  WRAP (rb_build (&ta->range, &builder->rb, e));

  tab_link (builder, ta);
  builder->in_range = false;

  return SUCCESS;
}

static void
tab_ensure_range (struct type_accessor_builder *builder)
{
  if (!builder->in_range)
  {
    builder->rb       = rb_create (builder->b);
    builder->in_range = true;
  }
}

struct type_accessor_builder
tab_create (struct builder *b)
{
  return (struct type_accessor_builder){
      .head     = NULL,
      .tail     = NULL,
      .b        = b,
      .in_range = false,
  };
}

err_t
tab_accept_select (struct type_accessor_builder *builder, struct string key, error *e)
{
  DBG_ASSERT (type_accessor_builder, builder);

  WRAP (tab_flush_range (builder, e));

  struct type_accessor *ta = tab_alloc (builder, e);
  if (!ta)
  {
    return error_trace (e);
  }

  key.data = allocator_copy (builder->b->persistent, key.data, key.len, e);
  if (!key.data)
  {
    return error_trace (e);
  }

  ta->type          = TA_SELECT;
  ta->select.key    = key;
  ta->select.sub_ta = NULL;

  tab_link (builder, ta);

  return SUCCESS;
}

err_t
tab_accept_stride (struct type_accessor_builder *builder, struct user_stride stride, error *e)
{
  DBG_ASSERT (type_accessor_builder, builder);
  tab_ensure_range (builder);
  return rb_accept_stride (&builder->rb, stride, e);
}

err_t
tab_accept_take (struct type_accessor_builder *builder, error *e)
{
  DBG_ASSERT (type_accessor_builder, builder);

  WRAP (tab_flush_range (builder, e));

  struct type_accessor *ta = tab_alloc (builder, e);
  if (!ta)
  {
    return error_trace (e);
  }

  ta->type = TA_TAKE;

  tab_link (builder, ta);

  return SUCCESS;
}

err_t
tab_build (struct type_accessor *dest, struct type_accessor_builder *builder, error *e)
{
  DBG_ASSERT (type_accessor_builder, builder);

  if (tab_accept_take (builder, e))
  {
    goto theend;
  }

  *dest = builder->ret;

theend:
  return error_trace (e);
}

#ifdef TESTING
TEST (type_accessor_builder)
{
  ALLOC_INIT (alloc);
  BUILDER_INIT (b, &alloc);

  error e = error_create ();

  // 0. freshly-created builder must be clean
  struct type_accessor_builder builder = tab_create (&b);
  test_fail_if (builder.head != NULL);
  test_fail_if (builder.tail != NULL);

  struct type_accessor acc;
  tab_build (&acc, &builder, &e);
  test_assert_int_equal (acc.type, TA_TAKE);

  builder = tab_create (&b);

  // 2. accept a select accessor
  struct string key1 = strfcstr ("field1");
  test_assert_int_equal (tab_accept_select (&builder, key1, &e), SUCCESS);
  // 3. accept a stride + single (enters range mode)
  test_assert_int_equal (tab_accept_stride (&builder, ustride012 (0, 10, 2), &e), SUCCESS);
  test_assert (builder.in_range);
  test_assert_int_equal (tab_accept_stride (&builder, ustride_single (5), &e), SUCCESS);
  test_assert_int_equal (builder.rb.len, 2);

  // 4. accept another select accessor (should flush the range)
  struct string key2 = strfcstr ("field2");
  test_assert_int_equal (tab_accept_select (&builder, key2, &e), SUCCESS);
  test_fail_if (builder.in_range);

  // 5. successful build
  test_assert_int_equal (tab_build (&acc, &builder, &e), SUCCESS);

  // 6. verify chain: SELECT(field1) → RANGE([0:10:2, 5]) →
  // SELECT(field2) → TAKE
  test_assert_int_equal (acc.type, TA_SELECT);
  test_assert_int_equal (string_equal (acc.select.key, key1), true);
  struct type_accessor *range_acc = acc.select.sub_ta;
  test_assert_int_equal (range_acc->type, TA_RANGE);
  test_assert_int_equal (range_acc->range.dlen, 2);

  test_assert_int_equal (range_acc->range.dim_accessors[0].start, 0);
  test_assert_int_equal (range_acc->range.dim_accessors[0].stop, 10);
  test_assert_int_equal (range_acc->range.dim_accessors[0].step, 2);

  test_assert_int_equal (range_acc->range.dim_accessors[1].start, 5);
  test_assert_int_equal (range_acc->range.dim_accessors[1].stop, 0);
  test_assert_int_equal (range_acc->range.dim_accessors[1].step, 0);

  struct type_accessor *select_acc = range_acc->range.sub_ta;
  test_assert_int_equal (select_acc->type, TA_SELECT);
  test_assert_int_equal (string_equal (select_acc->select.key, key2), true);
  test_assert_int_equal (select_acc->select.sub_ta->type, TA_TAKE);

  BUILDER_CLOSE (b);
  ALLOC_CLOSE (alloc);
}
#endif
