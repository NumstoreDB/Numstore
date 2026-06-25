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
 * @brief NSHandle is a stateful wrapper for numstore
 */

#ifndef NSHANDLE_H
#define NSHANDLE_H

#include "error.h"
#include "pager.h"
#include "query.h"
#include "txn_table.h"

/******************************************************************************
 * SECTION: NSHandle
 * ----------------------------------------------------------------------------
 * @brief A Handle of a numstore database that has static transaction context
 *
 * This is a wrapper around the individual numstore functions get create delete
 * insert read write with "state". You can start and stop transactions within
 * a state and split into new contexts for new threads
 ******************************************************************************/

struct nsdb_root
{
  struct pager *p;     // The database resources
  struct string path;  // Path to the database
  int           count; // When this reaches 0 - close the root
  error         e;
};

struct nsdb
{
  struct nsdb_root *root;
  int               is_auto_txn; // If atx is an auto transaction
  struct txn       *atx;         // Active transaction
  struct txn        tx;          // Transaction storage
  error             e;
};

// Testing

/*-----------------------------------------------------------------------------
 * SUBSECTION: Testing Utilities
 * @brief Utilities used mostly for testing and fault injection
 *----------------------------------------------------------------------------*/

struct nsdb *nsh_remove_and_open (const char *name, error *e);
int          nsh_crash (struct nsdb *ns);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Lifecycle
 * @brief Opening and closing a handle
 *----------------------------------------------------------------------------*/

struct nsdb *nsh_open (const char *path);
int          nsh_cleanup (const char *path);
struct nsdb *nsh_new_context (struct nsdb *ns);
int          nsh_close (struct nsdb *ns);

err_t        nsh_root_close (struct nsdb_root *root, error *e);
err_t        nsh_root_crash (struct nsdb_root *root, error *e);
struct nsdb *nsh_root_load (struct nsdb_root *root, error *e);
void         nsh_root_release (struct nsdb_root *root, struct nsdb *sm);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Error reporting
 * @brief Monitoring the error of a nsdb state
 *----------------------------------------------------------------------------*/

const char *nsh_strerror (struct nsdb *ns);
int         nsh_perror (struct nsdb *ns, const char *prefix);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Transaction Control
 * @brief Utilities used mostly for testing and fault injection
 *----------------------------------------------------------------------------*/

int nsh_begin (struct nsdb *smf);
int nsh_commit (struct nsdb *smf);
int nsh_rollback (struct nsdb *smf);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Auto transactions
 * @brief When no explicit transaction is supplied - use an automatic
 * transaction
 *----------------------------------------------------------------------------*/

err_t nsh_auto_begin_txn (struct nsdb *sm, error *e);
err_t nsh_auto_commit (struct nsdb *sm, error *e);
void  nsh_auto_rollback (struct nsdb *sm);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Execute Internal
 *----------------------------------------------------------------------------*/

sb_size nsdb_execute_on_buffer (
    struct nsdb        *ns,
    struct query       *q,
    void               *data,
    struct chunk_alloc *alc,
    error              *e
);

sb_size nsdb_execute_in_console (
    struct nsdb        *ns,
    struct query       *q,
    struct chunk_alloc *alc,
    error              *e
);

#endif // NSHANDLE_H
