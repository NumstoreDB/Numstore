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

#include "nshandle.h"

#include "pager.h"
#include "var_algorithms.h"

#ifdef TESTING
#  include "testing/testing.h"
#endif

struct nshandle *
nsh_remove_and_open (const char *name, error *e)
{
  if (pgr_delete_single_file (name, e))
  {
    return NULL;
  }
  return nsh_open (name);
}

err_t
nsh_root_crash (struct nshandle_root *root, error *e)
{
  ASSERT (root->count == 0);
  err_t err = pgr_crash (root->p, e);
  i_free ((void *)root->path.data);
  i_free (root);
  return err;
}

int
nsh_perror (struct nshandle *ns, const char *prefix)
{
  const char *err = nsh_strerror (ns);
  if (err)
  {
    return fprintf (stderr, "%s: %s\n", prefix, nsh_strerror (ns));
  }
  else
  {
    return fprintf (stderr, "%s: success\n", prefix);
  }
}

const char *
nsh_strerror (struct nshandle *ns)
{
  if (ns->e.cause_code < 0)
  {
    return ns->e.cause_msg;
  }
  else
  {
    return NULL;
  }
}

int
nsh_cleanup (const char *path)
{
  error e = error_create ();
  pgr_delete_single_file (path, &e);
  return error_trace (&e);
}

// nsh_root

err_t
nsh_root_close (struct nshandle_root *root, error *e)
{
  ASSERT (root->count == 0);
  err_t err = pgr_close (root->p, e);
  i_free ((void *)root->path.data);
  i_free (root);
  return err;
}

struct nshandle *
nsh_root_load (struct nshandle_root *ns, error *e)
{
  struct nshandle *ret = i_malloc (1, sizeof *ret, e);
  if (ret == NULL)
  {
    return NULL;
  }

  ret->root        = ns;
  ret->is_auto_txn = 0;
  ret->atx         = NULL;
  ret->e           = error_create ();
  ns->count++;

  return ret;
}

void
nsh_root_release (struct nshandle_root *root, struct nshandle *sm)
{
  ASSERT (root->count > 0);
  i_free (sm);
  root->count -= 1;
}

// Transactional Support
err_t
nsh_auto_begin_txn (struct nshandle *sm, error *e)
{
  if (sm->atx == NULL)
  {
    WRAP (pgr_begin_txn (&sm->tx, sm->root->p, e));
    sm->is_auto_txn = 1;
    sm->atx         = &sm->tx;
  }

  return SUCCESS;
}

err_t
nsh_auto_commit (struct nshandle *sm, error *e)
{
  if (sm->is_auto_txn)
  {
    ASSERT (sm->atx);
    WRAP (pgr_commit (sm->root->p, sm->atx, e));
    sm->atx = NULL;
  }
  return SUCCESS;
}

void
nsh_auto_rollback (struct nshandle *sm)
{
  if (pgr_rollback (sm->root->p, sm->atx, 0, &sm->e))
  {
    panic ("Failed to rollback");
  }
  sm->atx = NULL;
}

/////////////////////////////////////////////////////////////////////
////// Begin

static err_t
_nsh_begin (struct nshandle *smf, error *e)
{
  if (smf->atx)
  {
    return error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "Can't start another transaction, already a part of an existing "
        "transaction: %" PRtxid ". Either commit or rollback first",
        smf->atx->tid
    );
  }

  WRAP (pgr_begin_txn (&smf->tx, smf->root->p, &smf->e));

  smf->is_auto_txn = 0;
  smf->atx         = &smf->tx;

  return SUCCESS;
}

int
nsh_begin (struct nshandle *smf)
{
  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  return _nsh_begin (smf, &smf->e);
}

/////////////////////////////////////////////////////////////////////
////// Close

static err_t
_nsh_close (struct nshandle *n, error *e)
{
  struct nshandle_root *root = n->root;
  nsh_root_release (root, n);
  if (root->count == 0)
  {
    return nsh_root_close (root, &root->e);
  }
  return SUCCESS;
}

int
nsh_close (struct nshandle *ns)
{
  return _nsh_close (ns, &ns->e);
}

/////////////////////////////////////////////////////////////////////
////// Commit

static err_t
_nsh_commit (struct nshandle *smf, error *e)
{
  if (smf->atx == NULL)
  {
    return error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "Can't commit transaction, not a part of an existing transaction"
    );
  }

  WRAP (pgr_commit (smf->root->p, smf->atx, e));
  smf->atx = NULL;

  return SUCCESS;
}

int
nsh_commit (struct nshandle *smf)
{
  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  return _nsh_commit (smf, &smf->e);
}

/////////////////////////////////////////////////////////////////////
////// Rollback

static err_t
_nsh_rollback (struct nshandle *smf, error *e)
{
  if (smf->atx == NULL)
  {
    return error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "Can't rollback transaction, not a part of an existing transaction"
    );
  }

  WRAP (pgr_rollback (smf->root->p, smf->atx, 0, &smf->e));
  smf->atx = NULL;

  return SUCCESS;
}

int
nsh_rollback (struct nshandle *smf)
{
  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  return _nsh_rollback (smf, &smf->e);
}

/////////////////////////////////////////////////////////////////////
////// Crash

static err_t
_nsh_crash (struct nshandle *n, error *e)
{
  struct nshandle_root *root = n->root;

  err_t err = pgr_crash (root->p, e);
  i_free ((void *)root->path.data);
  i_free (n);
  i_free (root);

  return err;
}

int
nsh_crash (struct nshandle *ns)
{
  return _nsh_crash (ns, &ns->e);
}

/////////////////////////////////////////////////////////////////////
////// New Context

struct nshandle *
nsh_new_context (struct nshandle *ns)
{
  ns->e.cause_code = SUCCESS;
  ns->e.cmlen      = 0;
  return nsh_root_load (ns->root, &ns->e);
}

/////////////////////////////////////////////////////////////////////
////// Open

static struct nshandle *
_nsh_open (const char *path, error *e)
{
  struct nshandle_root *ret = i_malloc (1, sizeof *ret, e);
  page_h                hp  = page_h_create ();

  if (ret == NULL)
  {
    return NULL;
  }

  // Initialize inner values
  {
    ret->e     = error_create ();
    ret->count = 0;

    // path
    ret->path.len  = strlen (path);
    ret->path.data = i_malloc (ret->path.len, 1, e);
    if (ret->path.data == NULL)
    {
      goto failed;
    }

    // db
    ret->p = pgr_open (path, e);
    if (ret->p == NULL)
    {
      goto failed;
    }
  }

  // Upfront initialization
  if (pgr_isnew (ret->p))
  {
    // Initialize the upfront hash page
    if (ns_init_var_hash_map (ret->p, e))
    {
      goto failed;
    }
  }

  // Launch the checkpoint writer thread
  if (pgr_launch_checkpoint_thread (ret->p, 5000, e))
  {
    goto failed;
  }

  // Load the default context
  struct nshandle *sret = nsh_root_load (ret, e);

  return sret;

failed:
  // TODO just delete the file
  i_free (ret);
  return NULL;
}

struct nshandle *
nsh_open (const char *path)
{
  error            e   = error_create ();
  struct nshandle *ret = _nsh_open (path, &e);
  if (ret == NULL)
  {
    return NULL;
  }
  return ret;
}
