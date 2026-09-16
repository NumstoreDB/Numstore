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

#define NSDB_AUTO_TXN(condition, txname) \
  struct txn *txname = NULL;             \
  do {                                   \
    if (condition) { txname = nsdb_begin(

#endif // NSHANDLE_H
