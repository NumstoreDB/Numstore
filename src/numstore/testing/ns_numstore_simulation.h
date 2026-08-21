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

/******************************************************************************
 * SECTION: NSS Internals
 ******************************************************************************/

// Actions you can take in a irwr only database
enum ns_action_type
{
  NSS_BEGIN_TXN,
  NSS_COMMIT_TXN,
  NSS_ROLLBACK_TXN,

  NSS_CRASH_AND_REOPEN,
  NSS_CLOSE_AND_REOPEN,

  NSS_CREATE, // Create a new variable (don't swap)
  NSS_SWITCH, // Swap to an existing variable
  NSS_DELETE, // Delete current variable and swap to a different one

  NSS_INSERT, // Insert data into this one
  NSS_REMOVE, // remove data from this one
  NSS_READ,   // Read data from this one
  NSS_WRITE,  // Write data to this one

  NSS_AT_LEN,
};

// Open a new irwr simulation
struct ns_simulation *ns_simul_open (
    int         initial_enabled[NSS_AT_LEN], // Starting enabled sample space
    const char *dbname,                      // Name of the database
    int         max_insert_len,              // Maximum elements to insert
    float       sample_space_prob            // Probability swap sample space
);
int ns_simul_close (struct ns_simulation *meta);
int ns_simul_step (struct ns_simulation *meta);

#endif // NSS_SWARM_TEST_FIXTURE_H
