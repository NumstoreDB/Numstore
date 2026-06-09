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

#include "error.h"     // error
#include "pager.h"     // pager
#include "serial.h"    // string
#include "txn_table.h" // txn

/******************************************************************************
 * SECTION: NSHandle
 * ----------------------------------------------------------------------------
 * @brief A Handle of a numstore database that has static transaction context
 *
 * This is a wrapper around the individual numstore functions get create delete
 * insert read write with "state". You can start and stop transactions within
 * a state and split into new contexts for new threads
 ******************************************************************************/

struct nshandle_root
{
  struct pager *p;     // The database resources
  struct string path;  // Path to the database
  int           count; // When this reaches 0 - close the root
  error         e;
};

struct nshandle
{
  struct nshandle_root *root;
  int                   is_auto_txn; // If atx is an auto transaction
  struct txn           *atx;         // Active transaction
  struct txn            tx;          // Transaction storage
  error                 e;
};

// Testing

/*-----------------------------------------------------------------------------
 * SUBSECTION: Testing Utilities
 * @brief Utilities used mostly for testing and fault injection
 *----------------------------------------------------------------------------*/

struct nshandle *nsh_remove_and_open (const char *name, error *e);
int              nsh_crash (struct nshandle *ns);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Lifecycle
 * @brief Opening and closing a handle
 *----------------------------------------------------------------------------*/

struct nshandle *nsh_open (const char *path);
int              nsh_cleanup (const char *path);
struct nshandle *nsh_new_context (struct nshandle *ns);
int              nsh_close (struct nshandle *ns);

err_t            nsh_root_close (struct nshandle_root *root, error *e);
err_t            nsh_root_crash (struct nshandle_root *root, error *e);
struct nshandle *nsh_root_load (struct nshandle_root *root, error *e);
void nsh_root_release (struct nshandle_root *root, struct nshandle *sm);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Error reporting
 * @brief Monitoring the error of a nshandle state
 *----------------------------------------------------------------------------*/

const char *nsh_strerror (struct nshandle *ns);
int         nsh_perror (struct nshandle *ns, const char *prefix);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Transaction Control
 * @brief Utilities used mostly for testing and fault injection
 *----------------------------------------------------------------------------*/

int nsh_begin (struct nshandle *smf);
int nsh_commit (struct nshandle *smf);
int nsh_rollback (struct nshandle *smf);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Auto transactions
 * @brief When no explicit transaction is supplied - use an automatic
 * transaction
 *----------------------------------------------------------------------------*/

err_t nsh_auto_begin_txn (struct nshandle *sm, error *e);
err_t nsh_auto_commit (struct nshandle *sm, error *e);
void  nsh_auto_rollback (struct nshandle *sm);
#endif // NSHANDLE_H
