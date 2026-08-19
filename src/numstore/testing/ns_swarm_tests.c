
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

#include "numstore/testing/ns_swarm_tests.h"

#include "core/ns_alloc.h"
#include "core/ns_block_array.h"
#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_logging.h"
#include "core/ns_numerics.h"
#include "core/ns_platform.h"
#include "core/ns_stride.h"
#include "core/ns_string.h"
#include "nscore/compiler/ns_compiler.h"
#include "nscore/ns_mem_vhmap.h"
#include "nscore/ns_variables.h"
#include "nscore/types/ns_types.h"

#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/******************************************************************************
 * SECTION: Diagnostics
 * ----------------------------------------------------------------------------
 * @brief Assertion macros, action-name table, and state dumper used by every
 *        failure path in this fixture.
 ******************************************************************************/

static const char *const irwr_action_names[IRWR_AT_LEN] = {
    [IRWR_BEGIN_TXN]        = "BEGIN_TXN",
    [IRWR_COMMIT_TXN]       = "COMMIT_TXN",
    [IRWR_ROLLBACK_TXN]     = "ROLLBACK_TXN",
    [IRWR_CRASH_AND_REOPEN] = "CRASH_AND_REOPEN",
    [IRWR_CLOSE_AND_REOPEN] = "CLOSE_AND_REOPEN",
    [IRWR_INSERT]           = "INSERT",
    [IRWR_REMOVE]           = "REMOVE",
    [IRWR_READ]             = "READ",
    [IRWR_WRITE]            = "WRITE",
};

/**
 * Forward decl so the assert macros can dump state on failure if the caller
 * passes a meta pointer via IRWR_SWMT_ASSERT_STATE.
 */
static void irwr_swmt_print_state (const struct irwr_swarm_test *meta);

/**
 * Bare assertion: prints the failing expression and source location.
 * Use when there's no extra context worth printing.
 */
#define IRWR_SWMT_ASSERT(expr)                                                 \
  do {                                                                         \
    if (!(expr)) {                                                             \
      i_log_failure ("IRWR swarm test FAILED\n");                              \
      i_log_failure ("  expr: %s\n", #expr);                                   \
      i_log_failure ("  loc:  %s:%d in %s()\n", __FILE__, __LINE__, __func__); \
      panic ("Aborting early\n");                                              \
    }                                                                          \
  }                                                                            \
  while (0)

/**
 * Formatted assertion: same as IRWR_SWMT_ASSERT, plus a printf-style
 * context line. Prefer this anywhere you have useful runtime state
 * (offsets, lengths, names, return codes).
 */
#define IRWR_SWMT_ASSERTF(expr, fmt, ...)                                      \
  do {                                                                         \
    if (!(expr)) {                                                             \
      i_log_failure ("IRWR swarm test FAILED\n");                              \
      i_log_failure ("  expr: %s\n", #expr);                                   \
      i_log_failure ("  loc:  %s:%d in %s()\n", __FILE__, __LINE__, __func__); \
      i_log_failure ("  why:  " fmt "\n", __VA_ARGS__);                        \
      panic ("Aborting early\n");                                              \
    }                                                                          \
  }                                                                            \
  while (0)

/**
 * Same as IRWR_SWMT_ASSERTF but also dumps full fixture state before
 * panicking. Use at the high-impact integrity checks (cross-checks,
 * txn boundaries) where the surrounding state matters for triage.
 */
#define IRWR_SWMT_ASSERT_STATE(meta, expr, fmt, ...)                           \
  do {                                                                         \
    if (!(expr)) {                                                             \
      i_log_failure ("IRWR swarm test FAILED\n");                              \
      i_log_failure ("  expr: %s\n", #expr);                                   \
      i_log_failure ("  loc:  %s:%d in %s()\n", __FILE__, __LINE__, __func__); \
      i_log_failure ("  why:  " fmt "\n", __VA_ARGS__);                        \
      irwr_swmt_print_state (meta);                                            \
      panic ("Aborting early\n");                                              \
    }                                                                          \
  }                                                                            \
  while (0)

/**
 * Dump the entire fixture state in a human-readable form. Safe to call
 * with NULL.
 */
static void
irwr_swmt_print_state (const struct irwr_swarm_test *meta)
{
  i_log_info ("=== IRWR Swarm Test State ===\n");
  if (meta == NULL) {
    i_log_info ("  (meta is NULL)\n");
    return;
  }
  i_log_info ("  dbname:         %s\n", meta->dbname);
  i_log_info ("  varname:        %s\n", meta->varname);
  i_log_info ("  vartype:        %s\n", meta->vartype);
  i_log_info ("  esize:          %u bytes/elem\n", (unsigned)meta->esize);
  i_log_info ("  len:            %" PRb_size " elems\n", meta->len);
  i_log_info ("  max_insert_len: %d\n", meta->max_insert_len);
  i_log_info ("  in_txn:         %s\n", meta->in_txn ? "yes" : "no");
  i_log_info ("  committed:      %s\n", meta->committed ? "present" : "<null>");
  i_log_info ("  working:        %s\n", meta->working ? "present" : "<null>");
  i_log_info ("  sample_space_p: %.3f\n", (double)meta->sample_space_prob);
  i_log_info ("  Actions             enabled   allowed\n");
  for (int i = 0; i < IRWR_AT_LEN; ++i) {
    i_log_info (
        "    %-18s   %-3s       %-3s\n",
        irwr_action_names[i],
        meta->enabled[i] ? "yes" : "no",
        meta->allowed[i] ? "yes" : "no"
    );
  }
}

/******************************************************************************
 * SECTION: Utilities
 ******************************************************************************/

static struct block_array *
active_db (struct irwr_swarm_test *meta)
{
  return meta->in_txn ? meta->working : meta->committed;
}

static void
irwr_swmt_random_slice (int total, int *ofst, int *stride, int *len)
{
  ASSERT (total > 0);

  *ofst         = randu32r (0, total - 1);
  int remaining = total - *ofst;
  *stride       = randu32r (1, remaining);

  int max_len   = (remaining + *stride - 1) / *stride;
  *len          = randu32r (1, max_len);
}

static struct stride
to_block_stride (int ofst, int stride, int len)
{
  return (struct stride){
      .start  = (u64)ofst,
      .stride = (u64)stride,
      .nelems = (u64)len,
  };
}

static void
irwr_swmt_set_random_enabled (struct irwr_swarm_test *meta)
{
  int mask = rand () % ((1 << IRWR_AT_LEN) - 1) + 1;
  for (int i = 0; i < IRWR_AT_LEN; ++i) {
    meta->enabled[i] = (mask >> i) & 1;
  }
}

static void
irwr_swmt_set_allowed (struct irwr_swarm_test *meta)
{
  ASSERT (meta);
  ASSERT (meta->db);
  ASSERT (meta->dbname);

  memset (meta->allowed, 0, sizeof (meta->allowed));

  meta->allowed[IRWR_CRASH_AND_REOPEN] = meta->enabled[IRWR_CRASH_AND_REOPEN];

  if (!meta->in_txn) {
    meta->allowed[IRWR_BEGIN_TXN]        = meta->enabled[IRWR_BEGIN_TXN];
    meta->allowed[IRWR_CLOSE_AND_REOPEN] = meta->enabled[IRWR_CLOSE_AND_REOPEN];
  } else {
    meta->allowed[IRWR_COMMIT_TXN]   = meta->enabled[IRWR_COMMIT_TXN];
    meta->allowed[IRWR_ROLLBACK_TXN] = meta->enabled[IRWR_ROLLBACK_TXN];
  }

  meta->allowed[IRWR_INSERT] = meta->enabled[IRWR_INSERT];
  if (meta->len > 0) {
    meta->allowed[IRWR_REMOVE] = meta->enabled[IRWR_REMOVE];
    meta->allowed[IRWR_READ]   = meta->enabled[IRWR_READ];
    meta->allowed[IRWR_WRITE]  = meta->enabled[IRWR_WRITE];
  }
}

/******************************************************************************
 * SECTION: Concrete Actions
 ******************************************************************************/

static void
irwr_swmt_begin_txn (struct irwr_swarm_test *meta)
{
  ASSERT (!meta->in_txn);
  ASSERT (meta->working == NULL);

  IRWR_SWMT_ASSERTF (nsdb_begin (meta->db) == 0, "nsdb_begin failed on db='%s'", meta->dbname);

  meta->working = block_array_clone (meta->committed, NULL);
  IRWR_SWMT_ASSERTF (
      meta->working != NULL,
      "block_array_clone returned NULL (committed len=%llu)",
      (unsigned long long)block_array_getlen (meta->committed)
  );
  meta->in_txn = 1;
}

static void
irwr_swmt_commit_txn (struct irwr_swarm_test *meta)
{
  ASSERT (meta->in_txn);
  ASSERT (meta->working != NULL);

  IRWR_SWMT_ASSERTF (
      nsdb_commit (meta->db) == 0,
      "nsdb_commit failed on db='%s' (len=%" PRb_size ")",
      meta->dbname,
      meta->len
  );

  block_array_free (meta->committed);
  meta->committed = meta->working;
  meta->working   = NULL;
  meta->in_txn    = 0;
}

static void
irwr_swmt_rollback_txn (struct irwr_swarm_test *meta)
{
  ASSERT (meta->in_txn);

  IRWR_SWMT_ASSERTF (
      nsdb_rollback (meta->db) == 0,
      "nsdb_rollback failed on db='%s'",
      meta->dbname
  );

  block_array_free (meta->working);
  meta->working = NULL;
  meta->in_txn  = 0;
  meta->len     = block_array_getlen (meta->committed) / (u64)meta->esize;
}

static void
irwr_swmt_crash_and_reopen (struct irwr_swarm_test *meta)
{
  IRWR_SWMT_ASSERTF (nsdb_crash (meta->db) == 0, "nsdb_crash failed on db='%s'", meta->dbname);

  meta->db = nsdb_open (meta->dbname);
  IRWR_SWMT_ASSERTF (
      meta->db != NULL,
      "nsdb_open after crash returned NULL (db='%s')",
      meta->dbname
  );

  if (meta->working) {
    block_array_free (meta->working);
    meta->working = NULL;
  }
  meta->in_txn = 0;
  meta->len    = block_array_getlen (meta->committed) / (u64)meta->esize;
}

static void
irwr_swmt_close_and_reopen (struct irwr_swarm_test *meta)
{
  ASSERT (!meta->in_txn);
  IRWR_SWMT_ASSERTF (nsdb_close (meta->db) == 0, "nsdb_close failed on db='%s'", meta->dbname);

  meta->db = nsdb_open (meta->dbname);
  IRWR_SWMT_ASSERTF (
      meta->db != NULL,
      "nsdb_open after close returned NULL (db='%s')",
      meta->dbname
  );
}

static void
irwr_swmt_insert (struct irwr_swarm_test *meta)
{
  int      len  = (rand () % meta->max_insert_len) + 1;
  int      ofst = rand () % (meta->len + 1);

  int      blen = len * (int)meta->esize;
  uint8_t *data = malloc ((size_t)blen);
  IRWR_SWMT_ASSERTF (
      data != NULL,
      "malloc(%d) failed in insert (len=%d esize=%u)",
      blen,
      len,
      (unsigned)meta->esize
  );
  for (int i = 0; i < blen; ++i) {
    data[i] = (uint8_t)rand ();
  }

  /* DB side (NOTE: was previously executed twice - collapsed to one call). */
  int got = nsdb_fexecute (meta->db, "insert %s %d %d", data, meta->varname, ofst, len);
  IRWR_SWMT_ASSERT_STATE (
      meta,
      got == len,
      "nsdb insert returned %d, expected %d "
      "(var='%s' ofst=%d len=%d)",
      got,
      len,
      meta->varname,
      ofst,
      len
  );

  /* Reference side */
  int ba =
      block_array_insert (active_db (meta), (u32)(ofst * (int)meta->esize), data, (u32)blen, NULL);
  IRWR_SWMT_ASSERT_STATE (
      meta,
      ba == 0,
      "block_array_insert returned %d (ofst=%d blen=%d)",
      ba,
      ofst,
      blen
  );

  meta->len += len;

  free (data);
}

static void
irwr_swmt_remove (struct irwr_swarm_test *meta)
{
  int ofst, stride, len;
  irwr_swmt_random_slice (meta->len, &ofst, &stride, &len);

  size_t   buf_sz  = (size_t)len * (size_t)meta->esize;
  uint8_t *db_buf  = calloc (1, buf_sz);
  uint8_t *ref_buf = calloc (1, buf_sz);
  IRWR_SWMT_ASSERTF (
      db_buf && ref_buf,
      "calloc(%zu) failed in remove (len=%d esize=%u)",
      buf_sz,
      len,
      (unsigned)meta->esize
  );

  /* DB side */
  int exec_ret = nsdb_fexecute (
      meta->db,
      "remove %s[%d:%d:%d]",
      db_buf,
      meta->varname,
      ofst,
      ofst + len * stride,
      stride
  );
  IRWR_SWMT_ASSERT_STATE (
      meta,
      exec_ret,
      "nsdb remove returned %d "
      "(var='%s' slice=[%d:%d:%d])",
      exec_ret,
      meta->varname,
      ofst,
      ofst + len * stride,
      stride
  );

  /* Reference side */
  struct stride str = to_block_stride (ofst, stride, len);
  i64           got = block_array_remove (active_db (meta), str, meta->esize, ref_buf, NULL);
  IRWR_SWMT_ASSERT_STATE (
      meta,
      got == (i64)len,
      "block_array_remove returned %lld, expected %d "
      "(slice=[%d:%d:%d])",
      (long long)got,
      len,
      ofst,
      ofst + len * stride,
      stride
  );

  /* Cross-check */
  IRWR_SWMT_ASSERT_STATE (
      meta,
      memcmp (db_buf, ref_buf, buf_sz) == 0,
      "DB/reference data mismatch on remove "
      "(slice=[%d:%d:%d] esize=%u bytes=%zu)",
      ofst,
      ofst + len * stride,
      stride,
      (unsigned)meta->esize,
      buf_sz
  );

  meta->len -= len;

  free (db_buf);
  free (ref_buf);
}

static void
irwr_swmt_read (struct irwr_swarm_test *meta)
{
  int ofst, stride, len;
  irwr_swmt_random_slice (meta->len, &ofst, &stride, &len);

  size_t   buf_sz  = (size_t)len * (size_t)meta->esize;
  uint8_t *db_buf  = calloc (1, buf_sz);
  uint8_t *ref_buf = calloc (1, buf_sz);
  IRWR_SWMT_ASSERTF (
      db_buf && ref_buf,
      "calloc(%zu) failed in read (len=%d esize=%u)",
      buf_sz,
      len,
      (unsigned)meta->esize
  );

  /* DB side */
  int exec_ret = nsdb_fexecute (
      meta->db,
      "read %s[%d:%d:%d]",
      db_buf,
      meta->varname,
      ofst,
      ofst + len * stride,
      stride
  );
  IRWR_SWMT_ASSERT_STATE (
      meta,
      exec_ret,
      "nsdb read returned %d "
      "(var='%s' slice=[%d:%d:%d])",
      exec_ret,
      meta->varname,
      ofst,
      ofst + len * stride,
      stride
  );

  struct stride str = to_block_stride (ofst, stride, len);
  u64           got = block_array_read (active_db (meta), str, meta->esize, ref_buf);
  IRWR_SWMT_ASSERT_STATE (
      meta,
      got == (u64)len,
      "block_array_read returned %llu, expected %d "
      "(slice=[%d:%d:%d])",
      (unsigned long long)got,
      len,
      ofst,
      ofst + len * stride,
      stride
  );

  IRWR_SWMT_ASSERT_STATE (
      meta,
      memcmp (db_buf, ref_buf, buf_sz) == 0,
      "DB/reference data mismatch on read "
      "(slice=[%d:%d:%d] esize=%u bytes=%zu)",
      ofst,
      ofst + len * stride,
      stride,
      (unsigned)meta->esize,
      buf_sz
  );

  free (db_buf);
  free (ref_buf);
}

static void
irwr_swmt_write (struct irwr_swarm_test *meta)
{
  int ofst, stride, len;
  irwr_swmt_random_slice (meta->len, &ofst, &stride, &len);

  int      blen = len * (int)meta->esize;
  uint8_t *data = malloc ((size_t)blen);
  IRWR_SWMT_ASSERTF (
      data != NULL,
      "malloc(%d) failed in write (len=%d esize=%u)",
      blen,
      len,
      (unsigned)meta->esize
  );
  for (int i = 0; i < blen; ++i) {
    data[i] = (uint8_t)rand ();
  }

  /* DB side */
  int exec_ret = nsdb_fexecute (
      meta->db,
      "write %s[%d:%d:%d]",
      data,
      meta->varname,
      ofst,
      ofst + len * stride,
      stride
  );
  IRWR_SWMT_ASSERT_STATE (
      meta,
      exec_ret,
      "nsdb write returned %d "
      "(var='%s' slice=[%d:%d:%d])",
      exec_ret,
      meta->varname,
      ofst,
      ofst + len * stride,
      stride
  );

  struct stride str = to_block_stride (ofst, stride, len);
  u64           got = block_array_write (active_db (meta), str, meta->esize, data);
  IRWR_SWMT_ASSERT_STATE (
      meta,
      got == (u64)len,
      "block_array_write returned %llu, expected %d "
      "(slice=[%d:%d:%d])",
      (unsigned long long)got,
      len,
      ofst,
      ofst + len * stride,
      stride
  );

  free (data);
}

/******************************************************************************
 * SECTION: Main Api
 ******************************************************************************/

struct irwr_swarm_test *
irwr_swmt_open (
    int         initial_enabled[IRWR_AT_LEN],
    const char *dbname,
    int         max_insert_len,
    const char *varname,
    const char *vartype,
    float       sample_space_prob
)
{
  ASSERT (sample_space_prob >= 0 && sample_space_prob <= 1);

  struct irwr_swarm_test *ret = malloc (sizeof *ret);
  IRWR_SWMT_ASSERTF (
      ret != NULL,
      "malloc(%zu) failed for irwr_swarm_test (db='%s')",
      sizeof *ret,
      dbname
  );

  IRWR_SWMT_ASSERTF (nsdb_cleanup (dbname) == 0, "nsdb_cleanup failed for db='%s'", dbname);

  ALLOC_INIT (alloc);

  error       e = error_create ();
  struct type t;
  IRWR_SWMT_ASSERTF (
      compile_type (&t, vartype, &alloc, &e) == 0,
      "compile_type failed for vartype='%s'",
      vartype
  );
  t_size esize = type_byte_size (&t);

  ALLOC_CLOSE (alloc);

  *ret = (struct irwr_swarm_test){
      .committed         = block_array_create (512, default_mem (), NULL),
      .working           = NULL,
      .db                = nsdb_open (dbname),
      .in_txn            = 0,
      .dbname            = dbname,
      .varname           = varname,
      .vartype           = vartype,
      .esize             = esize,
      .max_insert_len    = max_insert_len,
      .len               = 0,
      .sample_space_prob = sample_space_prob,
  };

  IRWR_SWMT_ASSERTF (
      ret->committed != NULL,
      "block_array_create(512) returned NULL (db='%s')",
      dbname
  );
  IRWR_SWMT_ASSERTF (ret->db != NULL, "nsdb_open returned NULL (db='%s')", dbname);

  memcpy (ret->enabled, initial_enabled, IRWR_AT_LEN * sizeof (int));
  irwr_swmt_set_allowed (ret);

  IRWR_SWMT_ASSERTF (
      nsdb_fexecute (ret->db, "create %s %s", NULL, varname, vartype) == 0,
      "nsdb create failed (var='%s' type='%s' db='%s')",
      varname,
      vartype,
      dbname
  );

  return ret;
}

void
irwr_swmt_close (struct irwr_swarm_test *meta)
{
  if (meta->in_txn) {
    irwr_swmt_commit_txn (meta);
  }
  IRWR_SWMT_ASSERTF (nsdb_close (meta->db) == 0, "nsdb_close failed on db='%s'", meta->dbname);

  if (meta->committed) {
    block_array_free (meta->committed);
  }
  if (meta->working) {
    block_array_free (meta->working);
  }
  free (meta);
}

void
irwr_swmt_step (struct irwr_swarm_test *meta)
{
  irwr_swmt_print_state (meta);

  /* Count allowed actions */
  int len = 0;
  for (int i = 0; i < IRWR_AT_LEN; ++i) {
    len += meta->allowed[i];
  }

  /* If the irwr_swarm has masked everything off, re-roll and try again next
   * step rather than divide by zero. */
  if (len == 0) {
    i_log_info ("No allowed actions - re-rolling enabled mask\n");
    irwr_swmt_set_random_enabled (meta);
    irwr_swmt_set_allowed (meta);
    return;
  }

  /* Pick the n-th allowed action */
  int next   = rand () % len;
  int index  = 0;
  int choice = 0;
  for (; index < IRWR_AT_LEN; ++index) {
    if (meta->allowed[index]) {
      if (choice == next) {
        break;
      } else {
        choice++;
      }
    }
  }

  enum irwr_action_type action = (enum irwr_action_type)index;
  i_log_info ("-> %s\n", irwr_action_names[action]);

  switch (action) {
    case IRWR_BEGIN_TXN: irwr_swmt_begin_txn (meta); break;
    case IRWR_COMMIT_TXN: irwr_swmt_commit_txn (meta); break;
    case IRWR_ROLLBACK_TXN: irwr_swmt_rollback_txn (meta); break;
    case IRWR_CRASH_AND_REOPEN: irwr_swmt_crash_and_reopen (meta); break;
    case IRWR_CLOSE_AND_REOPEN: irwr_swmt_close_and_reopen (meta); break;
    case IRWR_INSERT: irwr_swmt_insert (meta); break;
    case IRWR_REMOVE: irwr_swmt_remove (meta); break;
    case IRWR_READ: irwr_swmt_read (meta); break;
    case IRWR_WRITE: irwr_swmt_write (meta); break;
    default:
      IRWR_SWMT_ASSERT_STATE (
          meta,
          0,
          "unknown action type %d (index=%d, allowed=%d)",
          (int)action,
          index,
          len
      );
  }

  // Choose a set of randomized actions
  if (randf () <= meta->sample_space_prob) {
    irwr_swmt_set_random_enabled (meta);
    i_log_info ("Changing Enabled. After:\n");
    irwr_swmt_print_state (meta);
  }

  irwr_swmt_set_allowed (meta);
}

/******************************************************************************
 * SECTION: irwr_swarm_test
 ******************************************************************************/

static volatile sig_atomic_t irwr_keep_running = 1;

static void
irwr_handle_sigint (int sig)
{
  (void)sig;
  irwr_keep_running = 0;
}

void
irwr_swarm_test (const char *dbname, int timeout_seconds, unsigned seed)
{
  srand (seed);

  int start_enabled[IRWR_AT_LEN];
  for (int i = 0; i < IRWR_AT_LEN; ++i) {
    start_enabled[i] = 1;
  }

  struct irwr_swarm_test *meta =
      irwr_swmt_open (start_enabled, dbname, 100000, "testvar", "u32", 0.01f);

#if PLATFORM_WINDOWS
  if (signal (SIGINT, irwr_handle_sigint) == SIG_ERR) {
    irwr_swmt_close (meta);
    return 0;
  }
#else
  struct sigaction sa;
  sa.sa_handler = irwr_handle_sigint;
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = 0;
  if (sigaction (SIGINT, &sa, NULL) == -1) {
    irwr_swmt_close (meta);
    return;
  }
#endif

  time_t start = time (NULL);

  while (irwr_keep_running) {
    if (time (NULL) - start >= timeout_seconds) {
      break;
    }
    irwr_swmt_step (meta);
  }

  irwr_swmt_close (meta);
}

/******************************************************************************
 * SECTION: CGD Internals
 * ----------------------------------------------------------------------------
 * @brief Assertion macros, action-name table, and state dumper used by every
 *        failure path in this fixture.
 ******************************************************************************/

static const char *const cgd_action_names[CDS_AT_LEN] = {
    [CDS_BEGIN_TXN]        = "BEGIN_TXN",
    [CDS_COMMIT_TXN]       = "COMMIT_TXN",
    [CDS_ROLLBACK_TXN]     = "ROLLBACK_TXN",
    [CDS_CRASH_AND_REOPEN] = "CRASH_AND_REOPEN",
    [CDS_CLOSE_AND_REOPEN] = "CLOSE_AND_REOPEN",
    [CDS_CREATE]           = "CREATE",
    [CDS_SWITCH]           = "SWITCH",
    [CDS_DELETE]           = "DELETE",
};

static void cgd_swmt_print_state (const struct cgd_swarm_test *meta);

/**
 * Bare ASSERTion: prints the failing expression and source location.
 */
#define CGD_SWMT_ASSERT(expr)                                                  \
  do {                                                                         \
    if (!(expr)) {                                                             \
      i_log_failure ("CGD swarm test FAILED\n");                               \
      i_log_failure ("  expr: %s\n", #expr);                                   \
      i_log_failure ("  loc:  %s:%d in %s()\n", __FILE__, __LINE__, __func__); \
      panic ("Aborting early\n");                                              \
    }                                                                          \
  }                                                                            \
  while (0)

/**
 * Formatted ASSERTion: adds a printf-style context line.
 */
#define CGD_SWMT_ASSERTF(expr, fmt, ...)                                       \
  do {                                                                         \
    if (!(expr)) {                                                             \
      i_log_failure ("CGD swarm test FAILED\n");                               \
      i_log_failure ("  expr: %s\n", #expr);                                   \
      i_log_failure ("  loc:  %s:%d in %s()\n", __FILE__, __LINE__, __func__); \
      i_log_failure ("  why:  " fmt "\n", __VA_ARGS__);                        \
      panic ("Aborting early\n");                                              \
    }                                                                          \
  }                                                                            \
  while (0)

/**
 * As CGD_SWMT_ASSERTF, but also dumps full fixture state on failure.
 */
#define CGD_SWMT_ASSERT_STATE(meta, expr, fmt, ...)                            \
  do {                                                                         \
    if (!(expr)) {                                                             \
      i_log_failure ("CGD swarm test FAILED\n");                               \
      i_log_failure ("  expr: %s\n", #expr);                                   \
      i_log_failure ("  loc:  %s:%d in %s()\n", __FILE__, __LINE__, __func__); \
      i_log_failure ("  why:  " fmt "\n", __VA_ARGS__);                        \
      cgd_swmt_print_state (meta);                                             \
      panic ("Aborting early\n");                                              \
    }                                                                          \
  }                                                                            \
  while (0)

/**
 * Dump the fixture state in a human-readable form. Safe with NULL meta
 * and NULL meta->cur.
 */
static void
cgd_swmt_print_state (const struct cgd_swarm_test *meta)
{
  i_log_info ("=== CGD Swarm Test State ===\n");
  if (meta == NULL) {
    i_log_info ("  (meta is NULL)\n");
    return;
  }
  i_log_info ("  dbname:         %s\n", meta->dbname);
  i_log_info ("  in_txn:         %s\n", meta->in_txn ? "yes" : "no");
  i_log_info (
      "  committed:      %s (count=%d)\n",
      meta->committed ? "present" : "<null>",
      meta->committed ? mem_vhmap_count (meta->committed) : -1
  );
  i_log_info (
      "  working:        %s (count=%d)\n",
      meta->working ? "present" : "<null>",
      meta->working ? mem_vhmap_count (meta->working) : -1
  );
  i_log_info ("  cur:            %s\n", meta->cur ? meta->cur->vname.data : "<null>");
  i_log_info ("  sample_space_p: %.3f\n", (double)meta->sample_space_prob);
  i_log_info ("  Actions             enabled   allowed\n");
  for (int i = 0; i < CDS_AT_LEN; ++i) {
    i_log_info (
        "    %-18s   %-3s       %-3s\n",
        cgd_action_names[i],
        meta->enabled[i] ? "yes" : "no",
        meta->allowed[i] ? "yes" : "no"
    );
  }
}

///////////////////////////////////////////////////////////
/// Utils

static struct mem_vhmap *
cgd_active_db (struct cgd_swarm_test *meta)
{
  return meta->in_txn ? meta->working : meta->committed;
}

static void
rebind_cur (struct cgd_swarm_test *meta, const char *preferred_name)
{
  struct mem_vhmap *db = cgd_active_db (meta);
  meta->cur            = preferred_name ? mem_vhmap_get_var (db, strfcstr (preferred_name)) : NULL;
  if (!meta->cur && mem_vhmap_count (db) > 0) {
    meta->cur = mem_vhmap_random (db);
  }
}

static void
cgd_swmt_set_random_enabled (struct cgd_swarm_test *meta)
{
  int mask = rand () % ((1 << CDS_AT_LEN) - 1) + 1;
  for (int i = 0; i < CDS_AT_LEN; ++i) {
    meta->enabled[i] = (mask >> i) & 1;
  }
}

static void
cgd_swmt_set_allowed (struct cgd_swarm_test *meta)
{
  ASSERT (meta);
  ASSERT (meta->db);
  ASSERT (meta->dbname);

  memset (meta->allowed, 0, sizeof (meta->allowed));

  struct mem_vhmap *db                = cgd_active_db (meta);
  int               nvars             = mem_vhmap_count (db);

  meta->allowed[CDS_CRASH_AND_REOPEN] = meta->enabled[CDS_CRASH_AND_REOPEN];
  meta->allowed[CDS_CREATE]           = meta->enabled[CDS_CREATE];

  if (!meta->in_txn) {
    meta->allowed[CDS_BEGIN_TXN]        = meta->enabled[CDS_BEGIN_TXN];
    meta->allowed[CDS_CLOSE_AND_REOPEN] = meta->enabled[CDS_CLOSE_AND_REOPEN];
  } else {
    meta->allowed[CDS_COMMIT_TXN]   = meta->enabled[CDS_COMMIT_TXN];
    meta->allowed[CDS_ROLLBACK_TXN] = meta->enabled[CDS_ROLLBACK_TXN];
  }

  if (nvars > 1) {
    meta->allowed[CDS_DELETE] = meta->enabled[CDS_DELETE];
    meta->allowed[CDS_SWITCH] = meta->enabled[CDS_SWITCH];
  }
}

static u32
get_random_name_len (void)
{
  u32 roll = randu32r (1, 100);

  if (roll <= 90) {
    return randu32r (2, 10);
  }

  if (roll <= 95) {
    return randu32r (10, NS_PAGE_SIZE);
  }

  return randu32r (NS_PAGE_SIZE, 10 * NS_PAGE_SIZE);
}

static char *
random_name (void)
{
  u32   length = get_random_name_len ();
  char *buffer = malloc (length * sizeof (char));
  var_random_name (buffer, length);

  return buffer;
}

static u32
get_random_type_depth (void)
{
  u32 roll = randu32r (1, 100);

  if (roll <= 95) {
    return randu32r (1, 3);
  }

  return randu32r (3, 10);
}

static char *
type_str (const struct type *t)
{
  u32   len    = type_get_string_size (t);
  char *buffer = malloc ((len + 1) * sizeof (char));
  type_generate_string (buffer, t);
  return buffer;
}

///////////////////////////////////////////////////////////
/// Concrete Actions

static void
cgd_swmt_begin_txn (struct cgd_swarm_test *meta)
{
  ASSERT (!meta->in_txn);
  ASSERT (meta->working == NULL);

  CGD_SWMT_ASSERTF (nsdb_begin (meta->db) == 0, "nsdb_begin failed on db='%s'", meta->dbname);

  error e       = error_create ();
  meta->working = mem_vhmap_clone (meta->committed, &e);
  CGD_SWMT_ASSERTF (
      meta->working != NULL,
      "mem_vhmap_clone returned NULL (committed count=%d)",
      mem_vhmap_count (meta->committed)
  );
  meta->in_txn = 1;
}

static void
cgd_swmt_commit_txn (struct cgd_swarm_test *meta)
{
  ASSERT (meta->in_txn);
  ASSERT (meta->working != NULL);

  CGD_SWMT_ASSERTF (
      nsdb_commit (meta->db) == 0,
      "nsdb_commit failed on db='%s' (working count=%d)",
      meta->dbname,
      mem_vhmap_count (meta->working)
  );

  char *saved = meta->cur ? strdup (meta->cur->vname.data) : NULL;
  mem_vhmap_free (meta->committed);
  meta->committed = meta->working;
  meta->working   = NULL;
  meta->in_txn    = 0;

  rebind_cur (meta, saved);
  free (saved);
}

static void
cgd_swmt_rollback_txn (struct cgd_swarm_test *meta)
{
  ASSERT (meta->in_txn);

  CGD_SWMT_ASSERTF (nsdb_rollback (meta->db) == 0, "nsdb_rollback failed on db='%s'", meta->dbname);

  char *saved = meta->cur ? strdup (meta->cur->vname.data) : NULL;
  mem_vhmap_free (meta->working);
  meta->working = NULL;
  meta->in_txn  = 0;

  rebind_cur (meta, saved);
  free (saved);
}

static void
cgd_swmt_crash_and_reopen (struct cgd_swarm_test *meta)
{
  char *saved = meta->cur ? strdup (meta->cur->vname.data) : NULL;

  CGD_SWMT_ASSERTF (nsdb_crash (meta->db) == 0, "nsdb_crash failed on db='%s'", meta->dbname);

  meta->db = nsdb_open (meta->dbname);
  CGD_SWMT_ASSERTF (
      meta->db != NULL,
      "nsdb_open after crash returned NULL (db='%s')",
      meta->dbname
  );

  if (meta->working) {
    mem_vhmap_free (meta->working);
    meta->working = NULL;
  }
  meta->in_txn = 0;

  rebind_cur (meta, saved);
  free (saved);
}

static void
cgd_swmt_close_and_reopen (struct cgd_swarm_test *meta)
{
  ASSERT (!meta->in_txn);
  CGD_SWMT_ASSERTF (nsdb_close (meta->db) == 0, "nsdb_close failed on db='%s'", meta->dbname);

  meta->db = nsdb_open (meta->dbname);
  CGD_SWMT_ASSERTF (
      meta->db != NULL,
      "nsdb_open after close returned NULL (db='%s')",
      meta->dbname
  );
}

static void
cgd_swmt_create (struct cgd_swarm_test *meta)
{
  struct mem_vhmap *db = cgd_active_db (meta);

  // Loop until you get a unique variable name
  for (;;) {
    ALLOC_INIT (temp);

    error        e       = error_create ();
    char        *name    = random_name ();
    struct type *type    = type_random (&temp, get_random_type_depth (), &e);
    char        *typestr = type_str (type);

    // Already exists - try again
    if (mem_vhmap_get_var (db, strfcstr (name)) != NULL) {
      free (name);
      free (typestr);
      ALLOC_CLOSE (temp);
      continue;
    }

    struct variable var = {
        .vname    = strfcstr (name),
        .dtype    = type,
        .nbytes   = 0,
        .rpt_root = 0,
        .var_root = 0,
    };

    /* DB side */
    int exec_ret = nsdb_fexecute (meta->db, "create %s %s", NULL, name, typestr);
    CGD_SWMT_ASSERT_STATE (
        meta,
        exec_ret == 0,
        "nsdb create returned %d (name='%s' type='%s')",
        exec_ret,
        name,
        typestr
    );

    /* Reference side -- takes ownership of name & typestr */
    int add_ret = mem_vhmap_add_var (db, &var, &e);
    CGD_SWMT_ASSERT_STATE (
        meta,
        add_ret == 0,
        "mem_vhmap_add_var returned %d (name='%s')",
        add_ret,
        name
    );

    /* First variable becomes the current one */
    if (meta->cur == NULL) {
      meta->cur = mem_vhmap_get_var (db, var.vname);
    }

    free (name);
    free (typestr);
    ALLOC_CLOSE (temp);

    return;
  }
}

static void
cgd_swmt_switch (struct cgd_swarm_test *meta)
{
  struct mem_vhmap *db = cgd_active_db (meta);
  meta->cur            = mem_vhmap_random (db);
  CGD_SWMT_ASSERT_STATE (
      meta,
      meta->cur != NULL,
      "mem_vhmap_random returned NULL (count=%d)",
      mem_vhmap_count (db)
  );
}

static void
cgd_swmt_delete (struct cgd_swarm_test *meta)
{
  CGD_SWMT_ASSERT_STATE (
      meta,
      meta->cur != NULL,
      "delete with no current variable (active count=%d)",
      mem_vhmap_count (cgd_active_db (meta))
  );

  const char *name     = meta->cur->vname.data;
  int         exec_ret = nsdb_fexecute (meta->db, "delete %s", NULL, name);
  CGD_SWMT_ASSERT_STATE (
      meta,
      exec_ret == 0,
      "nsdb delete returned %d (name='%s')",
      exec_ret,
      name
  );

  mem_vhmap_remove_var (cgd_active_db (meta), meta->cur->vname);
  meta->cur = mem_vhmap_random (cgd_active_db (meta));
}

///////////////////////////////////////////////////////////
/// Main Api

struct cgd_swarm_test *
cgd_swmt_open (int start_enabled[CDS_AT_LEN], const char *dbname, float sample_space_prob)
{
  ASSERT (sample_space_prob >= 0 && sample_space_prob <= 1);

  struct cgd_swarm_test *ret = malloc (sizeof *ret);
  CGD_SWMT_ASSERTF (
      ret != NULL,
      "malloc(%zu) failed for cgd_swarm_test (db='%s')",
      sizeof *ret,
      dbname
  );

  CGD_SWMT_ASSERTF (nsdb_cleanup (dbname) == 0, "nsdb_cleanup failed for db='%s'", dbname);

  *ret = (struct cgd_swarm_test){
      .committed         = mem_vhmap_create (NULL),
      .working           = NULL,
      .cur               = NULL,
      .db                = nsdb_open (dbname),
      .in_txn            = 0,
      .dbname            = dbname,
      .sample_space_prob = sample_space_prob,
  };

  CGD_SWMT_ASSERTF (ret->committed != NULL, "mem_vhmap_create returned NULL (db='%s')", dbname);
  CGD_SWMT_ASSERTF (ret->db != NULL, "nsdb_open returned NULL (db='%s')", dbname);

  memcpy (ret->enabled, start_enabled, CDS_AT_LEN * sizeof (int));
  cgd_swmt_set_allowed (ret);

  return ret;
}

void
cgd_swmt_close (struct cgd_swarm_test *meta)
{
  if (meta->in_txn) {
    cgd_swmt_commit_txn (meta);
  }
  CGD_SWMT_ASSERTF (nsdb_close (meta->db) == 0, "nsdb_close failed on db='%s'", meta->dbname);

  if (meta->committed) {
    mem_vhmap_free (meta->committed);
  }
  if (meta->working) {
    mem_vhmap_free (meta->working);
  }

  free (meta);
}

void
cgd_swmt_step (struct cgd_swarm_test *meta)
{
  cgd_swmt_print_state (meta);

  /* Count allowed actions */
  int len = 0;
  for (int i = 0; i < CDS_AT_LEN; ++i) {
    len += meta->allowed[i];
  }

  /* If the cgd_swarm has masked everything off, re-roll and try again next
   * step rather than divide by zero. */
  if (len == 0) {
    i_log_info ("No allowed actions - re-rolling enabled mask\n");
    cgd_swmt_set_random_enabled (meta);
    cgd_swmt_set_allowed (meta);
    return;
  }

  /* Pick the n-th allowed action */
  int next   = rand () % len;
  int index  = 0;
  int choice = 0;
  for (; index < CDS_AT_LEN; ++index) {
    if (meta->allowed[index]) {
      if (choice == next) {
        break;
      } else {
        choice++;
      }
    }
  }

  enum cgd_action_type action = (enum cgd_action_type)index;
  i_log_info ("-> %s\n", cgd_action_names[action]);

  switch (action) {
    case CDS_BEGIN_TXN: cgd_swmt_begin_txn (meta); break;
    case CDS_COMMIT_TXN: cgd_swmt_commit_txn (meta); break;
    case CDS_ROLLBACK_TXN: cgd_swmt_rollback_txn (meta); break;
    case CDS_CRASH_AND_REOPEN: cgd_swmt_crash_and_reopen (meta); break;
    case CDS_CLOSE_AND_REOPEN: cgd_swmt_close_and_reopen (meta); break;
    case CDS_CREATE: cgd_swmt_create (meta); break;
    case CDS_SWITCH: cgd_swmt_switch (meta); break;
    case CDS_DELETE: cgd_swmt_delete (meta); break;
    default:
      CGD_SWMT_ASSERT_STATE (
          meta,
          0,
          "unknown action type %d (index=%d, allowed=%d)",
          (int)action,
          index,
          len
      );
  }

  // Choose a set of randomized actions
  if (randf () <= meta->sample_space_prob) {
    cgd_swmt_set_random_enabled (meta);
    i_log_info ("Changing state space. After:\n");
    cgd_swmt_print_state (meta);
  }

  cgd_swmt_set_allowed (meta);
}

/******************************************************************************
 * SECTION: cgd_swarm_test
 ******************************************************************************/

static volatile sig_atomic_t cgd_keep_running = 1;

static void
cgd_handle_sigint (int sig)
{
  (void)sig;
  cgd_keep_running = 0;
}

void
cgd_swarm_test (const char *dbname, int timeout_seconds, unsigned seed)
{
  srand (seed);

  int start_enabled[CDS_AT_LEN];
  for (int i = 0; i < CDS_AT_LEN; ++i) {
    start_enabled[i] = 1;
  }

  struct cgd_swarm_test *meta = cgd_swmt_open (start_enabled, dbname, 0.01f);

#if PLATFORM_WINDOWS
  if (signal (SIGINT, cgd_handle_sigint) == SIG_ERR) {
    cgd_swmt_close (meta);
    return 0;
  }
#else
  struct sigaction sa;
  sa.sa_handler = cgd_handle_sigint;
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = 0;
  if (sigaction (SIGINT, &sa, NULL) == -1) {
    cgd_swmt_close (meta);
    return;
  }
#endif

  time_t start = time (NULL);

  while (cgd_keep_running) {
    if (time (NULL) - start >= timeout_seconds) {
      break;
    }
    cgd_swmt_step (meta);
  }

  cgd_swmt_close (meta);
}
