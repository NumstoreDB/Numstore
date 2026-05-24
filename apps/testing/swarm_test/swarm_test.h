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

#include <c_specx.h>
#include "numstore.h"

// Actions you can take in a database
enum action_type
{
  BEGIN_TXN,
  COMMIT_TXN,
  ROLLBACK_TXN,
  CRASH_AND_REOPEN,
  CLOSE_AND_REOPEN,
  INSERT,
  REMOVE,
  READ,
  WRITE,
  CREATE,
  SWITCH,
  DELETE,

  AT_LEN,
};

const char *action_type_tostr (enum action_type type);

// Opaque swarm test
struct swarm_test;

// Main API
struct swarm_test *swmt_open (
    float       probability_of_swarm_change,
    float       probability_of_full_read,
    int         start_enabled[AT_LEN],
    const char *dbname,
    int         max_insert_len
);
void swmt_close (struct swarm_test *meta);
void swmt_step (struct swarm_test *meta);

// Concrete Actions
void swmt_begin_txn (struct swarm_test *meta);
void swmt_commit_txn (struct swarm_test *meta);
void swmt_rollback_txn (struct swarm_test *meta);
void swmt_crash_and_reopen (struct swarm_test *meta);
void swmt_close_and_reopen (struct swarm_test *meta);
void swmt_insert (struct swarm_test *meta);
void swmt_remove (struct swarm_test *meta);
void swmt_read (struct swarm_test *meta);
void swmt_write (struct swarm_test *meta);
void swmt_create (struct swarm_test *meta);
void swmt_switch (struct swarm_test *meta);
void swmt_delete (struct swarm_test *meta);
