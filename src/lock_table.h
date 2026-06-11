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

#include "alloc.h"          // slab alloc
#include "compile_config.h" // pgno ...etc
#include "concurrency.h"    // latch / grlock
#include "error.h"          // error
#include "htable.h"         // htable
#include "numerics.h"       // randu64
#include "platform.h"       // HEADER_FUNC/PLATFORM_WINDOWS ...etc
#include "stdtypes.h"       // u32 ...etc

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

    LOCK_VHP,     // The variable hash page
    LOCK_VHP_POS, // An individual spot in the variable hash page

    // Individual Variables
    LOCK_VAR,        // A variable
    LOCK_VAR_NEXT,   // The next pointer of a variable
    LOCK_VAR_ROOT,   // The variable root of a variable
    LOCK_VAR_NBYTES, // The number of bytes in a variable

    // Individual rptrees
    LOCK_RPTREE, // An entire rptree
  } type;

  union lt_lock_data {
    pgno vhp_pos;     // LOCK_VHP_POS,
    pgno var_root;    // LOCK_VAR, LOCK_VAR_NEXT, LOCK_VAR_ROOT,
                      // LOCK_VAR_NBYTES,
    pgno rptree_root; // LOCK_RPTREE,
  } data;
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
 * @brief Prints a lt_lock cleanly
 *
 * @param log_level What log level to print at
 * @param l The lock to print
 */
void i_print_lt_lock (int log_level, struct lt_lock l);

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
lock_create (const enum lt_lock_type type, const union lt_lock_data data)
{
  return (struct lt_lock){.type = type, .data = data};
}

/**
 * @brief Shorthand for database lock
 *
 * @return a database lock
 */
HEADER_FUNC struct lt_lock
lock_db (void)
{
  return lock_create (LOCK_DB, (union lt_lock_data){0});
}

/**
 * @brief Shorthand for variable hash page
 *
 * @return A variable hash page lock
 */
HEADER_FUNC struct lt_lock
lock_vhp (void)
{
  return lock_create (LOCK_VHP, (union lt_lock_data){0});
}

/**
 * @brief Shorthand for variable hash page position page
 *
 * @param vhp_pos The position within the variable hash page
 * @return The variable hash page
 */
HEADER_FUNC struct lt_lock
lock_vhp_pos (const pgno vhp_pos)
{
  return lock_create (LOCK_VHP_POS, (union lt_lock_data){.vhp_pos = vhp_pos});
}

/**
 * @brief Shorthand for locking a variable
 *
 * @param var_root The root of the variable (careful, not the rpt_root)
 * @return The variable to lock
 */
HEADER_FUNC struct lt_lock
lock_var (const pgno var_root)
{
  return lock_create (LOCK_VAR, (union lt_lock_data){.var_root = var_root});
}

/**
 * @brief Shorthand for the next page of a variable
 *
 * @param var_root The root of the variable
 * @return The lock for the next pointer
 */
HEADER_FUNC struct lt_lock
lock_var_next (const pgno var_root)
{
  return lock_create (
      LOCK_VAR_NEXT,
      (union lt_lock_data){.var_root = var_root}
  );
}

/**
 * @brief Shorthand for the root of a variable
 *
 * @param var_root The root of the variable
 * @return The lock for the rpt root of the variable
 */
HEADER_FUNC struct lt_lock
lock_var_root (const pgno var_root)
{
  return lock_create (
      LOCK_VAR_ROOT,
      (union lt_lock_data){.var_root = var_root}
  );
}

/**
 * @brief Shorthand for the bytes value for a variable
 *
 * @param var_root The root of the variable
 * @return The lock for the nbytes of the variable
 */
HEADER_FUNC struct lt_lock
lock_var_nbytes (const pgno var_root)
{
  return lock_create (
      LOCK_VAR_NBYTES,
      (union lt_lock_data){.var_root = var_root}
  );
}

/**
 * @brief Shorthand for locking an entire rptree root
 *
 * @param rptree_root The root of the tree
 * @return The lock for the root of the variable
 */
HEADER_FUNC struct lt_lock
lock_rptree (const pgno rptree_root)
{
  return lock_create (
      LOCK_RPTREE,
      (union lt_lock_data){.rptree_root = rptree_root}
  );
}

#define LT_LOCK_FOR_EACH(X) \
  X (LOCK_DB)               \
  X (LOCK_VHP)              \
  X (LOCK_VHP_POS)          \
  X (LOCK_VAR)              \
  X (LOCK_VAR_NEXT)         \
  X (LOCK_VAR_ROOT)         \
  X (LOCK_VAR_NBYTES)       \
  X (LOCK_RPTREE)

#define LT_LOCK_FOR_EACH_RANDOM(X)                           \
  X (LOCK_DB, lock_db ())                                    \
  X (LOCK_VHP, lock_vhp ())                                  \
  X (LOCK_VHP_POS, lock_vhp_pos (randu64r (0, 10000)))       \
  X (LOCK_VAR, lock_var (randu64r (0, 10000)))               \
  X (LOCK_VAR_NEXT, lock_var_next (randu64r (0, 10000)))     \
  X (LOCK_VAR_ROOT, lock_var_root (randu64r (0, 10000)))     \
  X (LOCK_VAR_NBYTES, lock_var_nbytes (randu64r (0, 10000))) \
  X (LOCK_RPTREE, lock_rptree (randu64r (0, 10000)))

/**
 * @brief Generate a random lock
 * @return a random lock
 */
struct lt_lock random_lt_lock (void);

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

void i_log_lockt (int log_level, const struct lockt *t);

#endif // LOCK_TABLE_H
