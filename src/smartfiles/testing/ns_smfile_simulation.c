#include "smartfiles/testing/ns_smfile_simulation.h"

#include "core/ns_block_array.h"
#include "core/ns_csx_assert.h"
#include "core/ns_numerics.h"
#include "core/ns_stride.h"
#include "smartfiles/smartfiles.h"

struct smfile_simulation
{
  // Fake database transaction semantics
  struct block_array *committed;
  struct block_array *working;

  // Which actions are turned on
  int                 enabled[SMF_AT_LEN];

  // Which logical actions
  // are available (e.g. you can't
  // commit a non open txn)
  int                 allowed[SMF_AT_LEN];

  smfile_t           *db;
  int                 in_txn;
  const char         *dbname;
  int                 max_insert_len;
  int                 max_size;
  b_size              len;
  float               sample_space_prob;
};

static struct block_array *
active_db (struct smfile_simulation *meta)
{
  return meta->in_txn ? meta->working : meta->committed;
}

static void
smfile_simul_random_slice (int total, int max_size, int *ofst, int *stride, int *len)
{
  ASSERT (total > 0);
  ASSERT (max_size >= 0);

  *ofst         = randu32r (0, total - 1);
  int remaining = total - *ofst;
  *stride       = randu32r (1, remaining);

  int max_len   = (remaining + *stride - 1) / *stride;
  if (max_size < max_len) {
    max_len = max_size;
  }

  *len = (max_len > 0) ? randu32r (0, max_len) : 0;
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
smfile_simul_set_random_enabled (struct smfile_simulation *meta)
{
  int mask = rand () % ((1 << SMF_AT_LEN) - 1) + 1;
  for (int i = 0; i < SMF_AT_LEN; ++i) {
    meta->enabled[i] = (mask >> i) & 1;
  }
}

static void
smfile_simul_set_allowed (struct smfile_simulation *meta)
{
  ASSERT (meta);
  ASSERT (meta->db);
  ASSERT (meta->dbname);

  memset (meta->allowed, 0, sizeof (meta->allowed));

  meta->allowed[SMF_CRASH_REOPEN] = meta->enabled[SMF_CRASH_REOPEN];

  if (!meta->in_txn) {
    meta->allowed[SMF_BEGIN_TXN]    = meta->enabled[SMF_BEGIN_TXN];
    meta->allowed[SMF_CLOSE_REOPEN] = meta->enabled[SMF_CLOSE_REOPEN];
  } else {
    meta->allowed[SMF_COMMIT_TXN]   = meta->enabled[SMF_COMMIT_TXN];
    meta->allowed[SMF_ROLLBACK_TXN] = meta->enabled[SMF_ROLLBACK_TXN];
  }

  meta->allowed[SMF_INSERT] = meta->enabled[SMF_INSERT];

  if (meta->len > 0) {
    meta->allowed[SMF_REMOVE] = meta->enabled[SMF_REMOVE];
    meta->allowed[SMF_READ]   = meta->enabled[SMF_READ];
    meta->allowed[SMF_WRITE]  = meta->enabled[SMF_WRITE];
  }
}

/******************************************************************************
 * SECTION: Concrete Actions
 ******************************************************************************/

static int
smfile_simul_begin_txn (struct smfile_simulation *meta)
{
  ASSERT (!meta->in_txn);
  ASSERT (meta->working == NULL);

  if (smfile_begin (meta->db) < 0) {
    i_log_failure ("smfile_begin failed: %s\n", meta->dbname);
    return -1;
  }

  meta->working = block_array_clone (meta->committed, NULL);

  if (meta->working == NULL) {
    i_log_failure ("block_array_clone failed: %s\n", meta->dbname);
    /* db is now mid-transaction but meta->in_txn is still 0 - abort the
     * real txn too so the two stay in sync. */
    smfile_rollback (meta->db);
    return -1;
  }

  meta->in_txn = 1;

  return 0;
}

static int
smfile_simul_commit_txn (struct smfile_simulation *meta)
{
  ASSERT (meta->in_txn);
  ASSERT (meta->working != NULL);

  if (smfile_commit (meta->db) < 0) {
    i_log_failure ("smfile_commit failed: %s\n", meta->dbname);
    return -1;
  }

  block_array_free (meta->committed);
  meta->committed = meta->working;
  meta->working   = NULL;
  meta->in_txn    = 0;

  return 0;
}

static int
smfile_simul_rollback_txn (struct smfile_simulation *meta)
{
  ASSERT (meta->in_txn);
  ASSERT (meta->working != NULL);

  if (smfile_rollback (meta->db) < 0) {
    i_log_failure ("smfile_rollback failed: %s\n", meta->dbname);
    return -1;
  }

  block_array_free (meta->working);
  meta->working = NULL;
  meta->in_txn  = 0;
  meta->len     = block_array_getlen (meta->committed);

  return 0;
}

static int
smfile_simul_crash_and_reopen (struct smfile_simulation *meta)
{
  if (smfile_crash (meta->db) < 0) {
    i_log_failure ("smfile_crash failed: %s\n", meta->dbname);
    return -1;
  }

  meta->db = smfile_open (meta->dbname);
  if (meta->db == NULL) {
    i_log_failure ("smfile_open failed after crash: %s\n", meta->dbname);
    return -1;
  }

  if (meta->working) {
    block_array_free (meta->working);
    meta->working = NULL;
  }

  meta->in_txn = 0;
  meta->len    = block_array_getlen (meta->committed);

  return 0;
}

static int
smfile_simul_close_and_reopen (struct smfile_simulation *meta)
{
  ASSERT (!meta->in_txn);

  if (smfile_close (meta->db) < 0) {
    i_log_failure ("smfile_close failed: %s\n", meta->dbname);
    return -1;
  }

  meta->db = smfile_open (meta->dbname);
  if (meta->db == NULL) {
    i_log_failure ("smfile_open failed after close: %s\n", meta->dbname);
    return -1;
  }

  if (meta->working) {
    block_array_free (meta->working);
    meta->working = NULL;
  }

  meta->in_txn = 0;
  meta->len    = block_array_getlen (meta->committed);

  return 0;
}

static int
smfile_simul_insert (struct smfile_simulation *meta)
{
  // Random length between 1 and max_insert_len
  int      len  = (rand () % meta->max_insert_len) + 1;

  // Random offset from [0, end]
  int      ofst = rand () % (meta->len + 1);

  // Create insert buffer
  uint8_t *data = malloc ((size_t)len);
  if (data == NULL) {
    i_log_failure ("malloc failed for insert buffer, len=%d\n", len);
    return -1;
  }
  for (int i = 0; i < len; ++i) {
    data[i] = (uint8_t)rand ();
  }

  // Do real insert
  sb_size got = smfile_insert (meta->db, data, ofst, len);
  if (got < 0) {
    i_log_failure ("smfile_insert failed: ofst=%d len=%d\n", ofst, len);
    free (data);
    return -1;
  }

  // Do reference insert
  sb_size actual = block_array_insert (active_db (meta), ofst, data, len, NULL);
  if (actual < 0) {
    i_log_failure ("block_array_insert failed: ofst=%d len=%d\n", ofst, len);
    free (data);
    return -1;
  }

  // Compare the two
  if (got != actual) {
    i_log_failure ("insert mismatch: got=%lld actual=%lld\n", (long long)got, (long long)actual);
    free (data);
    return -1;
  }

  meta->len += len;

  free (data);

  return 0;
}

static int
smfile_simul_remove (struct smfile_simulation *meta)
{
  int ofst, stride, len;
  smfile_simul_random_slice (meta->len, meta->max_size, &ofst, &stride, &len);

  size_t   buf_sz  = (size_t)len;
  uint8_t *db_buf  = calloc (1, buf_sz);
  uint8_t *ref_buf = calloc (1, buf_sz);

  if (db_buf == NULL || ref_buf == NULL) {
    i_log_failure ("calloc failed for remove buffers, len=%d\n", len);
    free (db_buf);
    free (ref_buf);
    return -1;
  }

  // Do real remove
  sb_size got = smfile_remove (meta->db, db_buf, 1, ofst, stride, len);
  if (got < 0) {
    i_log_failure ("smfile_remove failed: ofst=%d stride=%d len=%d\n", ofst, stride, len);
    free (db_buf);
    free (ref_buf);
    return -1;
  }

  // Do reference remove
  struct stride str    = to_block_stride (ofst, stride, len);
  i64           actual = block_array_remove (active_db (meta), str, 1, ref_buf, NULL);
  if (actual < 0) {
    i_log_failure ("block_array_remove failed: ofst=%d stride=%d len=%d\n", ofst, stride, len);
    free (db_buf);
    free (ref_buf);
    return -1;
  }

  // compare the two
  if (got != actual) {
    i_log_failure (
        "remove count mismatch: got=%lld actual=%lld\n",
        (long long)got,
        (long long)actual
    );
    free (db_buf);
    free (ref_buf);
    return -1;
  }
  if (memcmp (db_buf, ref_buf, buf_sz) != 0) {
    i_log_failure ("remove data mismatch: ofst=%d stride=%d len=%d\n", ofst, stride, len);
    free (db_buf);
    free (ref_buf);
    return -1;
  }

  meta->len -= len;

  free (db_buf);
  free (ref_buf);

  return 0;
}

static int
smfile_simul_read (struct smfile_simulation *meta)
{
  int ofst, stride, len;
  smfile_simul_random_slice (meta->len, meta->max_size, &ofst, &stride, &len);

  size_t   buf_sz  = (size_t)len;
  uint8_t *db_buf  = calloc (1, buf_sz);
  uint8_t *ref_buf = calloc (1, buf_sz);

  if (db_buf == NULL || ref_buf == NULL) {
    i_log_failure ("calloc failed for read buffers, len=%d\n", len);
    free (db_buf);
    free (ref_buf);
    return -1;
  }

  // Do real read
  sb_size got = smfile_read (meta->db, db_buf, 1, ofst, stride, len);
  if (got < 0) {
    i_log_failure ("smfile_read failed: ofst=%d stride=%d len=%d\n", ofst, stride, len);
    free (db_buf);
    free (ref_buf);
    return -1;
  }

  // Do reference read
  struct stride str    = to_block_stride (ofst, stride, len);
  i64           actual = block_array_read (active_db (meta), str, 1, ref_buf);
  if (actual < 0) {
    i_log_failure ("block_array_read failed: ofst=%d stride=%d len=%d\n", ofst, stride, len);
    free (db_buf);
    free (ref_buf);
    return -1;
  }

  // compare the two
  if (got != actual) {
    i_log_failure (
        "read count mismatch: got=%lld actual=%lld\n",
        (long long)got,
        (long long)actual
    );
    free (db_buf);
    free (ref_buf);
    return -1;
  }
  if (memcmp (db_buf, ref_buf, buf_sz) != 0) {
    i_log_failure ("read data mismatch: ofst=%d stride=%d len=%d\n", ofst, stride, len);
    free (db_buf);
    free (ref_buf);
    return -1;
  }

  free (db_buf);
  free (ref_buf);

  return 0;
}

static int
smfile_simul_write (struct smfile_simulation *meta)
{
  int ofst, stride, len;
  smfile_simul_random_slice (meta->len, meta->max_size, &ofst, &stride, &len);

  uint8_t *data = malloc ((size_t)len);

  if (data == NULL) {
    i_log_failure ("malloc failed for write buffer, len=%d\n", len);
    return -1;
  }
  for (int i = 0; i < len; ++i) {
    data[i] = (uint8_t)rand ();
  }

  // Do real write
  sb_size got = smfile_write (meta->db, data, 1, ofst, stride, len);
  if (got < 0) {
    i_log_failure ("smfile_write failed: ofst=%d stride=%d len=%d\n", ofst, stride, len);
    free (data);
    return -1;
  }

  // To reference write
  struct stride str    = to_block_stride (ofst, stride, len);
  u64           actual = block_array_write (active_db (meta), str, 1, data);
  if (actual == (u64)-1) {
    i_log_failure ("block_array_write failed: ofst=%d stride=%d len=%d\n", ofst, stride, len);
    free (data);
    return -1;
  }

  if (got != (i64)actual) {
    i_log_failure (
        "write count mismatch: got=%lld actual=%lld\n",
        (long long)got,
        (long long)actual
    );
    free (data);
    return -1;
  }

  free (data);

  return 0;
}

/******************************************************************************
 * SECTION: Main Api
 ******************************************************************************/

struct smfile_simulation *
smf_simul_open (
    int         initial_enabled[SMF_AT_LEN],
    const char *dbname,
    int         max_insert_len,
    int         max_size,
    float       sample_space_prob
)
{
  ASSERT (sample_space_prob >= 0 && sample_space_prob <= 1);
  ASSERT (max_size >= 0);

  struct smfile_simulation *ret = malloc (sizeof *ret);
  if (ret == NULL) {
    i_log_failure ("malloc failed for simulation state\n");
    return NULL;
  }

  if (smfile_cleanup (dbname) < 0) {
    i_log_failure ("smfile_cleanup failed: %s\n", dbname);
    free (ret);
    return NULL;
  }

  *ret = (struct smfile_simulation){
      .committed         = block_array_create (512, default_mem (), NULL),
      .working           = NULL,
      .db                = smfile_open (dbname),
      .in_txn            = 0,
      .dbname            = dbname,
      .max_insert_len    = max_insert_len,
      .max_size          = max_size,
      .len               = 0,
      .sample_space_prob = sample_space_prob,
  };

  if (ret->committed == NULL) {
    i_log_failure ("block_array_create failed: %s\n", dbname);
    panic ("Failed to initialize");
  }

  if (ret->db == NULL) {
    i_log_failure ("smfile_open failed: %s\n", dbname);
    panic ("Failed to initialize");
  }

  memcpy (ret->enabled, initial_enabled, SMF_AT_LEN * sizeof (int));
  smfile_simul_set_allowed (ret);

  return ret;
}

int
smfile_simul_close (struct smfile_simulation *meta)
{
  if (meta->in_txn) {
    if (smfile_simul_commit_txn (meta) < 0) {
      i_log_failure ("final commit failed on close: %s\n", meta->dbname);
      return -1;
    }
  }

  if (smfile_close (meta->db) < 0) {
    i_log_failure ("smfile_close failed: %s\n", meta->dbname);
    return -1;
  }

  if (meta->committed) {
    block_array_free (meta->committed);
  }
  if (meta->working) {
    block_array_free (meta->working);
  }
  free (meta);

  return 0;
}

static const char *const smfile_action_names[SMF_AT_LEN] = {
    [SMF_BEGIN_TXN]    = "BEGIN_TXN",
    [SMF_COMMIT_TXN]   = "COMMIT_TXN",
    [SMF_ROLLBACK_TXN] = "ROLLBACK_TXN",
    [SMF_CRASH_REOPEN] = "CRASH_REOPEN",
    [SMF_CLOSE_REOPEN] = "CLOSE_REOPEN",
    [SMF_INSERT]       = "INSERT",
    [SMF_REMOVE]       = "REMOVE",
    [SMF_READ]         = "READ",
    [SMF_WRITE]        = "WRITE",
};

static void
smfile_simul_print_state (const struct smfile_simulation *meta)
{
  i_log_info ("=== SMF Swarm Test State ===\n");
  if (meta == NULL) {
    i_log_info ("  (meta is NULL)\n");
    return;
  }
  i_log_info ("  dbname:         %s\n", meta->dbname);
  i_log_info ("  len:            %" PRb_size " elems\n", meta->len);
  i_log_info ("  max_insert_len: %d\n", meta->max_insert_len);
  i_log_info ("  max_size:       %d\n", meta->max_size);
  i_log_info ("  in_txn:         %s\n", meta->in_txn ? "yes" : "no");
  i_log_info ("  committed:      %s\n", meta->committed ? "present" : "<null>");
  i_log_info ("  working:        %s\n", meta->working ? "present" : "<null>");
  i_log_info ("  sample_space_p: %.3f\n", (double)meta->sample_space_prob);
  i_log_info ("  Actions             enabled   allowed\n");

  for (int i = 0; i < SMF_AT_LEN; ++i) {
    i_log_info (
        "    %-18s   %-3s       %-3s\n",
        smfile_action_names[i],
        meta->enabled[i] ? "yes" : "no",
        meta->allowed[i] ? "yes" : "no"
    );
  }
}

int
smfile_simul_step (struct smfile_simulation *meta)
{
  smfile_simul_print_state (meta);

  /* Count allowed actions */
  int len = 0;
  for (int i = 0; i < SMF_AT_LEN; ++i) {
    len += meta->allowed[i];
  }

  /* If the smfile_swarm has masked everything off, re-roll and try again next
   * step rather than divide by zero. */
  if (len == 0) {
    i_log_info ("No allowed actions - re-rolling enabled mask\n");
    smfile_simul_set_random_enabled (meta);
    smfile_simul_set_allowed (meta);
    return 0;
  }

  /* Pick the n-th allowed action */
  int next   = rand () % len;
  int index  = 0;
  int choice = 0;
  for (; index < SMF_AT_LEN; ++index) {
    if (meta->allowed[index]) {
      if (choice == next) {
        break;
      } else {
        choice++;
      }
    }
  }

  enum smfile_action_type action = (enum smfile_action_type)index;
  i_log_info ("-> %s\n", smfile_action_names[action]);

  int rc = 0;
  switch (action) {
    case SMF_BEGIN_TXN: rc = smfile_simul_begin_txn (meta); break;
    case SMF_COMMIT_TXN: rc = smfile_simul_commit_txn (meta); break;
    case SMF_ROLLBACK_TXN: rc = smfile_simul_rollback_txn (meta); break;
    case SMF_CRASH_REOPEN: rc = smfile_simul_crash_and_reopen (meta); break;
    case SMF_CLOSE_REOPEN: rc = smfile_simul_close_and_reopen (meta); break;
    case SMF_INSERT: rc = smfile_simul_insert (meta); break;
    case SMF_REMOVE: rc = smfile_simul_remove (meta); break;
    case SMF_READ: rc = smfile_simul_read (meta); break;
    case SMF_WRITE: rc = smfile_simul_write (meta); break;
    default: {
      UNREACHABLE ();
    }
  }

  if (rc < 0) {
    i_log_failure ("action %s failed\n", smfile_action_names[action]);
    return rc;
  }

  // Choose a set of randomized actions
  if (randf () <= meta->sample_space_prob) {
    smfile_simul_set_random_enabled (meta);
    i_log_info ("Changing Enabled. After:\n");
    smfile_simul_print_state (meta);
  }

  smfile_simul_set_allowed (meta);

  return 0;
}
