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

#ifndef TYPES_H
#define TYPES_H

#include "collections.h"    // llnode
#include "compile_config.h" // pgno ...etc
#include "error.h"          // error
#include "platform.h"
#include "serial.h"   // string
#include "stdtypes.h" // u32 ...etc

/******************************************************************************
 * SECTION: Type Data structure
 * ----------------------------------------------------------------------------
 * @brief The primary data structure that represents a numstore type
 ******************************************************************************/

struct type
{
  enum type_t
  {
    T_PRIM   = 0,
    T_STRUCT = 1,
    T_UNION  = 2,
    T_SARRAY = 3,
  } type;

  union {
    enum prim_t
    {
      U8    = 0,
      U16   = 1,
      U32   = 2,
      U64   = 3,
      I8    = 4,
      I16   = 5,
      I32   = 6,
      I64   = 7,
      F16   = 8,
      F32   = 9,
      F64   = 10,
      F128  = 11,
      CF32  = 12,
      CF64  = 13,
      CF128 = 14,
      CF256 = 15,
      CI16  = 16,
      CI32  = 17,
      CI64  = 18,
      CI128 = 19,
      CU16  = 20,
      CU32  = 21,
      CU64  = 22,
      CU128 = 23,
    } p;

    struct struct_t
    {
      u16            len;
      struct string *keys;
      struct type  **types;
    } st;

    struct union_t
    {
      u16            len;
      struct string *keys;
      struct type  **types;
    } un;

    struct sarray_t
    {
      u16          rank;
      u32         *dims;
      struct type *t;
    } sa;
  };
};

err_t type_validate (const struct type *t, error *e);
i32   type_snprintf (char *str, u32 size, struct type *t);
char *type_tostr (struct type *t);
u32   type_byte_size (const struct type *t);

// String generation
u32  type_get_string_size (const struct type *t);
void type_generate_string (char *dest, const struct type *t);

// Serialization
u32          type_get_serial_size (const struct type *t);
void         type_serialize (struct serializer *dest, const struct type *src);
struct type *type_deserialize (
    struct deserializer *src,
    struct chunk_alloc  *alloc,
    error               *e
);

// Random
struct type *type_random (struct chunk_alloc *alloc, u32 depth, error *e);
bool         type_equal (const struct type *left, const struct type *right);
err_t        i_log_type (struct type *t, error *e);
struct type      *
type_movemem (struct type *src, struct chunk_alloc *alloc, error *e);
void type_print_data (
    int                log_level,
    const u8          *buf,
    const struct type *t,
    u32                max_elems
);
err_t type_stream_printer_init (struct stream *s, struct type *t, error *e);
struct type *type_malloc_copy (struct type *t, struct malloc_plan *plan);

////////////////////////////////////////////////////////////
/// KVT List

struct kvt_list
{
  u16            len;
  struct string *keys;
  struct type  **types;
};

struct kv_llnode
{
  struct string key;
  struct type  *value;
  struct llnode link;
};

struct kvt_list_builder
{
  struct llnode *head;

  u16 klen;
  u16 tlen;

  struct chunk_alloc *temp;
  struct chunk_alloc *persistent;
};

void kvlb_create (
    struct kvt_list_builder *dest,
    struct chunk_alloc      *temp,
    struct chunk_alloc      *persistent
);

err_t
kvlb_accept_key (struct kvt_list_builder *ub, struct string key, error *e);

err_t kvlb_accept_type (struct kvt_list_builder *eb, struct type *t, error *e);

err_t kvlb_build (struct kvt_list *dest, struct kvt_list_builder *eb, error *e);

////////////////////////////////////////////////////////////
/// Struct

err_t struct_t_create (
    struct struct_t    *dest,
    struct kvt_list     list,
    struct chunk_alloc *dalloc,
    error              *e
);

err_t struct_t_validate (const struct struct_t *t, error *e);

i32 struct_t_snprintf (char *str, u32 size, const struct struct_t *st);

u32 struct_t_byte_size (const struct struct_t *t);

u32 struct_t_get_serial_size (const struct struct_t *t);

void struct_t_serialize (struct serializer *dest, const struct struct_t *src);

err_t struct_t_deserialize (
    struct struct_t     *dest,
    struct deserializer *src,
    struct chunk_alloc  *a,
    error               *e
);

err_t struct_t_random (
    struct struct_t    *st,
    struct chunk_alloc *alloc,
    u32                 depth,
    error              *e
);

bool struct_t_equal (const struct struct_t *left, const struct struct_t *right);

struct type *struct_t_resolve_key (
    t_size          *offset,
    struct struct_t *t,
    struct string    key,
    error           *e
);

////////////////////////////////////////////////////////////
/// Prim

const char *prim_to_str (enum prim_t p);

err_t prim_t_validate (const enum prim_t *t, error *e);

i32 prim_t_snprintf (char *str, u32 size, const enum prim_t *p);

u32 prim_t_byte_size (const enum prim_t *t);

void prim_t_serialize (struct serializer *dest, const enum prim_t *src);

err_t
prim_t_deserialize (enum prim_t *dest, struct deserializer *src, error *e);

enum prim_t prim_t_random (void);

enum prim_t strtoprim (const char *text, u32 len);

////////////////////////////////////////////////////////////
/// Union

err_t union_t_create (
    struct union_t     *dest,
    struct kvt_list     list,
    struct chunk_alloc *dalloc,
    error              *e
);

err_t union_t_validate (const struct union_t *t, error *e);

i32 union_t_snprintf (char *str, u32 size, const struct union_t *p);

u32 union_t_byte_size (const struct union_t *t);

u32 union_t_get_serial_size (const struct union_t *t);

void union_t_serialize (struct serializer *dest, const struct union_t *src);

err_t union_t_deserialize (
    struct union_t      *dest,
    struct deserializer *src,
    struct chunk_alloc  *a,
    error               *e
);

err_t union_t_random (
    struct union_t     *un,
    struct chunk_alloc *alloc,
    u32                 depth,
    error              *e
);

bool union_t_equal (const struct union_t *left, const struct union_t *right);

struct type *
union_t_resolve_key (struct union_t *t, struct string key, error *e);

////////////////////////////////////////////////////////////
/// SARRAY

err_t sarray_t_validate (const struct sarray_t *t, error *e);

i32 sarray_t_snprintf (char *str, u32 size, const struct sarray_t *p);

u32 sarray_t_byte_size (const struct sarray_t *t);

u32 sarray_t_get_serial_size (const struct sarray_t *t);

void
sarray_t_serialize (struct serializer *persistent, const struct sarray_t *src);

err_t sarray_t_deserialize (
    struct sarray_t     *persistent,
    struct deserializer *src,
    struct chunk_alloc  *a,
    error               *e
);

err_t sarray_t_random (
    struct sarray_t    *sa,
    struct chunk_alloc *temp,
    u32                 depth,
    error              *e
);

bool sarray_t_equal (const struct sarray_t *left, const struct sarray_t *right);

struct dim_llnode
{
  u32           dim;
  struct llnode link;
};

struct sarray_builder
{
  struct llnode *head;
  struct type   *type;

  struct chunk_alloc *temp;
  struct chunk_alloc *persistent;
};

void sab_create (
    struct sarray_builder *dest,
    struct chunk_alloc    *temp,
    struct chunk_alloc    *persistent
);

err_t sab_accept_dim (struct sarray_builder *eb, i32 dim, error *e);

err_t sab_accept_type (struct sarray_builder *eb, struct type *t, error *e);

err_t
sab_build (struct sarray_t *persistent, struct sarray_builder *eb, error *e);

////////////////////////////////////////////////////////////
/// Quick Builders

#define _mk_prim(_p) {.type = T_PRIM, .p = _p}
HEADER_FUNC struct type
mk_prim (enum prim_t p)
{
  return (struct type)_mk_prim (p);
}

HEADER_FUNC struct type
mk_struct (u16 len, struct string *keys, struct type **types)
{
  return (struct type){
      .type = T_STRUCT,
      .st =
          (struct struct_t){
              .len   = len,
              .keys  = keys,
              .types = types,
          },
  };
}

HEADER_FUNC struct type
mk_union (u16 len, struct string *keys, struct type **types)
{
  return (struct type){
      .type = T_UNION,
      .un =
          (struct union_t){
              .len   = len,
              .keys  = keys,
              .types = types,
          },
  };
}

HEADER_FUNC struct type
mk_sarray (u16 rank, u32 *dims, struct type *sub)
{
  return (struct type){
      .type = T_SARRAY,
      .sa =
          (struct sarray_t){
              .rank = rank,
              .dims = dims,
              .t    = sub,
          },
  };
}

static struct type TU8    = _mk_prim (U8);
static struct type TU16   = _mk_prim (U16);
static struct type TU32   = _mk_prim (U32);
static struct type TU64   = _mk_prim (U64);
static struct type TI8    = _mk_prim (I8);
static struct type TI16   = _mk_prim (I16);
static struct type TI32   = _mk_prim (I32);
static struct type TI64   = _mk_prim (I64);
static struct type TF16   = _mk_prim (F16);
static struct type TF32   = _mk_prim (F32);
static struct type TF64   = _mk_prim (F64);
static struct type TF128  = _mk_prim (F128);
static struct type TCF32  = _mk_prim (CF32);
static struct type TCF64  = _mk_prim (CF64);
static struct type TCF128 = _mk_prim (CF128);
static struct type TCF256 = _mk_prim (CF256);
static struct type TCI16  = _mk_prim (CI16);
static struct type TCI32  = _mk_prim (CI32);
static struct type TCI64  = _mk_prim (CI64);
static struct type TCI128 = _mk_prim (CI128);
static struct type TCU16  = _mk_prim (CU16);
static struct type TCU32  = _mk_prim (CU32);
static struct type TCU64  = _mk_prim (CU64);
static struct type TCU128 = _mk_prim (CU128);

/******************************************************************************
 * SECTION: Type Accessor
 * ----------------------------------------------------------------------------
 * @brief How to access an individual type
 ******************************************************************************/

struct type_accessor
{
  enum ta_type type;

  union {
    struct select_ta
    {
      struct string         key;
      struct type_accessor *sub_ta;
    } select;

    struct range_ta
    {
      struct user_stride   *dim_accessors;
      u32                   dlen;
      struct type_accessor *sub_ta;
    } range;
  };
};

bool type_accessor_equal (
    const struct type_accessor left,
    const struct type_accessor right
);

struct type *ta_subtype (
    struct type          *reftype,
    struct type_accessor *ta,
    struct chunk_alloc   *alloc,
    error                *e
);

bool user_stride_equal (
    const struct user_stride *left,
    const struct user_stride *right
);

struct byte_accessor *type_to_byte_accessor (
    struct type_accessor *src,
    struct type          *reftype,
    struct chunk_alloc   *dalloc,
    error                *e
);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Simple Stack Constructors
 *----------------------------------------------------------------------------*/

#define ta_take() ((struct type_accessor){.type = TA_TAKE})

#define ta_select(_key, _sub_ta)   \
  ((struct type_accessor){         \
      .type = TA_SELECT,           \
      .select =                    \
          {                        \
              .key    = (_key),    \
              .sub_ta = (_sub_ta), \
          },                       \
  })

HEADER_FUNC struct type_accessor
ta_range (
    struct user_stride   *dim_accessors,
    u16                   dlen,
    struct type_accessor *sub_ta
)
{
  return (struct type_accessor){
      .type = TA_RANGE,
      .range =
          (struct range_ta){
              .dim_accessors = dim_accessors,
              .dlen          = dlen,
              .sub_ta        = sub_ta,
          },
  };
}

////////////////////////////////////////////////////////////
/// BUILDER

struct rb_llnode
{
  struct user_stride stride;
  struct llnode      link;
};

struct range_builder
{
  struct llnode      *head;
  u32                 len;
  struct chunk_alloc *temp;
  struct chunk_alloc *persistent;
};

void rb_create (
    struct range_builder *dest,
    struct chunk_alloc   *temp,
    struct chunk_alloc   *persistent
);

err_t rb_accept_stride (
    struct range_builder *rb,
    struct user_stride    stride,
    error                *e
);

err_t rb_build (struct range_ta *dest, struct range_builder *rb, error *e);

struct type_accessor_builder
{
  struct type_accessor  ret;
  struct type_accessor *head;
  struct type_accessor *tail;
  struct chunk_alloc   *temp;
  struct chunk_alloc   *persistent;
  struct range_builder  rb;
  bool                  in_range;
};

void tab_create (
    struct type_accessor_builder *dest,
    struct chunk_alloc           *temp,
    struct chunk_alloc           *persistent
);

err_t tab_accept_select (
    struct type_accessor_builder *builder,
    struct string                 key,
    error                        *e
);

err_t tab_accept_stride (
    struct type_accessor_builder *builder,
    struct user_stride            stride,
    error                        *e
);

err_t tab_accept_take (struct type_accessor_builder *builder, error *e);

err_t tab_build (
    struct type_accessor         *dest,
    struct type_accessor_builder *builder,
    error                        *e
);

/******************************************************************************
 * SECTION: Type Reference
 * ----------------------------------------------------------------------------
 * @brief A reference to a specific type
 ******************************************************************************/

struct type_ref
{
  enum type_ref_t
  {
    TR_TAKE,
    TR_STRUCT,
  } type;

  union {
    struct take_tr
    {
      struct string        vname;
      struct type_accessor ta;
    } tk;

    struct struct_tr
    {
      u16              len;
      struct string   *keys;
      struct type_ref *types;
    } st;
  };
};

bool type_ref_equal (struct type_ref left, const struct type_ref right);

struct type *tr_construct (
    struct type        *reftype,
    struct type_ref    *tr,
    struct chunk_alloc *alloc,
    error              *e
);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Simple Stack Constructors
 *----------------------------------------------------------------------------*/

HEADER_FUNC struct type_ref
tr_take (struct string name, struct type_accessor ta)
{
  return (struct type_ref){
      .type = TR_TAKE,
      .tk =
          {
              .vname = name,
              .ta    = ta,
          },
  };
}

HEADER_FUNC struct type_ref
tr_struct (u16 len, struct string *keys, struct type_ref *types)
{
  return (struct type_ref){
      .type = TR_STRUCT,
      .st =
          {
              .len   = (len),
              .keys  = keys,
              .types = types,
          },
  };
}

/******************************************************************************
 * SECTION: Key Value Type Reference List
 * ----------------------------------------------------------------------------
 * @brief A reference of key and values
 ******************************************************************************/

struct kvt_ref_list
{
  u16              len;
  struct string   *keys;
  struct type_ref *types;
};

struct kv_ref_llnode
{
  struct string   key;
  struct type_ref value;
  struct llnode   link;
};

struct kvt_ref_list_builder
{
  struct llnode *head;

  u16 klen;
  u16 tlen;

  struct chunk_alloc *temp;
  struct chunk_alloc *persistent;
};

void kvrlb_create (
    struct kvt_ref_list_builder *dest,
    struct chunk_alloc          *temp,
    struct chunk_alloc          *persistent
);

err_t
kvrlb_accept_key (struct kvt_ref_list_builder *ub, struct string key, error *e);

err_t kvrlb_accept_type (
    struct kvt_ref_list_builder *eb,
    struct type_ref              t,
    error                       *e
);

err_t kvrlb_build (
    struct kvt_ref_list         *dest,
    struct kvt_ref_list_builder *eb,
    error                       *e
);

/******************************************************************************
 * SECTION: Sub Type
 * ----------------------------------------------------------------------------
 * @brief A sub type accesses sub elements of a type
 ******************************************************************************/

struct subtype
{
  struct string        vname;
  struct type_accessor ta;
};

err_t subtype_create (
    struct subtype      *dest,
    struct string        vname,
    struct type_accessor ta,
    error               *e
);

bool subtype_equal (const struct subtype *left, const struct subtype *right);

struct type *subtype_get_type (
    struct type          *stype,
    struct type_accessor *ta,
    struct chunk_alloc   *alloc,
    error                *e
);
#endif // TYPES_H
