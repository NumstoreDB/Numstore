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

#include "variables.h"

#include "alloc.h"
#include "compiler.h"
#include "error.h"
#include "page.h"
#include "types.h"

#ifdef TESTING
#  include "testing/testing.h"
#endif

err_t
i_print_variable (struct variable *v, error *e)
{
  i_log_info ("=========== Variable: %.*s\n", strfmt (&v->vname));
  i_log_info ("   root: %" PRpgno "\n", v->var_root);
  i_log_info ("   array root: %" PRpgno "\n", v->rpt_root);
  i_log_info ("   nbytes: %" PRb_size "\n", v->nbytes);
  i_log_info ("   Data type:\n");
  err_t ret = i_log_type (v->dtype, e);
  i_log_info ("===========\n");
  return ret;
}

#ifdef TESTING
TEST (i_print_variable)
{
  ALLOC_INIT (alloc);

  error e = error_create ();

  struct variable v = {
      .vname    = strfcstr ("foo"),
      .dtype    = compile_type_alloc ("struct { a u32, b f32}", &alloc, &e),
      .var_root = 123,
      .rpt_root = 456,
      .nbytes   = 789,
  };

  // Just logging - make sure it succeeds and doesn't crash.
  test_assert (i_print_variable (&v, &e) == SUCCESS);

  ALLOC_CLOSE (alloc);
}
#endif

bool
variable_equal (const struct variable *left, const struct variable *right)
{
  if (!string_equal (left->vname, right->vname))
  {
    return false;
  }
  if (!type_equal (left->dtype, right->dtype))
  {
    return false;
  }
  if (left->var_root != right->var_root)
  {
    return false;
  }
  if (left->rpt_root != right->rpt_root)
  {
    return false;
  }
  if (left->nbytes != right->nbytes)
  {
    return false;
  }

  return true;
}

#ifdef TESTING
TEST (variable_equal)
{
  ALLOC_INIT (alloc);

  error e = error_create ();

  struct variable a = {
      .vname    = strfcstr ("foo"),
      .dtype    = compile_type_alloc ("struct { a u32, b f32}", &alloc, &e),
      .var_root = 123,
      .rpt_root = 456,
      .nbytes   = 789,
  };

  struct variable b = {
      .vname    = strfcstr ("foo"),
      .dtype    = compile_type_alloc ("struct { a u32, b f32}", &alloc, &e),
      .var_root = 123,
      .rpt_root = 456,
      .nbytes   = 789,
  };

  test_assert (variable_equal (&a, &b));

  a.vname = strfcstr ("biz");
  test_assert (!variable_equal (&a, &b));
  a.vname = strfcstr ("foo");

  a.dtype = compile_type_alloc ("u32", &alloc, &e);
  test_assert (!variable_equal (&a, &b));
  a.dtype = compile_type_alloc ("struct { a u32, b f32}", &alloc, &e);

  a.var_root = 987;
  test_assert (!variable_equal (&a, &b));
  a.var_root = 123;

  a.rpt_root = 999;
  test_assert (!variable_equal (&a, &b));
  a.rpt_root = 456;

  a.nbytes = 111;
  test_assert (!variable_equal (&a, &b));
  a.nbytes = 789;

  ALLOC_CLOSE (alloc);
}
#endif

err_t
validate_vname (struct string vname, error *e)
{
  if (vname.len == 0)
  {
    return error_causef (e, ERR_INVALID_ARGUMENT, "variable name is empty");
  }

  if (vname.len >= 4096)
  {
    return error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "variable name exceeds 4096 chars"
    );
  }

  if (!is_alpha (vname.data[0]))
  {
    return error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "variable name '%.*s' must start with a letter",
        vname.len,
        vname.data
    );
  }

  for (u32 i = 1; i < vname.len; ++i)
  {
    char c = vname.data[i];
    if (!is_alpha_num_generous (c))
    {
      return error_causef (
          e,
          ERR_INVALID_ARGUMENT,
          "variable name '%.*s' contains "
          "invalid characters",
          vname.len,
          vname.data
      );
    }
  }

  return SUCCESS;
}

#ifdef TESTING
TEST (validate_vname)
{
  error e = error_create ();

  // Valid: plain letters.
  test_assert (validate_vname (strfcstr ("foo"), &e) == SUCCESS);

  // Valid: leading underscore, mixed digits/letters after.
  test_assert (validate_vname (strfcstr ("_valid_Name123"), &e) == SUCCESS);

  // Invalid: empty name.
  test_assert (validate_vname (strfcstr (""), &e) != SUCCESS);

  // Invalid: starts with a digit.
  test_assert (validate_vname (strfcstr ("1abc"), &e) != SUCCESS);

  // Invalid: contains a character outside the allowed set.
  test_assert (validate_vname (strfcstr ("abc!def"), &e) != SUCCESS);

  // Invalid: name is too long (>= 4096 chars).
  static char long_name[5000];
  memset (long_name, 'a', sizeof (long_name));
  struct string too_long = {.data = long_name, .len = sizeof (long_name)};
  test_assert (validate_vname (too_long, &e) != SUCCESS);
}
#endif

// Pool 1: Valid first characters (Letters and underscore)
const char alpha_pool[] =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "_";

// Pool 2: Valid remaining characters (Letters, digits, underscore, dot, slash,
// hyphen)
const char generous_pool[] =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789"
    "_";

void
var_random_name (char *buffer, int length)
{
  if (length <= 0)
  {
    return;
  }

  int alpha_size    = sizeof (alpha_pool) - 1;
  int generous_size = sizeof (generous_pool) - 1;

  // First char must strictly be an alpha or underscore
  buffer[0] = alpha_pool[randu32 () % alpha_size];

  // Remaining chars can use the generous pool
  for (int i = 1; i < length - 1; i++)
  {
    buffer[i] = generous_pool[randu32 () % generous_size];
  }
  buffer[length - 1] = '\0';
}

#ifdef TESTING

static bool
test_char_in_pool (char c, const char *pool, int pool_size)
{
  for (int i = 0; i < pool_size; i++)
  {
    if (pool[i] == c)
    {
      return true;
    }
  }
  return false;
}

TEST (var_random_name)
{
  // First char is always from alpha_pool, and the last is always the
  // null terminator, with everything in between from generous_pool.
  for (int trial = 0; trial < 10; trial++)
  {
    char buf[16];
    var_random_name (buf, sizeof (buf));

    test_assert (
        test_char_in_pool (buf[0], alpha_pool, sizeof (alpha_pool) - 1)
    );
    test_assert_int_equal (buf[sizeof (buf) - 1], '\0');

    for (u32 i = 1; i < sizeof (buf) - 1; i++)
    {
      test_assert (
          test_char_in_pool (buf[i], generous_pool, sizeof (generous_pool) - 1)
      );
    }
  }

  // length <= 0 should leave the buffer untouched.
  char sentinel[4] = {'x', 'x', 'x', 'x'};
  var_random_name (sentinel, 0);
  test_assert (memcmp (sentinel, "xxxx", 4) == 0);
  var_random_name (sentinel, -1);
  test_assert (memcmp (sentinel, "xxxx", 4) == 0);

  // NOTE: with length == 1, the null terminator write at
  // buffer[length - 1] overwrites the alpha char written moments
  // before, so the "name" ends up being just '\0'.
  char single[1];
  var_random_name (single, 1);
  test_assert_int_equal (single[0], '\0');
}

#endif // TESTING

err_t
rand_varname (
    struct string    *dest,
    struct allocator *alloc,
    const u32         minlen,
    const u32         maxlen,
    error            *e
)
{
  ASSERT (dest);
  ASSERT (alloc);
  ASSERT (minlen > 0);
  ASSERT (minlen <= maxlen);

  u32   len    = randu32r (minlen, maxlen);
  char *buffer = allocate (alloc, len, 1, e);
  if (buffer == NULL)
  {
    return error_trace (e);
  }
  var_random_name (buffer, len);

  dest->data = buffer;
  dest->len  = len;

  return SUCCESS;
}

#ifdef TESTING
TEST (rand_varname)
{
  ALLOC_INIT (alloc);

  error e = error_create ();

  for (int i = 0; i < 10; ++i)
  {
    struct string name;
    err_t         ret = rand_varname (&name, &alloc, 5, 10, &e);

    test_assert_int_equal (ret, SUCCESS);
    test_assert (name.len >= 5 && name.len <= 10);
    test_assert (
        test_char_in_pool (name.data[0], alpha_pool, sizeof (alpha_pool) - 1)
    );
    test_assert_int_equal (name.data[name.len - 1], '\0');
  }

  ALLOC_CLOSE (alloc);
}
#endif

err_t
rand_varname_same_hash (
    struct string    *name1,
    struct string    *name2,
    struct allocator *alloc,
    error            *e
)
{
  ASSERT (name1);
  ASSERT (name2);
  ASSERT (alloc);

  char temp[20];

  while (true)
  {
    // Random sizes
    u32 len1 = randu32r (5, 10);
    u32 len2 = randu32r (5, 10);

    // Random names
    var_random_name (temp, len1);
    var_random_name (temp + len1, len2);

    // Assign them
    name1->data = temp;
    name2->data = temp + len1;
    name1->len  = len1;
    name2->len  = len2;

    // Get hash positions
    p_size hpos1 = vh_get_hash_pos (*name1);
    p_size hpos2 = vh_get_hash_pos (*name2);

    // Check if they are good
    if (hpos1 == hpos2)
    {
      // commit strings - copy them to dest
      char *data = allocate (alloc, len1 + len2, 1, e);
      if (data == NULL)
      {
        goto failed;
      }
      memcpy (data, name1->data, len1);
      memcpy (data + len1, name2->data, len2);
      name1->data = data;
      name2->data = data + len1;
      break;
    }
  }

  return SUCCESS;

failed:
  return error_trace (e);
}

err_t
rand_varname_different_hash (
    struct string    *name1,
    struct string    *name2,
    struct allocator *alloc,
    error            *e
)
{
  ASSERT (name1);
  ASSERT (name2);
  ASSERT (alloc);

  char temp[20];

  while (true)
  {
    // Random sizes
    u32 len1 = randu32r (5, 10);
    u32 len2 = randu32r (5, 10);

    // Random names
    var_random_name (temp, len1);
    var_random_name (temp + len1, len2);

    // Assign them
    name1->data = temp;
    name2->data = temp + len1;
    name1->len  = len1;
    name2->len  = len2;

    // Get hash positions
    p_size hpos1 = vh_get_hash_pos (*name1);
    p_size hpos2 = vh_get_hash_pos (*name2);

    // Check if they are good
    if (hpos1 != hpos2)
    {
      // commit strings - copy them to dest
      char *data = allocate (alloc, len1 + len2, 1, e);
      if (data == NULL)
      {
        goto failed;
      }
      memcpy (data, name1->data, len1);
      memcpy (data + len1, name2->data, len2);
      name1->data = data;
      name2->data = data + len1;
      break;
    }
  }

  return SUCCESS;

failed:
  return error_trace (e);
}

#ifdef TESTING
TEST (rand_varname_same_hash)
{
  ALLOC_INIT (alloc);

  error e = error_create ();

  for (int i = 0; i < 10; ++i)
  {
    struct string name1;
    struct string name2;
    rand_varname_same_hash (&name1, &name2, &alloc, &e);
    test_assert_int_equal (vh_get_hash_pos (name1), vh_get_hash_pos (name2));
  }

  ALLOC_CLOSE (alloc);
}

TEST (rand_varname_different_hash)
{
  ALLOC_INIT (alloc);

  error e = error_create ();

  for (int i = 0; i < 10; ++i)
  {
    struct string name1;
    struct string name2;
    rand_varname_different_hash (&name1, &name2, &alloc, &e);
    test_assert (vh_get_hash_pos (name1) != vh_get_hash_pos (name2));
  }

  ALLOC_CLOSE (alloc);
}
#endif
