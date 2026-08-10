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

#ifndef SMARTFILES_H
#define SMARTFILES_H

#include <inttypes.h>
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

typedef struct smfile smfile_t;

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
 * SECTION: Smart Files
 * ----------------------------------------------------------------------------
 * @brief An api for ACID arrays of bytes (e.g. files)
 ******************************************************************************/

smfile_t *smfile_open (const char *path);
int       smfile_cleanup (const char *path);
int       smfile_close (smfile_t *ns);
int       smfile_crash (smfile_t *ns);

const char *smfile_strerror (smfile_t *ns);
int         smfile_perror (smfile_t *ns, const char *prefix);
sb_size     smfile_size (smfile_t *smf);

int smfile_begin (smfile_t *smf);
int smfile_commit (smfile_t *smf);
int smfile_rollback (smfile_t *smf);

sb_size smfile_insert (smfile_t *smf, const void *src, sb_size bofst, b_size slen);

sb_size smfile_write (
    smfile_t   *smf,
    const void *src,
    t_size      size,
    b_size      bofst,
    sb_size     stride,
    b_size      nelem
);

sb_size smfile_read (
    smfile_t *smf,
    void     *dest,
    t_size    size,
    sb_size   bofst,
    sb_size   stride,
    b_size    nelem
);

sb_size smfile_remove (
    smfile_t *smf,
    void     *dest,
    t_size    size,
    sb_size   bofst,
    sb_size   stride,
    b_size    nelem
);

#endif
