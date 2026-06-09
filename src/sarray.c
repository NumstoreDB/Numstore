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

#include "error.h"
#include "numerics.h"
#include "platform.h"
#include "testing/testing.h"
#include "types.h"

/////////////////////////////////////////////////////////////////////
////// Strict Array

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
  struct sarray_t s = {
      .dims = (u32[]){10, 11, 12},
      .rank = 3,
      .t    = &(struct type){
          .type = T_PRIM,
          .p    = U32,
      },
  };

  char        buffer[200];
  const char *expected = "[10][11][12]u32";
  u32         len      = strlen (expected);

  int i = sarray_t_snprintf (buffer, 200, &s);

  test_assert_int_equal (i, len);
  test_assert_int_equal (strncmp (expected, buffer, len), 0);
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
