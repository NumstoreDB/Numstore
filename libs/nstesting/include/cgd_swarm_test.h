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

/**
 * Create Delete Swarm Test
 *
 * Swarm Test for the
 *    CREATE
 *    GET
 *    DELETE
 *
 * Operations for variables
 *
 * No data inserting in this test
 */

#include <c_specx.h>

#include "mem_vhmap.h"
#include "numstore.h"

// Actions you can take in a database
enum action_type
{
  // Other outside actions that can occur
  CDS_BEGIN_TXN,
  CDS_COMMIT_TXN,
  CDS_ROLLBACK_TXN,
  CDS_CRASH_AND_REOPEN,
  CDS_CLOSE_AND_REOPEN,

  // The main actions (subject to "swarm distribution"
  CDS_CREATE,
  CDS_SWITCH,
  CDS_DELETE,

  // Length of options
  CDS_AT_LEN,
};

// Opaque swarm test
struct cgd_swarm_test
{
  // Fake database transaction semantics
  struct mem_vhmap *committed;
  struct mem_vhmap *working;

  struct variable *cur;

  int enabled[CDS_AT_LEN];
  int allowed[CDS_AT_LEN];

  nsdb_t     *db;
  int         in_txn;
  const char *dbname;
};

// Main API
struct cgd_swarm_test *
cgd_swmt_open (int start_enabled[CDS_AT_LEN], const char *dbname);
void cgd_swmt_close (struct cgd_swarm_test *meta);
void cgd_swmt_step (struct cgd_swarm_test *meta);

// Concrete Actions
void cgd_swmt_begin_txn (struct cgd_swarm_test *meta);
void cgd_swmt_commit_txn (struct cgd_swarm_test *meta);
void cgd_swmt_rollback_txn (struct cgd_swarm_test *meta);
void cgd_swmt_crash_and_reopen (struct cgd_swarm_test *meta);
void cgd_swmt_close_and_reopen (struct cgd_swarm_test *meta);

void cgd_swmt_create (struct cgd_swarm_test *meta);
void cgd_swmt_switch (struct cgd_swarm_test *meta);
void cgd_swmt_delete (struct cgd_swarm_test *meta);
