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

#include "c_specx.h"
#include "nscore/compile_config.h"
#include "nscore/nshandle.h"
#include "nscore/page_h.h"
#include "nscore/pager.h"
#include "nscore/types.h"
#include "nscore/var.h"

static struct nshandle *_nsh_open (const char *path, error *e) {
  struct nshandle_root *ret = i_malloc (1, sizeof *ret, e);
  page_h                hp  = page_h_create ();

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
  }

  // Load the default context
  struct nshandle *sret = nsh_root_load (ret, e);

  return sret;

failed:
  // TODO just delete the file
  i_free (ret);
  return NULL;
}

struct nshandle *nsh_open (const char *path) {
  error            e   = error_create ();
  struct nshandle *ret = _nsh_open (path, &e);
  if (ret == NULL) { return NULL; }
  return ret;
}
