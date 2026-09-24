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
 * @brief Internals of numstore user exposed type
 */

#ifndef NSHANDLE_H
#define NSHANDLE_H

#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_string.h"
#include "core/os/ns_filesystem.h"
#include "core/os/ns_memory.h"
#include "nscore/pager/ns_pager.h"
#include "nscore/types/ns_query.h"
#include "nscore/variables/ns_variables.h"

/////////////////////////////////////// NSDB

struct nsdb
{
  // Allocates transaction objects
  struct slab_alloc    txn_alloc;

  // Allocates plans
  struct slab_alloc    plan_alloc;

  struct pager        *p;

  // Database path
  struct string        path;

  // OS Resources
  struct i_mem         mem;
  struct i_file_system fs;

  error                e;
};

DEFINE_DBG_ASSERT (struct nsdb, nsdb, n, {
  ASSERT (n);
  ASSERT (n->p);
  ASSERT (n->path.data != NULL);
  ASSERT (n->path.len > 0);
})

struct nsdb *nsdb_open_with_resources (
    const char          *path,
    struct i_mem         mem,
    struct i_file_system fs,
    error               *e
);
err_t nsdb_cleanup (const char *path, error *e);
err_t nsdb_close (struct nsdb *ns, error *e);
err_t nsdb_crash (struct nsdb *ns, error *e);

struct txn *nsdb_begin (struct nsdb *db);
err_t nsdb_commit (struct nsdb *db, struct txn *txn);
err_t nsdb_rollback (struct nsdb *db, struct txn *txn);

/////////////////////////////////////// Numstore Var

struct nsdb_var
{
  struct variable    var;
  struct arena_alloc alloc;
  struct i_mem       mem;
};

DEFINE_DBG_ASSERT (struct nsdb_var, nsdb_variable, v, {
  ASSERT (v);
  DBG_ASSERT (variable, &v->var);
});

struct nsdb_var *nsdb_var_create (struct i_mem mem, error *e);
void nsdb_var_free (struct nsdb_var *var);

struct arena_alloc *nsdb_var_alloc (struct nsdb_var *var);
struct variable *nsdb_var_var (struct nsdb_var *var);

b_size nsdb_var_len (struct nsdb_var *var);
b_size nsdb_var_nbytes (struct nsdb_var *var);
pgno nsdb_var_var_root (struct nsdb_var *var);
pgno nsdb_var_rpt_root (struct nsdb_var *var);
struct string nsdb_var_name (struct nsdb_var *var);
struct type *nsdb_var_type (struct nsdb_var *var);

/////////////////////////////////////// Plan

struct ns_plan
{
  struct pager      *p;     // The database to use
  struct i_mem       mem;   // Memory to malloc variables in plan_malloc
  struct arena_alloc alloc; // Allocator for stuff in this variable
  struct query       q;     // The active query
  error             *e;
};

DEFINE_DBG_ASSERT (struct ns_plan, ns_plan, n, {
  ASSERT (n);
  ASSERT (n->p);
})

struct ns_plan *ns_plan_create (struct nsdb *db, const char *query);
void ns_plan_free (struct nsdb *db, struct ns_plan *plan);

// Execute the plan
err_t ns_plan_execute (struct ns_plan *ns, struct txn *tx);
struct nsdb_var *ns_plan_get_var (struct ns_plan *st, struct txn *tx);
sb_size ns_plan_read (struct ns_plan *st, struct txn *tx, void *dest, b_size dlen);
void *ns_plan_read_malloc (struct ns_plan *st, struct txn *tx, b_size *dlen);
sb_size ns_plan_write (struct ns_plan *st, struct txn *tx, const void *src, b_size dlen);
err_t ns_plan_execute_in_console (struct ns_plan *st, struct txn *tx);

/////////////////////////////////////// Auto Plan

err_t nsdb_exec (struct nsdb *db, struct txn *tx, const char *query);
struct nsdb_var *nsdb_get_var (struct nsdb *db, struct txn *tx, const char *query);
sb_size nsdb_read (struct nsdb *db, struct txn *txn, void *dest, b_size dlen, const char *query);
void *nsdb_read_malloc (struct nsdb *db, struct txn *txn, b_size *dlen, const char *query);
sb_size nsdb_write (
    struct nsdb *db,
    struct txn  *txn,
    const void  *src,
    b_size       dlen,
    const char  *query
);
err_t nsdb_console (struct nsdb *db, struct txn *txn, const char *query);

/////////////////////////////////////// Auto Transaction

struct auto_txn
{
  struct txn *tx;
  bool        is_auto_txn;
};

static inline err_t
nsdb_auto_begin (struct nsdb *db, struct txn *tx, struct auto_txn *auto_tx)
{
  auto_tx->tx          = tx;
  auto_tx->is_auto_txn = false;

  if (tx == NULL) {
    auto_tx->tx = nsdb_begin (db);
    if (auto_tx->tx == NULL) {
      return error_trace (&db->e);
    }
    auto_tx->is_auto_txn = true;
  }

  return SUCCESS;
}

static inline err_t
nsdb_auto_commit (struct nsdb *db, struct auto_txn *auto_tx)
{
  ASSERT (auto_tx->tx);
  if (auto_tx->is_auto_txn) {
    struct txn *tx = auto_tx->tx;
    auto_tx->tx    = NULL;
    return nsdb_commit (db, tx);
  }
  return SUCCESS;
}

static inline err_t
nsdb_auto_rollback (struct nsdb *db, struct auto_txn *auto_tx)
{
  ASSERT (auto_tx->tx);
  if (auto_tx->is_auto_txn) {
    struct txn *tx = auto_tx->tx;
    auto_tx->tx    = NULL;
    return nsdb_rollback (db, tx);
  }
  return SUCCESS;
}

#define WITH_AUTO_TXN(res, db, _tx, expr, e)               \
  do {                                                     \
    struct txn     *_saved_tx = (_tx);                     \
    struct auto_txn _auto_tx;                              \
    if (nsdb_auto_begin ((db), (_tx), &_auto_tx)) {        \
      (res) = error_trace (e);                             \
    } else {                                               \
      (_tx) = _auto_tx.tx;                                 \
      (res) = (expr);                                      \
      if ((res) < 0) {                                     \
        nsdb_auto_rollback ((db), &_auto_tx);              \
      } else if (nsdb_auto_commit ((db), &_auto_tx) < 0) { \
        (res) = error_trace (e);                           \
      }                                                    \
      (_tx) = _saved_tx;                                   \
    }                                                      \
  }                                                        \
  while (0)

#define WITH_AUTO_TXN_PTR(res, db, _tx, expr)              \
  do {                                                     \
    struct txn     *_saved_tx = (_tx);                     \
    struct auto_txn _auto_tx;                              \
    if (nsdb_auto_begin ((db), (_tx), &_auto_tx)) {        \
      (res) = NULL;                                        \
    } else {                                               \
      (_tx) = _auto_tx.tx;                                 \
      (res) = (expr);                                      \
      if ((res) == NULL) {                                 \
        nsdb_auto_rollback ((db), &_auto_tx);              \
      } else if (nsdb_auto_commit ((db), &_auto_tx) < 0) { \
        (res) = NULL;                                      \
      }                                                    \
      (_tx) = _saved_tx;                                   \
    }                                                      \
  }                                                        \
  while (0)

#endif // NSHANDLE_H
