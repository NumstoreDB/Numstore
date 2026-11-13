/// Copyright 2026 Theo Linckelock_ta
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

#ifndef LOCK_TABLE_H
#define LOCK_TABLE_H

#include "lock_table/ns_lt_lock.h"
#include "ns_csx_assert.h"
#include "ns_error.h"
#include "ns_gr_lock.h"
#include "ns_htable.h"
#include "ns_latch.h"
#include "ns_slab_alloc.h"
#include "os/ns_malloc.h"
#include "os/ns_os.h"
#include "txn_table/ns_txn.h"

struct lockt
{
  struct i_os       os;         // The OS every gr_lock in the table is built on
  struct slab_alloc lock_alloc; // Allocate gr locks
  struct htable    *table;      // The table of locks
  latch             l;          // Latch for modifications
};

DEFINE_DBG_ASSERT (struct lockt, lockt, t, {
  ASSERT (t);
  ASSERT (t->table);
})

///////////////////////
///// Lifecycle
err_t lockt_init (struct lockt *t, struct i_mem mem, struct i_os os, error *e);
void lockt_destroy (struct lockt *t);

HEADER_FUNC struct lockt *
lockt_create (struct i_mem mem, struct i_os os, error *e)
{
  struct lockt *lt = mem.table->malloc (mem.self, 1, sizeof *lt, e);
  if (lt == NULL) {
    return NULL;
  }
  if (lockt_init (lt, mem, os, e) < 0) {
    mem.table->free (mem.self, lt);
    return NULL;
  }
  return lt;
}

// Undoes lockt_create: destroys the table and frees the struct itself
HEADER_FUNC void
lockt_free (struct lockt *lt, struct i_mem mem)
{
  lockt_destroy (lt);
  mem.table->free (mem.self, lt);
}

///////////////////////
///// Utilities
void i_log_lockt (int log_level, struct lockt *t);

///////////////////////
///// Primary api
err_t lockt_lock (
    struct lockt  *t,
    struct lt_lock lock,
    enum lock_mode mode,
    struct txn    *tx,
    error         *e
);
void lockt_unlock (struct lockt *t, struct lt_lock lock, enum lock_mode mode);
void lockt_unlock_tx (struct lockt *t, struct txn *tx);

#endif // LOCK_TABLE_H
