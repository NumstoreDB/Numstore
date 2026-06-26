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

/// Copyright 2026 Theo Lincke
/// ... (license header unchanged)

#include "testing/cgd_swarm_test_fixture.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "logging.h"
#include "mem_vhmap.h"
#include "numerics.h"
#include "numstore.h"
#include "serial.h" // strfcstr
#include "types.h"
#include "variables.h"

///////////////////////////////////////////////////////////
/// Diagnostics

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
 * Bare assertion: prints the failing expression and source location.
 */
#define CGD_SWMT_ASSERT(expr)                                                  \
  do                                                                           \
  {                                                                            \
    if (!(expr))                                                               \
    {                                                                          \
      i_log_failure ("CGD swarm test FAILED\n");                               \
      i_log_failure ("  expr: %s\n", #expr);                                   \
      i_log_failure ("  loc:  %s:%d in %s()\n", __FILE__, __LINE__, __func__); \
      panic ("Aborting early\n");                                              \
    }                                                                          \
  }                                                                            \
  while (0)

/**
 * Formatted assertion: adds a printf-style context line.
 */
#define CGD_SWMT_ASSERTF(expr, fmt, ...)                                       \
  do                                                                           \
  {                                                                            \
    if (!(expr))                                                               \
    {                                                                          \
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
  do                                                                           \
  {                                                                            \
    if (!(expr))                                                               \
    {                                                                          \
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
  if (meta == NULL)
  {
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
  i_log_info (
      "  cur:            %s\n",
      meta->cur ? meta->cur->vname.data : "<null>"
  );
  i_log_info ("  sample_space_p: %.3f\n", (double)meta->sample_space_prob);
  i_log_info ("  Actions             enabled   allowed\n");
  for (int i = 0; i < CDS_AT_LEN; ++i)
  {
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
active_db (struct cgd_swarm_test *meta)
{
  return meta->in_txn ? meta->working : meta->committed;
}

static void
rebind_cur (struct cgd_swarm_test *meta, const char *preferred_name)
{
  struct mem_vhmap *db = active_db (meta);
  meta->cur =
      preferred_name ? mem_vhmap_get_var (db, strfcstr (preferred_name)) : NULL;
  if (!meta->cur && mem_vhmap_count (db) > 0)
  {
    meta->cur = mem_vhmap_random (db);
  }
}

static void
cgd_swmt_set_random_enabled (struct cgd_swarm_test *meta)
{
  int mask = rand () % ((1 << CDS_AT_LEN) - 1) + 1;
  for (int i = 0; i < CDS_AT_LEN; ++i)
  {
    meta->enabled[i] = (mask >> i) & 1;
  }
}

static void
cgd_swmt_set_allowed (struct cgd_swarm_test *meta)
{
  assert (meta);
  assert (meta->db);
  assert (meta->dbname);

  memset (meta->allowed, 0, sizeof (meta->allowed));

  struct mem_vhmap *db    = active_db (meta);
  int               nvars = mem_vhmap_count (db);

  meta->allowed[CDS_CRASH_AND_REOPEN] = meta->enabled[CDS_CRASH_AND_REOPEN];
  meta->allowed[CDS_CREATE]           = meta->enabled[CDS_CREATE];

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

  if (roll <= 90)
  {
    return randu32r (2, 10);
  }

  if (roll <= 95)
  {
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

  if (roll <= 95)
  {
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
  assert (!meta->in_txn);
  assert (meta->working == NULL);

  CGD_SWMT_ASSERTF (
      nsdb_begin (meta->db) == 0,
      "nsdb_begin failed on db='%s'",
      meta->dbname
  );

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
  assert (meta->in_txn);
  assert (meta->working != NULL);

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
  assert (meta->in_txn);

  CGD_SWMT_ASSERTF (
      nsdb_rollback (meta->db) == 0,
      "nsdb_rollback failed on db='%s'",
      meta->dbname
  );

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

  CGD_SWMT_ASSERTF (
      nsdb_crash (meta->db) == 0,
      "nsdb_crash failed on db='%s'",
      meta->dbname
  );

  meta->db = nsdb_open (meta->dbname);
  CGD_SWMT_ASSERTF (
      meta->db != NULL,
      "nsdb_open after crash returned NULL (db='%s')",
      meta->dbname
  );

  if (meta->working)
  {
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
  assert (!meta->in_txn);
  CGD_SWMT_ASSERTF (
      nsdb_close (meta->db) == 0,
      "nsdb_close failed on db='%s'",
      meta->dbname
  );

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
    int exec_ret = nsdb_execute (meta->db, "create %s %s", NULL, name, typestr);
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
    if (meta->cur == NULL)
    {
      meta->cur = mem_vhmap_get_var (db, var.vname);
    }

    free (name);
    free (typestr);
    chunk_alloc_free_all (&temp);

    return;
  }
}

static void
cgd_swmt_switch (struct cgd_swarm_test *meta)
{
  struct mem_vhmap *db = active_db (meta);
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
      mem_vhmap_count (active_db (meta))
  );

  const char *name     = meta->cur->vname.data;
  int         exec_ret = nsdb_execute (meta->db, "delete %s", NULL, name);
  CGD_SWMT_ASSERT_STATE (
      meta,
      exec_ret == 0,
      "nsdb delete returned %d (name='%s')",
      exec_ret,
      name
  );

  mem_vhmap_remove_var (active_db (meta), meta->cur->vname);
  meta->cur = mem_vhmap_random (active_db (meta));
}

///////////////////////////////////////////////////////////
/// Main Api

struct cgd_swarm_test *
cgd_swmt_open (
    int         start_enabled[CDS_AT_LEN],
    const char *dbname,
    float       sample_space_prob
)
{
  ASSERT (sample_space_prob >= 0 && sample_space_prob <= 1);

  struct cgd_swarm_test *ret = malloc (sizeof *ret);
  CGD_SWMT_ASSERTF (
      ret != NULL,
      "malloc(%zu) failed for cgd_swarm_test (db='%s')",
      sizeof *ret,
      dbname
  );

  CGD_SWMT_ASSERTF (
      nsdb_cleanup (dbname) == 0,
      "nsdb_cleanup failed for db='%s'",
      dbname
  );

  *ret = (struct cgd_swarm_test){
      .committed         = mem_vhmap_create (NULL),
      .working           = NULL,
      .cur               = NULL,
      .db                = nsdb_open (dbname),
      .in_txn            = 0,
      .dbname            = dbname,
      .sample_space_prob = sample_space_prob,
  };

  CGD_SWMT_ASSERTF (
      ret->committed != NULL,
      "mem_vhmap_create returned NULL (db='%s')",
      dbname
  );
  CGD_SWMT_ASSERTF (
      ret->db != NULL,
      "nsdb_open returned NULL (db='%s')",
      dbname
  );

  memcpy (ret->enabled, start_enabled, CDS_AT_LEN * sizeof (int));
  cgd_swmt_set_allowed (ret);

  return ret;
}

void
cgd_swmt_close (struct cgd_swarm_test *meta)
{
  if (meta->in_txn)
  {
    cgd_swmt_commit_txn (meta);
  }
  CGD_SWMT_ASSERTF (
      nsdb_close (meta->db) == 0,
      "nsdb_close failed on db='%s'",
      meta->dbname
  );

  if (meta->committed)
  {
    mem_vhmap_free (meta->committed);
  }
  if (meta->working)
  {
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
  for (int i = 0; i < CDS_AT_LEN; ++i)
  {
    len += meta->allowed[i];
  }

  /* If the cgd_swarm has masked everything off, re-roll and try again next
   * step rather than divide by zero. */
  if (len == 0)
  {
    i_log_info ("No allowed actions — re-rolling enabled mask\n");
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
      if (choice == next)
      {
        break;
      }
      else
      {
        choice++;
      }
    }
  }

  enum cgd_action_type action = (enum cgd_action_type)index;
  i_log_info ("-> %s\n", cgd_action_names[action]);

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
  if (randf () <= meta->sample_space_prob)
  {
    cgd_swmt_set_random_enabled (meta);
    i_log_info ("Changing state space. After:\n");
    cgd_swmt_print_state (meta);
  }

  cgd_swmt_set_allowed (meta);
}
