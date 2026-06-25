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

#ifndef PAGE_H
#define PAGE_H

#include "collections.h" // cbuffer
#include "csx_assert.h"  // DBG_ASSERT
#include "error.h"       // error
#include "htable.h"
#include "numerics.h" // checksum
#include "numstore.h" // pgno ...etc
#include "page.h"
#include "platform.h" // string.h
#include "platform.h" // HEADER_FUNC
#include "serial.h"   // bytes
#include "stdtypes.h" // u32 ...etc
#include "stdtypes.h" // u8 ...etc

#ifdef TESTING
#  include "testing/testing.h"
#endif

/******************************************************************************
 * SECTION: Page Common
 * ----------------------------------------------------------------------------
 * @brief
 *
 *
 ******************************************************************************/

/*
 * Page Layout
 *
 * Every page in the database file is NS_PAGE_SIZE bytes.  The first bytes of
 * every page form a fixed common header:
 *
 *   [0, 4)              u32 checksum   — CRC of all bytes after the checksum
 *                                        field (bytes [4, NS_PAGE_SIZE)).
 *   [4, 4+sizeof(pgh))  pgh type       — page type tag (enum page_type).
 *   [4+sizeof(pgh), ...) lsn pageLSN   — LSN of the last WAL record that
 *                                        modified this page.
 *
 * Page content begins at PG_COMMN_END.  Each page type interprets the
 * remaining bytes differently.
 *
 * The checksum is written by pgr_flush() just before the page is written to
 * disk.  It is checked by page_validate_for_db() when a page is read into
 * the buffer pool, detecting corruption.
 *
 * PG_TRASH is a sentinel type used as the initial value for read frames of
 * newly allocated pages that have not yet been written.  Any attempt to
 * validate a PG_TRASH frame will fail unless PG_PERMISSIVE is supplied.
 */
enum page_type
{
  // Common page types
  PG_FREE_SPACE_MAP = (1 << 0), // A free space map page

  // Rptree page types
  PG_DATA_LIST  = (1 << 2), // r+tree data node
  PG_INNER_NODE = (1 << 3), // r+tree Inner node

  // Variable page types
  PG_VAR_HASH_PAGE =
      (1 << 4), // A Hash Table for variable names - links to a linked list
  PG_VAR_PAGE = (1 << 5), // A Single link in the hash table linked list
  PG_VAR_TAIL = (1 << 6), // Overflow to a VAR_PAGE
};

#define PG_PERMISSIVE    (1 << 7)
#define PG_SKIP_CHECKSUM (1 << 8)
#define PG_TRASH         ((u8) ~((u8)0))

// COMMON PAGE HEADER
#define PG_CKSM_OFST ((p_size)0)
#define PG_HEDR_OFST ((p_size)(PG_CKSM_OFST + sizeof (u32)))
#define PG_PLSN_OFST ((p_size)(PG_HEDR_OFST + sizeof (pgh)))
#define PG_COMMN_END ((p_size)(PG_PLSN_OFST + sizeof (lsn)))

#define PG_CKSM_DATA_LEN (NS_PAGE_SIZE - PG_CKSM_OFST)

typedef struct
{
  u8   raw[NS_PAGE_SIZE];
  pgno pg;
} page;

DEFINE_DBG_ASSERT (page, page_base, p, { ASSERT (p); })

// Initialization
void page_init_empty (page *p, enum page_type t);

// Validate
err_t page_validate_for_db (const page *p, int page_types, error *e);

////////////////////////////////////////////////////////////
/////// Utility Macros

#define PAGE_SIMPLE_GET_IMPL(v, type, ofst)         \
  do                                                \
  {                                                 \
    ASSERT ((ofst) + sizeof (type) < NS_PAGE_SIZE); \
    type ret;                                       \
    memcpy (&(ret), &(v)->raw[ofst], sizeof (ret)); \
    return ret;                                     \
  }                                                 \
  while (0)

#define PAGE_SIMPLE_SET_IMPL(v, val, ofst)          \
  do                                                \
  {                                                 \
    ASSERT ((ofst) + sizeof (val) < NS_PAGE_SIZE);  \
    memcpy (&(v)->raw[ofst], &(val), sizeof (val)); \
  }                                                 \
  while (0)

////////////////////////////////////////////////////////////
// GETTERS

HEADER_FUNC u32
page_get_checksum (const page *p)
{
  DBG_ASSERT (page_base, p);
  PAGE_SIMPLE_GET_IMPL (p, u32, PG_CKSM_OFST);
}

HEADER_FUNC u32
page_compute_checksum (const page *p)
{
  DBG_ASSERT (page_base, p);
  const void *data     = &p->raw[4];
  u32         checksum = checksum_init ();
  checksum_execute (&checksum, data, NS_PAGE_SIZE - 4);
  return checksum;
}

HEADER_FUNC pgh
page_get_type (const page *p)
{
  PAGE_SIMPLE_GET_IMPL (p, pgh, PG_HEDR_OFST);
}

HEADER_FUNC lsn
page_get_page_lsn (const page *p)
{
  DBG_ASSERT (page_base, p);
  PAGE_SIMPLE_GET_IMPL (p, lsn, PG_PLSN_OFST);
}

////////////////////////////////////////////////////////////
// SETTERS

HEADER_FUNC void
page_set_checksum (page *p, const u32 checksum)
{
  DBG_ASSERT (page_base, p);
  PAGE_SIMPLE_SET_IMPL (p, checksum, PG_CKSM_OFST);
}

HEADER_FUNC void
page_set_type (page *p, const enum page_type t)
{
  DBG_ASSERT (page_base, p);
  const pgh _type = t;
  PAGE_SIMPLE_SET_IMPL (p, _type, PG_HEDR_OFST);
}

HEADER_FUNC void
page_set_page_lsn (page *p, const lsn page_lsn)
{
  DBG_ASSERT (page_base, p);
  PAGE_SIMPLE_SET_IMPL (p, page_lsn, PG_PLSN_OFST);
}

HEADER_FUNC void
page_memcpy (page *dest, const struct bytes src)
{
  DBG_ASSERT (page_base, dest);
  ASSERT (src.len == NS_PAGE_SIZE);
  memcpy (dest->raw, src.head, src.len);
}

// Logging
void i_log_page (int log_level, const page *p);

/******************************************************************************
 * SECTION: Free Space Map
 * ----------------------------------------------------------------------------
 * @brief
 *
 *
 ******************************************************************************/

// OFFSETS and _Static_asserts
#define FS_BTMP_OFST PG_COMMN_END

#define FS_BTMP_SIZE (NS_PAGE_SIZE - FS_BTMP_OFST)
#define FS_BTMP_NPGS ((NS_PAGE_SIZE - FS_BTMP_OFST) * 8)

void fsm_init_empty (page *in);

HEADER_FUNC pgno
pgtofsm (const pgno pg)
{
  const pgno ret = pg / FS_BTMP_NPGS;
  return ret * FS_BTMP_NPGS;
}

HEADER_FUNC p_size
pgtoidx (const pgno pg)
{
  return pg % FS_BTMP_NPGS;
}

////////////////////////////////////////////////////////////
/// GETTERS

HEADER_FUNC const void *
fsm_get_bitmap_imut (const page *p)
{
  return (void *)&p->raw[FS_BTMP_OFST];
}

HEADER_FUNC void *
fsm_get_bitmap_mut (page *p)
{
  return (void *)&p->raw[FS_BTMP_OFST];
}

HEADER_FUNC p_size
fsm_get_bit (const page *p, const p_size idx)
{
  return (((const u8 *)fsm_get_bitmap_imut (p))[idx / 8] >> (idx % 8)) & 1;
}

HEADER_FUNC sp_size
fsm_next_freebit (const page *p, const p_size frombit)
{
  const void  *data     = fsm_get_bitmap_imut (p);
  const p_size frombyte = frombit / 8;

  // TODO - (25) optimize using u64's or u32's
  for (p_size byte_i = frombyte; byte_i < FS_BTMP_SIZE; ++byte_i)
  {
    u8 b = ~((u8 *)data)[byte_i];
    if (b == 0)
    {
      continue;
    }

    if (byte_i == frombyte)
    {
      b &= ~((1 << (frombit % 8)) - 1);
    }

    if (!b)
    {
      continue;
    }

    for (p_size bit = 0; bit < 8; ++bit)
    {
      if ((b >> bit) & 1)
      {
        return byte_i * 8 + bit;
      }
    }
  }

  return -1;
}

////////////////////////////////////////////////////////////
/// SETTERS

HEADER_FUNC void
fsm_set_bit (page *p, const p_size idx)
{
  ASSERT (idx < FS_BTMP_SIZE * 8);
  ((u8 *)fsm_get_bitmap_mut (p))[idx / 8] |= (1 << (idx % 8));
}

HEADER_FUNC void
fsm_clr_bit (page *p, const p_size idx)
{
  ASSERT (idx < NS_PAGE_SIZE * 8);
  ((u8 *)fsm_get_bitmap_mut (p))[idx / 8] &= ~(1 << (idx % 8));
}

// Validation
err_t fsm_validate_for_db (const page *hl, error *e);

// Utils
void i_log_fsm (int level, const page *t);

/******************************************************************************
 * SECTION: Data List
 * ----------------------------------------------------------------------------
 * @brief
 *
 *
 ******************************************************************************/

/*
 * Data List Page (R+Tree leaf / data node)
 *
 * Data-list pages form the leaf level of the R+Tree.  They are doubly-linked
 * across the entire leaf chain so that sequential reads and writes can walk
 * the list without returning to the index.  Each page stores up to
 * DL_DATA_SIZE bytes of raw user data.
 *
 * MEMORY LAYOUT (after PG_COMMN_END):
 *   [DL_NEXT_OFST]  pgno next  — right sibling (PGNO_NULL if last).
 *   [DL_PREV_OFST]  pgno prev  — left sibling (PGNO_NULL if first).
 *   [DL_BLEN_OFST]  p_size len — number of bytes currently stored in this
 *                                page's data region.
 *   [DL_DATA_OFST .. NS_PAGE_SIZE)  raw data bytes.
 *
 * Data is always tightly packed starting at DL_DATA_OFST.  dl_used() is the
 * high-water mark; bytes beyond dl_used() are garbage and must not be read.
 *
 * A lone data-list page with no siblings (next == prev == PGNO_NULL) acts as
 * the root of a single-page R+Tree.  Once data grows beyond DL_DATA_SIZE,
 * the R+Tree insert algorithm allocates additional pages and links them here.
 */

DEFINE_DBG_ASSERT (page, data_list, d, { ASSERT (d); })

// OFFSETS and _Static_asserts
#define DL_NEXT_OFST PG_COMMN_END
#define DL_PREV_OFST ((p_size)(DL_NEXT_OFST + sizeof (pgno)))
#define DL_BLEN_OFST ((p_size)(DL_PREV_OFST + sizeof (pgno)))
#define DL_DATA_OFST ((p_size)(DL_BLEN_OFST + sizeof (p_size)))

_Static_assert (
    NS_PAGE_SIZE > DL_DATA_OFST + 10,
    "Data List: NS_PAGE_SIZE must be > DL_DATA_OFST "
    "plus at least 10 extra bytes of data"
);

#define DL_DATA_SIZE ((p_size)(NS_PAGE_SIZE - DL_DATA_OFST))
#define DL_REM       (DL_DATA_SIZE % 2)

struct dl_data
{
  void *data;
  u32   blen;
};

err_t  dl_validate_for_db (const page *d, error *e);
p_size dl_append (page *d, const u8 *src, p_size bytes);
void   dl_append_from_cbuffer (page *d, struct cbuffer *src, p_size amnt);
p_size dl_write (const page *d, const u8 *src, p_size offset, p_size bytes);
p_size dl_write_from_buffer (
    const page     *d,
    struct cbuffer *src,
    p_size          offset,
    p_size          nbytes
);
p_size dl_memset_from_buffer (page *d, struct cbuffer *src, p_size nbytes);
void dl_memset_from_buffer_expect (page *d, struct cbuffer *src, p_size nbytes);
void dl_memset (page *d, const u8 *buf, p_size len);
void dl_set_data (page *p, struct dl_data d);
void dl_move_left (page *dest, page *src, p_size len);
void dl_move_right (page *src, page *dest, p_size len);
void i_log_dl (int level, const page *d);
p_size dl_read (const page *d, u8 *dest, p_size offset, p_size bytes);
p_size dl_read_into_cbuffer (
    const page     *d,
    struct cbuffer *c,
    p_size          offset,
    p_size          bytes
);
p_size dl_read_out_into_cbuffer (
    page           *d,
    struct cbuffer *dest,
    p_size          offset,
    p_size          bytes
);
void   dl_read_expect (const page *d, u8 *dest, p_size offset, p_size bytes);
p_size dl_read_out_from (page *d, u8 *dest, p_size offset);
void   dl_shift_right (page *d, p_size len);
void   dl_make_valid (page *d);

////////////////////////////////////////////////////////////
// GETTERS

HEADER_FUNC pgno
dl_get_next (const page *d)
{
  PAGE_SIMPLE_GET_IMPL (d, pgno, DL_NEXT_OFST);
}

HEADER_FUNC pgno
dl_get_prev (const page *d)
{
  PAGE_SIMPLE_GET_IMPL (d, pgno, DL_PREV_OFST);
}

HEADER_FUNC void *
dl_get_data (const page *d)
{
  return (void *)&d->raw[DL_DATA_OFST];
}

HEADER_FUNC p_size
dl_used (const page *d)
{
  PAGE_SIMPLE_GET_IMPL (d, p_size, DL_BLEN_OFST);
}

HEADER_FUNC void *
dl_avail_data (const page *d)
{
  return (u8 *)&d->raw[DL_DATA_OFST] + dl_used (d);
}

HEADER_FUNC u8
dl_get_byte (const page *d, const p_size i)
{
  ASSERT (i < dl_used (d));
  return ((u8 *)dl_get_data (d))[i];
}

HEADER_FUNC p_size
dl_avail (const page *d)
{
  return DL_DATA_SIZE - dl_used (d);
}

HEADER_FUNC bool
dl_is_root (const page *p)
{
  DBG_ASSERT (data_list, p);
  return dl_get_next (p) == PGNO_NULL && dl_get_prev (p) == PGNO_NULL;
}

// Shorthands
#define dl_full(dl) (dl_used (dl) == DL_DATA_SIZE)

////////////////////////////////////////////////////////////
// SETTERS

HEADER_FUNC void
dl_set_next (page *d, const pgno next)
{
  PAGE_SIMPLE_SET_IMPL (d, next, DL_NEXT_OFST);
}

HEADER_FUNC void
dl_set_prev (page *d, const pgno prev)
{
  PAGE_SIMPLE_SET_IMPL (d, prev, DL_PREV_OFST);
}

HEADER_FUNC void
dl_set_byte (const page *d, const p_size i, const u8 byte)
{
  ASSERT (i < dl_used (d));
  ((u8 *)dl_get_data (d))[i] = byte;
}

HEADER_FUNC void
dl_set_used (page *d, const p_size used)
{
  ASSERT (used <= DL_DATA_SIZE);
  PAGE_SIMPLE_SET_IMPL (d, used, DL_BLEN_OFST);
}

HEADER_FUNC void
dl_init_empty (page *d)
{
  ASSERT (page_get_type (d) == PG_DATA_LIST);
  dl_set_next (d, PGNO_NULL);
  dl_set_prev (d, PGNO_NULL);
  dl_set_used (d, 0);
}

HEADER_FUNC void
dl_dl_memmove_permissive (
    page        *dest,
    const page  *src,
    const p_size didx,
    const p_size sidx,
    const p_size nbytes
)
{
  ASSERT (dest);
  ASSERT (src);

  ASSERT (sidx < DL_DATA_SIZE);
  ASSERT (sidx + nbytes <= DL_DATA_SIZE);

  if (dest->pg == src->pg)
  {
    ASSERT (sidx >= didx); // Nothing to do on same ptr
    if (didx == sidx)
    {
      return;
    }
  }

  memmove (
      (u8 *)dl_get_data (dest) + didx,
      (u8 *)dl_get_data (src) + sidx,
      nbytes
  );
}
/******************************************************************************
 * SECTION: Inner Node
 * ----------------------------------------------------------------------------
 * @brief
 *
 *
 ******************************************************************************/

struct in_pair
{
  pgno   pg;
  b_size key; // number of bytes covered by this child's subtree
};

#define in_pair_from(_pg, _key) \
  (struct in_pair)              \
  {                             \
    .pg = (_pg), .key = (_key), \
  }

#define in_pair_is_empty(o) (o.pg == PGNO_NULL)

struct three_in_pair
{
  struct in_pair prev;
  struct in_pair cur;
  struct in_pair next;
};

#define in_pair_empty \
  (struct in_pair)    \
  {                   \
    .pg = PGNO_NULL   \
  }

DEFINE_DBG_ASSERT (page, inner_node, i, { ASSERT (i); })

struct in_data
{
  struct in_pair *nodes;
  u32             len;
};

#define IN_NEXT_OFST PG_COMMN_END
#define IN_PREV_OFST ((p_size)(IN_NEXT_OFST + sizeof (pgno)))
#define IN_NLEN_OFST ((p_size)(IN_PREV_OFST + sizeof (pgno)))
#define IN_LEAF_OFST ((p_size)(IN_NLEN_OFST + sizeof (p_size)))

_Static_assert (
    NS_PAGE_SIZE > IN_LEAF_OFST + 5 * sizeof (b_size) + 6 * sizeof (pgno),
    "Inner Node: NS_PAGE_SIZE must be > IN_LEAF_OFST plus at least 5 keys"
);

#define IN_MAX_KEYS \
  (p_size) ((NS_PAGE_SIZE - IN_LEAF_OFST) / (sizeof (pgno) + sizeof (b_size)))
#define IN_MIN_KEYS (IN_MAX_KEYS / 2)

_Static_assert (IN_MAX_KEYS > 5, "Inner Node: IN_MAX_KEYS must be > 5");

void   in_init_empty (page *in);
err_t  in_validate_for_db (const page *in, error *e);
p_size in_page_memcpy_right (pgno *dest, const page *src, p_size ofst);
p_size in_key_memcpy_right (b_size *dest, const page *src, p_size ofst);
void   in_push_left (page *in, p_size len);
void   in_push_right_permissive (page *in, p_size amnt);
void   in_push_left_permissive (page *in, p_size len);
void   in_push_all_left (page *in);
void   in_cut_left (page *in, p_size end);
void   in_data_from_arrays (
    const struct in_data *dest,
    const pgno           *pgs,
    const b_size         *keys
);
void           in_set_data (page *p, struct in_data data);
struct in_data in_get_data (const page *p, struct in_pair nodes[IN_MAX_KEYS]);
void           in_move_left (page *dest, page *src, p_size len);
void           in_move_right (page *src, page *dest, p_size len);
void in_choose_lidx (p_size *idx, b_size *nleft, const page *node, b_size loc);
void i_log_in (int level, const page *in);
void in_make_valid (page *in);

////////////////////////////////////////////////////////////
// GETTERS

HEADER_FUNC p_size
in_get_len (const page *in)
{
  PAGE_SIMPLE_GET_IMPL (in, p_size, IN_NLEN_OFST);
}

HEADER_FUNC const void *
in_get_backwards_keys_imut (const page *in)
{
  const p_size n      = in_get_len (in);
  const p_size nbytes = n * sizeof (b_size);
  ASSERT (nbytes <= NS_PAGE_SIZE);
  if (nbytes > NS_PAGE_SIZE)
  {
    UNREACHABLE_HINT (); // invariant: callers guarantee nbytes fits in page
  }

  if (nbytes == 0)
  {
    return NULL;
  }

  return &in->raw[NS_PAGE_SIZE - nbytes];
}

HEADER_FUNC b_size
in_get_key (const page *in, const p_size idx)
{
  const p_size n = in_get_len (in);
  ASSERT (idx < n);

  const u8    *base         = in_get_backwards_keys_imut (in);
  const p_size offset_elems = n - 1 - idx;
  const p_size offset_bytes = offset_elems * sizeof (b_size);

  b_size ret;
  memcpy (&ret, base + offset_bytes, sizeof ret);

  return ret;
}

HEADER_FUNC b_size
in_get_size (const page *in)
{
  b_size ret = 0;
  // TODO - (17) this could be cached

  for (p_size i = 0; i < in_get_len (in); ++i)
  {
    ret += in_get_key (in, i);
  }

  return ret;
}

HEADER_FUNC p_size
in_get_avail (const page *in)
{
  return IN_MAX_KEYS - in_get_len (in);
}

HEADER_FUNC void *
in_get_backwards_keys_mut (page *in)
{
  const p_size n      = in_get_len (in);
  const p_size nbytes = n * sizeof (b_size);
  ASSERT (nbytes <= NS_PAGE_SIZE);

  return &in->raw[NS_PAGE_SIZE - nbytes];
}

HEADER_FUNC void *
in_get_backwards_keys_mut_limit (page *in)
{
  const p_size nbytes = IN_MAX_KEYS * sizeof (b_size);
  ASSERT (nbytes <= NS_PAGE_SIZE);
  return &in->raw[NS_PAGE_SIZE - nbytes];
}

HEADER_FUNC void *
in_get_leafs_mut (page *in)
{
  return &in->raw[IN_LEAF_OFST];
}

HEADER_FUNC const void *
in_get_leafs_imut (const page *in)
{
  return &in->raw[IN_LEAF_OFST];
}

#define in_get_right_most_key(in) in_get_key (in, in_get_len (in) - 1)

HEADER_FUNC pgno
in_get_leaf (const page *in, const p_size idx)
{
  const p_size n = in_get_len (in);
  ASSERT (idx < n);

  pgno leaf;

  const u8 *head = in_get_leafs_imut (in);
  memcpy (&leaf, head + idx * sizeof (pgno), sizeof (leaf));

  return leaf;
}

HEADER_FUNC pgno
in_get_next (const page *in)
{
  PAGE_SIMPLE_GET_IMPL (in, pgno, IN_NEXT_OFST);
}

HEADER_FUNC pgno
in_get_prev (const page *in)
{
  PAGE_SIMPLE_GET_IMPL (in, pgno, IN_PREV_OFST);
}

HEADER_FUNC bool
in_is_root (const page *in)
{
  DBG_ASSERT (inner_node, in);
  return in_get_next (in) == PGNO_NULL && in_get_prev (in) == PGNO_NULL;
}

// Shorthands
#define in_get_right_most_key(in)  in_get_key (in, in_get_len (in) - 1)
#define in_get_right_most_leaf(in) in_get_leaf (in, in_get_len (in) - 1)
#define in_get_nchildren(in)       (in_get_len (in) + 1)
#define in_get_first_leaf(in)      in_get_leaf (in, 0)
#define in_get_last_leaf(in)       in_get_leaf (in, in_get_len (in) - 1)
#define in_full(in)                (in_get_len (in) == IN_MAX_KEYS)

////////////////////////////////////////////////////////////
// /
////////////////////////////////////////////////////////////
// SETTERS

HEADER_FUNC void
in_set_len (page *in, const p_size len)
{
  PAGE_SIMPLE_SET_IMPL (in, len, IN_NLEN_OFST);
}

HEADER_FUNC void
in_set_leaf (page *in, const p_size idx, const pgno pg)
{
  ASSERT (idx < in_get_len (in) + 1);
  PAGE_SIMPLE_SET_IMPL (in, pg, IN_LEAF_OFST + idx * sizeof (pgno));
}

HEADER_FUNC void
in_set_key (page *in, const p_size idx, const b_size key)
{
  const p_size n = in_get_len (in);
  ASSERT (idx < n);

  u8          *base         = in_get_backwards_keys_mut (in);
  const p_size offset_elems = n - 1 - idx;
  const p_size offset_bytes = offset_elems * sizeof (b_size);

  memcpy (base + offset_bytes, &key, sizeof key);
}

HEADER_FUNC void
in_set_key_leaf (page *in, const p_size idx, const b_size key, const pgno pg)
{
  in_set_key (in, idx, key);
  in_set_leaf (in, idx, pg);
}

HEADER_FUNC void
in_push_end (page *dest, const b_size key, const pgno pg)
{
  ASSERT (!in_full (dest));
  in_set_len (dest, in_get_len (dest) + 1);
  in_set_key_leaf (dest, in_get_len (dest) - 1, key, pg);
}

HEADER_FUNC void
in_set_leaf_ignore_len (page *in, const p_size idx, const pgno pg)
{
  ASSERT (idx < IN_MAX_KEYS);
  PAGE_SIMPLE_SET_IMPL (in, pg, IN_LEAF_OFST + idx * sizeof (pgno));
}

HEADER_FUNC void
in_set_key_ignore_len (page *in, const p_size idx, const b_size key)
{
  ASSERT (idx < IN_MAX_KEYS);

  u8          *base         = in_get_backwards_keys_mut (in);
  const p_size offset_elems = IN_MAX_KEYS - 1 - idx;
  const p_size offset_bytes = offset_elems * sizeof (b_size);

  memcpy (base + offset_bytes, &key, sizeof key);
}

HEADER_FUNC void
in_set_key_leaf_ignore_len (
    page        *in,
    const p_size idx,
    const b_size key,
    const pgno   pg
)
{
  in_set_key_ignore_len (in, idx, key);
  in_set_leaf_ignore_len (in, idx, pg);
}

HEADER_FUNC void
in_set_prev (page *in, const pgno prev)
{
  PAGE_SIMPLE_SET_IMPL (in, prev, IN_PREV_OFST);
}

HEADER_FUNC void
in_set_next (page *in, const pgno next)
{
  PAGE_SIMPLE_SET_IMPL (in, next, IN_NEXT_OFST);
}

HEADER_FUNC void
in_link (page *left, page *right)
{
  const pgno lpg = left ? left->pg : PGNO_NULL;
  const pgno rpg = right ? right->pg : PGNO_NULL;
  if (left)
  {
    in_set_next (left, rpg);
  }
  if (right)
  {
    in_set_prev (right, lpg);
  }
}
/******************************************************************************
 * SECTION: var_hash_page
 * ----------------------------------------------------------------------------
 * @brief
 *
 *
 ******************************************************************************/

/*
 * PG_VAR_HASH_PAGE — root of the variable name hash table.
 *
 * There is exactly one of these pages in every database, at VHASH_PGNO.
 * It contains an array of VH_HASH_LEN pgno slots, each the head of a
 * singly-linked chain of PG_VAR_PAGE nodes.  A variable name is looked up
 * by hashing the name (FNV-1a) modulo VH_HASH_LEN to find the bucket, then
 * walking the PG_VAR_PAGE chain until the name matches or PGNO_NULL is
 * reached.
 *
 * Because all bucket pointers live in a single page, creating or deleting a
 * variable always requires a write to this page.  Collisions are resolved by
 * chaining via vp_get_next/vp_set_next on the PG_VAR_PAGE nodes.
 */

////////////////////////////////////////////////////////////
/////// VAR HASH PAGE

// ============ PAGE START
// HEADER
// HASH0     [pgno]
// HASH1     [pgno]
// ...
// HASHn     [pgno]
// 0         Maybe extra space
// ============ PAGE END

// OFFSETS and _Static_asserts
#define VH_HASH_OFST PG_COMMN_END
#define VH_HASH_LEN  ((NS_PAGE_SIZE - VH_HASH_OFST) / sizeof (pgno))

_Static_assert (
    NS_PAGE_SIZE > VH_HASH_OFST + 10 * sizeof (pgno),
    "Root Page: NS_PAGE_SIZE must be > RN_HASH_OFST plus at least 10 "
    "extra hashes"
);

// Initialization
void vh_init_empty (page *p);

// Getters
HEADER_FUNC p_size
vh_get_hash_pos (const struct string vname)
{
  return (p_size)fnv1a_hash (vname) % (VH_HASH_LEN);
}

HEADER_FUNC pgno
vh_get_hash_value (const page *p, const p_size pos)
{
  ASSERT (pos < VH_HASH_LEN);
  PAGE_SIMPLE_GET_IMPL (p, pgno, VH_HASH_OFST + pos * sizeof (pgno));
}

// Setters
HEADER_FUNC void
vh_set_hash_value (page *p, const p_size pos, const pgno value)
{
  ASSERT (pos < VH_HASH_LEN);
  PAGE_SIMPLE_SET_IMPL (p, value, VH_HASH_OFST + pos * sizeof (pgno));
}

// Validation
err_t vh_validate_for_db (const page *p, error *e);

// Utils
void i_log_vh (int level, const page *vh);

/******************************************************************************
 * SECTION: Var Page
 * ----------------------------------------------------------------------------
 * @brief
 *
 *
 ******************************************************************************/

////////////////////////////////////////////////////////////
/////// VAR PAGE

// ============ PAGE START
// HEADER
// NEXT     [pgno]      - Next var page in the hash table chain
// OVNEXT   [pgno]      - Next Overflow page for name / serialized type
// VLEN     [2 bytes]   - Length of the name string in bytes
// TLEN     [2 bytes]   - Length of the type in bytes
// ROOT     [pgno]      - Root page of the rptree chain
// NBYTES   [b_size]    - Root page of the rptree chain
// NBYTES   [b_size]    - Root page of the rptree chain
// VNAME
// VNAME
// VNAME
// ...
// TSTR
// TSTR
// TSTR
// ...
// ============ PAGE END

// OFFSETS and _Static_asserts
#define VP_NEXT_OFST PG_COMMN_END
#define VP_OVNX_OFST ((p_size)(VP_NEXT_OFST + sizeof (pgno)))
#define VP_VLEN_OFST ((p_size)(VP_OVNX_OFST + sizeof (pgno)))
#define VP_TLEN_OFST ((p_size)(VP_VLEN_OFST + sizeof (u16)))
#define VP_ROOT_OFST ((p_size)(VP_TLEN_OFST + sizeof (u16)))
#define VP_NBYT_OFST ((p_size)(VP_ROOT_OFST + sizeof (pgno)))
#define VP_VNME_OFST ((p_size)(VP_NBYT_OFST + sizeof (b_size)))
#define VP_MAX_LEN   (NS_PAGE_SIZE - VP_VNME_OFST)

// Initialization
void vp_init_empty (page *p);

// Setters
void vp_set_next (page *p, pgno pg);
void vp_set_ovnext (page *p, pgno pg);
void vp_set_vlen (page *p, u16 vlen);
void vp_set_tlen (page *p, u16 tlen);
void vp_set_root (page *p, pgno root);
void vp_set_nbytes (page *p, b_size nbytes);

// Getters
pgno   vp_get_next (const page *p);
pgno   vp_get_ovnext (const page *p);
u16    vp_get_vlen (const page *p);
u16    vp_get_tlen (const page *p);
pgno   vp_get_root (const page *p);
b_size vp_get_nbytes (const page *p);

b_size        vp_calc_tofst (const page *p);
bool          vp_is_overflow (const page *p);
struct bytes  vp_get_bytes (page *p);
struct cbytes vp_get_bytes_imut (const page *p);

// Validation
err_t vp_validate_for_db (const page *p, error *e);

// Utils
void i_log_vp (int level, const page *vp);

/******************************************************************************
 * SECTION: var_tail
 * ----------------------------------------------------------------------------
 * @brief
 *
 *
 ******************************************************************************/

////////////////////////////////////////////////////////////
/////// VAR PAGE TAIL

// OFFSETS and _Static_asserts
#define VT_NEXT_OFST PG_COMMN_END
#define VT_DATA_OFST ((p_size)(VT_NEXT_OFST + sizeof (pgno)))
#define VT_DATA_LEN  (NS_PAGE_SIZE - VT_DATA_OFST)

// Setters
HEADER_FUNC void
vt_set_next (page *p, const pgno pg)
{
  PAGE_SIMPLE_SET_IMPL (p, pg, VT_NEXT_OFST);
}

// Getters
HEADER_FUNC pgno
vt_get_next (const page *p)
{
  PAGE_SIMPLE_GET_IMPL (p, pgno, VT_NEXT_OFST);
}

HEADER_FUNC void
vt_init_empty (page *p)
{
  ASSERT (page_get_type (p) == PG_VAR_TAIL);
  vt_set_next (p, PGNO_NULL);
}

HEADER_FUNC struct bytes
vt_get_bytes (page *p)
{
  return (struct bytes){
      .head = (void *)&p->raw[VT_DATA_OFST],
      .len  = NS_PAGE_SIZE - VT_DATA_OFST,
  };
}

HEADER_FUNC struct cbytes
vt_get_bytes_imut (const page *p)
{
  return (struct cbytes){
      .head = (void *)&p->raw[VT_DATA_OFST],
      .len  = NS_PAGE_SIZE - VT_DATA_OFST,
  };
}

// Validation
err_t vt_validate_for_db (const page *p, error *e);

// Utils
void i_log_vt (int level, const page *vp);

/******************************************************************************
 * SECTION: Page Delegate
 * ----------------------------------------------------------------------------
 * @brief
 *
 *
 ******************************************************************************/

typedef union {
  struct dl_data dl;
  struct in_data in;
} in_dl_data;

#define dlgt_move_all_right(src, dest) \
  dlgt_move_right (src, dest, dlgt_get_len (src))
#define dlgt_move_all_left(dest, src) \
  dlgt_move_left (dest, src, dlgt_get_len (src))

HEADER_FUNC pgno
dlgt_get_next (const page *p)
{
  switch (page_get_type (p))
  {
    case PG_INNER_NODE:
    {
      return in_get_next (p);
    }
    case PG_DATA_LIST:
    {
      return dl_get_next (p);
    }
    case PG_VAR_PAGE:
    {
      return vp_get_next (p);
    }
    case PG_VAR_TAIL:
    {
      return vt_get_next (p);
    }
    default:
    {
      UNREACHABLE ();
    }
  }
}

HEADER_FUNC pgno
dlgt_get_ovnext (const page *p)
{
  switch (page_get_type (p))
  {
    case PG_VAR_PAGE:
    {
      return vp_get_ovnext (p);
    }
    case PG_VAR_TAIL:
    {
      return vt_get_next (p);
    }
    default:
    {
      UNREACHABLE ();
    }
  }
}

HEADER_FUNC void
dlgt_set_next (page *p, const pgno n)
{
  switch (page_get_type (p))
  {
    case PG_INNER_NODE:
    {
      in_set_next (p, n);
      break;
    }
    case PG_DATA_LIST:
    {
      dl_set_next (p, n);
      break;
    }
    case PG_VAR_PAGE:
    {
      vp_set_next (p, n);
      break;
    }
    case PG_VAR_TAIL:
    {
      vt_set_next (p, n);
      break;
    }
    default:
    {
      UNREACHABLE ();
    }
  }
}

HEADER_FUNC pgno
dlgt_get_prev (const page *p)
{
  switch (page_get_type (p))
  {
    case PG_INNER_NODE:
    {
      return in_get_prev (p);
    }
    case PG_DATA_LIST:
    {
      return dl_get_prev (p);
    }
    default:
    {
      UNREACHABLE ();
    }
  }
}

HEADER_FUNC void
dlgt_set_prev (page *p, const pgno prev)
{
  switch (page_get_type (p))
  {
    case PG_INNER_NODE:
    {
      in_set_prev (p, prev);
      break;
    }
    case PG_DATA_LIST:
    {
      dl_set_prev (p, prev);
      break;
    }
    default:
    {
      UNREACHABLE ();
    }
  }
}

HEADER_FUNC void
dlgtset_ovnext (page *p, const pgno next)
{
  switch (page_get_type (p))
  {
    case PG_VAR_PAGE:
    {
      vp_set_ovnext (p, next);
      break;
    }
    case PG_VAR_TAIL:
    {
      vt_set_next (p, next);
      break;
    }
    default:
    {
      UNREACHABLE ();
    }
  }
}

HEADER_FUNC p_size
dlgt_get_len (const page *p)
{
  switch (page_get_type (p))
  {
    case PG_INNER_NODE:
    {
      return in_get_len (p);
    }
    case PG_DATA_LIST:
    {
      return dl_used (p);
    }
    default:
    {
      UNREACHABLE ();
    }
  }
}

HEADER_FUNC b_size
dlgt_get_size (const page *p)
{
  switch (page_get_type (p))
  {
    case PG_INNER_NODE:
    {
      return in_get_size (p);
    }
    case PG_DATA_LIST:
    {
      return dl_used (p);
    }
    default:
    {
      UNREACHABLE ();
    }
  }
}

HEADER_FUNC p_size
dlgt_get_max_len (const page *p)
{
  switch (page_get_type (p))
  {
    case PG_INNER_NODE:
    {
      return IN_MAX_KEYS;
    }
    case PG_DATA_LIST:
    {
      return DL_DATA_SIZE;
    }
    default:
    {
      UNREACHABLE ();
    }
  }
}

HEADER_FUNC bool
dlgt_is_root (const page *p)
{
  return dlgt_get_prev (p) == PGNO_NULL && dlgt_get_next (p) == PGNO_NULL;
}

HEADER_FUNC bool
dlgt_is_full (const page *p)
{
  switch (page_get_type (p))
  {
    case PG_INNER_NODE:
    {
      return in_full (p);
    }
    case PG_DATA_LIST:
    {
      return dl_full (p);
    }
    default:
    {
      UNREACHABLE ();
    }
  }
}

HEADER_FUNC struct bytes
dlgt_get_bytes (page *p)
{
  switch (page_get_type (p))
  {
    case PG_VAR_PAGE:
    {
      return vp_get_bytes (p);
    }
    case PG_VAR_TAIL:
    {
      return vt_get_bytes (p);
    }
    default:
    {
      UNREACHABLE ();
    }
  }
}

HEADER_FUNC struct cbytes
dlgt_get_bytes_imut (const page *p)
{
  switch (page_get_type (p))
  {
    case PG_VAR_PAGE:
    {
      return vp_get_bytes_imut (p);
    }
    case PG_VAR_TAIL:
    {
      return vt_get_bytes_imut (p);
    }
    default:
    {
      UNREACHABLE ();
    }
  }
}

HEADER_FUNC void
dlgt_set_data (page *p, const in_dl_data d)
{
  switch (page_get_type (p))
  {
    case PG_INNER_NODE:
    {
      in_set_data (p, d.in);
      break;
    }
    case PG_DATA_LIST:
    {
      dl_set_data (p, d.dl);
      break;
    }
    default:
    {
      UNREACHABLE ();
    }
  }
}

HEADER_FUNC void
dlgt_move_left (page *dest, page *src, const p_size len)
{
  ASSERT (page_get_type (dest) == page_get_type (src));

  switch (page_get_type (src))
  {
    case PG_INNER_NODE:
    {
      in_move_left (dest, src, len);
      break;
    }
    case PG_DATA_LIST:
    {
      dl_move_left (dest, src, len);
      break;
    }
    default:
    {
      UNREACHABLE ();
    }
  }
}

HEADER_FUNC void
dlgt_move_right (page *src, page *dest, const p_size len)
{
  ASSERT (page_get_type (dest) == page_get_type (src));

  switch (page_get_type (src))
  {
    case PG_INNER_NODE:
    {
      in_move_right (src, dest, len);
      break;
    }
    case PG_DATA_LIST:
    {
      dl_move_right (src, dest, len);
      break;
    }
    default:
    {
      UNREACHABLE ();
    }
  }
}

HEADER_FUNC void
dlgt_link (page *left, page *right)
{
  pgno _left  = PGNO_NULL;
  pgno _right = PGNO_NULL;

  if (left)
  {
    _left = left->pg;
  }

  if (right)
  {
    _right = right->pg;
  }

  if (left)
  {
    dlgt_set_next (left, _right);
  }

  if (right)
  {
    dlgt_set_prev (right, _left);
  }
}

HEADER_FUNC void
dlgtovlink (page *left, const page *right)
{
  ASSERT (left);
  pgno _right = PGNO_NULL;

  if (right)
  {
    _right = right->pg;
  }

  dlgtset_ovnext (left, _right);
}

HEADER_FUNC bool
dlgt_valid_neighbors (const page *left, const page *right)
{
  pgno lpg = PGNO_NULL;
  pgno rpg = PGNO_NULL;

  if (left)
  {
    lpg = left->pg;
  }

  if (right)
  {
    rpg = right->pg;
  }

  bool ret = true;

  if (left)
  {
    ret = ret && dlgt_get_next (left) == rpg;
  }

  if (right)
  {
    ret = ret && lpg == dlgt_get_prev (right);
  }

  return ret;
}

HEADER_FUNC void
make_valid (page *p)
{
  switch (page_get_type (p))
  {
    case PG_DATA_LIST:
    {
      dl_make_valid (p);
      break;
    }
    case PG_INNER_NODE:
    {
      in_make_valid (p);
      break;
    }
    default:
    {
      return;
    }
  }
}

#endif // PAGE_H
