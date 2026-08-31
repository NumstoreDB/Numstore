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

enum ns_action_type
{
  NSS_BEGIN_TXN,
  NSS_COMMIT_TXN,
  NSS_ROLLBACK_TXN,

  NSS_CRASH_AND_REOPEN,
  NSS_CLOSE_AND_REOPEN,

  NSS_CREATE, // Create a new variable (don't swap unless it's the first one)
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
    u64         seed,
    const char *commit_hash,
    u64         sequence_id,
    const char *dbname,           // Name of the database
    int         max_insert_len,   // Maximum elements to insert
    float       sample_space_prob // Probability swap sample space
);
int ns_simul_close (struct ns_simulation *meta);

struct ns_simul_record
{
  uint64_t    seed;        // Random seed
  const char *commit_hash; // Commit Hash
  uint64_t    sequence_id; // I forget

  // Run metrics
  uint64_t    step_number;   // Which step is the test in
  uint64_t    clock;         // Total milliseconds since the start
  uint64_t    working_clock; // Duration of just the below operations

  // Database metrics
  uint64_t    db_total_size; // Total size in bytes of the physical database
  uint32_t    nvars;         // Number of variables
  uint32_t    tracked_bytes; // Number of bytes in all variables

  struct
  {
    enum ns_result_record_type
    {
      RS_PRE, // Everything except time is filled out
      RS_SUCCESS,
      RS_FAILURE,
    } record_type;

    // Operation metrics
    struct
    {
      enum ns_action_type op_type;

      union {
        struct create_op
        {
          char        *vname;
          struct type *type;
          char        *typestr;
        } create;

        struct insert_op
        {
          t_size tsize;  // Size of the type being inserted
          b_size ofst;   // Offset (elements)
          b_size nelems; // Number of elements inserted
          b_size len;    // Length of this array
        } insert;

        struct read_op
        {
          t_size tsize;  // Size of the type being inserted
          b_size ofst;   // Start offset (elements)
          b_size stride; // Stride
          b_size nelems; // Number of elements
          b_size len;    // Length of this array
        } read;

        struct remove_op
        {
          t_size tsize;  // Size of the type being inserted
          b_size ofst;   // Start offset (elements)
          b_size stride; // Stride
          b_size nelems; // Number of elements
          b_size len;    // Length of this array
        } remove;

        struct write_op
        {
          t_size tsize;  // Size of the type being inserted
          b_size ofst;   // Start offset (elements)
          b_size stride; // Stride
          b_size nelems; // Number of elements
          b_size len;    // Length of this array
        } write;

        struct delete_op
        {
          t_size tsize; // Size of the type being deleted
          b_size len;   // Length of this array
        } delete;
      };
    } operation;

    uint64_t op_duration_ms;
  } inner;
};

struct ns_simul_record *ns_simul_prepare (struct ns_simulation *meta);
struct ns_simul_record *ns_simul_execute (struct ns_simulation *meta);

#endif // NSS_SWARM_TEST_FIXTURE_H
