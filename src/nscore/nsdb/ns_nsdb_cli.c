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

#include "nscore/nsdb/ns_nsdb_cli.h"

#include "core/ns_alloc.h"
#include "core/ns_error.h"
#include "core/ns_logging.h"
#include "nscore/compiler/ns_compiler.h"
#include "nscore/nsdb/ns_nsdb.h"
#include "nscore/nsdb/ns_nsdb_execute.h"
#include "nscore/types/ns_query.h"
#include "numstore/numstore.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

err_t
nscli_init (struct nscli *cli, const char *dbname)
{
  cli->db = nsdb_open (dbname);

  if (cli->db == NULL) {
    return -1;
  }

  return SUCCESS;
}

err_t
nscli_step_init (struct nscli *cli)
{
  create_default_allocator (&cli->step_alloc);

  if (dblb_create (&cli->stmt, &cli->step_alloc, 1, 128, &cli->db->e)) {
    allocator_free (&cli->step_alloc);
    return error_trace (&cli->db->e);
  }

  return SUCCESS;
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

  while ((c = fgetc (stream)) != EOF && c != '\n') {
    char _c = (char)c;
    WRAP (dblb_append (b, &_c, 1, e));
  }

  if (c == EOF && b->nelem == before) {
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
  for (size_t i = 0; i < b->nelem; i++) {
    if (((char *)b->data)[i] != ' ' && ((char *)b->data)[i] != '\t') {
      return 0;
    }
  }
  return 1;
}

enum nscli_read_result
nscli_step_read_stdin (struct nscli *cli)
{
  // Print out the repl prefix
  fputs ("numstore> ", stdout);
  fflush (stdout);

  // Accumulate lines until a ';' appears or EOF.
  while (true) {
    // Read a whole line
    int r = append_line (&cli->stmt, stdin, &cli->db->e);

    // handle error
    if (r < 0) {
      return CMD_FATAL;
    }

    // nothing - got eof
    if (r == 0) {
      fputc ('\n', stdout);
      return CMD_NOTHING_TO_DO;
    }

    // Check if this line is the last one
    if (has_terminator (&cli->stmt)) {
      char c = '\0';
      if (dblb_append (&cli->stmt, &c, 1, &cli->db->e)) {
        return CMD_FATAL;
      }
      break;
    }

    if (cli->stmt.nelem > 0) {
      /* Separate lines with a space so tokens don't merge. */
      char c = ' ';
      if (dblb_append (&cli->stmt, &c, 1, &cli->db->e)) {
        return CMD_FATAL;
      }
    }

    fputs ("      ... ", stdout);
    fflush (stdout);
  }

  if (is_blank (&cli->stmt)) {
    return CMD_NOTHING_TO_DO;
  }

  return CMD_RUN;
}

enum nscli_execute_result
nscli_step_execute (struct nscli *cli)
{
  enum nscli_execute_result ret = EXE_SUCCESS;
  struct query              q;

  // compile the query
  if (compile_query (&q, cli->stmt.data, &cli->step_alloc, &cli->db->e)) {
    ret = EXE_ERROR;
    goto theend;
  }

  i_log_query (LOG_INFO, &q);

  // Exit
  if (q.type == QT_EXIT) {
    ret = EXE_EXIT;
    goto theend;
  }

  // Execute the query
  if (nsdb_execute_in_console (cli->db, &q, &cli->step_alloc) < 0) {
    ret = EXE_ERROR;
    goto theend;
  }

theend:
  return ret;
}

void
nscli_step_clean (struct nscli *cli)
{
  allocator_free (&cli->step_alloc);
  dblb_reset (&cli->stmt);
  cli->db->e.cause_code = SUCCESS;
  cli->db->e.cmlen      = 0;
}

void
nscli_close (struct nscli *cli)
{
  allocator_free (&cli->step_alloc);
  nsdb_close (cli->db);
}
