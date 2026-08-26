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
#include "nscore/pager/ns_pager.h"

struct nsdb
{
  error              e;
  struct pager      *p;
  struct string      path;
  struct slab_alloc *txn_alloc;
  latch              l;
};

struct nsdb *nsdb_open (const char *path);
int nsdb_cleanup (const char *path);
int nsdb_close (struct nsdb *ns);
int nsdb_crash (struct nsdb *ns);

// Error reporting
const char *nsdb_strerror (struct nsdb *ns);
int nsdb_perror (struct nsdb *ns, const char *prefix);

// Transaction Control
int nsdb_begin (struct nsdb *smf);
int nsdb_commit (struct nsdb *smf);
int nsdb_rollback (struct nsdb *smf);

// Auto Transaction
err_t nsdb_auto_begin_txn (struct nsdb *sm, error *e);
err_t nsdb_auto_commit (struct nsdb *sm, error *e);
void nsdb_auto_rollback (struct nsdb *sm);

#endif // NSHANDLE_H
