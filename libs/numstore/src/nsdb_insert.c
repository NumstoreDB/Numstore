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
#include "nscore/var.h"

static sb_size
_nsdb_insert (
    struct nshandle *db,
    const char      *name,
    const void      *src,
    sb_size          _ofst,
    const b_size     slen,
    error           *e
)
{
  sb_size                     ret;                     // Return value
  b_size                      bofst;                   // Resolved offset
  struct stream               _input;                  // Input stream
  struct stream_ibuf_ctx      ctx;                     // Context for input stream
  struct chunk_alloc          temp;                    // Allocator for get operation
  struct ns_var_get_params    gparams;                 // Get or create operation
  struct ns_insert_params     iparams;                 // Insert operation
  struct ns_var_update_params uparams;                 // Update operation
  struct string               vname = strfcstr (name); // Variable name

  chunk_alloc_create_default (&temp);

  // Parameter validation
  if (slen == 0) { return 0; }

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
  t_size tsize = type_byte_size (gparams.dest.dtype);
  stream_ibuf_init (&_input, &ctx, src, slen * tsize);
  bofst = var_resolve_index (&gparams.dest, tsize * _ofst);

  i_log_debug (
      "INSERT (txn = %" PRtxid
      ")"
      " - %s"
      " size (bytes): %" PRt_size " curlen: %" PRb_size " curlen (bytes): %" PRb_size
      " Requested: "
      " ofst: %" PRId64 " ofst (bytes): %" PRId64 " nelem: %" PRId64 " nbytes (bytes): %" PRId64
      " Granted: "
      " start: %" PRIu64 " start (bytes): %" PRIu64 " granted: %" PRIu64
      " granted (bytes): %" PRIu64 "\n",
      db->atx->tid,
      name,
      tsize,
      gparams.dest.nbytes / tsize,
      gparams.dest.nbytes,
      _ofst,
      _ofst * tsize,
      slen,
      slen * tsize,
      bofst / tsize,
      bofst,
      slen,
      slen * tsize
  );

  // INSERT
  {
    iparams = (struct ns_insert_params){
        .p     = db->root->p,
        .src   = &_input,
        .tx    = db->atx,
        .root  = gparams.dest.rpt_root,
        .bofst = bofst,
    };
    ret = ns_insert (&iparams, e);
    if (ret != (sb_size)(slen * tsize)) { goto failed_rollback; }
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
        .newpg  = iparams.root,
        .nbytes = gparams.dest.nbytes + ret,
    };
    WRAP_GOTO (ns_var_update (uparams, e), failed_rollback);
  }

  ASSERT (ret % tsize == 0);
  ret /= tsize;

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
nsdb_insert (nsdb_t *_smf, const char *name, const void *src, sb_size bofst, b_size slen)
{
  struct nshandle *smf = (struct nshandle *)_smf;

  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  return _nsdb_insert (smf, name, src, bofst, slen, &smf->e);
}
