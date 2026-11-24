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

#ifndef DIRTY_PAGE_TABLE_H
#define DIRTY_PAGE_TABLE_H

#include "ns_error.h"
#include "ns_htable.h"
#include "ns_slab_alloc.h"
#include "ns_stdtypes.h"
#include "os/ns_malloc.h"

struct dpg_table
{
  struct i_mem      mem;
  struct htable    *t;
  struct slab_alloc alloc;
};

DEFINE_DBG_ASSERT (struct dpg_table, dirty_pg_table, d, {
  ASSERT (d);
  ASSERT (d->t);
})

///////////////////////
///// Lifecycle
struct dpg_table *dpgt_open (struct i_mem mem, error *e);
void dpgt_close (struct dpg_table *t);
void dpgt_crash (struct dpg_table *t);

///////////////////////
///// Utilities
err_t dpgt_merge_into (struct dpg_table *dest, struct dpg_table *src, error *e);
bool dpgt_equal (struct dpg_table *left, struct dpg_table *right);
typedef void (*dpgt_action) (pgno pg, lsn rec_lsn, void *ctx);
void dpgt_foreach (const struct dpg_table *t, dpgt_action action, void *ctx);

///////////////////////
///// Getters
u32 dpgt_get_size (const struct dpg_table *d);
lsn dpgt_min_rec_lsn (struct dpg_table *d);

///////////////////////
///// Main API
bool dpgt_exists (const struct dpg_table *t, pgno pg);
err_t dpgt_upsert (struct dpg_table *t, pgno pg, lsn rec_lsn, error *e);
err_t dpgt_add_if_ne (struct dpg_table *t, pgno pg, lsn rec_lsn, error *e);
bool dpgt_get (lsn *dest, struct dpg_table *t, pgno pg);
void dpgt_remove_expect (struct dpg_table *t, pgno pg);
void dpgt_update (struct dpg_table *t, pgno pg, lsn new_rec_lsn);

#endif // DIRTY_PAGE_TABLE_H
