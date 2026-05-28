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

#include "nscore/types.h"

#include "nscore/errors.h"

#include <c_specx.h>

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
