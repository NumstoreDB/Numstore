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

#include "core/ns_error.h"
#include "core/os/ns_time.h"
#include "nscore/nsdb/ns_nsdb.h"
#include "numstore/testing/ns_numstore_simulation.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static _Atomic bool running = true;

void
exit_handler (int sig)
{
  (void)sig;
  running = false;
}

int
main (int argc, char **argv)
{
  error e = error_create ();

  if (argc != 6) {
    fprintf (stderr, "Usage: %s DB DURATION SEED COMMIT_HASH UUID\n", argv[0]);
    return EXIT_FAILURE;
  }

  // Parse arguments
  const char *dbname      = argv[1];
  int         duration    = atoi (argv[2]);
  u64         seed        = strtoul (argv[3], NULL, 10);
  const char *commit_hash = argv[4];
  u32         seqid       = strtoul (argv[5], NULL, 10);

  // TODO - validate arguments

  srand (seed);

  // Clean up any remnants of the database
  nsdb_cleanup (dbname);
  struct ns_simulation *simul = ns_simul_open (seed, commit_hash, seqid, dbname, 10000, 0);

  i_timer               timer;
  i_timer_create (&timer, &e);

  // Register SIGINT
  struct sigaction sa;
  memset (&sa, 0, sizeof (sa));
  sa.sa_handler = exit_handler;
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = 0;

  if (sigaction (SIGINT, &sa, NULL) == -1) {
    perror ("sigaction");
    return 1;
  }

  while (running) {
    struct ns_simul_record *record = ns_simul_prepare (simul);
    print_ns_simul_record (record);

    // Terminate early on error
    if (record->inner.record_type == RS_FAILURE) {
      return -1;
    }

    record = ns_simul_execute (simul);
    print_ns_simul_record (record);

    if (record->inner.record_type == RS_FAILURE) {
      return -1;
    }

    f64 now = i_timer_now_s (&timer);
    if (now > duration) {
      running = false;
    }
  }

  return EXIT_SUCCESS;
}
