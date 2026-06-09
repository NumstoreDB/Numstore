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

#include "numstore.h"



#include "alloc.h"
#include "compiler.h"
#include "nshandle.h"
#include "pager.h"
#include "rope.h"
#include "var.h"

int
nsdb_validate (nsdb_t *ns)
{
  return 0;
}

nsdb_t *
nsdb_open (const char *path)
{
  struct nshandle *ret = nsh_open (path);

  if (ret == NULL)
  {
    return NULL;
  }

  return (nsdb_t *)ret;
}

int
nsdb_perror (nsdb_t *ns, const char *prefix)
{
  return nsh_perror ((struct nshandle *)ns, prefix);
}

const char *
nsdb_strerror (nsdb_t *ns)
{
  return nsh_strerror ((struct nshandle *)ns);
}

int
nsdb_cleanup (const char *path)
{
  return nsh_cleanup (path);
}

nsdb_t *
nsdb_new_context (nsdb_t *n)
{
  return (nsdb_t *)nsh_new_context ((struct nshandle *)n);
}

int
nsdb_close (nsdb_t *ns)
{
  return nsh_close ((struct nshandle *)ns);
}

int
nsdb_crash (nsdb_t *ns)
{
  return nsh_crash ((struct nshandle *)ns);
}

int
nsdb_begin (nsdb_t *_smf)
{
  int ret = nsh_begin ((struct nshandle *)_smf);
  if (ret == 0)
  {
    i_log_debug (
        "a BEGIN TXN: %" PRtxid "\n",
        ((struct nshandle *)_smf)->atx->tid
    );
  }
  return ret;
}

int
nsdb_commit (nsdb_t *_smf)
{
  i_log_debug ("COMMIT: %" PRtxid "\n", ((struct nshandle *)_smf)->atx->tid);
  return nsh_commit ((struct nshandle *)_smf);
}

int
nsdb_rollback (nsdb_t *smf)
{
  i_log_debug ("ROLLBACK: %" PRtxid "\n", ((struct nshandle *)smf)->atx->tid);
  return nsh_rollback ((struct nshandle *)smf);
}

void
nsdb_free (nsdb_var_t *var)
{
  chunk_alloc_free_all (&var->alloc);
  i_free (var);
}

sb_size
nsdb_vinsert (
    nsdb_t     *ns,
    const char *name,
    const void *src,
    sb_size     ofst,
    b_size      slen
)
{
  nsdb_var_t *var = nsdb_get (ns, name);
  sb_size     ret = nsdb_insert (ns, var, src, ofst, slen);
  nsdb_free (var);
  return ret;
}

sb_size
nsdb_vwrite (
    nsdb_t *ns,

    const char *name,
    const void *src,
    sb_size     start,
    sb_size     step,
    sb_size     stop,
    int         flags
)
{
  nsdb_var_t *var = nsdb_get (ns, name);
  sb_size     ret = nsdb_write (ns, var, src, start, step, stop, flags);
  nsdb_free (var);
  return ret;
}

sb_size
nsdb_vread (
    nsdb_t     *ns,
    const char *name,
    void       *dest,
    sb_size     start,
    sb_size     step,
    sb_size     stop,
    int         flags
)
{
  nsdb_var_t *var = nsdb_get (ns, name);
  sb_size     ret = nsdb_read (ns, var, dest, start, step, stop, flags);
  nsdb_free (var);
  return ret;
}

sb_size
nsdb_vremove (
    nsdb_t     *ns,
    const char *name,
    void       *dest,
    sb_size     start,
    sb_size     step,
    sb_size     stop,
    int         flags
)
{
  nsdb_var_t *var = nsdb_get (ns, name);
  sb_size     ret = nsdb_remove (ns, var, dest, start, step, stop, flags);
  nsdb_free (var);
  return ret;
}

/////////////////////////////////////////////////////////////////////
////// Create

static int
_nsdb_create (struct nshandle *db, const char *name, const char *type, error *e)
{
  struct chunk_alloc                 temp;    // Allocator for get operation
  struct ns_var_get_or_create_params gparams; // Get or create operation

  struct string vname = strfcstr (name); // Variable name
  chunk_alloc_create_default (&temp);

  // Compile type
  struct type dtype;
  WRAP_GOTO (compile_type (&dtype, type, &temp, e), failed);

  // BEGIN TXN
  WRAP_GOTO (nsh_auto_begin_txn (db, e), failed);

  i_log_debug ("CREATE (txn = %" PRtxid "): %s %s\n", db->atx->tid, name, type);

  // GET OR CREATE VARIABLE
  {
    gparams = (struct ns_var_get_or_create_params){
        .p     = db->root->p,
        .tx    = db->atx,
        .vname = vname,
        .type  = &dtype,
        .alloc = &temp,
    };
    WRAP_GOTO (ns_var_get_or_create (&gparams, e), failed_rollback);
  }

  // COMMIT
  WRAP_GOTO (nsh_auto_commit (db, e), failed_rollback);
  chunk_alloc_free_all (&temp);

  return SUCCESS;

failed_rollback:

  nsh_auto_rollback (db);

failed:
  chunk_alloc_free_all (&temp);
  return error_trace (e);
}

int
nsdb_create (nsdb_t *_smf, const char *name, const char *type)
{
  struct nshandle *smf = (struct nshandle *)_smf;

  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  return _nsdb_create (smf, name, type, &smf->e);
}

/////////////////////////////////////////////////////////////////////
////// Delete

static err_t
_nsdb_delete (struct nshandle *db, const char *vname, error *e)
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
    if (err < SUCCESS)
    {
      goto failed_rollback;
    }
  }

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
nsdb_delete (nsdb_t *_smf, const char *vname)
{
  struct nshandle *smf = (struct nshandle *)_smf;

  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  return _nsdb_delete (smf, vname, &smf->e);
}

/////////////////////////////////////////////////////////////////////
////// Get

static nsdb_var_t *
_nsdb_get (struct nshandle *db, const char *name, error *e)
{
  nsdb_var_t              *ret = NULL;              // Return value
  struct ns_var_get_params gparams;                 // Get or create operation
  struct string            vname = strfcstr (name); // Variable name

  ret = i_malloc (1, sizeof *ret, e);
  if (ret == NULL)
  {
    return NULL;
  }

  chunk_alloc_create_default (&ret->alloc);

  // BEGIN TXN
  WRAP_GOTO (nsh_auto_begin_txn (db, e), failed);

  i_log_debug (
      "GET (txn = %" PRtxid
      ")"
      " - %s\n",
      db->atx->tid,
      name
  );

  // GET VARIABLE
  {
    gparams = (struct ns_var_get_params){
        .p     = db->root->p,
        .tx    = db->atx,
        .vname = vname,
        .alloc = &ret->alloc,
    };
    err_t err = ns_var_get (&gparams, e);
    WRAP_GOTO (err, failed_rollback);

    ret->var = gparams.dest;
  }

  // COMMIT
  WRAP_GOTO (nsh_auto_commit (db, e), failed_rollback);

  return ret;

failed_rollback:

  nsh_auto_rollback (db);

failed:
  chunk_alloc_free_all (&ret->alloc);
  i_free (ret);
  return NULL;
}

nsdb_var_t *
nsdb_get (nsdb_t *_smf, const char *name)
{
  struct nshandle *smf = (struct nshandle *)_smf;

  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  return _nsdb_get (smf, name, &smf->e);
}

/////////////////////////////////////////////////////////////////////
////// Get if exists

static int
_nsdb_get_if_exists (
    struct nshandle *db,
    const char      *name,
    nsdb_var_t     **dest,
    error           *e
)
{
  nsdb_var_t              *ret = NULL;              // Return value
  struct ns_var_get_params gparams;                 // Get or create operation
  struct string            vname = strfcstr (name); // Variable name

  ret = i_malloc (1, sizeof *ret, e);
  if (ret == NULL)
  {
    return error_trace (e);
  }

  chunk_alloc_create_default (&ret->alloc);

  // BEGIN TXN
  WRAP_GOTO (nsh_auto_begin_txn (db, e), failed);

  i_log_debug (
      "GET IF EXISTS (txn = %" PRtxid
      ")"
      " - %s\n",
      db->atx->tid,
      name
  );

  // GET VARIABLE
  {
    gparams = (struct ns_var_get_params){
        .p     = db->root->p,
        .tx    = db->atx,
        .vname = vname,
        .alloc = &ret->alloc,
    };
    err_t err = ns_var_get (&gparams, e);
    if (err == ERR_VARIABLE_NE)
    {
      e->cause_code = SUCCESS;
      e->cmlen      = 0;
      i_free (ret);
      *dest = NULL;
      goto commit;
    }
    WRAP_GOTO (err, failed_rollback);

    ret->var = gparams.dest;
    *dest    = ret;
  }

commit:
  // COMMIT
  WRAP_GOTO (nsh_auto_commit (db, e), failed_rollback);

  return SUCCESS;

failed_rollback:

  nsh_auto_rollback (db);

failed:
  chunk_alloc_free_all (&ret->alloc);
  i_free (ret);
  return error_trace (e);
}

int
nsdb_get_if_exists (nsdb_t *_smf, nsdb_var_t **dest, const char *name)
{
  struct nshandle *smf = (struct nshandle *)_smf;

  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  return _nsdb_get_if_exists (smf, name, dest, &smf->e);
}

/////////////////////////////////////////////////////////////////////
////// Insert

static sb_size
_nsdb_insert (
    struct nshandle *db,
    nsdb_var_t      *var,
    const void      *src,
    sb_size          _ofst,
    const b_size     slen,
    error           *e
)
{
  sb_size                     ret;     // Return value
  b_size                      bofst;   // Resolved offset
  struct stream               _input;  // Input stream
  struct stream_ibuf_ctx      ctx;     // Context for input stream
  struct chunk_alloc          temp;    // Allocator for get operation
  struct ns_var_get_params    gparams; // Get or create operation
  struct ns_insert_params     iparams; // Insert operation
  struct ns_var_update_params uparams; // Update operation

  chunk_alloc_create_default (&temp);

  // Parameter validation
  if (slen == 0)
  {
    return 0;
  }

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
  t_size tsize = type_byte_size (gparams.dest.dtype);
  stream_ibuf_init (&_input, &ctx, src, slen * tsize);
  bofst = var_resolve_index (&gparams.dest, tsize * _ofst);

  i_log_debug (
      "INSERT (txn = %" PRtxid
      ")"
      " - %.*s"
      " size (bytes): %" PRt_size " curlen: %" PRb_size
      " curlen (bytes): %" PRb_size
      " Requested: "
      " ofst: %" PRId64 " ofst (bytes): %" PRId64 " nelem: %" PRId64
      " nbytes (bytes): %" PRId64
      " Granted: "
      " start: %" PRIu64 " start (bytes): %" PRIu64 " granted: %" PRIu64
      " granted (bytes): %" PRIu64 "\n",
      db->atx->tid,
      strfmt (&var->var.vname),
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
    if (ret != (sb_size)(slen * tsize))
    {
      goto failed_rollback;
    }
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
nsdb_insert (
    nsdb_t     *_smf,
    nsdb_var_t *var,
    const void *src,
    sb_size     bofst,
    b_size      slen
)
{
  struct nshandle *smf = (struct nshandle *)_smf;

  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  return _nsdb_insert (smf, var, src, bofst, slen, &smf->e);
}

/////////////////////////////////////////////////////////////////////
////// Insert

static sb_size
_nsdb_len (struct nshandle *db, const char *name, error *e)
{
  struct chunk_alloc temp;
  chunk_alloc_create_default (&temp);
  struct string            vname = strfcstr (name);
  struct ns_var_get_params gparams;
  b_size                   len;
  t_size                   tsize;

  // BEGIN TXN
  if (nsh_auto_begin_txn (db, e) < 0)
  {
    goto failed;
  }

  i_log_debug ("LEN (txn = %" PRtxid "): %s\n", db->atx->tid, name);

  // GET OR CREATE VARIABLE
  {
    gparams = (struct ns_var_get_params){
        .p     = db->root->p,
        .tx    = db->atx,
        .vname = vname,
        .alloc = &temp,
    };
    if (ns_var_get (&gparams, e))
    {
      goto failed_rollback;
    }
  }

  // Resolve length
  {
    tsize = type_byte_size (gparams.dest.dtype);
    len   = gparams.dest.nbytes;

    if (len % tsize != 0)
    {
      error_causef (e, ERR_CORRUPT, "Variable: %s has invalid byte size", name);
      goto failed_rollback;
    }

    len /= tsize;
  }

  // COMMIT
  if (nsh_auto_commit (db, e) < 0)
  {
    goto failed_rollback;
  }

  chunk_alloc_free_all (&temp);

  return len;

failed_rollback:

  nsh_auto_rollback (db);

failed:
  chunk_alloc_free_all (&temp);
  return error_trace (e);
}

sb_size
nsdb_len (nsdb_t *_smf, const char *name)
{
  struct nshandle *smf = (struct nshandle *)_smf;

  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  return _nsdb_len (smf, name, &smf->e);
}

/////////////////////////////////////////////////////////////////////
////// Read

static sb_size
_nsdb_read (
    struct nshandle   *db,
    nsdb_var_t        *var,
    void              *dest,
    struct user_stride ustr,
    error             *e
)
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

    if (stride_resolve (&stride, ustr, len, e))
    {
      goto failed_rollback;
    }

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
      " size (bytes): %" PRt_size " curlen: %" PRb_size
      " curlen (bytes): %" PRb_size
      " Requested: "
      " start: %" PRId64 " stride: %" PRId64 " stop: %" PRId64
      " start (bytes): %" PRId64 " stride (bytes): %" PRId64
      " stop (bytes): %" PRId64
      " Granted: "
      " start: %" PRIu64 " stride: %" PRIu64 " nelems: %" PRIu64
      " start (bytes): %" PRIu64 " stride (bytes): %" PRIu64
      " nelems (bytes): %" PRIu64 "\n",
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

/////////////////////////////////////////////////////////////////////
////// Remove

static sb_size
_nsdb_premove (
    struct nshandle   *db,
    nsdb_var_t        *var,
    void              *dest,
    struct user_stride ustr,
    error             *e
)
{
  sb_size                     ret;           // Return value
  b_size                      ofst;          // Resolved offset
  t_size                      tsize;         // Size of  the variable
  b_size                      len;           // Length of the variable
  struct stream               _output;       // Output stream if present
  struct stream_obuf_ctx      ctx;           // Context for output stream
  struct stream              *output = NULL; // Pointer to output stream
  struct chunk_alloc          temp;          // Allocator for get operation
  struct ns_var_get_params    gparams;       // Get operation
  struct ns_remove_params     rparams;       // Remove operation
  struct ns_var_update_params uparams;       // Update operation
  struct stride               stride;        // Resolved stride

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

    if (stride_resolve (&stride, ustr, len, e))
    {
      goto failed_rollback;
    }

    if (dest)
    {
      stream_obuf_init (&_output, &ctx, dest, stride.nelems * len);
      output = &_output;
    }
  }

  i_log_debug (
      "REMOVE (txn = %" PRtxid
      ")"
      " - %.*s"
      " size (bytes): %" PRt_size " curlen: %" PRb_size
      " curlen (bytes): %" PRb_size
      " Requested: "
      " start: %" PRId64 " stride: %" PRId64 " stop: %" PRId64
      " start (bytes): %" PRId64 " stride (bytes): %" PRId64
      " stop (bytes): %" PRId64
      " Granted: "
      " start: %" PRIu64 " stride: %" PRIu64 " nelems: %" PRIu64
      " start (bytes): %" PRIu64 " stride (bytes): %" PRIu64
      " nelems (bytes): %" PRIu64 "\n",
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

  return _nsdb_premove (smf, var, dest, stride, &smf->e);
}

/////////////////////////////////////////////////////////////////////
////// Write

static sb_size
_nsdb_pwrite (
    struct nshandle   *db,
    nsdb_var_t        *var,
    const void        *src,
    struct user_stride ustr,
    error             *e
)
{
  sb_size                  ret;     // Return value
  t_size                   tsize;   // Size of  the variable
  b_size                   len;     // Length of the variable
  b_size                   ofst;    // Resolved offset
  struct stream            _input;  // Input stream
  struct stream_ibuf_ctx   ctx;     // Context for input stream
  struct chunk_alloc       temp;    // Allocator for get operation
  struct ns_var_get_params gparams; // Get or create operation
  struct ns_write_params   wparams; // Write operation
  struct stride            stride;  // Resolved stride

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

    if (stride_resolve (&stride, ustr, len, e))
    {
      goto failed_rollback;
    }

    stream_ibuf_init (&_input, &ctx, src, stride.nelems * tsize);
  }

  i_log_debug (
      "WRITE (txn = %" PRtxid
      ")"
      " - %.*s"
      " size (bytes): %" PRt_size " curlen: %" PRb_size
      " curlen (bytes): %" PRb_size
      " Requested: "
      " start: %" PRId64 " stride: %" PRId64 " stop: %" PRId64
      " start (bytes): %" PRId64 " stride (bytes): %" PRId64
      " stop (bytes): %" PRId64
      " Granted: "
      " start: %" PRIu64 " stride: %" PRIu64 " nelems: %" PRIu64
      " start (bytes): %" PRIu64 " stride (bytes): %" PRIu64
      " nelems (bytes): %" PRIu64 "\n",
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

  // WRITE
  {
    wparams = (struct ns_write_params){
        .p      = db->root->p,
        .src    = &_input,
        .tx     = db->atx,
        .root   = gparams.dest.rpt_root,
        .size   = tsize,
        .bofst  = tsize * stride.start,
        .stride = stride.stride,
        .nelem  = stride.nelems,
    };
    ret = ns_write (wparams, e);
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
nsdb_write (
    nsdb_t     *_smf,
    nsdb_var_t *var,
    const void *src,
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

  return _nsdb_pwrite (smf, var, src, stride, &smf->e);
}
