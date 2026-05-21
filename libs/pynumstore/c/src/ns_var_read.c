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

#include "c_specx.h"
#include "nscore/nshandle.h"
#include "nscore/rope.h"
#include "nscore/types.h"

#include <Python.h>
#include <stdio.h>
#include <string.h>

err_t pybr_ns_var_get (
    struct pager    *p,
    struct txn      *tx,
    const char      *vname_str,
    u32              vname_len,
    struct variable *dest,
    error           *e);

PyObject *pyns_var_read (PyObject *Py_UNUSED (m), PyObject *args) {
  PyObject *db;
  PyObject *txn_or_none;

  long long var_id, key;
  if (!PyArg_ParseTuple (args, "OOLL", &db, &txn_or_none, &var_id, &key)) { return NULL; }

  nsdb_t *handle = _resolve_handle (db, txn_or_none);
  if (!handle) { return NULL; }

  struct nshandle *ns = (struct nshandle *)handle;

  char buf[32];
  snprintf (buf, sizeof (buf), "%lld", var_id);

  error e = error_create ();

  if (nsh_auto_begin_txn (ns, &e) < 0) {
    PyErr_Format (PyExc_RuntimeError, "%.*s", e.cmlen, e.cause_msg);
    return NULL;
  }

  struct variable dest;
  if (pybr_ns_var_get (ns->root->p, ns->atx, buf, (u32)strlen (buf), &dest, &e) < 0) {
    nsh_auto_rollback (ns);
    PyErr_Format (PyExc_RuntimeError, "%.*s", e.cmlen, e.cause_msg);
    return NULL;
  }

  u32 tsize = type_byte_size (dest.dtype);

  PyObject *ret = PyBytes_FromStringAndSize (NULL, (Py_ssize_t)tsize);
  if (!ret) {
    nsh_auto_rollback (ns);
    return NULL;
  }

  struct user_stride ustr = {
      .start   = key,
      .step    = 1,
      .stop    = key + 1,
      .present = START_PRESENT | STOP_PRESENT | STEP_PRESENT,
  };

  u64 total_elems = (tsize > 0) ? (dest.nbytes / tsize) : 0;

  struct stride stride;
  if (stride_resolve (&stride, ustr, total_elems, &e) < 0) {
    nsh_auto_rollback (ns);
    Py_DECREF (ret);
    PyErr_Format (PyExc_RuntimeError, "%.*s", e.cmlen, e.cause_msg);
    return NULL;
  }

  struct stream          output;
  struct stream_obuf_ctx ctx;
  stream_obuf_init (&output, &ctx, PyBytes_AS_STRING (ret), tsize);

  struct ns_read_params rparams = {
      .p      = ns->root->p,
      .dest   = &output,
      .tx     = ns->atx,
      .root   = dest.rpt_root,
      .size   = tsize,
      .bofst  = tsize * stride.start,
      .stride = (sb_size)stride.stride,
      .nelem  = stride.nelems,
  };

  sb_size nread = ns_read (rparams, &e);

  if (nread < 0) {
    nsh_auto_rollback (ns);
    Py_DECREF (ret);
    PyErr_Format (PyExc_RuntimeError, "%.*s", e.cmlen, e.cause_msg);
    return NULL;
  }

  if (nsh_auto_commit (ns, &e) < 0) {
    Py_DECREF (ret);
    PyErr_Format (PyExc_RuntimeError, "%.*s", e.cmlen, e.cause_msg);
    return NULL;
  }

  return ret;
}
