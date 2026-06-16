/*
 * repl.c — minimal self-contained REPL in ISO C (C99).
 *
 * Accumulates input across lines until a ';' is seen, then echoes
 * the complete statement. Shows a continuation prompt for
 * unfinished statements. No external dependencies, no memory leaks.
 *
 * Build:  cc -std=c99 -Wall -Wextra -pedantic -o repl repl.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alloc.h"
#include "collections.h" // dblb_buffer
#include "compiler.h"    // lexer
#include "error.h"       // error

#define REPL_PREFIX "numstore> "
#define CONT_PREFIX "      ... "
#define INITIAL_CAP 128

enum cmd_status
{
  CMD_SUCCESS,
  CMD_TERMINATE,
  CMD_FAILURE,
};

static enum cmd_status
handle_command (const char *command, error *e)
{
  struct chunk_alloc alloc;
  chunk_alloc_create_default (&alloc);
  struct query q;
  if (compile_query (&q, command, &alloc, e))
  {
    return CMD_FAILURE;
  }

  return CMD_SUCCESS;
}

/*
 * Append one line from stream to the buffer (newline not stored).
 * Returns 1 on success, 0 on EOF with nothing read, -1 on error.
 */
static int
append_line (struct dbl_buffer *b, FILE *stream, error *e)
{
  int    c;
  size_t before = b->nelem;

  while ((c = fgetc (stream)) != EOF && c != '\n')
  {
    char _c = (char)c;
    WRAP (dblb_append (b, &_c, 1, e));
  }

  if (c == EOF && b->nelem == before)
  {
    return 0;
  }

  return 1;
}

static int
has_terminator (const struct dbl_buffer *b)
{
  return memchr (b->data, ';', b->nelem) != NULL;
}

static int
is_blank (const struct dbl_buffer *b)
{
  for (size_t i = 0; i < b->nelem; i++)
  {
    if (((char *)b->data)[i] != ' ' && ((char *)b->data)[i] != '\t')
    {
      return 0;
    }
  }
  return 1;
}

int
main (void)
{
  error e = error_create ();

  while (true)
  {
    // Build the line as a double buffer
    struct dbl_buffer stmt;
    if (dblb_create (&stmt, 1, INITIAL_CAP, &e))
    {
      error_log_consume (&e);
      return EXIT_FAILURE;
    }

    // Print out the repl prefix
    fputs (REPL_PREFIX, stdout);
    fflush (stdout);

    // Accumulate lines until a ';' appears or EOF.
    while (true)
    {
      // Read a whole line
      int r = append_line (&stmt, stdin, &e);

      // handle error
      if (r < 0)
      {
        goto failure;
      }

      // nothing - got eof
      if (r == 0)
      {
        fputc ('\n', stdout);
        goto complete;
      }

      // Check if this line is the last one
      if (has_terminator (&stmt))
      {
        char c = '\0';
        if (dblb_append (&stmt, &c, 1, &e))
        {
          goto failure;
        }
        break;
      }

      if (stmt.nelem > 0)
      {
        /* Separate lines with a space so tokens don't merge. */
        char c = ' ';
        if (dblb_append (&stmt, &c, 1, &e))
        {
          goto failure;
        }
      }

      fputs (CONT_PREFIX, stdout);
      fflush (stdout);
    }

    if (is_blank (&stmt))
    {
      continue;
    }

    switch (handle_command (stmt.data, &e))
    {
      case CMD_SUCCESS:
      {
        continue;
      }
      case CMD_TERMINATE:
      {
        goto complete;
      }
      case CMD_FAILURE:
      {
        goto failure;
      }
    }

  failure:
    error_log_consume (&e);
    dblb_free (&stmt);
    return EXIT_FAILURE;

  complete:
    dblb_free (&stmt);
    return EXIT_SUCCESS;
  }
}
