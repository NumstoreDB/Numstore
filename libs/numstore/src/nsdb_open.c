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
#include "nscore/page_h.h"
#include "nscore/pager.h"
#include "nscore/types.h"
#include "nscore/var.h"
#include "nsdb.h"

static struct nsdb *_nsdb_open (const char *path, error *e) {
  struct nsdb_root *ret = i_malloc (1, sizeof *ret, e);
  page_h            hp  = page_h_create ();

  if (ret == NULL) { return NULL; }

  // Initialize inner values
  {
    ret->e     = error_create ();
    ret->count = 0;

    // path
    ret->path.len  = strlen (path);
    ret->path.data = i_malloc (ret->path.len, 1, e);
    if (ret->path.data == NULL) { goto failed; }

    // db
    ret->p = pgr_open_single_file (path, e);
    if (ret->p == NULL) { goto failed; }
  }

  // Upfront initialization
  if (pgr_isnew (ret->p)) {
    // Initialize the upfront hash page
    if (ns_init_var_hash_map (ret->p, e)) { goto failed; }

    // Create the default variable
    {
      // BEGIN TXN
      struct txn tx;
      if (pgr_begin_txn (&tx, ret->p, e)) { goto failed; }

      struct ns_var_create_params params = {
          .p     = ret->p,
          .tx    = &tx,
          .vname = strfcstr (DEFAULT_VARIABLE),
          .type  = &(struct type){.type = T_PRIM, .p = U8},
      };
      if (ns_var_create (params, e)) { goto failed; }

      // COMMIT
      if (pgr_commit (ret->p, &tx, e)) { goto failed; }
    }
  }

  // Load the default context
  struct nsdb *sret = _nsdb_root_load (ret, e);

  return sret;

failed:
  // TODO just delete the file
  i_free (ret);
  return NULL;
}

nsdb_t *nsdb_open (const char *path) {
  error        e   = error_create ();
  struct nsdb *ret = _nsdb_open (path, &e);
  if (ret == NULL) { return NULL; }
  return ret;
}
