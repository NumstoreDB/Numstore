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

#include "_nsdb.h"
#include "c_specx.h"
#include "nscore/pager.h"
#include "nsdb.h"

// nsdb

struct nsdb *_nsdb_remove_and_open (const char *name, error *e) {
  if (pgr_delete_single_file (name, e)) { return NULL; }
  return nsdb_open (name);
}

int nsdb_perror (nsdb_t *ns, const char *prefix) {
  const char *err = nsdb_strerror (ns);
  if (err) {
    return fprintf (stderr, "%s: %s\n", prefix, nsdb_strerror (ns));
  } else {
    return fprintf (stderr, "%s: success\n", prefix);
  }
}

const char *nsdb_strerror (nsdb_t *ns) {
  if (ns->e.cause_code < 0) {
    return ns->e.cause_msg;
  } else {
    return NULL;
  }
}

int nsdb_cleanup (const char *path) {
  error e = error_create ();
  pgr_delete_single_file (path, &e);
  return error_trace (&e);
}

// nsdb_root

err_t _nsdb_root_close (struct nsdb_root *root, error *e) {
  ASSERT (root->count == 0);
  err_t err = pgr_close (root->p, e);
  i_free ((void *)root->path.data);
  i_free (root);
  return err;
}

err_t _nsdb_root_crash (struct nsdb_root *root, error *e) {
  ASSERT (root->count == 0);
  err_t err = pgr_crash (root->p, e);
  i_free ((void *)root->path.data);
  i_free (root);
  return err;
}

struct nsdb *_nsdb_root_load (struct nsdb_root *ns, error *e) {
  struct nsdb *ret = i_malloc (1, sizeof *ret, e);
  if (ret == NULL) { return NULL; }

  ret->root        = ns;
  ret->is_auto_txn = 0;
  ret->atx         = NULL;
  ret->e           = error_create ();
  ns->count++;

  return ret;
}

void _nsdb_root_release (struct nsdb_root *root, struct nsdb *sm) {
  ASSERT (root->count > 0);
  i_free (sm);
  root->count -= 1;
}

// Core Operations
sb_size nsdb_size (nsdb_t *ns) { return nsdb_psize (ns, NULL); }

sb_size nsdb_insert (nsdb_t *ns, const void *src, sb_size bofst, b_size slen) {
  return nsdb_pinsert (ns, NULL, src, bofst, slen);
}

sb_size nsdb_write (nsdb_t *ns, const void *src, b_size bofst, b_size nelem) {
  return nsdb_pwrite (ns, NULL, src, 1, bofst, 1, nelem);
}

sb_size nsdb_read (nsdb_t *ns, void *dest, sb_size bofst, b_size nelem) {
  return nsdb_pread (ns, NULL, dest, 1, bofst, 1, nelem);
}

sb_size nsdb_remove (nsdb_t *ns, void *dest, sb_size bofst, b_size nelem) {
  return nsdb_premove (ns, NULL, dest, 1, bofst, 1, nelem);
}

// Transactional Support
err_t _nsdb_auto_begin_txn (struct nsdb *sm, error *e) {
  if (sm->atx == NULL) {
    WRAP (pgr_begin_txn (&sm->tx, sm->root->p, e));
    sm->is_auto_txn = 1;
    sm->atx         = &sm->tx;
  }

  return SUCCESS;
}

err_t _nsdb_auto_commit (struct nsdb *sm, error *e) {
  if (sm->is_auto_txn) {
    ASSERT (sm->atx);
    WRAP (pgr_commit (sm->root->p, sm->atx, e));
    sm->atx = NULL;
  }
  return SUCCESS;
}

void _nsdb_auto_rollback (struct nsdb *sm) {
  if (pgr_rollback (sm->root->p, sm->atx, 0, &sm->e)) {
    panic ("Failed to rollback");
  }
  sm->atx = NULL;
}
