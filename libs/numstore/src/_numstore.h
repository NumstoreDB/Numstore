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

#pragma once

#include "c_specx.h"
#include "nscore/pager.h"
#include "nscore/variables.h"

struct nsdb_root {
  struct pager *p;     // The database resources
  struct string path;  // Path to the database
  int           count; // When this reaches 0 - close the root
  error         e;
};

struct nsdb {
  // Shared root file
  struct nsdb_root *root;

  int         is_auto_txn; // If atx is an auto transaction
  struct txn *atx;         // Active transaction
  struct txn  tx;          // Transaction storage

  error e;
};

struct nsdb *_nsdb_remove_and_open (const char *name, error *e);
int          _nsdb_crash (struct nsdb *ns);

// Auto Transactions
err_t _nsdb_auto_begin_txn (struct nsdb *sm, error *e);
err_t _nsdb_auto_commit (struct nsdb *sm, error *e);
void  _nsdb_auto_rollback (struct nsdb *sm);

err_t        _nsdb_root_close (struct nsdb_root *root, error *e);
err_t        _nsdb_root_crash (struct nsdb_root *root, error *e);
struct nsdb *_nsdb_root_load (struct nsdb_root *root, error *e);
void         _nsdb_root_release (struct nsdb_root *root, struct nsdb *sm);

HEADER_FUNC struct string vname_or_default (const char *name) {
  if (name != NULL) {
    return strfcstr (name);
  } else {
    return strfcstr (DEFAULT_VARIABLE);
  }
}
