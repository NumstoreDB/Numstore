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
#include "nscore/pager/ns_pager.h"
#include "nscore/types/ns_query.h"
#include "nscore/variables/ns_variables.h"
#include "os/ns_filesystem.h"
#include "os/ns_memory.h"

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

  // Optionally supply an error for error handling
  error               *e;

  // Auto transaction mode
  struct
  {
    bool       allow;
    bool       in_auto_txn;
    struct txn tx;
  } auto_tx;
};

DEFINE_DBG_ASSERT (struct nsdb, nsdb, n, {
  ASSERT (n);
  ASSERT (n->p);
  ASSERT (n->path.data != NULL);
  ASSERT (n->path.len > 0);
})

// Lifecycle
struct nsdb *nsdb_open (const char *path, struct i_mem mem, struct i_file_system fs, error *e);
err_t nsdb_cleanup (const char *path, error *e);
err_t nsdb_close (struct nsdb *ns, error *e);
err_t nsdb_crash (struct nsdb *ns, error *e);

// Options / configuration
void nsdb_allow_auto_txn (struct nsdb *db);
void nsdb_set_error (struct nsdb *db, error *e);
err_t nsdb_writeit_numstore (struct nsdb *db, error *e);
err_t nsdb_writeit_smartfiles (struct nsdb *db, error *e);

/////////////////////////////////////// Transaction Control

struct txn *nsdb_begin (struct nsdb *db, error *e);
err_t nsdb_commit (struct nsdb *db, struct txn *txn, error *e);
err_t nsdb_rollback (struct nsdb *db, struct txn *txn, error *e);

/////////////////////////////////////// Variables - immutable variable data from
/// the database

/**
 * A variable has it's own memory space - and has a lifecycle
 * that's decoupled from nsdb
 */
struct nsdb_var
{
  struct variable    var;   // the actual variable
  struct arena_alloc alloc; // the allocator for the variable
  struct i_mem       mem;   // The memory used to allocate this struct
};

DEFINE_DBG_ASSERT (struct nsdb_var, nsdb_variable, v, {
  ASSERT (v);
  DBG_ASSERT (variable, &v->var);
});

// Lifecycle
struct nsdb_var *nsdb_var_create (struct i_mem mem, error *e);
void nsdb_var_free (struct nsdb_var *var);

// Getters
struct arena_alloc *nsdb_var_alloc (struct nsdb_var *var);
struct variable *nsdb_var_var (struct nsdb_var *var);
b_size nsdb_var_len (struct nsdb_var *var);
b_size nsdb_var_nbytes (struct nsdb_var *var);
pgno nsdb_var_var_root (struct nsdb_var *var);
pgno nsdb_var_rpt_root (struct nsdb_var *var);
struct string nsdb_var_name (struct nsdb_var *var);
struct type *nsdb_var_type (struct nsdb_var *var);

/////////////////////////////////////// Nsdb Plan - a pre compiled statement
/// that can be executed

struct nsdb_plan
{
  struct arena_alloc alloc; // Allocator for [q] and [copied_query]

  // The compiled query
  struct query       q;

  struct nsdb       *parent;
};

DEFINE_DBG_ASSERT (struct nsdb_plan, nsdb_plan, n, {
  ASSERT (n);
  DBG_ASSERT (nsdb, n->parent);
})

// Lifecycle
struct nsdb_plan *nsdb_plan_create (struct nsdb *db, const char *query, error *e);
struct nsdb_plan *nsdb_plan_fcreate (struct nsdb *db, const char *query, error *e, ...);
struct nsdb_plan *nsdb_plan_vcreate (struct nsdb *db, const char *query, error *e, va_list args);
void nsdb_plan_free (struct nsdb_plan *plan);

/////////////////////////////////////// Executions

/**
 * Execute a single query.
 * Must be any query that doesn't take in parameters
 *
 * Example:
 *    ns_execute(db, tx, "create foo u32");
 *    ns_execute(db, tx, "remove foo[0:]");
 *    ns_execute(db, tx, "insert foo 0 10");       X FAILS
 *    ns_execute(db, tx, "read foo[0:10]");        X FAILS
 *    ns_execute(db, tx, "write foo[0:10]");       X FAILS
 */
err_t nsdb_execute (struct nsdb *db, struct txn *tx, const char *query, error *e);
sb_size nsdb_plan_execute (struct nsdb_plan *ns, struct txn *tx, error *e);

/**
 * Get the variable associated with a query
 * Doesn't every actually execute anything
 *
 * Example:
 *    nsdb_var_t* var = ns_get_var(db, tx, "get foo");
 *    nsdb_var_t* var = ns_get_var(db, tx, "insert foo[0:10]");
 *    nsdb_var_t* var = ns_get_var(db, tx, "delete foo");
 */
struct nsdb_var *nsdb_get_var (struct nsdb *db, struct txn *tx, const char *query, error *e);
struct nsdb_var *nsdb_plan_get_var (struct nsdb_plan *st, struct txn *tx, error *e);

/**
 * Execute a query and read into a fixed sized buffer
 * Must be a "readable" query (READ/REMOVE only)
 *
 * Example:
 *    u32 dest[10];
 *    sb_size read = ns_read(db, tx, dest, sizeof(dest), "read foo[0:10]");
 *    sb_size removed = ns_read(db, tx, dest, sizeof(dest), "remove foo[0:10]");
 *    sb_size len = ns_read(db, tx, dest, sizeof(dest), "insert foo 0 10");   X
 * FAILS sb_size len = ns_read(db, tx, dest, sizeof(dest), "get foo"); X FAILS
 */
sb_size nsdb_read (
    struct nsdb *db,
    struct txn  *txn,
    void        *dest,
    b_size       dlen,
    const char  *query,
    error       *e
);
sb_size nsdb_plan_read (struct nsdb_plan *st, struct txn *tx, void *dest, b_size dlen, error *e);

/**
 * Execute a query and malloc an output buffer
 * Must be a "readable" query (READ/REMOVE only)
 *
 * Example:
 *    b_size len;
 *    void* data = ns_malloc(db, tx, &len, "read foo[0:10]");
 *    void* data = ns_malloc(db, tx, &len, "remove foo[0:10]");
 *    void* data = ns_malloc(db, tx, &len, "remove foo[0:10]");
 *    void* data = ns_malloc(db, tx, &len, "insert foo 0 10");   X FAILS
 *    void* data = ns_malloc(db, tx, &len, "delete foo");        X FAILS
 */
void *nsdb_read_malloc (
    struct nsdb *db,
    struct txn  *txn,
    b_size      *dlen,
    const char  *query,
    error       *e
);
void *nsdb_plan_read_malloc (struct nsdb_plan *st, struct txn *tx, b_size *dlen, error *e);

/**
 * Execute a query and write out of a fixed sized buffer
 * Must be a "writable" query (INSERT/WRITE only)
 *
 * Example:
 *    b_size len;
 *    void* data = ns_malloc(db, tx, &len, "read foo[0:10]");
 *    void* data = ns_malloc(db, tx, &len, "remove foo[0:10]");
 *    void* data = ns_malloc(db, tx, &len, "remove foo[0:10]");
 *    void* data = ns_malloc(db, tx, &len, "insert foo 0 10");   X FAILS
 *    void* data = ns_malloc(db, tx, &len, "delete foo");        X FAILS
 */
sb_size nsdb_write (
    struct nsdb *db,
    struct txn  *txn,
    const void  *src,
    b_size       dlen,
    const char  *query,
    error       *e
);
sb_size nsdb_plan_write (
    struct nsdb_plan *st,
    struct txn       *tx,
    const void       *src,
    b_size            dlen,
    error            *e
);

/**
 * Execute a query and output to the terminal
 */
err_t nsdb_console (struct nsdb *db, struct txn *txn, const char *query, error *e);
err_t nsdb_plan_console (struct nsdb_plan *st, struct txn *tx, error *e);

// Smart files patterns
sb_size nsdb_smfile_size (struct nsdb *smf, struct txn *tx, error *e);
sb_size nsdb_smfile_insert (
    struct nsdb *smf,
    struct txn  *tx,
    const void  *src,
    sb_size      bofst,
    b_size       slen,
    error       *e
);
sb_size nsdb_smfile_write (
    struct nsdb *smf,
    struct txn  *tx,
    const void  *src,
    t_size       size,
    sb_size      bofst,
    sb_size      stride,
    b_size       nelem,
    error       *e
);
sb_size nsdb_smfile_read (
    struct nsdb *smf,
    struct txn  *tx,
    void        *dest,
    t_size       size,
    sb_size      bofst,
    sb_size      stride,
    b_size       nelem,
    error       *e
);
sb_size nsdb_smfile_remove (
    struct nsdb *smf,
    struct txn  *tx,
    void        *dest,
    t_size       size,
    sb_size      bofst,
    sb_size      stride,
    b_size       nelem,
    error       *e
);

#endif // NSHANDLE_H
