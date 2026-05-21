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

#include "_pynumstore.h"
#include "pynumstore.h"

#include <Python.h>
#include <string.h>

PyObject *ns_txn_rollback (PyObject *Py_UNUSED (m), PyObject *arg) {
  struct ns_txn_wrap *w = (struct ns_txn_wrap *)PyCapsule_GetPointer (arg, TXN_CAPSULE);
  if (!w || !w->ns) { return NULL; }

  nsdb_rollback (w->ns);
  nsdb_close (w->ns);
  w->ns = NULL;

  Py_RETURN_NONE;
}
