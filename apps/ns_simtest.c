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

#include "ns_error.h"
#include "ns_nsdb.h"
#include "ns_platform.h"
#include "os/ns_malloc.h"
#include "os/ns_os.h"
#include "tests/simulation/ns_numstore_simulation.h"
#include "tests/simulation/ns_operation_generator.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Defaults
#define DEFAULT_DBNAME      "ns_simtest"
#define DEFAULT_DURATION    50
#define DEFAULT_SEED        1231241123ULL
#define DEFAULT_COMMIT_HASH "foo"
#define DEFAULT_SEQID       10
#define DEFAULT_FAIL_PCT    0.0F
#define DEFAULT_THREADS     2
#define SIM_MAX_THREADS     64

static _Atomic bool running = true;

static void
exit_handler (int sig)
{
  (void)sig;
  running = false;
}

static void
print_usage (FILE *out, const char *prog)
{
  fprintf (
      out,
      "Usage: %s [options]\n"
      "\n"
      "Options:\n"
      "  --dbname NAME        database name                 (default: %s)\n"
      "  --duration SECONDS   run time in seconds, > 0      (default: %d)\n"
      "  --seed N             RNG seed                      (default: %llu)\n"
      "  --commit-hash HASH   commit hash to record         (default: %s)\n"
      "  --seqid N            sequence id                   (default: %d)\n"
      "  --fail-percent P     fault injection rate in [0,1]  (default: %.4f)\n"
      "  --threads N          threads, 1 to %d              (default: %d)\n"
      "  --disable ACTION     disable an action; repeatable\n"
      "  -h, --help           show this help\n"
      "\n"
      "Values can be given as --flag VALUE or --flag=VALUE.\n"
      "\n"
      "The system under test runs on a faulty OS and a faulty allocator that\n"
      "wrap the real ones: every fallible call fails with probability\n"
      "--fail-percent. At 0 they are pure pass-through. Metrics and the\n"
      "reference model always run on the real OS and allocator.\n"
      "\n"
      "With --threads above 1, every thread runs a simulation of its own,\n"
      "all at the same time on one shared database. Each works on its own\n"
      "variables and checks them against its own reference model. Two things\n"
      "differ from a single threaded run: the crash and close actions are\n"
      "off (they would take the database away from the other threads), and\n"
      "a seed does not replay, because the threads interleave differently\n"
      "on every run.\n"
      "\n"
      "Actions (all enabled by default; NSS_ prefix and case optional):\n",
      prog,
      DEFAULT_DBNAME,
      DEFAULT_DURATION,
      DEFAULT_SEED,
      DEFAULT_COMMIT_HASH,
      DEFAULT_SEQID,
      (double)DEFAULT_FAIL_PCT,
      SIM_MAX_THREADS,
      DEFAULT_THREADS
  );

  for (int a = 0; a < NSS_AT_LEN; ++a) {
    fprintf (out, "  NSS_%s\n", action_names[a] != NULL ? action_names[a] : "?");
  }
}

static int
usage_error (const char *prog, const char *fmt, ...)
{
  va_list ap;
  fprintf (stderr, "%s: ", prog);
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fprintf (stderr, "\nTry '%s --help' for usage.\n", prog);
  return EXIT_FAILURE;
}

/// Matches "--name VALUE" or "--name=VALUE" at argv[*i].
/// Returns false if argv[*i] is a different argument. On a match, *value is
/// the value (NULL if missing) and *i is advanced past a separate value.
static bool
take_flag (const char *name, int argc, char **argv, int *i, const char **value)
{
  const char *arg = argv[*i];
  size_t      len = strnlen (name, 64);

  if (strncmp (arg, name, len) != 0) {
    return false;
  }

  if (arg[len] == '=') {
    *value = arg + len + 1;
  } else if (arg[len] != '\0') {
    return false; // e.g. --seedling is not --seed
  } else if (*i + 1 < argc) {
    *value = argv[++*i];
  } else {
    *value = NULL;
  }

  return true;
}

struct sim_thread
{
  struct ns_simulation *simul;
  i_timer               timer;
  u64                   duration;
  int                   rc;
};

// Steps one simulation until the time is up, something stops the run, or a
// step fails. A failed step stops every thread
static void *
sim_thread_run (void *arg)
{
  struct sim_thread *t = arg;
  error              e = error_create ();

  while (running) {
    if (ns_simul_step (t->simul, &e) < 0) {
      error_log_consume (&e);
      t->rc   = EXIT_FAILURE;
      running = false;
      break;
    }

    const u64 elapsed_ns = t->timer.table->timer_now_ns (t->timer.self);
    if ((f64)elapsed_ns / 1e9 > (f64)t->duration) {
      running = false;
    }
  }

  // An open transaction holds locks that the other threads wait on. End it
  // here, or a thread that waits never sees that the run is over
  if (ns_simul_end_txn (t->simul, &e) < 0) {
    error_log_consume (&e);
    t->rc = EXIT_FAILURE;
  }

  return NULL;
}

/// Strict unsigned parse: whole string must be a number in [0, max]
static bool
parse_u64 (const char *s, u64 max, u64 *out)
{
  char              *end;
  unsigned long long v;

  if (*s == '\0' || *s == '-' || *s == '+') {
    return false;
  }

  errno = 0;
  v     = strtoull (s, &end, 10);
  if (errno != 0 || *end != '\0' || v > max) {
    return false;
  }

  *out = (u64)v;
  return true;
}

/// Strict parse of a probability: whole string must be a number in [0, 1]
static bool
parse_prob (const char *s, float *out)
{
  char  *end;
  double v;

  if (*s == '\0') {
    return false;
  }

  errno = 0;
  v     = strtod (s, &end);
  if (errno != 0 || *end != '\0' || !(v >= 0.0 && v <= 1.0)) {
    return false;
  }

  *out = (float)v;
  return true;
}

/// ASCII case-insensitive compare of up to n chars
static bool
ieqn (const char *a, const char *b, size_t n)
{
  for (size_t k = 0; k < n; ++k) {
    if (toupper ((unsigned char)a[k]) != toupper ((unsigned char)b[k])) {
      return false;
    }
    if (a[k] == '\0') {
      return true;
    }
  }
  return true;
}

/// "NSS_BEGIN_TXN", "BEGIN_TXN", "begin_txn" -> NSS_BEGIN_TXN
static bool
parse_action (const char *s, enum ns_action_type *out)
{
  if (ieqn (s, "NSS_", 4)) {
    s += 4;
  }

  for (int a = 0; a < NSS_AT_LEN; ++a) {
    if (action_names[a] != NULL && ieqn (s, action_names[a], SIZE_MAX)) {
      *out = (enum ns_action_type)a;
      return true;
    }
  }
  return false;
}

int
main (int argc, char **argv)
{
  error                       e           = error_create ();
  struct ns_simulation_params params      = {0};

  const char                 *dbname      = DEFAULT_DBNAME;
  u64                         duration    = DEFAULT_DURATION;
  u64                         seed        = DEFAULT_SEED;
  const char                 *commit_hash = DEFAULT_COMMIT_HASH;
  u64                         seqid       = DEFAULT_SEQID;
  float                       fail_pct    = DEFAULT_FAIL_PCT;
  u64                         nthreads    = DEFAULT_THREADS;

  // All actions enabled by default
  memset (params.enabled, 1, sizeof (params.enabled));

  for (int i = 1; i < argc; ++i) {
    const char *arg = argv[i];
    const char *v   = NULL;

    if (strcmp (arg, "-h") == 0 || strcmp (arg, "--help") == 0) {
      print_usage (stdout, argv[0]);
      return EXIT_SUCCESS;
    } else if (take_flag ("--dbname", argc, argv, &i, &v)) {
      if (v == NULL || *v == '\0') {
        return usage_error (argv[0], "%s requires a value", arg);
      }
      dbname = v;
    } else if (take_flag ("--duration", argc, argv, &i, &v)) {
      if (v == NULL || *v == '\0') {
        return usage_error (argv[0], "%s requires a value", arg);
      }
      if (!parse_u64 (v, INT_MAX, &duration) || duration == 0) {
        return usage_error (argv[0], "invalid duration '%s' (must be > 0)", v);
      }
    } else if (take_flag ("--seed", argc, argv, &i, &v)) {
      if (v == NULL || *v == '\0') {
        return usage_error (argv[0], "%s requires a value", arg);
      }
      if (!parse_u64 (v, UINT64_MAX, &seed)) {
        return usage_error (argv[0], "invalid seed '%s'", v);
      }
    } else if (take_flag ("--commit-hash", argc, argv, &i, &v)) {
      if (v == NULL || *v == '\0') {
        return usage_error (argv[0], "%s requires a value", arg);
      }
      commit_hash = v;
    } else if (take_flag ("--seqid", argc, argv, &i, &v)) {
      if (v == NULL || *v == '\0') {
        return usage_error (argv[0], "%s requires a value", arg);
      }
      if (!parse_u64 (v, UINT32_MAX, &seqid)) {
        return usage_error (argv[0], "invalid seqid '%s'", v);
      }
    } else if (take_flag ("--fail-percent", argc, argv, &i, &v)) {
      if (v == NULL || *v == '\0') {
        return usage_error (argv[0], "%s requires a value", arg);
      }
      if (!parse_prob (v, &fail_pct)) {
        return usage_error (argv[0], "invalid fail percent '%s' (must be in [0, 1])", v);
      }
    } else if (take_flag ("--threads", argc, argv, &i, &v)) {
      if (v == NULL || *v == '\0') {
        return usage_error (argv[0], "%s requires a value", arg);
      }
      if (!parse_u64 (v, SIM_MAX_THREADS, &nthreads) || nthreads == 0) {
        return usage_error (argv[0], "invalid threads '%s' (must be 1 to %d)", v, SIM_MAX_THREADS);
      }
    } else if (take_flag ("--disable", argc, argv, &i, &v)) {
      enum ns_action_type a;

      if (v == NULL || *v == '\0') {
        return usage_error (argv[0], "%s requires a value", arg);
      }
      if (!parse_action (v, &a)) {
        return usage_error (argv[0], "unknown action '%s' (see --help)", v);
      }

      params.enabled[a] = 0;
    } else {
      return usage_error (argv[0], "unknown argument '%s'", arg);
    }
  }

  // At least one action must remain
  bool any_enabled = false;
  for (int a = 0; a < NSS_AT_LEN; ++a) {
    any_enabled = any_enabled || params.enabled[a] != 0;
  }

  if (!any_enabled) {
    return usage_error (argv[0], "all actions are disabled");
  }

  // Fault injection is deterministic: randf() is backed by libc rand(), which
  // srand(seed) below pins, so one (seed, fail-percent) pair replays exactly.
  srand ((unsigned)seed);

  // The reliable layer - metrics, the reference model, and the faulty
  // decorators' own bookkeeping all run on this
  const struct i_mem reliable_mem = default_mem ();

  struct i_os        reliable_os;
  if (system_os_create (reliable_mem, &reliable_os, &e) < 0) {
    error_log_consume (&e);
    return EXIT_FAILURE;
  }

  // The layer under test - wraps the reliable one and injects faults
  struct i_os test_os;
  if (faulty_os_create (reliable_mem, reliable_os, fail_pct, &test_os, &e) < 0) {
    error_log_consume (&e);
    system_os_free (reliable_os);
    return EXIT_FAILURE;
  }

  struct i_mem test_mem;
  if (faulty_mem_create (reliable_mem, fail_pct, &test_mem, &e) < 0) {
    error_log_consume (&e);
    faulty_os_free (test_os);
    system_os_free (reliable_os);
    return EXIT_FAILURE;
  }

  params.seed              = seed;
  params.commit_hash       = commit_hash;
  params.sequence_id       = (u32)seqid; // range-checked above
  params.dbname            = dbname;
  params.max_insert_len    = 1000000;
  params.max_tsize         = 4096;
  params.sample_space_prob = 0;
  params.reliable_mem      = reliable_mem;
  params.reliable_os       = reliable_os;
  params.test_mem          = test_mem;
  params.test_os           = test_os;

  i_timer timer;
  if (reliable_os.table->timer_create (reliable_os.self, &timer, &e) < 0) {
    error_log_consume (&e);
    faulty_mem_free (test_mem);
    faulty_os_free (test_os);
    system_os_free (reliable_os);
    return EXIT_FAILURE;
  }

  // Register SIGINT.
#if PLATFORM_WINDOWS
  if (signal (SIGINT, exit_handler) == SIG_ERR) {
    perror ("signal");
    goto fail;
  }
#else
  struct sigaction sa;
  memset (&sa, 0, sizeof (sa));
  sa.sa_handler = exit_handler;
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = 0;

  if (sigaction (SIGINT, &sa, NULL) == -1) {
    perror ("sigaction");
    goto fail;
  }
#endif

  int               rc     = EXIT_SUCCESS;
  struct nsdb      *shared = NULL;
  struct sim_thread threads[SIM_MAX_THREADS];
  i_thread          handles[SIM_MAX_THREADS];
  u64               nopen    = 0;
  u64               nstarted = 0;

  // One thread keeps the database to itself: it can crash and reopen it, and
  // a seed replays. Several threads share one database
  if (nthreads > 1) {
    shared = ns_simul_shared_db_open (params, &e);
    if (shared == NULL) {
      error_log_consume (&e);
      goto fail;
    }
    params.shared_db = shared;
  }

  for (u64 t = 0; t < nthreads; ++t) {
    params.thread_id = (u32)t;
    threads[t]       = (struct sim_thread){
        .simul    = ns_simul_open (params, &e),
        .timer    = timer,
        .duration = duration,
        .rc       = EXIT_SUCCESS,
    };
    if (threads[t].simul == NULL) {
      error_log_consume (&e);
      rc = EXIT_FAILURE;
      break;
    }
    nopen++;
  }

  if (rc == EXIT_SUCCESS && nthreads == 1) {
    // On this thread, as it always was
    sim_thread_run (&threads[0]);
  } else if (rc == EXIT_SUCCESS) {
    // All at once
    for (u64 t = 0; t < nthreads; ++t) {
      if (reliable_os.table
              ->thread_create (reliable_os.self, &handles[t], sim_thread_run, &threads[t], &e)
          < 0) {
        error_log_consume (&e);
        rc      = EXIT_FAILURE;
        running = false;
        break;
      }
      nstarted++;
    }
    for (u64 t = 0; t < nstarted; ++t) {
      reliable_os.table->thread_join (reliable_os.self, handles[t].self);
    }
  }

  for (u64 t = 0; t < nopen; ++t) {
    if (threads[t].rc != EXIT_SUCCESS) {
      rc = EXIT_FAILURE;
    }
    if (ns_simul_close (threads[t].simul, &e) < 0) {
      error_log_consume (&e);
      rc = EXIT_FAILURE;
    }
  }

  if (shared != NULL && ns_simul_shared_db_close (shared, &e) < 0) {
    error_log_consume (&e);
    rc = EXIT_FAILURE;
  }

  reliable_os.table->timer_free (reliable_os.self, timer.self);
  faulty_mem_free (test_mem);
  faulty_os_free (test_os);
  system_os_free (reliable_os);

  return rc;

fail:
  reliable_os.table->timer_free (reliable_os.self, timer.self);
  faulty_mem_free (test_mem);
  faulty_os_free (test_os);
  system_os_free (reliable_os);

  return EXIT_FAILURE;
}
