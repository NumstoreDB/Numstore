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

#include "nscore/testing/simulation/ns_operation_generator.h"

#ifndef NDEBUG
#  include "core/ns_testing.h"
#  include "nscore/testing/simulation/ns_numstore_simulation.h"
#endif

#ifndef NDEBUG

/**
 * This test is meant to mimic the crash and reopen bug
 * and should be made more reliable once root cause is found
 *
 * This is not a finalized test
 */
TEST (0005_crash_after_churn_diverges)
{
  struct ns_simulation_params params = {
      .seed              = 20261027,
      .commit_hash       = "regression-0004",
      .sequence_id       = 20261027,
      .dbname            = "test",
      .max_insert_len    = 100000,
      .max_tsize         = 4096,
      .sample_space_prob = 0,
      .test_os           = ctx->os,
      .test_mem          = ctx->mem,
      .reliable_mem      = ctx->mem,
      .reliable_os       = ctx->os,
  };
  memset (params.enabled, 1, sizeof (params.enabled));

  struct ns_simulation *s = ns_simul_open (params, &ctx->e);

  ns_simul_step_with (s, opg_rand_create (simtorandopp (s), &ctx->e), &ctx->e);
  ns_simul_step_with (s, opg_rand_begin (simtorandopp (s), &ctx->e), &ctx->e);
  ns_simul_step_with (s, opg_rand_insert (simtorandopp (s), &ctx->e), &ctx->e);
  ns_simul_step_with (s, opg_rand_remove (simtorandopp (s), &ctx->e), &ctx->e);
  ns_simul_step_with (s, opg_rand_delete (simtorandopp (s), &ctx->e), &ctx->e);
  ns_simul_step_with (s, opg_rand_crash_and_reopen (simtorandopp (s), &ctx->e), &ctx->e);

  // Crash again
  //
  // This failed -
  //      CAUSE:
  //          The recovery phase read logs then appended to the end of a torn
  //          log record rather than start appending at the last known valid
  //          log record
  ns_simul_step_with (s, opg_rand_crash_and_reopen (simtorandopp (s), &ctx->e), &ctx->e);

  ns_simul_close (s, &ctx->e);
}

#endif
