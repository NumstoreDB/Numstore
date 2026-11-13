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

#include "types/ns_variables.h"

#include "ns_arena_alloc.h"
#include "ns_bounds.h"
#include "ns_compiler.h"
#include "ns_csx_assert.h"
#include "ns_error.h"
#include "ns_lexer.h"
#include "ns_logging.h"
#include "ns_numerics.h"
#include "ns_page_var_hash_page.h"
#include "ns_string.h"
#include "ns_testing.h"
#include "ns_utils.h"
#include "os/ns_malloc.h"
#include "types/ns_types.h"

#include <string.h>

err_t
i_print_variable (struct variable *v, error *e)
{
  DBG_ASSERT (variable, v);
  i_printf ("{\n");
  i_printf ("    \"Status\"          : \"Ok\",\n");
  i_printf ("    \"Name\"            : \"%.*s\",\n", strfmt (&v->vname));

  u32   len;
  char *var_str = get_var_str (v->dtype, &len, e);
  if (var_str == NULL) {
    return error_trace (e);
  }
  i_printf ("    \"DType\"           : \"%.*s\",\n", len, var_str);
  default_mem ().table->free (default_mem ().self, var_str);
  i_printf ("    \"DSize\"           : %" PRIu32 ",\n", type_byte_size (v->dtype));

  i_printf ("    \"Nelems\"          : %" PRb_size ",\n", v->nbytes / type_byte_size (v->dtype));
  i_printf ("    \"Bytes\"           : %" PRb_size ",\n", v->nbytes);

  // TODO: JSON spells it lowercase null, so a NULL root makes this output
  // unparseable as JSON. Check that nothing reads the uppercase form (the
  // print_all_vars app emits it) before changing it.
  if (v->var_root == PGNO_NULL) {
    i_printf ("    \"Variable_Root\"   : NULL\n");
  } else {
    i_printf ("    \"Variable_Root\"   : %" PRpgno "\n", v->var_root);
  }

  if (v->rpt_root == PGNO_NULL) {
    i_printf ("    \"Rope_Root\"       : NULL\n");
  } else {
    i_printf ("    \"Rope_Root\"       : %" PRpgno "\n", v->rpt_root);
  }

  i_printf ("}\n");

  return error_trace (e);
}

#ifndef NDEBUG
TEST (i_print_variable)
{
  struct variable v = {
      .vname    = strfcstr ("foo"),
      .dtype    = compile_type_alloc ("struct { a u32, b f32}", &ctx->alloc, &ctx->e),
      .var_root = 123,
      .rpt_root = 456,
      .nbytes   = 792, // 99 elements of the 8 byte struct
  };

  TEST_CASE ("A variable with both roots set")
  {
    test_assert_int_equal (i_print_variable (&v, &ctx->e), SUCCESS);
  }

  TEST_CASE ("A variable with no pages yet prints its roots as NULL")
  {
    v.var_root = PGNO_NULL;
    v.rpt_root = PGNO_NULL;
    test_assert_int_equal (i_print_variable (&v, &ctx->e), SUCCESS);
  }

  arena_alloc_free_all (&ctx->alloc);
}
#endif

err_t
var_copy (struct variable *dest, struct variable *src, struct arena_alloc *alloc, error *e)
{
  *dest = (struct variable){
      .vname =
          {
              .data = arena_alloc_copy (alloc, src->vname.data, src->vname.len, e),
              .len  = src->vname.len,
          },
      .var_root = src->var_root,
      .rpt_root = src->rpt_root,
      .nbytes   = src->nbytes,
  };

  if (dest->vname.data == NULL) {
    return error_trace (e);
  }

  dest->dtype = type_movemem (src->dtype, alloc, e);
  if (dest->dtype == NULL) {
    return error_trace (e);
  }

  return SUCCESS;
}

#ifndef NDEBUG
TEST (var_copy)
{
  struct variable src = {
      .vname    = strfcstr ("foo"),
      .dtype    = compile_type_alloc ("struct { a u32, b f32}", &ctx->alloc, &ctx->e),
      .var_root = 123,
      .rpt_root = 456,
      .nbytes   = 792,
  };

  struct variable dest;
  test_assert_int_equal (var_copy (&dest, &src, &ctx->alloc, &ctx->e), SUCCESS);

  test_assert (variable_equal (&dest, &src));
  test_assert (dest.vname.data != src.vname.data);
  test_assert (dest.dtype != src.dtype);

  arena_alloc_free_all (&ctx->alloc);
}
#endif

bool
variable_equal (const struct variable *left, const struct variable *right)
{
  DBG_ASSERT (variable, left);
  DBG_ASSERT (variable, right);
  if (!string_equal (left->vname, right->vname)) {
    return false;
  }
  if (!type_equal (left->dtype, right->dtype)) {
    return false;
  }
  if (left->var_root != right->var_root) {
    return false;
  }
  if (left->rpt_root != right->rpt_root) {
    return false;
  }
  if (left->nbytes != right->nbytes) {
    return false;
  }

  return true;
}

#ifndef NDEBUG
TEST (variable_equal)
{
  struct variable a = {
      .vname    = strfcstr ("foo"),
      .dtype    = compile_type_alloc ("struct { a u32, b f32}", &ctx->alloc, &ctx->e),
      .var_root = 123,
      .rpt_root = 456,
      .nbytes   = 792,
  };

  struct variable b = {
      .vname    = strfcstr ("foo"),
      .dtype    = compile_type_alloc ("struct { a u32, b f32}", &ctx->alloc, &ctx->e),
      .var_root = 123,
      .rpt_root = 456,
      .nbytes   = 792,
  };

  test_assert (variable_equal (&a, &b));

  a.vname = strfcstr ("biz");
  test_assert (!variable_equal (&a, &b));
  a.vname = strfcstr ("foo");

  a.dtype = compile_type_alloc ("u32", &ctx->alloc, &ctx->e);
  test_assert (!variable_equal (&a, &b));
  a.dtype    = compile_type_alloc ("struct { a u32, b f32}", &ctx->alloc, &ctx->e);

  a.var_root = 987;
  test_assert (!variable_equal (&a, &b));
  a.var_root = 123;

  a.rpt_root = 999;
  test_assert (!variable_equal (&a, &b));
  a.rpt_root = 456;

  a.nbytes   = 112;
  test_assert (!variable_equal (&a, &b));
  a.nbytes = 792;

  arena_alloc_free_all (&ctx->alloc);
}
#endif

err_t
validate_vname (struct string vname, error *e)
{
  if (vname.len == 0) {
    return error_causef (e, ERR_INVALID_ARGUMENT, "variable name is empty");
  }

  if (vname.len >= MAX_VARIABLE_LENGTH) {
    return error_causef (e, ERR_INVALID_ARGUMENT, "variable name exceeds 4096 chars");
  }

  if (!is_alpha (vname.data[0])) {
    return error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "variable name '%.*s' must start with a letter",
        (int)vname.len,
        vname.data
    );
  }

  for (u32 i = 1; i < vname.len; ++i) {
    char c = vname.data[i];
    if (!is_alpha_num_generous (c)) {
      return error_causef (
          e,
          ERR_INVALID_ARGUMENT,
          "variable name '%.*s' contains "
          "invalid characters",
          (int)vname.len,
          vname.data
      );
    }
  }

  return SUCCESS;
}

#ifndef NDEBUG
TEST (validate_vname)
{
  TEST_CASE ("Valid names")
  {
    test_assert_int_equal (validate_vname (strfcstr ("foo"), &ctx->e), SUCCESS);
    test_assert_int_equal (validate_vname (strfcstr ("_valid_Name123"), &ctx->e), SUCCESS);
  }

  TEST_CASE ("Invalid names")
  {
    test_err_t_check (validate_vname (strfcstr (""), &ctx->e), ERR_INVALID_ARGUMENT, &ctx->e);
    test_err_t_check (validate_vname (strfcstr ("1abc"), &ctx->e), ERR_INVALID_ARGUMENT, &ctx->e);
    test_err_t_check (
        validate_vname (strfcstr ("abc!def"), &ctx->e),
        ERR_INVALID_ARGUMENT,
        &ctx->e
    );

    static char long_name[5000];
    memset (long_name, 'a', sizeof (long_name));
    struct string too_long = {.data = long_name, .len = sizeof (long_name)};
    test_err_t_check (validate_vname (too_long, &ctx->e), ERR_INVALID_ARGUMENT, &ctx->e);
  }
}
#endif

err_t
rand_varname_same_hash (
    struct string      *name1,
    struct string      *name2,
    struct arena_alloc *alloc,
    error              *e
)
{
  ASSERT (name1);
  ASSERT (name2);
  ASSERT (alloc);

  char temp[20];

  while (true) {
    u32 len1 = randu32r (5, 10);
    u32 len2 = randu32r (5, 10);

    var_random_name (temp, len1);
    var_random_name (temp + len1, len2);

    name1->data  = temp;
    name2->data  = temp + len1;
    name1->len   = len1;
    name2->len   = len2;

    p_size hpos1 = vh_get_hash_pos (*name1);
    p_size hpos2 = vh_get_hash_pos (*name2);

    if (hpos1 == hpos2) {
      // Copy the pair out of [temp] into [alloc]
      char *data = arena_malloc (alloc, len1 + len2, 1, e);
      if (data == NULL) {
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
    struct arena_alloc *alloc,
    error              *e
)
{
  ASSERT (name1);
  ASSERT (name2);
  ASSERT (alloc);

  char temp[20];

  while (true) {
    u32 len1 = randu32r (5, 10);
    u32 len2 = randu32r (5, 10);

    var_random_name (temp, len1);
    var_random_name (temp + len1, len2);

    name1->data  = temp;
    name2->data  = temp + len1;
    name1->len   = len1;
    name2->len   = len2;

    p_size hpos1 = vh_get_hash_pos (*name1);
    p_size hpos2 = vh_get_hash_pos (*name2);

    if (hpos1 != hpos2) {
      // Copy the pair out of [temp] into [alloc]
      char *data = arena_malloc (alloc, len1 + len2, 1, e);
      if (data == NULL) {
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

#ifndef NDEBUG
TEST (rand_varname_same_hash)
{
  for (int i = 0; i < 10; ++i) {
    struct string name1;
    struct string name2;
    rand_varname_same_hash (&name1, &name2, &ctx->alloc, &ctx->e);
    test_assert_type_equal (vh_get_hash_pos (name1), vh_get_hash_pos (name2), p_size, PRp_size);
  }

  arena_alloc_free_all (&ctx->alloc);

  TEST_CASE ("A failed copy into the arena is reported")
  {
    struct string name1;
    struct string name2;
    faulty_mem_fail_after (ctx->faulty_mem, 0);
    test_err_t_check (
        rand_varname_same_hash (&name1, &name2, &ctx->faulty_alloc, &ctx->e),
        ERR_NOMEM,
        &ctx->e
    );
    faulty_mem_fail_after (ctx->faulty_mem, 0);
    test_err_t_check (
        rand_varname_different_hash (&name1, &name2, &ctx->faulty_alloc, &ctx->e),
        ERR_NOMEM,
        &ctx->e
    );
    faulty_mem_reset (ctx->faulty_mem);
  }
}

TEST (rand_varname_different_hash)
{
  for (int i = 0; i < 10; ++i) {
    struct string name1;
    struct string name2;
    rand_varname_different_hash (&name1, &name2, &ctx->alloc, &ctx->e);
    test_assert (vh_get_hash_pos (name1) != vh_get_hash_pos (name2));
  }

  arena_alloc_free_all (&ctx->alloc);
}
#endif

err_t
var_resolve_index_ofst_size (
    struct variable *v,
    b_size          *dest,
    sb_size          sofst,
    t_size           tsize,
    error           *e
)
{

  // Signed byte offset
  sb_size sbofst;
  if (safe_mul_i64_err (&sbofst, sofst, (i64)tsize, e) < 0) {
    return error_trace (e);
  }

  *dest = var_resolve_index (v, sbofst);

  return SUCCESS;
}

b_size
var_resolve_index (struct variable *v, sb_size bofst)
{
  ASSERT (v->nbytes <= (b_size)INT64_MAX);

  // A negative offset counts back from the end
  if (bofst < 0) {
    bofst = (sb_size)v->nbytes + bofst;
  }

  // Further back than the start clamps to the start
  if (bofst < 0) {
    bofst = 0;
  }

  // > rather than >=, so nbytes itself is valid (append)
  if ((b_size)bofst > v->nbytes) {
    bofst = (sb_size)v->nbytes;
  }

  ASSERT (bofst >= 0);
  return (b_size)bofst;
}

#ifndef NDEBUG
TEST (var_resolve_index)
{
  struct variable v = {.nbytes = 100};

  // Positive offset within range is unchanged.
  test_assert_type_equal (var_resolve_index (&v, 10), 10U, b_size, PRb_size);

  // Offset exactly at nbytes is valid (append position).
  test_assert_type_equal (var_resolve_index (&v, 100), 100U, b_size, PRb_size);

  // Offset past nbytes is clamped to nbytes.
  test_assert_type_equal (var_resolve_index (&v, 150), 100U, b_size, PRb_size);

  // Offset of 0 is unchanged.
  test_assert_type_equal (var_resolve_index (&v, 0), 0U, b_size, PRb_size);

  // Negative offset is translated relative to nbytes.
  test_assert_type_equal (var_resolve_index (&v, -10), 90U, b_size, PRb_size);

  // Negative offset equal to -nbytes translates to 0.
  test_assert_type_equal (var_resolve_index (&v, -100), 0U, b_size, PRb_size);

  // Negative offset more negative than -nbytes clamps to 0.
  test_assert_type_equal (var_resolve_index (&v, -150), 0U, b_size, PRb_size);
}
#endif

b_size
var_resolve_nelem (struct variable *v, b_size bofst, b_size nelem, t_size size, sb_size stride)
{
  ASSERT (size > 0);
  ASSERT (stride > 0);
  ASSERT (bofst <= v->nbytes);

  // Element i starts at bofst + i * stride * size and must fit as a whole
  const b_size avail = v->nbytes - bofst;
  if (avail < size) {
    return 0;
  }

  // A step too big to count reaches no second element
  b_size step;
  b_size fit = 1;
  if (safe_mul_u64 (&step, (b_size)stride, size)) {
    fit += (avail - size) / step;
  }

  return MIN (nelem, fit);
}

#ifndef NDEBUG
TEST (var_resolve_nelem)
{
  struct variable v = {.nbytes = 100};

  // nelem within the available remainder is unchanged.
  test_assert_type_equal (var_resolve_nelem (&v, 0, 10, 1, 1), 10U, b_size, PRb_size);

  // nelem exactly at the remainder is unchanged.
  test_assert_type_equal (var_resolve_nelem (&v, 0, 100, 1, 1), 100U, b_size, PRb_size);

  // nelem past the remainder is clamped down to it.
  test_assert_type_equal (var_resolve_nelem (&v, 0, 200, 1, 1), 100U, b_size, PRb_size);

  // Remainder shrinks as bofst moves forward.
  test_assert_type_equal (var_resolve_nelem (&v, 90, 20, 1, 1), 10U, b_size, PRb_size);

  // Larger element size reduces the available element count
  // (integer division truncates any partial trailing element).
  test_assert_type_equal (var_resolve_nelem (&v, 0, 100, 8, 1), 12U, b_size, PRb_size);

  // bofst == nbytes leaves no remainder at all.
  test_assert_type_equal (var_resolve_nelem (&v, 100, 5, 1, 1), 0U, b_size, PRb_size);

  // A stride spreads the elements out: 8 byte elements every 16 bytes start
  // at 0, 16, ... 80. The one at 96 doesn't fit.
  test_assert_type_equal (var_resolve_nelem (&v, 0, 100, 8, 2), 6U, b_size, PRb_size);

  // The last element only has to fit itself, not a whole stride.
  test_assert_type_equal (var_resolve_nelem (&v, 0, 100, 4, 24), 2U, b_size, PRb_size);

  // Less than one element left.
  test_assert_type_equal (var_resolve_nelem (&v, 97, 5, 4, 3), 0U, b_size, PRb_size);

  // A stride * size that overflows still reaches the first element.
  test_assert_type_equal (var_resolve_nelem (&v, 0, 5, 4, INT64_MAX), 1U, b_size, PRb_size);
}
#endif

err_t
variable_copy (
    struct variable       *dest,
    const struct variable *src,
    struct arena_alloc    *alloc,
    error                 *e
)
{
  DBG_ASSERT (variable, src);
  // TODO: the +1 copies a byte past vname.len, which over-reads any name
  // that isn't NUL-terminated. Drop it once no caller relies on the copy
  // being NUL-terminated.
  dest->vname.data = arena_alloc_copy (alloc, src->vname.data, src->vname.len + 1, e);
  dest->dtype      = type_movemem (src->dtype, alloc, e);
  if (dest->vname.data == NULL) {
    return error_trace (e);
  }
  if (dest->dtype == NULL) {
    return error_trace (e);
  }

  dest->vname.len = src->vname.len;
  dest->var_root  = src->var_root;
  dest->rpt_root  = src->rpt_root;
  dest->nbytes    = src->nbytes;

  return SUCCESS;
}

struct string
vname_or_default (const char *name)
{
  if (name != NULL) {
    return strfcstr (name);
  }
  return strfcstr (DEFAULT_VARIABLE);
}

#ifndef NDEBUG
TEST (vname_or_default)
{
  test_assert (string_equal (vname_or_default ("foo"), strfcstr ("foo")));
  test_assert (string_equal (vname_or_default (NULL), strfcstr (DEFAULT_VARIABLE)));
}
#endif
