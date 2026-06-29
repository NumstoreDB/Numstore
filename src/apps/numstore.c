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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nsdb.h"

int
main (int argc, char **argv)
{
  if (argc != 2)
  {
    fprintf (stderr, "Usage: %s filename", argv[0]);
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
