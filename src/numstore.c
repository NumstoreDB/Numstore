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
#include "query.h"
#include "rope_algorithms.h"
#include "serial.h"
#include "types.h"
#include "var_algorithms.h"

/******************************************************************************
 * SECTION: Simple Functions
 ******************************************************************************/

nsdb_t *
nsdb_open (const char *path)
{
  struct nsdb *ret = nsh_open (path);

  if (ret == NULL)
  {
    return NULL;
  }

  return (nsdb_t *)ret;
}

int
nsdb_perror (nsdb_t *ns, const char *prefix)
{
  return nsh_perror ((struct nsdb *)ns, prefix);
}

const char *
nsdb_strerror (nsdb_t *ns)
{
  return nsh_strerror ((struct nsdb *)ns);
}

int
nsdb_cleanup (const char *path)
{
  return nsh_cleanup (path);
}

int
nsdb_close (nsdb_t *ns)
{
  return nsh_close ((struct nsdb *)ns);
}

int
nsdb_crash (nsdb_t *ns)
{
  return nsh_crash ((struct nsdb *)ns);
}

b_size
nsdb_var_len (nsdb_var_t *var)
{
  return var->var->nbytes / type_byte_size (var->var->dtype);
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
  int ret = nsh_begin ((struct nsdb *)_smf);
  if (ret == 0)
  {
    i_log_debug ("a BEGIN TXN: %" PRtxid "\n", ((struct nsdb *)_smf)->atx->tid);
  }
  return ret;
}

int
nsdb_commit (nsdb_t *_smf)
{
  i_log_debug ("COMMIT: %" PRtxid "\n", ((struct nsdb *)_smf)->atx->tid);
  return nsh_commit ((struct nsdb *)_smf);
}

int
nsdb_rollback (nsdb_t *smf)
{
  i_log_debug ("ROLLBACK: %" PRtxid "\n", ((struct nsdb *)smf)->atx->tid);
  return nsh_rollback ((struct nsdb *)smf);
}

/******************************************************************************
 * SECTION: nsdb_get_if_exists
 ******************************************************************************/

static err_t
nsdb_get (
    struct nsdb        *db,
    struct get_query   *query,
    struct chunk_alloc *alloc,
    struct variable   **dest,
    error              *e
)
{
  ASSERT (dest);
  struct ns_var_get_params gparams; // Get or create operation

  *dest = chunk_malloc (alloc, 1, sizeof (struct variable), e);
  if (*dest == NULL)
  {
    return error_trace (e);
  }

  // BEGIN TXN
  WRAP_GOTO (nsh_auto_begin_txn (db, e), failed);

  i_log_debug (
      "GET (txn = %" PRtxid
      ")"
      " - %.*s\n",
      db->atx->tid,
      strfmt (&query->name)
  );

  // GET VARIABLE
  {
    gparams = (struct ns_var_get_params){
        .p     = db->root->p,
        .tx    = db->atx,
        .vname = query->name,
        .alloc = alloc,
    };
    err_t err = ns_var_get (&gparams, e);
    if (query->if_exists && err == ERR_VARIABLE_NE)
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
    struct nsdb        *db,
    struct chunk_alloc *alloc,
    struct string       vname,
    struct type         dtype,
    error              *e
)
{
  struct ns_var_get_or_create_params gparams; // Get or create operation

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
nsdb_delete (struct nsdb *db, struct string vname, error *e)
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
    struct nsdb        *db,
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
    struct nsdb         *db,
    struct insert_query *query,
    struct chunk_alloc  *alloc,
    struct stream       *src,
    error               *e
)
{
  sb_size                     ret;     // Return value
  b_size                      bofst;   // Resolved offset
  struct ns_var_get_params    gparams; // Get or create operation
  struct ns_insert_params     iparams; // Insert operation
  struct ns_var_update_params uparams; // Update operation

  // Parameter validation
  if (query->len == 0)
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
        .vname = query->name,
        .alloc = alloc,
    };
    WRAP_GOTO (ns_var_get (&gparams, e), failed_rollback);
  }

  // Resolve sizes
  t_size tsize = type_byte_size (gparams.dest.dtype);
  bofst        = var_resolve_index (&gparams.dest, tsize * query->ofst);

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
      strfmt (&query->name),
      tsize,
      gparams.dest.nbytes / tsize,
      gparams.dest.nbytes,
      query->ofst,
      query->ofst * tsize,
      query->len,
      query->len * tsize,
      bofst / tsize,
      bofst,
      query->len,
      query->len * tsize
  );

  // INSERT
  {
    iparams = (struct ns_insert_params){
        .p     = db->root->p,
        .src   = src,
        .tx    = db->atx,
        .root  = gparams.dest.rpt_root,
        .bofst = bofst,
        .nelem = query->len,
    };
    ret = ns_insert (&iparams, e);
    if (ret != (sb_size)(query->len * tsize))
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
    struct nsdb        *db,    // The database handle
    struct read_query  *query, // The query that got parsed
    struct chunk_alloc *alloc, // Where to allocate stuff
    struct stream      *dest,  // destination stream
    error              *e      // Error pointer
)
{
  sb_size                  ret;     // Return value
  t_size                   tsize;   // Size of  the variable
  b_size                   len;     // Length of the variable
  struct ns_var_get_params gparams; // Get operation
  struct ns_read_params    rparams; // Read operation
  struct stride            stride;  // Resolved stride

  // BEGIN TXN
  WRAP_GOTO (nsh_auto_begin_txn (db, e), failed);

  // GET VARIABLE
  {
    gparams = (struct ns_var_get_params){
        .p     = db->root->p,
        .tx    = db->atx,
        .vname = query->name,
        .alloc = alloc,
    };
    WRAP_GOTO (ns_var_get (&gparams, e), failed_rollback);
  }

  // Resolve sizes
  {
    // Size of each variable
    tsize = type_byte_size (gparams.dest.dtype);

    // Total size in bytes of the variable
    len = gparams.dest.nbytes;

    // A consistent database has this be a multiple of tsize
    if (len % tsize != 0)
    {
      error_causef (
          e,
          ERR_CORRUPT,
          "Variable: %.*s has invalid byte size",
          strfmt (&query->name)
      );
      goto failed_rollback;
    }
    len /= tsize;

    // Resolve length based on the stride
    if (stride_resolve (&stride, query->ustr, len, e))
    {
      goto failed_rollback;
    }

    // Check limit
    if (query->limit > 0)
    {
      if (query->blimit)
      {
        stride.nelems = query->limit / tsize;
      }
      else
      {
        stride.nelems = query->limit;
      }
    }
    else
    {
      ASSERT (!query->blimit);
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
      strfmt (&query->name),
      tsize,
      len,
      gparams.dest.nbytes,
      query->ustr.present & START_PRESENT ? query->ustr.start : 0,
      query->ustr.present & STEP_PRESENT ? query->ustr.step : 0,
      query->ustr.present & STOP_PRESENT ? query->ustr.stop : 0,
      query->ustr.present & START_PRESENT ? tsize * query->ustr.start : 0,
      query->ustr.present & STEP_PRESENT ? tsize * query->ustr.step : 0,
      query->ustr.present & STOP_PRESENT ? tsize * query->ustr.stop : 0,
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
        .dest   = dest,
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
    struct nsdb         *db,
    struct remove_query *query,
    struct chunk_alloc  *alloc,
    struct stream       *dest,
    error               *e
)
{
  sb_size                     ret;     // Return value
  b_size                      ofst;    // Resolved offset
  t_size                      tsize;   // Size of  the variable
  b_size                      len;     // Length of the variable
  struct ns_var_get_params    gparams; // Get operation
  struct ns_remove_params     rparams; // Remove operation
  struct ns_var_update_params uparams; // Update operation
  struct stride               stride;  // Resolved stride

  // BEGIN TXN
  WRAP_GOTO (nsh_auto_begin_txn (db, e), failed);

  // GET VARIABLE
  {
    gparams = (struct ns_var_get_params){
        .p     = db->root->p,
        .tx    = db->atx,
        .vname = query->name,
        .alloc = alloc,
    };
    WRAP_GOTO (ns_var_get (&gparams, e), failed_rollback);
  }

  // Resolve sizes
  {
    // Size of each variable
    tsize = type_byte_size (gparams.dest.dtype);

    // Total size in bytes of the variable
    len = gparams.dest.nbytes;

    // A consistent database has this be a multiple of tsize
    if (len % tsize != 0)
    {
      error_causef (
          e,
          ERR_CORRUPT,
          "Variable: %.*s has invalid byte size",
          strfmt (&query->name)
      );
      goto failed_rollback;
    }
    len /= tsize;

    // Resolve length based on the stride
    if (stride_resolve (&stride, query->ustr, len, e))
    {
      goto failed_rollback;
    }

    // Check limit
    if (query->limit > 0)
    {
      if (query->blimit)
      {
        stride.nelems = query->limit / tsize;
      }
      else
      {
        stride.nelems = query->limit;
      }
    }
    else
    {
      ASSERT (!query->blimit);
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
      strfmt (&query->name),
      tsize,
      len,
      gparams.dest.nbytes,
      query->ustr.present & START_PRESENT ? query->ustr.start : 0,
      query->ustr.present & STEP_PRESENT ? query->ustr.step : 0,
      query->ustr.present & STOP_PRESENT ? query->ustr.stop : 0,
      query->ustr.present & START_PRESENT ? tsize * query->ustr.start : 0,
      query->ustr.present & STEP_PRESENT ? tsize * query->ustr.step : 0,
      query->ustr.present & STOP_PRESENT ? tsize * query->ustr.stop : 0,
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
        .dest   = dest,
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
    struct nsdb        *db,
    struct write_query *query,
    struct chunk_alloc *alloc,
    struct stream      *src,
    error              *e
)
{
  sb_size                  ret;     // Return value
  t_size                   tsize;   // Size of  the variable
  b_size                   len;     // Length of the variable
  b_size                   ofst;    // Resolved offset
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
        .vname = query->name,
        .alloc = alloc,
    };
    WRAP_GOTO (ns_var_get (&gparams, e), failed_rollback);
  }

  // Resolve sizes
  {
    // Size of each variable
    tsize = type_byte_size (gparams.dest.dtype);

    // Total size in bytes of the variable
    len = gparams.dest.nbytes;

    // A consistent database has this be a multiple of tsize
    if (len % tsize != 0)
    {
      error_causef (
          e,
          ERR_CORRUPT,
          "Variable: %.*s has invalid byte size",
          strfmt (&query->name)
      );
      goto failed_rollback;
    }
    len /= tsize;

    // Resolve length based on the stride
    if (stride_resolve (&stride, query->ustr, len, e))
    {
      goto failed_rollback;
    }

    // Check limit
    if (query->limit > 0)
    {
      if (query->blimit)
      {
        // byte limit
        stride.nelems = query->limit / tsize;
      }
      else
      {
        // element limit
        stride.nelems = query->limit;
      }
    }
    else
    {
      ASSERT (!query->blimit);
    }
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
      strfmt (&query->name),
      tsize,
      len,
      gparams.dest.nbytes,
      query->ustr.present & START_PRESENT ? query->ustr.start : 0,
      query->ustr.present & STEP_PRESENT ? query->ustr.step : 0,
      query->ustr.present & STOP_PRESENT ? query->ustr.stop : 0,
      query->ustr.present & START_PRESENT ? tsize * query->ustr.start : 0,
      query->ustr.present & STEP_PRESENT ? tsize * query->ustr.step : 0,
      query->ustr.present & STOP_PRESENT ? tsize * query->ustr.stop : 0,
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
        .src    = src,
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
 * SECTION: nsdb_execute in console
 ******************************************************************************/

sb_size
nsdb_execute_in_console (
    struct nsdb        *ns,
    struct query       *q,
    struct chunk_alloc *alc,
    error              *e
)
{
  return 0;
}

/******************************************************************************
 * SECTION: nsdb_execute on buffer
 ******************************************************************************/

sb_size
nsdb_execute_on_buffer (
    struct nsdb        *ns,
    struct query       *q,
    void               *data,
    struct chunk_alloc *alc,
    error              *e
)
{
  sb_size                ret;
  struct variable       *var;
  struct stream          stream;
  struct stream_obuf_ctx octx;
  struct stream_ibuf_ctx ictx;

  switch (q->type)
  {
    case QT_READ:
    {
      // Destination pointer is required
      if (data == NULL)
      {
        error_causef (
            e,
            ERR_INVALID_ARGUMENT,
            "data is required for a get operation"
        );
        goto failed;
      }

      if (q->read.limit && q->read.blimit)
      {
        stream_obuf_init (&stream, &octx, data, q->read.limit);
      }
      else
      {
        stream_obuf_init (&stream, &octx, data, 0);
      }
      ret = nsdb_read (ns, &q->read, alc, &stream, e);
      if (ret < 0)
      {
        goto failed;
      }

      break;
    }
    case QT_WRITE:
    {
      // Source pointer is required
      if (data == NULL)
      {
        error_causef (
            e,
            ERR_INVALID_ARGUMENT,
            "data is required for a get operation"
        );
        goto failed;
      }

      if (q->write.limit && q->write.blimit)
      {
        stream_ibuf_init (&stream, &ictx, data, q->write.limit);
      }
      else
      {
        stream_ibuf_init (&stream, &ictx, data, 0);
      }
      ret = nsdb_write (ns, &q->write, alc, &stream, e);
      if (ret < 0)
      {
        goto failed;
      }

      break;
    }
    case QT_REMOVE:
    {
      if (data)
      {
        if (q->remove.limit && q->remove.blimit)
        {
          stream_obuf_init (&stream, &octx, data, q->remove.limit);
        }
        else
        {
          stream_obuf_init (&stream, &octx, data, 0);
        }

        ret = nsdb_remove (ns, &q->remove, alc, &stream, e);
      }
      else
      {
        ret = nsdb_remove (ns, &q->remove, alc, NULL, e);
      }
      if (ret < 0)
      {
        goto failed;
      }

      break;
    }
    case QT_INSERT:
    {
      // Source pointer is required
      if (data == NULL)
      {
        error_causef (
            e,
            ERR_INVALID_ARGUMENT,
            "data is required for a get operation"
        );
        goto failed;
      }

      stream_ibuf_init (&stream, &ictx, data, 0);
      ret = nsdb_insert (ns, &q->insert, alc, &stream, e);
      if (ret < 0)
      {
        goto failed;
      }

      break;
    }

    case QT_CREATE:
    {
      if (nsdb_create (ns, alc, q->create.name, q->create.type, e))
      {
        goto failed;
      }

      ret = SUCCESS;

      break;
    }
    case QT_DELETE:
    {
      if (nsdb_delete (ns, q->delete.name, e))
      {
        goto failed;
      }

      ret = SUCCESS;

      break;
    }
    case QT_GET:
    {
      // Destination pointer is required
      if (data == NULL)
      {
        error_causef (
            e,
            ERR_INVALID_ARGUMENT,
            "data is required for a get operation"
        );
        goto failed;
      }

      // Variables get their own chunk allocator
      // context that gets freed on nsdb_var_free
      struct chunk_alloc *valloc = i_malloc (1, sizeof *valloc, e);
      if (valloc == NULL)
      {
        goto failed;
      }
      chunk_alloc_create_default (valloc);

      // Get the variable
      if (nsdb_get (ns, &q->get, valloc, &var, e) < 0)
      {
        chunk_alloc_free_all (valloc);
        i_free (valloc);
        goto failed;
      }

      // Transfer over to a variable handle (that can be free'd)
      struct nsdb_var **dest = (struct nsdb_var **)data;
      *dest = chunk_malloc (valloc, 1, sizeof (struct nsdb_var), e);

      if (*dest == NULL)
      {
        chunk_alloc_free_all (valloc);
        i_free (valloc);
        goto failed;
      }

      (*dest)->var   = var;
      (*dest)->alloc = valloc;

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

  return ret;

failed:

  return error_trace (e);
}

/******************************************************************************
 * SECTION: Library exposed nsdb_execute
 ******************************************************************************/

sb_size
nsdb_execute (nsdb_t *nh, const char *query, void *data, ...)
{
  struct chunk_alloc alloc;          // Memory allocation context
  sb_size            ret;            // return variable
  char               stackbuf[2048]; // Stack buffer if the query fits
  char              *buf = stackbuf; // Pointer to the buffer
  va_list            ap, ap2;        // Argument list
  i32                qlen;           // Length of the query
  struct query       q;              // The AST

  // Reset errors before proceeding
  nh->e.cause_code = 0;
  nh->e.cmlen      = 0;

  chunk_alloc_create_default (&alloc);

  // A small stack buffer - if the query doesn't fit into
  // this buffer - we'll need to malloc

  va_start (ap, data);
  va_copy (ap2, ap);

  qlen = vsnprintf (stackbuf, sizeof stackbuf, query, ap);
  va_end (ap);

  if (qlen < 0)
  {
    va_end (ap2);
    ret =
        error_causef (&nh->e, ERR_INVALID_ARGUMENT, "Invalid printf argument");
    goto theend;
  }

  if ((size_t)qlen >= sizeof stackbuf)
  {
    buf = chunk_malloc (&alloc, qlen + 1, 1, &nh->e);
    if (!buf)
    {
      va_end (ap2);
      ret = error_trace (&nh->e);
      goto theend;
    }
    qlen = vsnprintf (buf, qlen + 1, query, ap2);
    ASSERT (qlen >= 0);
  }
  va_end (ap2);

  // Compile the query
  if (compile_query (&q, buf, &alloc, &nh->e))
  {
    ret = error_trace (&nh->e);
    goto theend;
  }

  ret = nsdb_execute_on_buffer (nh, &q, data, &alloc, &nh->e);

  if (buf != stackbuf)
  {
    i_free (buf);
  }

theend:
  chunk_alloc_free_all (&alloc);
  return ret;
}
