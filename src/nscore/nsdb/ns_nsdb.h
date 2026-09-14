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
  error                e;
  struct slab_alloc    txn_alloc;
  latch                l;
  struct i_mem         mem;
  struct i_file_system fs;
  struct string        path;
  struct pager        *p;
};

struct nsdb *nsdb_open_with_resources (
    const char          *path,
    struct i_mem         mem,
    struct i_file_system fs
    // TODO - add error *e here
);
int nsdb_cleanup (const char *path /* TODO add error *e here */);
int nsdb_close (struct nsdb *ns /* TODO add error *e here */);
int nsdb_crash (struct nsdb *ns /* TODO add error *e here */);

// Error reporting
const char *nsdb_strerror (struct nsdb *ns /* TODO add error *e here */);
int nsdb_perror (struct nsdb *ns, const char *prefix /* TODO add error *e here */);

// Transaction Control
struct ns_txn *nsdb_begin (struct nsdb *smf /* TODO add error *e here */);
int nsdb_commit (struct nsdb *smf, struct ns_txn *txn /* TODO add error *e here */);
int nsdb_rollback (struct nsdb *smf, struct ns_txn *txn /* TODO add error *e here */);

// Create a variable
int nsdb_create (
    struct nsdb        *db,
    struct ns_txn      *tx,
    struct arena_alloc *alloc,
    struct string       vname,
    struct type         dtype
    /* TODO - add error* e here */
);

// Delete a variable
err_t nsdb_delete (struct nsdb *db, struct ns_txn *tx, struct delete_query *query);

// Get a variable
err_t nsdb_get (
    struct nsdb        *db,
    struct ns_txn      *tx,
    struct get_query   *query,
    struct arena_alloc *alloc,
    struct variable   **dest
    /* TODO - add error* e here */
);

// Insert
sb_size nsdb_insert (
    struct nsdb         *db,
    struct ns_txn       *tx,
    struct insert_query *query,
    struct arena_alloc  *alloc,
    struct stream       *src
    /* TODO - add error* e here */
);

// Read
sb_size nsdb_read (
    struct nsdb        *db,
    struct ns_txn      *tx,
    struct read_query  *query,
    struct arena_alloc *alloc,
    struct stream      *dest
    /* TODO - add error* e here */
);

// Write
sb_size nsdb_write (
    struct nsdb        *db,
    struct ns_txn      *tx,
    struct write_query *query,
    struct arena_alloc *alloc,
    struct stream      *src
    /* TODO - add error* e here */
);

// Remove
sb_size nsdb_remove (
    struct nsdb         *db,
    struct ns_txn       *tx,
    struct remove_query *query,
    struct arena_alloc  *alloc,
    struct stream       *dest
    /* TODO - add error* e here */
);

#define AUTO_BEGIN(db, tx)    \
  bool auto_txn = false;      \
  do {                        \
    if ((tx) == NULL) {       \
      (tx) = nsdb_begin (db); \
      if ((tx) == NULL) {     \
        goto failed;          \
      }                       \
      auto_txn = true;        \
    }                         \
  }                           \
  while (0)

#define AUTO_COMMIT(db, tx)       \
  do {                            \
    if (auto_txn) {               \
      if (nsdb_commit (db, tx)) { \
        goto failed;              \
      }                           \
    }                             \
  }                               \
  while (0)

#define ROLLBACK_PRESERVING_ERROR(db, tx) \
  do {                                    \
    if (auto_txn) {                       \
      error _saved_e = (db)->e;           \
      nsdb_rollback ((db), (tx));         \
      (db)->e = _saved_e;                 \
    }                                     \
  }                                       \
  while (0)

#endif // NSHANDLE_H
