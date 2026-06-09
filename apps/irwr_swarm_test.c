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
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "testing/irwr_swarm_test_fixture.h"

#define DEFAULT_TIMEOUT_SECONDS 10

static volatile sig_atomic_t keep_running = 1;

static void
handle_sigint (int sig)
{
  (void)sig;
  keep_running = 0;
}

int
main (int argc, char *argv[])
{
  int timeout_seconds = DEFAULT_TIMEOUT_SECONDS;

  if (argc > 1)
  {
    timeout_seconds = atoi (argv[1]);
    if (timeout_seconds <= 0)
    {
      fprintf (
          stderr,
          "Invalid timeout '%s', using default %ds\n",
          argv[1],
          DEFAULT_TIMEOUT_SECONDS
      );
      timeout_seconds = DEFAULT_TIMEOUT_SECONDS;
    }
  }

  srand (10000);

  int start_enabled[IRWR_AT_LEN];
  for (int i = 0; i < IRWR_AT_LEN; ++i)
  {
    start_enabled[i] = 1;
  }

  struct irwr_swarm_test *meta = irwr_swmt_open (
      start_enabled,
      "test",
      100000,
      "testvar",
      "u32",
      sizeof (u32)
  );

#if PLATFORM_WINDOWS
  if (signal (SIGINT, handle_sigint) == SIG_ERR)
  {
    irwr_swmt_close (meta);
    return 0;
  }
#else
  struct sigaction sa;
  sa.sa_handler = handle_sigint;
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = 0;
  if (sigaction (SIGINT, &sa, NULL) == -1)
  {
    irwr_swmt_close (meta);
    return 0;
  }
#endif

  time_t start = time (NULL);

  while (keep_running)
  {
    if (time (NULL) - start >= timeout_seconds)
    {
      break;
    }
    irwr_swmt_step (meta);
  }

  irwr_swmt_close (meta);
  return 0;
}
