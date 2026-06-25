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
typedef struct smfile   smfile_t;

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
 * SECTION: Smart Files
 * ----------------------------------------------------------------------------
 * @brief An api for ACID arrays of bytes (e.g. files)
 *
 * Simple vs Power API
 * ===================
 *
 * The simple API (smfile_read, smfile_write, smfile_remove, smfile_insert)
 * operates on the default variable and treats the file as a flat byte sequence.
 * That covers the vast majority of use cases.
 *
 * The power API (smfile_pread, smfile_pwrite, smfile_premove, smfile_pinsert)
 * adds two knobs: a named variable target, and element stride. Use it when you
 * need multiple independent variables in one file, or when you want to touch
 * every nth element without reading the rest into memory.
 *
 * Usage
 * =====
 *
 * <code>
 *  // Open a database
 *  smfile_t *smf = smfile_open ("sample1_crud");
 *  smfile_remove (smf, NULL, 0, SMF_END);
 *
 *  // Insert data at offset 0
 *  const char *initial = "The quick brown fox jumps over the lazy dog";
 *  smfile_insert (smf, initial, 0, strlen (initial));
 *
 *  // Read that data (all of it)
 *  sb_size n = smfile_read (smf, buf, 0, SMF_END);
 *  printf ("after insert:  \\"%.*s\\"\\n", (int)n, buf);
 *
 *  // Execute a transaction
 *  smfile_begin (smf);
 *  {
 *    // Insert data at offset 34
 *    const char *adverb = " really";
 *    smfile_insert (smf, adverb, 34, strlen (adverb));
 *
 *    n = smfile_read (smf, buf, 0, SMF_END);
 *    printf ("after insert:  \\"%.*s\\"\\n", (int)n, buf);
 *
 *    // Overwrite data at offset 16
 *    smfile_write (smf, "cat", 16, 3);
 *
 *    n = smfile_read (smf, buf, 0, SMF_END);
 *    printf ("after write:   \\"%.*s\\"\\n", (int)n, buf);
 *
 *    // Remove data starting at offset
 *    n = smfile_remove (smf, buf, 4, 6);
 *    printf ("removed:       \\"%.*s\\"\\n", (int)n, buf);
 *
 *    n = smfile_read (smf, buf, 0, SMF_END);
 *    printf ("after remove (inside txn):  \\"%.*s\\"\\n", (int)n, buf);
 *  }
 *  smfile_rollback (smf);
 *
 *  n = smfile_read (smf, buf, 0, SMF_END);
 *  printf ("after rollback:  \\"%.*s\\"\\n", (int)n, buf);
 *
 *  smfile_close (smf);
 * </code>
 *
 ******************************************************************************/

/**
 * @brief Opens a smart file at the given path. Unlike c FILE's all
 * smart files are in read write mode always
 *
 * @param path The file path of the smart file to open
 * @return A new smart file
 */
smfile_t *smfile_open (const char *path);

/**
 * @brief Cleans all resources associated with [path]
 *
 * @param path The file to clean up
 * @return < 0 on error 0 on success
 */
int smfile_cleanup (const char *path);

/**
 * @brief Smart files can be attached to only one transaction at a time.
 * In order to allow for concurrent smart file operations at the same time,
 * you can create new contexts from existing smart files. This allows you to
 * run multiple smart file operations at the same time on different transactions
 *
 * @param ns The existing smart file
 * @return A new "clone" of the smart file that has it's own transaction space
 */
smfile_t *smfile_new_context (smfile_t *ns);

/**
 * @brief Close a smart file. This releases all resources and terminates
 * any open transactions
 *
 * @return < 0 on error, 0 on success
 */
int smfile_close (smfile_t *ns);

/**
 * @brief Crash a smart file - this keeps the WAL alive
 * and enters recovery mode on next open
 *
 * @param ns
 * @return
 */
int smfile_crash (smfile_t *ns);

/**
 * @brief Similar to strerror. If any of the functions failed, you can
 * call this method to fetch the error string. If there are no errors, this
 * method returns NULL
 */
const char *smfile_strerror (smfile_t *ns);

/**
 * @brief Similar to perror.
 *
 * @param prefix The prefix to the perror string
 * @return forward fprintf errors
 */
int smfile_perror (smfile_t *ns, const char *prefix);

/**
 * @brief Delete a variable with the name vname
 *
 * @param vname The variable to delete
 * @return < 0 on error, 0 on success
 */
int smfile_delete (smfile_t *ns, const char *vname);

/**
 * @brief The size of a smfile
 * @return < 0 on error -> size on success
 */
sb_size smfile_size (smfile_t *smf);

/**
 * @brief Insert data into the middle of a smart file
 *
 * @param src The byte data to insert
 * @param bofst The byte offset in the file that you want to begin the insert.
 *              If negative, goes from the end.
 * @param slen The length in bytes of [src]
 * @return < 0 on error, 0 on success
 */
sb_size
smfile_insert (smfile_t *smf, const void *src, sb_size bofst, b_size slen);

/**
 * @brief Write elements into a smart file, overwriting existing data at that
 * location. Unlike fwrite, this operation is atomic — it either fully completes
 * or has no effect.
 *
 * @param src The data to write
 * @param bofst The byte offset in the file to begin writing at
 * @param nelem The number of elements to write
 * @return The number of elements written, or < 0 on error
 */
sb_size
smfile_write (smfile_t *smf, const void *src, b_size bofst, b_size nelem);

/**
 * @brief Read elements from a smart file into dest.
 * Reads up to nelem elements starting at byte offset bofst. Returns fewer
 * elements if the end of the file is reached before nelem elements are read.
 *
 * @param dest Buffer to read data into. Must be large enough to hold nelem
 * elements
 * @param bofst The byte offset in the file to begin reading from
 * @param nelem The number of elements to read
 * @return The number of elements read, or < 0 on error
 */
sb_size smfile_read (smfile_t *smf, void *dest, sb_size bofst, b_size nelem);

/**
 * @brief Remove elements from the middle of a smart file, closing the gap.
 * Unlike a write of zeroes, remove shrinks the file — bytes after the removed
 * region shift down. Optionally captures the removed data into dest if
 * non-NULL. This operation is atomic — it either fully completes or has no
 * effect.
 *
 * @param dest Buffer to capture the removed data into, or NULL to discard
 * @param bofst The byte offset in the file to begin removing from
 * @param nelem The number of elements to remove
 * @return The number of elements removed, or < 0 on error
 */
sb_size smfile_remove (smfile_t *smf, void *dest, sb_size bofst, b_size nelem);

/**
 * @brief Returns the size of an individual variable
 */
sb_size smfile_psize (smfile_t *smf, const char *vname);

/**
 * @brief [Power] Insert data into the middle of a named variable within a smart
 * file. Equivalent to smfile_insert but targets a specific named variable
 * rather than the default variable.
 *
 * @param name The name of the variable to insert into
 * @param src The byte data to insert
 * @param bofst The byte offset within the variable to begin the insert
 * @param slen The length in bytes of [src]
 * @return < 0 on error, 0 on success
 */
sb_size smfile_pinsert (
    smfile_t   *smf,
    const char *name,
    const void *src,
    sb_size     bofst,
    b_size      slen
);

/**
 * @brief [Power] Write elements into a named variable within a smart file,
 * with support for strided access. A stride of 1 means contiguous elements;
 * a stride of n means every nth element is written, leaving the elements
 * in between untouched.
 *
 * @param name The name of the variable to write into
 * @param src The data to write
 * @param size The size in bytes of a single element
 * @param bofst The byte offset within the variable to begin writing at
 * @param stride Element stride. 1 for contiguous, n to write every nth element
 * @param nelem The number of elements to write
 * @return The number of elements written, or < 0 on error
 */
sb_size smfile_pwrite (
    smfile_t   *smf,
    const char *name,
    const void *src,
    t_size      size,
    b_size      bofst,
    sb_size     stride,
    b_size      nelem
);

/**
 * @brief [Power] Read elements from a named variable within a smart file,
 * with support for strided access. A stride of 1 means contiguous elements;
 * a stride of n means every nth element is read, skipping the elements
 * in between.
 *
 * @param name The name of the variable to read from
 * @param dest Buffer to read data into. Must be large enough to hold nelem
 * elements
 * @param size The size in bytes of a single element
 * @param bofst The byte offset within the variable to begin reading from
 * @param stride Element stride. 1 for contiguous, n to read every nth element
 * @param nelem The number of elements to read
 * @return The number of elements read, or < 0 on error
 */
sb_size smfile_pread (
    smfile_t   *smf,
    const char *name,
    void       *dest,
    t_size      size,
    sb_size     bofst,
    sb_size     stride,
    b_size      nelem
);

/**
 * @brief [Power] Remove elements from a named variable within a smart file,
 * with support for strided access. A stride of 1 removes contiguous elements;
 * a stride of n removes every nth element, closing each gap independently.
 * Optionally captures the removed data into dest if non-NULL.
 * This operation is atomic — it either fully completes or has no effect.
 *
 * @param name The name of the variable to remove from
 * @param dest Buffer to capture the removed data into, or NULL to discard
 * @param size The size in bytes of a single element
 * @param bofst The byte offset within the variable to begin removing from
 * @param stride Element stride. 1 for contiguous, n to remove every nth element
 * @param nelem The number of elements to remove
 * @return The number of elements removed, or < 0 on error
 */
sb_size smfile_premove (
    smfile_t   *smf,
    const char *name,
    void       *dest,
    t_size      size,
    sb_size     bofst,
    sb_size     stride,
    b_size      nelem
);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Transaction Control
 *
 * Every simple and power operation is individually atomic by default. Wrapping
 * a sequence of operations in smfile_begin / smfile_commit promotes that group
 * to a single atomic unit backed by a write-ahead log and two-phase locking —
 * either every operation in the transaction lands, or none of them do.
 * smfile_rollback undoes all mutations since the last smfile_begin.
 *----------------------------------------------------------------------------*/

/**
 * @brief Begin a transaction. If smf is already apart of a transaction, this
 * method errors.
 *
 * @return < 0 on error, 0 on success
 */
int smfile_begin (smfile_t *smf);

/**
 * @brief Commit a transaction. If smf is not part of a transaction, this method
 * errors
 *
 * @return < 0 on error, 0 on success
 */
int smfile_commit (smfile_t *smf);

/**
 * @brief Rollback a transaction. If smf is not part of a transaction, this
 * method errors
 *
 * @return < 0 on error, 0 on success
 */
int smfile_rollback (smfile_t *smf);

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

// Printf attribute
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
