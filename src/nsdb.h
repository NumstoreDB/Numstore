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

#include "error.h"
#include "pager.h"
#include "query.h"
#include "txn_table.h"
#include "variables.h"

/******************************************************************************
 * SECTION: nsdb
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

/*-----------------------------------------------------------------------------
 * SUBSECTION: Testing Utilities
 * @brief Utilities used mostly for testing and fault injection
 *----------------------------------------------------------------------------*/

int nsdb_crash (struct nsdb *ns);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Lifecycle
 * @brief Opening and closing a handle
 *----------------------------------------------------------------------------*/

struct nsdb *nsdb_open (const char *path);
int          nsdb_cleanup (const char *path);
int          nsdb_close (struct nsdb *ns);

err_t        nsdb_root_close (struct nsdb_root *root, error *e);
struct nsdb *nsdb_root_load (struct nsdb_root *root, error *e);
void         nsdb_root_release (struct nsdb_root *root, struct nsdb *sm);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Error reporting
 * @brief Monitoring the error of a nsdb state
 *----------------------------------------------------------------------------*/

const char *nsdb_strerror (struct nsdb *ns);
int         nsdb_perror (struct nsdb *ns, const char *prefix);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Transaction Control
 * @brief Utilities used mostly for testing and fault injection
 *----------------------------------------------------------------------------*/

int nsdb_begin (struct nsdb *smf);
int nsdb_commit (struct nsdb *smf);
int nsdb_rollback (struct nsdb *smf);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Auto transactions
 * @brief When no explicit transaction is supplied - use an automatic
 * transaction
 *----------------------------------------------------------------------------*/

err_t nsdb_auto_begin_txn (struct nsdb *sm, error *e);
err_t nsdb_auto_commit (struct nsdb *sm, error *e);
void  nsdb_auto_rollback (struct nsdb *sm);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Execute Internal
 *----------------------------------------------------------------------------*/

sb_size nsdb_execute_on_buffer (
    struct nsdb      *ns,
    struct query     *q,
    void             *data,
    struct allocator *alc
);

/******************************************************************************
 * SECTION: Query literal routines
 * ----------------------------------------------------------------------------
 * @brief Individual actions you can take on a numstore database
 ******************************************************************************/

int nsdb_create (
    struct nsdb      *db,
    struct allocator *alloc,
    struct string     vname,
    struct type       dtype
);
err_t nsdb_delete (struct nsdb *db, struct string vname);
err_t nsdb_get (
    struct nsdb      *db,
    struct get_query *query,
    struct allocator *alloc,
    struct variable **dest
);
err_t nsdb_get_and_print (
    struct nsdb      *db,
    struct get_query *query,
    struct allocator *alloc
);
sb_size nsdb_insert (
    struct nsdb         *db,
    struct insert_query *query,
    struct allocator    *alloc,
    struct stream       *src
);
sb_size nsdb_read (
    struct nsdb       *db,
    struct read_query *query,
    struct allocator  *alloc,
    struct stream     *dest
);
sb_size nsdb_read_and_print (
    struct nsdb       *db,
    struct read_query *query,
    struct allocator  *alloc
);
sb_size nsdb_write (
    struct nsdb        *db,
    struct write_query *query,
    struct allocator   *alloc,
    struct stream      *src
);
sb_size nsdb_remove (
    struct nsdb         *db,
    struct remove_query *query,
    struct allocator    *alloc,
    struct stream       *dest
);

/******************************************************************************
 * SECTION: NSDB CLI
 * ----------------------------------------------------------------------------
 * @brief A stateful cli tool for nsdb
 ******************************************************************************/

struct nscli
{
  struct nsdb      *db;         // The Database
  struct dbl_buffer stmt;       // Statement
  struct allocator  step_alloc; // Allocator for anything per step
};

err_t nscli_init (struct nscli *cli, const char *dbname);
err_t nscli_step_init (struct nscli *cli);

enum nscli_read_result
{
  CMD_NOTHING_TO_DO,
  CMD_FATAL,
  CMD_RUN,
} nscli_step_read_stdin (struct nscli *cli);

enum nscli_execute_result
{
  EXE_SUCCESS,
  EXE_ERROR,
  EXE_EXIT,
} nscli_step_execute (struct nscli *cli);

void nscli_step_clean (struct nscli *cli);
void nscli_close (struct nscli *cli);

#endif // NSHANDLE_H
