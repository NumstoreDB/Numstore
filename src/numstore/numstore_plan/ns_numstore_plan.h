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

#include "core/ns_arena_alloc.h"
#include "core/ns_error.h"
#include "core/ns_slab_alloc.h"
#include "core/os/ns_memory.h"
#include "nscore/types/ns_query.h"
#include "numstore/numstore.h"

struct ns_plan_manager
{
  struct slab_alloc alloc;
};

struct ns_plan
{
  struct nsdb            *db;
  struct numstore_var    *var;
  struct i_mem            mem;
  struct arena_alloc      alloc;
  struct query            q;
  struct ns_plan_manager *parent;
};

struct ns_plan *ns_plan_vcreate (struct ns_plan_manager *mgr, const char *query, error *e);
void ns_plan_free (struct ns_plan *plan);

// Execute statments
struct numstore_var *ns_plan_get_var (struct ns_plan *st, ns_txn_t *txn, error *e);
sb_size ns_plan_read (struct ns_plan *st, ns_txn_t *txn, void *dest, b_size dlen, error *e);
void *ns_plan_read_malloc (struct ns_plan *st, ns_txn_t *txn, b_size *dlen, error *e);
sb_size ns_plan_write (struct ns_plan *st, ns_txn_t *txn, const void *src, b_size dlen, error *e);
