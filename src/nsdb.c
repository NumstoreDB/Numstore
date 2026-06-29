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

#include "nsdb.h"

#include "error.h"
#include "pager.h"
#include "rope_algorithms.h"
#include "var_algorithms.h"

struct nsdb *
nsdb_remove_and_open (const char *name, error *e)
{
  if (pgr_delete_single_file (name, e))
  {
    return NULL;
  }
  return nsdb_open (name);
}

err_t
nsdb_root_crash (struct nsdb_root *root, error *e)
{
  ASSERT (root->count == 0);
  err_t err = pgr_crash (root->p, e);
  i_free ((void *)root->path.data);
  i_free (root);
  return err;
}

int
nsdb_perror (struct nsdb *ns, const char *prefix)
{
  const char *err = nsdb_strerror (ns);
  if (err)
  {
    return fprintf (stderr, "%s: %s\n", prefix, nsdb_strerror (ns));
  }
  else
  {
    return fprintf (stderr, "%s: success\n", prefix);
  }
}

const char *
nsdb_strerror (struct nsdb *ns)
{
  if (ns->e.cause_code < 0)
  {
    return ns->e.cause_msg;
  }
  else
  {
    return NULL;
  }
}

int
nsdb_cleanup (const char *path)
{
  error e = error_create ();
  pgr_delete_single_file (path, &e);
  return error_trace (&e);
}

/******************************************************************************
 * SECTION: nsdb_root functions
 ******************************************************************************/

err_t
nsdb_root_close (struct nsdb_root *root, error *e)
{
  ASSERT (root->count == 0);
  err_t err = pgr_close (root->p, e);
  i_free ((void *)root->path.data);
  i_free (root);
  return err;
}

struct nsdb *
nsdb_root_load (struct nsdb_root *ns, error *e)
{
  struct nsdb *ret = i_malloc (1, sizeof *ret, e);
  if (ret == NULL)
  {
    return NULL;
  }

  ret->root        = ns;
  ret->is_auto_txn = 0;
  ret->atx         = NULL;
  ret->e           = error_create ();
  ns->count++;

  return ret;
}

void
nsdb_root_release (struct nsdb_root *root, struct nsdb *sm)
{
  ASSERT (root->count > 0);
  i_free (sm);
  root->count -= 1;
}

/******************************************************************************
 * SECTION: Auto transaction behavior
 ******************************************************************************/

err_t
nsdb_auto_begin_txn (struct nsdb *sm, error *e)
{
  if (sm->atx == NULL)
  {
    WRAP (pgr_begin_txn (&sm->tx, sm->root->p, e));
    sm->is_auto_txn = 1;
    sm->atx         = &sm->tx;
  }

  return SUCCESS;
}

err_t
nsdb_auto_commit (struct nsdb *sm, error *e)
{
  if (sm->is_auto_txn)
  {
    ASSERT (sm->atx);
    WRAP (pgr_commit (sm->root->p, sm->atx, e));
    sm->atx = NULL;
  }
  return SUCCESS;
}

void
nsdb_auto_rollback (struct nsdb *sm)
{
  if (pgr_rollback (sm->root->p, sm->atx, 0, &sm->e))
  {
    panic ("Failed to rollback");
  }
  sm->atx = NULL;
}

/******************************************************************************
 * SECTION: nsdb_begin
 * ----------------------------------------------------------------------------
 * @brief Begin a new transaction
 ******************************************************************************/

err_t
nsdb_begin (struct nsdb *smf)
{
  smf->e.cause_code = 0;
  smf->e.cmlen      = 0;

  if (smf->atx)
  {
    return error_causef (
        &smf->e,
        ERR_INVALID_ARGUMENT,
        "Can't start another transaction, already a part of an existing "
        "transaction: %" PRtxid ". Either commit or rollback first",
        smf->atx->tid
    );
  }

  if (pgr_begin_txn (&smf->tx, smf->root->p, &smf->e))
  {
    return error_trace (&smf->e);
  }

  smf->is_auto_txn = 0;
  smf->atx         = &smf->tx;

  return SUCCESS;
}

/******************************************************************************
 * SECTION: nsdb_close
 * ----------------------------------------------------------------------------
 * @brief Closes a database
 ******************************************************************************/

err_t
nsdb_close (struct nsdb *n)
{
  struct nsdb_root *root = n->root;
  nsdb_root_release (root, n);
  if (root->count == 0)
  {
    return nsdb_root_close (root, &root->e);
  }
  return SUCCESS;
}

/******************************************************************************
 * SECTION: nsdb_commit
 * ----------------------------------------------------------------------------
 * @brief Commits a transaction
 ******************************************************************************/

err_t
nsdb_commit (struct nsdb *smf)
{
  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  if (smf->atx == NULL)
  {
    return error_causef (
        &smf->e,
        ERR_INVALID_ARGUMENT,
        "Can't commit transaction, not a part of an existing transaction"
    );
  }

  if (pgr_commit (smf->root->p, smf->atx, &smf->e))
  {
    return error_trace (&smf->e);
  }

  smf->atx = NULL;

  return SUCCESS;
}

/******************************************************************************
 * SECTION: nsdb_rollback
 * ----------------------------------------------------------------------------
 * @brief Rolls back a transaction
 ******************************************************************************/

err_t
nsdb_rollback (struct nsdb *smf)
{
  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  if (smf->atx == NULL)
  {
    return error_causef (
        &smf->e,
        ERR_INVALID_ARGUMENT,
        "Can't rollback transaction, not a part of an existing transaction"
    );
  }

  if (pgr_rollback (smf->root->p, smf->atx, 0, &smf->e))
  {
    return error_trace (&smf->e);
  }

  smf->atx = NULL;

  return SUCCESS;
}

/******************************************************************************
 * SECTION: nsdb_crash
 * ----------------------------------------------------------------------------
 * @brief Simulate a database crash
 ******************************************************************************/

err_t
nsdb_crash (struct nsdb *n)
{
  n->e.cause_code = SUCCESS;
  n->e.cmlen      = 0;

  struct nsdb_root *root = n->root;

  err_t err = pgr_crash (root->p, &n->e);
  i_free ((void *)root->path.data);
  i_free (n);
  i_free (root);

  return err;
}

/******************************************************************************
 * SECTION: nsdb_open
 * ----------------------------------------------------------------------------
 * @brief Opens a new database
 ******************************************************************************/

struct nsdb *
nsdb_open (const char *path)
{
  error e = error_create ();

  struct nsdb_root *ret = i_malloc (1, sizeof *ret, &e);

  page_h hp = page_h_create ();

  if (ret == NULL)
  {
    return NULL;
  }

  // Initialize inner values
  {
    ret->e     = error_create ();
    ret->count = 0;

    // path
    ret->path.len  = strlen (path);
    ret->path.data = i_malloc (ret->path.len, 1, &e);
    if (ret->path.data == NULL)
    {
      goto failed;
    }

    // db
    ret->p = pgr_open (path, &e);
    if (ret->p == NULL)
    {
      goto failed;
    }
  }

  // Upfront initialization
  if (pgr_isnew (ret->p))
  {
    // Initialize the upfront hash page
    if (ns_init_var_hash_map (ret->p, &e))
    {
      goto failed;
    }
  }

  // Launch the checkpoint writer thread
  if (pgr_launch_checkpoint_thread (ret->p, 5000, &e))
  {
    goto failed;
  }

  // Load the default context
  struct nsdb *sret = nsdb_root_load (ret, &e);

  return sret;

failed:
  // TODO just delete the file
  i_free (ret);
  return NULL;
}

/******************************************************************************
 * SECTION: Variable stuff
 ******************************************************************************/

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

/******************************************************************************
 * SECTION: nsdb_get
 ******************************************************************************/

err_t
nsdb_get (
    struct nsdb        *db,
    struct get_query   *query,
    struct chunk_alloc *alloc,
    struct variable   **dest
)
{
  ASSERT (dest);
  struct ns_var_get_params gparams; // Get or create operation

  *dest = chunk_malloc (alloc, 1, sizeof (struct variable), &db->e);
  if (*dest == NULL)
  {
    return error_trace (&db->e);
  }

  // BEGIN TXN
  WRAP_GOTO (nsdb_auto_begin_txn (db, &db->e), failed);

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
    err_t err = ns_var_get (&gparams, &db->e);
    if (query->if_exists && err == ERR_VARIABLE_NE)
    {
      db->e.cause_code = SUCCESS;
      db->e.cmlen      = 0;
      *dest            = NULL;
      goto commit;
    }
    WRAP_GOTO (err, failed_rollback);

    *(*dest) = gparams.dest;
  }

commit:
  // COMMIT
  WRAP_GOTO (nsdb_auto_commit (db, &db->e), failed_rollback);

  return SUCCESS;

failed_rollback:

  nsdb_auto_rollback (db);

failed:
  return error_trace (&db->e);
}

/******************************************************************************
 * SECTION: nsdb_create
 ******************************************************************************/

int
nsdb_create (
    struct nsdb        *db,
    struct chunk_alloc *alloc,
    struct string       vname,
    struct type         dtype
)
{
  struct ns_var_get_or_create_params gparams; // Get or create operation

  // BEGIN TXN
  WRAP_GOTO (nsdb_auto_begin_txn (db, &db->e), failed);

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
    WRAP_GOTO (ns_var_get_or_create (&gparams, &db->e), failed_rollback);
  }

  // COMMIT
  WRAP_GOTO (nsdb_auto_commit (db, &db->e), failed_rollback);
  chunk_alloc_free_all (alloc);

  return SUCCESS;

failed_rollback:

  nsdb_auto_rollback (db);

failed:
  chunk_alloc_free_all (alloc);
  return error_trace (&db->e);
}

/******************************************************************************
 * SECTION: nsdb_delete
 ******************************************************************************/

err_t
nsdb_delete (struct nsdb *db, struct string vname)
{
  struct txn auto_txn;

  // BEGIN TXN
  WRAP_GOTO (nsdb_auto_begin_txn (db, &db->e), failed);

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
    err_t err = ns_var_delete (params, &db->e);
    if (err < SUCCESS)
    {
      goto failed_rollback;
    }
  }

  if (nsdb_auto_commit (db, &db->e))
  {
    goto failed_rollback;
  }
  return error_trace (&db->e);

failed_rollback:

  nsdb_auto_rollback (db);

failed:
  return error_trace (&db->e);
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
  if (nsdb_auto_begin_txn (db, e) < 0)
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
  if (nsdb_auto_commit (db, e) < 0)
  {
    goto failed_rollback;
  }

  return len;

failed_rollback:

  nsdb_auto_rollback (db);

failed:
  return error_trace (e);
}

/******************************************************************************
 * SECTION: nsdb_insert
 ******************************************************************************/

sb_size
nsdb_insert (
    struct nsdb         *db,
    struct insert_query *query,
    struct chunk_alloc  *alloc,
    struct stream       *src
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
  WRAP_GOTO (nsdb_auto_begin_txn (db, &db->e), failed);

  // GET VARIABLE
  {
    gparams = (struct ns_var_get_params){
        .p     = db->root->p,
        .tx    = db->atx,
        .vname = query->name,
        .alloc = alloc,
    };
    WRAP_GOTO (ns_var_get (&gparams, &db->e), failed_rollback);
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
        .bytes = query->len * tsize,
    };
    ret = ns_insert (&iparams, &db->e);
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
    WRAP_GOTO (ns_var_update (uparams, &db->e), failed_rollback);
  }

  ASSERT (ret % tsize == 0);
  ret /= tsize;

  // COMMIT
  WRAP_GOTO (nsdb_auto_commit (db, &db->e), failed_rollback);
  return ret;

failed_rollback:

  nsdb_auto_rollback (db);

failed:
  return error_trace (&db->e);
}

/******************************************************************************
 * SECTION: nsdb_read
 ******************************************************************************/

sb_size
nsdb_read (
    struct nsdb        *db,    // The database handle
    struct read_query  *query, // The query that got parsed
    struct chunk_alloc *alloc, // Where to allocate stuff
    struct stream      *dest   // destination stream
)
{
  sb_size                  ret;     // Return value
  t_size                   tsize;   // Size of  the variable
  b_size                   len;     // Length of the variable
  struct ns_var_get_params gparams; // Get operation
  struct ns_read_params    rparams; // Read operation
  struct stride            stride;  // Resolved stride

  // BEGIN TXN
  WRAP_GOTO (nsdb_auto_begin_txn (db, &db->e), failed);

  // GET VARIABLE
  {
    gparams = (struct ns_var_get_params){
        .p     = db->root->p,
        .tx    = db->atx,
        .vname = query->name,
        .alloc = alloc,
    };
    WRAP_GOTO (ns_var_get (&gparams, &db->e), failed_rollback);
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
          &db->e,
          ERR_CORRUPT,
          "Variable: %.*s has invalid byte size",
          strfmt (&query->name)
      );
      goto failed_rollback;
    }
    len /= tsize;

    // Resolve length based on the stride
    if (stride_resolve (&stride, query->ustr, len, &db->e))
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
    ret = ns_read (rparams, &db->e);
    WRAP_GOTO (ret, failed_rollback);
  }

  // COMMIT
  WRAP_GOTO (nsdb_auto_commit (db, &db->e), failed_rollback);
  return ret;

failed_rollback:

  nsdb_auto_rollback (db);

failed:
  return error_trace (&db->e);
}

/******************************************************************************
 * SECTION: nsdb_remove
 ******************************************************************************/

sb_size
nsdb_remove (
    struct nsdb         *db,
    struct remove_query *query,
    struct chunk_alloc  *alloc,
    struct stream       *dest
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
  WRAP_GOTO (nsdb_auto_begin_txn (db, &db->e), failed);

  // GET VARIABLE
  {
    gparams = (struct ns_var_get_params){
        .p     = db->root->p,
        .tx    = db->atx,
        .vname = query->name,
        .alloc = alloc,
    };
    WRAP_GOTO (ns_var_get (&gparams, &db->e), failed_rollback);
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
          &db->e,
          ERR_CORRUPT,
          "Variable: %.*s has invalid byte size",
          strfmt (&query->name)
      );
      goto failed_rollback;
    }
    len /= tsize;

    // Resolve length based on the stride
    if (stride_resolve (&stride, query->ustr, len, &db->e))
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
    ret = ns_remove (&rparams, &db->e);
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
    WRAP_GOTO (ns_var_update (uparams, &db->e), failed_rollback);
  }

  // COMMIT
  WRAP_GOTO (nsdb_auto_commit (db, &db->e), failed_rollback);
  return ret;

failed_rollback:

  nsdb_auto_rollback (db);

failed:
  return error_trace (&db->e);
}

/******************************************************************************
 * SECTION: nsdb_write
 ******************************************************************************/

sb_size
nsdb_write (
    struct nsdb        *db,
    struct write_query *query,
    struct chunk_alloc *alloc,
    struct stream      *src
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
  WRAP_GOTO (nsdb_auto_begin_txn (db, &db->e), failed);

  // GET VARIABLE
  {
    gparams = (struct ns_var_get_params){
        .p     = db->root->p,
        .tx    = db->atx,
        .vname = query->name,
        .alloc = alloc,
    };
    WRAP_GOTO (ns_var_get (&gparams, &db->e), failed_rollback);
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
          &db->e,
          ERR_CORRUPT,
          "Variable: %.*s has invalid byte size",
          strfmt (&query->name)
      );
      goto failed_rollback;
    }
    len /= tsize;

    // Resolve length based on the stride
    if (stride_resolve (&stride, query->ustr, len, &db->e))
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
    ret = ns_write (wparams, &db->e);
    WRAP_GOTO (ret, failed_rollback);
  }

  // COMMIT
  WRAP_GOTO (nsdb_auto_commit (db, &db->e), failed_rollback);
  return ret;

failed_rollback:

  nsdb_auto_rollback (db);

failed:
  return error_trace (&db->e);
}

/******************************************************************************
 * SECTION: nsdb_execute on buffer
 ******************************************************************************/

sb_size
nsdb_execute_on_buffer (
    struct nsdb        *ns,
    struct query       *q,
    void               *data,
    struct chunk_alloc *alc
)
{
  sb_size          ret = SUCCESS;
  struct variable *var;

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
            &ns->e,
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
      ret = nsdb_read (ns, &q->read, alc, &stream);
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
            &ns->e,
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
      ret = nsdb_write (ns, &q->write, alc, &stream);
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

        ret = nsdb_remove (ns, &q->remove, alc, &stream);
      }
      else
      {
        ret = nsdb_remove (ns, &q->remove, alc, NULL);
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
            &ns->e,
            ERR_INVALID_ARGUMENT,
            "data is required for a get operation"
        );
        goto failed;
      }

      stream_ibuf_init (&stream, &ictx, data, 0);
      ret = nsdb_insert (ns, &q->insert, alc, &stream);
      if (ret < 0)
      {
        goto failed;
      }

      break;
    }

    case QT_CREATE:
    {
      if (nsdb_create (ns, alc, q->create.name, q->create.type))
      {
        goto failed;
      }

      ret = SUCCESS;

      break;
    }
    case QT_DELETE:
    {
      if (nsdb_delete (ns, q->delete.name))
      {
        goto failed;
      }

      ret = SUCCESS;

      break;
    }
    case QT_GET:
    {
      struct nsdb_var **_data = data;

      // Destination pointer is required
      if (data == NULL)
      {
        error_causef (
            &ns->e,
            ERR_INVALID_ARGUMENT,
            "data is required for a get operation"
        );
        goto failed;
      }

      // Variables get their own chunk allocator
      // context that gets freed on nsdb_var_free
      struct chunk_alloc *valloc = i_malloc (1, sizeof *valloc, &ns->e);
      if (valloc == NULL)
      {
        goto failed;
      }
      chunk_alloc_create_default (valloc);

      // Get the variable
      if (nsdb_get (ns, &q->get, valloc, &var) < 0)
      {
        chunk_alloc_free_all (valloc);
        i_free (valloc);
        goto failed;
      }

      if (var == NULL)
      {
        *_data = NULL;
        chunk_alloc_free_all (valloc);
        i_free (valloc);
        ret = SUCCESS;
        break;
      }

      // Transfer over to a variable handle (that can be free'd)
      *_data = chunk_malloc (valloc, 1, sizeof (struct nsdb_var), &ns->e);

      if (*_data == NULL)
      {
        chunk_alloc_free_all (valloc);
        i_free (valloc);
        goto failed;
      }

      (*_data)->var   = var;
      (*_data)->alloc = valloc;

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

  return error_trace (&ns->e);
}
