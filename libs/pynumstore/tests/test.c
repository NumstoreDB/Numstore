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

#include "c_specx.h"

#include <Python.h>
#include <stdio.h>
#include <stdlib.h>

extern int pynumstore_import_numpy (void);

TEST_SUITE (pynumstore, 16);

int main (void) {
  Py_Initialize ();

  /* Seed sys.path so 'import pynumstore' finds the pure-Python package
   * without requiring the C extension to be installed first. */
  PyRun_SimpleString ("import sys; sys.path.insert(0, '" PYNUMSTORE_PYPATH "')");

  if (pynumstore_import_numpy () < 0) {
    PyErr_Print ();
    Py_Finalize ();
    return 1;
  }

  REGISTER (pynumstore, numstore_to_dtype);
  REGISTER (pynumstore, numpy_to_numstore_cases);

  bool all_passed = true;
  for (int i = 0; i < pynumstore_count; i++) {
    test *t = &pynumstore_tests[i];
    if (!t->test ()) {
      i_log_failure ("FAILED: %s\n", t->test_name);
      all_passed = false;
    }
  }

  if (all_passed) {
    i_log_passed ("ALL TESTS PASSED\n");
  }

  Py_Finalize ();
  return all_passed ? 0 : 1;
}
