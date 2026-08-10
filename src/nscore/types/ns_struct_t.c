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

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "core/ns_alloc.h"
#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_numerics.h"
#include "core/ns_platform.h"
#include "core/ns_serial.h"
#include "core/ns_stdtypes.h"
#include "core/ns_string.h"
#include "core/os/ns_os.h"
#include "core/testing/ns_testing.h"
#include "nscore/compiler/ns_compiler.h"
#include "nscore/ns_variables.h"
#include "nscore/types/ns_kvt.h"
#include "nscore/types/ns_types.h"

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

#ifdef TESTING
TEST (struct_t_validate_shallow)
{
  ALLOC_INIT (alloc);
  BUILDER_INIT (b, &alloc);

  TEST_CASE ("Len == 0")
  {
    error           e = error_create ();
    struct struct_t s = {
        .keys  = (struct string[]){strfcstr ("foo")},
        .types = (struct type *[]){&TU8},
        .len   = 0,
    };
    test_err_t_check (struct_t_validate_shallow (&s, &e), ERR_INTERP, &e);
  }

  TEST_CASE ("One Invalid label")
  {
    error           e = error_create ();
    struct struct_t s = {
        .keys  = (struct string[]){strfcstr ("foo"), strfcstr (""), strfcstr ("biz")},
        .types = (struct type *[]){&TU8, &TU16, &TU32},
        .len   = 3,
    };
    test_err_t_check (struct_t_validate_shallow (&s, &e), ERR_INTERP, &e);
  }

  TEST_CASE ("Non Unique labels")
  {
    error           e = error_create ();
    struct struct_t s = {
        .keys  = (struct string[]){strfcstr ("foo"), strfcstr ("biz"), strfcstr ("biz")},
        .types = (struct type *[]){&TU8, &TU16, &TU32},
        .len   = 3,
    };
    test_err_t_check (struct_t_validate_shallow (&s, &e), ERR_INTERP, &e);
  }

  TEST_CASE ("Valid")
  {
    error           e = error_create ();
    struct struct_t s = {
        .keys  = (struct string[]){strfcstr ("foo"), strfcstr ("bar"), strfcstr ("biz")},
        .types = (struct type *[]){&TU8, &TU16, &TU32},
        .len   = 3,
    };
    test_assert (struct_t_validate_shallow (&s, &e) == SUCCESS);
  }

  BUILDER_CLOSE (b);
  ALLOC_CLOSE (alloc);
}
#endif

DEFINE_DBG_ASSERT (struct struct_t, valid_struct_t, s, {
  error e = error_create ();
  ASSERT (struct_t_validate_shallow (s, &e) == SUCCESS);
})

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
struct type *
struct_t_resolve_key (t_size *offset, struct struct_t *t, struct string key)
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

#ifdef TESTING
TEST (struct_t_resolve_key)
{
  ALLOC_INIT (alloc);
  error  e = error_create ();
  t_size ofst;

  struct type *t = compile_type_alloc (
      "struct { "
      "     a struct { "
      "         a u32, "
      "         b [10]f32 "
      "     }, "
      "     b f32, "
      "     c [10][20]f32, "
      "     d i8, "
      "     e cf256 "
      "}",
      &alloc,
      &e
  );

  test_assert (type_equal (
      compile_type_alloc ("struct { a u32, b [10]f32}", &alloc, &e),
      struct_t_resolve_key (&ofst, &t->st, strfcstr ("a"))
  ));
  test_assert_int_equal (ofst, 0);

  test_assert (type_equal (
      compile_type_alloc ("f32", &alloc, &e),
      struct_t_resolve_key (&ofst, &t->st, strfcstr ("b"))
  ));
  test_assert_int_equal (ofst, 10 * sizeof (f32) + sizeof (u32));

  test_assert (type_equal (
      compile_type_alloc ("[10][20]f32", &alloc, &e),
      struct_t_resolve_key (&ofst, &t->st, strfcstr ("c"))
  ));
  test_assert_int_equal (ofst, 10 * sizeof (f32) + sizeof (u32) + sizeof (f32));

  test_assert (type_equal (
      compile_type_alloc ("i8", &alloc, &e),
      struct_t_resolve_key (&ofst, &t->st, strfcstr ("d"))
  ));
  test_assert_int_equal (
      ofst,
      10 * sizeof (f32) + sizeof (u32) + sizeof (f32) + 10 * 20 * sizeof (f32)
  );

  test_assert (type_equal (
      compile_type_alloc ("cf256", &alloc, &e),
      struct_t_resolve_key (&ofst, &t->st, strfcstr ("e"))
  ));
  test_assert_int_equal (
      ofst,
      10 * sizeof (f32) + sizeof (u32) + sizeof (f32) + 10 * 20 * sizeof (f32) + sizeof (i8)
  );

  ALLOC_CLOSE (alloc);
}

#endif

err_t
struct_t_create (struct struct_t *dest, struct kvt_list list, struct allocator *dalloc, error *e)
{
  if (list.len == 0)
  {
    return struct_t_type_err ("struct must have greater than 0 keys", e);
  }

  // Copy stuff over
  if (dalloc)
  {
    dest->len  = list.len;
    dest->keys = allocator_copy (dalloc, list.keys, list.len * sizeof *dest->keys, e);
    if (dest->keys == NULL)
    {
      return error_trace (e);
    }

    dest->types = allocator_copy (dalloc, list.types, list.len * sizeof (struct type *), e);
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

#ifdef TESTING
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

#ifdef TESTING
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

  struct type t = {
      .type = T_STRUCT,
      .st   = st,
  };
  u64 act = type_byte_size (&t);
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

  for (u32 i = 0; i < t->len; ++i)
  {
    ret += sizeof (u16);
    ret += t->keys[i].len;
    ret += type_get_serial_size (t->types[i]);
  }

  return ret;
}

#ifdef TESTING
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
    struct string key = src->keys[i];
    ret               = srlizr_write (dest, (const u8 *)&key.len, sizeof (u16));
    ASSERT (ret);

    // KEY)
    ret = srlizr_write (dest, (u8 *)key.data, key.len);
    ASSERT (ret);

    // (TYPE)
    type_serialize (dest, src->types[i]);
  }
}

#ifdef TESTING
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
    struct allocator    *a,
    error               *e
)
{
  ASSERT (dest);
  BUILDER_INIT (b, a);

  struct kvt_list_builder unb = kvlb_create (&b);

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
        .data = allocate (a, key.len, 1, e),
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
  BUILDER_CLOSE (b);
  return error_trace (e);

early_termination:
  BUILDER_CLOSE (b);
  return struct_t_type_deser ("Early end of serialized string", e);
}

#ifdef TESTING
TEST (struct_t_deserialize_green_path)
{
  ALLOC_INIT (st_alloc);

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

  ALLOC_CLOSE (st_alloc);
}
#endif

#ifdef TESTING
TEST (struct_t_deserialize_red_path)
{
  ALLOC_INIT (alloc);

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

  struct struct_t     sret = {0};
  struct deserializer d    = dsrlizr_create (data, sizeof (data));

  error e   = error_create ();
  err_t ret = struct_t_deserialize (&sret, &d, &alloc, &e);

  test_assert_int_equal (ret, ERR_INTERP); // Duplicate

  ALLOC_CLOSE (alloc);
}
#endif

err_t
struct_t_random (struct struct_t *st, struct allocator *alloc, u32 depth, error *e)
{
  ASSERT (st);

  st->len = (u16)randu32r (1, 5);

  st->keys = allocate (alloc, st->len, sizeof (struct string), e);
  if (!st->keys)
  {
    return error_trace (e);
  }

  st->types = allocate (alloc, st->len, sizeof (struct type *), e);
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
