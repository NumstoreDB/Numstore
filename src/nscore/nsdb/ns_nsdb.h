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

/**
 * @file
 * @brief Internals of numstore user exposed type
 */

#ifndef NSHANDLE_H
#define NSHANDLE_H

#include "core/ns_error.h"
#include "core/ns_string.h"
#include "core/os/ns_filesystem.h"
#include "core/os/ns_memory.h"
#include "nscore/pager/ns_pager.h"

struct nsdb
{
  struct slab_alloc    txn_alloc;
  latch                l;
  struct i_mem         mem;
  struct i_file_system fs;
  struct string        path;
  struct pager        *p;
};

struct numstore
{
  struct nsdb *db;
  error        e;
};

struct nsdb *nsdb_open_with_resources (
    const char          *path,
    struct i_mem         mem,
    struct i_file_system fs,
    error               *e
);
int nsdb_cleanup (const char *path, error *e);
int nsdb_close (struct nsdb *ns, error *e);
int nsdb_crash (struct nsdb *ns, error *e);

struct ns_txn *nsdb_begin (struct nsdb *smf, error *e);
int nsdb_commit (struct nsdb *smf, struct ns_txn *txn, error *e);
int nsdb_rollback (struct nsdb *smf, struct ns_txn *txn, error *e);

struct auto_txn
{
  struct ns_txn *tx;
  bool           is_auto_txn;
};

static inline err_t
nsdb_auto_begin (struct nsdb *db, struct ns_txn *tx, struct auto_txn *auto_tx, error *e)
{
  auto_tx->tx          = tx;
  auto_tx->is_auto_txn = false;

  if (tx == NULL) {
    auto_tx->tx = nsdb_begin (db, e);
    if (auto_tx->tx == NULL) {
      return error_trace (e);
    }
    auto_tx->is_auto_txn = true;
  }

  return SUCCESS;
}

static inline err_t
nsdb_auto_commit (struct nsdb *db, struct auto_txn *auto_tx, error *e)
{
  ASSERT (auto_tx->tx);
  if (auto_tx->is_auto_txn) {
    struct ns_txn *tx = auto_tx->tx;
    auto_tx->tx       = NULL;
    return nsdb_commit (db, tx, e);
  }
  return SUCCESS;
}

static inline err_t
nsdb_auto_rollback (struct nsdb *db, struct auto_txn *auto_tx, error *e)
{
  ASSERT (auto_tx->tx);
  if (auto_tx->is_auto_txn) {
    struct ns_txn *tx = auto_tx->tx;
    auto_tx->tx       = NULL;
    return nsdb_rollback (db, tx, e);
  }
  return SUCCESS;
}

#endif // NSHANDLE_H
