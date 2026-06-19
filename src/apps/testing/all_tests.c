#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "error.h"
#include "logging.h"
#include "os.h"
#include "testing_only/cgd_swarm_test_fixture.h"
#include "testing_only/irwr_swarm_test_fixture.h"
#include "testing_only/unit_tests.h"

#if PLATFORM_WINDOWS
#  include <windows.h>
#endif

static volatile sig_atomic_t keep_running = 1;

static void
handle_sigint (int sig)
{
  (void)sig;
  keep_running = 0;
}

static void
sleep_ms (int ms)
{
#if PLATFORM_WINDOWS
  Sleep ((DWORD)ms);
#else
  struct timespec ts;
  ts.tv_sec  = ms / 1000;
  ts.tv_nsec = (ms % 1000) * 1000000L;
  nanosleep (&ts, NULL);
#endif
}

static void *
unit_test_thread (void *_ctx)
{
  int *result = _ctx;
  *result     = run_unit_tests (NULL);
  return NULL;
}

static void *
irwr_test_thread (void *_ctx)
{
  (void)_ctx;
  int start_enabled[IRWR_AT_LEN];
  for (int i = 0; i < IRWR_AT_LEN; ++i)
  {
    start_enabled[i] = 1;
  }
  struct irwr_swarm_test *meta = irwr_swmt_open (
      start_enabled,
      "___irwr_test",
      100000,
      "testvar",
      "u32",
      sizeof (u32)
  );
  while (keep_running)
  {
    irwr_swmt_step (meta);
  }
  irwr_swmt_close (meta);
  return NULL;
}

static void *
cgd_test_thread (void *_ctx)
{
  (void)_ctx;
  int start_enabled[CDS_AT_LEN];
  for (int i = 0; i < CDS_AT_LEN; ++i)
  {
    start_enabled[i] = 1;
  }
  struct cgd_swarm_test *meta = cgd_swmt_open (start_enabled, "___cgd_test");
  while (keep_running)
  {
    cgd_swmt_step (meta);
  }
  cgd_swmt_close (meta);
  return NULL;
}

int
main (int argc, char *argv[])
{
  if (argc != 3)
  {
    fprintf (stderr, "usage: %s TIMEOUT SEED\n", argv[0]);
    return 2;
  }

  // Parse arguments
  int      timeout_seconds = atoi (argv[1]);
  unsigned seed            = (unsigned)strtoul (argv[2], NULL, 10);

  // Register signal handler
  signal (SIGINT, handle_sigint);

  // Register random seed
  srand (seed);
  fprintf (stderr, "running tests for %ds (seed=%u)\n", timeout_seconds, seed);

  int   unit_result = 0;
  error e           = error_create ();

  // Start test threads
  i_thread unit_thread, cgd_thread, irwr_thread;
  i_thread_create (&unit_thread, unit_test_thread, &unit_result, &e);
  i_thread_create (&cgd_thread, cgd_test_thread, NULL, &e);
  i_thread_create (&irwr_thread, irwr_test_thread, NULL, &e);

  // Loop until time is done
  time_t start = time (NULL);
  while (keep_running && time (NULL) - start < timeout_seconds)
  {
    sleep_ms (100);
  }

  // Notify threads and join
  keep_running = 0;
  i_thread_join (&cgd_thread, &e);
  i_thread_join (&irwr_thread, &e);
  i_thread_join (&unit_thread, &e);

  if (unit_result)
  {
    i_log_passed ("All Tests Passed\n");
  }
  else
  {
    i_log_failure ("Unit Tests Failed\n");
  }

  return unit_result;
}
