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

#include "nscore/nsdb/ns_nsdb.h"

#include "core/ns_concurrency.h"
#include "core/ns_error.h"
#include "core/ns_slab_alloc.h"
#include "core/os/ns_filesystem.h"
#include "core/os/ns_memory.h"
#include "nscore/algorithms/var/ns_var_algorithms.h"
#include "nscore/pager/ns_pager.h"

struct nsdb *
nsdb_open_with_resources (const char *path, struct i_mem mem, struct i_file_system fs, error *e)
{
  struct nsdb *ret = i_malloc (mem, 1, sizeof *ret, e);

  if (ret == NULL) {
    return NULL;
  }

  // Initialize inner values
  {
    // Trivial initializers
    slab_alloc_init (&ret->txn_alloc, mem, sizeof (struct ns_txn), 512);
    latch_init (&ret->l);
    ret->mem       = mem;
    ret->fs        = fs;
    ret->path.data = NULL;
    ret->p         = NULL;

    // Path
    ret->path.len  = strlen (path);
    ret->path.data = i_malloc (mem, ret->path.len, 1, e);
    if (ret->path.data == NULL) {
      goto failed;
    }

    // Pager
    ret->p = pgr_open (path, mem, fs, e);
    if (ret->p == NULL) {
      goto failed;
    }
  }

  // New pager - initialze the upfront hash map
  if ((pgr_isnew (ret->p)) && (ns_init_var_hash_map (ret->p, e)))
  // Initialize the upfront hash page
  {
    goto failed;
  }

  // Launch the checkpoint writer thread
  if (pgr_launch_checkpoint_thread (ret->p, 5000, e)) {
    goto failed;
  }

  return ret;

failed:
  if (ret->p) {
    pgr_close (ret->p, e);
  }
  i_free (mem, (void *)ret->path.data);
  i_free (mem, ret);
  pgr_delete_single_file (path, e);
  return NULL;
}

int
nsdb_cleanup (const char *path, error *e)
{
  pgr_delete_single_file (path, e);
  return error_trace (e);
}

err_t
nsdb_close (struct nsdb *n, error *e)
{
  e->cause_code = SUCCESS;
  e->cmlen      = 0;

  err_t ret     = pgr_close (n->p, e);
  slab_alloc_destroy (&n->txn_alloc);

  struct i_mem mem = n->mem;
  i_free (mem, (void *)n->path.data);
  i_free (mem, n);

  return ret;
}

err_t
nsdb_crash (struct nsdb *n, error *e)
{
  e->cause_code = SUCCESS;
  e->cmlen      = 0;

  err_t err     = pgr_crash (n->p, e);
  slab_alloc_destroy (&n->txn_alloc);

  struct i_mem mem = n->mem;
  i_free (mem, (void *)n->path.data);
  i_free (mem, n);

  return err;
}

struct ns_txn *
nsdb_begin (struct nsdb *smf, error *e)
{
  e->cause_code     = 0;
  e->cmlen          = 0;

  struct ns_txn *tx = slab_alloc_alloc (&smf->txn_alloc, e);
  if (tx == NULL) {
    return NULL;
  }

  if (pgr_begin_txn (tx, smf->p, e)) {
    slab_alloc_free (&smf->txn_alloc, tx);
    return NULL;
  }

  return tx;
}

err_t
nsdb_commit (struct nsdb *smf, struct ns_txn *tx, error *e)
{
  e->cause_code = SUCCESS;
  e->cmlen      = 0;

  if (pgr_commit (smf->p, tx, e)) {
    slab_alloc_free (&smf->txn_alloc, tx);
    return error_trace (e);
  }

  slab_alloc_free (&smf->txn_alloc, tx);
  return SUCCESS;
}

err_t
nsdb_rollback (struct nsdb *smf, struct ns_txn *tx, error *e)
{
  e->cause_code = SUCCESS;
  e->cmlen      = 0;

  if (pgr_rollback (smf->p, tx, 0, e)) {
    slab_alloc_free (&smf->txn_alloc, tx);
    return error_trace (e);
  }

  slab_alloc_free (&smf->txn_alloc, tx);
  return SUCCESS;
}
