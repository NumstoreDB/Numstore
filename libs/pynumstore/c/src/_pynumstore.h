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

#pragma once

#include "numstore.h"
#include "pynumstore.h"

static const char DB_CAPSULE[]  = "numstore.db";
static const char TXN_CAPSULE[] = "numstore.txn";

HEADER_FUNC void
_nspy_release_db (PyObject *obj)
{
  nsdb_t *ns = (nsdb_t *)PyCapsule_GetPointer (obj, DB_CAPSULE);
  ASSERT (ns);
  nsdb_close (ns);
}

HEADER_FUNC struct nshandle *
_unwrap_db (PyObject *db)
{
  struct nshandle *p = (struct nshandle *)PyCapsule_GetPointer (db, DB_CAPSULE);
  return p;
}

HEADER_FUNC struct txn *
_unwrap_txn (PyObject *txn_or_none)
{
  if (txn_or_none == Py_None) { return NULL; }
  struct txn *p = (struct txn *)PyCapsule_GetPointer (txn_or_none, TXN_CAPSULE);
  return p;
}
