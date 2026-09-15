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

#include "core/ns_logging.h"

#include "core/testing/ns_testing.h"

#include <stdarg.h>

////////////////////////////////////////////////////////////
// LOGGING
void
i_log_internal (const char *prefix, const char *color, const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  fprintf (stdout, "%s[%-8.8s]: ", color, prefix);
  vfprintf (stdout, fmt, args);
  fprintf (stdout, "%s", RESET);
  va_end (args);
}

void
i_printf (const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  vfprintf (stdout, fmt, args);
  va_end (args);
}

void
i_log_flush (void)
{
  fflush (stdout);
}

////////////////////////////////////////////////////////////
// JSON PRINTING

void
print_json (const char *first, ...)
{
  va_list args;
  va_start (args, first);

  fputs ("{ ", stdout);

  bool        is_first = true;
  const char *key      = first;
  while (key != NULL) {
    const char *value = va_arg (args, const char *);

    if (!is_first) {
      fputs (", ", stdout);
    }
    is_first = false;

    fprintf (stdout, "\"%s\": %s", key, value);

    key = va_arg (args, const char *);
  }

  fputs (" }\n", stdout);

  va_end (args);
}

#ifdef TESTING
TEST (i_log)
{
  // Just make sure it doesn't crash
  TEST_CASE ("Smoke test")
  {
    i_log_internal ("FOO", BLUE, "bar: %s %d\n", "biz", 1);
    i_log_flush ();
  }
}

TEST (i_printf)
{
  // Just make sure it doesn't crash
  TEST_CASE ("Smoke test")
  {
    i_printf ("bar: %s %d\n", "biz", 1);
    i_log_flush ();
  }
}
#endif
