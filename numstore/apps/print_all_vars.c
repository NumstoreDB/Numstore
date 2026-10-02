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
#include "core/ns_string.h"
#include "nscore/algorithms/var/ns_var_algorithms.h"
#include "nscore/variables/ns_variables.h"

#include <stdio.h>
#include <stdlib.h>

struct args
{
  const char *dbname;
  const char *filter; // NULL if not supplied
};

static void
usage (FILE *out, const char *prog)
{
  fprintf (out, "USAGE: %s DBNAME [FILTER]\n", prog);
}

static int
parse_args (struct args *dest, const int argc, char **argv)
{
  if (argc < 2 || argc > 3) {
    return -1;
  }

  dest->dbname = argv[1];
  dest->filter = argc == 3 ? argv[2] : NULL;

  if (dest->dbname[0] == '\0') {
    return -1;
  }

  return 0;
}

static err_t
print_variable (struct variable *var, void *ctx, error *e)
{
  struct args *args = ctx;

  if (args->filter == NULL || string_contains (var->vname, strfcstr (args->filter))) {
    if (i_print_variable (var, e)) {
      return error_trace (e);
    }
    printf (",");
  }

  return SUCCESS;
}

int
main (const int argc, char **argv)
{
  struct args args;

  if (parse_args (&args, argc, argv)) {
    usage (stderr, argc > 0 ? argv[0] : "print_all_vars");
    return EXIT_FAILURE;
  }

  error         e = error_create ();

  struct pager *p = pgr_open (args.dbname, default_mem (), default_filesystem (), &e);
  if (p == NULL) {
    error_log_consume (&e);
    return -1;
  }

  printf ("[\n");
  if (ns_visit_variables (p, print_variable, &args, &e) < 0) {
    return error_trace (&e);
  }
  printf ("null ]\n");

  return EXIT_SUCCESS;
}
