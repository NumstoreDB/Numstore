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
#include "numstore.h"
#include "variables.h"

PyObject *
pyns_var_exists (PyObject *Py_UNUSED (m), PyObject *args)
{
  PyObject   *db          = NULL;
  PyObject   *txn_or_none = NULL;
  const char *name        = NULL;
  nsdb_var_t *var         = NULL;

  if (!PyArg_ParseTuple (args, "OOs", &db, &txn_or_none, &name))
  {
    goto fail;
  }

  nsdb_t *ns = _active_ns (db, txn_or_none);
  if (!ns)
  {
    goto fail;
  }

  if (nsdb_get_if_exists (ns, &var, name))
  {
    goto fail;
  }

  bool exists = var != NULL;
  if (var)
  {
    nsdb_free (var);
  }

  return PyBool_FromLong (exists);

fail:
  nsdb_free (var);
  return NULL;
}
