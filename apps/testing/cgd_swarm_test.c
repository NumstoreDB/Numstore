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

#include "cgd_swarm_test.h"

#include <signal.h>
#include <stddef.h>
#include <stdlib.h>

static volatile sig_atomic_t keep_running = 1;

static void
handle_sigint (int sig)
{
  (void)sig;
  keep_running = 0;
}

int
main (void)
{
  int start_enabled[CDS_AT_LEN];
  for (int i = 0; i < CDS_AT_LEN; ++i)
  {
    start_enabled[i] = 1;
  }

  struct cgd_swarm_test *meta = cgd_swmt_open (start_enabled, "test");
  srand (100);

#if PLATFORM_WINDOWS
  // Windows / ISO C fallback
  if (signal (SIGINT, handle_sigint) == SIG_ERR)
  {
    cgd_swmt_close (meta);
    return 0;
  }
#else
  // POSIX robust signal handling (macOS and Linux)
  struct sigaction sa;
  sa.sa_handler = handle_sigint;
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = 0;

  if (sigaction (SIGINT, &sa, NULL) == -1)
  {
    cgd_swmt_close (meta);
    return 0;
  }
#endif

  while (keep_running)
  {
    cgd_swmt_step (meta);
  }

  cgd_swmt_close (meta);
  return 0;
}
