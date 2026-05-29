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

#include "register_test_suite.h"

#define PY_ARRAY_UNIQUE_SYMBOL _NUMSTORE_ARRAY_API
#define PY_SSIZE_T_CLEAN
#define NPY_NO_DEPRECATED_API NPY_2_0_API_VERSION
#include <Python.h>
#include <c_specx.h>
#include <numpy/arrayobject.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main (const int argc, char **argv)
{
  Py_Initialize ();
  PyRun_SimpleString ("import sys; sys.path.insert(0, '" NUMSTORE_SO_DIR "')");
  PyObject *mod = PyImport_ImportModule ("_numstore");
  _import_array ();

  register_tests ();

  const char *filter = (argc > 1) ? argv[1] : NULL;

  error e = error_create ();

  i_timer timer;
  if (i_timer_create (&timer, &e) != SUCCESS)
  {
    return -1;
  }

  struct dbl_buffer f;
  if (dblb_create (&f, sizeof (char *), 1, &e))
  {
    return -1;
  }

  for (u32 i = 0; i < (u32)smartfiles_count; i++)
  {
    test *t = &smartfiles_tests[i];

    if (filter)
    {
      const struct string tn  = strfcstr (t->test_name);
      const struct string flt = strfcstr (filter);

      if (!string_contains (tn, flt))
      {
        continue;
      }
    }

    if (!t->test ())
    {
      char **n = &t->test_name;
      if (dblb_append (&f, n, 1, &e))
      {
        dblb_free (&f);
        return -1;
      }
    }
  }

  printf ("Time: %llu ms\n", (unsigned long long)i_timer_now_ms (&timer));
  i_timer_free (&timer);

  char **fl = f.data;
  if (f.nelem > 0)
  {
    i_log_failure ("FAILED TESTS:\n");
    for (u32 i = 0; i < f.nelem; i++)
    {
      i_log_failure ("  %s\n", fl[i]);
    }
  }
  else
  {
    i_log_passed ("ALL TESTS PASSED\n");
  }

  dblb_free (&f);

  Py_Finalize ();

  return test_ret;
}
