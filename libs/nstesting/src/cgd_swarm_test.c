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

#include "cgd_swarm_test.h"

#include "_numstore.h"
#include "c_specx/error.h"
#include "mem_vhmap.h"
#include "nscore/compile_config.h"
#include "nscore/types.h"
#include "nscore/variables.h"
#include "numstore.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

///////////////////////////////////////////////////////////
/// Utils

static void
cgd_swmt_assert (int result)
{
  if (!result)
  {
    printf ("Failed cgd_swarm test\n");
    abort ();
  }
}

static struct mem_vhmap *
active_db (struct cgd_swarm_test *meta)
{ return meta->in_txn ? meta->working : meta->committed; }

static void
rebind_cur (struct cgd_swarm_test *meta, const char *preferred_name)
{
  struct mem_vhmap *db = active_db (meta);
  meta->cur            = preferred_name ? mem_vhmap_get_var (db, strfcstr (preferred_name)) : NULL;
  if (!meta->cur && mem_vhmap_count (db) > 0) { meta->cur = mem_vhmap_random (db); }
}

/**
 * Re-roll the enabled action subset.
 *
 * Given N action types, the cgd_swarm action space is the power set minus the
 * empty set: 2^N - 1 non-empty subsets. We sample uniformly by drawing an
 * integer in [1, 2^N) and interpreting each bit as inclusion.
 */
static void
cgd_swmt_set_random_enabled (struct cgd_swarm_test *meta)
{
  int mask = rand () % ((1 << CDS_AT_LEN) - 1) + 1;
  for (int i = 0; i < CDS_AT_LEN; ++i) { meta->enabled[i] = (mask >> i) & 1; }
}

/**
 * Recompute which actions can actually fire right now.
 *
 * An action is `allowed` iff (a) it is in the current cgd_swarm (`enabled`)
 * AND (b) the test state permits it. CREATE and DELETE are permitted
 * inside a transaction because the mem_vhmap snapshot model gives
 * them transactional semantics for free.
 */
static void
cgd_swmt_set_allowed (struct cgd_swarm_test *meta)
{
  assert (meta);
  assert (meta->db);
  assert (meta->dbname);

  memset (meta->allowed, 0, sizeof (meta->allowed));

  struct mem_vhmap *db    = active_db (meta);
  int               nvars = mem_vhmap_count (db);

  /* CRASH is always permissible if cgd_swarm-enabled */
  meta->allowed[CDS_CRASH_AND_REOPEN] = meta->enabled[CDS_CRASH_AND_REOPEN];

  /* CREATE: only constrained by capacity */
  meta->allowed[CDS_CREATE] = meta->enabled[CDS_CREATE];

  /* Txn-state-dependent ops */
  if (!meta->in_txn)
  {
    meta->allowed[CDS_BEGIN_TXN]        = meta->enabled[CDS_BEGIN_TXN];
    meta->allowed[CDS_CLOSE_AND_REOPEN] = meta->enabled[CDS_CLOSE_AND_REOPEN];
  }
  else
  {
    meta->allowed[CDS_COMMIT_TXN]   = meta->enabled[CDS_COMMIT_TXN];
    meta->allowed[CDS_ROLLBACK_TXN] = meta->enabled[CDS_ROLLBACK_TXN];
  }

  /* Need at least two vars to switch or to safely delete */
  if (nvars > 1)
  {
    meta->allowed[CDS_DELETE] = meta->enabled[CDS_DELETE];
    meta->allowed[CDS_SWITCH] = meta->enabled[CDS_SWITCH];
  }
}

static u32
get_random_name_len (void)
{
  u32 roll = randu32r (1, 100);

  if (roll <= 90) { return randu32r (2, 10); }

  if (roll <= 95) { return randu32r (10, PAGE_SIZE); }

  return randu32r (PAGE_SIZE, 10 * PAGE_SIZE);
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

  if (roll <= 95) { return randu32r (1, 3); }

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
/// Main Api

struct cgd_swarm_test *
cgd_swmt_open (int start_enabled[CDS_AT_LEN], const char *dbname)
{
  struct cgd_swarm_test *ret = malloc (sizeof *ret);
  cgd_swmt_assert (ret != NULL);

  cgd_swmt_assert (nsdb_cleanup (dbname) == 0);

  *ret = (struct cgd_swarm_test){
      .committed = mem_vhmap_create (NULL),
      .working   = NULL,
      .cur       = NULL,
      .db        = nsdb_open (dbname),
      .in_txn    = 0,
      .dbname    = dbname,
  };

  cgd_swmt_assert (ret->committed != NULL);
  cgd_swmt_assert (ret->db != NULL);

  memcpy (ret->enabled, start_enabled, CDS_AT_LEN * sizeof (int));
  cgd_swmt_set_allowed (ret);

  return ret;
}

void
cgd_swmt_close (struct cgd_swarm_test *meta)
{
  if (meta->in_txn) { cgd_swmt_commit_txn (meta); }
  cgd_swmt_assert (nsdb_close (meta->db) == 0);

  if (meta->committed) { mem_vhmap_free (meta->committed); }
  if (meta->working) { mem_vhmap_free (meta->working); }

  free (meta);
}

void
cgd_swmt_step (struct cgd_swarm_test *meta)
{
  // Temporary
  // meta->allowed[CDS_CRASH_AND_REOPEN] = 0;
  // meta->allowed[CDS_CLOSE_AND_REOPEN] = 0;
  // meta->allowed[CDS_ROLLBACK_TXN]     = 0;
  // meta->allowed[CDS_COMMIT_TXN]       = 0;
  // meta->allowed[CDS_BEGIN_TXN]        = 0;

  /* Count allowed actions */
  int len = 0;
  for (int i = 0; i < CDS_AT_LEN; ++i) { len += meta->allowed[i]; }

  /* If the cgd_swarm has masked everything off, re-roll and try again next
   * step rather than divide by zero. */
  if (len == 0)
  {
    cgd_swmt_set_random_enabled (meta);
    cgd_swmt_set_allowed (meta);
    return;
  }

  /* Pick the n-th allowed action */
  int next   = rand () % len;
  int index  = 0;
  int choice = 0;
  for (; index < CDS_AT_LEN; ++index)
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

  switch (action)
  {
    case CDS_BEGIN_TXN: cgd_swmt_begin_txn (meta); break;
    case CDS_COMMIT_TXN: cgd_swmt_commit_txn (meta); break;
    case CDS_ROLLBACK_TXN: cgd_swmt_rollback_txn (meta); break;
    case CDS_CRASH_AND_REOPEN: cgd_swmt_crash_and_reopen (meta); break;
    case CDS_CLOSE_AND_REOPEN: cgd_swmt_close_and_reopen (meta); break;
    case CDS_CREATE: cgd_swmt_create (meta); break;
    case CDS_SWITCH: cgd_swmt_switch (meta); break;
    case CDS_DELETE: cgd_swmt_delete (meta); break;
    default: assert (0);
  }

  cgd_swmt_set_allowed (meta);
}

///////////////////////////////////////////////////////////
/// Concrete Actions

void
cgd_swmt_begin_txn (struct cgd_swarm_test *meta)
{
  assert (!meta->in_txn);
  assert (meta->working == NULL);

  cgd_swmt_assert (nsdb_begin (meta->db) == 0);

  error e       = error_create ();
  meta->working = mem_vhmap_clone (meta->committed, &e);
  cgd_swmt_assert (meta->working != NULL);
  meta->in_txn = 1;
}

/**
 * Commit the open transaction. The working snapshot becomes the new
 * committed; the old committed is freed.
 */
void
cgd_swmt_commit_txn (struct cgd_swarm_test *meta)
{
  assert (meta->in_txn);
  assert (meta->working != NULL);

  cgd_swmt_assert (nsdb_commit (meta->db) == 0);

  char *saved = meta->cur ? strdup (meta->cur->vname.data) : NULL;
  mem_vhmap_free (meta->committed);
  meta->committed = meta->working;
  meta->working   = NULL;
  meta->in_txn    = 0;

  rebind_cur (meta, saved);
  free (saved);
}

/**
 * Rollback the open transaction. Throws away the working snapshot;
 * committed is untouched.
 */
void
cgd_swmt_rollback_txn (struct cgd_swarm_test *meta)
{
  assert (meta->in_txn);

  cgd_swmt_assert (nsdb_rollback (meta->db) == 0);

  char *saved = meta->cur ? strdup (meta->cur->vname.data) : NULL;
  mem_vhmap_free (meta->working);
  meta->working = NULL;
  meta->in_txn  = 0;

  rebind_cur (meta, saved);
  free (saved);
}

/**
 * Forcibly crash the database, then reopen it. Reference-side this behaves
 * like a rollback: any in-flight working snapshot is discarded.
 */
void
cgd_swmt_crash_and_reopen (struct cgd_swarm_test *meta)
{
  char *saved = meta->cur ? strdup (meta->cur->vname.data) : NULL;

  cgd_swmt_assert (_nsdb_crash (meta->db) == 0);

  meta->db = nsdb_open (meta->dbname);
  cgd_swmt_assert (meta->db != NULL);

  if (meta->working)
  {
    mem_vhmap_free (meta->working);
    meta->working = NULL;
  }
  meta->in_txn = 0;

  rebind_cur (meta, saved);
  free (saved);
}

void
cgd_swmt_close_and_reopen (struct cgd_swarm_test *meta)
{
  assert (!meta->in_txn);
  cgd_swmt_assert (nsdb_close (meta->db) == 0);

  meta->db = nsdb_open (meta->dbname);
  cgd_swmt_assert (meta->db != NULL);
}

void
cgd_swmt_create (struct cgd_swarm_test *meta)
{
  struct mem_vhmap *db = active_db (meta);

  // Loop until you get a unique variable name
  for (;;)
  {
    struct chunk_alloc temp;
    chunk_alloc_create_default (&temp);

    error        e       = error_create ();
    char        *name    = random_name ();
    struct type *type    = type_random (&temp, get_random_type_depth (), &e);
    char        *typestr = type_str (type);

    // Already exists - try again
    if (mem_vhmap_get_var (db, strfcstr (name)) != NULL)
    {
      free (name);
      free (typestr);
      chunk_alloc_free_all (&temp);
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
    cgd_swmt_assert (nsdb_create (meta->db, name, typestr) == 0);

    /* Reference side -- takes ownership of name & typestr */
    cgd_swmt_assert (mem_vhmap_add_var (db, &var, &e) == 0);

    /* First variable becomes the current one */
    if (meta->cur == NULL) { meta->cur = mem_vhmap_get_var (db, var.vname); }

    free (name);
    free (typestr);
    chunk_alloc_free_all (&temp);

    return;
  }
}

/**
 * Switch the current variable to a different existing one in the active
 * database. Allowed inside or outside a transaction.
 */
void
cgd_swmt_switch (struct cgd_swarm_test *meta)
{
  struct mem_vhmap *db = active_db (meta);
  meta->cur            = mem_vhmap_random (db);
  ASSERT (meta->cur);
  // You could also loop over the variables if you want
}

void
cgd_swmt_delete (struct cgd_swarm_test *meta)
{
  assert (meta->cur != NULL);
  cgd_swmt_assert (nsdb_delete (meta->db, meta->cur->vname.data) == 0);
  mem_vhmap_remove_var (active_db (meta), meta->cur->vname);
  meta->cur = mem_vhmap_random (active_db (meta));
}
