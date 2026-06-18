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

#include "types.h"

#include "compiler.h"
#include "error.h"
#include "numerics.h"
#include "testing/testing.h"
#include "variables.h"

/******************************************************************************
 * SECTION: Types
 * ----------------------------------------------------------------------------
 * @brief Common Type wrapper code
 ******************************************************************************/

DEFINE_DBG_ASSERT (struct type, unchecked_type, t, { ASSERT (t); })

DEFINE_DBG_ASSERT (struct type, valid_type, t, {
  ASSERT (t);
  ASSERT (type_validate (t, NULL) == SUCCESS);
})

err_t
type_validate (const struct type *t, error *e)
{
  DBG_ASSERT (unchecked_type, t);
  switch (t->type)
  {
    case T_PRIM:
    {
      return prim_t_validate (&t->p, e);
    }
    case T_STRUCT:
    {
      return struct_t_validate (&t->st, e);
    }
    case T_UNION:
    {
      return union_t_validate (&t->un, e);
    }
    case T_SARRAY:
    {
      return sarray_t_validate (&t->sa, e);
    }
    default:
    {
      UNREACHABLE ();
      return -1;
    }
  }
}

i32
type_snprintf (char *str, u32 size, struct type *t)
{
  DBG_ASSERT (valid_type, t);

  switch (t->type)
  {
    case T_PRIM:
    {
      return prim_t_snprintf (str, size, &t->p);
    }
    case T_STRUCT:
    {
      return struct_t_snprintf (str, size, &t->st);
    }
    case T_UNION:
    {
      return union_t_snprintf (str, size, &t->un);
    }
    case T_SARRAY:
    {
      return sarray_t_snprintf (str, size, &t->sa);
    }
    default:
    {
      UNREACHABLE ();
      return -1;
    }
  }
}

char *
type_tostr (struct type *t)
{
  int len = type_snprintf (NULL, 0, t);
  if (len < 0)
  {
    return NULL;
  }

  char *msg = i_malloc (len + 1, 1, NULL);
  if (msg == NULL)
  {
    return NULL;
  }

  if (type_snprintf (msg, len + 1, t) < 0)
  {
    i_free (msg);
    return NULL;
  }

  return msg;
}

u32
type_byte_size (const struct type *t)
{
  DBG_ASSERT (valid_type, t);

  switch (t->type)
  {
    case T_PRIM:
    {
      return prim_t_byte_size (&t->p);
    }

    case T_STRUCT:
    {
      return struct_t_byte_size (&t->st);
    }

    case T_UNION:
    {
      return union_t_byte_size (&t->un);
    }

    case T_SARRAY:
    {
      return sarray_t_byte_size (&t->sa);
    }

    default:
    {
      UNREACHABLE ();
      return 0;
    }
  }
}

u32
type_get_string_size (const struct type *t)
{
  DBG_ASSERT (valid_type, t);

  switch (t->type)
  {
    case T_PRIM:
    {
      // Room for largest primitive name (e.g., "cf128") + 1 null terminator
      return sizeof ("cf128");
    }

    case T_STRUCT:
    case T_UNION:
    {
      // "struct { }" or "union { }"
      u32 base_len =
          (t->type == T_STRUCT) ? sizeof ("struct { }") : sizeof ("union { }");
      u32 sublen = 0;

      // Accessing st or un identically since they share identical structural
      // layouts
      u16 len = t->st.len;
      for (u16 i = 0; i < len; ++i)
      {
        // Size of the key text + " " + string length of subtype
        sublen += t->st.keys[i].len + 1 + type_get_string_size (t->st.types[i]);

        if (i < len - 1)
        {
          sublen += sizeof (", ") - 1;
        }
      }
      return base_len + sublen;
    }

    case T_SARRAY:
    {
      u32 sublen = 0;
      // Calculate brackets layout string overhead "[10][200]..."
      for (u32 i = 0; i < t->sa.rank; ++i)
      {
        // Accounts for digits up to 4 billion + brackets '[' and ']'
        sublen += sizeof ("[4294967295]") - 1;
      }
      // Add a single space separation " TYPE"
      return sublen + 1 + type_get_string_size (t->sa.t);
    }

    default:
    {
      UNREACHABLE ();
      return 0;
    }
  }
}

static char *
type_generate_string_rec (char *dest, char *end, const struct type *t)
{
  switch (t->type)
  {
    case T_PRIM:
    {
      int n = snprintf (dest, (size_t)(end - dest), "%s", prim_to_str (t->p));
      return dest + (n > 0 ? n : 0);
    }
    case T_STRUCT:
    case T_UNION:
    {
      char *p = dest;
      int   n = snprintf (
          p,
          (size_t)(end - p),
          "%s { ",
          (t->type == T_STRUCT) ? "struct" : "union"
      );
      p += (n > 0 ? n : 0);

      u16 len = t->st.len;
      for (u16 i = 0; i < len; ++i)
      {
        n = snprintf (
            p,
            (size_t)(end - p),
            "%.*s ",
            t->st.keys[i].len,
            t->st.keys[i].data
        );
        p += (n > 0 ? n : 0);

        p = type_generate_string_rec (p, end, t->st.types[i]);

        if (i < len - 1)
        {
          n = snprintf (p, (size_t)(end - p), ", ");
          p += (n > 0 ? n : 0);
        }
      }
      n = snprintf (p, (size_t)(end - p), " }");
      p += (n > 0 ? n : 0);
      return p;
    }
    case T_SARRAY:
    {
      char *p = dest;
      for (u16 i = 0; i < t->sa.rank; ++i)
      {
        int n = snprintf (p, (size_t)(end - p), "[%u]", t->sa.dims[i]);
        p += (n > 0 ? n : 0);
      }
      int n = snprintf (p, (size_t)(end - p), " ");
      p += (n > 0 ? n : 0);
      return type_generate_string_rec (p, end, t->sa.t);
    }
    default:
    {
      UNREACHABLE ();
      return dest;
    }
  }
}

// Public interface function entry point
void
type_generate_string (char *dest, const struct type *t)
{
  DBG_ASSERT (valid_type, t);
  if (dest)
  {
    type_generate_string_rec (dest, dest + type_get_string_size (t), t);
  }
}

#ifndef NTEST
TEST (type_generate_string)
{
  TEST_CASE ("primitive")
  {
    struct type t            = {.type = T_PRIM, .p = CF128};
    const char *expected     = "cf128";
    u32         expected_len = (u32)strlen (expected);

    char buf[64];
    type_generate_string (buf, &t);

    test_assert_int_equal (memcmp (buf, expected, expected_len + 1) == 0, 1);
  }

  TEST_CASE ("sarray")
  {
    struct type element = {.type = T_PRIM, .p = I32};
    u32         dims[3] = {5, 20, 100};
    struct type t       = {
        .type = T_SARRAY,
        .sa   = {.rank = 3, .dims = dims, .t = &element}
    };
    const char *expected     = "[5][20][100] i32";
    u32         expected_len = (u32)strlen (expected);

    char buf[64];
    type_generate_string (buf, &t);

    test_assert_int_equal (memcmp (buf, expected, expected_len + 1) == 0, 1);
  }

  TEST_CASE ("struct")
  {
    struct string keys[2]  = {{.data = "x", .len = 1}, {.data = "y", .len = 1}};
    struct type   f1       = {.type = T_PRIM, .p = F32};
    struct type   f2       = {.type = T_PRIM, .p = F32};
    struct type  *types[2] = {&f1, &f2};

    struct type t = {
        .type = T_STRUCT,
        .st   = {.len = 2, .keys = keys, .types = types}
    };
    const char *expected     = "struct { x f32, y f32 }";
    u32         expected_len = (u32)strlen (expected);

    char buf[64];
    type_generate_string (buf, &t);

    test_assert_int_equal (memcmp (buf, expected, expected_len + 1) == 0, 1);
  }

  TEST_CASE ("union")
  {
    struct string keys[2] = {
        {.data = "as_int", .len = 6},
        {.data = "as_ptr", .len = 6}
    };
    struct type  f1       = {.type = T_PRIM, .p = I64};
    struct type  f2       = {.type = T_PRIM, .p = U64};
    struct type *types[2] = {&f1, &f2};

    struct type t = {
        .type = T_UNION,
        .un   = {
            .len   = 2,
            .keys  = keys,
            .types = types
        } // Using .un overlay explicitly
    };
    const char *expected     = "union { as_int i64, as_ptr u64 }";
    u32         expected_len = (u32)strlen (expected);

    char buf[128];
    type_generate_string (buf, &t);

    test_assert_int_equal (memcmp (buf, expected, expected_len + 1) == 0, 1);
  }

  TEST_CASE ("complex_nested")
  {
    // Sub-component A: union { raw u8, state i32 }
    struct string un_keys[2] = {
        {.data = "raw", .len = 3},
        {.data = "state", .len = 5}
    };
    struct type  prim_u8     = {.type = T_PRIM, .p = U8};
    struct type  prim_i32    = {.type = T_PRIM, .p = I32};
    struct type *un_types[2] = {&prim_u8, &prim_i32};
    struct type  inner_union = {
        .type = T_UNION,
        .un   = {.len = 2, .keys = un_keys, .types = un_types}
    };

    // Sub-component B: [5] cf32
    struct type prim_cf32         = {.type = T_PRIM, .p = CF32};
    u32         inner_arr_dims[1] = {5};
    struct type inner_array       = {
        .type = T_SARRAY,
        .sa   = {.rank = 1, .dims = inner_arr_dims, .t = &prim_cf32}
    };

    // Parent Struct: struct { payload <union>, tags <array> }
    struct string st_keys[2] = {
        {.data = "payload", .len = 7},
        {.data = "tags", .len = 4}
    };
    struct type *st_types[2]   = {&inner_union, &inner_array};
    struct type  parent_struct = {
        .type = T_STRUCT,
        .st   = {.len = 2, .keys = st_keys, .types = st_types}
    };

    // Root Array: [2] <struct>
    u32         root_dims[1] = {2};
    struct type root_type    = {
        .type = T_SARRAY,
        .sa   = {.rank = 1, .dims = root_dims, .t = &parent_struct}
    };

    const char *expected =
        "[2] struct { payload union { raw u8, state i32 }, tags [5] cf32 }";
    u32 expected_len = (u32)strlen (expected);

    // Verify type_get_string_size returns enough space for safe serialization
    u32 calculated_size = type_get_string_size (&root_type);
    test_assert_int_equal (calculated_size >= expected_len + 1, 1);

    char buf[256];
    type_generate_string (buf, &root_type);

    test_assert_int_equal (memcmp (buf, expected, expected_len + 1) == 0, 1);
  }
}
#endif

u32
type_get_serial_size (const struct type *t)
{
  DBG_ASSERT (valid_type, t);

  // LABEL TYPE
  u32 ret = sizeof (u8);

  switch (t->type)
  {
    case T_PRIM:
    {
      return ret + sizeof (u8);
    }
    case T_STRUCT:
    {
      return ret + struct_t_get_serial_size (&t->st);
    }
    case T_UNION:
    {
      return ret + union_t_get_serial_size (&t->un);
    }
    case T_SARRAY:
    {
      return ret + sarray_t_get_serial_size (&t->sa);
    }
    default:
    {
      UNREACHABLE ();
      return 0;
    }
  }
}

void
type_serialize (struct serializer *dest, const struct type *src)
{
  DBG_ASSERT (valid_type, src);
  bool ret;

  u8 type_val = (u8)src->type;
  ret         = srlizr_write (dest, &type_val, sizeof (u8));
  ASSERT (ret);

  switch (src->type)
  {
    case T_PRIM:
    {
      prim_t_serialize (dest, &src->p);
      break;
    }
    case T_STRUCT:
    {
      struct_t_serialize (dest, &src->st);
      break;
    }
    case T_UNION:
    {
      union_t_serialize (dest, &src->un);
      break;
    }
    case T_SARRAY:
    {
      sarray_t_serialize (dest, &src->sa);
      break;
    }
    default:
    {
      UNREACHABLE ();
      break;
    }
  }
}

struct type *
type_deserialize (struct deserializer *src, struct chunk_alloc *alloc, error *e)
{
  u8           header;
  struct type *dest = chunk_malloc (alloc, 1, sizeof *dest, e);
  if (dest == NULL)
  {
    return NULL;
  }
  bool ret   = dsrlizr_read (&header, sizeof (u8), src);
  dest->type = (enum type_t)header;

  switch (header)
  {
    case T_PRIM:
    {
      if (prim_t_deserialize (&dest->p, src, e))
      {
        return NULL;
      }
      return dest;
    }
    case T_STRUCT:
    {
      if (struct_t_deserialize (&dest->st, src, alloc, e))
      {
        return NULL;
      }
      return dest;
    }
    case T_UNION:
    {
      if (union_t_deserialize (&dest->un, src, alloc, e))
      {
        return NULL;
      }
      return dest;
    }
    case T_SARRAY:
    {
      if (sarray_t_deserialize (&dest->sa, src, alloc, e))
      {
        return NULL;
      }
      return dest;
    }
    default:
    {
      if (error_causef (e, ERR_INTERP, "Unknown type code: %d", ret))
      {
        return NULL;
      }
      return dest;
    }
  }
}

struct type *
type_random (struct chunk_alloc *alloc, u32 depth, error *e)
{
  struct type *dest = chunk_malloc (alloc, 1, sizeof *dest, e);
  if (dest == NULL)
  {
    return NULL;
  }

  if (depth == 0)
  {
    dest->type = T_PRIM;
    dest->p    = prim_t_random ();
    return dest;
  }

  static const enum type_t weighted[] = {T_PRIM, T_STRUCT, T_UNION, T_SARRAY};

  dest->type = weighted[randu32r (0, arrlen (weighted) - 1)];

  switch (dest->type)
  {
    case T_PRIM:
    {
      dest->p = prim_t_random ();
      return dest;
    }

    case T_STRUCT:
    {
      if (struct_t_random (&dest->st, alloc, depth, e))
      {
        return NULL;
      }
      return dest;
    }

    case T_UNION:
    {
      if (union_t_random (&dest->un, alloc, depth, e))
      {
        return NULL;
      }
      return dest;
    }

    case T_SARRAY:
    {
      if (sarray_t_random (&dest->sa, alloc, depth, e))
      {
        return NULL;
      }
      return dest;
    }

    default:
    {
      error_causef (e, ERR_NOMEM, "invalid type tag");
      return NULL;
    }
  }
  UNREACHABLE ();
}

bool
type_equal (const struct type *left, const struct type *right)
{
  if (left->type != right->type)
  {
    return false;
  }

  switch (left->type)
  {
    case T_PRIM:
    {
      return left->p == right->p;
    }
    case T_STRUCT:
    {
      return struct_t_equal (&left->st, &right->st);
    }
    case T_UNION:
    {
      return union_t_equal (&left->un, &right->un);
    }
    case T_SARRAY:
    {
      return sarray_t_equal (&left->sa, &right->sa);
    }
    default:
    {
      UNREACHABLE ();
    }
  }
}

err_t
i_log_type (struct type *t, error *e)
{
  i32 len = type_snprintf (NULL, 0, t);
  if (len < 0)
  {
    return error_causef (e, ERR_IO, "snprintf failed");
  }

  char *dest = i_malloc (len + 1, sizeof *dest, e);
  if (dest == NULL)
  {
    return error_causef (e, ERR_NOMEM, "alloc failed for type log string");
  }

  len = type_snprintf (dest, len + 1, t);
  if (len < 0)
  {
    i_free (dest);
    return error_causef (e, ERR_IO, "snprintf failed");
  }

  i_log_info ("%.*s\n", len, dest);
  i_free (dest);

  return SUCCESS;
}

static struct string
string_movemem (struct string src, struct chunk_alloc *alloc, error *e)
{
  char *data = chunk_alloc_move_mem (alloc, src.data, src.len, e);
  if (!data)
  {
    return (struct string){0};
  }
  return (struct string){.data = data, .len = src.len};
}

static struct string *
keylist_movemem (
    struct string      *src,
    u32                 len,
    struct chunk_alloc *alloc,
    error              *e
)
{
  struct string *keys = chunk_malloc (alloc, len, sizeof *keys, e);
  if (!keys)
  {
    return NULL;
  }

  for (u32 i = 0; i < len; ++i)
  {
    keys[i] = string_movemem (src[i], alloc, e);
    if (!keys[i].data)
    {
      return NULL;
    }
  }
  return keys;
}

static struct type **
typelist_movemem (
    struct type       **src,
    u32                 len,
    struct chunk_alloc *alloc,
    error              *e
)
{
  struct type **types = chunk_malloc (alloc, len, sizeof (struct type *), e);
  if (!types)
  {
    return NULL;
  }

  for (u32 i = 0; i < len; ++i)
  {
    types[i] = type_movemem (src[i], alloc, e);
    if (!types[i])
    {
      return NULL;
    }
  }
  return types;
}

struct type *
type_movemem (struct type *src, struct chunk_alloc *alloc, error *e)
{
  struct type *ret = chunk_malloc (alloc, 1, sizeof *ret, e);
  if (!ret)
  {
    return NULL;
  }

  ret->type = src->type;

  switch (src->type)
  {
    case T_PRIM:
    {
      ret->p = src->p;
      break;
    }
    case T_STRUCT:
    {
      ret->st.len  = src->st.len;
      ret->st.keys = keylist_movemem (src->st.keys, src->st.len, alloc, e);
      if (!ret->st.keys)
      {
        return NULL;
      }
      ret->st.types = typelist_movemem (src->st.types, src->st.len, alloc, e);
      if (!ret->st.types)
      {
        return NULL;
      }
      break;
    }
    case T_UNION:
    {
      ret->un.len  = src->un.len;
      ret->un.keys = keylist_movemem (src->un.keys, src->un.len, alloc, e);
      if (!ret->un.keys)
      {
        return NULL;
      }
      ret->un.types = typelist_movemem (src->un.types, src->un.len, alloc, e);
      if (!ret->un.types)
      {
        return NULL;
      }
      break;
    }
    case T_SARRAY:
    {
      ret->sa.rank = src->sa.rank;
      ret->sa.t    = type_movemem (src->sa.t, alloc, e);
      if (ret->sa.t == NULL)
      {
        return NULL;
      }
      ret->sa.dims =
          chunk_malloc (alloc, src->sa.rank, sizeof *ret->sa.dims, e);
      if (!ret->sa.dims)
      {
        return NULL;
      }
      memcpy (ret->sa.dims, src->sa.dims, src->sa.rank * sizeof *ret->sa.dims);
      break;
    }
  }

  return ret;
}

struct type *
type_malloc_copy (struct type *t, struct malloc_plan *plan)
{
  bool active = plan->mode == MP_ALLOCING;

  /**
   * [ type ]
   */
  struct type *ret = malloc_plan_memcpy (plan, t, sizeof (struct type));

  switch (t->type)
  {
    case T_PRIM:
    {
      return ret;
    }
    case T_STRUCT:
    {
      struct string *keys =
          malloc_plan_memcpy (plan, t->st.keys, t->st.len * sizeof *t->st.keys);
      struct type **types = malloc_plan_memcpy (
          plan,
          t->st.types,
          t->st.len * sizeof (struct type *)
      );

      if (active)
      {
        ret->st.keys  = keys;
        ret->st.types = types;
      }

      for (u32 i = 0; i < t->st.len; ++i)
      {
        void *data =
            malloc_plan_memcpy (plan, t->st.keys[i].data, t->st.keys[i].len);
        struct type *type = type_malloc_copy (t->st.types[i], plan);

        // Change pointers
        if (active)
        {
          keys[i].data = data;
          types[i]     = type;
        }
      }
      return ret;
    }
    case T_UNION:
    {
      struct string *keys =
          malloc_plan_memcpy (plan, t->un.keys, t->un.len * sizeof *t->un.keys);
      struct type **types = malloc_plan_memcpy (
          plan,
          t->un.types,
          t->un.len * sizeof (struct type *)
      );

      if (active)
      {
        ret->un.keys  = keys;
        ret->un.types = types;
      }

      for (u32 i = 0; i < t->un.len; ++i)
      {
        void *data =
            malloc_plan_memcpy (plan, t->un.keys[i].data, t->un.keys[i].len);
        struct type *type = type_malloc_copy (t->un.types[i], plan);

        // Change pointers
        if (active)
        {
          keys[i].data = data;
          types[i]     = type;
        }
      }
      return ret;
    }
    case T_SARRAY:
    {
      u32 *dims = malloc_plan_memcpy (
          plan,
          t->sa.dims,
          t->sa.rank * sizeof *t->sa.dims
      );

      if (active)
      {
        ret->sa.dims = dims;
      }

      struct type *type = type_malloc_copy (t->sa.t, plan);
      if (active)
      {
        ret->sa.t = type;
      }

      return ret;
    }
  }
  UNREACHABLE ();
}

#ifndef NTEST
TEST (type_malloc_copy)
{
  struct type t = {
      .type = T_PRIM,
      .p    = U32,
  };

  struct malloc_plan plan = malloc_plan_create ();
  type_malloc_copy (&t, &plan);
  malloc_plan_alloc (&plan, NULL);
  struct type *_t = type_malloc_copy (&t, &plan);
  test_assert_int_equal (plan.size, sizeof (struct type));
  test_assert (type_equal (_t, &t));
  i_free (_t);

  t = (struct type){
      .type = T_STRUCT,
      .st   = (struct struct_t){
          .len = 2,
          .keys =
              (struct string[]){
                  {
                      .len  = strlen ("hello"),
                      .data = "hello",
                  },
                  {
                      .len  = strlen ("world"),
                      .data = "world",
                  },
              },
          .types = (struct type *[]){
              &(struct type){
                  .type = T_PRIM,
                  .p    = U32,
              },
              &(struct type){
                  .type = T_PRIM,
                  .p    = U32,
              },
          },
      },
  };

  u32 exp = 3 * sizeof (struct type) + strlen ("hello") + strlen ("world")
            + 2 * (sizeof (struct string) + sizeof (struct type *));

  plan = malloc_plan_create ();
  type_malloc_copy (&t, &plan);
  malloc_plan_alloc (&plan, NULL);
  _t = type_malloc_copy (&t, &plan);
  test_assert_int_equal (plan.size, exp);
  test_assert (type_equal (_t, &t));
  i_free (_t);

  t = (struct type){
      .type = T_UNION,
      .un   = (struct union_t){
          .len = 2,
          .keys =
              (struct string[]){
                  {
                      .len  = strlen ("hello"),
                      .data = "hello",
                  },
                  {
                      .len  = strlen ("world"),
                      .data = "world",
                  },
              },
          .types = (struct type *[]){
              &(struct type){
                  .type = T_PRIM,
                  .p    = U32,
              },
              &(struct type){
                  .type = T_PRIM,
                  .p    = U32,
              },
          },
      },
  };

  exp = 3 * sizeof (struct type) + strlen ("hello") + strlen ("world")
        + 2 * (sizeof (struct string) + sizeof (struct type *));

  plan = malloc_plan_create ();
  type_malloc_copy (&t, &plan);
  malloc_plan_alloc (&plan, NULL);
  _t = type_malloc_copy (&t, &plan);
  test_assert_int_equal (plan.size, exp);
  test_assert (type_equal (_t, &t));
  i_free (_t);

  t = (struct type){
      .type = T_SARRAY,
      .sa   = (struct sarray_t){
          .rank = 10,
          .dims = (u32[]){0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
          .t    = &(struct type){
              .type = T_PRIM,
              .p    = U32,
          },
      },
  };

  exp = 2 * sizeof (struct type) + 10 * sizeof (u32);

  plan = malloc_plan_create ();
  type_malloc_copy (&t, &plan);
  malloc_plan_alloc (&plan, NULL);
  _t = type_malloc_copy (&t, &plan);
  test_assert_int_equal (plan.size, exp);
  test_assert (type_equal (_t, &t));
  i_free (_t);
}
#endif

/******************************************************************************
 * SECTION: Primitive Types
 * ----------------------------------------------------------------------------
 * @brief
 *
 *
 ******************************************************************************/

DEFINE_DBG_ASSERT (enum prim_t, prim_t, s, {
  ASSERT (s);
  error e = error_create ();
  ASSERT (prim_t_validate (s, &e) == SUCCESS);
})

err_t
prim_t_validate (const enum prim_t *t, error *e)
{
  ASSERT (t);
  if (!(*t <= CU128 && *t >= U8))
  {
    return error_causef (
        e,
        ERR_INTERP,
        "invalid prim type %d (valid range %d..%d)",
        *t,
        U8,
        CU128
    );
  }

  return SUCCESS;
}

#ifndef NTEST
TEST (prim_t_validate)
{
  error err = error_create ();

  // 1.1 happy path – legal value
  enum prim_t ok = U32;
  test_assert_int_equal (prim_t_validate (&ok, &err), SUCCESS);

  // 1.2 too‑small → ERR_INTERP
  enum prim_t bad_lo = (enum prim_t) (U8 - 1);
  test_assert_int_equal (prim_t_validate (&bad_lo, &err), ERR_INTERP);
  err.cause_code = SUCCESS;

  // 1.3 too‑large → ERR_INTERP
  enum prim_t bad_hi = (enum prim_t) (CU128 + 1);
  test_assert_int_equal (prim_t_validate (&bad_hi, &err), ERR_INTERP);
  err.cause_code = SUCCESS;
}
#endif

const char *
prim_to_str (enum prim_t p)
{
  DBG_ASSERT (prim_t, &p);
  switch (p)
  {
    case U8: return "u8";
    case U16: return "u16";
    case U32: return "u32";
    case U64: return "u64";

    case I8: return "i8";
    case I16: return "i16";
    case I32: return "i32";
    case I64: return "i64";

    case F16: return "f16";
    case F32: return "f32";
    case F64: return "f64";
    case F128: return "f128";

    case CF32: return "cf32";
    case CF64: return "cf64";
    case CF128: return "cf128";
    case CF256: return "cf256";

    case CI16: return "ci16";
    case CI32: return "ci32";
    case CI64: return "ci64";
    case CI128: return "ci128";

    case CU16: return "cu16";
    case CU32: return "cu32";
    case CU64: return "cu64";
    case CU128: return "cu128";
  }
  UNREACHABLE ();
  return "";
}

i32
prim_t_snprintf (char *str, u32 size, const enum prim_t *p)
{
  DBG_ASSERT (prim_t, p);

  char       *out   = str;
  u32         avail = size;
  int         len   = 0;
  int         n;
  const char *name = prim_to_str (*p);

  n = snprintf (out, avail, "%s", name);
  if (n < 0)
  {
    return n;
  }
  len += n;

  return len;
}

#ifndef NTEST
TEST (prim_t_snprintf)
{
  struct type t = {
      .type = T_PRIM,
      .p    = F64,
  };

  const char *expect = "f64";
  char       *ret    = type_tostr (&t);
  error       e      = error_create ();
  i_log_type (&t, &e);
  test_assert_int_equal (strncmp (expect, ret, strlen (expect)), 0);
  i_free (ret);
}
#endif

u32
prim_t_byte_size (const enum prim_t *t)
{
  DBG_ASSERT (prim_t, t);

  switch (*t)
  {
    case U8:
    case I8: return 1;

    case U16:
    case I16:
    case F16:
    case CI16:
    case CU16: return 2;

    case U32:
    case I32:
    case F32:
    case CF32:
    case CI32:
    case CU32: return 4;

    case U64:
    case I64:
    case F64:
    case CF64:
    case CI64:
    case CU64: return 8;

    case F128:
    case CF128:
    case CI128:
    case CU128: return 16;

    case CF256: return 32;
  }

  UNREACHABLE ();
  return 0;
}

void
prim_t_serialize (struct serializer *dest, const enum prim_t *src)
{
  DBG_ASSERT (prim_t, src);
  bool ret;

  // PRIM
  u8 prim_val = (u8)*src;
  ret         = srlizr_write (dest, (const u8 *)&prim_val, sizeof (u8));
  ASSERT (ret);
}
#ifndef NTEST
TEST (prim_t_byte_size)
{
  enum prim_t p1 = U8;
  enum prim_t p2 = CF128;
  enum prim_t p3 = CF256;

  test_assert_int_equal (prim_t_byte_size (&p1), 1);
  test_assert_int_equal (prim_t_byte_size (&p2), 16);
  test_assert_int_equal (prim_t_byte_size (&p3), 32);
}

#  ifndef NTEST
TEST (prim_t_serialize)
{
  enum prim_t       p = I16;
  u8                out[4];
  struct serializer s = srlizr_create (out, sizeof out);
  prim_t_serialize (&s, &p);

  u8 exp[] = {(u8)I16};
  test_assert_int_equal (s.dlen, sizeof exp);
  test_assert_int_equal (memcmp (out, exp, sizeof exp), 0);
}
#  endif
#endif

err_t
prim_t_deserialize (enum prim_t *dest, struct deserializer *src, error *e)
{
  ASSERT (dest);

  u8   p;
  bool ret = dsrlizr_read ((u8 *)&p, sizeof (u8), src);
  if (!ret)
  {
    return error_causef (e, ERR_CORRUPT, "prim: missing length header");
  }

  enum prim_t _p = p;

  WRAP (prim_t_validate (&_p, e));

  DBG_ASSERT (prim_t, &_p);

  *dest = _p;

  return SUCCESS;
}

#ifndef NTEST
TEST (prim_t_deserialize)
{
  // 5.1 green path
  u8                  data[] = {(u8)CI32};
  struct deserializer d      = dsrlizr_create (data, sizeof data);
  error               err    = error_create ();
  enum prim_t         out    = 0;

  test_assert_int_equal (prim_t_deserialize (&out, &d, &err), SUCCESS);
  test_assert_int_equal (out, CI32);

  // 5.2 red path – invalid enum value (CU128+1)
  u8                  bad[] = {(u8)(CU128 + 1)};
  struct deserializer d2    = dsrlizr_create (bad, sizeof bad);
  test_assert_int_equal (prim_t_deserialize (&out, &d2, &err), ERR_INTERP);
  err.cause_code = SUCCESS;
}
#endif

enum prim_t
prim_t_random (void)
{
  return (enum prim_t)randu32r (U8, CU128);
}

#ifndef NTEST
TEST (prim_t_random)
{
  error err = error_create ();
  for (u32 i = 0; i < 1000; ++i)
  {
    enum prim_t p = prim_t_random ();
    test_assert_int_equal (prim_t_validate (&p, &err), SUCCESS);
  }
}
#endif

enum prim_t
strtoprim (const char *text, u32 len)
{
  struct string str = {.data = (char *)text, .len = len};

  if (string_equal (str, strfcstr ("u8")))
  {
    return U8;
  }
  if (string_equal (str, strfcstr ("u16")))
  {
    return U16;
  }
  if (string_equal (str, strfcstr ("u32")))
  {
    return U32;
  }
  if (string_equal (str, strfcstr ("u64")))
  {
    return U64;
  }
  if (string_equal (str, strfcstr ("i8")))
  {
    return I8;
  }
  if (string_equal (str, strfcstr ("i16")))
  {
    return I16;
  }
  if (string_equal (str, strfcstr ("i32")))
  {
    return I32;
  }
  if (string_equal (str, strfcstr ("i64")))
  {
    return I64;
  }
  if (string_equal (str, strfcstr ("f16")))
  {
    return F16;
  }
  if (string_equal (str, strfcstr ("f32")))
  {
    return F32;
  }
  if (string_equal (str, strfcstr ("f64")))
  {
    return F64;
  }
  if (string_equal (str, strfcstr ("f128")))
  {
    return F128;
  }
  if (string_equal (str, strfcstr ("cf32")))
  {
    return CF32;
  }
  if (string_equal (str, strfcstr ("cf64")))
  {
    return CF64;
  }
  if (string_equal (str, strfcstr ("cf128")))
  {
    return CF128;
  }
  if (string_equal (str, strfcstr ("cf256")))
  {
    return CF256;
  }
  if (string_equal (str, strfcstr ("ci16")))
  {
    return CI16;
  }
  if (string_equal (str, strfcstr ("ci32")))
  {
    return CI32;
  }
  if (string_equal (str, strfcstr ("ci64")))
  {
    return CI64;
  }
  if (string_equal (str, strfcstr ("ci128")))
  {
    return CI128;
  }
  if (string_equal (str, strfcstr ("cu16")))
  {
    return CU16;
  }
  if (string_equal (str, strfcstr ("cu32")))
  {
    return CU32;
  }
  if (string_equal (str, strfcstr ("cu64")))
  {
    return CU64;
  }
  if (string_equal (str, strfcstr ("cu128")))
  {
    return CU128;
  }

  return (enum prim_t) - 1;
}

/******************************************************************************
 * SECTION: Struct
 * ----------------------------------------------------------------------------
 * @brief A structured type
 ******************************************************************************/

DEFINE_DBG_ASSERT (struct struct_t, unchecked_struct_t, s, {
  ASSERT (s);
  ASSERT (s->keys);
  ASSERT (s->types);
})

static err_t
struct_t_type_err (const char *msg, error *e)
{
  return error_causef (e, ERR_INTERP, "Struct: %s", msg);
}

static err_t
struct_t_type_deser (const char *msg, error *e)
{
  return error_causef (e, ERR_CORRUPT, "Struct: %s", msg);
}

static err_t
struct_t_validate_shallow (const struct struct_t *s, error *e)
{
  DBG_ASSERT (unchecked_struct_t, s);

  if (s->len == 0)
  {
    return struct_t_type_err ("Keys length must be > 0", e);
  }

  for (u32 i = 0; i < s->len; ++i)
  {
    if (s->keys[i].len == 0)
    {
      return struct_t_type_err ("Key length must be > 0", e);
    }
    ASSERT (s->keys[i].data);
  }

  if (!strings_all_unique (s->keys, s->len))
  {
    return struct_t_type_err ("Duplicate keys", e);
  }

  return SUCCESS;
}

DEFINE_DBG_ASSERT (struct struct_t, valid_struct_t, s, {
  error e = error_create ();
  ASSERT (struct_t_validate_shallow (s, &e) == SUCCESS);
})

err_t
struct_t_create (
    struct struct_t    *dest,
    struct kvt_list     list,
    struct chunk_alloc *dalloc,
    error              *e
)
{
  if (list.len == 0)
  {
    return struct_t_type_err ("struct must have greater than 0 keys", e);
  }

  // Copy stuff over
  if (dalloc)
  {
    dest->len  = list.len;
    dest->keys = chunk_alloc_move_mem (
        dalloc,
        list.keys,
        list.len * sizeof *dest->keys,
        e
    );
    if (dest->keys == NULL)
    {
      return error_trace (e);
    }

    dest->types = chunk_alloc_move_mem (
        dalloc,
        list.types,
        list.len * sizeof (struct type *),
        e
    );
    if (dest->keys == NULL)
    {
      return error_trace (e);
    }
  }

  // Don't copy
  else
  {
    dest->len   = list.len;
    dest->keys  = list.keys;
    dest->types = list.types;
  }

  return SUCCESS;
}

err_t
struct_t_validate (const struct struct_t *s, error *e)
{
  WRAP (struct_t_validate_shallow (s, e));
  {
    return false;
  }
  for (u32 i = 0; i < s->len; ++i)
  {
    WRAP (type_validate (s->types[i], e));
    {
      return false;
    }
  }
  return true;
}

i32
struct_t_snprintf (char *str, u32 size, const struct struct_t *st)
{
  DBG_ASSERT (valid_struct_t, st);

  char *out   = str;
  u32   avail = size;
  int   len   = 0;
  int   n;

  n = snprintf (out, avail, "struct { ");
  if (n < 0)
  {
    return n;
  }
  len += n;
  if (out)
  {
    out += n;
    if ((u32)n < avail)
    {
      avail -= n;
    }
    else
    {
      avail = 0;
    }
  }

  for (u32 i = 0; i < st->len; ++i)
  {
    struct string key = st->keys[i];
    n                 = snprintf (out, avail, "%.*s ", key.len, key.data);
    if (n < 0)
    {
      return n;
    }
    len += n;
    if (out)
    {
      out += n;
      if ((u32)n < avail)
      {
        avail -= n;
      }
      else
      {
        avail = 0;
      }
    }

    n = type_snprintf (out, avail, st->types[i]);
    if (n < 0)
    {
      return n;
    }
    len += n;
    if (out)
    {
      out += n;
      if ((u32)n < avail)
      {
        avail -= n;
      }
      else
      {
        avail = 0;
      }
    }

    if (i + 1 < st->len)
    {
      n = snprintf (out, avail, ", ");
      if (n < 0)
      {
        return n;
      }
      len += n;
      if (out)
      {
        out += n;
        if ((u32)n < avail)
        {
          avail -= n;
        }
        else
        {
          avail = 0;
        }
      }
    }
  }

  n = snprintf (out, avail, " }");
  if (n < 0)
  {
    return n;
  }
  len += n;

  return len;
}

#ifndef NTEST
TEST (struct_t_snprintf)
{
  struct struct_t st;
  st.len  = 4;
  st.keys = (struct string[]){
      {
          .len  = 3,
          .data = "foo",
      },
      {
          .len  = 2,
          .data = "fo",
      },
      {
          .len  = 4,
          .data = "baro",
      },
      {
          .len  = 5,
          .data = "bazbi",
      },
  };
  st.types = (struct type *[]){
      &(struct type){
          .type = T_PRIM,
          .p    = U32,
      },
      &(struct type){
          .type = T_PRIM,
          .p    = U8,
      },
      &(struct type){
          .type = T_PRIM,
          .p    = U16,
      },
      &(struct type){
          .type = T_PRIM,
          .p    = CF128,
      },
  };

  struct type t = {
      .type = T_STRUCT,
      .st   = st,
  };

  const char *expected = "struct { foo u32, fo u8, baro u16, bazbi cf128 }";

  char *ret = type_tostr (&t);
  error e   = error_create ();
  i_log_type (&t, &e);
  test_assert_int_equal (strncmp (expected, ret, strlen (expected)), 0);
  i_free (ret);
}
#endif

u32
struct_t_byte_size (const struct struct_t *t)
{
  DBG_ASSERT (valid_struct_t, t);
  u32 ret = 0;

  // Each type is layed out contiguously
  for (u32 i = 0; i < t->len; ++i)
  {
    ret += type_byte_size (t->types[i]);
  }

  return ret;
}

#ifndef NTEST
TEST (struct_t_byte_size)
{
  struct struct_t st;
  st.len  = 4;
  st.keys = (struct string[]){
      {
          .len  = 3,
          .data = "foo",
      },
      {
          .len  = 2,
          .data = "fo",
      },
      {
          .len  = 4,
          .data = "baro",
      },
      {
          .len  = 5,
          .data = "bazbi",
      },
  };
  st.types = (struct type *[]){
      &(struct type){
          .type = T_PRIM,
          .p    = U32,
      },
      &(struct type){
          .type = T_PRIM,
          .p    = U8,
      },
      &(struct type){
          .type = T_PRIM,
          .p    = U16,
      },
      &(struct type){
          .type = T_PRIM,
          .p    = CF128,
      },
  };

  u64 act = struct_t_byte_size (&st);
  u64 exp = (sizeof (u32) + sizeof (u8) + sizeof (u16) + sizeof (cf128));

  test_assert_int_equal (exp, act);
}
#endif

u32
struct_t_get_serial_size (const struct struct_t *t)
{
  DBG_ASSERT (valid_struct_t, t);
  u32 ret = 0;

  // LEN (KLEN KEY) (TYPE) (KLEN KEY) (TYPE) ....
  ret += sizeof (u16);
  ret += t->len * sizeof (u16);

  for (u32 i = 0; i < t->len; ++i)
  {
    ret += t->keys[i].len;
    ret += type_get_serial_size (t->types[i]);
  }

  return ret;
}

#ifndef NTEST
TEST (struct_t_get_serial_size)
{
  struct struct_t st;
  st.len  = 4;
  st.keys = (struct string[]){
      {
          .len  = 3,
          .data = "foo",
      },
      {
          .len  = 2,
          .data = "fo",
      },
      {
          .len  = 4,
          .data = "baro",
      },
      {
          .len  = 5,
          .data = "bazbi",
      },
  };
  st.types = (struct type *[]){
      &(struct type){
          .type = T_PRIM,
          .p    = U32,
      },
      &(struct type){
          .type = T_PRIM,
          .p    = U8,
      },
      &(struct type){
          .type = T_PRIM,
          .p    = U16,
      },
      &(struct type){
          .type = T_PRIM,
          .p    = CF128,
      },
  };

  u64 act = struct_t_get_serial_size (&st);
  u64 exp = 2 + 4 * 2 + 3 + 2 + 4 + 5 + 4 * 2;

  test_assert_int_equal (exp, act);
}
#endif

void
struct_t_serialize (struct serializer *dest, const struct struct_t *src)
{
  DBG_ASSERT (valid_struct_t, src);
  bool ret;

  // LEN (KLEN KEY) (TYPE) (KLEN KEY) (TYPE) ....
  ret = srlizr_write (dest, (const u8 *)&src->len, sizeof (u16));
  ASSERT (ret);

  for (u32 i = 0; i < src->len; ++i)
  {
    // (KLEN
    struct string next = src->keys[i];
    ret = srlizr_write (dest, (const u8 *)&next.len, sizeof (u16));
    ASSERT (ret);

    // KEY)
    ret = srlizr_write (dest, (u8 *)next.data, next.len);
    ASSERT (ret);

    // (TYPE)
    type_serialize (dest, src->types[i]);
  }
}

#ifndef NTEST
TEST (struct_t_serialize)
{
  struct struct_t st;
  st.len  = 4;
  st.keys = (struct string[]){
      {
          .len  = 3,
          .data = "foo",
      },
      {
          .len  = 2,
          .data = "fo",
      },
      {
          .len  = 4,
          .data = "baro",
      },
      {
          .len  = 5,
          .data = "bazbi",
      },
  };
  st.types = (struct type *[]){
      &(struct type){
          .type = T_PRIM,
          .p    = U32,
      },
      &(struct type){
          .type = T_PRIM,
          .p    = U8,
      },
      &(struct type){
          .type = T_PRIM,
          .p    = U16,
      },
      &(struct type){
          .type = T_PRIM,
          .p    = CF128,
      },
  };

  u8  act[200]; // Sloppy sizing
  u8  exp[] = {0,       0,   0,   0,   'f', 'o',        'o',        (u8)T_PRIM,
               (u8)U32, 0,   0,   'f', 'o', (u8)T_PRIM, (u8)U8,     0,
               0,       'b', 'a', 'r', 'o', (u8)T_PRIM, (u8)U16,    0,
               0,       'b', 'a', 'z', 'b', 'i',        (u8)T_PRIM, (u8)CF128};
  u16 len   = 4;
  u16 l0    = 3;
  u16 l2    = 2;
  u16 l3    = 4;
  u16 l4    = 5;
  memcpy (&exp[0], &len, sizeof (u16));
  memcpy (&exp[2], &l0, sizeof (u16));
  memcpy (&exp[9], &l2, sizeof (u16));
  memcpy (&exp[15], &l3, sizeof (u16));
  memcpy (&exp[23], &l4, sizeof (u16));

  // Expected
  struct serializer s = srlizr_create (act, 200);
  struct_t_serialize (&s, &st);

  test_assert_int_equal (s.dlen, sizeof (exp));
  test_assert_int_equal (memcmp (act, exp, sizeof (exp)), 0);
}
#endif

err_t
struct_t_deserialize (
    struct struct_t     *dest,
    struct deserializer *src,
    struct chunk_alloc  *a,
    error               *e
)
{
  ASSERT (dest);

  struct chunk_alloc temp;
  chunk_alloc_create_default (&temp);

  struct kvt_list_builder unb;
  kvlb_create (&unb, &temp, a);

  // LEN
  u16 len;
  if (!dsrlizr_read ((u8 *)&len, sizeof (u16), src))
  {
    goto early_termination;
  }

  for (u32 i = 0; i < len; ++i)
  {
    // Read the string key length
    u16 klen;
    if (!dsrlizr_read ((u8 *)&klen, sizeof (u16), src))
    {
      goto early_termination;
    }

    struct string key = {
        .len  = klen,
        .data = chunk_malloc (a, key.len, 1, e),
    };
    // Read the string data
    if (key.data == NULL)
    {
      goto theend;
    }
    if (!dsrlizr_read ((u8 *)key.data, key.len, src))
    {
      goto early_termination;
    }

    // Deserialize sub type
    struct type *t = type_deserialize (src, a, e);
    if (t == NULL)
    {
      goto theend;
    }

    if (unlikely ((kvlb_accept_key (&unb, key, e)) < SUCCESS))
    {
      goto theend;
    }
    if (unlikely ((kvlb_accept_type (&unb, t, e)) < SUCCESS))
    {
      goto theend;
    }
  }

  struct kvt_list list;
  if (unlikely ((kvlb_build (&list, &unb, e)) < SUCCESS))
  {
    goto theend;
  }

  if (unlikely ((struct_t_create (dest, list, NULL, e)) < SUCCESS))
  {
    goto theend;
  }

theend:
  chunk_alloc_free_all (&temp);
  return error_trace (e);

early_termination:
  chunk_alloc_free_all (&temp);
  return struct_t_type_deser ("Early end of serialized string", e);
}

#ifndef NTEST
TEST (struct_t_deserialize_green_path)
{
  u8  data[] = {0,       0,   0,   0,   'f', 'o',        'o',        (u8)T_PRIM,
                (u8)U32, 0,   0,   'f', 'o', (u8)T_PRIM, (u8)U8,     0,
                0,       'b', 'a', 'r', 'o', (u8)T_PRIM, (u8)U16,    0,
                0,       'b', 'a', 'z', 'b', 'i',        (u8)T_PRIM, (u8)CF128};
  u16 len    = 4;
  u16 l0     = 3;
  u16 l2     = 2;
  u16 l3     = 4;
  u16 l4     = 5;
  memcpy (&data[0], &len, sizeof (u16));
  memcpy (&data[2], &l0, sizeof (u16));
  memcpy (&data[9], &l2, sizeof (u16));
  memcpy (&data[15], &l3, sizeof (u16));
  memcpy (&data[23], &l4, sizeof (u16));

  struct chunk_alloc st_alloc;
  chunk_alloc_create_default (&st_alloc);

  struct deserializer d = dsrlizr_create (data, sizeof (data));

  error e = error_create ();

  struct struct_t eret;
  err_t           ret = struct_t_deserialize (&eret, &d, &st_alloc, &e);

  test_assert_int_equal (ret, SUCCESS);

  test_assert_int_equal (eret.len, 4);

  test_assert_int_equal (eret.keys[0].len, 3);
  test_assert_int_equal (memcmp (eret.keys[0].data, "foo", 3), 0);
  test_assert_int_equal (eret.types[0]->type, T_PRIM);
  test_assert_int_equal (eret.types[0]->p, U32);

  test_assert_int_equal (eret.keys[1].len, 2);
  test_assert_int_equal (memcmp (eret.keys[1].data, "fo", 2), 0);
  test_assert_int_equal (eret.types[1]->type, T_PRIM);
  test_assert_int_equal (eret.types[1]->p, U8);

  test_assert_int_equal (eret.keys[2].len, 4);
  test_assert_int_equal (memcmp (eret.keys[2].data, "baro", 4), 0);
  test_assert_int_equal (eret.types[2]->type, T_PRIM);
  test_assert_int_equal (eret.types[2]->p, U16);

  test_assert_int_equal (eret.keys[3].len, 5);
  test_assert_int_equal (memcmp (eret.keys[3].data, "bazbi", 5), 0);
  test_assert_int_equal (eret.types[3]->type, T_PRIM);
  test_assert_int_equal (eret.types[3]->p, CF128);

  chunk_alloc_free_all (&st_alloc);
}
#endif

#ifndef NTEST
TEST (struct_t_deserialize_red_path)
{
  u8 data[] = {
      0,
      0, // Total length (4)
      0,
      0,
      'f',
      'o',
      'o',
      (u8)T_PRIM,
      (u8)U32, // STRLEN STRING
               // (sub) TYPE
      0,
      0,
      'f',
      'o',
      'o',
      (u8)T_PRIM,
      (u8)U8,
      0,
      0,
      'b',
      'a',
      'r',
      'o',
      (u8)T_PRIM,
      (u8)U16,
      0,
      0,
      'b',
      'a',
      'z',
      'b',
      'i',
      (u8)T_PRIM,
      (u8)CF128
  };
  u16 len = 4;
  u16 l0  = 3;
  u16 l2  = 3;
  u16 l3  = 4;
  u16 l4  = 5;
  memcpy (&data[0], &len, sizeof (u16));
  memcpy (&data[2], &l0, sizeof (u16));
  memcpy (&data[9], &l2, sizeof (u16));
  memcpy (&data[16], &l3, sizeof (u16));
  memcpy (&data[24], &l4, sizeof (u16));

  struct struct_t    sret;
  struct chunk_alloc alloc;
  chunk_alloc_create_default (&alloc);
  struct deserializer d = dsrlizr_create (data, sizeof (data));

  error e   = error_create ();
  err_t ret = struct_t_deserialize (&sret, &d, &alloc, &e);

  test_assert_int_equal (ret, ERR_INTERP); // Duplicate
  chunk_alloc_free_all (&alloc);
}
#endif

err_t
struct_t_random (
    struct struct_t    *st,
    struct chunk_alloc *alloc,
    u32                 depth,
    error              *e
)
{
  ASSERT (st);

  st->len = (u16)randu32r (1, 5);

  st->keys =
      (struct string *)chunk_malloc (alloc, st->len, sizeof (struct string), e);
  if (!st->keys)
  {
    return error_trace (e);
  }

  st->types =
      (struct type **)chunk_malloc (alloc, st->len, sizeof (struct type *), e);
  if (!st->types)
  {
    return error_trace (e);
  }

  for (u16 i = 0; i < st->len; ++i)
  {
    WRAP (rand_varname (&st->keys[i], alloc, 5, 11, e));
    st->types[i] = type_random (alloc, depth - 1, e);
    if (st->types[i] == NULL)
    {
      return error_trace (e);
    }
  }

  return SUCCESS;
}

bool
struct_t_equal (const struct struct_t *left, const struct struct_t *right)
{
  if (left->len != right->len)
  {
    return false;
  }

  for (u32 i = 0; i < left->len; ++i)
  {
    if (!string_equal (left->keys[i], right->keys[i]))
    {
      return false;
    }
    if (!type_equal (left->types[i], right->types[i]))
    {
      return false;
    }
  }

  return true;
}

struct type *
struct_t_resolve_key (
    t_size          *offset,
    struct struct_t *t,
    struct string    key,
    error           *e
)
{
  t_size roffset = 0;
  for (u32 i = 0; i < t->len; ++i)
  {
    if (string_equal (t->keys[i], key))
    {
      if (offset)
      {
        *offset = roffset;
      }
      return t->types[i];
    }
    roffset += type_byte_size (t->types[i]);
  }

  return NULL;
}

/******************************************************************************
 * SECTION: Union
 * ----------------------------------------------------------------------------
 * @brief A Union type
 ******************************************************************************/

DEFINE_DBG_ASSERT (struct union_t, unchecked_union_t, s, {
  ASSERT (s);
  ASSERT (s->keys);
  ASSERT (s->types);
})

static err_t
union_t_type_err (const char *msg, error *e)
{
  return error_causef (e, ERR_INTERP, "Union: %s", msg);
}

static err_t
union_t_type_deser (const char *msg, error *e)
{
  return error_causef (e, ERR_CORRUPT, "Union: %s", msg);
}

static err_t
union_t_validate_shallow (const struct union_t *s, error *e)
{
  DBG_ASSERT (unchecked_union_t, s);

  if (s->len == 0)
  {
    return union_t_type_err ("Keys length must be > 0", e);
  }

  for (u32 i = 0; i < s->len; ++i)
  {
    if (s->keys[i].len == 0)
    {
      return union_t_type_err ("Key length must be > 0", e);
    }
    ASSERT (s->keys[i].data);
  }

  if (!strings_all_unique (s->keys, s->len))
  {
    return union_t_type_err ("Duplicate keys", e);
  }

  return SUCCESS;
}

DEFINE_DBG_ASSERT (struct union_t, valid_union_t, s, {
  error e = error_create ();
  ASSERT (union_t_validate_shallow (s, &e) == SUCCESS);
})

err_t
union_t_create (
    struct union_t     *dest,
    struct kvt_list     list,
    struct chunk_alloc *dalloc,
    error              *e
)
{
  if (list.len == 0)
  {
    return union_t_type_err ("union must have greater than 0 keys", e);
  }

  // Copy stuff over
  if (dalloc)
  {
    dest->len  = list.len;
    dest->keys = chunk_alloc_move_mem (
        dalloc,
        list.keys,
        list.len * sizeof *dest->keys,
        e
    );
    if (dest->keys == NULL)
    {
      return error_trace (e);
    }

    dest->types = chunk_alloc_move_mem (
        dalloc,
        list.types,
        list.len * sizeof (struct type *),
        e
    );
    if (dest->keys == NULL)
    {
      return error_trace (e);
    }
  }

  // Don't copy
  else
  {
    dest->len   = list.len;
    dest->keys  = list.keys;
    dest->types = list.types;
  }

  return SUCCESS;
}

err_t
union_t_validate (const struct union_t *s, error *e)
{
  WRAP (union_t_validate_shallow (s, e));
  {
    return false;
  }
  for (u32 i = 0; i < s->len; ++i)
  {
    WRAP (type_validate (s->types[i], e));
    {
      return false;
    }
  }
  return true;
}

i32
union_t_snprintf (char *str, u32 size, const struct union_t *st)
{
  DBG_ASSERT (valid_union_t, st);

  char *out   = str;
  u32   avail = size;
  int   len   = 0;
  int   n;

  n = snprintf (out, avail, "union { ");
  if (n < 0)
  {
    return n;
  }
  len += n;
  if (out)
  {
    out += n;
    if ((u32)n < avail)
    {
      avail -= n;
    }
    else
    {
      avail = 0;
    }
  }

  for (u32 i = 0; i < st->len; ++i)
  {
    struct string key = st->keys[i];
    n                 = snprintf (out, avail, "%.*s ", key.len, key.data);
    if (n < 0)
    {
      return n;
    }
    len += n;
    if (out)
    {
      out += n;
      if ((u32)n < avail)
      {
        avail -= n;
      }
      else
      {
        avail = 0;
      }
    }

    n = type_snprintf (out, avail, st->types[i]);
    if (n < 0)
    {
      return n;
    }
    len += n;
    if (out)
    {
      out += n;
      if ((u32)n < avail)
      {
        avail -= n;
      }
      else
      {
        avail = 0;
      }
    }

    if (i + 1 < st->len)
    {
      n = snprintf (out, avail, ", ");
      if (n < 0)
      {
        return n;
      }
      len += n;
      if (out)
      {
        out += n;
        if ((u32)n < avail)
        {
          avail -= n;
        }
        else
        {
          avail = 0;
        }
      }
    }
  }

  n = snprintf (out, avail, " }");
  if (n < 0)
  {
    return n;
  }
  len += n;

  return len;
}

#ifndef NTEST
TEST (union_t_snprintf)
{
  struct union_t st;
  st.len  = 4;
  st.keys = (struct string[]){
      {
          .len  = 3,
          .data = "foo",
      },
      {
          .len  = 2,
          .data = "fo",
      },
      {
          .len  = 4,
          .data = "baro",
      },
      {
          .len  = 5,
          .data = "bazbi",
      },
  };
  st.types = (struct type *[]){
      &(struct type){
          .type = T_PRIM,
          .p    = U32,
      },
      &(struct type){
          .type = T_PRIM,
          .p    = U8,
      },
      &(struct type){
          .type = T_PRIM,
          .p    = U16,
      },
      &(struct type){
          .type = T_PRIM,
          .p    = CF128,
      },
  };

  struct type t = {
      .type = T_UNION,
      .un   = st,
  };

  const char *expected = "union { foo u32, fo u8, baro u16, bazbi cf128 }";
  char       *ret      = type_tostr (&t);
  error       e        = error_create ();
  i_log_type (&t, &e);
  test_assert_int_equal (strncmp (expected, ret, strlen (expected)), 0);
  i_free (ret);
}
#endif

u32
union_t_byte_size (const struct union_t *t)
{
  DBG_ASSERT (valid_union_t, t);
  u32 ret = 0;

  for (u32 i = 0; i < t->len; ++i)
  {
    u32 next = type_byte_size (t->types[i]);
    if (next > ret)
    {
      ret = next;
    }
  }

  return ret;
}

#ifndef NTEST
TEST (union_t_byte_size)
{
  struct union_t st;
  st.len  = 4;
  st.keys = (struct string[]){
      {
          .len  = 3,
          .data = "foo",
      },
      {
          .len  = 2,
          .data = "fo",
      },
      {
          .len  = 4,
          .data = "baro",
      },
      {
          .len  = 5,
          .data = "bazbi",
      },
  };
  st.types = (struct type *[]){
      &(struct type){
          .type = T_PRIM,
          .p    = U32,
      },
      &(struct type){
          .type = T_PRIM,
          .p    = U8,
      },
      &(struct type){
          .type = T_PRIM,
          .p    = U16,
      },
      &(struct type){
          .type = T_PRIM,
          .p    = CF128,
      },
  };

  u64 act = union_t_byte_size (&st);
  u64 exp = sizeof (cf128);

  test_assert_int_equal (exp, act);
}
#endif

u32
union_t_get_serial_size (const struct union_t *t)
{
  DBG_ASSERT (valid_union_t, t);
  u32 ret = 0;

  // LEN (KLEN KEY) (TYPE) (KLEN KEY) (TYPE) ....
  ret += sizeof (u16);
  ret += t->len * sizeof (u16);

  for (u32 i = 0; i < t->len; ++i)
  {
    ret += t->keys[i].len;
    ret += type_get_serial_size (t->types[i]);
  }

  return ret;
}

#ifndef NTEST
TEST (union_t_get_serial_size)
{
  struct union_t st;
  st.len  = 4;
  st.keys = (struct string[]){
      {
          .len  = 3,
          .data = "foo",
      },
      {
          .len  = 2,
          .data = "fo",
      },
      {
          .len  = 4,
          .data = "baro",
      },
      {
          .len  = 5,
          .data = "bazbi",
      },
  };
  st.types = (struct type *[]){
      &(struct type){
          .type = T_PRIM,
          .p    = U32,
      },
      &(struct type){
          .type = T_PRIM,
          .p    = U8,
      },
      &(struct type){
          .type = T_PRIM,
          .p    = U16,
      },
      &(struct type){
          .type = T_PRIM,
          .p    = CF128,
      },
  };

  u64 act = union_t_get_serial_size (&st);
  u64 exp = 2 + 4 * 2 + 3 + 2 + 4 + 5 + 4 * 2;

  test_assert_int_equal (exp, act);
}
#endif

void
union_t_serialize (struct serializer *dest, const struct union_t *src)
{
  DBG_ASSERT (valid_union_t, src);
  bool ret;

  // LEN (KLEN KEY) (TYPE) (KLEN KEY) (TYPE) ....
  ret = srlizr_write (dest, (const u8 *)&src->len, sizeof (u16));
  ASSERT (ret);

  for (u32 i = 0; i < src->len; ++i)
  {
    // (KLEN
    struct string next = src->keys[i];
    ret = srlizr_write (dest, (const u8 *)&next.len, sizeof (u16));
    ASSERT (ret);

    // KEY)
    ret = srlizr_write (dest, (u8 *)next.data, next.len);
    ASSERT (ret);

    // (TYPE)
    type_serialize (dest, src->types[i]);
  }
}

#ifndef NTEST
TEST (union_t_serialize)
{
  struct union_t st;
  st.len  = 4;
  st.keys = (struct string[]){
      {
          .len  = 3,
          .data = "foo",
      },
      {
          .len  = 2,
          .data = "fo",
      },
      {
          .len  = 4,
          .data = "baro",
      },
      {
          .len  = 5,
          .data = "bazbi",
      },
  };
  st.types = (struct type *[]){
      &(struct type){
          .type = T_PRIM,
          .p    = U32,
      },
      &(struct type){
          .type = T_PRIM,
          .p    = U8,
      },
      &(struct type){
          .type = T_PRIM,
          .p    = U16,
      },
      &(struct type){
          .type = T_PRIM,
          .p    = CF128,
      },
  };

  u8  act[200]; // Sloppy sizing
  u8  exp[] = {0,       0,   0,   0,   'f', 'o',        'o',        (u8)T_PRIM,
               (u8)U32, 0,   0,   'f', 'o', (u8)T_PRIM, (u8)U8,     0,
               0,       'b', 'a', 'r', 'o', (u8)T_PRIM, (u8)U16,    0,
               0,       'b', 'a', 'z', 'b', 'i',        (u8)T_PRIM, (u8)CF128};
  u16 len   = 4;
  u16 l0    = 3;
  u16 l2    = 2;
  u16 l3    = 4;
  u16 l4    = 5;
  memcpy (&exp[0], &len, sizeof (u16));
  memcpy (&exp[2], &l0, sizeof (u16));
  memcpy (&exp[9], &l2, sizeof (u16));
  memcpy (&exp[15], &l3, sizeof (u16));
  memcpy (&exp[23], &l4, sizeof (u16));

  // Expected
  struct serializer s = srlizr_create (act, 200);
  union_t_serialize (&s, &st);

  test_assert_int_equal (s.dlen, sizeof (exp));
  test_assert_int_equal (memcmp (act, exp, sizeof (exp)), 0);
}
#endif

err_t
union_t_deserialize (
    struct union_t      *dest,
    struct deserializer *src,
    struct chunk_alloc  *a,
    error               *e
)
{
  ASSERT (dest);

  struct chunk_alloc temp;
  chunk_alloc_create_default (&temp);
  struct kvt_list_builder unb;
  kvlb_create (&unb, &temp, a);

  // LEN
  u16 len;
  if (!dsrlizr_read ((u8 *)&len, sizeof (u16), src))
  {
    goto early_termination;
  }

  for (u32 i = 0; i < len; ++i)
  {
    u16 klen;
    if (!dsrlizr_read ((u8 *)&klen, sizeof (u16), src))
    {
      goto early_termination;
    }

    struct string key = {
        .len  = klen,
        .data = chunk_malloc (a, klen, 1, e),
    };
    if (key.data == NULL)
    {
      goto theend;
    }

    if (!dsrlizr_read ((u8 *)key.data, key.len, src))
    {
      goto early_termination;
    }

    struct type *t = type_deserialize (src, a, e);
    if (t == NULL)
    {
      goto theend;
    }

    if (unlikely ((kvlb_accept_key (&unb, key, e)) < SUCCESS))
    {
      goto theend;
    }
    if (unlikely ((kvlb_accept_type (&unb, t, e)) < SUCCESS))
    {
      goto theend;
    }
  }

  struct kvt_list list;
  if (unlikely ((kvlb_build (&list, &unb, e)) < SUCCESS))
  {
    goto theend;
  }
  if (unlikely ((union_t_create (dest, list, NULL, e)) < SUCCESS))
  {
    goto theend;
  }

theend:
  chunk_alloc_free_all (&temp);
  return error_trace (e);

early_termination:
  chunk_alloc_free_all (&temp);
  return union_t_type_deser ("Early end of serialized string", e);
}

#ifndef NTEST
TEST (union_t_deserialize_green_path)
{
  u8  data[] = {0,       0,   0,   0,   'f', 'o',        'o',        (u8)T_PRIM,
                (u8)U32, 0,   0,   'f', 'o', (u8)T_PRIM, (u8)U8,     0,
                0,       'b', 'a', 'r', 'o', (u8)T_PRIM, (u8)U16,    0,
                0,       'b', 'a', 'z', 'b', 'i',        (u8)T_PRIM, (u8)CF128};
  u16 len    = 4;
  u16 l0     = 3;
  u16 l2     = 2;
  u16 l3     = 4;
  u16 l4     = 5;
  memcpy (&data[0], &len, sizeof (u16));
  memcpy (&data[2], &l0, sizeof (u16));
  memcpy (&data[9], &l2, sizeof (u16));
  memcpy (&data[15], &l3, sizeof (u16));
  memcpy (&data[23], &l4, sizeof (u16));

  struct chunk_alloc st_alloc;
  chunk_alloc_create_default (&st_alloc);

  struct deserializer d = dsrlizr_create (data, sizeof (data));

  error e = error_create ();

  struct union_t eret;
  err_t          ret = union_t_deserialize (&eret, &d, &st_alloc, &e);

  test_assert_int_equal (ret, SUCCESS);

  test_assert_int_equal (eret.len, 4);

  test_assert_int_equal (eret.keys[0].len, 3);
  test_assert_int_equal (memcmp (eret.keys[0].data, "foo", 3), 0);
  test_assert_int_equal (eret.types[0]->type, T_PRIM);
  test_assert_int_equal (eret.types[0]->p, U32);

  test_assert_int_equal (eret.keys[1].len, 2);
  test_assert_int_equal (memcmp (eret.keys[1].data, "fo", 2), 0);
  test_assert_int_equal (eret.types[1]->type, T_PRIM);
  test_assert_int_equal (eret.types[1]->p, U8);

  test_assert_int_equal (eret.keys[2].len, 4);
  test_assert_int_equal (memcmp (eret.keys[2].data, "baro", 4), 0);
  test_assert_int_equal (eret.types[2]->type, T_PRIM);
  test_assert_int_equal (eret.types[2]->p, U16);

  test_assert_int_equal (eret.keys[3].len, 5);
  test_assert_int_equal (memcmp (eret.keys[3].data, "bazbi", 5), 0);
  test_assert_int_equal (eret.types[3]->type, T_PRIM);
  test_assert_int_equal (eret.types[3]->p, CF128);

  chunk_alloc_free_all (&st_alloc);
}
#endif

#ifndef NTEST
TEST (union_t_deserialize_red_path)
{
  u8  data[] = {0,          0,          0,       0,          'f',      'o', 'o',
                (u8)T_PRIM, (u8)U32,    0,       0,          'f',      'o', 'o',
                (u8)T_PRIM, (u8)U8,     0,       0,          'b',      'a', 'r',
                'o',        (u8)T_PRIM, (u8)U16, 0,          0,        'b', 'a',
                'z',        'b',        'i',     (u8)T_PRIM, (u8)CF128};
  u16 len    = 4;
  u16 l0     = 3;
  u16 l2     = 3;
  u16 l3     = 4;
  u16 l4     = 5;
  memcpy (&data[0], &len, sizeof (u16));
  memcpy (&data[2], &l0, sizeof (u16));
  memcpy (&data[9], &l2, sizeof (u16));
  memcpy (&data[16], &l3, sizeof (u16));
  memcpy (&data[24], &l4, sizeof (u16));

  struct union_t     sret;
  struct chunk_alloc alloc;
  chunk_alloc_create_default (&alloc);
  struct deserializer d = dsrlizr_create (data, sizeof (data));

  error e   = error_create ();
  err_t ret = union_t_deserialize (&sret, &d, &alloc, &e);

  test_assert_int_equal (ret, ERR_INTERP); // Duplicate
  chunk_alloc_free_all (&alloc);
}
#endif

err_t
union_t_random (
    struct union_t     *un,
    struct chunk_alloc *alloc,
    u32                 depth,
    error              *e
)
{
  ASSERT (un);

  un->len = (u16)randu32r (1, 5);

  un->keys =
      (struct string *)chunk_malloc (alloc, un->len, sizeof (struct string), e);
  if (!un->keys)
  {
    return error_trace (e);
  }

  un->types =
      (struct type **)chunk_malloc (alloc, un->len, sizeof (struct type *), e);
  if (!un->types)
  {
    return error_trace (e);
  }

  for (u16 i = 0; i < un->len; ++i)
  {
    WRAP (rand_varname (&un->keys[i], alloc, 5, 11, e));
    un->types[i] = type_random (alloc, depth - 1, e);
    if (un->types[i] == NULL)
    {
      return error_trace (e);
    }
  }

  return SUCCESS;
}

bool
union_t_equal (const struct union_t *left, const struct union_t *right)
{
  if (left->len != right->len)
  {
    return false;
  }

  for (u32 i = 0; i < left->len; ++i)
  {
    if (!string_equal (left->keys[i], right->keys[i]))
    {
      return false;
    }
    if (!type_equal (left->types[i], right->types[i]))
    {
      return false;
    }
  }

  return true;
}

struct type *
union_t_resolve_key (struct union_t *t, struct string key, error *e)
{
  for (u32 i = 0; i < t->len; ++i)
  {
    if (string_equal (t->keys[i], key))
    {
      return t->types[i];
    }
  }

  return NULL;
}

/******************************************************************************
 * SECTION: Strict Array
 * ----------------------------------------------------------------------------
 * @brief A strict array type
 ******************************************************************************/

DEFINE_DBG_ASSERT (struct sarray_t, unchecked_sarray_t, s, {
  ASSERT (s);
  ASSERT (s->dims);
  ASSERT (s->t);
})

DEFINE_DBG_ASSERT (struct sarray_t, valid_sarray_t, s, {
  error e = error_create ();
  ASSERT (sarray_t_validate (s, &e) == SUCCESS);
})

static err_t
sarray_t_type_err (const char *msg, error *e)
{
  return error_causef (e, ERR_INTERP, "Strict Array: %s", msg);
}

static err_t
sarray_t_type_deser (const char *msg, error *e)
{
  return error_causef (e, ERR_CORRUPT, "Strict Array: %s", msg);
}

static err_t
sarray_t_validate_shallow (const struct sarray_t *t, error *e)
{
  DBG_ASSERT (unchecked_sarray_t, t);
  if (t->rank == 0)
  {
    return sarray_t_type_err ("Rank must be > 0", e);
  }
  for (u32 i = 0; i < t->rank; ++i)
  {
    if (t->dims[i] == 0)
    {
      return sarray_t_type_err ("dimensions cannot be 0", e);
    }
  }
  return SUCCESS;
}

err_t
sarray_t_validate (const struct sarray_t *t, error *e)
{
  DBG_ASSERT (unchecked_sarray_t, t);

  WRAP (sarray_t_validate_shallow (t, e));
  WRAP (type_validate (t->t, e));

  return SUCCESS;
}

i32
sarray_t_snprintf (char *str, u32 size, const struct sarray_t *p)
{
  DBG_ASSERT (valid_sarray_t, p);

  char *out   = str;
  u32   avail = size;
  int   len   = 0;
  int   n;

  for (u16 i = 0; i < p->rank; ++i)
  {
    n = snprintf (out, avail, "[%u]", p->dims[i]);
    if (n < 0)
    {
      return n;
    }
    len += n;
    if (out)
    {
      out += n;
      if ((u32)n < avail)
      {
        avail -= n;
      }
      else
      {
        avail = 0;
      }
    }
  }

  n = type_snprintf (out, avail, p->t);
  if (n < 0)
  {
    return n;
  }
  len += n;

  return len;
}

#ifndef NTEST
TEST (sarray_t_snprintf)
{
  struct type s = (struct type){
      .type = T_SARRAY,
      .sa   = {
          .dims = (u32[]){10, 11, 12},
          .rank = 3,
          .t    = &(struct type){
              .type = T_PRIM,
              .p    = U32,
          },
      },
  };

  const char *expected = "[10][11][12]u32";

  char *ret = type_tostr (&s);
  error e   = error_create ();
  i_log_type (&s, &e);
  test_assert_int_equal (strncmp (expected, ret, strlen (expected)), 0);
  i_free (ret);
}
#endif

u32
sarray_t_byte_size (const struct sarray_t *t)
{
  DBG_ASSERT (valid_sarray_t, t);
  u32 ret = 1;

  // multiply up all ranks and multiply by size of type
  for (u32 i = 0; i < t->rank; ++i)
  {
    ret *= t->dims[i];
  }

  return ret * type_byte_size (t->t);
}

#ifndef NTEST
TEST (sarray_t_byte_size)
{
  struct sarray_t s = {
      .dims = (u32[]){10, 11, 12},
      .rank = 3,
      .t    = &(struct type){
          .type = T_PRIM,
          .p    = U32,
      },
  };
  test_assert_int_equal (sarray_t_byte_size (&s), 10 * 11 * 12 * 4);
}
#endif

u32
sarray_t_get_serial_size (const struct sarray_t *t)
{
  DBG_ASSERT (valid_sarray_t, t);
  u32 ret = 0;

  // RANK DIM0 DIM1 DIM2 ... TYPE
  ret += sizeof (u16);
  ret += sizeof (u32) * t->rank;
  ret += type_get_serial_size (t->t);

  return ret;
}

#ifndef NTEST
TEST (sarray_t_get_serial_size)
{
  struct sarray_t s = {
      .dims = (u32[]){10, 11, 12},
      .rank = 3,
      .t    = &(struct type){
          .type = T_PRIM,
          .p    = U32,
      },
  };
  test_assert_int_equal (sarray_t_get_serial_size (&s), 3 * 4 + 2 + 2);
}
#endif

void
sarray_t_serialize (struct serializer *persistent, const struct sarray_t *src)
{
  DBG_ASSERT (valid_sarray_t, src);
  bool ret;

  // RANK DIM0 DIM1 DIM2 ... TYPE
  ret = srlizr_write (persistent, (const u8 *)&src->rank, sizeof (u16));
  ASSERT (ret);

  for (u32 i = 0; i < src->rank; ++i)
  {
    // DIMi
    ret = srlizr_write (persistent, (const u8 *)&src->dims[i], sizeof (u32));
    ASSERT (ret);
  }

  // (TYPE)
  type_serialize (persistent, src->t);
}

#ifndef NTEST
TEST (sarray_t_serialize)
{
  struct sarray_t s = {
      .dims = (u32[]){10, 11, 12},
      .rank = 3,
      .t    = &(struct type){
          .type = T_PRIM,
          .p    = U32,
      },
  };

  u8  act[200];
  u8  exp[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, (u8)T_PRIM, (u8)U32};
  u16 len   = 3;
  u32 d0    = 10;
  u32 d1    = 11;
  u32 d2    = 12;
  memcpy (exp, &len, 2);
  memcpy (exp + 2, &d0, 4);
  memcpy (exp + 6, &d1, 4);
  memcpy (exp + 10, &d2, 4);

  struct serializer sr = srlizr_create (act, 200);
  sarray_t_serialize (&sr, &s);

  test_assert_int_equal (sr.dlen, sizeof (exp));
  test_assert_int_equal (memcmp (act, exp, sizeof (exp)), 0);
}
#endif

err_t
sarray_t_deserialize (
    struct sarray_t     *persistent,
    struct deserializer *src,
    struct chunk_alloc  *a,
    error               *e
)
{
  ASSERT (persistent);

  struct sarray_t sa = {0};

  // RANK
  if (!dsrlizr_read ((u8 *)&sa.rank, sizeof (u16), src))
  {
    goto early_terimination;
  }

  // Allocate dimensions buffer
  u32 *dims = chunk_malloc (a, sa.rank, sizeof *dims, e);
  if (dims == NULL)
  {
    return error_trace (e);
  }
  sa.dims = dims;

  // Allocate type
  struct type *t = chunk_malloc (a, 1, sizeof *t, e);
  if (t == NULL)
  {
    return error_trace (e);
  }
  sa.t = t;

  for (u32 i = 0; i < sa.rank; ++i)
  {
    u32 dim;

    // DIMi
    if (!dsrlizr_read ((u8 *)&dim, sizeof (u32), src))
    {
      goto early_terimination;
    }

    sa.dims[i] = dim;
  }

  // (TYPE)
  sa.t = type_deserialize (src, a, e);
  if (sa.t == NULL)
  {
    return error_trace (e);
  }
  WRAP (sarray_t_validate_shallow (&sa, e));

  *persistent = sa;
  return SUCCESS;

early_terimination:
  return sarray_t_type_deser ("Early end of serialized string", e);
}

#ifndef NTEST
TEST (sarray_t_deserialize_green_path)
{
  u8  data[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, (u8)T_PRIM, (u8)U32};
  u16 len    = 3;
  u32 d0     = 10;
  u32 d1     = 11;
  u32 d2     = 12;
  memcpy (data, &len, 2);
  memcpy (data + 2, &d0, 4);
  memcpy (data + 6, &d1, 4);
  memcpy (data + 10, &d2, 4);

  struct chunk_alloc sab_temp;
  chunk_alloc_create_default (&sab_temp);

  struct deserializer d = dsrlizr_create (data, sizeof (data));

  error e = error_create ();

  struct sarray_t sret;
  err_t           ret = sarray_t_deserialize (&sret, &d, &sab_temp, &e);

  test_assert_int_equal (ret, SUCCESS);

  test_assert_int_equal (sret.rank, 3);

  test_assert_int_equal (sret.dims[0], 10);
  test_assert_int_equal (sret.dims[1], 11);
  test_assert_int_equal (sret.dims[2], 12);

  chunk_alloc_free_all (&sab_temp);
}
#endif

#ifndef NTEST
TEST (sarray_t_deserialize_red_path)
{
  u8  data[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, (u8)T_PRIM, (u8)U32};
  u16 len    = 3;
  u32 d0     = 10;
  u32 d1     = 0;
  u32 d2     = 12;
  memcpy (data, &len, 2);
  memcpy (data + 2, &d0, 4);
  memcpy (data + 6, &d1, 4);
  memcpy (data + 10, &d2, 4);

  struct sarray_t    eret;
  struct chunk_alloc temp;
  chunk_alloc_create_default (&temp);
  struct deserializer d = dsrlizr_create (data, sizeof (data));

  error e   = error_create ();
  err_t ret = sarray_t_deserialize (&eret, &d, &temp, &e);

  test_assert_int_equal (ret, ERR_INTERP); // 0 value

  chunk_alloc_free_all (&temp);
}
#endif

err_t
sarray_t_random (
    struct sarray_t    *sa,
    struct chunk_alloc *temp,
    u32                 depth,
    error              *e
)
{
  ASSERT (sa);

  sa->rank = (u16)randu32r (1, 4);

  sa->dims = (u32 *)chunk_malloc (temp, sa->rank, sizeof (u32), e);
  if (!sa->dims)
  {
    return error_trace (e);
  }

  for (u16 i = 0; i < sa->rank; ++i)
  {
    sa->dims[i] = randu32r (1, 11);
  }

  sa->t = (struct type *)chunk_malloc (temp, 1, sizeof (struct type), e);
  if (!sa->t)
  {
    return error_trace (e);
  }

  sa->t = type_random (temp, depth - 1, e);
  if (sa->t == NULL)
  {
    return error_trace (e);
  }

  return SUCCESS;
}

bool
sarray_t_equal (const struct sarray_t *left, const struct sarray_t *right)
{
  if (left->rank != right->rank)
  {
    return false;
  }

  for (u32 i = 0; i < left->rank; ++i)
  {
    if (left->dims[i] != right->dims[i])
    {
      return false;
    }
  }

  return true;
}

/////////////////////////////////////////////////////////////////////
////// Builder

DEFINE_DBG_ASSERT (struct sarray_builder, sarray_builder, s, { ASSERT (s); })

void
sab_create (
    struct sarray_builder *dest,
    struct chunk_alloc    *temp,
    struct chunk_alloc    *persistent
)
{
  *dest = (struct sarray_builder){
      .head       = NULL,
      .type       = NULL,
      .temp       = temp,
      .persistent = persistent,
  };

  DBG_ASSERT (sarray_builder, dest);
}

err_t
sab_accept_dim (struct sarray_builder *eb, i32 dim, error *e)
{
  DBG_ASSERT (sarray_builder, eb);

  if (dim <= 0)
  {
    return error_causef (e, ERR_SYNTAX, "sarray dimension must be > 0");
  }

  u16                idx  = (u16)list_length (eb->head);
  struct llnode     *slot = llnode_get_n (eb->head, idx);
  struct dim_llnode *node;

  if (slot)
  {
    node = container_of (slot, struct dim_llnode, link);
  }
  else
  {
    node = chunk_malloc (eb->temp, 1, sizeof *node, e);
    if (!node)
    {
      return error_trace (e);
    }
    llnode_init (&node->link);
    if (!eb->head)
    {
      eb->head = &node->link;
    }
    else
    {
      list_append (&eb->head, &node->link);
    }
  }

  node->dim = dim;
  return SUCCESS;
}

err_t
sab_accept_type (struct sarray_builder *eb, struct type *t, error *e)
{
  DBG_ASSERT (sarray_builder, eb);

  if (eb->type)
  {
    return error_causef (e, ERR_INTERP, "type already set");
  }

  eb->type = type_movemem (t, eb->persistent, e);

  return error_trace (e);
}

err_t
sab_build (struct sarray_t *persistent, struct sarray_builder *eb, error *e)
{
  DBG_ASSERT (sarray_builder, eb);
  ASSERT (persistent);

  if (!eb->type)
  {
    return error_causef (e, ERR_INTERP, "type not set");
  }

  u16 rank = (u16)list_length (eb->head);
  if (rank == 0)
  {
    return error_causef (e, ERR_INTERP, "no dims to build");
  }

  u32 *dims = chunk_malloc (eb->persistent, rank, sizeof *dims, e);
  if (!dims)
  {
    return error_trace (e);
  }

  // Copy type to persistent memory (eb->type is on temp)
  struct type *t = chunk_malloc (eb->persistent, 1, sizeof *t, e);
  if (!t)
  {
    return error_trace (e);
  }
  *t = *eb->type;

  u16 i = 0;
  for (struct llnode *it = eb->head; it; it = it->next)
  {
    struct dim_llnode *dn = container_of (it, struct dim_llnode, link);
    dims[i++]             = dn->dim;
  }

  persistent->rank = rank;
  persistent->dims = dims;
  persistent->t    = t;

  return SUCCESS;
}

#ifndef NTEST
TEST (sarray_builder)
{
  error err = error_create ();

  // provide two fixed-size allocators for nodes + dims array
  struct chunk_alloc persistent;
  chunk_alloc_create_default (&persistent);

  // 0. freshly-created builder must be clean
  struct sarray_builder sb;
  sab_create (&sb, &persistent, &persistent);
  test_fail_if (sb.head != NULL);
  test_fail_if (sb.type != NULL);

  // 1. build without type -> ERR_INTERP
  struct sarray_t sar = {0};
  test_assert_int_equal (sab_build (&sar, &sb, &err), ERR_INTERP);
  err.cause_code = SUCCESS;

  // 2. set type but no dims -> still ERR_INTERP
  struct type t_u32 = (struct type){.type = T_PRIM, .p = U32};
  test_assert_int_equal (sab_accept_type (&sb, &t_u32, &err), SUCCESS);
  test_assert_int_equal (sab_build (&sar, &sb, &err), ERR_INTERP);
  err.cause_code = SUCCESS;

  // 3. duplicate type must fail
  test_assert_int_equal (sab_accept_type (&sb, &t_u32, &err), ERR_INTERP);
  err.cause_code = SUCCESS;

  // 4. accept first dim 10
  test_assert_int_equal (sab_accept_dim (&sb, 10, &err), SUCCESS);

  // 5. successful build now that we have type and one dim
  test_assert_int_equal (sab_build (&sar, &sb, &err), SUCCESS);
  test_assert_int_equal (sar.rank, 1);
  test_assert_int_equal (*sar.dims, 10);
  test_assert_int_equal (sar.t->p, t_u32.p);

  // 6. accept additional dims and rebuild (rank 3)
  test_assert_int_equal (sab_accept_dim (&sb, 4, &err), SUCCESS);
  test_assert_int_equal (sab_accept_dim (&sb, 2, &err), SUCCESS);
  test_assert_int_equal (sab_build (&sar, &sb, &err), SUCCESS);
  test_assert_int_equal (sar.rank, 3);
  test_assert_int_equal (sar.dims[0], 10);
  test_assert_int_equal (sar.dims[1], 4);
  test_assert_int_equal (sar.dims[2], 2);

  chunk_alloc_free_all (&persistent);
}
#endif

/******************************************************************************
 * SECTION: Key Value Type Builder
 ******************************************************************************/

DEFINE_DBG_ASSERT (struct kvt_list_builder, kvt_list_builder, s, {
  ASSERT (s);
  ASSERT (s->klen <= 10);
  ASSERT (s->tlen <= 10);
})

void
kvlb_create (
    struct kvt_list_builder *dest,
    struct chunk_alloc      *temp,
    struct chunk_alloc      *persistent
)
{
  *dest = (struct kvt_list_builder){
      .head       = NULL,
      .klen       = 0,
      .tlen       = 0,
      .temp       = temp,
      .persistent = persistent,
  };
  DBG_ASSERT (kvt_list_builder, dest);
}

static bool
kvlb_has_key_been_used (const struct kvt_list_builder *ub, struct string key)
{
  for (struct llnode *it = ub->head; it; it = it->next)
  {
    struct kv_llnode *kn = container_of (it, struct kv_llnode, link);
    if (string_equal (kn->key, key))
    {
      return true;
    }
  }
  return false;
}

err_t
kvlb_accept_key (struct kvt_list_builder *ub, struct string key, error *e)
{
  DBG_ASSERT (kvt_list_builder, ub);

  // Check for duplicate keys
  if (kvlb_has_key_been_used (ub, key))
  {
    return error_causef (
        e,
        ERR_INTERP,
        "duplicate key: %.*s",
        key.len,
        key.data
    );
  }

  // Copy key data to persistent memory
  key.data = chunk_alloc_move_mem (ub->persistent, key.data, key.len, e);
  if (key.data == NULL)
  {
    return error_trace (e);
  }

  // Find where to insert this new key in the linked list
  struct llnode    *slot = llnode_get_n (ub->head, ub->klen);
  struct kv_llnode *node;

  if (slot)
  {
    node = container_of (slot, struct kv_llnode, link);
  }
  else
  {
    // Allocate new node onto temp
    node = chunk_malloc (ub->temp, 1, sizeof *node, e);
    if (!node)
    {
      return error_trace (e);
    }
    llnode_init (&node->link);
    node->value = NULL;

    // Set the head if it doesn't exist
    if (!ub->head)
    {
      ub->head = &node->link;
    }

    // Otherwise, append to the list
    else
    {
      list_append (&ub->head, &node->link);
    }
  }

  // Create the node
  node->key = key;
  ub->klen++;

  return SUCCESS;
}

err_t
kvlb_accept_type (struct kvt_list_builder *ub, struct type *t, error *e)
{
  DBG_ASSERT (kvt_list_builder, ub);

  struct llnode    *slot = llnode_get_n (ub->head, ub->tlen);
  struct kv_llnode *node;
  if (slot)
  {
    node = container_of (slot, struct kv_llnode, link);
  }
  else
  {
    node = chunk_malloc (ub->temp, 1, sizeof *node, e);
    if (!node)
    {
      return error_trace (e);
    }
    llnode_init (&node->link);
    node->key = (struct string){0};
    if (!ub->head)
    {
      ub->head = &node->link;
    }
    else
    {
      list_append (&ub->head, &node->link);
    }
  }

  node->value = t;
  ub->tlen++;
  return SUCCESS;
}

err_t
kvlb_build (struct kvt_list *dest, struct kvt_list_builder *ub, error *e)
{
  ASSERT (dest);

  if (ub->klen == 0)
  {
    return error_causef (e, ERR_INTERP, "no keys");
  }
  if (ub->klen != ub->tlen)
  {
    return error_causef (e, ERR_INTERP, "key/value count mismatch");
  }

  struct string *keys =
      chunk_malloc (ub->persistent, ub->klen, sizeof *keys, e);
  if (!keys)
  {
    return error_trace (e);
  }

  struct type **types =
      chunk_malloc (ub->persistent, ub->tlen, sizeof (struct type *), e);
  if (!types)
  {
    return error_trace (e);
  }

  size_t i = 0;
  for (struct llnode *it = ub->head; it; it = it->next)
  {
    struct kv_llnode *kn = container_of (it, struct kv_llnode, link);
    keys[i]              = kn->key;
    types[i]             = kn->value;
    i++;
  }

  dest->keys  = keys;
  dest->types = types;
  dest->len   = ub->klen;

  return SUCCESS;
}

#ifndef NTEST
TEST (kvt_list_builder)
{
  error err = error_create ();

  struct chunk_alloc persistent;
  chunk_alloc_create_default (&persistent);

  // 0. freshly-created builder must be clean
  struct kvt_list_builder kb;
  kvlb_create (&kb, &persistent, &persistent);
  test_assert_int_equal (kb.klen, 0);
  test_assert_int_equal (kb.tlen, 0);
  test_fail_if (kb.head != NULL);

  // 1. accept first key "id"
  struct string key_id = strfcstr ("id");
  test_assert_int_equal (kvlb_accept_key (&kb, key_id, &err), SUCCESS);

  // 2. duplicate key "id" must fail
  test_assert_int_equal (kvlb_accept_key (&kb, key_id, &err), ERR_INTERP);
  err.cause_code = SUCCESS;

  // 3. accept a type for that key (u32)
  struct type t_u32 = (struct type){.type = T_PRIM, .p = U32};
  test_assert_int_equal (kvlb_accept_type (&kb, &t_u32, &err), SUCCESS);

  // 4. accept second key/value pair ("name", i32)
  struct string key_name = strfcstr ("name");
  test_assert_int_equal (kvlb_accept_key (&kb, key_name, &err), SUCCESS);
  struct type t_i32 = (struct type){.type = T_PRIM, .p = I32};
  test_assert_int_equal (kvlb_accept_type (&kb, &t_i32, &err), SUCCESS);

  // 5. mismatched key/type counts -> build must fail
  struct string key_extra = strfcstr ("extra");
  test_assert_int_equal (
      kvlb_accept_key (&kb, key_extra, &err),
      SUCCESS
  ); // klen=3, tlen=2
  struct kvt_list list_fail = {0};
  test_assert_int_equal (kvlb_build (&list_fail, &kb, &err), ERR_INTERP);
  err.cause_code = SUCCESS;

  // 6. add matching type so counts align
  struct type t_f32 = (struct type){.type = T_PRIM, .p = F32};
  test_assert_int_equal (kvlb_accept_type (&kb, &t_f32, &err), SUCCESS);

  // 7. successful build
  struct kvt_list list = {0};
  test_assert_int_equal (kvlb_build (&list, &kb, &err), SUCCESS);
  test_assert_int_equal (list.len, 3);
  // 8. ensure key order preserved (id, name, extra)
  test_assert_int_equal (string_equal (list.keys[0], key_id), true);
  test_assert_int_equal (string_equal (list.keys[1], key_name), true);
  test_assert_int_equal (string_equal (list.keys[2], key_extra), true);

  // 9. ensure type mapping correct
  test_assert_int_equal (list.types[0]->p, t_u32.p);
  test_assert_int_equal (list.types[1]->p, t_i32.p);
  test_assert_int_equal (list.types[2]->p, t_f32.p);

  chunk_alloc_free_all (&persistent);
}
#endif

/******************************************************************************
 * SECTION: Key Value Type List Builder
 ******************************************************************************/

DEFINE_DBG_ASSERT (struct kvt_ref_list_builder, kvt_ref_list_builder, s, {
  ASSERT (s);
  ASSERT (s->klen <= 10);
  ASSERT (s->tlen <= 10);
})

void
kvrlb_create (
    struct kvt_ref_list_builder *dest,
    struct chunk_alloc          *temp,
    struct chunk_alloc          *persistent
)
{
  *dest = (struct kvt_ref_list_builder){
      .head       = NULL,
      .klen       = 0,
      .tlen       = 0,
      .temp       = temp,
      .persistent = persistent,
  };
  DBG_ASSERT (kvt_ref_list_builder, dest);
}

static bool
kvrlb_has_key_been_used (
    const struct kvt_ref_list_builder *ub,
    struct string                      key
)
{
  for (struct llnode *it = ub->head; it; it = it->next)
  {
    struct kv_ref_llnode *kn = container_of (it, struct kv_ref_llnode, link);
    if (string_equal (kn->key, key))
    {
      return true;
    }
  }
  return false;
}

err_t
kvrlb_accept_key (struct kvt_ref_list_builder *ub, struct string key, error *e)
{
  DBG_ASSERT (kvt_ref_list_builder, ub);

  // Check for duplicate keys
  if (kvrlb_has_key_been_used (ub, key))
  {
    return error_causef (
        e,
        ERR_INTERP,
        "duplicate key: %.*s",
        key.len,
        key.data
    );
  }

  // Copy key data to persistent memory
  key.data = chunk_alloc_move_mem (ub->persistent, key.data, key.len, e);
  if (key.data == NULL)
  {
    return error_trace (e);
  }

  // Find where to insert this new key in the linked list
  struct llnode        *slot = llnode_get_n (ub->head, ub->klen);
  struct kv_ref_llnode *node;
  if (slot)
  {
    node = container_of (slot, struct kv_ref_llnode, link);
  }
  else
  {
    // Allocate new node onto temp
    node = chunk_malloc (ub->temp, 1, sizeof *node, e);
    if (!node)
    {
      return error_trace (e);
    }
    llnode_init (&node->link);
    node->value = (struct type_ref){0};

    // Set the head if it doesn't exist
    if (!ub->head)
    {
      ub->head = &node->link;
    }
    // Otherwise, append to the list
    else
    {
      list_append (&ub->head, &node->link);
    }
  }

  // Set the node key
  node->key = key;
  ub->klen++;

  return SUCCESS;
}

err_t
kvrlb_accept_type (struct kvt_ref_list_builder *ub, struct type_ref t, error *e)
{
  DBG_ASSERT (kvt_ref_list_builder, ub);

  struct llnode        *slot = llnode_get_n (ub->head, ub->tlen);
  struct kv_ref_llnode *node;
  if (slot)
  {
    node = container_of (slot, struct kv_ref_llnode, link);
  }
  else
  {
    node = chunk_malloc (ub->temp, 1, sizeof *node, e);
    if (!node)
    {
      return error_trace (e);
    }
    llnode_init (&node->link);
    node->key = (struct string){0};
    if (!ub->head)
    {
      ub->head = &node->link;
    }
    else
    {
      list_append (&ub->head, &node->link);
    }
  }

  node->value = t;
  ub->tlen++;
  return SUCCESS;
}

err_t
kvrlb_build (
    struct kvt_ref_list         *dest,
    struct kvt_ref_list_builder *ub,
    error                       *e
)
{
  ASSERT (dest);

  if (ub->klen == 0)
  {
    return error_causef (e, ERR_INTERP, "no keys");
  }
  if (ub->klen != ub->tlen)
  {
    return error_causef (e, ERR_INTERP, "key/value count mismatch");
  }

  struct string *keys =
      chunk_malloc (ub->persistent, ub->klen, sizeof *keys, e);

  if (!keys)
  {
    return error_trace (e);
  }

  struct type_ref *types =
      chunk_malloc (ub->persistent, ub->tlen, sizeof *types, e);

  if (!types)
  {
    return error_trace (e);
  }

  size_t i = 0;
  for (struct llnode *it = ub->head; it; it = it->next)
  {
    struct kv_ref_llnode *kn = container_of (it, struct kv_ref_llnode, link);
    keys[i]                  = kn->key;
    types[i]                 = kn->value;
    i++;
  }

  dest->keys  = keys;
  dest->types = types;
  dest->len   = ub->klen;

  return SUCCESS;
}

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
type_accessor_equal (
    const struct type_accessor left,
    const struct type_accessor right
)
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
ta_select_struct (
    struct type          *reftype,
    struct type_accessor *ta,
    struct chunk_alloc   *alloc,
    error                *e
)
{
  struct type *subtype =
      struct_t_resolve_key (NULL, &reftype->st, ta->select.key, e);
  if (subtype == NULL)
  {
    return NULL;
  }
  return ta_subtype (subtype, ta->select.sub_ta, alloc, e);
}

static struct type *
ta_select_union (
    struct type          *reftype,
    struct type_accessor *ta,
    struct chunk_alloc   *alloc,
    error                *e
)
{
  struct type *subtype = union_t_resolve_key (&reftype->un, ta->select.key, e);
  if (subtype == NULL)
  {
    return NULL;
  }
  return ta_subtype (subtype, ta->select.sub_ta, alloc, e);
}

static struct type *
ta_select_sarray (
    struct type          *reftype,
    struct type_accessor *ta,
    struct chunk_alloc   *alloc,
    error                *e
)
{
  struct type *ret = chunk_malloc (alloc, 1, sizeof *ret, e);
  if (ret == NULL)
  {
    goto failed;
  }

  struct sarray_builder builder;
  struct chunk_alloc    temp;
  chunk_alloc_create_default (&temp);
  sab_create (&builder, &temp, alloc);

  for (u32 i = 0; i < reftype->sa.rank; ++i)
  {
    if (sab_accept_dim (&builder, reftype->sa.dims[i], e))
    {
      goto fail_chunk_alloc;
    }
  }

  struct type *t = ta_subtype (reftype->sa.t, ta->select.sub_ta, alloc, e);
  if (t == NULL)
  {
    goto fail_chunk_alloc;
  }

  if (sab_accept_type (&builder, t, e))
  {
    goto fail_chunk_alloc;
  }

  ret->type = T_SARRAY;
  if (sab_build (&ret->sa, &builder, e))
  {
    goto fail_chunk_alloc;
  }

  chunk_alloc_free_all (&temp);
  return ret;

fail_chunk_alloc:
  chunk_alloc_free_all (&temp);
failed:
  return NULL;
}

static struct type *
ta_range_sarray (
    struct type          *reftype,
    struct type_accessor *ta,
    struct chunk_alloc   *alloc,
    error                *e
)
{
  struct sarray_builder builder;
  struct chunk_alloc    temp;
  chunk_alloc_create_default (&temp);
  sab_create (&builder, &temp, alloc);

  bool isarray = false;

  for (u32 i = 0; i < reftype->sa.rank; ++i)
  {
    if (i >= ta->range.dlen)
    {
      isarray = true;
      struct stride str;
      if (stride_resolve (&str, USER_STRIDE_ALL, reftype->sa.dims[i], e))
      {
        goto fail_chunk_alloc;
      }
      if (sab_accept_dim (&builder, str.nelems, e))
      {
        goto fail_chunk_alloc;
      }
    }
    else
    {
      isarray = isarray || ta->range.dim_accessors[i].present & COLON_PRESENT;
      struct stride str;
      if (stride_resolve (
              &str,
              ta->range.dim_accessors[i],
              reftype->sa.dims[i],
              e
          ))
      {
        goto fail_chunk_alloc;
      }
      if (ta->range.dim_accessors[i].present & COLON_PRESENT)
      {
        if (sab_accept_dim (&builder, str.nelems, e))
        {
          goto fail_chunk_alloc;
        }
      }
    }
  }

  struct type *ret = NULL;
  struct type *t   = ta_subtype (reftype->sa.t, ta->range.sub_ta, alloc, e);
  if (t == NULL)
  {
    goto fail_chunk_alloc;
  }

  if (isarray)
  {
    if (sab_accept_type (&builder, t, e))
    {
      goto fail_chunk_alloc;
    }

    ret = chunk_malloc (alloc, 1, sizeof *ret, e);
    if (ret == NULL)
    {
      return NULL;
    }

    ret->type = T_SARRAY;
    if (sab_build (&ret->sa, &builder, e))
    {
      goto fail_chunk_alloc;
    }
  }
  else
  {
    ret = t;
  }

  chunk_alloc_free_all (&temp);
  return ret;

fail_chunk_alloc:
  chunk_alloc_free_all (&temp);
  return NULL;
}

struct type *
ta_subtype (
    struct type          *reftype,
    struct type_accessor *ta,
    struct chunk_alloc   *alloc,
    error                *e
)
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
      UNREACHABLE ();
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
      UNREACHABLE ();
    }
  }

  UNREACHABLE ();
}

#ifndef NTEST

static void
test_ta_subtype_case (
    const char *typestr,
    const char *accessor,
    const char *expected_type
)
{
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

TEST (ta_subtype)
{
  struct test_entry
  {
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
      {"[10] struct { pos struct { x f64, y f64 } }",
       "a[0:10:5].pos.y",
       "[2] f64"},

      // ── struct → array → struct chain ────────────────────────
      {"struct { points [100] struct { val f32 } }",
       "a.points[0:50:2].val",
       "[25] f32"},
      {"struct { points [100] struct { val f32 } }", "a.points[7].val", "f32"},

      // ── deeply nested struct + array ─────────────────────────
      {"struct { a struct { b [20] struct { c i32 } } }",
       "a.a.b[0:20:4].c",
       "[5] i32"},
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

  for (u32 i = 0; i < arrlen (entries); ++i)
  {
    TEST_CASE (
        "%s %s %s",
        entries[i].typestr,
        entries[i].accessor,
        entries[i].expected_type
    )
    {
      test_ta_subtype_case (
          entries[i].typestr,
          entries[i].accessor,
          entries[i].expected_type
      );
    }
  }
}
#endif

/////////////////////////////////////////////////////////////////////
////// Builder

bool
user_stride_equal (
    const struct user_stride *left,
    const struct user_stride *right
)
{
  return left->start == right->start && left->step == right->step
         && left->stop == right->stop && left->present == right->present;
}

DEFINE_DBG_ASSERT (struct range_builder, range_builder, s, { ASSERT (s); })

void
rb_create (
    struct range_builder *dest,
    struct chunk_alloc   *temp,
    struct chunk_alloc   *persistent
)
{
  *dest = (struct range_builder){
      .head       = NULL,
      .len        = 0,
      .temp       = temp,
      .persistent = persistent,
  };
  DBG_ASSERT (range_builder, dest);
}

err_t
rb_accept_stride (struct range_builder *rb, struct user_stride stride, error *e)
{
  DBG_ASSERT (range_builder, rb);

  struct rb_llnode *node = chunk_malloc (rb->temp, 1, sizeof *node, e);
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
    return error_causef (e, ERR_INTERP, "range: no dimensions");
  }

  struct user_stride *dims =
      chunk_malloc (rb->persistent, rb->len, sizeof *dims, e);
  if (!dims)
  {
    return error_trace (e);
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

  return SUCCESS;
}

DEFINE_DBG_ASSERT (struct type_accessor_builder, type_accessor_builder, s, {
  ASSERT (s);
})

static struct type_accessor *
tab_alloc (struct type_accessor_builder *builder, error *e)
{
  if (builder->head == NULL)
  {
    return &builder->ret;
  }

  struct type_accessor *ta =
      chunk_malloc (builder->persistent, 1, sizeof *ta, e);
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
    rb_create (&builder->rb, builder->temp, builder->persistent);
    builder->in_range = true;
  }
}

void
tab_create (
    struct type_accessor_builder *dest,
    struct chunk_alloc           *temp,
    struct chunk_alloc           *persistent
)
{
  *dest = (struct type_accessor_builder){
      .head       = NULL,
      .tail       = NULL,
      .temp       = temp,
      .persistent = persistent,
      .in_range   = false,
  };

  DBG_ASSERT (type_accessor_builder, dest);
}

err_t
tab_accept_select (
    struct type_accessor_builder *builder,
    struct string                 key,
    error                        *e
)
{
  DBG_ASSERT (type_accessor_builder, builder);

  WRAP (tab_flush_range (builder, e));

  struct type_accessor *ta = tab_alloc (builder, e);
  if (!ta)
  {
    return error_trace (e);
  }

  key.data = chunk_alloc_move_mem (builder->persistent, key.data, key.len, e);
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
tab_accept_stride (
    struct type_accessor_builder *builder,
    struct user_stride            stride,
    error                        *e
)
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
tab_build (
    struct type_accessor         *dest,
    struct type_accessor_builder *builder,
    error                        *e
)
{
  DBG_ASSERT (type_accessor_builder, builder);

  WRAP (tab_accept_take (builder, e));

  *dest = builder->ret;
  return SUCCESS;
}

#ifndef NTEST
TEST (type_accessor_builder)
{
  error e = error_create ();

  struct chunk_alloc arena;
  chunk_alloc_create_default (&arena);

  // 0. freshly-created builder must be clean
  struct type_accessor_builder builder;
  tab_create (&builder, &arena, &arena);
  test_fail_if (builder.head != NULL);
  test_fail_if (builder.tail != NULL);

  struct type_accessor acc;
  tab_build (&acc, &builder, &e);
  test_assert_int_equal (acc.type, TA_TAKE);

  tab_create (&builder, &arena, &arena);

  // 2. accept a select accessor
  struct string key1 = strfcstr ("field1");
  test_assert_int_equal (tab_accept_select (&builder, key1, &e), SUCCESS);
  // 3. accept a stride + single (enters range mode)
  test_assert_int_equal (
      tab_accept_stride (&builder, ustride012 (0, 10, 2), &e),
      SUCCESS
  );
  test_assert (builder.in_range);
  test_assert_int_equal (
      tab_accept_stride (&builder, ustride_single (5), &e),
      SUCCESS
  );
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

  chunk_alloc_free_all (&arena);
}
#endif

/******************************************************************************
 * SECTION: Type Reference
 ******************************************************************************/

bool
type_ref_equal (const struct type_ref left, const struct type_ref right)
{
  if (left.type != right.type)
  {
    return false;
  }

  switch (left.type)
  {
    case TR_TAKE:
    {
      return string_equal (left.tk.vname, right.tk.vname)
             && type_accessor_equal (left.tk.ta, right.tk.ta);
    }

    case TR_STRUCT:
    {
      if (left.st.len != right.st.len)
      {
        return false;
      }

      for (u16 i = 0; i < left.st.len; i++)
      {
        if (!string_equal (left.st.keys[i], right.st.keys[i]))
        {
          return false;
        }

        if (!type_ref_equal (left.st.types[i], right.st.types[i]))
        {
          return false;
        }
      }

      return true;
    }

    default: return false;
  }
}

struct type *
tr_construct (
    struct type        *reftype,
    struct type_ref    *tr,
    struct chunk_alloc *alloc,
    error              *e
)
{
  struct chunk_alloc temp;

  switch (tr->type)
  {
    case TR_TAKE:
    {
      struct type_accessor *ta = &tr->tk.ta;
      return ta_subtype (reftype, ta, alloc, e);
    }

    case TR_STRUCT:
    {
      u16              len   = tr->st.len;
      struct string   *keys  = tr->st.keys;
      struct type_ref *types = tr->st.types;

      struct type *ret = chunk_malloc (alloc, 1, sizeof *ret, e);

      if (ret == NULL)
      {
        goto temp_failed;
      }

      // Struct building logic
      {
        struct kvt_list_builder builder;
        chunk_alloc_create_default (&temp);

        kvlb_create (&builder, &temp, alloc);

        for (u16 i = 0; i < len; ++i)
        {
          // The field name
          if (kvlb_accept_key (&builder, keys[i], e))
          {
            goto temp_failed;
          }

          // Get the sub type
          // (recursively)
          struct type *subtype = tr_construct (reftype, &types[i], alloc, e);
          if (subtype == NULL)
          {
            goto temp_failed;
          }

          if (kvlb_accept_type (&builder, subtype, e))
          {
            goto temp_failed;
          }
        }

        struct kvt_list kvl;
        if (kvlb_build (&kvl, &builder, e))
        {
          goto temp_failed;
        }

        if (struct_t_create (&ret->st, kvl, alloc, e))
        {
          goto temp_failed;
        }
      }

      chunk_alloc_free_all (&temp);
      return ret;
    }
  }

temp_failed:
  chunk_alloc_free_all (&temp);
  return NULL;
}

/******************************************************************************
 * SECTION: Sub Type
 ******************************************************************************/

err_t
subtype_create (
    struct subtype      *dest,
    struct string        vname,
    struct type_accessor ta,
    error               *e
)
{
  *dest = (struct subtype){
      .vname = vname,
      .ta    = ta,
  };
  return SUCCESS;
}

bool
subtype_equal (const struct subtype *left, const struct subtype *right)
{
  return string_equal (left->vname, right->vname)
         && type_accessor_equal (left->ta, right->ta);
}

/******************************************************************************
 * SECTION: Print Type Data
 ******************************************************************************/

static void
print_indent (int level, u32 spaces)
{
  for (u32 i = 0; i < spaces; ++i)
  {
    i_printf (level, " ");
  }
}

static void
print_prim_value (int level, const u8 *buf, enum prim_t p)
{
  switch (p)
  {
    case U8:
    {
      u8 v;
      memcpy (&v, buf, 1);
      i_printf (level, "%u", (unsigned)v);
      return;
    }
    case U16:
    {
      u16 v;
      memcpy (&v, buf, 2);
      i_printf (level, "%u", (unsigned)v);
      return;
    }
    case U32:
    {
      u32 v;
      memcpy (&v, buf, 4);
      i_printf (level, "%u", (unsigned)v);
      return;
    }
    case U64:
    {
      u64 v;
      memcpy (&v, buf, 8);
      i_printf (level, "%lu", (unsigned long)v);
      return;
    }
    case I8:
    {
      i8 v;
      memcpy (&v, buf, 1);
      i_printf (level, "%d", (int)v);
      return;
    }
    case I16:
    {
      i16 v;
      memcpy (&v, buf, 2);
      i_printf (level, "%d", (int)v);
      return;
    }
    case I32:
    {
      i32 v;
      memcpy (&v, buf, 4);
      i_printf (level, "%d", (int)v);
      return;
    }
    case I64:
    {
      i64 v;
      memcpy (&v, buf, 8);
      i_printf (level, "%ld", (long)v);
      return;
    }
    case F16:
    {
      u16 h;
      memcpy (&h, buf, 2);
      i_printf (level, "%g", (double)f16_to_f32 (h));
      return;
    }
    case F32:
    {
      float v;
      memcpy (&v, buf, 4);
      i_printf (level, "%g", (double)v);
      return;
    }
    case F64:
    {
      double v;
      memcpy (&v, buf, 8);
      i_printf (level, "%g", v);
      return;
    }
    case F128:
    {
      u64 lo, hi;
      memcpy (&lo, buf, 8);
      memcpy (&hi, buf + 8, 8);
      i_printf (
          level,
          "<f128:0x%016lx%016lx>",
          (unsigned long)hi,
          (unsigned long)lo
      );
      return;
    }
    case CF32:
    {
      u16 rh, ih;
      memcpy (&rh, buf, 2);
      memcpy (&ih, buf + 2, 2);
      i_printf (
          level,
          "(%g, %g)",
          (double)f16_to_f32 (rh),
          (double)f16_to_f32 (ih)
      );
      return;
    }
    case CF64:
    {
      float r, im;
      memcpy (&r, buf, 4);
      memcpy (&im, buf + 4, 4);
      i_printf (level, "(%g, %g)", (double)r, (double)im);
      return;
    }
    case CF128:
    {
      double r, im;
      memcpy (&r, buf, 8);
      memcpy (&im, buf + 8, 8);
      i_printf (level, "(%g, %g)", r, im);
      return;
    }
    case CF256:
    {
#if SIZEOF_LONG_DOUBLE >= 16
      long double r, im;
      memcpy (&r, buf, 16);
      memcpy (&im, buf + 16, 16);
      i_printf (level, "(%Lg, %Lg)", r, im);
#else
      u64 r_lo, r_hi, im_lo, im_hi;
      memcpy (&r_lo, buf, 8);
      memcpy (&r_hi, buf + 8, 8);
      memcpy (&im_lo, buf + 16, 8);
      memcpy (&im_hi, buf + 24, 8);
      i_printf (
          level,
          "(<f128:0x%016lx%016lx>, "
          "<f128:0x%016lx%016lx>)",
          (unsigned long)r_hi,
          (unsigned long)r_lo,
          (unsigned long)im_hi,
          (unsigned long)im_lo
      );
#endif
      return;
    }
    case CI16:
    {
      i8 r, im;
      memcpy (&r, buf, 1);
      memcpy (&im, buf + 1, 1);
      i_printf (level, "(%d, %d)", (int)r, (int)im);
      return;
    }
    case CI32:
    {
      i16 r, im;
      memcpy (&r, buf, 2);
      memcpy (&im, buf + 2, 2);
      i_printf (level, "(%d, %d)", (int)r, (int)im);
      return;
    }
    case CI64:
    {
      i32 r, im;
      memcpy (&r, buf, 4);
      memcpy (&im, buf + 4, 4);
      i_printf (level, "(%d, %d)", (int)r, (int)im);
      return;
    }
    case CI128:
    {
      i64 r, im;
      memcpy (&r, buf, 8);
      memcpy (&im, buf + 8, 8);
      i_printf (level, "(%ld, %ld)", (long)r, (long)im);
      return;
    }
    case CU16:
    {
      u8 r, im;
      memcpy (&r, buf, 1);
      memcpy (&im, buf + 1, 1);
      i_printf (level, "(%u, %u)", (unsigned)r, (unsigned)im);
      return;
    }
    case CU32:
    {
      u16 r, im;
      memcpy (&r, buf, 2);
      memcpy (&im, buf + 2, 2);
      i_printf (level, "(%u, %u)", (unsigned)r, (unsigned)im);
      return;
    }
    case CU64:
    {
      u32 r, im;
      memcpy (&r, buf, 4);
      memcpy (&im, buf + 4, 4);
      i_printf (level, "(%u, %u)", (unsigned)r, (unsigned)im);
      return;
    }
    case CU128:
    {
      u64 r, im;
      memcpy (&r, buf, 8);
      memcpy (&im, buf + 8, 8);
      i_printf (level, "(%lu, %lu)", (unsigned long)r, (unsigned long)im);
      return;
    }
  }
}

static void print_type_inner (
    int                level,
    const u8          *buf,
    const struct type *t,
    u32                max_elems,
    u32                indent
);

// Product of dims[dim_idx+1 .. rank-1] * element_size
static u32
sarray_sub_size (const struct sarray_t *sa, u16 dim_idx)
{
  u32 sub = type_byte_size (sa->t);
  for (u16 i = dim_idx + 1; i < sa->rank; ++i)
  {
    sub *= sa->dims[i];
  }
  return sub;
}

// col: visual column of the '[' just printed at this dimension,
// used to align continuation rows under it.
static void
print_sarray_dim (
    int                    level,
    const u8              *buf,
    const struct sarray_t *sa,
    u16                    dim_idx,
    u32                    max_elems,
    u32                    indent,
    u32                    col
)
{
  u32 dim_len  = sa->dims[dim_idx];
  u32 show     = dim_len < max_elems ? dim_len : max_elems;
  u32 sub_size = sarray_sub_size (sa, dim_idx);

  i_printf (level, "[");

  if (dim_idx == sa->rank - 1)
  {
    // Innermost dimension: elements inline
    for (u32 i = 0; i < show; ++i)
    {
      if (i > 0)
      {
        i_printf (level, ", ");
      }
      print_type_inner (
          level,
          buf + i * sub_size,
          sa->t,
          max_elems,
          indent + 1
      );
    }
    if (dim_len > max_elems)
    {
      i_printf (level, ", ...");
    }
  }
  else
  {
    // Outer dimension: each sub-array on its own line
    for (u32 i = 0; i < show; ++i)
    {
      if (i > 0)
      {
        i_printf (level, ",\n");
        print_indent (level, col + 1);
      }
      print_sarray_dim (
          level,
          buf + i * sub_size,
          sa,
          dim_idx + 1,
          max_elems,
          indent + 1,
          col + 1
      );
    }
    if (dim_len > max_elems)
    {
      i_printf (level, ",\n");
      print_indent (level, col + 1);
      i_printf (level, "...");
    }
  }

  i_printf (level, "]");
}

static void
print_type_inner (
    int                level,
    const u8          *buf,
    const struct type *t,
    u32                max_elems,
    u32                indent
)
{
  switch (t->type)
  {
    case T_PRIM:
    {
      print_prim_value (level, buf, t->p);
      return;
    }

    case T_STRUCT:
    {
      i_printf (level, "{\n");
      u32 offset = 0;
      for (u16 i = 0; i < t->st.len; ++i)
      {
        u32 field_indent = indent + 4;
        print_indent (level, field_indent);
        i_printf (level, "%.*s = ", (int)t->st.keys[i].len, t->st.keys[i].data);

        const struct type *ft = t->st.types[i];
        if (ft->type == T_SARRAY)
        {
          u32 col = field_indent + t->st.keys[i].len + 3;
          print_sarray_dim (
              level,
              buf + offset,
              &ft->sa,
              0,
              max_elems,
              field_indent,
              col
          );
        }
        else
        {
          print_type_inner (level, buf + offset, ft, max_elems, field_indent);
        }

        offset += type_byte_size (ft);
        if (i + 1 < t->st.len)
        {
          i_printf (level, ",");
        }
        i_printf (level, "\n");
      }
      print_indent (level, indent);
      i_printf (level, "}");
      return;
    }

    case T_UNION:
    {
      i_printf (level, "<union[0]: ");
      if (t->un.len > 0)
      {
        i_printf (level, "%.*s = ", (int)t->un.keys[0].len, t->un.keys[0].data);
        print_type_inner (level, buf, t->un.types[0], max_elems, indent);
      }
      else
      {
        i_printf (level, "empty");
      }
      i_printf (level, ">");
      return;
    }

    case T_SARRAY:
    {
      print_sarray_dim (level, buf, &t->sa, 0, max_elems, indent, indent);
      return;
    }
  }
}

void
type_print_data (
    int                log_level,
    const u8          *buf,
    const struct type *t,
    u32                max_elems
)
{
  print_type_inner (log_level, buf, t, max_elems, 0);
  i_printf (log_level, "\n");
}

struct type_printer_ostream_ctx
{
  struct type *t;
  t_size       pos;
  t_size       size;
  u8           buf[];
};

static i32
type_print_os_sink (
    struct stream *s,
    void          *vctx,
    const void    *src,
    u32            size,
    u32            n,
    error         *e
)
{
  ASSERT (size == 1);
  struct type_printer_ostream_ctx *ctx =
      (struct type_printer_ostream_ctx *)vctx;

  u32 avail = ctx->size - ctx->pos;
  u32 next  = MIN (avail, n);

  if (next == 0)
  {
    return 0;
  }

  memcpy (ctx->buf + ctx->pos, src, next);
  ctx->pos += next;

  if (ctx->pos == ctx->size)
  {
    type_print_data (LOG_INFO, ctx->buf, ctx->t, 3);
    ctx->pos = 0;
  }

  return (i32)next;
}

static void
type_print_os_close (void *ctx)
{
  i_free ((struct type_printer_ostream_ctx *)ctx);
}

static const struct stream_ops type_printer_os_ops = {
    .pull  = NULL,
    .push  = type_print_os_sink,
    .close = type_print_os_close,
};

err_t
type_stream_printer_init (struct stream *s, struct type *t, error *e)
{
  t_size                           size = type_byte_size (t);
  struct type_printer_ostream_ctx *ctx  = i_malloc (1, sizeof *ctx + size, e);
  if (ctx == NULL)
  {
    return error_trace (e);
  }

  ctx->size = size;
  ctx->pos  = 0;
  ctx->t    = t;
  stream_init (s, &type_printer_os_ops, ctx);

  return SUCCESS;
}
