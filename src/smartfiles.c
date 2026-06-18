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

#include "smartfiles.h"

#include "nshandle.h"
#include "pager.h"
#include "rope_algorithms.h"
#include "var_algorithms.h"

// smfile

int
smfile_perror (smfile_t *ns, const char *prefix)
{
  return nsh_perror ((struct nshandle *)ns, prefix);
}
const char *
smfile_strerror (smfile_t *ns)
{
  return nsh_strerror ((struct nshandle *)ns);
}

int
smfile_cleanup (const char *path)
{
  return nsh_cleanup (path);
}

// Core Operations
sb_size
smfile_size (smfile_t *smf)
{
  return smfile_psize (smf, NULL);
}

smfile_t *
smfile_new_context (smfile_t *n)
{
  return (smfile_t *)nsh_new_context ((struct nshandle *)n);
}

int
smfile_close (smfile_t *ns)
{
  return nsh_close ((struct nshandle *)ns);
}
int
smfile_crash (smfile_t *ns)
{
  return nsh_crash ((struct nshandle *)ns);
}

int
smfile_begin (smfile_t *_smf)
{
  return nsh_begin ((struct nshandle *)_smf);
}
int
smfile_commit (smfile_t *_smf)
{
  return nsh_commit ((struct nshandle *)_smf);
}
int
smfile_rollback (smfile_t *smf)
{
  return nsh_rollback ((struct nshandle *)smf);
}

/////////////////////////////////////////////////////////////////////
////// Delete

static err_t
_smfile_delete (struct nshandle *db, const char *vname, error *e)
{
  struct txn auto_txn;

  // BEGIN TXN
  WRAP_GOTO (nsh_auto_begin_txn (db, e), failed);

  i_log_debug ("DELETE (txn = %" PRtxid "): %s\n", db->atx->tid, vname);

  struct string vnamestr = strfcstr (vname);
  {
    // DELETE
    struct ns_var_delete_params params = {
        .p     = db->root->p,
        .tx    = db->atx,
        .vname = strfcstr (vname),
    };
    err_t err = ns_var_delete (params, e);
    if (err == ERR_VARIABLE_NE)
    {
      // It's ok - just return the error
      goto commit;
    }
    if (err < SUCCESS)
    {
      goto failed_rollback;
    }
  }

commit:

  if (nsh_auto_commit (db, e))
  {
    goto failed_rollback;
  }
  return error_trace (e);

failed_rollback:

  nsh_auto_rollback (db);

failed:
  return error_trace (e);
}

int
smfile_delete (smfile_t *_smf, const char *vname)
{
  struct nshandle *smf = (struct nshandle *)_smf;

  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  return _smfile_delete (smf, vname, &smf->e);
}

/////////////////////////////////////////////////////////////////////
////// Open

smfile_t *
smfile_open (const char *path)
{
  struct nshandle *ret = nsh_open (path);

  if (ret == NULL)
  {
    return NULL;
  }

  // Create the default variable
  if (pgr_isnew (ret->root->p))
  {
    // BEGIN TXN
    struct txn tx;
    if (pgr_begin_txn (&tx, ret->root->p, &ret->e))
    {
      goto failed;
    }

    struct ns_var_create_params params = {
        .p     = ret->root->p,
        .tx    = &tx,
        .vname = strfcstr (DEFAULT_VARIABLE),
        .type  = &(struct type){.type = T_PRIM, .p = U8},
    };
    if (ns_var_create (params, &ret->e))
    {
      goto failed;
    }

    // COMMIT
    if (pgr_commit (ret->root->p, &tx, &ret->e))
    {
      goto failed;
    }
  }

  return (smfile_t *)ret;

failed:
  nsh_close (ret);

  return NULL;
}

/////////////////////////////////////////////////////////////////////
////// Insert

static sb_size
_smfile_pinsert (
    struct nshandle *db,
    const char      *name,
    const void      *src,
    const b_size     slen,
    sb_size          bofst,
    error           *e
)
{
  sb_size                            ret;        // Return value
  b_size                             ofst;       // Resolved offset
  struct stream                      _input;     // Input stream
  struct stream_ibuf_ctx             ctx;        // Context for input stream
  struct chunk_alloc                 temp;       // Allocator for get operation
  struct ns_var_get_or_create_params gparams;    // Get or create operation
  struct ns_insert_params            iparams;    // Insert operation
  struct ns_var_update_params        uparams;    // Update operation
  struct string vname = vname_or_default (name); // Variable name

  // Parameter validation
  if (slen == 0)
  {
    return 0;
  }

  chunk_alloc_create_default (&temp);
  stream_ibuf_init (&_input, &ctx, src, slen);

  // BEGIN TXN
  WRAP_GOTO (nsh_auto_begin_txn (db, e), failed);

  // GET OR CREATE VARIABLE
  {
    gparams = (struct ns_var_get_or_create_params){
        .p     = db->root->p,
        .tx    = db->atx,
        .vname = vname,
        .type  = &(struct type){.type = T_PRIM, .p = U8},
        .alloc = &temp,
    };
    WRAP_GOTO (ns_var_get_or_create (&gparams, e), failed_rollback);
  }

  // Resolve sizes
  {
    ofst = var_resolve_index (&gparams.dest, bofst);
  }

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
        .newpg  = iparams.root,
        .nbytes = gparams.dest.nbytes + ret,
    };
    WRAP_GOTO (ns_var_update (uparams, e), failed_rollback);
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
smfile_pinsert (
    smfile_t   *_smf,
    const char *name,
    const void *src,
    sb_size     bofst,
    b_size      slen
)
{
  struct nshandle *smf = (struct nshandle *)_smf;

  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  return _smfile_pinsert (smf, name, src, slen, bofst, &smf->e);
}

sb_size
smfile_insert (smfile_t *smf, const void *src, sb_size bofst, b_size slen)
{
  return smfile_pinsert (smf, NULL, src, bofst, slen);
}

/////////////////////////////////////////////////////////////////////
////// Read

static sb_size
_smfile_pread (
    struct nshandle *db,
    const char      *name,
    void            *dest,
    const t_size     size,
    const sb_size    bofst,
    const sb_size    stride,
    b_size           nelem,
    error           *e
)
{
  sb_size                  ret;           // Return value
  b_size                   ofst;          // Resolved offset
  struct stream            _output;       // Output stream if present
  struct stream_obuf_ctx   ctx;           // Context for output stream
  struct stream           *output = NULL; // Pointer to output stream
  struct chunk_alloc       temp;          // Allocator for get operation
  struct ns_var_get_params gparams;       // Get operation
  struct ns_read_params    rparams;       // Read operation
  struct string            vname = vname_or_default (name); // Variable name

  // Parameter validation
  if (stride < 0)
  {
    return error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "Negative strides aren't supported yet"
    );
  }
  if (stride == 0)
  {
    return error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "Cannot read with stride == 0"
    );
  }
  if (size == 0)
  {
    return error_causef (e, ERR_INVALID_ARGUMENT, "Cannot read with size == 0");
  }
  if (nelem == 0)
  {
    return 0;
  }

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
    ofst  = var_resolve_index (&gparams.dest, bofst);
    nelem = var_resolve_nelem (&gparams.dest, ofst, nelem, size);
    if (nelem == 0)
    {
      ret = 0;
      goto commit;
    }
    if (dest)
    {
      stream_obuf_init (&_output, &ctx, dest, size * nelem);
      output = &_output;
    }
  }

  // READ
  {
    rparams = (struct ns_read_params){
        .p      = db->root->p,
        .dest   = output,
        .tx     = db->atx,
        .root   = gparams.dest.rpt_root,
        .size   = size,
        .bofst  = ofst,
        .stride = stride,
        .nelem  = nelem,
    };
    ret = ns_read (rparams, e);
    WRAP_GOTO (ret, failed_rollback);
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
smfile_pread (
    smfile_t   *_smf,
    const char *name,
    void       *dest,
    t_size      size,
    sb_size     bofst,
    sb_size     stride,
    b_size      nelem
)
{
  struct nshandle *smf = (struct nshandle *)_smf;

  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  return _smfile_pread (smf, name, dest, size, bofst, stride, nelem, &smf->e);
}

sb_size
smfile_read (smfile_t *smf, void *dest, sb_size bofst, b_size nelem)
{
  return smfile_pread (smf, NULL, dest, 1, bofst, 1, nelem);
}

/////////////////////////////////////////////////////////////////////
////// Remove

static sb_size
_smfile_premove (
    struct nshandle *db,
    const char      *name,
    void            *dest,
    const t_size     size,
    const sb_size    bofst,
    const sb_size    stride,
    b_size           nelem,
    error           *e
)
{
  sb_size                     ret;           // Return value
  b_size                      ofst;          // Resolved offset
  struct stream               _output;       // Output stream if present
  struct stream_obuf_ctx      ctx;           // Context for output stream
  struct stream              *output = NULL; // Pointer to output stream
  struct chunk_alloc          temp;          // Allocator for get operation
  struct ns_var_get_params    gparams;       // Get operation
  struct ns_remove_params     rparams;       // Remove operation
  struct ns_var_update_params uparams;       // Update operation
  struct string               vname = vname_or_default (name); // Variable name

  // Parameter validation
  if (stride < 0)
  {
    return error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "Negative strides aren't supported yet"
    );
  }
  if (stride == 0)
  {
    return error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "Cannot remove with stride == 0"
    );
  }
  if (size == 0)
  {
    return error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "Cannot remove with size == 0"
    );
  }
  if (nelem == 0)
  {
    return 0;
  }

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
    ofst  = var_resolve_index (&gparams.dest, bofst);
    nelem = var_resolve_nelem (&gparams.dest, ofst, nelem, size);
    if (nelem == 0)
    {
      ret = 0;
      goto commit;
    }
    if (dest)
    {
      stream_obuf_init (&_output, &ctx, dest, size * nelem);
      output = &_output;
    }
  }

  // REMOVE
  {
    rparams = (struct ns_remove_params){
        .p      = db->root->p,
        .dest   = output,
        .tx     = db->atx,
        .root   = gparams.dest.rpt_root,
        .size   = size,
        .bofst  = ofst,
        .stride = stride,
        .nelem  = nelem,
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
        .nbytes = gparams.dest.nbytes - ret * size,
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
smfile_premove (
    smfile_t   *_smf,
    const char *name,
    void       *dest,
    t_size      size,
    sb_size     bofst,
    sb_size     stride,
    b_size      nelem
)
{
  struct nshandle *smf = (struct nshandle *)_smf;

  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  return _smfile_premove (smf, name, dest, size, bofst, stride, nelem, &smf->e);
}

sb_size
smfile_remove (smfile_t *smf, void *dest, sb_size bofst, b_size nelem)
{
  return smfile_premove (smf, NULL, dest, 1, bofst, 1, nelem);
}

/////////////////////////////////////////////////////////////////////
////// Write

static sb_size
_smfile_pwrite (
    struct nshandle *db,
    const char      *name,
    const void      *src,
    const t_size     size,
    const sb_size    bofst,
    const sb_size    stride,
    const b_size     nelem,
    error           *e
)
{
  sb_size                ret;          // Return value
  sb_size                inserted;     // Number of bytes inserted
  b_size                 ofst;         // Resolved offset
  b_size                 write_nelem;  // Elements that fit in existing variable
  b_size                 insert_nelem; // Remainder to insert past the end
  struct stream          _input;       // Input stream
  struct stream_ibuf_ctx ctx;          // Context for input stream
  struct chunk_alloc     temp;         // Allocator for get operation
  struct ns_var_get_or_create_params gparams;    // Get or create operation
  struct ns_write_params             wparams;    // Write operation
  struct ns_insert_params            iparams;    // Insert operation
  struct ns_var_update_params        uparams;    // Update operation
  struct string vname = vname_or_default (name); // Variable name

  // Parameter validation
  if (stride < 0)
  {
    return error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "Negative strides aren't supported yet"
    );
  }
  if (stride == 0)
  {
    return error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "Cannot write with stride == 0"
    );
  }
  if (size == 0)
  {
    return error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "Cannot write with size == 0"
    );
  }
  if (nelem == 0)
  {
    return 0;
  }

  chunk_alloc_create_default (&temp);

  // BEGIN TXN
  WRAP_GOTO (nsh_auto_begin_txn (db, e), failed);

  // GET OR CREATE VARIABLE
  {
    gparams = (struct ns_var_get_or_create_params){
        .p     = db->root->p,
        .tx    = db->atx,
        .vname = vname,
        .type  = &(struct type){.type = T_PRIM, .p = U8},
        .alloc = &temp,
    };
    WRAP_GOTO (ns_var_get_or_create (&gparams, e), failed_rollback);
  }

  // Resolve sizes
  {
    ofst         = var_resolve_index (&gparams.dest, bofst);
    write_nelem  = var_resolve_nelem (&gparams.dest, ofst, nelem, size);
    insert_nelem = nelem - write_nelem;
    if (insert_nelem > 0 && stride != 1)
    {
      error_causef (
          e,
          ERR_INVALID_ARGUMENT,
          "Cannot write past end with stride != 1"
      );
      goto failed_rollback;
    }
  }

  // WRITE
  {
    stream_ibuf_init (&_input, &ctx, src, size * write_nelem);

    wparams = (struct ns_write_params){
        .p      = db->root->p,
        .src    = &_input,
        .tx     = db->atx,
        .root   = gparams.dest.rpt_root,
        .size   = size,
        .bofst  = ofst,
        .stride = stride,
        .nelem  = write_nelem,
    };

    ret = ns_write (wparams, e);
    WRAP_GOTO (ret, failed_rollback);
  }

  // INSERT REMAINDER
  if (insert_nelem > 0)
  {
    // INSERT
    {
      stream_ibuf_init (
          &_input,
          &ctx,
          (u8 *)src + write_nelem * size,
          insert_nelem * size
      );

      iparams = (struct ns_insert_params){
          .p     = db->root->p,
          .src   = &_input,
          .tx    = db->atx,
          .root  = wparams.root,
          .bofst = gparams.dest.nbytes, // Append
      };

      inserted = ns_insert (&iparams, e);
      WRAP_GOTO (inserted, failed_rollback);
      ret += inserted;
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
          .nbytes = gparams.dest.nbytes + inserted,
      };
      WRAP_GOTO (ns_var_update (uparams, e), failed_rollback);
    }
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
smfile_pwrite (
    smfile_t   *_smf,
    const char *name,
    const void *src,
    t_size      size,
    b_size      bofst,
    sb_size     stride,
    b_size      nelem
)
{
  struct nshandle *smf = (struct nshandle *)_smf;

  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  return _smfile_pwrite (smf, name, src, size, bofst, stride, nelem, &smf->e);
}

sb_size
smfile_write (smfile_t *smf, const void *src, b_size bofst, b_size nelem)
{
  return smfile_pwrite (smf, NULL, src, 1, bofst, 1, nelem);
}

/////////////////////////////////////////////////////////////////////
////// Size

static sb_size
_smfile_psize (struct nshandle *db, const char *name, error *e)
{
  struct chunk_alloc temp;
  chunk_alloc_create_default (&temp);
  struct string vname = vname_or_default (name);
  b_size        ret;

  // BEGIN TXN
  if (nsh_auto_begin_txn (db, e) < 0)
  {
    goto failed;
  }

  // GET OR CREATE VARIABLE
  struct ns_var_get_params gparams = {
      .p     = db->root->p,
      .tx    = db->atx,
      .vname = vname,
      .alloc = &temp,
  };
  if (ns_var_get (&gparams, e))
  {
    goto failed;
  }

  ret = gparams.dest.nbytes;

  // COMMIT
  if (nsh_auto_commit (db, e) < 0)
  {
    goto failed_rollback;
  }

  chunk_alloc_free_all (&temp);

  return ret;

failed_rollback:

  nsh_auto_rollback (db);

failed:
  chunk_alloc_free_all (&temp);
  return error_trace (e);
}

sb_size
smfile_psize (smfile_t *_smf, const char *name)
{
  struct nshandle *smf = (struct nshandle *)_smf;

  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  return _smfile_psize (smf, name, &smf->e);
}
