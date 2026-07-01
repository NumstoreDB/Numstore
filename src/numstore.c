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

#include "numstore.h"

#include "alloc.h"
#include "compiler.h"
#include "csx_assert.h"
#include "error.h"
#include "nsdb.h"
#include "query.h"

#ifdef TESTING
#  include "testing/testing.h"
#endif

/******************************************************************************
 * SECTION: Library exposed nsdb_execute
 ******************************************************************************/

sb_size
nsdb_execute (nsdb_t *nh, const char *query, void *data, ...)
{
  ALLOC_INIT (alloc);
  sb_size      ret; // return variable
  char        *buf; // The formatted query buffer
  va_list      ap, ap2;
  i32          qlen; // Length of the query
  struct query q;    // The AST

  // Reset errors before proceeding
  nh->e.cause_code = 0;
  nh->e.cmlen      = 0;

  // First pass: compute the length the formatted query needs,
  // without writing anything.
  va_start (ap, data);
  va_copy (ap2, ap);
  qlen = vsnprintf (NULL, 0, query, ap);
  va_end (ap);
  if (qlen < 0)
  {
    va_end (ap2);
    ret =
        error_causef (&nh->e, ERR_INVALID_ARGUMENT, "Invalid printf argument");
    goto theend;
  }

  buf = allocate (&alloc, (size_t)qlen + 1, 1, &nh->e);
  if (!buf)
  {
    va_end (ap2);
    ret = error_trace (&nh->e);
    goto theend;
  }

  // Second pass: actually write the formatted query into buf.
  qlen = vsnprintf (buf, (size_t)qlen + 1, query, ap2);
  ASSERT (qlen >= 0);
  va_end (ap2);

  // Compile the query
  if (compile_query (&q, buf, &alloc, &nh->e))
  {
    ret = error_trace (&nh->e);
    goto theend;
  }
  ret = nsdb_execute_on_buffer (nh, &q, data, &alloc);
theend:
  ALLOC_CLOSE (alloc);
  return ret;
}

#ifdef TESTING
TEST (nsdb_execute)
{
  ALLOC_INIT (alloc);
  error e = error_create ();

  nsdb_cleanup ("test");
  struct nsdb     *db  = nsdb_open ("test");
  struct nsdb_var *var = NULL;

  nsdb_execute (db, "get %s", &var, "a");
  test_assert_equal (var, NULL);

  nsdb_execute (db, "create %s %s", NULL, "a", "u32");
  nsdb_execute (db, "get %s", &var, "a");
  test_assert (var != NULL);

  test_assert (string_equal (var->var->vname, strfcstr ("a")));
  test_assert (
      type_equal (var->var->dtype, compile_type_alloc ("u32", &alloc, &e))
  );
  test_assert_equal (nsdb_var_len (var), 0);
  nsdb_var_free (var);

  nsdb_close (db);
  ALLOC_CLOSE (alloc);
}
#endif

#ifndef NUMSTORE_LIB
int
main (int argc, char **argv)
{
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
#endif
