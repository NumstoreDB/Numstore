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

#ifndef NSS_SWARM_TEST_FIXTURE_H
#define NSS_SWARM_TEST_FIXTURE_H

#include "core/ns_stdtypes.h"
#include "core/os/ns_memory.h"
#include "numstore/testing/ns_operation_generator.h"

struct ns_simulation_params
{
  u64          seed;
  const char  *commit_hash;
  u8           enabled[NSS_AT_LEN];
  u64          sequence_id;
  const char  *dbname;
  int          max_insert_len;
  float        sample_space_prob;
  struct i_mem reliable_mem;
  struct i_mem test_mem;
};

// Open a new irwr simulation
struct ns_simulation *ns_simul_open (struct ns_simulation_params params, error *e);

err_t ns_simul_close (struct ns_simulation *meta, error *e);

/*
 * Pick a random allowed action, decide its parameters, execute it against
 * both the real db and the reference model, and verify the result.
 *
 * Returns 0 on success, -1 on failure.
 *
 * once a step returns -1, the caller must not call ns_simul_step
 * again - only ns_simul_close.
 *
 * A failure here means an invalid database state - e.g. a bug
 */
err_t ns_simul_step (struct ns_simulation *meta, error *e);

#endif // NSS_SWARM_TEST_FIXTURE_H
