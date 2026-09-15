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
#include "nscore/types/ns_query.h"
#include "nscore/variables/ns_variables.h"

struct nsdb
{
  struct slab_alloc    txn_alloc;
  latch                l;
  struct i_mem         mem;
  struct i_file_system fs;
  struct string        path;
  struct pager        *p;
};

struct nsdb *ns_nsdb_open_with_resources (
    const char          *path,
    struct i_mem         mem,
    struct i_file_system fs,
    error               *e
);
int ns_nsdb_cleanup (const char *path, error *e);
int ns_nsdb_close (struct nsdb *ns, error *e);
int ns_nsdb_crash (struct nsdb *ns, error *e);

struct ns_txn *ns_nsdb_begin (struct nsdb *smf, error *e);
int ns_nsdb_commit (struct nsdb *smf, struct ns_txn *txn, error *e);
int ns_nsdb_rollback (struct nsdb *smf, struct ns_txn *txn, error *e);

// Create a variable
int ns_nsdb_create (
    struct nsdb         *db,
    struct ns_txn       *tx,
    struct create_query *query,
    struct arena_alloc  *alloc,
    error               *e
);

// Delete a variable
err_t ns_nsdb_delete (struct nsdb *db, struct ns_txn *tx, struct delete_query *query, error *e);

// Get a variable
err_t ns_nsdb_get (
    struct nsdb        *db,
    struct ns_txn      *tx,
    struct get_query   *query,
    struct arena_alloc *alloc,
    struct variable   **dest,
    error              *e
);

// Insert
sb_size ns_nsdb_insert (
    struct nsdb         *db,
    struct ns_txn       *tx,
    struct insert_query *query,
    struct arena_alloc  *alloc,
    struct stream       *src,
    error               *e
);

// Read
sb_size ns_nsdb_read (
    struct nsdb        *db,
    struct ns_txn      *tx,
    struct read_query  *query,
    struct arena_alloc *alloc,
    struct stream      *dest,
    error              *e
);

// Write
sb_size ns_nsdb_write (
    struct nsdb        *db,
    struct ns_txn      *tx,
    struct write_query *query,
    struct arena_alloc *alloc,
    struct stream      *src,
    error              *e
);

// Remove
sb_size ns_nsdb_remove (
    struct nsdb         *db,
    struct ns_txn       *tx,
    struct remove_query *query,
    struct arena_alloc  *alloc,
    struct stream       *dest,
    error               *e
);

#define AUTO_BEGIN(db, tx)          \
  bool auto_txn = false;            \
  do {                              \
    if ((tx) == NULL) {             \
      (tx) = ns_nsdb_begin (db, e); \
      if ((tx) == NULL) {           \
        goto failed;                \
      }                             \
      auto_txn = true;              \
    }                               \
  }                                 \
  while (0)

#define AUTO_COMMIT(db, tx)             \
  do {                                  \
    if (auto_txn) {                     \
      if (ns_nsdb_commit (db, tx, e)) { \
        goto failed;                    \
      }                                 \
    }                                   \
  }                                     \
  while (0)

#define ROLLBACK_PRESERVING_ERROR(db, tx) \
  do {                                    \
    if (auto_txn) {                       \
      error _saved_e = *(e);              \
      ns_nsdb_rollback ((db), (tx), e);   \
      *(e) = _saved_e;                    \
    }                                     \
  }                                       \
  while (0)

#endif // NSHANDLE_H
