/*
 * type_compile_cli.c
 *
 * Usage:
 *   ./resolve_type_ref "<type string>" "type ref"
 *
 * Example:
 *   ./resolve_type_ref a "struct { a u32, b f32 }" "struct { f a.a, g a }"
 */

#include "core/ns_alloc.h"
#include "core/ns_error.h"
#include "nscore/compiler/ns_compiler.h"
#include "nscore/types/ns_type_ref.h"
#include "nscore/types/ns_types.h"

#include <stdio.h>
#include <stdlib.h>

int
main (int argc, char **argv)
{
  if (argc != 3) {
    fprintf (stderr, "usage: %s \"<type string>\" \"<type ref string>\"\n", argv[0]);
    return EXIT_FAILURE;
  }

  const char *type_str     = argv[1];
  const char *type_ref_str = argv[2];

  ALLOC_INIT (alloc);

  struct type     q;
  struct type_ref ref;
  error           e  = error_create ();
  int             rc = EXIT_SUCCESS;

  if (compile_type (&q, type_str, &alloc, &e)) {
    error_log_consume (&e);
    rc = EXIT_FAILURE;
    goto theend;
  }

  if (compile_type_ref (&ref, type_ref_str, &alloc, &e)) {
    error_log_consume (&e);
    rc = EXIT_FAILURE;
    goto theend;
  }

  struct type *generated = tr_construct (&q, &ref, &alloc, &e);
  if (generated == NULL) {
    error_log_consume (&e);
    rc = EXIT_FAILURE;
    goto theend;
  }

  char *str = type_tostr (&alloc, generated, &e);
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
