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

/******************************************************************************
 * SECTION: Compiler specified constants
 * ----------------------------------------------------------------------------
 * @brief Pass compiler flags to override these constants
 ******************************************************************************/

#if defined(__GNUC__) || defined(__clang__)
#  define NSDB_PRINTF(fmt_idx, vargs_idx) __attribute__ ((format (printf, fmt_idx, vargs_idx)))
#else
#  define NSDB_PRINTF(fmt_idx, vargs_idx)
#endif

/******************************************************************************
 * SECTION: Opaque Types and constants
 * ----------------------------------------------------------------------------
 * @brief Opaque handles and types to pass into numstore functions
 ******************************************************************************/

typedef struct numstore     numstore_t;
typedef struct ns_txn       ns_txn_t;
typedef struct numstore_var numstore_var_t;

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

/******************************************************************************
 * SECTION: Numstore
 * ----------------------------------------------------------------------------
 * @brief A database for numerical arrays
 * ******************************************************************************/

// Lifecycle
numstore_t *numstore_open (const char *path);
int numstore_cleanup (const char *path);
int numstore_close (numstore_t *ns);
int numstore_crash (numstore_t *ns);

// Variables
//
// A captured variable owns its own arena, so every accessor below stays valid
// until numstore_var_free. numstore_var_name and numstore_var_type write into
// `dest` and return the length written, or a negative error code when `dest`
// is too small - pass NULL/0 to ask for the size first.
b_size numstore_var_len (numstore_var_t *var);
t_size numstore_var_tsize (numstore_var_t *var);
sb_size numstore_var_name (numstore_var_t *var, char *dest, size_t size);
sb_size numstore_var_type (numstore_var_t *var, char *dest, size_t size);
void numstore_var_free (numstore_var_t *var);

// Errors
const char *numstore_strerror (numstore_t *ns);
int numstore_perror (numstore_t *ns, const char *prefix);

// Transactions
ns_txn_t *numstore_begin (numstore_t *ns);
int numstore_commit (numstore_t *ns, ns_txn_t *txn);
int numstore_rollback (numstore_t *ns, ns_txn_t *txn);

typedef enum
{
  NSDB_PLAN_OPT_NONE          = 0,
  NSDB_PLAN_OPT_ALLOCATE_DATA = 1u << 0,
  NSDB_PLAN_OPT_CAPTURE_VAR   = 1u << 1,
} numstore_plan_opt_t;

// Build one of these directly at the call site - a plan is plain data:
//
//   struct numstore_plan plan = {.data = buf, .dlen = sizeof (buf)};
//   struct numstore_plan plan = {.options = NSDB_PLAN_OPT_CAPTURE_VAR};
//
// `data`/`dlen` must be NULL/0 under NSDB_PLAN_OPT_ALLOCATE_DATA and `var`
// must be NULL under NSDB_PLAN_OPT_CAPTURE_VAR; execute rejects the plan
// otherwise. On return, an allocated `data` and a captured `var` both belong
// to the caller (free them with i_free and numstore_var_free).
struct numstore_plan
{
  void           *data;
  b_size          dlen;
  numstore_var_t *var;
  uint32_t        options;
};

// Executes a data operation
sb_size numstore_fexecute (
    numstore_t           *ns,
    ns_txn_t             *txn,
    struct numstore_plan *plan,
    const char           *query_fmt,
    ...
) NSDB_PRINTF (4, 5);

sb_size numstore_vexecute (
    numstore_t           *ns,
    ns_txn_t             *txn,
    struct numstore_plan *plan,
    const char           *query_fmt,
    va_list               args
);

#endif
