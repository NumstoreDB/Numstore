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
#include "c_specx.h"
#include "nscore/nshandle.h"
#include "nscore/rope.h"
#include "nscore/txn.h"
#include "nscore/var.h"

static sb_size
_nsdb_premove (struct nshandle *db, const char *name, void *dest, struct user_stride ustr, error *e)
{
  sb_size                     ret;                     // Return value
  b_size                      ofst;                    // Resolved offset
  t_size                      tsize;                   // Size of  the variable
  b_size                      len;                     // Length of the variable
  struct stream               _output;                 // Output stream if present
  struct stream_obuf_ctx      ctx;                     // Context for output stream
  struct stream              *output = NULL;           // Pointer to output stream
  struct chunk_alloc          temp;                    // Allocator for get operation
  struct ns_var_get_params    gparams;                 // Get operation
  struct ns_remove_params     rparams;                 // Remove operation
  struct ns_var_update_params uparams;                 // Update operation
  struct stride               stride;                  // Resolved stride
  struct string               vname = strfcstr (name); // Variable name

  chunk_alloc_create_default (&temp);

  // BEGIN TXN
  WRAP_GOTO (nsh_auto_begin_txn (db, e), failed);

  // GET VARIABLE
  {
    gparams = (struct ns_var_get_params){
        .p     = db->root->p,
        .tx    = db->atx,
        .vname = vname,
        .alloc = &temp,
    };
    err_t err = ns_var_get (&gparams, e);
    if (err == ERR_VARIABLE_NE)
    {
      ret           = 0;
      e->cause_code = SUCCESS;
      e->cmlen      = 0;
      goto commit;
    }
    WRAP_GOTO (err, failed_rollback);
  }

  // Resolve sizes
  {
    tsize = type_byte_size (gparams.dest.dtype);
    len   = gparams.dest.nbytes;

    if (len % tsize != 0)
    {
      error_causef (e, ERR_CORRUPT, "Variable: %s has invalid byte size", name);
      goto failed_rollback;
    }

    len /= tsize;

    if (stride_resolve (&stride, ustr, len, e)) { goto failed_rollback; }

    if (dest)
    {
      stream_obuf_init (&_output, &ctx, dest, stride.nelems * len);
      output = &_output;
    }
  }

  i_log_debug (
      "REMOVE - %s"
      " size (bytes): %" PRt_size " curlen: %" PRb_size " curlen (bytes): %" PRb_size
      " Requested: "
      " start: %" PRId64 " stride: %" PRId64 " stop: %" PRId64 " start (bytes): %" PRId64
      " stride (bytes): %" PRId64 " stop (bytes): %" PRId64
      " Granted: "
      " start: %" PRIu64 " stride: %" PRIu64 " nelems: %" PRIu64 " start (bytes): %" PRIu64
      " stride (bytes): %" PRIu64 " nelems (bytes): %" PRIu64 "\n",
      name,
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

  // REMOVE
  {
    rparams = (struct ns_remove_params){
        .p      = db->root->p,
        .dest   = output,
        .tx     = db->atx,
        .root   = gparams.dest.rpt_root,
        .size   = tsize,
        .bofst  = tsize * stride.start,
        .stride = stride.stride,
        .nelem  = stride.nelems,
    };
    ret = ns_remove (&rparams, e);
    WRAP_GOTO (ret, failed_rollback);
  }

  // UPDATE VARIABLE
  {
    uparams = (struct ns_var_update_params){
        .p  = db->root->p,
        .tx = db->atx,
        .retr =
            (struct var_retrieval){
                .type = VR_PG,
                .root = gparams.dest.var_root,
            },
        .newpg  = rparams.root,
        .nbytes = gparams.dest.nbytes - ret * tsize,
    };
    WRAP_GOTO (ns_var_update (uparams, e), failed_rollback);
  }

commit:

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
nsdb_remove (
    nsdb_t     *_smf,
    const char *name,
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

  return _nsdb_premove (smf, name, dest, stride, &smf->e);
}
