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
#include "core/os/ns_filesystem.h"
#include "core/os/ns_time.h"
#include "nscore/simtest/ns_numstore_simulation.h"

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
  error       e           = error_create ();

  /**
  if (argc != 6) {
    fprintf (stderr, "Usage: %s DB DURATION SEED COMMIT_HASH UUID\n", argv[0]);
    return EXIT_FAILURE;
  }
  */

  // Parse arguments
  const char *dbname      = "foo"; // = argv[1];
  int         duration    = 50;    // = atoi (argv[2]);
  u64         seed        = 512;   // = strtoul (argv[3], NULL, 10);
  const char *commit_hash = "foo"; // = argv[4];
  u32         seqid       = 10;    // = strtoul (argv[5], NULL, 10);

  // TODO - validate arguments

  srand (seed);

  // Clean up any remnants of the database
  struct ns_simulation_params params = {
      .seed              = seed,
      .commit_hash       = commit_hash,
      // .enabled[NSS_AT_LEN] = 0,
      .sequence_id       = seqid,
      .dbname            = dbname,
      .max_insert_len    = 1000000,
      .max_tsize         = 4096,
      .sample_space_prob = 0,
      .reliable_mem      = default_mem (),
      .test_mem          = default_mem (),
      .test_filesystem   = default_filesystem (),
      .write_validation  = NSS_READ_ALL_AFTER_WRITES,
  };
  memset (&params.enabled, 1, sizeof (params.enabled));
  struct ns_simulation *simul = ns_simul_open (params, &e);

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
    if (ns_simul_step (simul, &e) < 0) {
      error_log_consume (&e);
      return -1;
    }

    f64 now = i_timer_now_s (&timer);
    if (now > duration) {
      running = false;
    }
  }

  ns_simul_close (simul, &e);

  return EXIT_SUCCESS;
}
