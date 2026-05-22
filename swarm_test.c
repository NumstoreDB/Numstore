#define _POSIX_C_SOURCE 200809L /* for strdup */

#include "swarm_test.h"

#include "fake_database.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------------- */
/*  Concrete state                                                        */
/* ---------------------------------------------------------------------- */

/**
 * Runtime state for a swarm test.
 *
 * `committed` always exists. `working` is non-NULL iff a transaction is
 * open. `cur_name` is an owned copy of the currently selected variable's
 * name (or NULL when no variables exist).
 */
struct swarm_test {
  /* Reference model -- transactional wrapper */
  struct fake_database *committed;
  struct fake_database *working; /* NULL outside a txn */

  /* Currently selected variable name (owned). NULL when no vars exist. */
  char *cur_name;

  int enabled[AT_LEN]; /* Swarm subset: which actions are eligible       */
  int allowed[AT_LEN]; /* Actions actually executable now                */

  nsdb_t     *db;
  int         in_txn;
  const char *dbname;
  int         max_insert_len;

  float probability_of_swarm_change;
  float probability_of_full_read;
};

/* ---------------------------------------------------------------------- */
/*  Internal forward declarations                                         */
/* ---------------------------------------------------------------------- */

static void swmt_assert (int result);
static void swmt_set_random_enabled (struct swarm_test *test);
static void swmt_full_validation (struct swarm_test *test);
static void swmt_set_allowed (struct swarm_test *test);

static void swmt_begin_txn (struct swarm_test *meta);
static void swmt_commit_txn (struct swarm_test *meta);
static void swmt_rollback_txn (struct swarm_test *meta);
static void swmt_crash_and_reopen (struct swarm_test *meta);
static void swmt_close_and_reopen (struct swarm_test *meta);
static void swmt_insert (struct swarm_test *meta);
static void swmt_remove (struct swarm_test *meta);
static void swmt_read (struct swarm_test *meta);
static void swmt_write (struct swarm_test *meta);
static void swmt_create (struct swarm_test *meta);
static void swmt_switch (struct swarm_test *meta);
static void swmt_delete (struct swarm_test *meta);

static struct fake_database *active_db (struct swarm_test *meta);
static struct fake_var      *current_var (struct swarm_test *meta);
static int                   current_var_len (const struct fake_var *v);
static void                  fixup_cur_name (struct swarm_test *meta);

static float random_unit_float (void);

/* ---------------------------------------------------------------------- */
/*  Small helpers                                                         */
/* ---------------------------------------------------------------------- */

/** Abort loudly when a DB or reference call returns an unexpected status. */
static void swmt_assert (int result) {
  if (!result) {
    printf ("Failed swarm test\n");
    abort ();
  }
}

/** Uniform float in [0, 1) for probability checks. */
static float random_unit_float (void) { return (float)rand () / (float)RAND_MAX; }

/**
 * Return the fake_database that next op should modify: working if inside
 * a txn, committed otherwise.
 */
static struct fake_database *active_db (struct swarm_test *meta) {
  return meta->in_txn ? meta->working : meta->committed;
}

/**
 * Look up the currently selected variable in the active fake_database.
 * Returns NULL if no current var exists.
 */
static struct fake_var *current_var (struct swarm_test *meta) {
  if (!meta->cur_name) { return NULL; }
  return fake_db_find (active_db (meta), meta->cur_name);
}

/** Element count for a variable (block_array length / elem size). */
static int current_var_len (const struct fake_var *v) {
  return (int)(block_array_getlen (v->data) / (u64)v->elem_size);
}

/**
 * After any operation that may have changed the var set (commit, rollback,
 * crash, delete), ensure `cur_name` points to a variable that still exists.
 * Falls back to the first variable, or NULL if the database is now empty.
 */
static void fixup_cur_name (struct swarm_test *meta) {
  struct fake_database *db = active_db (meta);

  if (meta->cur_name && fake_db_find (db, meta->cur_name)) { return; }

  free (meta->cur_name);
  meta->cur_name = NULL;

  if (fake_db_var_count (db) > 0) {
    meta->cur_name = strdup (fake_db_var_at (db, 0)->name);
    swmt_assert (meta->cur_name != NULL);
  }
}

/* ---------------------------------------------------------------------- */
/*  Swarm selection                                                       */
/* ---------------------------------------------------------------------- */

/**
 * Re-roll the enabled action subset.
 *
 * Given N action types, the swarm action space is the power set minus the
 * empty set: 2^N - 1 non-empty subsets. We sample uniformly by drawing an
 * integer in [1, 2^N) and interpreting each bit as inclusion.
 */
static void swmt_set_random_enabled (struct swarm_test *test) {
  int mask = rand () % ((1 << AT_LEN) - 1) + 1;
  for (int i = 0; i < AT_LEN; ++i) { test->enabled[i] = (mask >> i) & 1; }
}

/**
 * Trigger a full validation pass. Currently calls the DB's self-consistency
 * check; the per-op memcmp in read/remove already cross-checks slice data
 * against the reference model.
 */
static void swmt_full_validation (struct swarm_test *test) { nsdb_validate (test->db); }

/**
 * Recompute which actions can actually fire right now.
 *
 * An action is `allowed` iff (a) it is in the current swarm (`enabled`)
 * AND (b) the test state permits it. CREATE and DELETE are permitted
 * inside a transaction because the fake_database snapshot model gives
 * them transactional semantics for free.
 */
static void swmt_set_allowed (struct swarm_test *test) {
  assert (test);
  assert (test->db);
  assert (test->dbname);

  memset (test->allowed, 0, sizeof (test->allowed));

  struct fake_database *db    = active_db (test);
  int                   nvars = fake_db_var_count (db);

  /* CRASH is always permissible if swarm-enabled */
  test->allowed[CRASH_AND_REOPEN] = test->enabled[CRASH_AND_REOPEN];

  /* CREATE: only constrained by capacity */
  if (nvars < FAKE_DB_MAX_VARS) { test->allowed[CREATE] = test->enabled[CREATE]; }

  /* Txn-state-dependent ops */
  if (!test->in_txn) {
    test->allowed[BEGIN_TXN]        = test->enabled[BEGIN_TXN];
    test->allowed[CLOSE_AND_REOPEN] = test->enabled[CLOSE_AND_REOPEN];
  } else {
    test->allowed[COMMIT_TXN]   = test->enabled[COMMIT_TXN];
    test->allowed[ROLLBACK_TXN] = test->enabled[ROLLBACK_TXN];
  }

  /* Variable-dependent ops */
  struct fake_var *cur = current_var (test);
  if (cur) {
    test->allowed[INSERT] = test->enabled[INSERT];
    if (current_var_len (cur) > 0) {
      test->allowed[REMOVE] = test->enabled[REMOVE];
      test->allowed[READ]   = test->enabled[READ];
      test->allowed[WRITE]  = test->enabled[WRITE];
    }
  }

  /* Need at least two vars to switch or to safely delete */
  if (nvars > 1) {
    test->allowed[DELETE] = test->enabled[DELETE];
    test->allowed[SWITCH] = test->enabled[SWITCH];
  }
}

/* ---------------------------------------------------------------------- */
/*  Lifecycle                                                             */
/* ---------------------------------------------------------------------- */

struct swarm_test *swmt_open (
    float       probability_of_swarm_change,
    float       probability_of_full_read,
    int         start_enabled[AT_LEN],
    const char *dbname,
    int         max_insert_len) {
  struct swarm_test *ret = malloc (sizeof *ret);
  swmt_assert (ret != NULL);

  swmt_assert (nsdb_cleanup (dbname) == 0);

  *ret = (struct swarm_test){
      .committed                   = fake_db_create (),
      .working                     = NULL,
      .cur_name                    = NULL,
      .db                          = nsdb_open (dbname),
      .in_txn                      = 0,
      .dbname                      = dbname,
      .max_insert_len              = max_insert_len,
      .probability_of_swarm_change = probability_of_swarm_change,
      .probability_of_full_read    = probability_of_full_read,
  };

  swmt_assert (ret->committed != NULL);
  swmt_assert (ret->db != NULL);

  memcpy (ret->enabled, start_enabled, AT_LEN * sizeof (int));
  swmt_set_allowed (ret);

  return ret;
}

void swmt_close (struct swarm_test *meta) {
  if (meta->in_txn) { swmt_commit_txn (meta); }
  swmt_assert (nsdb_close (meta->db) == 0);

  fake_db_free (meta->committed);
  fake_db_free (meta->working);
  free (meta->cur_name);
  free (meta);
}

/* ---------------------------------------------------------------------- */
/*  Transaction actions                                                   */
/* ---------------------------------------------------------------------- */

/**
 * Begin a transaction. Snapshots committed -> working so that subsequent
 * mutations are isolated.
 */
static void swmt_begin_txn (struct swarm_test *meta) {
  assert (!meta->in_txn);
  assert (meta->working == NULL);

  swmt_assert (nsdb_begin (meta->db) == 0);

  meta->working = fake_db_clone (meta->committed);
  swmt_assert (meta->working != NULL);
  meta->in_txn = 1;
}

/**
 * Commit the open transaction. The working snapshot becomes the new
 * committed; the old committed is freed.
 */
static void swmt_commit_txn (struct swarm_test *meta) {
  assert (meta->in_txn);
  assert (meta->working != NULL);

  swmt_assert (nsdb_commit (meta->db) == 0);

  fake_db_free (meta->committed);
  meta->committed = meta->working;
  meta->working   = NULL;
  meta->in_txn    = 0;

  fixup_cur_name (meta);
}

/**
 * Rollback the open transaction. Throws away the working snapshot;
 * committed is untouched.
 */
static void swmt_rollback_txn (struct swarm_test *meta) {
  assert (meta->in_txn);

  swmt_assert (nsdb_rollback (meta->db) == 0);

  fake_db_free (meta->working);
  meta->working = NULL;
  meta->in_txn  = 0;

  fixup_cur_name (meta);
}

/**
 * Forcibly crash the database, then reopen it. Reference-side this behaves
 * like a rollback: any in-flight working snapshot is discarded.
 */
static void swmt_crash_and_reopen (struct swarm_test *meta) {
  swmt_assert (nsdb_crash (meta->db) == 0);

  meta->db = nsdb_open (meta->dbname);
  swmt_assert (meta->db != NULL);

  if (meta->working) {
    fake_db_free (meta->working);
    meta->working = NULL;
  }
  meta->in_txn = 0;

  fixup_cur_name (meta);
}

/**
 * Cleanly close and reopen the database. Only permitted outside a txn,
 * so committed is already authoritative and no reference work is needed.
 */
static void swmt_close_and_reopen (struct swarm_test *meta) {
  assert (!meta->in_txn);
  swmt_assert (nsdb_close (meta->db) == 0);

  meta->db = nsdb_open (meta->dbname);
  swmt_assert (meta->db != NULL);
}

/* ---------------------------------------------------------------------- */
/*  Data actions                                                          */
/* ---------------------------------------------------------------------- */

/**
 * Compute a strided [start, step, nelems) slice over the given variable
 * that is non-degenerate (at least one element selected). Outputs are in
 * element units; the caller converts to bytes for the block_array API.
 *
 * Caller must guarantee total > 0.
 */
static void swmt_random_slice (int total, int *ofst, int *stride, int *len) {
  assert (total > 0);

  *ofst         = rand () % total;
  int remaining = total - *ofst;
  *stride       = (rand () % remaining) + 1;

  int max_len = (remaining + *stride - 1) / *stride;
  *len        = (rand () % max_len) + 1;
}

/**
 * Build the byte-units stride descriptor consumed by the block_array API
 * from the element-units triple used by nsdb.
 */
static struct stride to_block_stride (int ofst, int stride, int len, u32 es) {
  return (struct stride){
      .start  = (u64)ofst * (u64)es,
      .stride = (u64)stride * (u64)es,
      .nelems = (u64)len,
  };
}

/**
 * Insert a random run of bytes into the current variable. The same bytes
 * are written to the DB and to the active reference fake_database.
 */
static void swmt_insert (struct swarm_test *meta) {
  struct fake_var *v = current_var (meta);
  assert (v);

  int len  = (rand () % meta->max_insert_len) + 1;
  int ofst = rand () % (current_var_len (v) + 1);

  int      blen = len * (int)v->elem_size;
  uint8_t *data = malloc ((size_t)blen);
  swmt_assert (data != NULL);
  for (int i = 0; i < blen; ++i) { data[i] = (uint8_t)rand (); }

  /* DB side */
  swmt_assert (nsdb_insert (meta->db, v->name, data, ofst, len) == 0);

  /* Reference side */
  swmt_assert (
      block_array_insert (v->data, (u32)(ofst * (int)v->elem_size), data, (u32)blen, NULL) == 0);

  free (data);
}

/**
 * Remove a strided slice. Cross-checks that the removed bytes match
 * between the DB and the reference model.
 */
static void swmt_remove (struct swarm_test *meta) {
  struct fake_var *v = current_var (meta);
  assert (v);

  int ofst, stride, len;
  swmt_random_slice (current_var_len (v), &ofst, &stride, &len);

  size_t   buf_sz  = (size_t)len * (size_t)v->elem_size;
  uint8_t *db_buf  = calloc (1, buf_sz);
  uint8_t *ref_buf = calloc (1, buf_sz);
  swmt_assert (db_buf && ref_buf);

  /* DB side */
  swmt_assert (
      nsdb_remove (meta->db, v->name, db_buf, ofst, stride, ofst + len * stride, 0xFF) == 0);

  /* Reference side */
  struct stride str = to_block_stride (ofst, stride, len, v->elem_size);
  i64           got = block_array_remove (v->data, str, v->elem_size, ref_buf, NULL);
  swmt_assert (got == (i64)len);

  /* Cross-check */
  swmt_assert (memcmp (db_buf, ref_buf, buf_sz) == 0);

  free (db_buf);
  free (ref_buf);
}

/**
 * Read a strided slice. Cross-checks that the read bytes match between
 * the DB and the reference model.
 */
static void swmt_read (struct swarm_test *meta) {
  struct fake_var *v = current_var (meta);
  assert (v);

  int ofst, stride, len;
  swmt_random_slice (current_var_len (v), &ofst, &stride, &len);

  size_t   buf_sz  = (size_t)len * (size_t)v->elem_size;
  uint8_t *db_buf  = calloc (1, buf_sz);
  uint8_t *ref_buf = calloc (1, buf_sz);
  swmt_assert (db_buf && ref_buf);

  swmt_assert (nsdb_read (meta->db, v->name, db_buf, ofst, stride, ofst + len * stride, 0xFF) == 0);

  struct stride str = to_block_stride (ofst, stride, len, v->elem_size);
  u64           got = block_array_read (v->data, str, v->elem_size, ref_buf);
  swmt_assert (got == (u64)len);

  swmt_assert (memcmp (db_buf, ref_buf, buf_sz) == 0);

  free (db_buf);
  free (ref_buf);
}

/**
 * Overwrite a strided slice with random bytes; same bytes go to both the
 * DB and the reference model.
 */
static void swmt_write (struct swarm_test *meta) {
  struct fake_var *v = current_var (meta);
  assert (v);

  int ofst, stride, len;
  swmt_random_slice (current_var_len (v), &ofst, &stride, &len);

  int      blen = len * (int)v->elem_size;
  uint8_t *data = malloc ((size_t)blen);
  swmt_assert (data != NULL);
  for (int i = 0; i < blen; ++i) { data[i] = (uint8_t)rand (); }

  swmt_assert (nsdb_write (meta->db, v->name, data, ofst, stride, ofst + len * stride, 0xFF) == 0);

  struct stride str = to_block_stride (ofst, stride, len, v->elem_size);
  u64           got = block_array_write (v->data, str, v->elem_size, data);
  swmt_assert (got == (u64)len);

  free (data);
}

/* ---------------------------------------------------------------------- */
/*  Variable lifecycle                                                    */
/* ---------------------------------------------------------------------- */

/**
 * Create a new variable with random name and type. Retries on name
 * collision. Adds the variable to the active fake_database. If this was
 * the first variable, it becomes the current one.
 *
 * Allowed inside or outside a transaction; if inside, the variable lives
 * in the working snapshot and is automatically rolled back on abort.
 */
static void swmt_create (struct swarm_test *meta) {
  struct fake_database *db = active_db (meta);

  for (;;) {
    char        *name    = random_name (1, 100);
    struct type *type    = random_type ();
    char        *typestr = type_str (type);
    u32          esize   = (u32)type_byte_size (type);
    type_free (type);

    if (fake_db_find (db, name) != NULL) {
      free (name);
      free (typestr);
      continue;
    }

    /* DB side */
    swmt_assert (nsdb_create (meta->db, name, typestr) == 0);

    /* Reference side -- takes ownership of name & typestr */
    swmt_assert (fake_db_add_var (db, name, typestr, esize) == 0);

    /* First variable becomes the current one */
    if (meta->cur_name == NULL) {
      struct fake_var *added = fake_db_var_at (db, fake_db_var_count (db) - 1);
      meta->cur_name         = strdup (added->name);
      swmt_assert (meta->cur_name != NULL);
    }
    return;
  }
}

/**
 * Switch the current variable to a different existing one in the active
 * database. Allowed inside or outside a transaction.
 */
static void swmt_switch (struct swarm_test *meta) {
  struct fake_database *db = active_db (meta);
  int                   n  = fake_db_var_count (db);
  assert (n > 1);
  assert (meta->cur_name != NULL);

  for (;;) {
    int                    choice = rand () % n;
    const struct fake_var *next   = fake_db_var_at (db, choice);
    if (strcmp (next->name, meta->cur_name) == 0) { continue; }
    free (meta->cur_name);
    meta->cur_name = strdup (next->name);
    swmt_assert (meta->cur_name != NULL);
    return;
  }
}

/**
 * Delete the current variable from both the DB and the active fake_database,
 * then move `cur_name` to a remaining variable. Allowed inside or outside
 * a transaction.
 */
static void swmt_delete (struct swarm_test *meta) {
  assert (meta->cur_name != NULL);
  assert (fake_db_var_count (active_db (meta)) > 1);

  /* The active fake_database will free its copy of the name when we call
   * fake_db_remove_var, so we need a local copy for the nsdb call and
   * for the post-removal fixup. */
  char *name = strdup (meta->cur_name);
  swmt_assert (name != NULL);

  swmt_assert (nsdb_delete (meta->db, name) == 0);
  swmt_assert (fake_db_remove_var (active_db (meta), name) == 0);

  free (name);

  /* cur_name now refers to a missing variable; rebind to the first
   * remaining one. */
  free (meta->cur_name);
  meta->cur_name = NULL;
  fixup_cur_name (meta);
}

/* ---------------------------------------------------------------------- */
/*  Step                                                                  */
/* ---------------------------------------------------------------------- */

void swmt_step (struct swarm_test *meta) {
  /* Count allowed actions */
  int len = 0;
  for (int i = 0; i < AT_LEN; ++i) { len += meta->allowed[i]; }

  /* If the swarm has masked everything off, re-roll and try again next
   * step rather than divide by zero. */
  if (len == 0) {
    swmt_set_random_enabled (meta);
    swmt_set_allowed (meta);
    return;
  }

  /* Pick the n-th allowed action */
  int next   = rand () % len;
  int index  = 0;
  int choice = 0;
  for (; index < AT_LEN; ++index) {
    if (meta->allowed[index]) {
      if (choice == next) {
        break;
      } else {
        choice++;
      }
    }
  }

  enum action_type action = (enum action_type)index;

  switch (action) {
    case BEGIN_TXN: swmt_begin_txn (meta); break;
    case COMMIT_TXN: swmt_commit_txn (meta); break;
    case ROLLBACK_TXN: swmt_rollback_txn (meta); break;
    case CRASH_AND_REOPEN: swmt_crash_and_reopen (meta); break;
    case CLOSE_AND_REOPEN: swmt_close_and_reopen (meta); break;
    case INSERT: swmt_insert (meta); break;
    case REMOVE: swmt_remove (meta); break;
    case READ: swmt_read (meta); break;
    case WRITE: swmt_write (meta); break;
    case CREATE: swmt_create (meta); break;
    case SWITCH: swmt_switch (meta); break;
    case DELETE: swmt_delete (meta); break;
    default: assert (0);
  }

  /* Possibly re-roll the swarm */
  if (random_unit_float () < meta->probability_of_swarm_change) { swmt_set_random_enabled (meta); }

  swmt_set_allowed (meta);

  /* Possibly run a full validation pass */
  if (random_unit_float () < meta->probability_of_full_read) { swmt_full_validation (meta); }
}

/* ---------------------------------------------------------------------- */
/*  Driver                                                                */
/* ---------------------------------------------------------------------- */

void run_swarm_test (void) {
  int start_enabled[AT_LEN];
  for (int i = 0; i < AT_LEN; ++i) { start_enabled[i] = 1; }

  struct swarm_test *meta = swmt_open (0.1f, 0.1f, start_enabled, "test", 10000);

  /* TODO - set up signal for timer that breaks out of the loop and
   *        falls through to swmt_close(meta) below. */
  while (1) { swmt_step (meta); }

  swmt_close (meta); /* unreachable until the timer is wired up */
}
