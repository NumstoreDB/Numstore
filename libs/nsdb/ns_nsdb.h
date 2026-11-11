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

#include "ns_csx_assert.h"
#include "ns_error.h"
#include "ns_latch.h"
#include "ns_slab_alloc.h"
#include "ns_string.h"
#include "os/ns_malloc.h"
#include "os/ns_os.h"
#include "pager/ns_pager.h"
#include "types/ns_query.h"
#include "types/ns_variables.h"

/////////////////////////////////////// NSDB

struct nsdb
{
  // Allocates transaction objects
  struct slab_alloc txn_alloc;

  // Allocates plans
  struct slab_alloc plan_alloc;

  struct pager     *p;

  // Database path
  struct string     path;

  // OS Resources
  struct i_mem      mem;
  struct i_os       os;

  // Protects every change to this struct after open. Today that is [e]
  latch             l;

  // The last error of any call on this database, until someone consumes it.
  // Every failed call copies its error here, so with several threads a
  // later failure replaces an earlier one. Guarded by [l]
  error             e;

  // Auto transaction mode - a call with a NULL transaction runs in one of
  // its own. Each of those comes from txn_alloc, the same as nsdb_begin, so
  // calls from several threads don't share one
  struct
  {
    bool allow;
  } auto_tx;
};

DEFINE_DBG_ASSERT (struct nsdb, nsdb, n, {
  ASSERT (n);
  ASSERT (n->p);
  ASSERT (n->path.data != NULL);
  ASSERT (n->path.len > 0);
})

// Lifecycle
struct nsdb *nsdb_open (const char *path, struct i_mem mem, struct i_os os, error *e);
err_t nsdb_cleanup (const char *path, struct i_os os, error *e);
err_t nsdb_close (struct nsdb *ns, error *e);
err_t nsdb_crash (struct nsdb *ns, error *e);

// Options / configuration
void nsdb_allow_auto_txn (struct nsdb *db);
err_t nsdb_init_numstore (struct nsdb *db, error *e);
err_t nsdb_init_smartfiles (struct nsdb *db, error *e);

/////////////////////////////////////// Transaction Control

/**
 * A transaction with an error inside it
 * indicating the status of this transaction
 */
struct nsdb_txn
{
  struct txn tx;
  error      e;
};

DEFINE_DBG_ASSERT (struct nsdb_txn, nsdb_txn, t, {
  ASSERT (t);
  DBG_ASSERT (ns_txn, &t->tx);
})

struct nsdb_txn *nsdb_begin (struct nsdb *db, error *e);

// Both end the transaction, also when they fail. A failure is in the
// database's error, because [txn] is gone by then
err_t nsdb_commit (struct nsdb *db, struct nsdb_txn *txn);
err_t nsdb_rollback (struct nsdb *db, struct nsdb_txn *txn);

/////////////////////////////////////// Errors

/**
 * Where the error of a failed call goes
 *
 * A call that takes a transaction has no error parameter. It works on an
 * error of its own, and when it fails:
 *
 *   1. The error is set on the transaction (the caller's, or the auto
 *      transaction of that call).
 *   2. The transaction's error is copied to the database's error.
 *
 * A call that fails with no transaction to hold the error sets only the
 * database's error.
 *
 * Two rules hold for both errors:
 *
 *   1. The database never takes an error back to SUCCESS. A failed call
 *      replaces the error that was there, a call that works leaves it alone.
 *   2. Only the consume functions take an error to SUCCESS.
 *
 * A transaction belongs to one thread, so its error is that thread's alone.
 * The database's error is shared: failures on several threads replace each
 * other, one whole error at a time.
 */

// Copies the error of [txn] to the database's error. [txn] must have failed
void nsdb_copy_txn_error (struct nsdb *db, const struct nsdb_txn *txn);

// Records [e], the error of a failed call: on [tx] when it isn't NULL, and
// on the database. Returns the code of [e]
err_t nsdb_report_error (struct nsdb *db, struct nsdb_txn *tx, const error *e);

/**
 * Takes the pending error and returns its message, or NULL when there is no
 * error. The error is SUCCESS afterwards, so the next call returns NULL.
 *
 * [copy] true:  the message is a new string from [db->mem], and the caller
 *               frees it. This is the thread safe form.
 * [copy] false: the message is the buffer inside the error. A later failure
 *               overwrites it - for the database's error that can be another
 *               thread, at any time.
 *
 * NULL is also the result when [copy] is true and the allocation fails. The
 * error stays pending then.
 */
const char *nsdb_consume_error (struct nsdb *db, bool copy);
const char *nsdb_txn_consume_error (struct nsdb *db, struct nsdb_txn *txn, bool copy);

// The same, for a caller that works with an error struct: moves the pending
// error (code and message) into [dest], which must be clean. Returns the
// code, SUCCESS when there was no error
err_t nsdb_consume_error_into (struct nsdb *db, error *dest);
err_t nsdb_txn_consume_error_into (struct nsdb *db, struct nsdb_txn *txn, error *dest);

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

// A plan has no error of its own. A failed call on a plan sets the error of
// its transaction and of its database, and this consumes the database's.
// See nsdb_consume_error
const char *nsdb_plan_consume_error (struct nsdb_plan *plan, bool copy);

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
err_t nsdb_execute (struct nsdb *db, struct nsdb_txn *tx, const char *query);
sb_size nsdb_plan_execute (struct nsdb_plan *ns, struct nsdb_txn *tx);

/**
 * Get the variable associated with a query
 * Doesn't every actually execute anything
 *
 * Example:
 *    nsdb_var_t* var = ns_get_var(db, tx, "get foo");
 *    nsdb_var_t* var = ns_get_var(db, tx, "insert foo[0:10]");
 *    nsdb_var_t* var = ns_get_var(db, tx, "delete foo");
 */
struct nsdb_var *nsdb_get_var (struct nsdb *db, struct nsdb_txn *tx, const char *query);
struct nsdb_var *nsdb_plan_get_var (struct nsdb_plan *st, struct nsdb_txn *tx);

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
    struct nsdb     *db,
    struct nsdb_txn *txn,
    void            *dest,
    b_size           dlen,
    const char      *query
);
sb_size nsdb_plan_read (struct nsdb_plan *st, struct nsdb_txn *tx, void *dest, b_size dlen);

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
void *nsdb_read_malloc (struct nsdb *db, struct nsdb_txn *txn, b_size *dlen, const char *query);
void *nsdb_plan_read_malloc (struct nsdb_plan *st, struct nsdb_txn *tx, b_size *dlen);

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
    struct nsdb     *db,
    struct nsdb_txn *txn,
    const void      *src,
    b_size           dlen,
    const char      *query
);
sb_size nsdb_plan_write (struct nsdb_plan *st, struct nsdb_txn *tx, const void *src, b_size dlen);

/**
 * Execute a query and output to the terminal
 */
err_t nsdb_console (struct nsdb *db, const char *query, error *e);
err_t nsdb_plan_console (struct nsdb_plan *st, error *e);

// Smart files patterns
sb_size nsdb_smfile_size (struct nsdb *smf, struct nsdb_txn *tx);
sb_size nsdb_smfile_insert (
    struct nsdb     *smf,
    struct nsdb_txn *tx,
    const void      *src,
    sb_size          bofst,
    b_size           slen
);
sb_size nsdb_smfile_write (
    struct nsdb     *smf,
    struct nsdb_txn *tx,
    const void      *src,
    t_size           size,
    sb_size          bofst,
    sb_size          stride,
    b_size           nelem
);
sb_size nsdb_smfile_read (
    struct nsdb     *smf,
    struct nsdb_txn *tx,
    void            *dest,
    t_size           size,
    sb_size          bofst,
    sb_size          stride,
    b_size           nelem
);
sb_size nsdb_smfile_remove (
    struct nsdb     *smf,
    struct nsdb_txn *tx,
    void            *dest,
    t_size           size,
    sb_size          bofst,
    sb_size          stride,
    b_size           nelem
);

#endif // NSHANDLE_H
