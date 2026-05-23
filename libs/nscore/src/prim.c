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

#include "nscore/prim.h"

#include "c_specx.h"
#include "nscore/errors.h"
#include "nscore/types.h"

#include <string.h>

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
    return error_causef (e, ERR_INTERP, "invalid prim type %d (valid range %d..%d)", *t, U8, CU128);
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
  if (n < 0) { return n; }
  len += n;

  return len;
}

#ifndef NTEST
TEST (prim_t_snprintf)
{
  enum prim_t p = F64;
  char        buf[32];
  const char *expect = "f64";
  u32         elen   = strlen (expect);

  int n = prim_t_snprintf (buf, sizeof buf, &p);
  test_assert_int_equal (n, elen);
  test_assert_int_equal (strncmp (expect, buf, elen), 0);
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
  if (!ret) { return error_causef (e, ERR_CORRUPT, "prim: missing length header"); }

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
{ return (enum prim_t)randu32r (U8, CU128); }

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

  if (string_equal (str, strfcstr ("u8"))) { return U8; }
  if (string_equal (str, strfcstr ("u16"))) { return U16; }
  if (string_equal (str, strfcstr ("u32"))) { return U32; }
  if (string_equal (str, strfcstr ("u64"))) { return U64; }
  if (string_equal (str, strfcstr ("i8"))) { return I8; }
  if (string_equal (str, strfcstr ("i16"))) { return I16; }
  if (string_equal (str, strfcstr ("i32"))) { return I32; }
  if (string_equal (str, strfcstr ("i64"))) { return I64; }
  if (string_equal (str, strfcstr ("f16"))) { return F16; }
  if (string_equal (str, strfcstr ("f32"))) { return F32; }
  if (string_equal (str, strfcstr ("f64"))) { return F64; }
  if (string_equal (str, strfcstr ("f128"))) { return F128; }
  if (string_equal (str, strfcstr ("cf32"))) { return CF32; }
  if (string_equal (str, strfcstr ("cf64"))) { return CF64; }
  if (string_equal (str, strfcstr ("cf128"))) { return CF128; }
  if (string_equal (str, strfcstr ("cf256"))) { return CF256; }
  if (string_equal (str, strfcstr ("ci16"))) { return CI16; }
  if (string_equal (str, strfcstr ("ci32"))) { return CI32; }
  if (string_equal (str, strfcstr ("ci64"))) { return CI64; }
  if (string_equal (str, strfcstr ("ci128"))) { return CI128; }
  if (string_equal (str, strfcstr ("cu16"))) { return CU16; }
  if (string_equal (str, strfcstr ("cu32"))) { return CU32; }
  if (string_equal (str, strfcstr ("cu64"))) { return CU64; }
  if (string_equal (str, strfcstr ("cu128"))) { return CU128; }

  return (enum prim_t) - 1;
}
