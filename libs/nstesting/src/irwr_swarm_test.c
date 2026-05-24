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

#include "irwr_swarm_test.h"

#include "_numstore.h"
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
irwr_swmt_assert (int result)
{
  if (!result)
  {
    printf ("Failed irwr_swarm test\n");
    abort ();
  }
}

static struct block_array *
active_db (struct irwr_swarm_test *meta)
{ return meta->in_txn ? meta->working : meta->committed; }

static void
irwr_swmt_random_slice (int total, int *ofst, int *stride, int *len)
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
 * Given N action types, the irwr_swarm action space is the power set minus the
 * empty set: 2^N - 1 non-empty subsets. We sample uniformly by drawing an
 * integer in [1, 2^N) and interpreting each bit as inclusion.
 */
static void
irwr_swmt_set_random_enabled (struct irwr_swarm_test *meta)
{
  int mask = rand () % ((1 << IRWR_AT_LEN) - 1) + 1;
  for (int i = 0; i < IRWR_AT_LEN; ++i) { meta->enabled[i] = (mask >> i) & 1; }
}

/**
 * Recompute which actions can actually fire right now.
 *
 * An action is `allowed` iff (a) it is in the current irwr_swarm (`enabled`)
 * AND (b) the test state permits it. CREATE and DELETE are permitted
 * inside a transaction because the block_array snapshot model gives
 * them transactional semantics for free.
 */
static void
irwr_swmt_set_allowed (struct irwr_swarm_test *meta)
{
  assert (meta);
  assert (meta->db);
  assert (meta->dbname);

  memset (meta->allowed, 0, sizeof (meta->allowed));

  struct block_array *db = active_db (meta);

  /* CRASH is always permissible if irwr_swarm-enabled */
  meta->allowed[IRWR_CRASH_AND_REOPEN] = meta->enabled[IRWR_CRASH_AND_REOPEN];

  /* Txn-state-dependent ops */
  if (!meta->in_txn)
  {
    meta->allowed[IRWR_BEGIN_TXN]        = meta->enabled[IRWR_BEGIN_TXN];
    meta->allowed[IRWR_CLOSE_AND_REOPEN] = meta->enabled[IRWR_CLOSE_AND_REOPEN];
  }
  else
  {
    meta->allowed[IRWR_COMMIT_TXN]   = meta->enabled[IRWR_COMMIT_TXN];
    meta->allowed[IRWR_ROLLBACK_TXN] = meta->enabled[IRWR_ROLLBACK_TXN];
  }

  /* Variable-dependent ops */
  meta->allowed[IRWR_INSERT] = meta->enabled[IRWR_INSERT];
  if (meta->len > 0)
  {
    meta->allowed[IRWR_REMOVE] = meta->enabled[IRWR_REMOVE];
    meta->allowed[IRWR_READ]   = meta->enabled[IRWR_READ];
    meta->allowed[IRWR_WRITE]  = meta->enabled[IRWR_WRITE];
  }
}

///////////////////////////////////////////////////////////
/// Main Api

struct irwr_swarm_test *
irwr_swmt_open (
    int         start_enabled[IRWR_AT_LEN],
    const char *dbname,
    int         max_insert_len,
    const char *varname,
    const char *vartype,
    u32         esize
)
{
  struct irwr_swarm_test *ret = malloc (sizeof *ret);
  irwr_swmt_assert (ret != NULL);

  irwr_swmt_assert (nsdb_cleanup (dbname) == 0);

  *ret = (struct irwr_swarm_test){
      .committed      = block_array_create (512, NULL),
      .working        = NULL,
      .db             = nsdb_open (dbname),
      .in_txn         = 0,
      .dbname         = dbname,
      .varname        = varname,
      .vartype        = vartype,
      .esize          = esize,
      .max_insert_len = max_insert_len,
      .len            = 0,
  };

  irwr_swmt_assert (ret->committed != NULL);
  irwr_swmt_assert (ret->db != NULL);

  memcpy (ret->enabled, start_enabled, IRWR_AT_LEN * sizeof (int));
  irwr_swmt_set_allowed (ret);

  irwr_swmt_assert (nsdb_create (ret->db, varname, vartype) == 0);

  return ret;
}

void
irwr_swmt_close (struct irwr_swarm_test *meta)
{
  if (meta->in_txn) { irwr_swmt_commit_txn (meta); }
  irwr_swmt_assert (nsdb_close (meta->db) == 0);

  if (meta->committed) { block_array_free (meta->committed); }
  if (meta->working) { block_array_free (meta->working); }
  free (meta);
}

void
irwr_swmt_step (struct irwr_swarm_test *meta)
{
  meta->allowed[IRWR_CRASH_AND_REOPEN] = 0;
  meta->allowed[IRWR_CLOSE_AND_REOPEN] = 0;
  meta->allowed[IRWR_ROLLBACK_TXN]     = 0;
  meta->allowed[IRWR_COMMIT_TXN]       = 0;
  meta->allowed[IRWR_BEGIN_TXN]        = 0;

  /* Count allowed actions */
  int len = 0;
  for (int i = 0; i < IRWR_AT_LEN; ++i) { len += meta->allowed[i]; }

  /* If the irwr_swarm has masked everything off, re-roll and try again next
   * step rather than divide by zero. */
  if (len == 0)
  {
    irwr_swmt_set_random_enabled (meta);
    irwr_swmt_set_allowed (meta);
    return;
  }

  /* Pick the n-th allowed action */
  int next   = rand () % len;
  int index  = 0;
  int choice = 0;
  for (; index < IRWR_AT_LEN; ++index)
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
    case IRWR_BEGIN_TXN: irwr_swmt_begin_txn (meta); break;
    case IRWR_COMMIT_TXN: irwr_swmt_commit_txn (meta); break;
    case IRWR_ROLLBACK_TXN: irwr_swmt_rollback_txn (meta); break;
    case IRWR_CRASH_AND_REOPEN: irwr_swmt_crash_and_reopen (meta); break;
    case IRWR_CLOSE_AND_REOPEN: irwr_swmt_close_and_reopen (meta); break;
    case IRWR_INSERT: irwr_swmt_insert (meta); break;
    case IRWR_REMOVE: irwr_swmt_remove (meta); break;
    case IRWR_READ: irwr_swmt_read (meta); break;
    case IRWR_WRITE: irwr_swmt_write (meta); break;
    default: assert (0);
  }

  irwr_swmt_set_allowed (meta);
}

///////////////////////////////////////////////////////////
/// Concrete Actions

void
irwr_swmt_begin_txn (struct irwr_swarm_test *meta)
{
  assert (!meta->in_txn);
  assert (meta->working == NULL);

  irwr_swmt_assert (nsdb_begin (meta->db) == 0);

  meta->working = block_array_create (512, NULL);
  irwr_swmt_assert (meta->working != NULL);
  meta->in_txn = 1;
}

/**
 * Commit the open transaction. The working snapshot becomes the new
 * committed; the old committed is freed.
 */
void
irwr_swmt_commit_txn (struct irwr_swarm_test *meta)
{
  assert (meta->in_txn);
  assert (meta->working != NULL);

  irwr_swmt_assert (nsdb_commit (meta->db) == 0);

  block_array_free (meta->committed);
  meta->committed = meta->working;
  meta->working   = NULL;
  meta->in_txn    = 0;
}

/**
 * Rollback the open transaction. Throws away the working snapshot;
 * committed is untouched.
 */
void
irwr_swmt_rollback_txn (struct irwr_swarm_test *meta)
{
  assert (meta->in_txn);

  irwr_swmt_assert (nsdb_rollback (meta->db) == 0);

  block_array_free (meta->working);
  meta->working = NULL;
  meta->in_txn  = 0;
  meta->len     = block_array_getlen (meta->committed) / (u64)meta->esize;
}

/**
 * Forcibly crash the database, then reopen it. Reference-side this behaves
 * like a rollback: any in-flight working snapshot is discarded.
 */
void
irwr_swmt_crash_and_reopen (struct irwr_swarm_test *meta)
{
  irwr_swmt_assert (_nsdb_crash (meta->db) == 0);

  meta->db = nsdb_open (meta->dbname);
  irwr_swmt_assert (meta->db != NULL);

  if (meta->working)
  {
    block_array_free (meta->working);
    meta->working = NULL;
  }
  meta->in_txn = 0;
  meta->len    = block_array_getlen (meta->committed) / (u64)meta->esize;
}

void
irwr_swmt_close_and_reopen (struct irwr_swarm_test *meta)
{
  assert (!meta->in_txn);
  irwr_swmt_assert (nsdb_close (meta->db) == 0);

  meta->db = nsdb_open (meta->dbname);
  irwr_swmt_assert (meta->db != NULL);
}

void
irwr_swmt_insert (struct irwr_swarm_test *meta)
{
  int len  = (rand () % meta->max_insert_len) + 1;
  int ofst = rand () % (meta->len + 1);

  int      blen = len * (int)meta->esize;
  uint8_t *data = malloc ((size_t)blen);
  irwr_swmt_assert (data != NULL);
  for (int i = 0; i < blen; ++i) { data[i] = (uint8_t)rand (); }

  /* DB side */
  irwr_swmt_assert (nsdb_insert (meta->db, meta->varname, data, ofst, len) == len);

  /* Reference side */
  irwr_swmt_assert (
      block_array_insert (active_db (meta), (u32)(ofst * (int)meta->esize), data, (u32)blen, NULL)
      == 0
  );

  meta->len += len;

  free (data);
}

void
irwr_swmt_remove (struct irwr_swarm_test *meta)
{
  int ofst, stride, len;
  irwr_swmt_random_slice (meta->len, &ofst, &stride, &len);

  size_t   buf_sz  = (size_t)len * (size_t)meta->esize;
  uint8_t *db_buf  = calloc (1, buf_sz);
  uint8_t *ref_buf = calloc (1, buf_sz);
  irwr_swmt_assert (db_buf && ref_buf);

  /* DB side */
  irwr_swmt_assert (
      nsdb_remove (meta->db, meta->varname, db_buf, ofst, stride, ofst + len * stride, 0xFF) == len
  );

  /* Reference side */
  struct stride str = to_block_stride (ofst, stride, len);
  i64           got = block_array_remove (active_db (meta), str, meta->esize, ref_buf, NULL);
  irwr_swmt_assert (got == (i64)len);

  /* Cross-check */
  irwr_swmt_assert (memcmp (db_buf, ref_buf, buf_sz) == 0);

  meta->len -= len;

  free (db_buf);
  free (ref_buf);
}

void
irwr_swmt_read (struct irwr_swarm_test *meta)
{
  int ofst, stride, len;
  irwr_swmt_random_slice (meta->len, &ofst, &stride, &len);

  size_t   buf_sz  = (size_t)len * (size_t)meta->esize;
  uint8_t *db_buf  = calloc (1, buf_sz);
  uint8_t *ref_buf = calloc (1, buf_sz);
  irwr_swmt_assert (db_buf && ref_buf);

  irwr_swmt_assert (
      nsdb_read (meta->db, meta->varname, db_buf, ofst, stride, ofst + len * stride, 0xFF) == len
  );

  struct stride str = to_block_stride (ofst, stride, len);
  u64           got = block_array_read (active_db (meta), str, meta->esize, ref_buf);
  irwr_swmt_assert (got == (u64)len);

  irwr_swmt_assert (memcmp (db_buf, ref_buf, buf_sz) == 0);

  free (db_buf);
  free (ref_buf);
}

void
irwr_swmt_write (struct irwr_swarm_test *meta)
{
  int ofst, stride, len;
  irwr_swmt_random_slice (meta->len, &ofst, &stride, &len);

  int      blen = len * (int)meta->esize;
  uint8_t *data = malloc ((size_t)blen);
  irwr_swmt_assert (data != NULL);
  for (int i = 0; i < blen; ++i) { data[i] = (uint8_t)rand (); }

  irwr_swmt_assert (
      nsdb_write (meta->db, meta->varname, data, ofst, stride, ofst + len * stride, 0xFF) == len
  );

  struct stride str = to_block_stride (ofst, stride, len);
  u64           got = block_array_write (active_db (meta), str, meta->esize, data);
  irwr_swmt_assert (got == (u64)len);

  free (data);
}
