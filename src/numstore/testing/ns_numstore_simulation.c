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

#include "numstore/testing/ns_numstore_simulation.h"

#include "core/ns_alloc.h"
#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_ext_array.h"
#include "core/ns_numerics.h"
#include "core/ns_stride.h"
#include "core/ns_string.h"
#include "core/os/ns_memory.h"
#include "core/os/ns_time.h"
#include "nscore/nsdb/ns_nsdb.h"
#include "nscore/types/ns_types.h"
#include "nscore/variables/ns_variables.h"
#include "numstore/numstore.h"
#include "numstore/testing/ns_mem_vhmap.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

/* CONTRACT: once any step records RS_FAILURE, the caller must not call
 * ns_simul_prepare / ns_simul_execute again. Failure paths therefore only
 * do the minimum needed to (a) not leak, (b) not leave dangling pointers
 * that would confuse ns_simul_close, and (c) log why. They do not try to
 * restore a runnable state. */

struct ns_simulation
{
  // Fake database semantics
  //    While in a txn, working is the source of truth
  //    While not in a txn, committed is the source of truth
  //    On commit - committed = deep_copy(working)
  //    On rollback - free(working)
  struct mem_vhmap      *committed;
  struct mem_vhmap      *working;

  // The current variable to work with
  // On switch, this variable gets swapped
  // out.
  //    Same transaction flow as above
  struct var_with_data  *cur_committed;
  struct var_with_data  *cur_working;

  // Which actions are turned on
  int                    enabled[NSS_AT_LEN];

  // Which logical actions are available
  int                    allowed[NSS_AT_LEN];

  nsdb_t                *db;
  struct ns_txn         *tx;
  const char            *dbname;
  int                    max_insert_len;
  float                  sample_space_prob;

  // FIX(#8): metric snapshots taken at begin_txn, restored on
  // rollback / crash so nvars & tracked_bytes stay transactional
  // like the reference maps.
  u32                    txn_saved_nvars;
  u32                    txn_saved_tracked_bytes;

  struct ns_simul_record results;
  i_timer                timer;
  struct allocator       alloc;
};

DEFINE_DBG_ASSERT (struct ns_simulation, ns_simulation, n, {
  ASSERT (n);
  ASSERT (n->committed);

  if (n->tx) {
    ASSERT (n->working);
    if (n->results.nvars > 0) {
      ASSERT (n->cur_working);
    } else {
      ASSERT (n->cur_working == NULL);
    }
  } else {
    ASSERT (n->working == NULL);
    if (n->results.nvars > 0) {
      ASSERT (n->cur_committed);
      ASSERT (n->cur_working == NULL);
    } else {
      ASSERT (n->cur_committed == NULL);
      ASSERT (n->cur_working == NULL);
    }
  }
})

static struct mem_vhmap *
active_db (const struct ns_simulation *meta)
{
  return meta->tx ? meta->working : meta->committed;
}

static struct var_with_data *
active_var (const struct ns_simulation *meta)
{
  return meta->tx ? meta->cur_working : meta->cur_committed;
}

static void
set_active_var (struct ns_simulation *meta, struct var_with_data *var)
{
  if (meta->tx) {
    meta->cur_working = var;
  } else {
    meta->cur_committed = var;
  }
}

/******************************************************************************
 * SECTION: Utilities
 * ----------------------------------------------------------------------------
 * @brief Assertion macros, action-name table, and state dumper used by every
 *        failure path in this fixture.
 ******************************************************************************/

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

// Returns NULL on allocation failure
static char *
random_name (void)
{
  // TODO - use i_malloc
  // FIX(minor): allocate room for a NUL and terminate explicitly so
  // later strfcstr()/"%s" use is safe regardless of whether
  // var_random_name terminates within `length`.
  u32   length = get_random_name_len ();
  char *buffer = malloc ((size_t)length + 1);
  if (buffer == NULL) {
    i_log_failure ("random_name: malloc failed, length=%u\n", length + 1);
    return NULL;
  }
  var_random_name (buffer, length);
  buffer[length] = '\0';

  return buffer;
}

static u32
get_random_type_depth (void)
{
  return randu32r (1, 3);
}

static void
nss_random_slice (b_size len, b_size *ofst, b_size *stride, b_size *nelems)
{
  ASSERT (len > 0);

  // Offset is any value between [0, len - 1]
  *ofst            = randu32r (0, len - 1);

  // Stride is anything between [1, len - offset]
  b_size remaining = len - *ofst;
  *stride          = randu32r (1, remaining);

  // Elements is between [1, (len - offset + stride - 1) / stride
  b_size max_len   = (remaining + *stride - 1) / *stride;
  *nelems          = randu32r (1, max_len);
}

static err_t
gen_new_var (struct ns_simulation *meta, struct create_op *dest, error *e)
{
  while (true) {
    create_default_allocator (&meta->alloc);

    char *name = random_name ();
    if (name == NULL) {
      allocator_free (&meta->alloc);
      return error_causef (e, ERR_NOMEM, "gen_new_var: name allocation failed");
    }

    // Generate a random type
    struct type *type = type_random (&meta->alloc, get_random_type_depth (), e);
    if (type == NULL) {
      free (name);
      allocator_free (&meta->alloc);
      return error_trace (e);
    }

    // Get the type string for that type
    char *typestr = type_tostr (type);
    if (typestr == NULL) {
      free (name);
      allocator_free (&meta->alloc); // frees `type` too
      return error_trace (e);
    }

    // Already exists - try again
    if (mem_vhmap_get (active_db (meta), strfcstr (name)) != NULL) {
      free (name);
      free (typestr);
      allocator_free (&meta->alloc);
      continue;
    }

    dest->vname   = name;
    dest->type    = type;
    dest->typestr = typestr;
    break;
  }

  return SUCCESS;
}

static struct stride
to_block_stride (b_size ofst, b_size stride, b_size len)
{
  return (struct stride){
      .start  = (u64)ofst,
      .stride = (u64)stride,
      .nelems = (u64)len,
  };
}

// Returns NULL on allocation failure
static u8 *
random_data (b_size blen)
{
  uint8_t *data = malloc ((size_t)blen);
  if (data == NULL) {
    i_log_failure ("random_data: malloc failed, blen=%" PRb_size "\n", blen);
    return NULL;
  }
  rand_bytes (data, blen);
  return data;
}

static void
nss_set_random_enabled (struct ns_simulation *meta)
{
  int mask = rand () % ((1 << NSS_AT_LEN) - 1) + 1;
  for (int i = 0; i < NSS_AT_LEN; ++i) {
    meta->enabled[i] = (mask >> i) & 1;
  }
}

static void
nss_set_allowed (struct ns_simulation *meta)
{
  ASSERT (meta);
  ASSERT (meta->db);
  ASSERT (meta->dbname);

  memset (meta->allowed, 0, sizeof (meta->allowed));

  meta->allowed[NSS_CRASH_AND_REOPEN] = 0; // meta->enabled[NSS_CRASH_AND_REOPEN];

  if (!meta->tx) {
    meta->allowed[NSS_BEGIN_TXN]        = meta->enabled[NSS_BEGIN_TXN];
    meta->allowed[NSS_CLOSE_AND_REOPEN] = meta->enabled[NSS_CLOSE_AND_REOPEN];
  } else {
    meta->allowed[NSS_COMMIT_TXN]   = meta->enabled[NSS_COMMIT_TXN];
    meta->allowed[NSS_ROLLBACK_TXN] = meta->enabled[NSS_ROLLBACK_TXN];
  }

  meta->allowed[NSS_CREATE] = meta->enabled[NSS_CREATE];
  if (meta->results.nvars > 1) {
    meta->allowed[NSS_SWITCH] = meta->enabled[NSS_SWITCH];
  }
  if (meta->results.nvars > 0) {
    meta->allowed[NSS_DELETE] = meta->enabled[NSS_DELETE];
  }

  if (active_var (meta) != NULL) {
    meta->allowed[NSS_INSERT] = meta->enabled[NSS_INSERT];

    if (ext_array_get_len (&active_var (meta)->data) > 0) {
      meta->allowed[NSS_REMOVE] = meta->enabled[NSS_REMOVE];
      meta->allowed[NSS_READ]   = meta->enabled[NSS_READ];
      meta->allowed[NSS_WRITE]  = meta->enabled[NSS_WRITE];
    }
  }
}

/******************************************************************************
 * SECTION: Timing / Records
 ******************************************************************************/

// TODO - put this in fs layer
long
get_file_size (const char *filename)
{
  struct stat st;
  if (stat (filename, &st) == 0) {
    return st.st_size;
  }
  i_log_failure ("get_file_size: stat failed: %s\n", filename);
  return -1;
}

static struct ns_simul_record
create_nssr (const char *dbname, u64 seed, const char *commit_hash, u64 sequence_id, error *e)
{
  (void)e; // TODO - use this
  return (struct ns_simul_record){
      .seed          = seed,
      .commit_hash   = commit_hash,
      .sequence_id   = sequence_id,
      .step_number   = 0,
      .clock         = 0,
      .working_clock = 0,
      .db_total_size = get_file_size (dbname),
      .nvars         = 0,
      .tracked_bytes = 0,
      .inner         = {0},
  };
}

static inline u64
nss_clock_step (struct ns_simulation *meta)
{
  u64 now             = i_timer_now_ns (&meta->timer);
  u64 prev            = meta->results.clock;
  meta->results.clock = now;
  // FIX(minor): a coarse clock may legally return the same value twice;
  // strict > fires spuriously.
  ASSERT (now >= prev);
  return now - prev;
}

/* Bookkeeping shared by every timed action's exit path. */
static void
nss_finish_op (struct ns_simulation *meta, u64 tdiff, enum ns_result_record_type rt)
{
  meta->results.working_clock += tdiff;
  meta->results.inner.op_duration_ms = tdiff;
  meta->results.db_total_size        = get_file_size (meta->dbname);
  meta->results.inner.record_type    = rt;
}

/******************************************************************************
 * SECTION: Op-Effect Verification
 * ----------------------------------------------------------------------------
 * @brief Checks that an operation which *reported* success actually had its
 *        effect on the real db:
 *          - length agreement after every size-changing op (INSERT/REMOVE/
 *            CREATE), via `get`
 *          - immediate readback of the exact slice after INSERT and WRITE,
 *            compared against the data we just sent
 *          - existence checks: CREATE must make `get` succeed with 0 bytes,
 *            DELETE must make `get` fail
 *
 *        These verify the db against *itself* (did the op land?), while the
 *        read-all check below verifies the db against the *reference model*
 *        (is the data right?). A write bug is caught here at the WRITE step
 *        even if READ_ALL is disabled.
 *
 *        Enable with -DVERIFY_OP_EFFECTS; compiles out otherwise.
 ******************************************************************************/

#define READ_ALL_AFTER_EXECUTION
#define VERIFY_OP_EFFECTS

#if defined(READ_ALL_AFTER_EXECUTION) || defined(VERIFY_OP_EFFECTS)

/* Compare the real db's byte count for `v` against the reference model's.
 * Also serves as an existence check: `get` failing on a variable the model
 * thinks exists is itself a divergence. */
static int
nss_verify_len (struct ns_simulation *meta, struct var_with_data *v)
{
  struct nsdb_var *dbvar = NULL;
  err_t ret = nsdb_fexecute (meta->db, meta->tx, "get %.*s", &dbvar, strfmt (&v->var.vname));
  if (ret != 0) {
    nsdb_var_free (meta->db, dbvar);
    i_log_failure (
        "verify-len: get failed for %.*s (model says it exists)\n",
        strfmt (&v->var.vname)
    );
    return -1;
  }

  b_size ref_nbytes = ext_array_get_len (&v->data);
  // NOTE: adjust this accessor to however nsdb_var exposes the byte count
  b_size db_nbytes  = dbvar->var->nbytes;
  nsdb_var_free (meta->db, dbvar);

  if (db_nbytes != ref_nbytes) {
    i_log_failure (
        "verify-len mismatch: var=%.*s db=%" PRb_size " bytes, ref=%" PRb_size " bytes\n",
        strfmt (&v->var.vname),
        db_nbytes,
        ref_nbytes
    );
    return -1;
  }

  return 0;
}

#endif /* READ_ALL_AFTER_EXECUTION || VERIFY_OP_EFFECTS */

#ifdef VERIFY_OP_EFFECTS

/* Read the given slice of the active variable back from the real db and
 * compare it byte-for-byte against `expect` (the buffer the op just sent).
 * Catches "op reported success but the bytes never landed / landed at the
 * wrong strided positions". */
static int
nss_verify_readback (
    struct ns_simulation *meta,
    b_size                ofst,
    b_size                stride,
    b_size                nelems,
    t_size                tsize,
    const uint8_t        *expect
)
{
  struct var_with_data *v      = active_var (meta);
  size_t                buf_sz = (size_t)nelems * tsize;
  uint8_t              *buf    = malloc (buf_sz);
  if (buf == NULL) {
    i_log_failure ("verify-readback: malloc failed, buf_sz=%zu\n", buf_sz);
    return -1;
  }

  err_t ret = nsdb_fexecute (
      meta->db,
      meta->tx,
      "read %.*s[%" PRb_size ":%" PRb_size ":%" PRb_size "]",
      buf,
      strfmt (&v->var.vname),
      ofst,
      ofst + nelems * stride,
      stride
  );
  if (ret != (sb_size)nelems) {
    i_log_failure (
        "verify-readback: read returned %lld, expected %" PRb_size " (ofst=%" PRb_size
        " stride=%" PRb_size ")\n",
        (long long)ret,
        nelems,
        ofst,
        stride
    );
    free (buf);
    return -1;
  }

  if (memcmp (buf, expect, buf_sz) != 0) {
    size_t i = 0;
    while (i < buf_sz && buf[i] == expect[i]) {
      ++i;
    }
    i_log_failure (
        "verify-readback mismatch: op reported success but byte %zu (elem %zu) "
        "reads 0x%02x, wrote 0x%02x (ofst=%" PRb_size " stride=%" PRb_size " nelems=%" PRb_size
        " tsize=%" PRIu64 ")\n",
        i,
        i / tsize,
        buf[i],
        expect[i],
        ofst,
        stride,
        nelems,
        (u64)tsize
    );
    free (buf);
    return -1;
  }

  free (buf);
  return 0;
}

/* After a successful DELETE, `get` on the deleted name must fail. */
static int
nss_verify_gone (struct ns_simulation *meta, struct var_with_data *v)
{
  struct nsdb_var *dbvar = NULL;
  err_t ret = nsdb_fexecute (meta->db, meta->tx, "get %.*s", &dbvar, strfmt (&v->var.vname));
  nsdb_var_free (meta->db, dbvar);

  if (ret == 0) {
    i_log_failure (
        "verify-gone: delete reported success but %.*s still exists in db\n",
        strfmt (&v->var.vname)
    );
    return -1;
  }

  return 0;
}

#  define VERIFY_EFFECT(meta, call)                     \
    do {                                                \
      if ((call) < 0) {                                 \
        (meta)->results.inner.record_type = RS_FAILURE; \
      }                                                 \
    }                                                   \
    while (0)

#else

/* Arguments are discarded unexpanded, so the guarded helpers may be
 * referenced at call sites without existing in this configuration. */
#  define VERIFY_EFFECT(meta, call) ((void)0)

#endif /* VERIFY_OP_EFFECTS */

/******************************************************************************
 * SECTION: Read-All Verification
 * ----------------------------------------------------------------------------
 * @brief After every successful action, read the entire active variable from
 *        the real db and from the reference model and compare byte-for-byte.
 *        Catches divergence at the step that caused it (e.g. an unverified
 *        WRITE) instead of N steps later at the next overlapping READ.
 *
 *        Enable with -DREAD_ALL_AFTER_EXECUTION; compiles out otherwise.
 ******************************************************************************/

#ifdef READ_ALL_AFTER_EXECUTION

static const char *op_type_str (enum ns_action_type t); // defined below

static int
nss_read_all_verify_var (struct ns_simulation *meta, struct var_with_data *v)
{
  /* Length agreement first. This is not just another check - db_buf below is
   * sized from the *reference* length, so if the real db thinks the variable
   * is longer, the open-ended [0:] read would overflow the buffer. */
  if (nss_verify_len (meta, v) < 0) {
    return -1;
  }

  b_size blen = ext_array_get_len (&v->data);
  if (blen == 0) {
    return 0;
  }

  t_size tsize = type_byte_size (v->var.dtype);
  ASSERT (tsize > 0);
  ASSERT (blen % tsize == 0);
  b_size   nelems  = blen / tsize;

  uint8_t *db_buf  = malloc ((size_t)blen);
  uint8_t *ref_buf = malloc ((size_t)blen);
  if (db_buf == NULL || ref_buf == NULL) {
    i_log_failure ("read-all: malloc failed, blen=%" PRb_size "\n", blen);
    free (db_buf);
    free (ref_buf);
    return -1;
  }

  // read var[0:] - full read of the real db, through the current txn so
  // mid-txn state is compared against the working map, not the committed one
  err_t ret = nsdb_fexecute (meta->db, meta->tx, "read %.*s[0:]", db_buf, strfmt (&v->var.vname));
  if (ret != (sb_size)nelems) {
    i_log_failure (
        "read-all: db read returned %lld, expected %" PRb_size ": %.*s\n",
        (long long)ret,
        nelems,
        strfmt (&v->var.vname)
    );
    free (db_buf);
    free (ref_buf);
    return -1;
  }

  // Same read against the reference
  struct stride all = to_block_stride (0, 1, nelems);
  i64           got = ext_array_read (&v->data, all, tsize, ref_buf);
  if (got != (i64)nelems) {
    i_log_failure (
        "read-all: reference read returned %lld, expected %" PRb_size "\n",
        (long long)got,
        nelems
    );
    free (db_buf);
    free (ref_buf);
    return -1;
  }

  int rc = 0;
  if (memcmp (db_buf, ref_buf, (size_t)blen) != 0) {
    // Find and report the first divergent byte / element
    b_size i = 0;
    while (i < blen && db_buf[i] == ref_buf[i]) {
      ++i;
    }
    i_log_failure (
        "read-all mismatch: var=%.*s byte=%" PRb_size " (elem %" PRb_size ", tsize=%" PRIu64
        "): db=0x%02x ref=0x%02x blen=%" PRb_size "\n",
        strfmt (&v->var.vname),
        i,
        i / tsize,
        (u64)tsize,
        db_buf[i],
        ref_buf[i],
        blen
    );
    rc = -1;
  }

  free (db_buf);
  free (ref_buf);
  return rc;
}

static int
nss_read_all_verify (struct ns_simulation *meta)
{
  // TODO: when mem_vhmap grows an iterator, verify every variable here -
  // that also catches ops that corrupt a *neighboring* variable's pages.
  struct var_with_data *v = active_var (meta);
  if (v == NULL) {
    return 0;
  }
  return nss_read_all_verify_var (meta, v);
}

/* Only meaningful after a successful op - after a failure the caller stops
 * anyway (see CONTRACT above) and the state is not guaranteed comparable. */
#  define READ_ALL_VERIFY(meta, action)                                                        \
    do {                                                                                       \
      if ((meta)->results.inner.record_type == RS_SUCCESS && nss_read_all_verify (meta) < 0) { \
        i_log_failure (                                                                        \
            "read-all verify failed after %s (step %" PRIu64 ")\n",                            \
            op_type_str (action),                                                              \
            (meta)->results.step_number                                                        \
        );                                                                                     \
        (meta)->results.inner.record_type = RS_FAILURE;                                        \
      }                                                                                        \
    }                                                                                          \
    while (0)

#else

#  define READ_ALL_VERIFY(meta, action) ((void)0)

#endif /* READ_ALL_AFTER_EXECUTION */

/******************************************************************************
 * SECTION: Concrete Actions
 * ----------------------------------------------------------------------------
 * @brief Each of these executes the action whose parameters were already
 *        decided in nssr_pre_op(). They must not re-roll their own random
 *        parameters - they only reach into
 *        meta->results.inner.operation.<action> for what pre already chose.
 ******************************************************************************/

static void
nss_begin_txn (struct ns_simulation *meta)
{
  DBG_ASSERT (ns_simulation, meta);
  ASSERT (!meta->tx);
  ASSERT (meta->working == NULL);

  nss_clock_step (meta);
  meta->tx                           = nsdb_begin (meta->db);
  meta->results.inner.op_duration_ms = nss_clock_step (meta);

  if (meta->tx == NULL) {
    i_log_failure ("nsdb_begin failed: %s\n", meta->dbname);
    meta->results.inner.record_type = RS_FAILURE;
    return;
  }

  error e       = error_create ();
  meta->working = mem_vhmap_clone (meta->committed, &e);

  if (meta->working == NULL) {
    i_log_failure ("mem_vhmap_clone failed: %s\n", meta->dbname);
    /* Real db is mid-transaction but the model isn't - abort the real txn
     * so ns_simul_close doesn't trip over a half-open txn. Result of the
     * rollback is irrelevant: we're failing either way. */
    nsdb_rollback (meta->db, meta->tx);
    meta->tx                        = NULL;
    meta->results.inner.record_type = RS_FAILURE;
    return;
  }

  meta->results.inner.record_type = RS_SUCCESS;

  // FIX(#3): don't refresh_cur() here - with tx set, active_var() would
  // read cur_working, which is stale from a previous txn. Derive the
  // working cursor from the committed cursor instead.
  if (meta->cur_committed != NULL) {
    meta->cur_working = mem_vhmap_get (meta->working, meta->cur_committed->var.vname);
    ASSERT (meta->cur_working);
  } else {
    meta->cur_working = NULL;
  }

  // FIX(#8): snapshot metrics so rollback/crash can restore them
  meta->txn_saved_nvars         = meta->results.nvars;
  meta->txn_saved_tracked_bytes = meta->results.tracked_bytes;
}

static void
nss_commit_txn (struct ns_simulation *meta)
{
  DBG_ASSERT (ns_simulation, meta);
  ASSERT (meta->tx);
  ASSERT (meta->working != NULL);

  nss_clock_step (meta);
  err_t ret                          = nsdb_commit (meta->db, meta->tx);
  meta->results.inner.op_duration_ms = nss_clock_step (meta);

  if (ret < 0) {
    i_log_failure ("nsdb_commit failed: %s\n", meta->dbname);
    /* No further steps will run - just drop the working map so close()
     * doesn't double-account, and clear tx so close() doesn't try to
     * commit a dead txn again. */
    mem_vhmap_free (meta->working);
    meta->working                   = NULL;
    meta->tx                        = NULL;
    meta->cur_working               = NULL;
    meta->results.inner.record_type = RS_FAILURE;
    return;
  }

  meta->results.inner.record_type = RS_SUCCESS;

  mem_vhmap_free (meta->committed);
  meta->committed     = meta->working;
  meta->working       = NULL;
  meta->tx            = NULL;

  // FIX(#2): the old refresh_cur() dereferenced cur_committed, which
  // pointed into the map we just freed. cur_working already points into
  // the surviving (promoted) map - just hand it over.
  meta->cur_committed = meta->cur_working;
  meta->cur_working   = NULL;

  // Did the commit actually persist the txn's size changes? (Data content
  // is READ_ALL's job; this catches a commit that dropped a tail insert.)
  VERIFY_EFFECT (meta, active_var (meta) == NULL ? 0 : nss_verify_len (meta, active_var (meta)));
}

static void
nss_rollback_txn (struct ns_simulation *meta)
{
  DBG_ASSERT (ns_simulation, meta);
  ASSERT (meta->tx);
  ASSERT (meta->working != NULL);

  nss_clock_step (meta);
  // FIX(#5): pass the actual transaction, matching nsdb_commit
  err_t ret                          = nsdb_rollback (meta->db, meta->tx);
  meta->results.inner.op_duration_ms = nss_clock_step (meta);

  /* Failed or not, the txn is over from the harness's perspective - free
   * the working map and clear tx either way so close() sees sane state.
   * Only the record_type differs. */
  mem_vhmap_free (meta->working);
  meta->working               = NULL;
  meta->tx                    = NULL;

  // FIX(#3): cur_working pointed into the freed map - drop it. The
  // committed cursor is untouched by the txn, so no refresh is needed.
  meta->cur_working           = NULL;

  // FIX(#8): restore metrics to their pre-txn values
  meta->results.nvars         = meta->txn_saved_nvars;
  meta->results.tracked_bytes = meta->txn_saved_tracked_bytes;

  if (ret < 0) {
    i_log_failure ("nsdb_rollback failed: %s\n", meta->dbname);
    meta->results.inner.record_type = RS_FAILURE;
    return;
  }

  meta->results.inner.record_type = RS_SUCCESS;

  // Did the rollback actually revert? The db's size for the active variable
  // must now match the *committed* reference again.
  VERIFY_EFFECT (meta, active_var (meta) == NULL ? 0 : nss_verify_len (meta, active_var (meta)));
}

static void
nss_crash_and_reopen (struct ns_simulation *meta)
{
  DBG_ASSERT (ns_simulation, meta);
  nss_clock_step (meta);
  err_t ret = nsdb_crash (meta->db);
  if (ret == 0) {
    meta->db = nsdb_open (meta->dbname);
  }
  meta->results.inner.op_duration_ms = nss_clock_step (meta);

  if (ret < 0 || meta->db == NULL) {
    i_log_failure (
        "crash_and_reopen failed (%s): %s\n",
        ret < 0 ? "crash" : "reopen",
        meta->dbname
    );
    meta->results.inner.record_type = RS_FAILURE;
    return;
  }

  meta->results.inner.record_type = RS_SUCCESS;

  // If we were in the middle of a transaction
  // revert back to the previous one
  // FIX(#4): clear tx/cur_working *before* anything consults
  // active_db()/active_var() - the old code freed `working` and then
  // called refresh_cur() while tx was still set, dereferencing a
  // dangling cursor against a NULL map.
  if (meta->working) {
    ASSERT (meta->tx);

    mem_vhmap_free (meta->working);
    meta->working               = NULL;
    meta->cur_working           = NULL;

    // FIX(#8): the txn's metric changes died with the crash
    meta->results.nvars         = meta->txn_saved_nvars;
    meta->results.tracked_bytes = meta->txn_saved_tracked_bytes;
  }

  meta->tx = NULL;
}

static void
nss_close_and_reopen (struct ns_simulation *meta)
{
  DBG_ASSERT (ns_simulation, meta);
  ASSERT (!meta->tx);

  nss_clock_step (meta);
  err_t ret = nsdb_close (meta->db);
  if (ret == 0) {
    meta->db = nsdb_open (meta->dbname);
  }
  meta->results.inner.op_duration_ms = nss_clock_step (meta);

  if (ret < 0 || meta->db == NULL) {
    i_log_failure (
        "close_and_reopen failed (%s): %s\n",
        ret < 0 ? "close" : "reopen",
        meta->dbname
    );
    meta->results.inner.record_type = RS_FAILURE;
    return;
  }

  meta->results.inner.record_type = RS_SUCCESS;

  // FIX(#4): removed the dead `if (meta->working)` block - this function
  // asserts !meta->tx on entry, so a working map here is impossible.
}

static void
nss_create (struct ns_simulation *meta)
{
  DBG_ASSERT (ns_simulation, meta);
  struct mem_vhmap *db     = active_db (meta);
  struct create_op  create = meta->results.inner.operation.create;

  // Create the variable
  struct variable   var    = {
      .vname    = strfcstr (create.vname),
      .dtype    = create.type,
      .nbytes   = 0,
      .rpt_root = 0,
      .var_root = 0,
  };

  err_t ret;
  u64   tdiff;
  {
    nss_clock_step (meta);
    // FIX(#7): run inside the current transaction like every other data
    // op - otherwise a rollback reverts the reference map but the real
    // DB keeps the variable, and the two models diverge permanently.
    ret   = nsdb_fexecute (meta->db, meta->tx, "create %s %s", NULL, create.vname, create.typestr);
    tdiff = nss_clock_step (meta);
  }
  if (ret != 0) {
    free (meta->results.inner.operation.create.vname);
    free (meta->results.inner.operation.create.typestr);
    allocator_free (&meta->alloc);
    nss_finish_op (meta, tdiff, RS_FAILURE);
    return;
  }

  nss_finish_op (meta, tdiff, RS_SUCCESS);
  meta->results.nvars += 1;

  // Create the variable in the reference side
  // NOTE(#10): this assumes mem_vhmap_add deep-copies `var` (vname buffer
  // and dtype). If it doesn't, the frees below are use-after-free bombs
  // for the later type_byte_size(active_var(...)->var.dtype) calls in
  // nssr_pre_op - verify the ownership contract.
  struct var_with_data *v = mem_vhmap_add (db, &var, NULL);
  if (v == NULL) {
    // Real db has the variable, the model doesn't - permanent divergence,
    // but the contract says nothing runs after a failure anyway.
    i_log_failure ("mem_vhmap_add failed: %s\n", create.vname);
    free (meta->results.inner.operation.create.vname);
    free (meta->results.inner.operation.create.typestr);
    allocator_free (&meta->alloc);
    meta->results.inner.record_type = RS_FAILURE;
    return;
  }

  // If there is no variable - then set it to this one
  if (active_var (meta) == NULL) {
    set_active_var (meta, mem_vhmap_get (db, var.vname));
  }

  // Did the create actually land? `get` must succeed and report the same
  // byte count as the (empty) reference entry.
  VERIFY_EFFECT (meta, nss_verify_len (meta, v));

  free (meta->results.inner.operation.create.vname);
  free (meta->results.inner.operation.create.typestr);
  allocator_free (&meta->alloc);
}

static void
nss_switch (struct ns_simulation *meta)
{
  DBG_ASSERT (ns_simulation, meta);
  struct mem_vhmap     *db   = active_db (meta);
  struct var_with_data *next = mem_vhmap_random (db);

  err_t                 ret;
  u64                   tdiff;

  // FIX(#6): initialize - if nsdb_fexecute fails before assigning it,
  // the failure path passed garbage to nsdb_var_free.
  struct nsdb_var      *var = NULL;
  {
    nss_clock_step (meta);
    // FIX(#7): read through the txn so we observe txn-local state (e.g.
    // a variable created earlier in this same uncommitted txn).
    ret   = nsdb_fexecute (meta->db, meta->tx, "get %.*s", &var, strfmt (&next->var.vname));
    tdiff = nss_clock_step (meta);
  }

  nsdb_var_free (meta->db, var);

  if (ret != 0) {
    i_log_failure ("switch: get failed: %.*s\n", strfmt (&next->var.vname));
    nss_finish_op (meta, tdiff, RS_FAILURE);
    return;
  }

  nss_finish_op (meta, tdiff, RS_SUCCESS);
  set_active_var (meta, next);

  // The `get` above proved existence but its payload was discarded - check
  // the db's idea of this variable's size against the reference entry we
  // just switched to.
  VERIFY_EFFECT (meta, nss_verify_len (meta, next));
}

static void
nss_delete (struct ns_simulation *meta)
{
  DBG_ASSERT (ns_simulation, meta);
  ASSERT (active_var (meta) != NULL);

  err_t ret;
  u64   tdiff;

  {
    nss_clock_step (meta);
    // FIX(minor): vname is a counted string everywhere else (%.*s +
    // strfmt); .data isn't guaranteed NUL-terminated, so %s over-read.
    ret = nsdb_fexecute (
        meta->db,
        meta->tx,
        "delete %.*s",
        NULL,
        strfmt (&active_var (meta)->var.vname)
    );
    tdiff = nss_clock_step (meta);
  }

  if (ret < 0) {
    i_log_failure ("delete failed: %.*s\n", strfmt (&active_var (meta)->var.vname));
    nss_finish_op (meta, tdiff, RS_FAILURE);
    return;
  }

  nss_finish_op (meta, tdiff, RS_SUCCESS);
  meta->results.nvars -= 1;
  // FIX(minor): the deleted variable's bytes are no longer tracked
  meta->results.tracked_bytes -= (u32)ext_array_get_len (&active_var (meta)->data);

  // Did the delete actually land? `get` must now fail. Checked *before*
  // mem_vhmap_remove because the name string lives in the map entry.
  VERIFY_EFFECT (meta, nss_verify_gone (meta, active_var (meta)));

  mem_vhmap_remove (active_db (meta), active_var (meta)->var.vname);
  set_active_var (meta, mem_vhmap_random (active_db (meta)));
}

static void
nss_insert (struct ns_simulation *meta)
{
  DBG_ASSERT (ns_simulation, meta);
  struct insert_op op    = meta->results.inner.operation.insert;
  b_size           ofst  = op.ofst;
  b_size           len   = op.nelems;
  t_size           tsize = op.tsize;

  // Create the data to insert
  // FIX(minor): keep the byte math in b_size instead of int to avoid
  // silent truncation on large arrays.
  b_size           blen  = len * (b_size)tsize;
  u8              *data  = random_data (blen);
  if (data == NULL) {
    meta->results.inner.record_type = RS_FAILURE;
    return;
  }

  // Timed section
  err_t ret;
  u64   tdiff;
  {
    nss_clock_step (meta);
    ret = nsdb_fexecute (
        meta->db,
        meta->tx,
        "insert %.*s %" PRb_size " %" PRb_size "",
        data,
        strfmt (&active_var (meta)->var.vname),
        ofst,
        len
    );
    tdiff = nss_clock_step (meta);
  }

  if (ret != (sb_size)len) {
    i_log_failure (
        "insert failed: ofst=%" PRb_size " nelems=%" PRb_size " ret=%lld\n",
        ofst,
        len,
        (long long)ret
    );
    free (data);
    nss_finish_op (meta, tdiff, RS_FAILURE);
    return;
  }

  nss_finish_op (meta, tdiff, RS_SUCCESS);
  meta->results.tracked_bytes += (u32)blen;

  // Do the reference side
  if (ext_array_insert (
          &active_var (meta)->data,
          (u32)(ofst * (b_size)tsize),
          data,
          (u32)blen,
          NULL
      )
      != (i64)blen) {
    i_log_failure (
        "insert: reference ext_array_insert failed: ofst=%" PRb_size " blen=%" PRb_size "\n",
        ofst,
        blen
    );
    meta->results.inner.record_type = RS_FAILURE;
    free (data);
    return;
  }

  // Did the insert actually land? Length must have grown by nelems, and the
  // inserted range must read back as the bytes we sent (stride 1: inserts
  // are contiguous).
  VERIFY_EFFECT (meta, nss_verify_len (meta, active_var (meta)));
  VERIFY_EFFECT (meta, nss_verify_readback (meta, ofst, 1, len, tsize, data));

  free (data);
}

static void
nss_remove (struct ns_simulation *meta)
{
  DBG_ASSERT (ns_simulation, meta);
  struct remove_op op      = meta->results.inner.operation.remove;
  // FIX(minor): b_size locals instead of int - no narrowing
  b_size           ofst    = op.ofst;
  b_size           stride  = op.stride;
  b_size           len     = op.nelems;
  t_size           tsize   = op.tsize;

  size_t           buf_sz  = (size_t)len * tsize;
  uint8_t         *db_buf  = calloc (1, buf_sz);
  uint8_t         *ref_buf = calloc (1, buf_sz);
  if (db_buf == NULL || ref_buf == NULL) {
    i_log_failure ("remove: calloc failed, buf_sz=%zu\n", buf_sz);
    free (db_buf);
    free (ref_buf);
    meta->results.inner.record_type = RS_FAILURE;
    return;
  }

  // Timed section
  err_t ret;
  u64   tdiff;
  {
    nss_clock_step (meta);
    ret = nsdb_fexecute (
        meta->db,
        meta->tx,
        "remove %.*s[%" PRb_size ":%" PRb_size ":%" PRb_size "]",
        db_buf,
        strfmt (&active_var (meta)->var.vname),
        ofst,
        ofst + len * stride,
        stride
    );
    tdiff = nss_clock_step (meta);
  }

  /* ret < 0 is an error; 0 <= ret < len is a *short remove* - the db only
   * removed part of the slice while claiming overall success. Both are
   * failures. The old `ret < 0` check let short ops through, leaving the
   * tail of db_buf as calloc zeros - which then showed up downstream as
   * "the last element read back wrong". */
  if (ret != (sb_size)len) {
    i_log_failure (
        "remove %s: ret=%lld expected=%" PRb_size " (ofst=%" PRb_size " stride=%" PRb_size ")\n",
        ret < 0 ? "failed" : "was short",
        (long long)ret,
        len,
        ofst,
        stride
    );
    free (db_buf);
    free (ref_buf);
    nss_finish_op (meta, tdiff, RS_FAILURE);
    return;
  }

  nss_finish_op (meta, tdiff, RS_SUCCESS);
  meta->results.tracked_bytes -= (u32)(len * tsize);

  // Do the reference
  struct stride str = to_block_stride (ofst, stride, len);
  u64           got = ext_array_remove (&active_var (meta)->data, str, tsize, ref_buf);
  if (got != len) {
    i_log_failure (
        "remove: reference removed %lld, expected %" PRb_size " (ofst=%" PRb_size
        " stride=%" PRb_size ")\n",
        (long long)got,
        len,
        ofst,
        stride
    );
    free (db_buf);
    free (ref_buf);
    meta->results.inner.record_type = RS_FAILURE;
    return;
  }

  // Check that the two buffers are the same
  if (memcmp (db_buf, ref_buf, buf_sz) != 0) {
    size_t i = 0;
    while (i < buf_sz && db_buf[i] == ref_buf[i]) {
      ++i;
    }
    i_log_failure (
        "remove data mismatch at byte %zu (elem %zu): db=0x%02x ref=0x%02x "
        "(ofst=%" PRb_size " stride=%" PRb_size " nelems=%" PRb_size " tsize=%" PRIu64 ")\n",
        i,
        i / tsize,
        db_buf[i],
        ref_buf[i],
        ofst,
        stride,
        len,
        (u64)tsize
    );
    free (db_buf);
    free (ref_buf);
    meta->results.inner.record_type = RS_FAILURE;
    return;
  }

  // The removed bytes matched - but did the db actually *shrink* by the
  // same amount? (Comparing extracted bytes says nothing about the
  // residual array.)
  VERIFY_EFFECT (meta, nss_verify_len (meta, active_var (meta)));

  free (db_buf);
  free (ref_buf);
}

static void
nss_read (struct ns_simulation *meta)
{
  DBG_ASSERT (ns_simulation, meta);
  struct read_op op      = meta->results.inner.operation.read;
  b_size         ofst    = op.ofst;
  b_size         stride  = op.stride;
  b_size         len     = op.nelems;
  t_size         tsize   = op.tsize;

  // Get the true size of the buffer
  size_t         buf_sz  = (size_t)len * tsize;
  uint8_t       *db_buf  = calloc (1, buf_sz);
  uint8_t       *ref_buf = calloc (1, buf_sz);
  if (db_buf == NULL || ref_buf == NULL) {
    i_log_failure ("read: calloc failed, buf_sz=%zu\n", buf_sz);
    free (db_buf);
    free (ref_buf);
    meta->results.inner.record_type = RS_FAILURE;
    return;
  }

  err_t ret;
  u64   tdiff;
  {
    nss_clock_step (meta);
    ret = nsdb_fexecute (
        meta->db,
        meta->tx,
        "read %.*s[%" PRb_size ":%" PRb_size ":%" PRb_size "]",
        db_buf,
        strfmt (&active_var (meta)->var.vname),
        ofst,
        ofst + len * stride,
        stride
    );
    tdiff = nss_clock_step (meta);
  }

  /* Same short-op logic as remove: a read that fills 2 of 3 elements and
   * returns 2 used to pass the old `ret < 0` check, leaving element 3 as
   * calloc zeros in db_buf and getting reported as a *data* mismatch on
   * the last element instead of a short read. */
  if (ret != (sb_size)len) {
    i_log_failure (
        "read %s: ret=%lld expected=%" PRb_size " (ofst=%" PRb_size " stride=%" PRb_size ")\n",
        ret < 0 ? "failed" : "was short",
        (long long)ret,
        len,
        ofst,
        stride
    );
    free (db_buf);
    free (ref_buf);
    nss_finish_op (meta, tdiff, RS_FAILURE);
    return;
  }

  nss_finish_op (meta, tdiff, RS_SUCCESS);

  // Do the reference side
  // FIX(#1): this was ext_array_remove - every successful READ was
  // deleting the read elements from the reference model while the real
  // DB kept them, silently diverging the two. Reads must not mutate.
  struct stride str = to_block_stride (ofst, stride, len);
  i64           got = ext_array_read (&active_var (meta)->data, str, tsize, ref_buf);
  if (got != (i64)len) {
    i_log_failure ("read: reference read %lld, expected %" PRb_size "\n", (long long)got, len);
    free (db_buf);
    free (ref_buf);
    meta->results.inner.record_type = RS_FAILURE;
    return;
  }

  // Check that the two buffers are the same
  if (memcmp (db_buf, ref_buf, buf_sz) != 0) {
    size_t i = 0;
    while (i < buf_sz && db_buf[i] == ref_buf[i]) {
      ++i;
    }
    i_log_failure (
        "read data mismatch at byte %zu (elem %zu): db=0x%02x ref=0x%02x "
        "(ofst=%" PRb_size " stride=%" PRb_size " nelems=%" PRb_size " tsize=%" PRIu64 ")\n",
        i,
        i / tsize,
        db_buf[i],
        ref_buf[i],
        ofst,
        stride,
        len,
        (u64)tsize
    );
    free (db_buf);
    free (ref_buf);
    meta->results.inner.record_type = RS_FAILURE;
    return;
  }

  free (db_buf);
  free (ref_buf);
}

static void
nss_write (struct ns_simulation *meta)
{
  DBG_ASSERT (ns_simulation, meta);
  struct write_op op     = meta->results.inner.operation.write;
  b_size          ofst   = op.ofst;
  b_size          stride = op.stride;
  b_size          len    = op.nelems;
  t_size          tsize  = op.tsize;

  b_size          blen   = len * tsize;
  uint8_t        *data   = random_data (blen);
  if (data == NULL) {
    meta->results.inner.record_type = RS_FAILURE;
    return;
  }

  err_t ret;
  u64   tdiff;
  {
    nss_clock_step (meta);
    ret = nsdb_fexecute (
        meta->db,
        meta->tx,
        "write %.*s[%" PRb_size ":%" PRb_size ":%" PRb_size "]",
        data,
        strfmt (&active_var (meta)->var.vname),
        ofst,
        ofst + len * stride,
        stride
    );
    tdiff = nss_clock_step (meta);
  }

  /* A short write is the nastiest of the three: the db claims success, the
   * missing tail elements keep their OLD values, and nothing notices until
   * a later READ overlaps them. This is the exact signature of "read of a
   * 3-element slice returns an invalid 3rd element". */
  if (ret != (sb_size)len) {
    i_log_failure (
        "write %s: ret=%lld expected=%" PRb_size " (ofst=%" PRb_size " stride=%" PRb_size ")\n",
        ret < 0 ? "failed" : "was short",
        (long long)ret,
        len,
        ofst,
        stride
    );
    free (data);
    nss_finish_op (meta, tdiff, RS_FAILURE);
    return;
  }

  nss_finish_op (meta, tdiff, RS_SUCCESS);

  // Do the reference side
  struct stride str = to_block_stride (ofst, stride, len);
  u64           got = ext_array_write (&active_var (meta)->data, str, tsize, data);
  if (got != (u64)len) {
    i_log_failure (
        "write: reference wrote %llu, expected %" PRb_size "\n",
        (unsigned long long)got,
        len
    );
    free (data);
    meta->results.inner.record_type = RS_FAILURE;
    return;
  }

  // Did the write actually land? Read the same slice back and compare
  // against the buffer we just sent.
  VERIFY_EFFECT (meta, nss_verify_readback (meta, ofst, stride, len, tsize, data));

  free (data);
}

/******************************************************************************
 * SECTION: Main Api
 ******************************************************************************/

struct ns_simulation *
ns_simul_open (
    u64         seed,
    const char *commit_hash,
    u64         sequence_id,
    const char *dbname,
    int         max_insert_len,
    float       sample_space_prob
)
{
  ASSERT (sample_space_prob >= 0 && sample_space_prob <= 1);
  ASSERT (max_insert_len > 0);

  error e = error_create ();

  if (nsdb_cleanup (dbname) < 0) {
    i_log_failure ("nsdb_cleanup failed: %s\n", dbname);
    return NULL;
  }

  struct ns_simulation *ret = malloc (sizeof *ret);
  if (ret == NULL) {
    i_log_failure ("malloc failed for simulation state\n");
    return NULL;
  }

  *ret = (struct ns_simulation){
      .committed         = mem_vhmap_create (default_mem (), NULL),
      .working           = NULL,
      .cur_committed     = NULL,
      .cur_working       = NULL,
      .db                = nsdb_open (dbname),
      .tx                = NULL,
      .dbname            = dbname,
      .max_insert_len    = max_insert_len,
      .sample_space_prob = sample_space_prob,
  };

  // Error handling
  if (ret->committed == NULL) {
    i_log_failure ("mem_vhmap_create failed: %s\n", dbname);
    if (ret->db) {
      nsdb_close (ret->db);
    }
    free (ret);
    return NULL;
  }

  if (ret->db == NULL) {
    i_log_failure ("nsdb_open failed: %s\n", dbname);
    mem_vhmap_free (ret->committed);
    free (ret);
    return NULL;
  }

  ret->results = create_nssr (dbname, seed, commit_hash, sequence_id, &e);

  for (int i = 0; i < NSS_AT_LEN; ++i) {
    ret->enabled[i] = 1;
  }

  nss_set_allowed (ret);
  // FIX(#9): `start` is the single epoch; everything (clock_step, the
  // printer's clock - start) measures against it. The separate,
  // never-initialized start_time field is gone.
  ret->results.start = i_timer_now_ns (&ret->timer);
  ret->results.clock = ret->results.start;

  return ret;
}

int
ns_simul_close (struct ns_simulation *meta)
{
  DBG_ASSERT (ns_simulation, meta);
  int rc = 0;

  if (meta->tx) {
    nss_commit_txn (meta);
    if (meta->results.inner.record_type == RS_FAILURE) {
      i_log_failure ("final commit failed on close: %s\n", meta->dbname);
      rc = -1;
      /* keep going - still release everything */
    }
  }

  if (nsdb_close (meta->db) < 0) {
    i_log_failure ("nsdb_close failed: %s\n", meta->dbname);
    rc = -1;
  }

  if (meta->committed) {
    mem_vhmap_free (meta->committed);
  }
  if (meta->working) {
    mem_vhmap_free (meta->working);
  }
  free (meta);

  return rc;
}

static enum ns_action_type
nss_get_random_action (struct ns_simulation *meta)
{
  DBG_ASSERT (ns_simulation, meta);
  while (true) {
    // Count the number of allowed actions
    int len = 0;
    for (int i = 0; i < NSS_AT_LEN; ++i) {
      len += meta->allowed[i];
    }

    // There are no available actions -
    // re roll the enabled actions and
    // try again
    if (len == 0) {
      nss_set_random_enabled (meta);
      // FIX(minor): allowed[] is derived from enabled[]; without
      // recomputing it, this loop spins forever on the same zeros.
      nss_set_allowed (meta);
      continue;
    }

    // Pick the n-th allowed action
    int next   = rand () % len;
    int index  = 0;
    int choice = 0;
    for (; index < NSS_AT_LEN; ++index) {
      if (meta->allowed[index]) {
        if (choice == next) {
          break;
        } else {
          choice++;
        }
      }
    }

    enum ns_action_type action = (enum ns_action_type)index;

    return action;
  }
}

static void
nssr_pre_op (struct ns_simulation *meta)
{
  DBG_ASSERT (ns_simulation, meta);
  // Set to 0
  memset (&meta->results.inner, 0, sizeof (meta->results.inner));

  // Choose a random action
  enum ns_action_type action      = nss_get_random_action (meta);
  meta->results.inner.record_type = RS_PRE;

  switch (action) {
    case NSS_BEGIN_TXN: {
      meta->results.inner.operation.op_type = NSS_BEGIN_TXN;
      break;
    }
    case NSS_COMMIT_TXN: {
      meta->results.inner.operation.op_type = NSS_COMMIT_TXN;
      break;
    }
    case NSS_ROLLBACK_TXN: {
      meta->results.inner.operation.op_type = NSS_ROLLBACK_TXN;
      break;
    }
    case NSS_CRASH_AND_REOPEN: {
      meta->results.inner.operation.op_type = NSS_CRASH_AND_REOPEN;
      break;
    }
    case NSS_CLOSE_AND_REOPEN: {
      meta->results.inner.operation.op_type = NSS_CLOSE_AND_REOPEN;
      break;
    }
    case NSS_CREATE: {
      meta->results.inner.operation.op_type = NSS_CREATE;
      error e                               = error_create ();

      // Generate the variable name
      if (gen_new_var (meta, &meta->results.inner.operation.create, &e)) {
        // Pre-op failure: record it and let the caller stop, same
        // contract as an execution failure. Nothing was created on
        // either side so no cleanup beyond gen_new_var's own.
        i_log_failure ("gen_new_var failed\n");
        error_log_consume (&e);
        meta->results.inner.record_type = RS_FAILURE;
        return;
      }
      break;
    }
    case NSS_SWITCH: {
      meta->results.inner.operation.op_type = NSS_SWITCH;
      break;
    }
    case NSS_DELETE: {
      meta->results.inner.operation.op_type = NSS_DELETE;

      // Fill the payload - the JSON printer reads tsize/len for DELETE
      t_size tsize                          = type_byte_size (active_var (meta)->var.dtype);
      b_size blen                           = ext_array_get_len (&active_var (meta)->data);
      ASSERT (blen % tsize == 0);

      meta->results.inner.operation.delete = (struct delete_op){
          .tsize = tsize,
          .len   = blen / tsize,
      };
      break;
    }
    case NSS_INSERT: {
      meta->results.inner.operation.op_type = NSS_INSERT;

      t_size tsize                          = type_byte_size (active_var (meta)->var.dtype);
      b_size blen                           = ext_array_get_len (&active_var (meta)->data);

      ASSERT (blen % tsize == 0);

      // FIX(minor): a deep random type can have tsize > max_insert_len,
      // making the upper bound 0 and the range [1, 0] invalid. Always
      // allow at least one element.
      b_size max_nelems = (b_size)meta->max_insert_len / tsize;
      if (max_nelems == 0) {
        max_nelems = 1;
      }

      meta->results.inner.operation.insert = (struct insert_op){
          .tsize  = tsize,
          .ofst   = randu32r (0, blen / tsize),
          .nelems = randu32r (1, max_nelems),
          .len    = blen / tsize,
      };
      break;
    }
    case NSS_REMOVE: {
      meta->results.inner.operation.op_type = NSS_REMOVE;

      t_size tsize                          = type_byte_size (active_var (meta)->var.dtype);
      b_size ofst, stride, len;
      b_size blen = ext_array_get_len (&active_var (meta)->data);

      ASSERT (blen % tsize == 0);

      nss_random_slice (blen / tsize, &ofst, &stride, &len);

      meta->results.inner.operation.remove = (struct remove_op){
          .tsize  = tsize,
          .ofst   = ofst,
          .stride = stride,
          .nelems = len,
          .len    = blen / tsize,
      };

      break;
    }
    case NSS_READ: {
      meta->results.inner.operation.op_type = NSS_READ;

      t_size tsize                          = type_byte_size (active_var (meta)->var.dtype);
      b_size ofst, stride, len;
      b_size blen = ext_array_get_len (&active_var (meta)->data);

      ASSERT (blen % tsize == 0);

      nss_random_slice (blen / tsize, &ofst, &stride, &len);

      meta->results.inner.operation.read = (struct read_op){
          .tsize  = tsize,
          .ofst   = ofst,
          .stride = stride,
          .nelems = len,
          .len    = blen / tsize,
      };

      break;
    }
    case NSS_WRITE: {
      meta->results.inner.operation.op_type = NSS_WRITE;

      t_size tsize                          = type_byte_size (active_var (meta)->var.dtype);
      b_size ofst, stride, len;
      b_size blen = ext_array_get_len (&active_var (meta)->data);

      ASSERT (blen % tsize == 0);

      nss_random_slice (blen / tsize, &ofst, &stride, &len);

      meta->results.inner.operation.write = (struct write_op){
          .tsize  = tsize,
          .ofst   = ofst,
          .stride = stride,
          .nelems = len,
          .len    = blen / tsize,
      };

      break;
    }
    default: UNREACHABLE ();
  }

  // FIX(#9): clock is an absolute timestamp (nss_clock_step and the
  // printer both treat it that way). The old code subtracted the
  // never-initialized start_time, which only worked because it happened
  // to be zero.
  meta->results.clock = i_timer_now_ns (&meta->timer);
}

static void
nssr_post_op (struct ns_simulation *meta)
{
  DBG_ASSERT (ns_simulation, meta);
  enum ns_action_type action = meta->results.inner.operation.op_type;

  switch (action) {
    case NSS_BEGIN_TXN: nss_begin_txn (meta); break;
    case NSS_COMMIT_TXN: nss_commit_txn (meta); break;
    case NSS_ROLLBACK_TXN: nss_rollback_txn (meta); break;
    case NSS_CRASH_AND_REOPEN: nss_crash_and_reopen (meta); break;
    case NSS_CLOSE_AND_REOPEN: nss_close_and_reopen (meta); break;
    case NSS_CREATE: nss_create (meta); break;
    case NSS_SWITCH: nss_switch (meta); break;
    case NSS_DELETE: nss_delete (meta); break;
    case NSS_INSERT: nss_insert (meta); break;
    case NSS_REMOVE: nss_remove (meta); break;
    case NSS_READ: nss_read (meta); break;
    case NSS_WRITE: nss_write (meta); break;
    default: UNREACHABLE ();
  }

  READ_ALL_VERIFY (meta, action);
}

struct ns_simul_record *
ns_simul_prepare (struct ns_simulation *meta)
{
  DBG_ASSERT (ns_simulation, meta);
  // Set which actions are allowed
  nss_set_allowed (meta);

  // Do the pre operation
  nssr_pre_op (meta);

  return &meta->results;
}

struct ns_simul_record *
ns_simul_execute (struct ns_simulation *meta)
{
  DBG_ASSERT (ns_simulation, meta);
  // Do the post operation
  nssr_post_op (meta);

  // Increment step
  meta->results.step_number++;

  // Choose a set of randomized actions
  if (randf () <= meta->sample_space_prob) {
    nss_set_random_enabled (meta);
  }

  return &meta->results;
}

static const char *
record_type_str (enum ns_result_record_type t)
{
  switch (t) {
    case RS_PRE: return "PRE";
    case RS_SUCCESS: return "SUCCESS";
    case RS_FAILURE: return "FAILURE";
    default: return "UNKNOWN";
  }
}

static const char *
op_type_str (enum ns_action_type t)
{
  switch (t) {
    case NSS_BEGIN_TXN: return "BEGIN_TXN";
    case NSS_COMMIT_TXN: return "COMMIT_TXN";
    case NSS_ROLLBACK_TXN: return "ROLLBACK_TXN";
    case NSS_CRASH_AND_REOPEN: return "CRASH_AND_REOPEN";
    case NSS_CLOSE_AND_REOPEN: return "CLOSE_AND_REOPEN";
    case NSS_CREATE: return "CREATE";
    case NSS_SWITCH: return "SWITCH";
    case NSS_DELETE: return "DELETE";
    case NSS_INSERT: return "INSERT";
    case NSS_REMOVE: return "REMOVE";
    case NSS_READ: return "READ";
    case NSS_WRITE: return "WRITE";
    case NSS_AT_LEN: return "AT_LEN";
    default: return "UNKNOWN";
  }
}

static double
ns_to_ms (uint64_t ns)
{
  return (double)ns / 1e6;
}

static void
print_json_string (const char *s)
{
  putchar ('"');
  if (s) {
    for (const char *p = s; *p; p++) {
      switch (*p) {
        case '"': fputs ("\\\"", stdout); break;
        case '\\': fputs ("\\\\", stdout); break;
        case '\n': fputs ("\\n", stdout); break;
        case '\t': fputs ("\\t", stdout); break;
        default:
          if ((unsigned char)*p < 0x20) {
            printf ("\\u%04x", *p);
          } else {
            putchar (*p);
          }
      }
    }
  }
  putchar ('"');
}

void
print_ns_simul_record (const struct ns_simul_record *r)
{
  printf ("{");

  printf ("\"seed\":%" PRIu64 ",", r->seed);

  printf ("\"commit_hash\":");
  print_json_string (r->commit_hash);
  printf (",");

  printf ("\"sequence_id\":%" PRIu64 ",", r->sequence_id);
  printf ("\"step_number\":%" PRIu64 ",", r->step_number);
  printf ("\"clock_ms\":%f,", ns_to_ms (r->clock - r->start));
  printf ("\"working_clock_ms\":%f,", ns_to_ms (r->working_clock));
  printf ("\"db_total_size\":%" PRIu64 ",", r->db_total_size);
  printf ("\"nvars\":%" PRIu32 ",", r->nvars);
  printf ("\"tracked_bytes\":%" PRIu32 ",", r->tracked_bytes);
  printf ("\"record_type\":\"%s\",", record_type_str (r->inner.record_type));
  printf ("\"op_type\":\"%s\",", op_type_str (r->inner.operation.op_type));

  printf ("\"operation\":");
  if (r->inner.record_type != RS_PRE) {
    /* Only one union branch is valid at a time, based on op_type. */
    switch (r->inner.operation.op_type) {
      case NSS_CREATE: {
        printf ("{}");
        break;
      }
      case NSS_INSERT: {
        const struct insert_op *op = &r->inner.operation.insert;
        printf (
            "{\"tsize\":%" PRIu64 ",\"ofst\":%" PRIu64 ",\"nelems\":%" PRIu64 ",\"len\":%" PRIu64
            "}",
            (uint64_t)op->tsize,
            (uint64_t)op->ofst,
            (uint64_t)op->nelems,
            (uint64_t)op->len
        );
        break;
      }
      case NSS_READ: {
        const struct read_op *op = &r->inner.operation.read;
        printf (
            "{\"tsize\":%" PRIu64 ",\"ofst\":%" PRIu64 ",\"stride\":%" PRIu64 ",\"nelems\":%" PRIu64
            ",\"len\":%" PRIu64 "}",
            (uint64_t)op->tsize,
            (uint64_t)op->ofst,
            (uint64_t)op->stride,
            (uint64_t)op->nelems,
            (uint64_t)op->len
        );
        break;
      }
      case NSS_REMOVE: {
        const struct remove_op *op = &r->inner.operation.remove;
        printf (
            "{\"tsize\":%" PRIu64 ",\"ofst\":%" PRIu64 ",\"stride\":%" PRIu64 ",\"nelems\":%" PRIu64
            ",\"len\":%" PRIu64 "}",
            (uint64_t)op->tsize,
            (uint64_t)op->ofst,
            (uint64_t)op->stride,
            (uint64_t)op->nelems,
            (uint64_t)op->len
        );
        break;
      }
      case NSS_WRITE: {
        const struct write_op *op = &r->inner.operation.write;
        printf (
            "{\"tsize\":%" PRIu64 ",\"ofst\":%" PRIu64 ",\"stride\":%" PRIu64 ",\"nelems\":%" PRIu64
            ",\"len\":%" PRIu64 "}",
            (uint64_t)op->tsize,
            (uint64_t)op->ofst,
            (uint64_t)op->stride,
            (uint64_t)op->nelems,
            (uint64_t)op->len
        );
        break;
      }
      case NSS_DELETE: {
        const struct delete_op *op = &r->inner.operation.delete;
        printf (
            "{\"tsize\":%" PRIu64 ",\"len\":%" PRIu64 "}",
            (uint64_t)op->tsize,
            (uint64_t)op->len
        );
        break;
      }
      /* No union payload for these. */
      case NSS_BEGIN_TXN:
      case NSS_COMMIT_TXN:
      case NSS_ROLLBACK_TXN:
      case NSS_CRASH_AND_REOPEN:
      case NSS_CLOSE_AND_REOPEN:
      case NSS_SWITCH:
      case NSS_AT_LEN:
      default: printf ("{}"); break;
    }
    printf (",\"op_duration_ms\":%f", ns_to_ms (r->inner.op_duration_ms));
  } else {
    printf ("null,\"op_duration_ms\":null");
  }

  printf ("}\n");
}
