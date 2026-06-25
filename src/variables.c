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

err_t
rand_varname (
    struct string      *dest,
    struct chunk_alloc *alloc,
    const u32           minlen,
    const u32           maxlen,
    error              *e
)
{
  ASSERT (dest);
  ASSERT (alloc);
  ASSERT (minlen > 0);
  ASSERT (minlen <= maxlen);

  u32   len    = randu32r (minlen, maxlen);
  char *buffer = chunk_malloc (alloc, len, 1, e);
  if (buffer == NULL)
  {
    return error_trace (e);
  }
  var_random_name (buffer, len);

  dest->data = buffer;
  dest->len  = len;

  return SUCCESS;
}

err_t
rand_varname_same_hash (
    struct string      *name1,
    struct string      *name2,
    struct chunk_alloc *alloc,
    error              *e
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
      char *data = chunk_malloc (alloc, len1 + len2, 1, e);
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
    struct string      *name1,
    struct string      *name2,
    struct chunk_alloc *alloc,
    error              *e
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
      char *data = chunk_malloc (alloc, len1 + len2, 1, e);
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
  struct chunk_alloc alloc;
  chunk_alloc_create_default (&alloc);
  error e = error_create ();

  for (int i = 0; i < 10; ++i)
  {
    struct string name1;
    struct string name2;
    rand_varname_same_hash (&name1, &name2, &alloc, &e);
    test_assert_int_equal (vh_get_hash_pos (name1), vh_get_hash_pos (name2));
  }

  chunk_alloc_free_all (&alloc);
}

TEST (rand_varname_different_hash)
{
  struct chunk_alloc alloc;
  chunk_alloc_create_default (&alloc);
  error e = error_create ();

  for (int i = 0; i < 10; ++i)
  {
    struct string name1;
    struct string name2;
    rand_varname_different_hash (&name1, &name2, &alloc, &e);
    test_assert (vh_get_hash_pos (name1) != vh_get_hash_pos (name2));
  }

  chunk_alloc_free_all (&alloc);
}
#endif

struct variable *
variable_malloc_copy (struct variable *v, struct malloc_plan *plan)
{
  bool active = plan->mode == MP_ALLOCING;

  struct variable *ret = malloc_plan_memcpy (plan, v, sizeof (struct variable));

  if (active)
  {
    ret->dtype = type_malloc_copy (ret->dtype, plan);
  }

  if (active)
  {
    ret->vname.data =
        malloc_plan_memcpy (plan, ret->vname.data, ret->vname.len);
  }

  return ret;
}
