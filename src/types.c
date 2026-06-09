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

void
type_free (struct type *t)
{
  switch (t->type)
  {
    case T_PRIM:
    {
      break;
    }
    case T_STRUCT:
    {
      for (u32 i = 0; i < t->st.len; ++i)
      {
        type_free (t->st.types[i]);
        i_free ((void *)t->st.keys[i].data);
        t->st.types[i]     = NULL;
        t->st.keys[i].data = NULL;
        t->st.keys[i].len  = 0;
      }
      t->st.keys  = NULL;
      t->st.types = NULL;
      t->st.len   = 0;
      break;
    }
    case T_UNION:
    {
      for (u32 i = 0; i < t->un.len; ++i)
      {
        type_free (t->un.types[i]);
        i_free ((void *)t->un.keys[i].data);
        t->un.types[i]     = NULL;
        t->un.keys[i].data = NULL;
        t->un.keys[i].len  = 0;
      }
      t->un.keys  = NULL;
      t->un.types = NULL;
      t->un.len   = 0;
      break;
    }
    case T_SARRAY:
    {
      type_free (t->sa.t);
      t->sa.t = NULL;
      i_free (t->sa.dims);
      t->sa.dims = NULL;
      t->sa.rank = 0;
      break;
    }
    default:
    {
      UNREACHABLE ();
    }
  }

  i_free (t);
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
  test_assert_int_equal (range_acc->range.dim_accessors[0].stop, 2);
  test_assert_int_equal (range_acc->range.dim_accessors[0].step, 10);

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
