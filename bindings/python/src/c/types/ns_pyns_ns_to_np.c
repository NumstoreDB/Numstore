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

#include "ns_pynumstore.h"
#include "nscore/compiler/ns_compiler.h"

PyObject *
pyns_ns_to_np (PyObject *Py_UNUSED (m), PyObject *arg)
{
  ALLOC_INIT (alloc);
  struct type    t;
  error          e   = error_create ();
  PyArray_Descr *ret = NULL;

  // Extract utf8 string from argument
  const char    *src = PyUnicode_AsUTF8 (arg);
  if (!src) {
    goto theend;
  }

  // compile the type string - a type that won't compile is a bad argument
  // value, not a database failure
  if (compile_type (&t, src, &alloc, &e)) {
    _pyns_set_error_from_e (PyExc_ValueError, &e);
    goto theend;
  }

  // Convert it to a numpy type
  ret = pyns_type_to_dtype (&t);

theend:
  ALLOC_CLOSE (alloc);
  return (PyObject *)ret;
}
