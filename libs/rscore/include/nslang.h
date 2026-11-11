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

#ifndef NS_NSLANG_H
#define NS_NSLANG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct multi_user_stride;
struct query;
struct string;
struct subtype;
struct type;
struct type_accessor;
struct type_ref;
struct user_stride;

///////////////////////// Function interfaces

// Returns [size] bytes aligned to [align] or NULL. nslang never frees
struct nslang_alloc
{
  void *ctx;
  void *(*alloc) (void *ctx, size_t size, size_t align);
};

// Returns a random number in [lo, hi]
struct nslang_rng
{
  void *ctx;
  uint32_t (*range) (void *ctx, uint32_t lo, uint32_t hi);
};

// [text] is not null terminated
struct nslang_sink
{
  void *ctx;
  void (*write) (void *ctx, const char *text, size_t len);
};

///////////////////////// Errors

enum nslang_status
{
  NSLANG_OK                   = 0,
  NSLANG_ERR_NOMEM            = -1,
  NSLANG_ERR_SYNTAX           = -2,
  NSLANG_ERR_INVALID_TYPE     = -3,
  NSLANG_ERR_CORRUPT          = -4,
  NSLANG_ERR_ARITH            = -5,
  NSLANG_ERR_INVALID_ARGUMENT = -6,
};

// Every [e] argument below can be NULL
struct nslang_error
{
  int32_t  code;     // enum nslang_status
  uint32_t len;      // Length of msg excluding the null terminator
  char     msg[256]; // Null terminated
};

///////////////////////// Types

// The enum prim_t value of [text] or -1
int32_t nslang_strtoprim (const char *text, uint32_t len);
bool lex_is_keyword (const char *text, uint32_t len);

uint32_t type_byte_size (const struct type *t);
bool type_equal (const struct type *left, const struct type *right);
int32_t type_snprintf (char *str, uint32_t size, const struct type *t);

int32_t nslang_type_validate (const struct type *t, struct nslang_error *e);
char *nslang_type_tostr (
    const struct type         *t,
    const struct nslang_alloc *alloc,
    struct nslang_error       *e
);
struct type *nslang_type_copy (
    const struct type         *src,
    const struct nslang_alloc *alloc,
    struct nslang_error       *e
);
struct type *nslang_type_random (
    const struct nslang_alloc *alloc,
    const struct nslang_rng   *rng,
    uint32_t                   depth,
    uint32_t                   max_size,
    struct nslang_error       *e
);

// Prints one element of [t] from [buf]
void nslang_type_print_data (
    const uint8_t            *buf,
    const struct type        *t,
    uint32_t                  max_elems,
    const struct nslang_sink *sink
);

///////////////////////// Serialization

int32_t nslang_type_serial_size (uint16_t *dest, const struct type *t, struct nslang_error *e);

// Returns the number of bytes written
int32_t nslang_type_serialize (
    uint8_t             *dest,
    uint32_t             cap,
    const struct type   *t,
    struct nslang_error *e
);

// [consumed] is the number of bytes read from [src]
struct type *nslang_type_deserialize (
    const uint8_t             *src,
    uint32_t                   len,
    uint32_t                  *consumed,
    const struct nslang_alloc *alloc,
    struct nslang_error       *e
);

///////////////////////// Strides

bool user_stride_equal (const struct user_stride *left, const struct user_stride *right);
bool subtype_equal (const struct subtype *left, const struct subtype *right);
bool nslang_type_accessor_equal (
    const struct type_accessor *left,
    const struct type_accessor *right
);
bool nslang_type_ref_equal (const struct type_ref *left, const struct type_ref *right);

struct type *nslang_ta_subtype (
    const struct type          *reftype,
    const struct type_accessor *ta,
    const struct nslang_alloc  *alloc,
    struct nslang_error        *e
);
struct type *nslang_tr_construct (
    const struct type         *reftype,
    const struct type_ref     *tr,
    const struct nslang_alloc *alloc,
    struct nslang_error       *e
);

///////////////////////// Queries

bool query_equal (const struct query *left, const struct query *right);

// False if [q] has no variable (exit, help)
bool query_vname (struct string *dest, const struct query *q);

///////////////////////// Compiler

int32_t nslang_compile_type (
    struct type               *dest,
    const char                *text,
    const struct nslang_alloc *alloc,
    struct nslang_error       *e
);

int32_t nslang_compile_subtype (
    struct subtype            *dest,
    const char                *text,
    const struct nslang_alloc *alloc,
    struct nslang_error       *e
);

int32_t nslang_compile_type_ref (
    struct type_ref           *dest,
    const char                *text,
    const struct nslang_alloc *alloc,
    struct nslang_error       *e
);

int32_t nslang_compile_query (
    struct query              *dest,
    const char                *text,
    const struct nslang_alloc *alloc,
    struct nslang_error       *e
);

int32_t nslang_compile_user_stride (
    struct user_stride  *dest,
    const char          *text,
    struct nslang_error *e
);

int32_t nslang_compile_multi_user_stride (
    struct multi_user_stride  *dest,
    const char                *text,
    const struct nslang_alloc *alloc,
    struct nslang_error       *e
);

#endif
