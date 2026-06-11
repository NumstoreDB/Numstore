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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

#include "compiler.h"
#include "error.h"
#include "linenoise.h"

////////////////////////////////////////////////////////////
// Completion + Hints

static const char *commands[] = {
    "exit",
    "create",
    "delete",
    "read",
    "remove",
    "insert",
    "write",
    NULL,
};

// Per-command argument templates
typedef struct
{
  const char *cmd;
  const char *args[8]; // hint for each successive arg slot
} cmd_spec;

static const cmd_spec specs[] = {
    {"exit", {NULL}},
    {"create", {"<VNAME>", "<TYPE>", NULL}},
    {"delete", {"<VNAME>", NULL}},
    {"read", {"<VNAME>[RANGE]", "to", "<file>", NULL}},
    {"remove", {"<VNAME>[RANGE]", "to", "<file>", NULL}},
    {"insert", {"<VNAME>", "<OFFSET>", "from", "<file>", NULL}},
    {"write", {"<VNAME>[RANGE]", "from", "<file|(read ...)>", NULL}},
    {NULL, {NULL}},
};

static const cmd_spec *
find_spec (const char *cmd, size_t len)
{
  for (int i = 0; specs[i].cmd != NULL; ++i)
  {
    if (strncmp (specs[i].cmd, cmd, len) == 0 && specs[i].cmd[len] == '\0')
    {
      return &specs[i];
    }
  }
  return NULL;
}

static void
completion (const char *buf, linenoiseCompletions *lc)
{
  // Skip leading whitespace
  while (*buf == ' ' || *buf == '\t')
  {
    buf++;
  }

  const char *sp = strchr (buf, ' ');

  if (sp == NULL)
  {
    // Still typing the command word — complete it
    for (int i = 0; commands[i] != NULL; ++i)
    {
      if (strncmp (buf, commands[i], strlen (buf)) == 0)
      {
        linenoiseAddCompletion (lc, commands[i]);
      }
    }
    return;
  }

  // Command word is done — find its spec
  size_t          cmdlen = (size_t)(sp - buf);
  const cmd_spec *spec   = find_spec (buf, cmdlen);
  if (spec == NULL)
  {
    return;
  }

  // Count how many args are already typed
  // Walk forward from after the command word
  const char *p     = sp;
  int         slots = 0;
  int         trail = 0;

  while (*p)
  {
    while (*p == ' ' || *p == '\t')
    {
      p++;
      trail = 1;
    }
    if (*p == '\0')
    {
      break;
    }
    trail = 0;
    // Walk to end of this token
    while (*p && *p != ' ' && *p != '\t')
    {
      p++;
    }
    slots++;
  }

  // If the last character was a space, cursor is on a fresh slot
  // and `slots` already reflects completed args.
  // If not, cursor is mid-token on slot index (slots - 1) —
  // complete the current token from the spec.

  if (trail)
  {
    // Cursor is on a new empty slot — nothing to complete yet,
    // hint will cover it
    return;
  }

  // Cursor is mid-token on slot (slots - 1)
  int cur_slot = slots - 1;
  if (cur_slot < 0)
  {
    return;
  }

  const char *expected = spec->args[cur_slot];
  if (expected == NULL)
  {
    return;
  }

  // Find where the current token started
  const char *tok_start = buf + strlen (buf);
  while (tok_start > buf && *(tok_start - 1) != ' ')
  {
    tok_start--;
  }

  // Only offer completion if what's typed is a prefix of the expected token
  size_t tok_len = strlen (tok_start);
  if (strncmp (tok_start, expected, tok_len) == 0
      && tok_len < strlen (expected))
  {
    // Build the full completed line
    char completed[4096];
    int  prefix_len = (int)(tok_start - buf);
    snprintf (
        completed,
        sizeof (completed),
        "%.*s%s",
        prefix_len,
        buf,
        expected
    );
    linenoiseAddCompletion (lc, completed);
  }
}

static char *
hints (const char *buf, int *color, int *bold)
{
  *color = 90;
  *bold  = 0;

  while (*buf == ' ' || *buf == '\t')
  {
    buf++;
  }
  if (*buf == '\0')
  {
    return NULL;
  }

  const char *sp = strchr (buf, ' ');

  if (sp == NULL)
  {
    // Partial command word — hint the rest of the command name
    for (int i = 0; commands[i] != NULL; ++i)
    {
      size_t clen = strlen (commands[i]);
      size_t blen = strlen (buf);
      if (blen < clen && strncmp (buf, commands[i], blen) == 0)
      {
        return (char *)(commands[i] + blen);
      }
    }
    return NULL;
  }

  size_t          cmdlen = (size_t)(sp - buf);
  const cmd_spec *spec   = find_spec (buf, cmdlen);
  if (spec == NULL)
  {
    return NULL;
  }

  // Walk tokens after command to find current slot
  const char *p     = sp;
  int         slots = 0;
  int         trail = 0;

  while (*p)
  {
    while (*p == ' ' || *p == '\t')
    {
      p++;
      trail = 1;
    }
    if (*p == '\0')
    {
      break;
    }
    trail = 0;
    while (*p && *p != ' ' && *p != '\t')
    {
      p++;
    }
    slots++;
  }

  // Which slot is the cursor on?
  // mid-token → current slot is (slots - 1), hint = rest of expected after
  // typed prefix trailing space → next empty slot is (slots), hint = full
  // expected token

  if (trail)
  {
    // Cursor on fresh slot
    const char *h = spec->args[slots];
    if (h == NULL)
    {
      return NULL;
    }

    // Build " <arg> <arg> ..." for remaining slots
    static char remaining[256];
    remaining[0] = '\0';
    for (int i = slots; spec->args[i] != NULL; ++i)
    {
      strncat (remaining, " ", sizeof (remaining) - strlen (remaining) - 1);
      strncat (
          remaining,
          spec->args[i],
          sizeof (remaining) - strlen (remaining) - 1
      );
    }
    return remaining[0] ? remaining : NULL;
  }
  else
  {
    // Mid-token on slot (slots - 1)
    int cur_slot = slots - 1;
    if (cur_slot < 0)
    {
      return NULL;
    }

    const char *expected = spec->args[cur_slot];
    if (expected == NULL)
    {
      return NULL;
    }

    // Find current token start
    const char *tok_start = buf + strlen (buf);
    while (tok_start > buf && *(tok_start - 1) != ' ')
    {
      tok_start--;
    }

    size_t tok_len = strlen (tok_start);
    if (tok_len < strlen (expected)
        && strncmp (tok_start, expected, tok_len) == 0)
    {
      // Show rest of current token + remaining slots
      static char remaining[256];
      remaining[0] = '\0';
      strncat (
          remaining,
          expected + tok_len,
          sizeof (remaining) - strlen (remaining) - 1
      );
      for (int i = cur_slot + 1; spec->args[i] != NULL; ++i)
      {
        strncat (remaining, " ", sizeof (remaining) - strlen (remaining) - 1);
        strncat (
            remaining,
            spec->args[i],
            sizeof (remaining) - strlen (remaining) - 1
        );
      }
      return remaining[0] ? remaining : NULL;
    }

    // Token doesn't match expected — just show remaining slots
    static char remaining2[256];
    remaining2[0] = '\0';
    for (int i = cur_slot + 1; spec->args[i] != NULL; ++i)
    {
      strncat (remaining2, " ", sizeof (remaining2) - strlen (remaining2) - 1);
      strncat (
          remaining2,
          spec->args[i],
          sizeof (remaining2) - strlen (remaining2) - 1
      );
    }
    return remaining2[0] ? remaining2 : NULL;
  }
}

int
main (int argc, char **argv)
{
  char *line;
  char *prgname = argv[0];
  int   async   = 0;

  linenoiseSetMultiLine (1);

  /* Set the completion callback. This will be called every time the
   * user uses the <tab> key. */
  linenoiseSetCompletionCallback (completion);
  linenoiseSetHintsCallback (hints);

  /* Load history from file. The history file is just a plain text file
   * where entries are separated by newlines. */
  linenoiseHistoryLoad ("history.txt"); /* Load the history at startup */

  /* Now this is the main loop of the typical linenoise-based application.
   * The call to linenoise() will block as long as the user types something
   * and presses enter.
   *
   * The typed string is returned as a malloc() allocated string by
   * linenoise, so the user needs to free() it. */

  while (1)
  {
    // Get the string
    line = linenoise ("numstore> ");
    if (line == NULL)
    {
      break;
    }

    // Handle the string
    if (strncmp (line, "exit", 4) == 0)
    {
      break;
    }
    else
    {
      error        e = error_create ();
      struct lexer lex;
      lex_tokens (line, strlen (line), &lex, &e);
      printf ("echo: '%s'\n", line);
    }

    free (line);
  }
  return 0;
}
