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

#include "core/ns_arena_alloc.h"
#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_logging.h"
#include "core/ns_numerics.h"
#include "core/ns_platform.h"
#include "core/ns_serial.h"
#include "core/ns_stdtypes.h"
#include "core/ns_stream.h"
#include "core/ns_string.h"
#include "core/os/ns_malloc.h"

#include <nslang.h>
#include <stdbool.h>
#include <stdio.h>

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
      uint16_t       len;
      struct string *keys;
      struct type  **types;
    } st;

    struct union_t
    {
      uint16_t       len;
      struct string *keys;
      struct type  **types;
    } un;

    struct sarray_t
    {
      uint16_t     rank;
      uint32_t    *dims;
      struct type *t;
    } sa;
  };
};

/******************************************************************************
 * SECTION: nslang
 * ----------------------------------------------------------------------------
 * @brief Lends an arena and an error to the nslang rust crate
 ******************************************************************************/

struct nslang_arena
{
  struct arena_alloc *alloc;
  error              *e;
};

// The arena doesn't align, so over allocate and round up to [align]
HEADER_FUNC void *
nslang_arena_malloc (void *ctx, size_t size, size_t align)
{
  struct nslang_arena *a = ctx;

  ASSERT (a);
  ASSERT (align > 0);

  if (size > UINT32_MAX - align) {
    error_causef (a->e, ERR_NOMEM, "alloc of %zu bytes is too large", size);
    return NULL;
  }

  u8 *raw = arena_malloc (a->alloc, (u32)(size + align - 1), 1, a->e);
  if (raw == NULL) {
    return NULL;
  }

  return raw + ((align - ((uintptr_t)raw % align)) % align);
}

#define NSLANG_ARENA(name, _alloc, _e) \
  struct nslang_arena name##_ctx = {   \
      .alloc = (_alloc),               \
      .e     = (_e),                   \
  };                                   \
  struct nslang_alloc name = {         \
      .ctx   = &name##_ctx,            \
      .alloc = nslang_arena_malloc,    \
  }

// Converts a nslang error to a numstore error
HEADER_FUNC err_t
nslang_raise (error *e, const struct nslang_error *ne)
{
  // The arena already reported this one
  if (e != NULL && e->cause_code < 0) {
    return error_trace (e);
  }

  err_t code = ERR_INVALID_ARGUMENT;
  switch ((enum nslang_status)ne->code) {
    case NSLANG_ERR_NOMEM: {
      code = ERR_NOMEM;
      break;
    }
    case NSLANG_ERR_SYNTAX: {
      code = ERR_SYNTAX;
      break;
    }
    case NSLANG_ERR_INVALID_TYPE: {
      code = ERR_INTERP;
      break;
    }
    case NSLANG_ERR_CORRUPT: {
      code = ERR_CORRUPT;
      break;
    }
    case NSLANG_ERR_ARITH: {
      code = ERR_ARITH;
      break;
    }
    case NSLANG_OK:
    case NSLANG_ERR_INVALID_ARGUMENT: {
      break;
    }
  }

  return error_causef (e, code, "%.*s", (int)ne->len, ne->msg);
}

/******************************************************************************
 * SECTION: Core api
 ******************************************************************************/

HEADER_FUNC err_t
type_validate (const struct type *t, error *e)
{
  struct nslang_error ne;
  if (nslang_type_validate (t, &ne) < 0) {
    return nslang_raise (e, &ne);
  }
  return SUCCESS;
}

DEFINE_DBG_ASSERT (struct type, unchecked_type, t, { ASSERT (t); })

DEFINE_DBG_ASSERT (struct type, valid_type, t, {
  ASSERT (t);
  error e = error_create ();
  ASSERT (type_validate (t, &e) == SUCCESS);
})

HEADER_FUNC char *
type_tostr (struct arena_alloc *alloc, struct type *t, error *e)
{
  DBG_ASSERT (valid_type, t);

  struct nslang_error ne;
  NSLANG_ARENA (a, alloc, e);

  char *ret = nslang_type_tostr (t, &a, &ne);
  if (ret == NULL) {
    nslang_raise (e, &ne);
  }
  return ret;
}

// Caller frees the result
HEADER_FUNC char *
get_var_str (struct type *t, u32 *dlen, error *e)
{
  i32 len = type_snprintf (NULL, 0, t);
  if (len < 0) {
    error_causef (e, ERR_INTERP, "type is not printable");
    return NULL;
  }

  char *dest = default_mem ().table->malloc (default_mem ().self, (u32)len + 1, sizeof *dest, e);
  if (dest == NULL) {
    return NULL;
  }

  type_snprintf (dest, (u32)len + 1, t);
  *dlen = (u32)len;

  return dest;
}

HEADER_FUNC enum prim_t
strtoprim (const char *text, u32 len)
{
  return (enum prim_t)nslang_strtoprim (text, len);
}

HEADER_FUNC struct type *
type_movemem (struct type *src, struct arena_alloc *alloc, error *e)
{
  struct nslang_error ne;
  NSLANG_ARENA (a, alloc, e);

  struct type *ret = nslang_type_copy (src, &a, &ne);
  if (ret == NULL) {
    nslang_raise (e, &ne);
  }
  return ret;
}

HEADER_FUNC err_t
type_get_serial_size (u16 *dest, const struct type *t, error *e)
{
  struct nslang_error ne;
  if (nslang_type_serial_size (dest, t, &ne) < 0) {
    return nslang_raise (e, &ne);
  }
  return SUCCESS;
}

// [dest] needs type_get_serial_size bytes of room
HEADER_FUNC void
type_serialize (struct serializer *dest, const struct type *src)
{
  DBG_ASSERT (valid_type, src);

  latch_lock (&dest->latch);

  i32 written = nslang_type_serialize (dest->data + dest->dlen, dest->dcap - dest->dlen, src, NULL);
  ASSERT (written > 0);
  if (written > 0) {
    dest->dlen += (u32)written;
  }

  latch_unlock (&dest->latch);
}

HEADER_FUNC struct type *
type_deserialize (struct deserializer *src, struct arena_alloc *alloc, error *e)
{
  struct nslang_error ne;
  NSLANG_ARENA (a, alloc, e);

  latch_lock (&src->latch);

  u32 used = 0;
  struct type
      *ret = nslang_type_deserialize (src->data + src->head, src->dlen - src->head, &used, &a, &ne);
  if (ret != NULL) {
    src->head += used;
  }

  latch_unlock (&src->latch);

  if (ret == NULL) {
    nslang_raise (e, &ne);
  }
  return ret;
}

HEADER_FUNC u32
nslang_randu32r (void *ctx, u32 lo, u32 hi)
{
  (void)ctx;
  return randu32r (lo, hi);
}

HEADER_FUNC struct type *
type_random (struct arena_alloc *alloc, u32 depth, t_size max_size, error *e)
{
  struct nslang_error ne;
  struct nslang_rng   rng = {.ctx = NULL, .range = nslang_randu32r};
  NSLANG_ARENA (a, alloc, e);

  struct type *ret = nslang_type_random (&a, &rng, depth, max_size, &ne);
  if (ret == NULL) {
    nslang_raise (e, &ne);
  }
  return ret;
}

HEADER_FUNC void
nslang_log_write (void *ctx, const char *text, size_t len)
{
  const int *log_level = ctx;
  (void)log_level;
  (void)text;
  (void)len;
  i_log_printf (*log_level, "%.*s", (int)len, text);
}

HEADER_FUNC void
type_print_data (int log_level, const u8 *buf, const struct type *t, u32 max_elems)
{
  struct nslang_sink sink = {.ctx = &log_level, .write = nslang_log_write};
  nslang_type_print_data (buf, t, max_elems, &sink);
}

/*-----------------------------------------------------------------------------
 * SUBSECTION: Simple Stack Constructors
 *----------------------------------------------------------------------------*/

#define _mk_prim(_p) {.type = T_PRIM, .p = (_p)}

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
      .st   = (struct struct_t){
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
      .un   = (struct union_t){
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
      .sa   = (struct sarray_t){
          .rank = rank,
          .dims = dims,
          .t    = sub,
      },
  };
}

#ifndef NDEBUG
HEADER_FUNC void
type_test_arena_create (struct arena_alloc *dest, struct i_mem mem)
{
  arena_alloc_create (
      dest,
      (struct arena_alloc_settings){
          .max_alloc_size    = 0,
          .max_total_size    = 0,
          .target_chunk_mult = 1,
          .min_chunk_size    = 10,
          .max_chunk_size    = 0,
          .max_chunks        = 0,
          .mem               = mem,
      }
  );
}
#endif

#ifndef NDEBUG
static struct type MAYBE_UNUSED TU8    = _mk_prim (U8);
static struct type MAYBE_UNUSED TU16   = _mk_prim (U16);
static struct type MAYBE_UNUSED TU32   = _mk_prim (U32);
static struct type MAYBE_UNUSED TU64   = _mk_prim (U64);
static struct type MAYBE_UNUSED TI8    = _mk_prim (I8);
static struct type MAYBE_UNUSED TI16   = _mk_prim (I16);
static struct type MAYBE_UNUSED TI32   = _mk_prim (I32);
static struct type MAYBE_UNUSED TI64   = _mk_prim (I64);
static struct type MAYBE_UNUSED TF16   = _mk_prim (F16);
static struct type MAYBE_UNUSED TF32   = _mk_prim (F32);
static struct type MAYBE_UNUSED TF64   = _mk_prim (F64);
static struct type MAYBE_UNUSED TF128  = _mk_prim (F128);
static struct type MAYBE_UNUSED TCF32  = _mk_prim (CF32);
static struct type MAYBE_UNUSED TCF64  = _mk_prim (CF64);
static struct type MAYBE_UNUSED TCF128 = _mk_prim (CF128);
static struct type MAYBE_UNUSED TCF256 = _mk_prim (CF256);
static struct type MAYBE_UNUSED TCI16  = _mk_prim (CI16);
static struct type MAYBE_UNUSED TCI32  = _mk_prim (CI32);
static struct type MAYBE_UNUSED TCI64  = _mk_prim (CI64);
static struct type MAYBE_UNUSED TCI128 = _mk_prim (CI128);
static struct type MAYBE_UNUSED TCU16  = _mk_prim (CU16);
static struct type MAYBE_UNUSED TCU32  = _mk_prim (CU32);
static struct type MAYBE_UNUSED TCU64  = _mk_prim (CU64);
static struct type MAYBE_UNUSED TCU128 = _mk_prim (CU128);
#endif

#endif // TYPES_H
