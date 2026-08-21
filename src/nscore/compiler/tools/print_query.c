/*
 * query_compile_cli.c
 *
 * Usage:
 *   ./query_compile_cli "<query string>"
 *
 * Compiles the given query string and prints the compiled query to stdout.
 *
 * Assumptions (per the caller's instructions):
 *   - compile_query(struct query *out, const char *src, allocator *alloc,
 *                    error *e) returns 0 (SUCCESS) on success and a
 *                    non-zero value on failure, matching the pattern used
 *                    in nscli_step_execute():
 *                        if (compile_query(&q, ...)) { -> error }
 *   - i_print_query(struct query *q) prints a compiled query to stdout
 *     (the print-oriented counterpart to i_log_query()).
 *   - ALLOC_INIT(name) / ALLOC_CLOSE(name) declare and tear down a local
 *     allocator variable called `name`, as used in the test helper.
 *   - error_create() returns an initialized `error` value that can be
 *     passed by address into compile_query().
 */

#include "core/ns_alloc.h"
#include "core/ns_logging.h"
#include "nscore/compiler/ns_compiler.h"
#include "nscore/types/ns_query.h"

#include <stdio.h>
#include <stdlib.h>

int
main (int argc, char **argv)
{
  if (argc != 2) {
    fprintf (stderr, "usage: %s \"<query string>\"\n", argv[0]);
    return EXIT_FAILURE;
  }

  const char *query_str = argv[1];

  ALLOC_INIT (alloc);

  struct query q;
  error        e  = error_create ();
  int          rc = EXIT_SUCCESS;

  if (compile_query (&q, query_str, &alloc, &e)) {
    fprintf (stderr, "error: failed to compile query: %s\n", query_str);
    rc = EXIT_FAILURE;
    goto theend;
  }

  i_log_query (LOG_INFO, &q);

theend:
  ALLOC_CLOSE (alloc);
  return rc;
}
