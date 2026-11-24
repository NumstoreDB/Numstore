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

#ifndef HTABLE_H
#define HTABLE_H

#include "ns_error.h"    // error
#include "ns_latch.h"    // latch
#include "ns_platform.h" // HEADER_FUNC
#include "ns_stdtypes.h" // u32 ...etc
#include "ns_string.h"
#include "os/ns_malloc.h"

#include <stdbool.h>
#include <stddef.h>

struct string;

typedef enum
{
  HTIR_SUCCESS,
  HTIR_EXISTS,
  HTIR_FULL,
} hti_res;

typedef enum
{
  HTAR_SUCCESS,
  HTAR_DOESNT_EXIST,
} hta_res;

u32 fnv1a_hash (struct string s);

struct hnode
{
  struct hnode *next;
  u32           hcode;
};

HEADER_FUNC void
hnode_init (struct hnode *dest, const u32 hcode)
{
  dest->hcode = hcode;
  dest->next  = NULL;
}

struct htable
{
  struct i_mem  mem;
  u32           cap;
  u32           size;
  latch         latch;
  struct hnode *table[];
};

struct htable *htable_create (u32 n, struct i_mem mem, error *e);
void htable_free (struct htable *t);

typedef bool (*hnode_eq) (const struct hnode *, const struct hnode *);
typedef void (*hnode_update) (struct hnode *, void *ctx);
typedef struct hnode *(*hnode_new) (void *ctx, error *e);

void htable_insert (struct htable *t, struct hnode *node);
err_t htable_upsert (
    struct htable *t,
    struct hnode  *key,
    void          *ctx,
    hnode_eq       eq,
    hnode_update   update,
    hnode_new      newnode,
    error         *e
);

struct hnode **htable_lookup (struct htable *t, const struct hnode *key, hnode_eq eq);
struct hnode *htable_delete (struct htable *t, struct hnode **from);
struct hnode **htable_random (struct htable *t);

HEADER_FUNC u32
htable_size (const struct htable *t)
{
  ASSERT (t);
  return t->size;
}

void htable_foreach (
    const struct htable *t,
    void (*action) (struct hnode *v, void *ctx),
    void *ctx
);

#endif // HTABLE_H
