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

#include "testing/unit_tests.h"

int
main (const int argc, char **argv)
{
  const char *filter = (argc > 1) ? argv[1] : NULL;
  int         ret    = run_unit_tests (filter);

  if (ret == 0)
  {
    i_log_passed ("All Tests Passed\n");
  }
  else
  {
    i_log_failure ("Unit Tests Failed\n");
  }

  return ret;
}
