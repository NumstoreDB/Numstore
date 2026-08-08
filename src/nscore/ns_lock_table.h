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

/**
 * @file
 * @brief Database Lock Table
 *
 * Contains:
 * 1. LT Lock - a heirarchial lock
 * 2. Lock table - a place to hold logical locks
 */

#ifndef LOCK_TABLE_H
#define LOCK_TABLE_H

#include "alloc.h"       // slab alloc
#include "concurrency.h" // latch / grlock
#include "error.h"       // error
#include "htable.h"      // htable
#include "platform.h"    // HEADER_FUNC/PLATFORM_WINDOWS ...etc
#include "stdtypes.h"    // u32 ...etc

/******************************************************************************
 * SECTION: LT Lock
 * ----------------------------------------------------------------------------
 * @brief A Heirarchial lock
 *
 * Locks are represented as tagged unions in memory
 * each lock type has its own meta information
 ******************************************************************************/

struct lt_lock
{
  enum lt_lock_type
  {
    LOCK_DB, // The whole database
  } type;
};

/**
 * @brief Generate the hash table key from a lock
 *
 * serializes the struct and
 * computes the checksum
 *
 * @param lock The lock to get the key on
 * @return The hash table key
 */
u32 lt_lock_key (struct lt_lock lock);

/**
 * @brief Are two locks equal to one another
 *
 * @param left The left lock to compare
 * @param right The right lock to compare
 * @return if left or right are equal
 */
bool lt_lock_equal (const struct lt_lock left, const struct lt_lock right);

/**
 * @brief Gets the parent lt_lock in heirarchial locking
 *
 * lt_locks are heirarchial locks - lock types have parents
 * and those parent locks represent an umbrella on the lock type
 *
 * @param parent The parent destination
 * @param lock The lock to get a parent from
 * @return If the lock even has a parent
 */
bool get_parent (struct lt_lock *parent, struct lt_lock lock);

/**
 * @brief Creates a lock of [type] with [data]
 *
 * @param type The lock type
 * @param data The data inside the lock
 * @return A new lt_lock
 */
HEADER_FUNC struct lt_lock
lock_create (const enum lt_lock_type type)
{
  return (struct lt_lock){.type = type};
}

/**
 * @brief Shorthand for database lock
 *
 * @return a database lock
 */
HEADER_FUNC struct lt_lock
lock_db (void)
{
  return lock_create (LOCK_DB);
}

/******************************************************************************
 * SECTION: Lock Table
 * ----------------------------------------------------------------------------
 * @brief A lock table for heirarchial locking
 ******************************************************************************/

// Forward declaration to avoid circular dependency
struct txn;

struct lockt
{
  struct slab_alloc lock_alloc; // Allocate gr locks
  struct htable    *table;      // The table of locks
  latch             l;          // Latch for modifications
};

err_t lockt_init (struct lockt *t, error *e);
void  lockt_destroy (struct lockt *t);
void  lockt_crash (struct lockt *t);

err_t lockt_lock (
    struct lockt  *t,
    struct lt_lock lock,
    enum lock_mode mode,
    struct txn    *tx,
    error         *e
);

err_t lockt_unlock (
    struct lockt  *t,
    struct lt_lock lock,
    enum lock_mode mode,
    error         *e
);

void lockt_unlock_tx (struct lockt *t, struct txn *tx);

#endif // LOCK_TABLE_H
