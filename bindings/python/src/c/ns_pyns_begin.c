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

/// pyns_begin.c
///
/// pyns_begin(db) -> capsule
/// Starts a new transaction and wraps the handle in a TXN_CAPSULE. No
/// destructor is registered - a transaction must be explicitly committed
/// or rolled back (unlike a Database, it isn't auto-cleaned on GC).

#include "ns_pynumstore.h"

PyObject *
pyns_begin (PyObject *Py_UNUSED (m), PyObject *arg)
{
  // Get the wrapped database
  nsdb_t *ns = _unwrap_db (arg);
  if (!ns) {
    return NULL;
  }

  // BEGIN TXN
  ns_txn_t *txn = nsdb_begin (ns);
  if (!txn) {
    _pyns_set_error (ns);
    return NULL;
  }

  return PyCapsule_New ((void *)txn, TXN_CAPSULE, NULL);
}
