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

#include "nscore/types/ns_type_ref.h"

#include <stdbool.h>
#include <stddef.h>

#include "core/ns_alloc.h"
#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "core/ns_string.h"
#include "core/testing/ns_testing.h"
#include "nscore/compiler/ns_compiler.h"
#include "nscore/types/ns_kvt.h"
#include "nscore/types/ns_struct_t.h"
#include "nscore/types/ns_type_accessor.h"
#include "nscore/types/ns_types.h"

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

#ifdef TESTING
TEST (type_ref_equal)
{
  ALLOC_INIT (alloc);
  error e = error_create ();

#  define TRE_TC(left, right, expected)                            \
    TEST_CASE ("%s == %s == %d", left, right, expected)            \
    {                                                              \
      struct type_ref left_tr;                                     \
      struct type_ref right_tr;                                    \
                                                                   \
      compile_type_ref (&left_tr, left, &alloc, &e);               \
      compile_type_ref (&right_tr, right, &alloc, &e);             \
                                                                   \
      if (expected)                                                \
      {                                                            \
        test_assert (type_ref_equal (left_tr, right_tr));          \
      }                                                            \
      else                                                         \
      {                                                            \
        test_assert (type_ref_equal (left_tr, right_tr) == false); \
      }                                                            \
    }

  TRE_TC ("foo", "foo", true);
  TRE_TC ("foo.bar", "foo.bar", true);
  TRE_TC ("struct { a foo }", "struct { a foo }", true);
  TRE_TC ("struct { a foo, b biz }", "struct { a foo, b biz }", true);
  TRE_TC ("struct { a foo.bar, b biz }", "struct { a foo.bar, b biz }", true);
  TRE_TC (
      "struct { a foo.bar, b struct { c biz, d bar.baz } }",
      "struct { a foo.bar, b struct { c biz, d bar.baz } }",
      true
  );

  TRE_TC ("foo", "bar", false);
  TRE_TC ("foo", "foo.bar", false);
  TRE_TC ("foo.biz", "foo.bar", false);
  TRE_TC ("foo", "struct { a foo }", false);
  TRE_TC ("struct { a bar }", "struct { a foo }", false);
  TRE_TC ("struct { b foo }", "struct { a foo }", false);
  TRE_TC ("struct { a foo.bar }", "struct { a foo.biz }", false);
  TRE_TC (
      "struct { a foo.bar, b struct { c foo, d bar } }",
      "struct { a foo.bar, b struct { c foo, d biz } }",
      false
  );

  ALLOC_CLOSE (alloc);
}
#endif

static struct type *
tr_construct_inner (struct type *reftype, struct type_ref *tr, struct builder *b, error *e)
{
  switch (tr->type)
  {
    case TR_TAKE:
    {
      struct type_accessor *ta = &tr->tk.ta;
      return ta_subtype (reftype, ta, b->persistent, e);
    }

    case TR_STRUCT:
    {
      u16              len   = tr->st.len;
      struct string   *keys  = tr->st.keys;
      struct type_ref *types = tr->st.types;

      struct type *ret = builder_malloc_persist (b, 1, sizeof *ret, e);

      if (ret == NULL)
      {
        return NULL;
      }

      // Struct building logic
      {
        struct kvt_list_builder builder = kvlb_create (b);

        for (u16 i = 0; i < len; ++i)
        {
          // The field name
          if (kvlb_accept_key (&builder, keys[i], e))
          {
            return NULL;
          }

          // Get the sub type
          // (recursively)
          struct type *subtype = tr_construct_inner (reftype, &types[i], b, e);
          if (subtype == NULL)
          {
            return NULL;
          }

          if (kvlb_accept_type (&builder, subtype, e))
          {
            return NULL;
          }
        }

        struct kvt_list kvl;
        if (kvlb_build (&kvl, &builder, e))
        {
          return NULL;
        }

        if (struct_t_create (&ret->st, kvl, b->persistent, e))
        {
          return NULL;
        }
        ret->type = T_STRUCT;
      }

      return ret;
    }
    default:
    {
      UNREACHABLE (); // LCOV_EXCL_LINE
      return 0;       // LCOV_EXCL_LINE
    }
  }
}

struct type *
tr_construct (struct type *reftype, struct type_ref *tr, struct allocator *alloc, error *e)
{
  BUILDER_INIT (b, alloc);
  struct type *ret = tr_construct_inner (reftype, tr, &b, e);
  BUILDER_CLOSE (b);
  return ret;
}

#ifdef TESTING
TEST (tr_construct)
{
  ALLOC_INIT (alloc);
  error e = error_create ();

#  define TRC_TC(typestr, trstr, expectedstr)                             \
    TEST_CASE ("tr_construct(%s, %s) == %s", typestr, trstr, expectedstr) \
    {                                                                     \
      struct type     base_type;                                          \
      struct type_ref tr;                                                 \
      struct type     expected_type;                                      \
                                                                          \
      compile_type (&base_type, typestr, &alloc, &e);                     \
      compile_type_ref (&tr, trstr, &alloc, &e);                          \
      compile_type (&expected_type, expectedstr, &alloc, &e);             \
                                                                          \
      struct type *actual = tr_construct (&base_type, &tr, &alloc, &e);   \
                                                                          \
      test_assert (type_equal (&expected_type, actual));                  \
    }

#  define TRC_TC_FAIL(typestr, trstr)                                   \
    TEST_CASE ("tr_construct(%s, %s) == FAIL", typestr, trstr)          \
    {                                                                   \
      struct type     base_type;                                        \
      struct type_ref tr;                                               \
                                                                        \
      compile_type (&base_type, typestr, &alloc, &e);                   \
      compile_type_ref (&tr, trstr, &alloc, &e);                        \
                                                                        \
      struct type *actual = tr_construct (&base_type, &tr, &alloc, &e); \
                                                                        \
      test_assert (actual == NULL);                                     \
      e.cause_code = SUCCESS;                                           \
      e.cmlen      = 0;                                                 \
    }

  TRC_TC ("u8", "foo", "u8");
  TRC_TC_FAIL ("u8", "foo.bar");

  TRC_TC ("struct { a u8 }", "foo", "struct { a u8 }");
  TRC_TC ("struct { a u8 }", "foo.a", "u8");
  TRC_TC ("struct { a u8, b struct { c u16 } }", "foo.b", "struct { c u16 }");
  TRC_TC ("struct { a u8, b struct { c u16 } }", "foo.b.c", "u16");
  TRC_TC_FAIL ("struct { a u8 }", "foo.bar");
  TRC_TC_FAIL ("struct { a u8 }", "foo.a.b");

  TRC_TC ("struct { a u8, b [10][20]u16 }", "foo.b", "[10][20]u16");

  TRC_TC (
      "struct { a u8, b struct { c u16 } }",
      "struct { a foo.b, b foo.a }",
      "struct { a struct { c u16 }, b u8 }"
  );

  ALLOC_CLOSE (alloc);
}
#endif
