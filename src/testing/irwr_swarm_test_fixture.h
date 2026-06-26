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

#ifndef IRWR_SWARM_TEST_FIXTURE_H
#define IRWR_SWARM_TEST_FIXTURE_H

#include "collections.h" // block_array
#include "numstore.h"    // nsdb_t
#include "stdtypes.h"    // u32 ...etc

// Actions you can take in a irwr only database
enum irwr_action_type
{
  IRWR_BEGIN_TXN,
  IRWR_COMMIT_TXN,
  IRWR_ROLLBACK_TXN,
  IRWR_CRASH_AND_REOPEN,
  IRWR_CLOSE_AND_REOPEN,

  IRWR_INSERT,
  IRWR_REMOVE,
  IRWR_READ,
  IRWR_WRITE,

  IRWR_AT_LEN,
};

struct irwr_swarm_test
{
  // Fake database transaction semantics
  struct block_array *committed;
  struct block_array *working;

  int enabled[IRWR_AT_LEN];
  int allowed[IRWR_AT_LEN];

  nsdb_t     *db;
  int         in_txn;
  const char *dbname;
  const char *varname;
  const char *vartype;
  u32         esize;
  int         max_insert_len;
  b_size      len;
  float       sample_space_prob;
};

struct irwr_swarm_test *irwr_swmt_open (
    int         initial_enabled[IRWR_AT_LEN], // Starting enabled sample space
    const char *dbname,                       // Name of the database
    int         max_insert_len,               // Maximum elements to insert
    const char *varname,                      // Variable name
    const char *vartype,                      // Variable type
    float       sample_space_prob             // Probability swap sample space
);
void irwr_swmt_close (struct irwr_swarm_test *meta);
void irwr_swmt_step (struct irwr_swarm_test *meta);

#endif // IRWR_SWARM_TEST_FIXTURE_H
