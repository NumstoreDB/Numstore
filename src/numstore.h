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

#pragma once

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

/******************************************************************************
 * SECTION: Compiler specified constants
 * ----------------------------------------------------------------------------
 * @brief Pass compiler flags to override these constants
 ******************************************************************************/

// Versions
#ifndef NS_VERSION_MAJOR
#  define NS_VERSION_MAJOR 0
#endif
#ifndef NS_VERSION_MINOR
#  define NS_VERSION_MINOR 0
#endif
#ifndef NS_VERSION_PATCH
#  define NS_VERSION_PATCH 0
#endif
#ifndef NS_VERSION_STRING
#  define NS_VERSION_STRING "0.0.0"
#endif

// Compile Time Options
#ifndef NS_PAGE_SIZE
#  define NS_PAGE_SIZE 4096
#endif
#ifndef MEMORY_PAGE_LEN
#  define MEMORY_PAGE_LEN 4096
#endif
#ifndef WAL_BUFFER_CAP
#  define WAL_BUFFER_CAP 1048576
#endif
#ifndef MAX_OPEN_FILES
#  define MAX_OPEN_FILES 20
#endif

/******************************************************************************
 * SECTION: Opaque Types and constants
 * ----------------------------------------------------------------------------
 * @brief Opaque handles and types to pass into numstore functions
 ******************************************************************************/

typedef struct nsdb     nsdb_t;
typedef struct nsdb_var nsdb_var_t;

#define NS_END  INT64_MAX
#define SMF_END INT64_MAX

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

#define PGNO_NULL U64_MAX
#define LSN_NULL  U64_MAX
#define WLH_NULL  U8_MAX

#define PRt_size  PRIu32
#define PRst_size PRId32
#define PRp_size  PRIu32
#define PRsp_size PRId32
#define PRb_size  PRIu64
#define PRsb_size PRId64
#define PRpgno    PRIu64
#define PRspgno   PRId64
#define PRtxid    PRIu64
#define PRstxid   PRId64
#define PRlsn     PRIu64
#define PRslsn    PRId64
#define PRpgh     PRIu8
#define PRwlh     PRIu8

/******************************************************************************
 * SECTION: Numstore
 * ----------------------------------------------------------------------------
 * @brief A database for numerical arrays
 *
 *
 *
 * Numstore is built on top of the smart file primitives above. Where smart
 * files expose untyped byte ropes, numstore exposes typed numerical arrays
 * driven by a query string. The lifecycle (open / close / crash), error
 * reporting, and transaction control mirror the smfile API one-to-one; the
 * heavy lifting is done through nsdb_execute, which parses a query and
 * dispatches to the appropriate read / write / insert / remove path.
 *
 * Usage
 * =====
 * <code>
 *  // Open up a database
 *  nsdb_t *ns = nsdb_open ("sample1_crud");
 *
 *  // Create a typed variable
 *  nsdb_execute (ns, "delete if exists example", NULL);
 *  nsdb_execute (ns, "create example struct { a f32, b [5][10] i32 }", NULL);
 *
 *  // Insert 200 elements of seed data at offset 0
 *  int n = nsdb_execute (ns, "insert example 0 %d", src, 200);
 *
 *  // Begin a transaction - mutations rolled back at the end
 *  nsdb_begin (ns);
 *  {
 *    // Read every 3rd element
 *    n = nsdb_execute (ns, "read example[0::3]", dest);
 *    print_example ("Every 3rd element", dest, n);
 *
 *    // Remove every 2nd element up to len - 10
 *    n = nsdb_execute (ns, "remove example[0:-10:2]", dest);
 *    print_example ("Removed Elements [0:-10:2]", dest, n);
 *
 *    // Overwrite every 2nd element with src
 *    nsdb_execute (ns, "write example[1::2]", src);
 *
 *    // Read all
 *    n = nsdb_execute (ns, "read example[0:]", dest);
 *    print_example ("Data After Write [1::2]", dest, n);
 *  }
 *  nsdb_rollback (ns);
 *
 *  // Read all after rollback
 *  n = nsdb_execute (ns, "read example[0:]", dest);
 *  print_example ("Data After Rollback", dest, n);
 *
 *  return nsdb_close (ns);
 * </code>
 * ******************************************************************************/

/*-----------------------------------------------------------------------------
 * SUBSECTION: Lifecycle
 *----------------------------------------------------------------------------*/

/**
 * @brief Opens a numstore database at the given path. The database file is
 * created if it does not already exist. Databases are always opened in
 * read-write mode.
 *
 * @param path The file path of the database to open
 * @return A new database handle, or NULL on error
 */
nsdb_t *nsdb_open (const char *path);

/**
 * @brief Cleans all resources associated with [path]
 *
 * @param path The database to clean up
 * @return < 0 on error 0 on success
 */
int nsdb_cleanup (const char *path);

/**
 * @brief Close a database. This releases all resources and terminates
 * any open transactions
 *
 * @return < 0 on error, 0 on success
 */
int nsdb_close (nsdb_t *ns);

/**
 * @brief Crash a database - this keeps the WAL alive
 * and enters recovery mode on next open
 *
 * @param ns
 * @return
 */
int nsdb_crash (nsdb_t *ns);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Variable handles
 *
 * Some queries produced by nsdb_execute return an nsdb_var_t handle into the
 * data buffer rather than copying every element out. These helpers query
 * and release such handles.
 *----------------------------------------------------------------------------*/

/**
 * @brief Returns the number of elements addressable through a variable handle.
 *
 * @param var The variable handle
 * @return The number of elements in the variable
 */
b_size nsdb_var_len (nsdb_var_t *var);

/**
 * @brief Release a variable handle and any resources it owns. Safe to call
 * with NULL.
 *
 * @param var The variable handle to free
 */
void nsdb_var_free (nsdb_var_t *var);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Error reporting
 *----------------------------------------------------------------------------*/

/**
 * @brief Similar to strerror. If any of the functions failed, you can
 * call this method to fetch the error string. If there are no errors, this
 * method returns NULL
 */
const char *nsdb_strerror (nsdb_t *ns);

/**
 * @brief Similar to perror.
 *
 * @param prefix The prefix to the perror string
 * @return forward fprintf errors
 */
int nsdb_perror (nsdb_t *ns, const char *prefix);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Transaction Control
 *
 * Every nsdb_execute call is individually atomic by default. Wrapping a
 * sequence of calls in nsdb_begin / nsdb_commit promotes that group to a
 * single atomic unit backed by the same WAL + two-phase locking used by
 * smart files — either every query in the transaction lands, or none of
 * them do. nsdb_rollback undoes all mutations since the last nsdb_begin.
 *----------------------------------------------------------------------------*/

/**
 * @brief Begin a transaction. If ns is already apart of a transaction, this
 * method errors.
 *
 * @return < 0 on error, 0 on success
 */
int nsdb_begin (nsdb_t *ns);

/**
 * @brief Commit a transaction. If ns is not part of a transaction, this method
 * errors
 *
 * @return < 0 on error, 0 on success
 */
int nsdb_commit (nsdb_t *ns);

/**
 * @brief Rollback a transaction. If ns is not part of a transaction, this
 * method errors
 *
 * @return < 0 on error, 0 on success
 */
int nsdb_rollback (nsdb_t *ns);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Query execution
 *----------------------------------------------------------------------------*/

#if defined(__GNUC__) || defined(__clang__)
#  define NSDB_PRINTF(fmt_idx, vargs_idx) \
    __attribute__ ((format (printf, fmt_idx, vargs_idx)))
#else
#  define NSDB_PRINTF(fmt_idx, vargs_idx)
#endif

/**
 * @brief The main entry point to numstore
 *
 * @param ns The database handle
 * @param query_fmt Your query (as a format string)
 * @param data A buffer that is accepted for insert / read / write / remove
 *
 * @return < 0 on error - otherwise 0 on non data related queries or > 0 number
 * of elements inserted / written / removed / read for each op code respectively
 */
sb_size
nsdb_execute (nsdb_t *ns, const char *query_fmt, void *data, ...) NSDB_PRINTF (
    2,
    4
);
