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
#include "nscore/types/ns_variables.h"

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

struct nsdb *nsdb_open_with_resources (const char *path, struct i_mem mem, struct i_file_system fs);
int nsdb_cleanup (const char *path);
int nsdb_close (struct nsdb *ns);
int nsdb_crash (struct nsdb *ns);

// Error reporting
const char *nsdb_strerror (struct nsdb *ns);
int nsdb_perror (struct nsdb *ns, const char *prefix);

// Transaction Control
struct ns_txn *nsdb_begin (struct nsdb *smf);
int nsdb_commit (struct nsdb *smf, struct ns_txn *txn);
int nsdb_rollback (struct nsdb *smf, struct ns_txn *txn);

// Create a variable
int nsdb_create (
    struct nsdb      *db,
    struct ns_txn    *tx,
    struct allocator *alloc,
    struct string     vname,
    struct type       dtype
);

// Delete a variable
err_t nsdb_delete (struct nsdb *db, struct ns_txn *tx, struct delete_query *query);

// Get a variable
err_t nsdb_get (
    struct nsdb      *db,
    struct ns_txn    *tx,
    struct get_query *query,
    struct allocator *alloc,
    struct variable **dest
);

// Insert
sb_size nsdb_insert (
    struct nsdb         *db,
    struct ns_txn       *tx,
    struct insert_query *query,
    struct allocator    *alloc,
    struct stream       *src
);

// Read
sb_size nsdb_read (
    struct nsdb       *db,
    struct ns_txn     *tx,
    struct read_query *query,
    struct allocator  *alloc,
    struct stream     *dest
);

// Write
sb_size nsdb_write (
    struct nsdb        *db,
    struct ns_txn      *tx,
    struct write_query *query,
    struct allocator   *alloc,
    struct stream      *src
);

// Remove
sb_size nsdb_remove (
    struct nsdb         *db,
    struct ns_txn       *tx,
    struct remove_query *query,
    struct allocator    *alloc,
    struct stream       *dest
);

#define AUTO_BEGIN(db, tx)  \
  bool auto_txn = false;    \
  do {                      \
    if (tx == NULL) {       \
      tx = nsdb_begin (db); \
      if (tx == NULL) {     \
        goto failed;        \
      }                     \
      auto_txn = true;      \
    }                       \
  }                         \
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

#endif // NSHANDLE_H
