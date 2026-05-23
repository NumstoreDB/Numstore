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

#include "c_specx.h"
#include "register_test_suite.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLI_MAX_FILTERS 32
#define CLI_MAX_SUITES  16
#define streq(a, b)     (strcmp (a, b) == 0)

////////////////////////////////////////////////////////////
/// SUITE STRUCT

struct suite
{
  const char *name;
  test       *tests;
  u32         len;
};

////////////////////////////////////////////////////////////
/// CLI

struct test_cli
{
  char *filters[CLI_MAX_FILTERS];
  int   flen;
  char *suites[CLI_MAX_SUITES];
  int   slen;
  bool  help_printed;
};

static void
test_print_help (const char *prog)
{
  fprintf (stderr, "Usage: %s [TYPE] [--suite NAME]... [filter...]\n", prog);
  fprintf (stderr, "\nSuites:\n");
  fprintf (stderr, "  --suite NAME     Run only tests in NAME (repeatable)\n");
  fprintf (stderr, "  Available:       core, intf, smartfiles, paging\n");
  fprintf (stderr, "\nFilters:\n");
  fprintf (stderr, "  [filter...]      Run tests whose names contain any filter\n");
  fprintf (stderr, "  If omitted, all tests of the selected type/suite run.\n");
  fprintf (stderr, "\nFlags:\n");
  fprintf (stderr, "  --help, -h       Show this message\n");
  fprintf (stderr, "\nExamples:\n");
  fprintf (stderr, "  %s\n", prog);
  fprintf (stderr, "  %s HEAVY\n", prog);
  fprintf (stderr, "  %s --suite core --suite intf\n", prog);
  fprintf (stderr, "  %s HEAVY --suite paging wal\n", prog);
}

static int
test_parse_cli_params (char **argv, const int argc, struct test_cli *p)
{
  p->flen         = 0;
  p->slen         = 0;
  p->help_printed = false;

  for (int i = 1; i < argc; i++)
  {
    char *arg = argv[i];

    if (streq (arg, "--help") || streq (arg, "-h"))
    {
      test_print_help (argv[0]);
      p->help_printed = true;
      return 0;
    }

    if (streq (arg, "--suite"))
    {
      if (i + 1 >= argc)
      {
        fprintf (stderr, "Error: --suite requires a value\n");
        test_print_help (argv[0]);
        return -1;
      }
      if (p->slen >= CLI_MAX_SUITES)
      {
        fprintf (stderr, "Error: too many --suite args (max %d)\n", CLI_MAX_SUITES);
        return -1;
      }
      p->suites[p->slen++] = argv[++i];
      continue;
    }

    bool matched = false;

    if (!matched)
    {
      if (p->flen >= CLI_MAX_FILTERS)
      {
        fprintf (stderr, "Error: too many filters (max %d)\n", CLI_MAX_FILTERS);
        return -1;
      }
      p->filters[p->flen++] = arg;
    }
  }

  return 0;
}

////////////////////////////////////////////////////////////
/// RUNNER

static bool
should_run (const test *t, const char *suite_name, const struct test_cli *p)
{
  if (p->slen > 0)
  {
    bool matched = false;
    for (int i = 0; i < p->slen; i++)
    {
      if (streq (p->suites[i], suite_name))
      {
        matched = true;
        break;
      }
    }
    if (!matched) { return false; }
  }

  if (p->flen == 0) { return true; }

  const struct string tn = strfcstr (t->test_name);
  for (int i = 0; i < p->flen; i++)
  {
    const struct string f = strfcstr (p->filters[i]);
    if (string_contains (tn, f)) { return true; }
  }

  return false;
}

////////////////////////////////////////////////////////////
/// MAIN

int
main (const int argc, char **argv)
{
  struct test_cli p;
  if (test_parse_cli_params (argv, argc, &p)) { return -1; }

  if (p.help_printed) { return 0; }

  register_tests();

  struct suite all_suites[] = {
      {"smartfiles", smartfiles_tests, (u32)smartfiles_count},
  };

  error e = error_create ();

  i_timer timer;
  if (i_timer_create (&timer, &e) != SUCCESS) { return -1; }

  struct dbl_buffer f;
  if (dblb_create (&f, sizeof (char *), 1, &e)) { return -1; }

  for (u32 s = 0; s < arrlen (all_suites); s++)
  {
    const struct suite *suite = &all_suites[s];
    for (u32 i = 0; i < suite->len; i++)
    {
      test *t = &suite->tests[i];
      if (!should_run (t, suite->name, &p)) { continue; }

      if (!t->test ())
      {
        char **n = &t->test_name;
        if (dblb_append (&f, n, 1, &e))
        {
          dblb_free (&f);
          return -1;
        }
      }
    }
  }

  printf ("Time: %llu ms\n", (unsigned long long)i_timer_now_ms (&timer));
  i_timer_free (&timer);

  char **fl = f.data;
  if (f.nelem > 0)
  {
    i_log_failure ("FAILED TESTS:\n");
    for (u32 i = 0; i < f.nelem; i++) { i_log_failure ("  %s\n", fl[i]); }
  }
  else
  {
    i_log_passed ("ALL TESTS PASSED\n");
  }

  dblb_free (&f);
  return test_ret;
}
