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
#include "csx_assert.h"
#include "error.h"
#include "logging.h"
#include "nshandle.h"
#include "os.h"
#include "rope_algorithms.h"
#include "types.h"
#include "var_algorithms.h"

/******************************************************************************
 * SECTION: Simple Functions
 ******************************************************************************/

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

b_size
nsdb_var_len (nsdb_var_t *var)
{
  return var->var.nbytes / type_byte_size (var->var.dtype);
}

void
nsdb_var_free (nsdb_var_t *var)
{
  struct chunk_alloc *alloc = var->alloc;
  chunk_alloc_free_all (alloc);
  i_free (alloc);
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

/******************************************************************************
 * SECTION: nsdb_get
 ******************************************************************************/

static struct variable *
nsdb_get (
    struct nshandle    *db,
    struct chunk_alloc *alloc,
    struct string       vname,
    error              *e
)
{
  struct variable         *ret = NULL; // Return value
  struct ns_var_get_params gparams;    // Get or create operation

  ret = chunk_malloc (alloc, 1, sizeof *ret, e);
  if (ret == NULL)
  {
    return NULL;
  }

  // BEGIN TXN
  WRAP_GOTO (nsh_auto_begin_txn (db, e), failed);

  i_log_debug (
      "GET (txn = %" PRtxid
      ")"
      " - %.*s\n",
      db->atx->tid,
      strfmt (&vname)
  );

  // GET VARIABLE
  {
    gparams = (struct ns_var_get_params){
        .p     = db->root->p,
        .tx    = db->atx,
        .vname = vname,
        .alloc = alloc,
    };
    err_t err = ns_var_get (&gparams, e);
    WRAP_GOTO (err, failed_rollback);

    *ret = gparams.dest;
  }

  // COMMIT
  WRAP_GOTO (nsh_auto_commit (db, e), failed_rollback);

  return ret;

failed_rollback:

  nsh_auto_rollback (db);

failed:
  i_free (ret);
  return NULL;
}

/******************************************************************************
 * SECTION: nsdb_get_if_exists
 ******************************************************************************/

HEADER_FUNC int
nsdb_get_if_exists (
    struct nshandle    *db,
    struct chunk_alloc *alloc,
    const char         *name,
    struct variable   **dest,
    error              *e
)
{
  ASSERT (dest);
  struct ns_var_get_params gparams;                 // Get or create operation
  struct string            vname = strfcstr (name); // Variable name

  *dest = chunk_malloc (alloc, 1, sizeof (struct variable), e);
  if (*dest == NULL)
  {
    return error_trace (e);
  }

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
        .alloc = alloc,
    };
    err_t err = ns_var_get (&gparams, e);
    if (err == ERR_VARIABLE_NE)
    {
      e->cause_code = SUCCESS;
      e->cmlen      = 0;
      *dest         = NULL;
      goto commit;
    }
    WRAP_GOTO (err, failed_rollback);

    *(*dest) = gparams.dest;
  }

commit:
  // COMMIT
  WRAP_GOTO (nsh_auto_commit (db, e), failed_rollback);

  return SUCCESS;

failed_rollback:

  nsh_auto_rollback (db);

failed:
  return error_trace (e);
}

/******************************************************************************
 * SECTION: nsdb_create
 ******************************************************************************/

static int
nsdb_create (
    struct nshandle    *db,
    struct chunk_alloc *alloc,
    struct string       vname,
    struct type         dtype,
    error              *e
)
{
  struct ns_var_get_or_create_params gparams; // Get or create operation

  chunk_alloc_create_default (alloc);

  // BEGIN TXN
  WRAP_GOTO (nsh_auto_begin_txn (db, e), failed);

  i_log_debug (
      "CREATE (txn = %" PRtxid "): %.*s\n",
      db->atx->tid,
      strfmt (&vname)
  );

  // GET OR CREATE VARIABLE
  {
    gparams = (struct ns_var_get_or_create_params){
        .p     = db->root->p,
        .tx    = db->atx,
        .vname = vname,
        .type  = &dtype,
        .alloc = alloc,
    };
    WRAP_GOTO (ns_var_get_or_create (&gparams, e), failed_rollback);
  }

  // COMMIT
  WRAP_GOTO (nsh_auto_commit (db, e), failed_rollback);
  chunk_alloc_free_all (alloc);

  return SUCCESS;

failed_rollback:

  nsh_auto_rollback (db);

failed:
  chunk_alloc_free_all (alloc);
  return error_trace (e);
}

/******************************************************************************
 * SECTION: nsdb_delete
 ******************************************************************************/

static err_t
nsdb_delete (struct nshandle *db, struct string vname, error *e)
{
  struct txn auto_txn;

  // BEGIN TXN
  WRAP_GOTO (nsh_auto_begin_txn (db, e), failed);

  i_log_debug (
      "DELETE (txn = %" PRtxid "): %.*s\n",
      db->atx->tid,
      strfmt (&vname)
  );

  {
    // DELETE
    struct ns_var_delete_params params = {
        .p     = db->root->p,
        .tx    = db->atx,
        .vname = vname,
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

/******************************************************************************
 * SECTION: nsdb_len
 ******************************************************************************/

HEADER_FUNC sb_size
nsdb_len (
    struct nshandle    *db,
    struct chunk_alloc *alloc,
    const char         *name,
    error              *e
)
{
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
        .alloc = alloc,
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

  return len;

failed_rollback:

  nsh_auto_rollback (db);

failed:
  return error_trace (e);
}

/******************************************************************************
 * SECTION: nsdb_insert
 ******************************************************************************/

static sb_size
nsdb_insert (
    struct nshandle    *db,
    struct variable    *var,
    struct chunk_alloc *alloc,
    const void         *src,
    sb_size             _ofst,
    const b_size        slen,
    error              *e
)
{
  sb_size                     ret;     // Return value
  b_size                      bofst;   // Resolved offset
  struct stream               _input;  // Input stream
  struct stream_ibuf_ctx      ctx;     // Context for input stream
  struct ns_var_get_params    gparams; // Get or create operation
  struct ns_insert_params     iparams; // Insert operation
  struct ns_var_update_params uparams; // Update operation

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
        .vname = var->vname,
        .alloc = alloc,
    };
    WRAP_GOTO (ns_var_get (&gparams, e), failed_rollback);

    // Type check
    if (!type_equal (var->dtype, gparams.dest.dtype))
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
      strfmt (&var->vname),
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
  return ret;

failed_rollback:

  nsh_auto_rollback (db);

failed:
  return error_trace (e);
}

/******************************************************************************
 * SECTION: nsdb_read
 ******************************************************************************/

static sb_size
nsdb_read (
    struct nshandle    *db,
    struct variable    *var,
    struct chunk_alloc *alloc,
    void               *dest,
    struct user_stride  ustr,
    error              *e
)
{
  sb_size                  ret;           // Return value
  t_size                   tsize;         // Size of  the variable
  b_size                   len;           // Length of the variable
  struct stream            _output;       // Output stream if present
  struct stream_obuf_ctx   ctx;           // Context for output stream
  struct stream           *output = NULL; // Pointer to output stream
  struct ns_var_get_params gparams;       // Get operation
  struct ns_read_params    rparams;       // Read operation
  struct stride            stride;        // Resolved stride

  // BEGIN TXN
  WRAP_GOTO (nsh_auto_begin_txn (db, e), failed);

  // GET VARIABLE
  {
    gparams = (struct ns_var_get_params){
        .p     = db->root->p,
        .tx    = db->atx,
        .vname = var->vname,
        .alloc = alloc,
    };
    WRAP_GOTO (ns_var_get (&gparams, e), failed_rollback);

    // Type check
    if (!type_equal (var->dtype, gparams.dest.dtype))
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
          strfmt (&var->vname)
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
      strfmt (&var->vname),
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
  return ret;

failed_rollback:

  nsh_auto_rollback (db);

failed:
  return error_trace (e);
}

/******************************************************************************
 * SECTION: nsdb_remove
 ******************************************************************************/

static sb_size
nsdb_remove (
    struct nshandle    *db,
    struct variable    *var,
    struct chunk_alloc *alloc,
    void               *dest,
    struct user_stride  ustr,
    error              *e
)
{
  sb_size                     ret;           // Return value
  b_size                      ofst;          // Resolved offset
  t_size                      tsize;         // Size of  the variable
  b_size                      len;           // Length of the variable
  struct stream               _output;       // Output stream if present
  struct stream_obuf_ctx      ctx;           // Context for output stream
  struct stream              *output = NULL; // Pointer to output stream
  struct ns_var_get_params    gparams;       // Get operation
  struct ns_remove_params     rparams;       // Remove operation
  struct ns_var_update_params uparams;       // Update operation
  struct stride               stride;        // Resolved stride

  // BEGIN TXN
  WRAP_GOTO (nsh_auto_begin_txn (db, e), failed);

  // GET VARIABLE
  {
    gparams = (struct ns_var_get_params){
        .p     = db->root->p,
        .tx    = db->atx,
        .vname = var->vname,
        .alloc = alloc,
    };
    WRAP_GOTO (ns_var_get (&gparams, e), failed_rollback);

    // Type check
    if (!type_equal (var->dtype, gparams.dest.dtype))
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
          strfmt (&var->vname)
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
      stream_obuf_init (&_output, &ctx, dest, tsize * stride.nelems * len);
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
      strfmt (&var->vname),
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
  return ret;

failed_rollback:

  nsh_auto_rollback (db);

failed:
  return error_trace (e);
}

/******************************************************************************
 * SECTION: nsdb_write
 ******************************************************************************/

static sb_size
nsdb_write (
    struct nshandle    *db,
    struct variable    *var,
    struct chunk_alloc *alloc,
    const void         *src,
    struct user_stride  ustr,
    error              *e
)
{
  sb_size                  ret;     // Return value
  t_size                   tsize;   // Size of  the variable
  b_size                   len;     // Length of the variable
  b_size                   ofst;    // Resolved offset
  struct stream            _input;  // Input stream
  struct stream_ibuf_ctx   ctx;     // Context for input stream
  struct ns_var_get_params gparams; // Get or create operation
  struct ns_write_params   wparams; // Write operation
  struct stride            stride;  // Resolved stride

  // BEGIN TXN
  WRAP_GOTO (nsh_auto_begin_txn (db, e), failed);

  // GET VARIABLE
  {
    gparams = (struct ns_var_get_params){
        .p     = db->root->p,
        .tx    = db->atx,
        .vname = var->vname,
        .alloc = alloc,
    };
    WRAP_GOTO (ns_var_get (&gparams, e), failed_rollback);

    // Type check
    if (!type_equal (var->dtype, gparams.dest.dtype))
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
          strfmt (&var->vname)
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
      strfmt (&var->vname),
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
  return ret;

failed_rollback:

  nsh_auto_rollback (db);

failed:
  return error_trace (e);
}

/******************************************************************************
 * SECTION: nsdb_execute
 ******************************************************************************/

static sb_size
_nsdb_execute (
    struct nshandle *ns,
    const char      *query,
    u32              len,
    void            *data,
    error           *e
)
{
  struct chunk_alloc alc;
  struct query       q;
  sb_size            ret;
  struct variable   *var;

  chunk_alloc_create_default (&alc);

  if (compile_query (&q, query, &alc, e))
  {
    goto failed;
  }

  switch (q.type)
  {
    case QT_READ:
    {
      var = nsdb_get (ns, &alc, q.read.name, e);
      if (var == NULL)
      {
        goto failed;
      }

      ret = nsdb_read (ns, var, &alc, data, q.read.ustr, e);
      if (ret < 0)
      {
        goto failed;
      }

      break;
    }
    case QT_WRITE:
    {
      var = nsdb_get (ns, &alc, q.write.name, e);
      if (var == NULL)
      {
        goto failed;
      }

      ret = nsdb_write (ns, var, &alc, data, q.write.ustr, e);
      if (ret < 0)
      {
        goto failed;
      }

      break;
    }
    case QT_REMOVE:
    {
      var = nsdb_get (ns, &alc, q.remove.name, e);
      if (var == NULL)
      {
        goto failed;
      }
      ret = nsdb_remove (ns, var, &alc, data, q.remove.ustr, e);
      if (ret < 0)
      {
        goto failed;
      }

      break;
    }
    case QT_INSERT:
    {
      var = nsdb_get (ns, &alc, q.insert.name, e);
      if (var == NULL)
      {
        goto failed;
      }
      ret = nsdb_insert (ns, var, &alc, data, q.insert.ofst, q.insert.len, e);
      if (ret < 0)
      {
        goto failed;
      }

      break;
    }

    case QT_CREATE:
    {
      if (nsdb_create (ns, &alc, q.create.name, q.create.type, e))
      {
        goto failed;
      }

      ret = SUCCESS;

      break;
    }
    case QT_DELETE:
    {
      if (nsdb_delete (ns, q.delete.name, e))
      {
        goto failed;
      }

      ret = SUCCESS;

      break;
    }
    case QT_GET:
    {
      struct chunk_alloc *valloc = i_malloc (1, sizeof *valloc, e);
      if (valloc == NULL)
      {
        goto failed;
      }

      var = nsdb_get (ns, valloc, q.get.name, e);
      if (var == NULL)
      {
        chunk_alloc_free_all (valloc);
        i_free (valloc);
        goto failed;
      }

      ASSERT (data);
      struct nsdb_var_t **dest = (struct nsdb_var_t **)data;
      *dest = chunk_malloc (valloc, 1, sizeof (struct nsdb_var), e);

      if (*dest == NULL)
      {
        chunk_alloc_free_all (valloc);
        i_free (valloc);
        goto failed;
      }

      ret = SUCCESS;

      break;
    }

    case QT_EXIT:
    {
      ret = SUCCESS;
      break;
    }

    case QT_HELP:
    {
      ret = SUCCESS;
      break;
    }
  }

  chunk_alloc_free_all (&alc);

  return ret;

failed:
  chunk_alloc_free_all (&alc);
  return error_trace (e);
}

sb_size
nsdb_execute (nsdb_t *ns, const char *query, void *data, ...)
{
  char    stackbuf[2048];
  char   *buf = stackbuf;
  va_list ap, ap2;

  va_start (ap, data);
  va_copy (ap2, ap);

  i32 n = vsnprintf (stackbuf, sizeof stackbuf, query, ap);
  va_end (ap);

  if (n < 0)
  {
    va_end (ap2);
    return -1;
  }

  if ((size_t)n >= sizeof stackbuf)
  {
    buf = malloc (n + 1);
    if (!buf)
    {
      va_end (ap2);
      return -1;
    }
    vsnprintf (buf, n + 1, query, ap2);
  }
  va_end (ap2);

  struct nshandle *nh  = (struct nshandle *)ns;
  sb_size          ret = _nsdb_execute (nh, buf, (u32)n, data, &nh->e);

  if (buf != stackbuf)
  {
    free (buf);
  }
  return ret;
}
