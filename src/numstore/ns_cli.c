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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "numstore/numstore.h"

#ifdef TESTING
#  include "core/ns_logging.h"
#  include "core/testing/ns_unit_tests.h"
#  include "nscore/ns_nsdb.h"
#  include "numstore/testing/ns_swarm_tests.h"

struct test_args
{
  bool        unit;   // --unit_tests [FILTER]
  const char *filter; // optional

  bool        irwr; // --irwr DB DURATION SEED
  const char *irwr_db;
  int         irwr_duration;
  unsigned    irwr_seed;

  bool        cgd; // --cgd DB DURATION SEED
  const char *cgd_db;
  int         cgd_duration;
  unsigned    cgd_seed;
};

static void
test_usage (const char *prog)
{
  fprintf (
      stderr,
      "Usage: %s FILE\n"
      "       %s --unit_tests [FILTER]\n"
      "       %s --irwr DB DURATION SEED\n"
      "       %s --cgd DB DURATION SEED\n",
      prog,
      prog,
      prog,
      prog
  );
}

/* Parse the 3 operands shared by --irwr / --cgd. Advances *i past them.
 * Returns 0 on success, -1 on missing/invalid operands. */
static int
parse_swarm_args (int argc, char **argv, int *i, const char **db, int *duration, unsigned *seed)
{
  if (*i + 3 >= argc)
  {
    return -1;
  }

  *db = argv[++*i];

  char *end = NULL;
  long  dur = strtol (argv[++*i], &end, 10);
  if (*end != '\0' || dur <= 0)
  {
    return -1;
  }
  *duration = (int)dur;

  unsigned long sd = strtoul (argv[++*i], &end, 10);
  if (*end != '\0')
  {
    return -1;
  }
  *seed = (unsigned)sd;

  return 0;
}

/* Fills `out` from argv. Returns 0 on success, -1 on bad usage.
 * Flag order doesn't matter; unknown args are rejected here since
 * test flags and the normal FILE arg are mutually exclusive. */
static int
parse_test_args (int argc, char **argv, struct test_args *out)
{
  memset (out, 0, sizeof (*out));

  for (int i = 1; i < argc; i++)
  {
    if (strcmp (argv[i], "--unit_tests") == 0)
    {
      out->unit = true;
      /* optional FILTER: next arg, unless it's another flag */
      if (i + 1 < argc && strncmp (argv[i + 1], "--", 2) != 0)
      {
        out->filter = argv[++i];
      }
    }
    else if (strcmp (argv[i], "--irwr") == 0)
    {
      out->irwr = true;
      if (parse_swarm_args (argc, argv, &i, &out->irwr_db, &out->irwr_duration, &out->irwr_seed))
      {
        return -1;
      }
    }
    else if (strcmp (argv[i], "--cgd") == 0)
    {
      out->cgd = true;
      if (parse_swarm_args (argc, argv, &i, &out->cgd_db, &out->cgd_duration, &out->cgd_seed))
      {
        return -1;
      }
    }
    else if (out->unit || out->irwr || out->cgd)
    {
      /* Stray operand mixed in with test flags */
      return -1;
    }
    else
    {
      /* Not a test flag at all -> normal numstore invocation */
      return 0;
    }
  }
  return 0;
}

/* Runs whichever test modes were requested. Returns process exit code. */
static int
run_tests (const struct test_args *t)
{
  int ret = 0;

  if (t->unit)
  {
    ret = run_unit_tests (1234, t->filter);
    if (ret == 0)
    {
      i_log_passed ("All Tests Passed\n");
    }
    else
    {
      i_log_failure ("Unit Tests Failed\n");
    }
  }
  if (t->irwr)
  {
    irwr_swarm_test (t->irwr_db, t->irwr_duration, t->irwr_seed);
  }
  if (t->cgd)
  {
    cgd_swarm_test (t->cgd_db, t->cgd_duration, t->cgd_seed);
  }

  return ret;
}

#endif /* TESTING */

int
main (int argc, char **argv)
{
#ifdef TESTING
  struct test_args targs;
  if (parse_test_args (argc, argv, &targs))
  {
    test_usage (argv[0]);
    return EXIT_FAILURE;
  }
  if (targs.unit || targs.irwr || targs.cgd)
  {
    return run_tests (&targs);
  }
#endif

  if (argc != 2)
  {
    fprintf (stderr, "Usage: %s filename\n", argv[0]);
    return -1;
  }
  struct nscli cli;
  if (nscli_init (&cli, argv[1]))
  {
    return EXIT_FAILURE;
  }
  while (true)
  {
    // Initialize
    if (nscli_step_init (&cli))
    {
      goto fatal;
    }
    // Read input
    switch (nscli_step_read_stdin (&cli))
    {
      case CMD_FATAL:
      {
        nsdb_perror (cli.db, "Error: ");
        goto complete;
      }
      case CMD_NOTHING_TO_DO:
      {
        break;
      }
      case CMD_RUN:
      {
        switch (nscli_step_execute (&cli))
        {
          case EXE_ERROR:
          {
            nsdb_perror (cli.db, "Error: ");
            break;
          }
          case EXE_SUCCESS:
          {
            break;
          }
          case EXE_EXIT:
          {
            goto complete;
          }
        }
      }
    }
    nscli_step_clean (&cli);
  }
fatal:
  nscli_close (&cli);
  return EXIT_FAILURE;
complete:
  nscli_close (&cli);
  return EXIT_SUCCESS;
}
