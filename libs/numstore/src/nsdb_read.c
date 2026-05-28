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

#include "_numstore.h"
#include "nscore/nshandle.h"
#include "nscore/rope.h"
#include "nscore/txn.h"
#include "nscore/types.h"
#include "nscore/var.h"
#include "nscore/variables.h"

#include <c_specx.h>

static sb_size
_nsdb_read (struct nshandle *db, nsdb_var_t *var, void *dest, struct user_stride ustr, error *e)
{
  sb_size                  ret;           // Return value
  t_size                   tsize;         // Size of  the variable
  b_size                   len;           // Length of the variable
  struct stream            _output;       // Output stream if present
  struct stream_obuf_ctx   ctx;           // Context for output stream
  struct stream           *output = NULL; // Pointer to output stream
  struct chunk_alloc       temp;          // Allocator for get operation
  struct ns_var_get_params gparams;       // Get operation
  struct ns_read_params    rparams;       // Read operation
  struct stride            stride;        // Resolved stride

  chunk_alloc_create_default (&temp);

  // BEGIN TXN
  WRAP_GOTO (nsh_auto_begin_txn (db, e), failed);

  // GET VARIABLE
  {
    gparams = (struct ns_var_get_params){
        .p     = db->root->p,
        .tx    = db->atx,
        .vname = var->var.vname,
        .alloc = &temp,
    };
    WRAP_GOTO (ns_var_get (&gparams, e), failed_rollback);

    // Type check
    if (!type_equal (var->var.dtype, gparams.dest.dtype))
    {
      error_causef (e, ERR_INVALID_ARGUMENT, "Conflicting types on insert");
      goto failed_rollback;
    }
  }

  // Resolve sizes
  {
    tsize = type_byte_size (gparams.dest.dtype);
    len   = gparams.dest.nbytes;

    if (len % tsize != 0)
    {
      error_causef (
          e,
          ERR_CORRUPT,
          "Variable: %.*s has invalid byte size",
          strfmt (&var->var.vname)
      );
      goto failed_rollback;
    }

    len /= tsize;

    if (stride_resolve (&stride, ustr, len, e)) { goto failed_rollback; }

    // Initialize the output buffer
    if (dest)
    {
      stream_obuf_init (&_output, &ctx, dest, stride.nelems * tsize);
      output = &_output;
    }
  }

  i_log_debug (
      "READ (txn = %" PRtxid
      ")"
      " - %.*s"
      " size (bytes): %" PRt_size " curlen: %" PRb_size " curlen (bytes): %" PRb_size
      " Requested: "
      " start: %" PRId64 " stride: %" PRId64 " stop: %" PRId64 " start (bytes): %" PRId64
      " stride (bytes): %" PRId64 " stop (bytes): %" PRId64
      " Granted: "
      " start: %" PRIu64 " stride: %" PRIu64 " nelems: %" PRIu64 " start (bytes): %" PRIu64
      " stride (bytes): %" PRIu64 " nelems (bytes): %" PRIu64 "\n",
      db->atx->tid,
      strfmt (&var->var.vname),
      tsize,
      len,
      gparams.dest.nbytes,
      ustr.present & START_PRESENT ? ustr.start : 0,
      ustr.present & STEP_PRESENT ? ustr.step : 0,
      ustr.present & STOP_PRESENT ? ustr.stop : 0,
      ustr.present & START_PRESENT ? tsize * ustr.start : 0,
      ustr.present & STEP_PRESENT ? tsize * ustr.step : 0,
      ustr.present & STOP_PRESENT ? tsize * ustr.stop : 0,
      stride.start,
      stride.stride,
      stride.nelems,
      tsize * stride.start,
      tsize * stride.stride,
      tsize * stride.nelems
  );

  // READ
  {
    rparams = (struct ns_read_params){
        .p      = db->root->p,
        .dest   = output,
        .tx     = db->atx,
        .root   = gparams.dest.rpt_root,
        .size   = tsize,
        .bofst  = tsize * stride.start,
        .stride = stride.stride,
        .nelem  = stride.nelems,
    };
    ret = ns_read (rparams, e);
    WRAP_GOTO (ret, failed_rollback);
  }

  // COMMIT
  WRAP_GOTO (nsh_auto_commit (db, e), failed_rollback);
  chunk_alloc_free_all (&temp);
  return ret;

failed_rollback:

  nsh_auto_rollback (db);

failed:
  chunk_alloc_free_all (&temp);
  return error_trace (e);
}

sb_size
nsdb_read (
    nsdb_t     *_smf,
    nsdb_var_t *var,
    void       *dest,
    sb_size     start,
    sb_size     step,
    sb_size     stop,
    int         flags
)
{
  struct user_stride stride = {
      .start   = start,
      .step    = step,
      .stop    = stop,
      .present = flags,
  };
  struct nshandle *smf = (struct nshandle *)_smf;

  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  return _nsdb_read (smf, var, dest, stride, &smf->e);
}
