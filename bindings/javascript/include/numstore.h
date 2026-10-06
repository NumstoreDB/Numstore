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
typedef struct txn       txn_t;
typedef struct nsdb_var  nsdb_var_t;
typedef struct nsdb_plan nsdb_plan_t;

/**
 * Type aliases. Most "things" in numstore
 * are configurable.
 *
 * For example:
 *  - A transaction id is a 64 bit int
 *  - A page number is a 64 bit int
 *  - The number of bytes you need to represent the location
 *    within a variable type is 4 bytes
 *  - The number of bytes you need to represent the location
 *    within a variable data is 8 bytes
 *  - ...
 */
#ifndef NS_TYPE_ALIASES

// Size of each page in the database
#  define NS_PAGE_SIZE 4096

// Number of page slots open in the
// pager buffer pool
#  define MEMORY_PAGE_LEN 4096

// The maximum bytes the WAL can hold before
// needing to flush due to size constraints
#  define WAL_BUFFER_CAP 1048576

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

// A page that doesn't exist
#  define PGNO_NULL U64_MAX

// A log sequence number that doesn't exist
#  define LSN_NULL U64_MAX

// The last wal log header
#  define WLH_NULL U8_MAX

// Printf formats
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

// The private ns_stdtypes.h carries the same block. Claiming the guard here
// stops it defining everything a second time when both headers are in one
// translation unit.
#  define NS_TYPE_ALIASES

#endif

//////////////////////////////////// Lifecycle

// Open a Numstore database
nsdb_t *ns_open (const char *path);

// Open a Smart file
nsdb_t *ns_smfile_open (const char *path);

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

// Get the error string of what just happened
const char *ns_strerror (nsdb_t *ns);

// Get the error string of what just happened to a plan
const char *ns_plan_strerror (nsdb_plan_t *plan);

//////////////////////////////////// Transactions

// Begin a transaction
txn_t *ns_begin (nsdb_t *ns);

// Commit a transaction
int ns_commit (nsdb_t *ns, txn_t *txn);

// Roll back a transaction
int ns_rollback (nsdb_t *ns, txn_t *txn);

//////////////////////////////////// Numstore Execution patterns

nsdb_plan_t *ns_plan_fcreate (nsdb_t *db, const char *fmt, ...);
void ns_plan_free (nsdb_plan_t *plan);

int ns_execute (nsdb_t *db, struct txn *tx, const char *fmt, ...);
int ns_plan_execute (nsdb_plan_t *plan, struct txn *tx);

nsdb_var_t *ns_get_var (nsdb_t *db, struct txn *tx, const char *query, ...);
nsdb_var_t *ns_plan_get_var (nsdb_plan_t *plan, struct txn *tx);

sb_size ns_read (nsdb_t *db, txn_t *txn, void *dest, b_size dlen, const char *fmt, ...);
sb_size ns_plan_read (nsdb_plan_t *plan, txn_t *txn, void *dest, b_size dlen);

sb_size ns_write (nsdb_t *db, txn_t *txn, const void *src, b_size dlen, const char *fmt, ...);
sb_size ns_plan_write (nsdb_plan_t *plan, txn_t *tx, const void *src, b_size dlen);

void *ns_malloc (nsdb_t *db, txn_t *txn, b_size *dlen, const char *fmt, ...);
void *ns_plan_malloc (nsdb_plan_t *plan, txn_t *tx, b_size *dlen);

//////////////////////////////////// Smartfiles Execution patterns

sb_size ns_smfile_size (nsdb_t *smf, txn_t *tx);

sb_size ns_smfile_insert (nsdb_t *smf, txn_t *tx, const void *src, sb_size bofst, b_size slen);

sb_size ns_smfile_write (
    nsdb_t     *smf,
    txn_t      *tx,
    const void *src,
    t_size      size,
    sb_size     bofst,
    sb_size     stride,
    b_size      nelem
);

sb_size ns_smfile_read (
    nsdb_t *smf,
    txn_t  *tx,
    void   *dest,
    t_size  size,
    sb_size bofst,
    sb_size stride,
    b_size  nelem
);

sb_size ns_smfile_remove (
    nsdb_t *smf,
    txn_t  *tx,
    void   *dest,
    t_size  size,
    sb_size bofst,
    sb_size stride,
    b_size  nelem
);

#endif
