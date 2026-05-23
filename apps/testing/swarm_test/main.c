#include "swarm_test.h"
#include <signal.h>

static volatile sig_atomic_t keep_running = 1;

static void
handle_sigint (int sig)
{
  (void)sig; 
  keep_running = 0;
}

int
main(void)
{
  int start_enabled[AT_LEN];
  for (int i = 0; i < AT_LEN; ++i)
  {
    start_enabled[i] = 1;
  }

  struct swarm_test *meta = swmt_open (0.1f, 0.1f, start_enabled, "test", 10000);

  struct sigaction sa;
  sa.sa_handler = handle_sigint;
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = 0; 

  if (sigaction (SIGINT, &sa, NULL) == -1)
  {
    swmt_close (meta);
    return 0;
  }

  while (keep_running)
  {
    swmt_step (meta);
  }

  swmt_close (meta);
}
