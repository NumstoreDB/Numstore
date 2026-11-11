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

#ifndef NUMSTORE_H
#define NUMSTORE_H

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * The printf function suffix for cross platform
 * This is a compiler extension and not standard C
 */
#if defined(__GNUC__) || defined(__clang__)
#  define NSDB_PRINTF(fmt_idx, vargs_idx) __attribute__ ((format (printf, fmt_idx, vargs_idx)))
#else
#  define NSDB_PRINTF(fmt_idx, vargs_idx)
#endif

// Main data types
typedef struct nsdb      nsdb_t;
typedef struct nsdb_txn  nsdb_txn_t;
typedef struct nsdb_var  nsdb_var_t;
typedef struct nsdb_plan nsdb_plan_t;

// Specific sizes of "things" in a numstore database
typedef uint32_t         t_size;  // Represents the size of a single type in bytes
typedef uint64_t         b_size;  // Bytes size to index into a contiguous rope bytes
typedef int64_t          sb_size; // Signed b_size
#define MAX_ERRSTR_LEN 256
#define NS_END         INT64_MAX

//////////////////////////////////// Lifecycle

// Open a Numstore database
nsdb_t *ns_open (const char *path);
nsdb_t *ns_open_with_error (const char *path, char erstr[], unsigned elen);

// Open a Smart file
nsdb_t *ns_smfile_open (const char *path);
nsdb_t *ns_smfile_open_with_error (const char *path, char erstr[], unsigned elen);

// Remove a numstore or smart file at [path]
int ns_cleanup (const char *path);

// Close the database or file gracefully
// If a transaction is still open,
// this function will block
int ns_close (nsdb_t *ns);

// Harshly close the database or file
// Any incomplete transactions will be
// roll'ed back on next open
int ns_crash (nsdb_t *ns);

//////////////////////////////////// Variable getters

// Get the length (in elements, not bytes) of a variable
b_size ns_var_len (nsdb_var_t *var);

// Variables get their own isolated memory space,
// you can free variables before or after closing the database
void ns_var_free (nsdb_var_t *var);

//////////////////////////////////// Error handling

/**
 * Where the error of a failed call is
 *
 * A call that fails returns a negative number or NULL. The reason is kept
 * in two places:
 *
 *   1. On the transaction you passed, if you passed one. A transaction
 *      belongs to one thread, so this error is yours alone.
 *   2. On the database. Every failed call on any thread sets it, so with
 *      several threads a later failure replaces an earlier one.
 *
 * An error stays until you take it with one of the functions below. A call
 * that works doesn't clear it. So take the error after each call that
 * fails - otherwise it's still there when you ask after a later call.
 *
 * Each function returns the message of the pending error, or NULL when
 * there is none, and clears the error. A second call returns NULL.
 *
 * [copy] true:  the message is a new string, and you free() it. This is
 *               thread safe. If there is no memory for the string the
 *               result is NULL and the error stays pending.
 * [copy] false: the message points into the database (or transaction) and
 *               the next failure overwrites it. This is NOT thread safe on
 *               a database that other threads use: another thread's failure
 *               can change the string while you read it.
 *
 * Example:
 *    if (ns_execute(db, NULL, "create foo u32") < 0) {
 *      char *msg = (char *)ns_strerror(db, true);
 *      fprintf(stderr, "%s\n", msg);
 *      free(msg);
 *    }
 */

// The error of the last failed call on the database, from any thread
const char *ns_strerror (nsdb_t *ns, bool copy);

// A plan has no error of its own: this is ns_strerror on the plan's database
const char *ns_plan_strerror (nsdb_plan_t *plan, bool copy);

// The error of the last failed call that ran in [txn]. Use this from a
// thread that shares the database - no other thread can replace it.
//
// ns_commit and ns_rollback end [txn], also when they fail. Take their error
// with ns_strerror
const char *ns_txn_strerror (nsdb_t *ns, nsdb_txn_t *txn, bool copy);

//////////////////////////////////// Transactions

// Begin a transaction. NULL on failure - the error is on the database
nsdb_txn_t *ns_begin (nsdb_t *ns);

// Commit a transaction
int ns_commit (nsdb_t *ns, nsdb_txn_t *txn);

// Roll back a transaction
int ns_rollback (nsdb_t *ns, nsdb_txn_t *txn);

typedef void (*nsdb_var_consumer) (nsdb_var_t *v, void *ctx);
int ns_visit_nsdb_vars (nsdb_t *db, nsdb_var_consumer consumer, nsdb_txn_t *tx, void *ctx);

//////////////////////////////////// Numstore Execution patterns

/**
 * Create a query plan.
 *
 * Allow queries to be executed in two steps, to get
 * information about a plan and how it will execute.
 *
 * Plans must be free'd before the database is closed
 *
 * Example
 *  plan = ns_plan_fcreate(db, "insert foo 0 10");
 *  ns_plan_write(plan, tx, src, dlen);
 *  ns_plan_free(plan);
 *
 *  is equivalent to:
 *
 *  ns_write(db, tx, src, dlen, "insert foo 0 10");
 */
nsdb_plan_t *ns_plan_fcreate (nsdb_t *db, const char *fmt, ...);
void ns_plan_free (nsdb_plan_t *plan);

/**
 * Execute a single query.
 *
 * This function is for one time operations that
 * don't take in any data.
 *
 * The query type must be any query that doesn't take in any
 * extra parameters than those provided in the query string
 *
 * Example:
 *    ns_execute(db, tx, "create foo u32");
 *    ns_execute(db, tx, "remove foo[0:]");
 *
 * All of these examples need more parameters (source or destination
 * buffers)
 *    ns_execute(db, tx, "insert foo 0 10");       X FAILS
 *    ns_execute(db, tx, "read foo[0:10]");        X FAILS
 *    ns_execute(db, tx, "write foo[0:10]");       X FAILS
 */
int ns_execute (nsdb_t *db, nsdb_txn_t *tx, const char *fmt, ...);
int ns_plan_execute (nsdb_plan_t *plan, nsdb_txn_t *tx);

/**
 * Get the variable associated with a query
 *
 * This function gets the variable that's associated with a query
 *
 * The query type must contain some variable in it
 *
 * Example:
 *    nsdb_var_t* var = ns_get_var(db, tx, "get foo");
 *    nsdb_var_t* var = ns_get_var(db, tx, "insert foo[0:10]");
 *    nsdb_var_t* var = ns_get_var(db, tx, "delete foo");
 *
 * NULL is not always an error: "get if exists foo" returns NULL when there
 * is no foo. Check ns_strerror - it's NULL when nothing went wrong. That
 * only works if no earlier error is still pending, so take the error after
 * every call that fails
 *
 *    nsdb_var_t* var = ns_get_var(db, tx, "get if exists foo");
 *    if (var == NULL && ns_strerror(db, false) == NULL) { // no foo }
 */
nsdb_var_t *ns_get_var (nsdb_t *db, nsdb_txn_t *tx, const char *query, ...);
nsdb_var_t *ns_plan_get_var (nsdb_plan_t *plan, nsdb_txn_t *tx);

/**
 * Execute a query and read into a fixed sized buffer
 *
 * This function is for any query that pulls data from the database
 * into a byte buffer
 *
 * Must be a "readable" query (READ/REMOVE only)
 *
 * Example:
 *    u32 dest[10];
 *    sb_size read = ns_read(db, tx, dest, sizeof(dest), "read foo[0:10]");
 *    sb_size removed = ns_read(db, tx, dest, sizeof(dest), "remove foo[0:10]");
 *    sb_size len = ns_read(db, tx, dest, sizeof(dest), "insert foo 0 10");   X
 * FAILS sb_size len = ns_read(db, tx, dest, sizeof(dest), "get foo"); X FAILS
 */
sb_size ns_read (nsdb_t *db, nsdb_txn_t *txn, void *dest, b_size dlen, const char *fmt, ...);
sb_size ns_plan_read (nsdb_plan_t *plan, nsdb_txn_t *txn, void *dest, b_size dlen);

/**
 * Execute a query and write out of a fixed sized buffer
 *
 * This function is for any query that writes data from a source buffer
 * into the database
 *
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
sb_size ns_write (nsdb_t *db, nsdb_txn_t *txn, const void *src, b_size dlen, const char *fmt, ...);
sb_size ns_plan_write (nsdb_plan_t *plan, nsdb_txn_t *tx, const void *src, b_size dlen);

/**
 * Execute a query and malloc an output buffer
 *
 * This function is like ns_read, but instead of providing the
 * buffer in the function signature, it allocates (via system malloc)
 * the output buffer. You _must_ free this buffer using default system free
 * function
 *
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
void *ns_malloc (nsdb_t *db, nsdb_txn_t *txn, b_size *dlen, const char *fmt, ...);
void *ns_plan_malloc (nsdb_plan_t *plan, nsdb_txn_t *tx, b_size *dlen);

//////////////////////////////////// Smartfiles Execution patterns

/**
 * Returns the size of a smart file under the view of transaction [tx]
 */
sb_size ns_smfile_size (nsdb_t *smf, nsdb_txn_t *tx);

/**
 * Insert array [src] into index [bofst] into the smart file
 * insert doesn't care about [size], everything is in terms of
 * "bytes", not "elements"
 *
 * smf:         [0, 1, 2, 3, 4, 5]
 * src:         [6, 7, 8]
 * bofst:       3
 * slen:        sizeof(src) = 3
 *
 * insert(smf, tx, src, bofst, 3)
 *
 * smf:         [0, 1, 2, 6, 7, 8, 3, 4, 5]
 */
sb_size ns_smfile_insert (nsdb_t *smf, nsdb_txn_t *tx, const void *src, sb_size bofst, b_size slen);

/**
 * Overwrite array with [src] - optionally provide size > 1 byte to
 * indicate that the elements you're writing are bigger than 1 byte.
 * Specify stride to "skip" elements within the file
 *
 * smf:         (u32[])[0, 1, 2, 3, 4, 5]
 * src:         (u32[])[6, 7, 8]        an array
 * size:        4                       sizeof(u32) size of each "element" in
 * bytes bofst:       4                       starting _byte_ offset stride: 2
 * "elements" to skip each write nelem:       3 sizeof(src) / size
 *
 * write(smf, tx, src, bofst, stride, nelem)
 *
 * smf:         [0, 6, 2, 7, 4, 8]
 *
 * Errors:
 *  * If the combination of (src, size, bofst, stride, nelem) would go off the
 *    end of the file - TODO.
 */
sb_size ns_smfile_write (
    nsdb_t     *smf,
    nsdb_txn_t *tx,
    const void *src,
    t_size      size,
    sb_size     bofst,
    sb_size     stride,
    b_size      nelem
);

/**
 * Read into with [dest] - optionally provide size > 1 byte to
 * indicate that the elements you're writing are bigger than 1 byte.
 * Specify stride to "skip" elements within the file
 *
 * smf:         (u32[])[0, 1, 2, 3, 4, 5]
 * dest:        (u32[])[_, _, _]        an array
 * size:        4                       sizeof(u32) size of each "element" in
 * bytes bofst:       4                       starting _byte_ offset stride: 2
 * "elements" to skip each write nelem:       3 sizeof(src) / size
 *
 * read(smf, tx, src, bofst, stride, nelem)
 *
 * dest:        [1, 3, 5]
 *
 * Errors:
 *  * TODO
 */
sb_size ns_smfile_read (
    nsdb_t     *smf,
    nsdb_txn_t *tx,
    void       *dest,
    t_size      size,
    sb_size     bofst,
    sb_size     stride,
    b_size      nelem
);

/**
 * Remove data from the database and optionally write the removed data into
 * [dest] optionally provide size > 1 byte to indicate that the elements you're
 * writing are bigger than 1 byte. Specify stride to "skip" elements within the
 * file
 *
 * smf:         (u32[])[0, 1, 2, 3, 4, 5]
 * dest:        (u32[])[_, _, _]        an array
 * size:        4                       sizeof(u32) size of each "element" in
 * bytes bofst:       4                       starting _byte_ offset stride: 2
 * "elements" to skip each write nelem:       3 sizeof(src) / size
 *
 * remove(smf, tx, src, bofst, stride, nelem)
 *
 * smf:         (u32[])[0, 2, 4]
 * dest:        [1, 3, 5]
 *
 * Errors:
 *  * TODO
 */
sb_size ns_smfile_remove (
    nsdb_t     *smf,
    nsdb_txn_t *tx,
    void       *dest,
    t_size      size,
    sb_size     bofst,
    sb_size     stride,
    b_size      nelem
);

#endif
