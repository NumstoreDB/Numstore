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

#include "swarm_test.h"

#include "_numstore.h"
#include "fake_database.h"
#include "nscore/types.h"
#include "nscore/variables.h"
#include "numstore.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct swarm_test
{
  // Fake database transaction semantics
  struct fake_database *committed;
  struct fake_database *working;

  char *cur_name;

  int enabled[AT_LEN];
  int allowed[AT_LEN];

  nsdb_t     *db;
  int         in_txn;
  const char *dbname;
  int         max_insert_len;

  float probability_of_swarm_change;
  float probability_of_full_read;
};

///////////////////////////////////////////////////////////
/// Utils

const char *
action_type_tostr (enum action_type type)
{
  switch (type)
  {
    case_ENUM_RETURN_STRING (BEGIN_TXN);
    case_ENUM_RETURN_STRING (COMMIT_TXN);
    case_ENUM_RETURN_STRING (ROLLBACK_TXN);
    case_ENUM_RETURN_STRING (CRASH_AND_REOPEN);
    case_ENUM_RETURN_STRING (CLOSE_AND_REOPEN);
    case_ENUM_RETURN_STRING (INSERT);
    case_ENUM_RETURN_STRING (REMOVE);
    case_ENUM_RETURN_STRING (READ);
    case_ENUM_RETURN_STRING (WRITE);
    case_ENUM_RETURN_STRING (CREATE);
    case_ENUM_RETURN_STRING (SWITCH);
    case_ENUM_RETURN_STRING (DELETE);
    case AT_LEN: UNREACHABLE ();
    default: UNREACHABLE ();
  }
}

static void
swmt_assert (int result)
{
  if (!result)
  {
    printf ("Failed swarm test\n");
    abort ();
  }
}

static float
random_unit_float (void)
{ return (float)rand () / (float)RAND_MAX; }

static struct fake_database *
active_db (struct swarm_test *meta)
{ return meta->in_txn ? meta->working : meta->committed; }

static struct fake_var *
current_var (struct swarm_test *meta)
{
  if (!meta->cur_name) { return NULL; }
  return fake_db_find (active_db (meta), meta->cur_name);
}

static int
current_var_len (const struct fake_var *v)
{ return (int)(block_array_getlen (v->data) / (u64)v->elem_size); }

static void
fixup_cur_name (struct swarm_test *meta)
{
  struct fake_database *db = active_db (meta);

  if (meta->cur_name && fake_db_find (db, meta->cur_name)) { return; }

  free (meta->cur_name);
  meta->cur_name = NULL;

  if (fake_db_var_count (db) > 0)
  {
    meta->cur_name = strdup (fake_db_var_at (db, 0)->name);
    swmt_assert (meta->cur_name != NULL);
  }
}

static void
swmt_random_slice (int total, int *ofst, int *stride, int *len)
{
  assert (total > 0);

  *ofst         = randu32r (0, total - 1);
  int remaining = total - *ofst;
  *stride       = randu32r (1, remaining);

  int max_len = (remaining + *stride - 1) / *stride;
  *len        = randu32r (1, max_len);
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

/**
 * Re-roll the enabled action subset.
 *
 * Given N action types, the swarm action space is the power set minus the
 * empty set: 2^N - 1 non-empty subsets. We sample uniformly by drawing an
 * integer in [1, 2^N) and interpreting each bit as inclusion.
 */
static void
swmt_set_random_enabled (struct swarm_test *meta)
{
  int mask = rand () % ((1 << AT_LEN) - 1) + 1;
  for (int i = 0; i < AT_LEN; ++i) { meta->enabled[i] = (mask >> i) & 1; }
}

static void
swmt_full_validation (struct swarm_test *meta)
{ nsdb_validate (meta->db); }

/**
 * Recompute which actions can actually fire right now.
 *
 * An action is `allowed` iff (a) it is in the current swarm (`enabled`)
 * AND (b) the test state permits it. CREATE and DELETE are permitted
 * inside a transaction because the fake_database snapshot model gives
 * them transactional semantics for free.
 */
static void
swmt_set_allowed (struct swarm_test *meta)
{
  assert (meta);
  assert (meta->db);
  assert (meta->dbname);

  memset (meta->allowed, 0, sizeof (meta->allowed));

  struct fake_database *db    = active_db (meta);
  int                   nvars = fake_db_var_count (db);

  /* CRASH is always permissible if swarm-enabled */
  meta->allowed[CRASH_AND_REOPEN] = meta->enabled[CRASH_AND_REOPEN];

  /* CREATE: only constrained by capacity */
  if (nvars < FAKE_DB_MAX_VARS) { meta->allowed[CREATE] = meta->enabled[CREATE]; }

  /* Txn-state-dependent ops */
  if (!meta->in_txn)
  {
    meta->allowed[BEGIN_TXN]        = meta->enabled[BEGIN_TXN];
    meta->allowed[CLOSE_AND_REOPEN] = meta->enabled[CLOSE_AND_REOPEN];
  }
  else
  {
    meta->allowed[COMMIT_TXN]   = meta->enabled[COMMIT_TXN];
    meta->allowed[ROLLBACK_TXN] = meta->enabled[ROLLBACK_TXN];
  }

  /* Variable-dependent ops */
  struct fake_var *cur = current_var (meta);
  if (cur)
  {
    meta->allowed[INSERT] = meta->enabled[INSERT];
    if (current_var_len (cur) > 0)
    {
      meta->allowed[REMOVE] = meta->enabled[REMOVE];
      meta->allowed[READ]   = meta->enabled[READ];
      meta->allowed[WRITE]  = meta->enabled[WRITE];
    }
  }

  /* Need at least two vars to switch or to safely delete */
  if (nvars > 1)
  {
    meta->allowed[DELETE] = meta->enabled[DELETE];
    meta->allowed[SWITCH] = meta->enabled[SWITCH];
  }
}

static char *
random_name (int low, int high)
{
  // Bound the high length below your validator's 4096 threshold if needed
  if (low <= 0) { low = 1; }
  if (high >= 4096) { high = 4095; }
  int   length = low + (rand () % (high - low + 1));
  char *buffer = malloc ((length + 1) * sizeof (char));
  var_random_name (buffer, length);
  return buffer;
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
/// Main Api

struct swarm_test *
swmt_open (
    float       probability_of_swarm_change,
    float       probability_of_full_read,
    int         start_enabled[AT_LEN],
    const char *dbname,
    int         max_insert_len
)
{
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

void
swmt_close (struct swarm_test *meta)
{
  if (meta->in_txn) { swmt_commit_txn (meta); }
  swmt_assert (nsdb_close (meta->db) == 0);

  fake_db_free (meta->committed);
  fake_db_free (meta->working);
  free (meta->cur_name);
  free (meta);
}

void
swmt_step (struct swarm_test *meta)
{
  meta->allowed[CRASH_AND_REOPEN] = 0;
  meta->allowed[CLOSE_AND_REOPEN] = 0;
  meta->allowed[ROLLBACK_TXN]     = 0;
  meta->allowed[COMMIT_TXN]       = 0;
  meta->allowed[BEGIN_TXN]        = 0;

  /* Count allowed actions */
  int len = 0;
  for (int i = 0; i < AT_LEN; ++i) { len += meta->allowed[i]; }

  /* If the swarm has masked everything off, re-roll and try again next
   * step rather than divide by zero. */
  if (len == 0)
  {
    swmt_set_random_enabled (meta);
    swmt_set_allowed (meta);
    return;
  }

  /* Pick the n-th allowed action */
  int next   = rand () % len;
  int index  = 0;
  int choice = 0;
  for (; index < AT_LEN; ++index)
  {
    if (meta->allowed[index])
    {
      if (choice == next) { break; }
      else
      {
        choice++;
      }
    }
  }

  enum action_type action = (enum action_type)index;

  i_log_info ("Taking Action: %s\n", action_type_tostr (action));
  switch (action)
  {
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

///////////////////////////////////////////////////////////
/// Concrete Actions

void
swmt_begin_txn (struct swarm_test *meta)
{
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
void
swmt_commit_txn (struct swarm_test *meta)
{
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
void
swmt_rollback_txn (struct swarm_test *meta)
{
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
void
swmt_crash_and_reopen (struct swarm_test *meta)
{
  swmt_assert (_nsdb_crash (meta->db) == 0);

  meta->db = nsdb_open (meta->dbname);
  swmt_assert (meta->db != NULL);

  if (meta->working)
  {
    fake_db_free (meta->working);
    meta->working = NULL;
  }
  meta->in_txn = 0;

  fixup_cur_name (meta);
}

void
swmt_close_and_reopen (struct swarm_test *meta)
{
  assert (!meta->in_txn);
  swmt_assert (nsdb_close (meta->db) == 0);

  meta->db = nsdb_open (meta->dbname);
  swmt_assert (meta->db != NULL);
}

void
swmt_insert (struct swarm_test *meta)
{
  struct fake_var *v = current_var (meta);
  assert (v);

  int len  = (rand () % meta->max_insert_len) + 1;
  int ofst = rand () % (current_var_len (v) + 1);

  int      blen = len * (int)v->elem_size;
  uint8_t *data = malloc ((size_t)blen);
  swmt_assert (data != NULL);
  for (int i = 0; i < blen; ++i) { data[i] = (uint8_t)rand (); }

  /* DB side */
  swmt_assert (nsdb_insert (meta->db, v->name, data, ofst, len) == len);

  /* Reference side */
  swmt_assert (
      block_array_insert (v->data, (u32)(ofst * (int)v->elem_size), data, (u32)blen, NULL) == 0
  );

  free (data);
}

void
swmt_remove (struct swarm_test *meta)
{
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
      nsdb_remove (meta->db, v->name, db_buf, ofst, stride, ofst + len * stride, 0xFF) == len
  );

  /* Reference side */
  struct stride str = (struct stride){.start = ofst, .stride = stride, .nelems = len};
  i64           got = block_array_remove (v->data, str, v->elem_size, ref_buf, NULL);
  swmt_assert (got == (i64)len);

  /* Cross-check */
  swmt_assert (memcmp (db_buf, ref_buf, buf_sz) == 0);

  free (db_buf);
  free (ref_buf);
}

void
swmt_read (struct swarm_test *meta)
{
  struct fake_var *v = current_var (meta);
  assert (v);

  int ofst, stride, len;
  swmt_random_slice (current_var_len (v), &ofst, &stride, &len);

  size_t   buf_sz  = (size_t)len * (size_t)v->elem_size;
  uint8_t *db_buf  = calloc (1, buf_sz);
  uint8_t *ref_buf = calloc (1, buf_sz);
  swmt_assert (db_buf && ref_buf);

  swmt_assert (
      nsdb_read (meta->db, v->name, db_buf, ofst, stride, ofst + len * stride, 0xFF) == len
  );

  struct stride str = to_block_stride (ofst, stride, len);
  u64           got = block_array_read (v->data, str, v->elem_size, ref_buf);
  swmt_assert (got == (u64)len);

  swmt_assert (memcmp (db_buf, ref_buf, buf_sz) == 0);

  free (db_buf);
  free (ref_buf);
}

void
swmt_write (struct swarm_test *meta)
{
  struct fake_var *v = current_var (meta);
  assert (v);

  int ofst, stride, len;
  swmt_random_slice (current_var_len (v), &ofst, &stride, &len);

  int      blen = len * (int)v->elem_size;
  uint8_t *data = malloc ((size_t)blen);
  swmt_assert (data != NULL);
  for (int i = 0; i < blen; ++i) { data[i] = (uint8_t)rand (); }

  swmt_assert (
      nsdb_write (meta->db, v->name, data, ofst, stride, ofst + len * stride, 0xFF) == len
  );

  struct stride str = to_block_stride (ofst, stride, len);
  u64           got = block_array_write (v->data, str, v->elem_size, data);
  swmt_assert (got == (u64)len);

  free (data);
}

void
swmt_create (struct swarm_test *meta)
{
  struct fake_database *db = active_db (meta);

  for (;;)
  {
    struct chunk_alloc temp;
    chunk_alloc_create_default (&temp);

    char        *name    = random_name (1, 100);
    struct type *type    = type_random (&temp, randu32r (1, 2), NULL);
    char        *typestr = type_str (type);
    u32          esize   = (u32)type_byte_size (type);

    // i_log_error("%s\n", typestr);

    if (fake_db_find (db, name) != NULL)
    {
      free (name);
      free (typestr);
      continue;
    }

    /* DB side */
    swmt_assert (nsdb_create (meta->db, name, typestr) == 0);

    /* Reference side -- takes ownership of name & typestr */
    swmt_assert (fake_db_add_var (db, name, typestr, esize) == 0);

    /* First variable becomes the current one */
    if (meta->cur_name == NULL)
    {
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
void
swmt_switch (struct swarm_test *meta)
{
  struct fake_database *db = active_db (meta);
  int                   n  = fake_db_var_count (db);
  assert (n > 1);
  assert (meta->cur_name != NULL);

  for (;;)
  {
    int                    choice = rand () % n;
    const struct fake_var *next   = fake_db_var_at (db, choice);
    if (strcmp (next->name, meta->cur_name) == 0) { continue; }
    free (meta->cur_name);
    meta->cur_name = strdup (next->name);
    swmt_assert (meta->cur_name != NULL);
    return;
  }
}

void
swmt_delete (struct swarm_test *meta)
{
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
