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

#include "core/ns_error.h"
#include "core/ns_gr_lock.h"
#include "core/ns_htable.h"
#include "core/ns_latch.h"
#include "core/ns_slab_alloc.h"
#include "core/os/ns_malloc.h"
#include "core/os/ns_os.h"

#include <stdbool.h>

struct txn;

///////////////////// Lock data
struct lt_lock
{
  enum lt_lock_type
  {
    LOCK_DB, // The whole database
  } type;
};

// Builders
#define lock_create(_type) ((struct lt_lock){.type = (_type)})
#define lock_db()          lock_create (LOCK_DB)

///////////////////// Lock Table
struct lockt
{
  struct i_os       os;         // The OS every gr_lock in the table is built on
  struct slab_alloc lock_alloc; // Allocate gr locks
  struct htable    *table;      // The table of locks
  latch             l;          // Latch for modifications
};

err_t lockt_init (struct lockt *t, struct i_mem mem, struct i_os os, error *e);
void lockt_destroy (struct lockt *t);
void lockt_crash (struct lockt *t);
err_t lockt_lock (
    struct lockt  *t,
    struct lt_lock lock,
    enum lock_mode mode,
    struct txn    *tx,
    error         *e
);
void lockt_unlock (struct lockt *t, struct lt_lock lock, enum lock_mode mode);
void lockt_unlock_tx (struct lockt *t, struct txn *tx);
void i_log_lockt (int log_level, struct lockt *t);
bool lt_lock_equal (struct lt_lock left, struct lt_lock right);

#endif // LOCK_TABLE_H
