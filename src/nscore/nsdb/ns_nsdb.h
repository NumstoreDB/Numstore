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
int nsdb_cleanup (const char *path, error *e);
int nsdb_close (struct nsdb *ns, error *e);
int nsdb_crash (struct nsdb *ns, error *e);

struct ns_txn *nsdb_begin (struct nsdb *smf, error *e);
int nsdb_commit (struct nsdb *smf, struct ns_txn *txn, error *e);
int nsdb_rollback (struct nsdb *smf, struct ns_txn *txn, error *e);

/////////////////////////////////////// Numstore Var

struct nsdb_var;

struct nsdb_var *nsdb_var_create (struct i_mem mem, error *e);
void nsdb_var_free (struct nsdb_var *var);

struct arena_alloc *nsdb_var_alloc(struct nsdb_var *var);
struct variable* nsdb_var_var(struct nsdb_var *var);

b_size nsdb_var_len (struct nsdb_var *var);
b_size nsdb_var_nbytes (struct nsdb_var *var);
pgno nsdb_var_var_root (struct nsdb_var *var);
pgno nsdb_var_rpt_root (struct nsdb_var *var);
struct string nsdb_var_name(struct nsdb_var *var);
struct type* nsdb_var_type(struct nsdb_var *var);

/////////////////////////////////////// Plan

struct ns_plan
{
  struct pager        *p;     // The database to use
  struct i_mem        mem;   // Memory to malloc variables in plan_malloc
  struct arena_alloc  alloc; // Allocator for stuff in this variable
  struct query        q;     // The active query
};

DEFINE_DBG_ASSERT (struct ns_plan, ns_plan, n, {
  ASSERT (n);
  ASSERT (n->p);
})

struct ns_plan *ns_plan_create (struct nsdb *db, const char *query, error *e);
void ns_plan_free (struct nsdb *db, struct ns_plan *plan);

// Execute the plan
err_t ns_plan_execute (struct ns_plan *ns, struct ns_txn *tx, error *e);
struct nsdb_var *ns_plan_get_var (struct ns_plan *st, struct ns_txn *tx, error *e);
sb_size ns_plan_read (struct ns_plan *st, struct ns_txn *tx, void *dest, b_size dlen, error *e);
void *ns_plan_read_malloc (struct ns_plan *st, struct ns_txn *tx, b_size *dlen, error *e);
sb_size ns_plan_write (
    struct ns_plan *st,
    struct ns_txn  *tx,
    const void     *src,
    b_size          dlen,
    error          *e
);
err_t ns_plan_execute_in_console (struct ns_plan *st, struct ns_txn *tx, error *e);

/////////////////////////////////////// Auto Plan

HEADER_FUNC err_t
nsdb_exec (struct nsdb *db, struct ns_txn *tx, const char *query, error *e)
{
  DBG_ASSERT (nsdb, db);
  DBG_ASSERT (ns_txn, tx);
  ASSERT (query);
  DBG_ASSERT (clean_error, e);

  struct ns_plan *plan = ns_plan_create (db, query, e);
  if (plan == NULL) {
    return error_trace (e);
  }

  sb_size ret = ns_plan_execute (plan, tx, e);
  if (ret < 0) {
    ns_plan_free (db, plan);
    return ret;
  }

  ns_plan_free (db, plan);

  return ret;
}

HEADER_FUNC struct nsdb_var* 
nsdb_get_var_exec (struct nsdb *db, struct ns_txn *tx, const char *query, error *e)
{
  DBG_ASSERT (nsdb, db);
  DBG_ASSERT (ns_txn, tx);
  ASSERT (query);
  DBG_ASSERT (clean_error, e);

  struct ns_plan *plan = ns_plan_create (db, query, e);
  if (plan == NULL) {
    return NULL;
  }

  struct nsdb_var* var = ns_plan_get_var(plan, tx, e);
  if (var == NULL) {
    ns_plan_free (db, plan);
    return NULL;
  }

  ns_plan_free (db, plan);

  return var;
}


HEADER_FUNC sb_size
nsdb_read_exec (
    struct nsdb   *db,
    struct ns_txn *txn,
    void          *dest,
    b_size         dlen,
    const char    *query,
    error         *e
)
{
  DBG_ASSERT (nsdb, db);
  DBG_ASSERT (ns_txn, txn);
  ASSERT (query);
  ASSERT (dest);
  ASSERT (dlen > 0);
  DBG_ASSERT (clean_error, e);

  struct ns_plan *plan = ns_plan_create (db, query, e);
  if (plan == NULL) {
    return error_trace (e);
  }

  sb_size ret = ns_plan_read (plan, txn, dest, dlen, e);
  if (ret < 0) {
    ns_plan_free (db, plan);
    return ret;
  }

  ns_plan_free (db, plan);

  return ret;
}

HEADER_FUNC void *
nsdb_read_malloc_exec (
    struct nsdb   *db,
    struct ns_txn *txn,
    b_size        *dlen,
    const char    *query,
    error         *e
)
{
  DBG_ASSERT (nsdb, db);
  DBG_ASSERT (ns_txn, txn);
  ASSERT (query);
  ASSERT (dlen);
  DBG_ASSERT (clean_error, e);

  struct ns_plan *plan = ns_plan_create (db, query, e);
  if (plan == NULL) {
    return NULL;
  }

  void *data = ns_plan_read_malloc (plan, txn, dlen, e);
  if (data == NULL) {
    ns_plan_free (db, plan);
    return NULL;
  }

  ns_plan_free (db, plan);

  return data;
}

HEADER_FUNC sb_size
nsdb_write_exec (
    struct nsdb   *db,
    struct ns_txn *txn,
    const void    *src,
    b_size         dlen,
    const char    *query,
    error         *e
)
{
  DBG_ASSERT (nsdb, db);
  DBG_ASSERT (ns_txn, txn);
  ASSERT (query);
  ASSERT (src);
  ASSERT (dlen > 0);
  DBG_ASSERT (clean_error, e);

  struct ns_plan *plan = ns_plan_create (db, query, e);
  if (plan == NULL) {
    return error_trace (e);
  }

  sb_size ret = ns_plan_write (plan, txn, src, dlen, e);
  if (ret < 0) {
    ns_plan_free (db, plan);
    return ret;
  }

  ns_plan_free (db, plan);

  return ret;
}

HEADER_FUNC err_t
ns_plan_execute_in_console_exec (struct nsdb *db, struct ns_txn *txn, const char *query, error *e)
{
  DBG_ASSERT (nsdb, db);
  DBG_ASSERT (ns_txn, txn);
  ASSERT (query);
  DBG_ASSERT (clean_error, e);

  struct ns_plan *plan = ns_plan_create (db, query, e);
  if (plan == NULL) {
    return error_trace (e);
  }

  err_t ret = ns_plan_execute_in_console (plan, txn, e);
  if (ret < 0) {
    ns_plan_free (db, plan);
    return ret;
  }

  ns_plan_free (db, plan);

  return ret;
}

/////////////////////////////////////// Auto Transaction

struct auto_txn
{
  struct ns_txn *tx;
  bool           is_auto_txn;
};

static inline err_t
nsdb_auto_begin (struct nsdb *db, struct ns_txn *tx, struct auto_txn *auto_tx, error *e)
{
  auto_tx->tx          = tx;
  auto_tx->is_auto_txn = false;

  if (tx == NULL) {
    auto_tx->tx = nsdb_begin (db, e);
    if (auto_tx->tx == NULL) {
      return error_trace (e);
    }
    auto_tx->is_auto_txn = true;
  }

  return SUCCESS;
}

static inline err_t
nsdb_auto_commit (struct nsdb *db, struct auto_txn *auto_tx, error *e)
{
  ASSERT (auto_tx->tx);
  if (auto_tx->is_auto_txn) {
    struct ns_txn *tx = auto_tx->tx;
    auto_tx->tx       = NULL;
    return nsdb_commit (db, tx, e);
  }
  return SUCCESS;
}

static inline err_t
nsdb_auto_rollback (struct nsdb *db, struct auto_txn *auto_tx, error *e)
{
  ASSERT (auto_tx->tx);
  if (auto_tx->is_auto_txn) {
    struct ns_txn *tx = auto_tx->tx;
    auto_tx->tx       = NULL;
    return nsdb_rollback (db, tx, e);
  }
  return SUCCESS;
}

#define WITH_AUTO_TXN(res, db, _tx, expr, e)                    \
  do {                                                          \
    struct ns_txn  *_saved_tx = (_tx);                          \
    struct auto_txn _auto_tx;                                   \
    if (nsdb_auto_begin ((db), (_tx), &_auto_tx, (e))) {        \
      (res) = error_trace (e);                                  \
    } else {                                                    \
      (_tx) = _auto_tx.tx;                                      \
      (res) = (expr);                                           \
      if ((res) < 0) {                                          \
        nsdb_auto_rollback ((db), &_auto_tx, (e));              \
      } else if (nsdb_auto_commit ((db), &_auto_tx, (e)) < 0) { \
        (res) = error_trace (e);                                \
      }                                                         \
      (_tx) = _saved_tx;                                        \
    }                                                           \
  }                                                             \
  while (0)

#endif // NSHANDLE_H
