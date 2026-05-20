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

#include "nscore/nshandle.h"

#include "c_specx.h"
#include "nscore/pager.h"

// nsh

struct nshandle *nsh_remove_and_open (const char *name, error *e) {
  if (pgr_delete_single_file ("test", e)) { return NULL; }
  return nsh_open (name);
}

int nsh_perror (struct nshandle *ns, const char *prefix) {
  const char *err = nsh_strerror (ns);
  if (err) {
    return fprintf (stderr, "%s: %s\n", prefix, nsh_strerror (ns));
  } else {
    return fprintf (stderr, "%s: success\n", prefix);
  }
}

const char *nsh_strerror (struct nshandle *ns) {
  if (ns->e.cause_code < 0) {
    return ns->e.cause_msg;
  } else {
    return NULL;
  }
}

int nsh_cleanup (const char *path) {
  error e = error_create ();
  pgr_delete_single_file (path, &e);
  return error_trace (&e);
}

// nshandle_root

err_t nsh_root_close (struct nshandle_root *root, error *e) {
  ASSERT (root->count == 0);
  err_t err = pgr_close (root->p, e);
  i_free ((void *)root->path.data);
  i_free (root);
  return err;
}

err_t nsh_root_crash (struct nshandle_root *root, error *e) {
  ASSERT (root->count == 0);
  err_t err = pgr_crash (root->p, e);
  i_free ((void *)root->path.data);
  i_free (root);
  return err;
}

struct nshandle *nsh_root_load (struct nshandle_root *ns, error *e) {
  struct nshandle *ret = i_malloc (1, sizeof *ret, e);
  if (ret == NULL) { return NULL; }

  ret->root        = ns;
  ret->is_auto_txn = 0;
  ret->atx         = NULL;
  ret->e           = error_create ();
  ns->count++;

  return ret;
}

void nsh_root_release (struct nshandle_root *root, struct nshandle *sm) {
  ASSERT (root->count > 0);
  i_free (sm);
  root->count -= 1;
}

// Transactional Support
err_t nsh_auto_begin_txn (struct nshandle *sm, error *e) {
  if (sm->atx == NULL) {
    WRAP (pgr_begin_txn (&sm->tx, sm->root->p, e));
    sm->is_auto_txn = 1;
    sm->atx         = &sm->tx;
  }

  return SUCCESS;
}

err_t nsh_auto_commit (struct nshandle *sm, error *e) {
  if (sm->is_auto_txn) {
    ASSERT (sm->atx);
    WRAP (pgr_commit (sm->root->p, sm->atx, e));
    sm->atx = NULL;
  }
  return SUCCESS;
}

void nsh_auto_rollback (struct nshandle *sm) {
  if (pgr_rollback (sm->root->p, sm->atx, 0, &sm->e)) { panic ("Failed to rollback"); }
  sm->atx = NULL;
}
