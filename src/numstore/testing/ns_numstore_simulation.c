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
#include "core/ns_block_array.h"
#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_logging.h"
#include "core/ns_numerics.h"
#include "core/ns_stride.h"
#include "core/ns_string.h"
#include "core/os/ns_memory.h"
#include "nscore/nsdb/ns_nsdb.h"
#include "nscore/types/ns_types.h"
#include "nscore/types/ns_variables.h"
#include "numstore/numstore.h"
#include "numstore/testing/ns_mem_vhmap.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct nss_swarm_test
{
  int                   enabled[NSS_AT_LEN];
  int                   allowed[NSS_AT_LEN];

  nsdb_t               *db;
  int                   in_txn;
  const char           *dbname;
  const char           *varname;
  const char           *vartype;
  int                   max_insert_len;
  float                 sample_space_prob;

  // Fake database transaction semantics
  struct mem_vhmap     *committed;
  struct mem_vhmap     *working;

  struct var_with_data *cur;
};

/******************************************************************************
 * SECTION: Utilities
 * ----------------------------------------------------------------------------
 * @brief Assertion macros, action-name table, and state dumper used by every
 *        failure path in this fixture.
 ******************************************************************************/

static const char *const nss_action_names[NSS_AT_LEN] = {
    [NSS_BEGIN_TXN]        = "BEGIN_TXN",
    [NSS_COMMIT_TXN]       = "COMMIT_TXN",
    [NSS_ROLLBACK_TXN]     = "ROLLBACK_TXN",

    [NSS_CRASH_AND_REOPEN] = "CRASH_AND_REOPEN",
    [NSS_CLOSE_AND_REOPEN] = "CLOSE_AND_REOPEN",

    [NSS_INSERT]           = "INSERT",
    [NSS_REMOVE]           = "REMOVE",
    [NSS_READ]             = "READ",
    [NSS_WRITE]            = "WRITE",
};

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

/**
 * Dump the entire fixture state in a human-readable form. Safe to call
 * with NULL.
 */
static void
nss_print_state (const struct nss_swarm_test *meta)
{
  i_log_info ("=== NSS Swarm Test State ===\n");
  if (meta == NULL) {
    i_log_info ("  (meta is NULL)\n");
    return;
  }
  i_log_info ("  dbname:         %s\n", meta->dbname);
  i_log_info ("  varname:        %s\n", meta->varname);
  i_log_info ("  vartype:        %s\n", meta->vartype);
  i_log_info ("  esize:          %u bytes/elem\n", (unsigned)type_byte_size (meta->cur->var.dtype));
  i_log_info ("  len:            %" PRb_size " elems\n", block_array_getlen (meta->cur->data));
  i_log_info ("  max_insert_len: %d\n", meta->max_insert_len);
  i_log_info ("  in_txn:         %s\n", meta->in_txn ? "yes" : "no");
  i_log_info ("  committed:      %s\n", meta->committed ? "present" : "<null>");
  i_log_info ("  working:        %s\n", meta->working ? "present" : "<null>");
  i_log_info ("  sample_space_p: %.3f\n", (double)meta->sample_space_prob);
  i_log_info ("  Actions             enabled   allowed\n");
  for (int i = 0; i < NSS_AT_LEN; ++i) {
    i_log_info (
        "    %-18s   %-3s       %-3s\n",
        nss_action_names[i],
        meta->enabled[i] ? "yes" : "no",
        meta->allowed[i] ? "yes" : "no"
    );
  }
}

static struct mem_vhmap *
active_db (struct nss_swarm_test *meta)
{
  return meta->in_txn ? meta->working : meta->committed;
}

static void
refresh_cur (struct nss_swarm_test *meta)
{
  // Replace cur with the variable in working
  if (meta->cur) {
    meta->cur = mem_vhmap_get (active_db (meta), meta->cur->var.vname);
    ASSERT (meta->cur);
  }
}

static void
nss_random_slice (int total, int *ofst, int *stride, int *len)
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
nss_set_random_enabled (struct nss_swarm_test *meta)
{
  int mask = rand () % ((1 << NSS_AT_LEN) - 1) + 1;
  for (int i = 0; i < NSS_AT_LEN; ++i) {
    meta->enabled[i] = (mask >> i) & 1;
  }
}

static void
nss_set_allowed (struct nss_swarm_test *meta)
{
  ASSERT (meta);
  ASSERT (meta->db);
  ASSERT (meta->dbname);

  memset (meta->allowed, 0, sizeof (meta->allowed));

  meta->allowed[NSS_CRASH_AND_REOPEN] = meta->enabled[NSS_CRASH_AND_REOPEN];

  if (!meta->in_txn) {
    meta->allowed[NSS_BEGIN_TXN]        = meta->enabled[NSS_BEGIN_TXN];
    meta->allowed[NSS_CLOSE_AND_REOPEN] = meta->enabled[NSS_CLOSE_AND_REOPEN];
  } else {
    meta->allowed[NSS_COMMIT_TXN]   = meta->enabled[NSS_COMMIT_TXN];
    meta->allowed[NSS_ROLLBACK_TXN] = meta->enabled[NSS_ROLLBACK_TXN];
  }

  meta->allowed[NSS_INSERT] = meta->enabled[NSS_INSERT];
  if (block_array_getlen (meta->cur->data) > 0) {
    meta->allowed[NSS_REMOVE] = meta->enabled[NSS_REMOVE];
    meta->allowed[NSS_READ]   = meta->enabled[NSS_READ];
    meta->allowed[NSS_WRITE]  = meta->enabled[NSS_WRITE];
  }
}

/******************************************************************************
 * SECTION: Concrete Actions
 ******************************************************************************/

static int
nss_begin_txn (struct nss_swarm_test *meta)
{
  ASSERT (!meta->in_txn);
  ASSERT (meta->working == NULL);

  // Begin Transaction
  if (nsdb_begin (meta->db) < 0) {
    return -1;
  }

  // Copy on write
  meta->working = mem_vhmap_clone (meta->committed, NULL);
  if (meta->working == NULL) {
    return -1;
  }

  refresh_cur (meta);

  meta->in_txn = 1;

  return 0;
}

static int
nss_commit_txn (struct nss_swarm_test *meta)
{
  ASSERT (meta->in_txn);
  ASSERT (meta->working != NULL);

  if (nsdb_commit (meta->db, NULL) < 0) {
    return -1;
  }

  mem_vhmap_free (meta->committed);
  meta->committed = meta->working;
  meta->working   = NULL;
  meta->in_txn    = 0;

  refresh_cur (meta);

  return 0;
}

static int
nss_rollback_txn (struct nss_swarm_test *meta)
{
  ASSERT (meta->in_txn);

  if (nsdb_rollback (meta->db, NULL) < 0) {
    return -1;
  }

  mem_vhmap_free (meta->working);
  meta->working = NULL;
  meta->in_txn  = 0;

  refresh_cur (meta);

  return 0;
}

static int
nss_crash_and_reopen (struct nss_swarm_test *meta)
{
  if (nsdb_crash (meta->db) < 0) {
    return -1;
  }

  // Re open
  meta->db = nsdb_open (meta->dbname);
  if (meta->db == NULL) {
    return -1;
  }

  // If we were in the middle of a transaction
  // revert back to the previous one
  if (meta->working) {
    ASSERT (meta->in_txn);

    mem_vhmap_free (meta->working);
    meta->working = NULL;

    refresh_cur (meta);

    meta->in_txn = 0;
  } else {
    ASSERT (!meta->in_txn);
  }

  return 0;
}

static int
nss_close_and_reopen (struct nss_swarm_test *meta)
{
  ASSERT (!meta->in_txn);

  // Close the database
  if (nsdb_close (meta->db) < 0) {
    return -1;
  }

  // Re open
  meta->db = nsdb_open (meta->dbname);
  if (meta->db == NULL) {
    return -1;
  }

  return 0;
}

static int
nss_create (struct nss_swarm_test *meta)
{
  struct mem_vhmap *db = active_db (meta);

  // Loop until you get a unique variable name
  while (true) {
    ALLOC_INIT (temp);

    error        e    = error_create ();
    char        *name = random_name ();

    struct type *type = type_random (&temp, get_random_type_depth (), &e);
    if (type == NULL) {
      return -1;
    }

    char *typestr = type_tostr (type);
    if (typestr == NULL) {
      return -1;
    }

    // Already exists - try again
    if (mem_vhmap_get (db, strfcstr (name)) != NULL) {
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

    // Create the variable in the database
    if (nsdb_fexecute (meta->db, "create %s %s", NULL, name, typestr) < 0) {
      return -1;
    }

    // Create the variable in the reference side
    if (mem_vhmap_add (db, &var, &e) < 0) {
      return -1;
    }

    // If there is no variable - then set it to this one
    if (meta->cur == NULL) {
      meta->cur = mem_vhmap_get (db, var.vname);
    }

    free (name);
    free (typestr);
    ALLOC_CLOSE (temp);

    return 0;
  }
}

static int
cgd_switch (struct nss_swarm_test *meta)
{
  struct mem_vhmap *db = active_db (meta);
  meta->cur            = mem_vhmap_random (db);
  if (meta->cur == NULL) {
    return -1;
  }

  return 0;
}

static int
cgd_delete (struct nss_swarm_test *meta)
{
  ASSERT (meta->cur != NULL);

  const char *name = meta->cur->var.vname.data;
  if (nsdb_fexecute (meta->db, "delete %s", NULL, name) < 0) {
    return -1;
  }

  mem_vhmap_remove (active_db (meta), meta->cur->var.vname);
  meta->cur = mem_vhmap_random (active_db (meta));

  return 0;
}

static int
nss_insert (struct nss_swarm_test *meta)
{
  // Choose a random length
  int      len  = randu32r (1, meta->max_insert_len);
  int      ofst = randu32r (1, block_array_getlen (meta->cur->data));

  // Create the data to insert
  int      blen = len * (int)type_byte_size (meta->cur->var.dtype);
  uint8_t *data = malloc ((size_t)blen);
  if (data == NULL) {
    return -1;
  }
  for (int i = 0; i < blen; ++i) {
    data[i] = (uint8_t)rand ();
  }

  // Do the database side
  if (nsdb_fexecute (meta->db, "insert %s %d %d", data, meta->varname, ofst, len) != len) {
    free (data);
    return -1;
  }

  // Do the reference side
  int ba = block_array_insert (
      meta->cur->data,
      (u32)(ofst * (int)type_byte_size (meta->cur->var.dtype)),
      data,
      (u32)blen,
      NULL
  );
  if (ba != 0) {
    free (data);
    return -1;
  }

  free (data);

  return 0;
}

static int
nss_remove (struct nss_swarm_test *meta)
{
  // Generate a random slice
  int ofst, stride, len;
  nss_random_slice (block_array_getlen (meta->cur->data), &ofst, &stride, &len);

  // Get the true size of the buffer
  size_t   buf_sz = (size_t)len * (size_t)type_byte_size (meta->cur->var.dtype);

  // Output database buffer
  uint8_t *db_buf = calloc (1, buf_sz);
  if (db_buf == NULL) {
    return -1;
  }

  // Output reference buffer
  uint8_t *ref_buf = calloc (1, buf_sz);
  if (ref_buf == NULL) {
    return -1;
  }

  // Do the database remove
  int exec_ret = nsdb_fexecute (
      meta->db,
      "remove %s[%d:%d:%d]",
      db_buf,
      meta->varname,
      ofst,
      ofst + len * stride,
      stride
  );
  if (exec_ret < 0) {
    return -1;
  }

  /* Reference side */
  struct stride str = to_block_stride (ofst, stride, len);
  i64           got = block_array_remove (
      meta->cur->data,
      str,
      type_byte_size (meta->cur->var.dtype),
      ref_buf,
      NULL
  );

  // Check that we got the same amount of data
  if (got != len) {
    return -1;
  }

  // Check that the two buffers are the same
  if (memcmp (db_buf, ref_buf, buf_sz) != 0) {
    return -1;
  }

  free (db_buf);
  free (ref_buf);

  return 0;
}

static int
nss_read (struct nss_swarm_test *meta)
{
  int ofst, stride, len;
  nss_random_slice (block_array_getlen (meta->cur->data), &ofst, &stride, &len);

  // Get the true size of the buffer
  size_t   buf_sz = (size_t)len * (size_t)type_byte_size (meta->cur->var.dtype);

  // Output database buffer
  uint8_t *db_buf = calloc (1, buf_sz);
  if (db_buf == NULL) {
    return -1;
  }

  // Output reference buffer
  uint8_t *ref_buf = calloc (1, buf_sz);
  if (ref_buf == NULL) {
    return -1;
  }

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

  if (exec_ret < 0) {
    return -1;
  }

  /* Reference side */
  struct stride str = to_block_stride (ofst, stride, len);
  i64           got = block_array_remove (
      meta->cur->data,
      str,
      type_byte_size (meta->cur->var.dtype),
      ref_buf,
      NULL
  );

  // Check that we got the same amount of data
  if (got != len) {
    return -1;
  }

  // Check that the two buffers are the same
  if (memcmp (db_buf, ref_buf, buf_sz) != 0) {
    return -1;
  }

  free (db_buf);
  free (ref_buf);

  return 0;
}

static int
nss_write (struct nss_swarm_test *meta)
{
  // Generate a random slice
  int ofst, stride, len;
  nss_random_slice (block_array_getlen (meta->cur->data), &ofst, &stride, &len);

  // Initialize the data to insert
  int      blen = len * (int)type_byte_size (meta->cur->var.dtype);
  uint8_t *data = malloc ((size_t)blen);
  if (data == NULL) {
    return -1;
  }
  for (int i = 0; i < blen; ++i) {
    data[i] = (uint8_t)rand ();
  }

  // Database side
  int exec_ret = nsdb_fexecute (
      meta->db,
      "write %s[%d:%d:%d]",
      data,
      meta->varname,
      ofst,
      ofst + len * stride,
      stride
  );
  if (exec_ret < 0) {
    return -1;
  }

  struct stride str = to_block_stride (ofst, stride, len);
  u64 got = block_array_write (meta->cur->data, str, type_byte_size (meta->cur->var.dtype), data);
  if (got != (u64)len) {
    return -1;
  }

  free (data);

  return 0;
}

/******************************************************************************
 * SECTION: Main Api
 ******************************************************************************/

struct nss_swarm_test *
nss_open (
    int         initial_enabled[NSS_AT_LEN],
    const char *dbname,
    int         max_insert_len,
    const char *varname,
    const char *vartype,
    float       sample_space_prob
)
{
  ASSERT (sample_space_prob >= 0 && sample_space_prob <= 1);

  if (nsdb_cleanup (dbname) < 0) {
    return NULL;
  }

  struct nss_swarm_test *ret = malloc (sizeof *ret);
  if (ret == NULL) {
    return NULL;
  }

  *ret = (struct nss_swarm_test){
      .committed         = mem_vhmap_create (default_mem (), NULL),
      .working           = NULL,
      .db                = nsdb_open (dbname),
      .in_txn            = 0,
      .dbname            = dbname,
      .max_insert_len    = max_insert_len,
      .sample_space_prob = sample_space_prob,
  };

  if (ret->committed == NULL) {
    return NULL;
  }
  if (ret->db == NULL) {
    return NULL;
  }

  memcpy (ret->enabled, initial_enabled, NSS_AT_LEN * sizeof (int));
  nss_set_allowed (ret);

  return ret;
}

int
nss_close (struct nss_swarm_test *meta)
{
  if (meta->in_txn) {
    nss_commit_txn (meta);
  }

  if (nsdb_close (meta->db) < 0) {
    return -1;
  }

  if (meta->committed) {
    mem_vhmap_free (meta->committed);
  }
  if (meta->working) {
    mem_vhmap_free (meta->working);
  }
  free (meta);

  return 0;
}

enum ns_action_type
nss_get_random_action (struct nss_swarm_test *meta)
{
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
    i_log_info ("-> %s\n", nss_action_names[action]);

    return action;
  }
}

void
nss_step (struct nss_swarm_test *meta)
{
  nss_print_state (meta);

  enum ns_action_type action = nss_get_random_action (meta);

  switch (action) {
    case NSS_BEGIN_TXN: nss_begin_txn (meta); break;
    case NSS_COMMIT_TXN: nss_commit_txn (meta); break;
    case NSS_ROLLBACK_TXN: nss_rollback_txn (meta); break;
    case NSS_CRASH_AND_REOPEN: nss_crash_and_reopen (meta); break;
    case NSS_CLOSE_AND_REOPEN: nss_close_and_reopen (meta); break;
    case NSS_CREATE: break;
    case NSS_SWITCH: break;
    case NSS_DELETE: break;
    case NSS_INSERT: nss_insert (meta); break;
    case NSS_REMOVE: nss_remove (meta); break;
    case NSS_READ: nss_read (meta); break;
    case NSS_WRITE: nss_write (meta); break;
    default: UNREACHABLE ();
  }

  // Choose a set of randomized actions
  if (randf () <= meta->sample_space_prob) {
  }

  nss_set_allowed (meta);
}
