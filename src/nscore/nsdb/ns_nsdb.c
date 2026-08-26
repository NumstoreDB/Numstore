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

#include "nscore/nsdb/ns_nsdb.h"

#include "core/ns_alloc.h"
#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_ext_array.h"
#include "core/ns_logging.h"
#include "core/ns_stream.h"
#include "core/ns_stride.h"
#include "core/os/ns_memory.h"
#include "nscore/algorithms/rope/ns_rope_algorithms.h"
#include "nscore/algorithms/var/ns_var_algorithms.h"
#include "nscore/compiler/ns_compiler.h"
#include "nscore/pager/ns_pager.h"
#include "nscore/types/ns_query.h"
#include "nscore/types/ns_types.h"
#include "nscore/types/ns_variables.h"
#include "numstore/numstore.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

int
nsdb_perror (struct nsdb *ns, const char *prefix)
{
  const char *err = nsdb_strerror (ns);
  if (err) {
    return fprintf (stderr, "%s: %s\n", prefix, nsdb_strerror (ns));
  } else {
    return fprintf (stderr, "%s: success\n", prefix);
  }
}

const char *
nsdb_strerror (struct nsdb *ns)
{
  if (ns->e.cause_code < 0) {
    return ns->e.cause_msg;
  } else {
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
  i_free (default_mem (), (void *)root->path.data);
  i_free (default_mem (), root);
  return err;
}

struct nsdb *
nsdb_root_load (struct nsdb_root *ns, error *e)
{
  struct nsdb *ret = i_malloc (default_mem (), 1, sizeof *ret, e);
  if (ret == NULL) {
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
  i_free (default_mem (), sm);
  root->count -= 1;
}

/******************************************************************************
 * SECTION: Auto transaction behavior
 ******************************************************************************/

err_t
nsdb_auto_begin_txn (struct nsdb *sm, error *e)
{
  if (sm->atx == NULL) {
    WRAP (pgr_begin_txn (&sm->tx, sm->root->p, e));
    sm->is_auto_txn = 1;
    sm->atx         = &sm->tx;
  }

  return SUCCESS;
}

err_t
nsdb_auto_commit (struct nsdb *sm, error *e)
{
  if (sm->is_auto_txn) {
    ASSERT (sm->atx);
    WRAP (pgr_commit (sm->root->p, sm->atx, e));
    sm->atx = NULL;
  }
  return SUCCESS;
}

void
nsdb_auto_rollback (struct nsdb *sm)
{
  if (pgr_rollback (sm->root->p, sm->atx, 0, &sm->e)) {
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

  if (smf->atx) {
    return error_causef (
        &smf->e,
        ERR_INVALID_ARGUMENT,
        "Can't start another transaction, already a part of an existing "
        "transaction: %" PRtxid ". Either commit or rollback first",
        smf->atx->tid
    );
  }

  if (pgr_begin_txn (&smf->tx, smf->root->p, &smf->e)) {
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
  if (root->count == 0) {
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

  if (smf->atx == NULL) {
    return error_causef (
        &smf->e,
        ERR_INVALID_ARGUMENT,
        "Can't commit transaction, not a part of an existing transaction"
    );
  }

  if (pgr_commit (smf->root->p, smf->atx, &smf->e)) {
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

  if (smf->atx == NULL) {
    return error_causef (
        &smf->e,
        ERR_INVALID_ARGUMENT,
        "Can't rollback transaction, not a part of an existing transaction"
    );
  }

  if (pgr_rollback (smf->root->p, smf->atx, 0, &smf->e)) {
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
  n->e.cause_code        = SUCCESS;
  n->e.cmlen             = 0;

  struct nsdb_root *root = n->root;

  err_t             err  = pgr_crash (root->p, &n->e);
  i_free (default_mem (), (void *)root->path.data);
  i_free (default_mem (), n);
  i_free (default_mem (), root);

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
  error             e   = error_create ();

  struct nsdb_root *ret = i_malloc (default_mem (), 1, sizeof *ret, &e);

  if (ret == NULL) {
    return NULL;
  }

  // Initialize inner values
  {
    ret->e         = error_create ();
    ret->count     = 0;

    // path
    ret->path.len  = strlen (path);
    ret->path.data = i_malloc (default_mem (), ret->path.len, 1, &e);
    if (ret->path.data == NULL) {
      goto failed;
    }

    // db
    ret->p = pgr_open (path, default_mem (), default_filesystem (), &e);
    if (ret->p == NULL) {
      goto failed;
    }
  }

  // Upfront initialization
  if (pgr_isnew (ret->p)) {
    // Initialize the upfront hash page
    if (ns_init_var_hash_map (ret->p, &e)) {
      goto failed;
    }
  }

  // Launch the checkpoint writer thread
  if (pgr_launch_checkpoint_thread (ret->p, 5000, &e)) {
    goto failed;
  }

  // Load the default context
  struct nsdb *sret = nsdb_root_load (ret, &e);

  return sret;

failed:
  // TODO just delete the file
  i_free (default_mem (), ret);
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
  struct allocator *alloc = var->alloc;
  allocator_free (alloc);
  i_free (default_mem (), alloc);
}

/******************************************************************************
 * SECTION: nsdb_get
 ******************************************************************************/

err_t
nsdb_get (struct nsdb *db, struct get_query *query, struct allocator *alloc, struct variable **dest)
{
  ASSERT (dest);
  struct ns_var_get_params gparams; // Get or create operation

  *dest = allocate (alloc, 1, sizeof (struct variable), &db->e);
  if (*dest == NULL) {
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
    if (query->if_exists && err == ERR_VARIABLE_NE) {
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
  if (nsdb_auto_commit (db, &db->e)) {
    goto failed_rollback;
  }

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
nsdb_create (struct nsdb *db, struct allocator *alloc, struct string vname, struct type dtype)
{
  struct ns_var_get_or_create_params gparams; // Get or create operation

  // BEGIN TXN
  WRAP_GOTO (nsdb_auto_begin_txn (db, &db->e), failed);

  i_log_debug ("CREATE (txn = %" PRtxid "): %.*s\n", db->atx->tid, strfmt (&vname));

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

  return SUCCESS;

failed_rollback:

  nsdb_auto_rollback (db);

failed:
  return error_trace (&db->e);
}

/******************************************************************************
 * SECTION: nsdb_delete
 ******************************************************************************/

err_t
nsdb_delete (struct nsdb *db, struct delete_query *query)
{
  // BEGIN TXN
  WRAP_GOTO (nsdb_auto_begin_txn (db, &db->e), failed);

  i_log_debug ("DELETE (txn = %" PRtxid "): %.*s\n", db->atx->tid, strfmt (&query->name));

  {
    // DELETE
    struct ns_var_delete_params params = {
        .p     = db->root->p,
        .tx    = db->atx,
        .vname = query->name,
    };
    err_t err = ns_var_delete (params, &db->e);
    if (query->if_exists && err == ERR_VARIABLE_NE) {
      db->e.cause_code = SUCCESS;
      db->e.cmlen      = 0;
      goto commit;
    }
    WRAP_GOTO (err, failed_rollback);
    if (err < SUCCESS) {
      goto failed_rollback;
    }
  }

commit:
  if (nsdb_auto_commit (db, &db->e)) {
    goto failed_rollback;
  }
  return error_trace (&db->e);

failed_rollback:

  nsdb_auto_rollback (db);

failed:
  return error_trace (&db->e);
}

/******************************************************************************
 * SECTION: nsdb_insert
 ******************************************************************************/

sb_size
nsdb_insert (
    struct nsdb         *db,
    struct insert_query *query,
    struct allocator    *alloc,
    struct stream       *src
)
{
  sb_size                     ret;     // Return value
  b_size                      bofst;   // Resolved offset
  struct ns_var_get_params    gparams; // Get or create operation
  struct ns_insert_params     iparams; // Insert operation
  struct ns_var_update_params uparams; // Update operation

  // Parameter validation
  if (query->len == 0) {
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
      " size (bytes): %" PRt_size " curlen: %" PRb_size " curlen (bytes): %" PRb_size
      " Requested: "
      " ofst: %" PRId64 " ofst (bytes): %" PRId64 " nelem: %" PRId64 " nbytes (bytes): %" PRId64
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
    if (ret != (sb_size)(query->len * tsize)) {
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
    struct nsdb       *db,    // The database handle
    struct read_query *query, // The query that got parsed
    struct allocator  *alloc, // Where to allocate stuff
    struct stream     *dest   // destination stream
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
    len   = gparams.dest.nbytes;

    // A consistent database has this be a multiple of tsize
    if (len % tsize != 0) {
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
    if (stride_resolve (&stride, query->ustr, len, &db->e)) {
      goto failed_rollback;
    }

    // Check limit
    if (query->limit > 0) {
      if (query->blimit) {
        stride.nelems = query->limit / tsize;
      } else {
        stride.nelems = query->limit;
      }
    } else {
      ASSERT (!query->blimit);
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
    struct allocator    *alloc,
    struct stream       *dest
)
{
  sb_size                     ret;     // Return value
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
    len   = gparams.dest.nbytes;

    // A consistent database has this be a multiple of tsize
    if (len % tsize != 0) {
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
    if (stride_resolve (&stride, query->ustr, len, &db->e)) {
      goto failed_rollback;
    }

    // Check limit
    if (query->limit > 0) {
      if (query->blimit) {
        stride.nelems = query->limit / tsize;
      } else {
        stride.nelems = query->limit;
      }
    } else {
      ASSERT (!query->blimit);
    }
  }

  i_log_debug (
      "REMOVE (txn = %" PRtxid
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
nsdb_write (struct nsdb *db, struct write_query *query, struct allocator *alloc, struct stream *src)
{
  sb_size                  ret;     // Return value
  t_size                   tsize;   // Size of  the variable
  b_size                   len;     // Length of the variable
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
    len   = gparams.dest.nbytes;

    // A consistent database has this be a multiple of tsize
    if (len % tsize != 0) {
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
    if (stride_resolve (&stride, query->ustr, len, &db->e)) {
      goto failed_rollback;
    }

    // Check limit
    if (query->limit > 0) {
      if (query->blimit) {
        // byte limit
        stride.nelems = query->limit / tsize;
      } else {
        // element limit
        stride.nelems = query->limit;
      }
    } else {
      ASSERT (!query->blimit);
    }
  }

  i_log_debug (
      "WRITE (txn = %" PRtxid
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
