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

#include "nscore/errors.h"
#include "nscore/types.h"

#include <c_specx.h>
#include <string.h>

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
{ return error_causef (e, ERR_INTERP, "Strict Array: %s", msg); }

static err_t
sarray_t_type_deser (const char *msg, error *e)
{ return error_causef (e, ERR_CORRUPT, "Strict Array: %s", msg); }

static err_t
sarray_t_validate_shallow (const struct sarray_t *t, error *e)
{
  DBG_ASSERT (unchecked_sarray_t, t);
  if (t->rank == 0) { return sarray_t_type_err ("Rank must be > 0", e); }
  for (u32 i = 0; i < t->rank; ++i)
  {
    if (t->dims[i] == 0) { return sarray_t_type_err ("dimensions cannot be 0", e); }
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
    if (n < 0) { return n; }
    len += n;
    if (out)
    {
      out += n;
      if ((u32)n < avail) { avail -= n; }
      else
      {
        avail = 0;
      }
    }
  }

  n = type_snprintf (out, avail, p->t);
  if (n < 0) { return n; }
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
  for (u32 i = 0; i < t->rank; ++i) { ret *= t->dims[i]; }

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
  if (!dsrlizr_read ((u8 *)&sa.rank, sizeof (u16), src)) { goto early_terimination; }

  // Allocate dimensions buffer
  u32 *dims = chunk_malloc (a, sa.rank, sizeof *dims, e);
  if (dims == NULL) { return error_trace (e); }
  sa.dims = dims;

  // Allocate type
  struct type *t = chunk_malloc (a, 1, sizeof *t, e);
  if (t == NULL) { return error_trace (e); }
  sa.t = t;

  for (u32 i = 0; i < sa.rank; ++i)
  {
    u32 dim;

    // DIMi
    if (!dsrlizr_read ((u8 *)&dim, sizeof (u32), src)) { goto early_terimination; }

    sa.dims[i] = dim;
  }

  // (TYPE)
  sa.t = type_deserialize (src, a, e);
  if (sa.t == NULL) { return error_trace (e); }
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
sarray_t_random (struct sarray_t *sa, struct chunk_alloc *temp, u32 depth, error *e)
{
  ASSERT (sa);

  sa->rank = (u16)randu32r (1, 4);

  sa->dims = (u32 *)chunk_malloc (temp, sa->rank, sizeof (u32), e);
  if (!sa->dims) { return error_trace (e); }

  for (u16 i = 0; i < sa->rank; ++i) { sa->dims[i] = randu32r (1, 11); }

  sa->t = (struct type *)chunk_malloc (temp, 1, sizeof (struct type), e);
  if (!sa->t) { return error_trace (e); }

  sa->t = type_random (temp, depth - 1, e);
  if (sa->t == NULL) { return error_trace (e); }

  return SUCCESS;
}

bool
sarray_t_equal (const struct sarray_t *left, const struct sarray_t *right)
{
  if (left->rank != right->rank) { return false; }

  for (u32 i = 0; i < left->rank; ++i)
  {
    if (left->dims[i] != right->dims[i]) { return false; }
  }

  return true;
}
