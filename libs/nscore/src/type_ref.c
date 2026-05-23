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

#include "nscore/parsers/type_ref.h"

#include "nscore/parsers/subtype.h"
#include "nscore/type_accessor.h"
#include "nscore/types.h"

bool
type_ref_equal (const struct type_ref left, const struct type_ref right)
{
  if (left.type != right.type) { return false; }

  switch (left.type)
  {
    case TR_TAKE:
    {
      return string_equal (left.tk.vname, right.tk.vname)
             && type_accessor_equal (left.tk.ta, right.tk.ta);
    }

    case TR_STRUCT:
    {
      if (left.st.len != right.st.len) { return false; }

      for (u16 i = 0; i < left.st.len; i++)
      {
        if (!string_equal (left.st.keys[i], right.st.keys[i])) { return false; }

        if (!type_ref_equal (left.st.types[i], right.st.types[i])) { return false; }
      }

      return true;
    }

    default: return false;
  }
}

struct type *
tr_construct (struct type *reftype, struct type_ref *tr, struct chunk_alloc *alloc, error *e)
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

      if (ret == NULL) { goto temp_failed; }

      // Struct building logic
      {
        struct kvt_list_builder builder;
        chunk_alloc_create_default (&temp);

        kvlb_create (&builder, &temp, alloc);

        for (u16 i = 0; i < len; ++i)
        {
          // The field name
          if (kvlb_accept_key (&builder, keys[i], e)) { goto temp_failed; }

          // Get the sub type
          // (recursively)
          struct type *subtype = tr_construct (reftype, &types[i], alloc, e);
          if (subtype == NULL) { goto temp_failed; }

          if (kvlb_accept_type (&builder, subtype, e)) { goto temp_failed; }
        }

        struct kvt_list kvl;
        if (kvlb_build (&kvl, &builder, e)) { goto temp_failed; }

        if (struct_t_create (&ret->st, kvl, alloc, e)) { goto temp_failed; }
      }

      chunk_alloc_free_all (&temp);
      return ret;
    }
  }

temp_failed:
  chunk_alloc_free_all (&temp);
  return NULL;
}
