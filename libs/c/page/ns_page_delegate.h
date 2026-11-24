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

#ifndef NS_PAGE_DELEGATE_H
#define NS_PAGE_DELEGATE_H

#include "ns_csx_assert.h"
#include "ns_page.h"
#include "ns_page_data_list.h"
#include "ns_page_inner_node.h"
#include "ns_page_var_page.h"
#include "ns_page_var_tail.h"
#include "ns_stdtypes.h"

/******************************************************************************
 * SECTION: Page Delegate
 ******************************************************************************/

typedef union {
  struct dl_data dl;
  struct in_data in;
} in_dl_data;

#define dlgt_move_all_right(src, dest) dlgt_move_right (src, dest, dlgt_get_len (src))
#define dlgt_move_all_left(dest, src)  dlgt_move_left (dest, src, dlgt_get_len (src))

HEADER_FUNC pgno
dlgt_get_next (const page *p)
{
  DBG_ASSERT (page_base, p);
  switch (page_get_type (p)) {
    case PG_INNER_NODE: {
      return in_get_next (p);
    }
    case PG_DATA_LIST: {
      return dl_get_next (p);
    }
    case PG_VAR_PAGE: {
      return vp_get_next (p);
    }
    case PG_VAR_TAIL: {
      return vt_get_next (p);
    }
    default: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
}

HEADER_FUNC pgno
dlgt_get_ovnext (const page *p)
{
  DBG_ASSERT (page_base, p);
  switch (page_get_type (p)) {
    case PG_VAR_PAGE: {
      return vp_get_ovnext (p);
    }
    case PG_VAR_TAIL: {
      return vt_get_next (p);
    }
    default: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
}

HEADER_FUNC void
dlgt_set_next (page *p, const pgno n)
{
  DBG_ASSERT (page_base, p);
  switch (page_get_type (p)) {
    case PG_INNER_NODE: {
      in_set_next (p, n);
      break;
    }
    case PG_DATA_LIST: {
      dl_set_next (p, n);
      break;
    }
    case PG_VAR_PAGE: {
      vp_set_next (p, n);
      break;
    }
    case PG_VAR_TAIL: {
      vt_set_next (p, n);
      break;
    }
    default: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
}

HEADER_FUNC pgno
dlgt_get_prev (const page *p)
{
  DBG_ASSERT (page_base, p);
  switch (page_get_type (p)) {
    case PG_INNER_NODE: {
      return in_get_prev (p);
    }
    case PG_DATA_LIST: {
      return dl_get_prev (p);
    }
    default: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
}

HEADER_FUNC void
dlgt_set_prev (page *p, const pgno prev)
{
  DBG_ASSERT (page_base, p);
  switch (page_get_type (p)) {
    case PG_INNER_NODE: {
      in_set_prev (p, prev);
      break;
    }
    case PG_DATA_LIST: {
      dl_set_prev (p, prev);
      break;
    }
    default: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
}

HEADER_FUNC void
dlgtset_ovnext (page *p, const pgno next)
{
  DBG_ASSERT (page_base, p);
  switch (page_get_type (p)) {
    case PG_VAR_PAGE: {
      vp_set_ovnext (p, next);
      break;
    }
    case PG_VAR_TAIL: {
      vt_set_next (p, next);
      break;
    }
    default: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
}

HEADER_FUNC p_size
dlgt_get_len (const page *p)
{
  DBG_ASSERT (page_base, p);
  switch (page_get_type (p)) {
    case PG_INNER_NODE: {
      return in_get_len (p);
    }
    case PG_DATA_LIST: {
      return dl_used (p);
    }
    default: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
}

HEADER_FUNC b_size
dlgt_get_size (const page *p)
{
  DBG_ASSERT (page_base, p);
  switch (page_get_type (p)) {
    case PG_INNER_NODE: {
      return in_get_size (p);
    }
    case PG_DATA_LIST: {
      return dl_used (p);
    }
    default: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
}

HEADER_FUNC p_size
dlgt_get_max_len (const page *p)
{
  DBG_ASSERT (page_base, p);
  switch (page_get_type (p)) {
    case PG_INNER_NODE: {
      return IN_MAX_KEYS;
    }
    case PG_DATA_LIST: {
      return DL_DATA_SIZE;
    }
    default: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
}

HEADER_FUNC bool
dlgt_is_root (const page *p)
{
  DBG_ASSERT (page_base, p);
  return (dlgt_get_prev (p) == PGNO_NULL && dlgt_get_next (p) == PGNO_NULL) != 0;
}

HEADER_FUNC bool
dlgt_is_full (const page *p)
{
  DBG_ASSERT (page_base, p);
  switch (page_get_type (p)) {
    case PG_INNER_NODE: {
      return in_full (p);
    }
    case PG_DATA_LIST: {
      return dl_full (p);
    }
    default: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
}

HEADER_FUNC struct bytes
dlgt_get_bytes (page *p)
{
  DBG_ASSERT (page_base, p);
  switch (page_get_type (p)) {
    case PG_VAR_PAGE: {
      return vp_get_bytes (p);
    }
    case PG_VAR_TAIL: {
      return vt_get_bytes (p);
    }
    default: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
}

HEADER_FUNC struct cbytes
dlgt_get_bytes_imut (const page *p)
{
  DBG_ASSERT (page_base, p);
  switch (page_get_type (p)) {
    case PG_VAR_PAGE: {
      return vp_get_bytes_imut (p);
    }
    case PG_VAR_TAIL: {
      return vt_get_bytes_imut (p);
    }
    default: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
}

HEADER_FUNC void
dlgt_set_data (page *p, const in_dl_data d)
{
  DBG_ASSERT (page_base, p);
  switch (page_get_type (p)) {
    case PG_INNER_NODE: {
      in_set_data (p, d.in);
      break;
    }
    case PG_DATA_LIST: {
      dl_set_data (p, d.dl);
      break;
    }
    default: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
}

HEADER_FUNC void
dlgt_move_left (page *dest, page *src, const p_size len)
{
  DBG_ASSERT (page_base, dest);
  DBG_ASSERT (page_base, src);
  ASSERT (page_get_type (dest) == page_get_type (src));

  switch (page_get_type (src)) {
    case PG_INNER_NODE: {
      in_move_left (dest, src, len);
      break;
    }
    case PG_DATA_LIST: {
      dl_move_left (dest, src, len);
      break;
    }
    default: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
}

HEADER_FUNC void
dlgt_move_right (page *src, page *dest, const p_size len)
{
  DBG_ASSERT (page_base, src);
  DBG_ASSERT (page_base, dest);
  ASSERT (page_get_type (dest) == page_get_type (src));

  switch (page_get_type (src)) {
    case PG_INNER_NODE: {
      in_move_right (src, dest, len);
      break;
    }
    case PG_DATA_LIST: {
      dl_move_right (src, dest, len);
      break;
    }
    default: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
}

HEADER_FUNC void
dlgt_link (page *left, page *right)
{
  // Either side can be NULL - the left most page has no left sibling
  pgno _left  = PGNO_NULL;
  pgno _right = PGNO_NULL;

  if (left != NULL) {
    _left = left->pg;
  }

  if (right != NULL) {
    _right = right->pg;
  }

  if (left != NULL) {
    dlgt_set_next (left, _right);
  }

  if (right != NULL) {
    dlgt_set_prev (right, _left);
  }
}

HEADER_FUNC void
dlgtovlink (page *left, const page *right)
{
  DBG_ASSERT (page_base, left);

  pgno _right = PGNO_NULL;

  if (right != NULL) {
    _right = right->pg;
  }

  dlgtset_ovnext (left, _right);
}

HEADER_FUNC bool
dlgt_valid_neighbors (const page *left, const page *right)
{
  DBG_ASSERT (page_base, left);
  pgno lpg = PGNO_NULL;
  pgno rpg = PGNO_NULL;

  if (left != NULL) {
    lpg = left->pg;
  }

  if (right != NULL) {
    rpg = right->pg;
  }

  bool ret = true;

  if (left != NULL) {
    ret = ((ret && dlgt_get_next (left) == rpg) != 0);
  }

  if (right != NULL) {
    ret = ((ret && lpg == dlgt_get_prev (right)) != 0);
  }

  return ret;
}

HEADER_FUNC void
make_valid (page *p)
{
  DBG_ASSERT (page_base, p);
  switch (page_get_type (p)) {
    case PG_DATA_LIST: {
      dl_make_valid (p);
      break;
    }
    case PG_INNER_NODE: {
      in_make_valid (p);
      break;
    }
    default: {
      return;
    }
  }
}

#endif
