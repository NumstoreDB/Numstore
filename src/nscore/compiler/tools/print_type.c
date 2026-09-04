/*
 * type_compile_cli.c
 *
 * Usage:
 *   ./type_compile_cli "<type string>"
 *
 * Compiles the given type string and prints the compiled type to stdout.
 */

#include "core/ns_alloc.h"
#include "nscore/compiler/ns_compiler.h"
#include "nscore/types/ns_types.h"

#include <stdio.h>
#include <stdlib.h>

int
main (int argc, char **argv)
{
  if (argc != 2) {
    fprintf (stderr, "usage: %s \"<type string>\"\n", argv[0]);
    return EXIT_FAILURE;
  }

  const char *type_str = argv[1];

  ALLOC_INIT (alloc);

  struct type q;
  error       e  = error_create ();
  int         rc = EXIT_SUCCESS;

  if (compile_type (&q, type_str, &alloc, &e)) {
    fprintf (stderr, "error: failed to compile type: %s\n", type_str);
    rc = EXIT_FAILURE;
    goto theend;
  }

  char *str = type_tostr (&alloc, &q, &e);

  if (str == NULL) {
    fprintf (stderr, "error: failed to allocate type string: %s\n", type_str);
    rc = EXIT_FAILURE;
    goto theend;
  }

  fprintf (stdout, "%s\n", str);

  free (str);

theend:
  ALLOC_CLOSE (alloc);
  return rc;
}
