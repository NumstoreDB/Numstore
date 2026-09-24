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
#include <stddef.h>
#include <stdint.h>

#if defined(__GNUC__) || defined(__clang__)
#  define NSDB_PRINTF(fmt_idx, vargs_idx) __attribute__ ((format (printf, fmt_idx, vargs_idx)))
#else
#  define NSDB_PRINTF(fmt_idx, vargs_idx)
#endif

typedef struct nsdb     nsdb_t;
typedef struct txn      txn_t;
typedef struct nsdb_var nsdb_var_t;

#ifndef NS_TYPE_ALIASES

#  define NS_PAGE_SIZE    4096
#  define MEMORY_PAGE_LEN 4096
#  define WAL_BUFFER_CAP  1048576
#  define MAX_OPEN_FILES  20

#  define NS_END  INT64_MAX
#  define SMF_END INT64_MAX

typedef uint32_t t_size;  // Represents the size of a single type in bytes
typedef int32_t  st_size; // Signed t_size
typedef uint32_t p_size;  // To index inside a page
typedef int32_t  sp_size; // Signed p_size
typedef uint64_t b_size;  // Bytes size to index into a contiguous rope bytes
typedef int64_t  sb_size; // Signed b_size
typedef uint64_t pgno;    // Page number
typedef int64_t  spgno;   // Signed page number
typedef uint64_t txid;    // Transaction id
typedef int64_t  stxid;   // Signed transaction id
typedef int64_t  slsn;    // Wall index (often called LSN)
typedef uint64_t lsn;     // Wall index (often called LSN)
typedef uint8_t  pgh;     // Page header
typedef uint8_t  wlh;     // WAL header

#  define PGNO_NULL U64_MAX
#  define LSN_NULL  U64_MAX
#  define WLH_NULL  U8_MAX

#  define PRt_size  PRIu32
#  define PRst_size PRId32
#  define PRp_size  PRIu32
#  define PRsp_size PRId32
#  define PRb_size  PRIu64
#  define PRsb_size PRId64
#  define PRpgno    PRIu64
#  define PRspgno   PRId64
#  define PRtxid    PRIu64
#  define PRstxid   PRId64
#  define PRlsn     PRIu64
#  define PRslsn    PRId64
#  define PRpgh     PRIu8
#  define PRwlh     PRIu8

#endif

// Lifecycle
nsdb_t *ns_open (const char *path);
int ns_cleanup (const char *path);
int ns_close (nsdb_t *ns);
int ns_crash (nsdb_t *ns);

// Variables
b_size ns_var_len (nsdb_var_t *var);
void ns_var_free (nsdb_var_t *var);

// Errors
const char *ns_strerror (nsdb_t *ns);
int ns_perror (nsdb_t *ns, const char *prefix);

// Transactions
txn_t *ns_begin (nsdb_t *ns);
int ns_commit (nsdb_t *ns, txn_t *txn);
int ns_rollback (nsdb_t *ns, txn_t *txn);

// Execution

/**
 * Execute a single query.
 * Must be any query that doesn't take in parameters
 *
 * Example:
 *    ns_exec(db, tx, "create foo u32");
 *    ns_exec(db, tx, "remove foo[0:]");
 *    ns_exec(db, tx, "insert foo 0 10");       X FAILS
 *    ns_exec(db, tx, "read foo[0:10]");        X FAILS
 *    ns_exec(db, tx, "write foo[0:10]");       X FAILS
 */
int ns_exec (nsdb_t *db, struct txn *tx, const char *fmt, ...);

/**
 * Get the variable associated with a query
 * Doesn't every actually execute anything
 *
 * Example:
 *    nsdb_var_t* var = ns_get_var(db, tx, "get foo");
 *    nsdb_var_t* var = ns_get_var(db, tx, "insert foo[0:10]");
 *    nsdb_var_t* var = ns_get_var(db, tx, "delete foo");
 */
nsdb_var_t *ns_get_var (nsdb_t *db, struct txn *tx, const char *query, ...);

/**
 * Execute a query and read into a fixed sized buffer
 * Must be a "readable" query (READ/REMOVE only)
 *
 * Example:
 *    u32 dest[10];
 *    sb_size read = ns_read(db, tx, dest, sizeof(dest), "read foo[0:10]");
 *    sb_size removed = ns_read(db, tx, dest, sizeof(dest), "remove foo[0:10]");
 *    sb_size len = ns_read(db, tx, dest, sizeof(dest), "insert foo 0 10");   X FAILS
 *    sb_size len = ns_read(db, tx, dest, sizeof(dest), "get foo");           X FAILS
 */
sb_size ns_read (nsdb_t *db, txn_t *txn, void *dest, b_size dlen, const char *fmt, ...);

/**
 * Execute a query and malloc an output buffer
 * Must be a "readable" query (READ/REMOVE only)
 *
 * Example:
 *    b_size len;
 *    void* data = ns_read_malloc(db, tx, &len, "read foo[0:10]");
 *    void* data = ns_read_malloc(db, tx, &len, "remove foo[0:10]");
 *    void* data = ns_read_malloc(db, tx, &len, "remove foo[0:10]");
 *    void* data = ns_read_malloc(db, tx, &len, "insert foo 0 10");   X FAILS
 *    void* data = ns_read_malloc(db, tx, &len, "delete foo");        X FAILS
 */
void *ns_read_malloc (nsdb_t *db, txn_t *txn, b_size *dlen, const char *fmt, ...);

/**
 * Execute a query and write out of a fixed sized buffer
 * Must be a "writable" query (INSERT/WRITE only)
 *
 * Example:
 *    b_size len;
 *    void* data = ns_read_malloc(db, tx, &len, "read foo[0:10]");
 *    void* data = ns_read_malloc(db, tx, &len, "remove foo[0:10]");
 *    void* data = ns_read_malloc(db, tx, &len, "remove foo[0:10]");
 *    void* data = ns_read_malloc(db, tx, &len, "insert foo 0 10");   X FAILS
 *    void* data = ns_read_malloc(db, tx, &len, "delete foo");        X FAILS
 */
sb_size ns_write (nsdb_t *db, txn_t *txn, const void *src, b_size dlen, const char *fmt, ...);

#endif
