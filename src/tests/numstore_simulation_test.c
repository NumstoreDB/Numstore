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

#include "nscore/nsdb/ns_nsdb.h"
#include "numstore/testing/ns_numstore_simulation.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main (int argc, char **argv)
{
  if (argc != 6) {
    fprintf (stderr, "Usage: %s DB DURATION SEED COMMIT_HASH UUID\n", argv[0]);
    return EXIT_FAILURE;
  }

  const char *db          = argv[1];
  int         duration    = atoi (argv[2]);
  unsigned    seed        = (unsigned)strtoul (argv[3], NULL, 10);
  const char *commit_hash = argv[4];
  unsigned    sequence_id = (unsigned)strtoul (argv[5], NULL, 10);

  nsdb_cleanup (db);
  struct ns_simulation   *simul = ns_simul_open (seed, commit_hash, sequence_id, db, 10000, 0);

  struct ns_simul_record *record;

  while (true) {
    record = ns_simul_prepare (simul);
    print_ns_simul_record (record);
    if (record->inner.record_type == RS_FAILURE) {
      return -1;
    }
    record = ns_simul_execute (simul);
    print_ns_simul_record (record);
    if (record->inner.record_type == RS_FAILURE) {
      return -1;
    }
  }

  return EXIT_SUCCESS;
}
