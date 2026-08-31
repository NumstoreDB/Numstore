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
#include "core/ns_concurrency.h"
#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_ext_array.h"
#include "core/ns_logging.h"
#include "core/ns_slab_alloc.h"
#include "core/ns_stream.h"
#include "core/ns_stride.h"
#include "core/os/ns_filesystem.h"
#include "core/os/ns_memory.h"
#include "nscore/algorithms/rope/ns_rope_algorithms.h"
#include "nscore/algorithms/var/ns_var_algorithms.h"
#include "nscore/pager/ns_pager.h"
#include "nscore/types/ns_query.h"
#include "nscore/types/ns_types.h"
#include "nscore/variables/ns_variables.h"
#include "numstore/numstore.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

struct nsdb *
nsdb_open_with_resources (const char *path, struct i_mem mem, struct i_file_system fs)
{
  error        e   = error_create ();
  struct nsdb *ret = i_malloc (mem, 1, sizeof *ret, &e);

  if (ret == NULL) {
    // TODO - what to do with the error?
    return NULL;
  }

  // Initialize inner values
  {
    // Trivial initializers
    ret->e = error_create ();
    slab_alloc_init (&ret->txn_alloc, mem, sizeof (struct ns_txn), 512);
    latch_init (&ret->l);
    ret->mem       = mem;
    ret->fs        = fs;
    ret->path.data = NULL;
    ret->p         = NULL;

    // Path
    ret->path.len  = strlen (path);
    ret->path.data = i_malloc (mem, ret->path.len, 1, &e);
    if (ret->path.data == NULL) {
      goto failed;
    }

    // Pager
    ret->p = pgr_open (path, mem, fs, &e);
    if (ret->p == NULL) {
      goto failed;
    }
  }

  // New pager - initialze the upfront hash map
  if ((pgr_isnew (ret->p)) && (ns_init_var_hash_map (ret->p, &e)))
  // Initialize the upfront hash page
  {
    goto failed;
  }

  // Launch the checkpoint writer thread
  if (pgr_launch_checkpoint_thread (ret->p, 5000, &e)) {
    goto failed;
  }

  return ret;

failed:
  if (ret->p) {
    pgr_close (ret->p, &e);
  }
  i_free (mem, (void *)ret->path.data);
  i_free (mem, ret);
  pgr_delete_single_file (path, &e);
  return NULL;
}

int
nsdb_cleanup (const char *path)
{
  error e = error_create ();
  pgr_delete_single_file (path, &e);
  return error_trace (&e);
}

err_t
nsdb_close (struct nsdb *n)
{
  n->e.cause_code = SUCCESS;
  n->e.cmlen      = 0;

  err_t ret       = pgr_close (n->p, &n->e);
  slab_alloc_destroy (&n->txn_alloc);

  struct i_mem mem = n->mem;
  i_free (mem, (void *)n->path.data);
  i_free (mem, n);

  return ret;
}

err_t
nsdb_crash (struct nsdb *n)
{
  n->e.cause_code = SUCCESS;
  n->e.cmlen      = 0;

  err_t err       = pgr_crash (n->p, &n->e);
  slab_alloc_destroy (&n->txn_alloc);

  struct i_mem mem = n->mem;
  i_free (mem, (void *)n->path.data);
  i_free (mem, n);

  return err;
}

const char *
nsdb_strerror (struct nsdb *ns)
{
  if (ns->e.cause_code < 0) {
    return ns->e.cause_msg;
  }
  return NULL;
}

int
nsdb_perror (struct nsdb *ns, const char *prefix)
{
  const char *err = nsdb_strerror (ns);
  if (err) {
    return fprintf (stderr, "%s: %s\n", prefix, nsdb_strerror (ns));
  }
  return fprintf (stderr, "%s: success\n", prefix);
}

struct ns_txn *
nsdb_begin (struct nsdb *smf)
{
  smf->e.cause_code = 0;
  smf->e.cmlen      = 0;

  struct ns_txn *tx = slab_alloc_alloc (&smf->txn_alloc, &smf->e);
  if (tx == NULL) {
    return NULL;
  }

  if (pgr_begin_txn (tx, smf->p, &smf->e)) {
    slab_alloc_free (&smf->txn_alloc, tx);
    return NULL;
  }

  return tx;
}

err_t
nsdb_commit (struct nsdb *smf, struct ns_txn *tx)
{
  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  if (pgr_commit (smf->p, tx, &smf->e)) {
    slab_alloc_free (&smf->txn_alloc, tx);
    return error_trace (&smf->e);
  }

  slab_alloc_free (&smf->txn_alloc, tx);
  return SUCCESS;
}

err_t
nsdb_rollback (struct nsdb *smf, struct ns_txn *tx)
{
  smf->e.cause_code = SUCCESS;
  smf->e.cmlen      = 0;

  if (pgr_rollback (smf->p, tx, 0, &smf->e)) {
    slab_alloc_free (&smf->txn_alloc, tx);
    return error_trace (&smf->e);
  }

  slab_alloc_free (&smf->txn_alloc, tx);
  return SUCCESS;
}

err_t
nsdb_create (
    struct nsdb      *db,
    struct ns_txn    *tx,
    struct allocator *alloc,
    struct string     vname,
    struct type       dtype
)
{
  db->e.cause_code = SUCCESS;
  db->e.cmlen      = 0;

  AUTO_BEGIN (db, tx);

  // Log the call
  i_log_debug ("CREATE (txn = %" PRtxid "): %.*s\n", tx->tid, strfmt (&vname));

  // Get or create
  {
    struct ns_var_get_or_create_params gparams = {
        .p     = db->p,
        .tx    = tx,
        .vname = vname,
        .type  = &dtype,
        .alloc = alloc,
    };
    if (ns_var_get_or_create (&gparams, &db->e)) {
      goto failed_rollback;
    }
  }

  AUTO_COMMIT (db, tx);

  return SUCCESS;

failed_rollback:
  ROLLBACK_PRESERVING_ERROR (db, tx);

failed:
  return error_trace (&db->e);
}

err_t
nsdb_delete (struct nsdb *db, struct ns_txn *tx, struct delete_query *query)
{
  db->e.cause_code = SUCCESS;
  db->e.cmlen      = 0;

  AUTO_BEGIN (db, tx);

  i_log_debug ("DELETE (txn = %" PRtxid "): %.*s\n", tx->tid, strfmt (&query->name));

  {
    // DELETE
    struct ns_var_delete_params params = {
        .p     = db->p,
        .tx    = tx,
        .vname = query->name,
    };

    err_t err = ns_var_delete (params, &db->e);
    if (query->if_exists && err == ERR_VARIABLE_NE) {
      db->e.cause_code = SUCCESS;
      db->e.cmlen      = 0;
      goto commit;
    }

    if (err < SUCCESS) {
      goto failed_rollback;
    }
  }

commit:

  AUTO_COMMIT (db, tx);

  return error_trace (&db->e);

failed_rollback:
  ROLLBACK_PRESERVING_ERROR (db, tx);

failed:
  return error_trace (&db->e);
}

err_t
nsdb_get (
    struct nsdb      *db,
    struct ns_txn    *tx,
    struct get_query *query,
    struct allocator *alloc,
    struct variable **dest
)
{
  ASSERT (dest);

  db->e.cause_code = SUCCESS;
  db->e.cmlen      = 0;

  *dest            = allocate (alloc, 1, sizeof (struct variable), &db->e);
  if (*dest == NULL) {
    return error_trace (&db->e);
  }

  AUTO_BEGIN (db, tx);

  i_log_debug ("GET (txn = %" PRtxid ") - %.*s\n", tx->tid, strfmt (&query->name));

  // Get Variable
  {
    struct ns_var_get_params gparams = {
        .p     = db->p,
        .tx    = tx,
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

    if (err < 0) {
      goto failed_rollback;
    }

    *(*dest) = gparams.dest;
  }

commit:
  AUTO_COMMIT (db, tx);

  return SUCCESS;

failed_rollback:
  ROLLBACK_PRESERVING_ERROR (db, tx);

failed:
  return error_trace (&db->e);
}

sb_size
nsdb_insert (
    struct nsdb         *db,
    struct ns_txn       *tx,
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

  db->e.cause_code = SUCCESS;
  db->e.cmlen      = 0;

  // Skip len 0 inserts
  if (query->len == 0) {
    return 0;
  }

  // BEGIN TXN
  AUTO_BEGIN (db, tx);

  // Get Variable
  {
    gparams = (struct ns_var_get_params){
        .p     = db->p,
        .tx    = tx,
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
      tx->tid,
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

  // Insert
  {
    iparams = (struct ns_insert_params){
        .p     = db->p,
        .src   = src,
        .tx    = tx,
        .root  = gparams.dest.rpt_root,
        .bofst = bofst,
        .bytes = query->len * tsize,
    };
    ret = ns_insert (&iparams, &db->e);
    if (ret != (sb_size)(query->len * tsize)) {
      goto failed_rollback;
    }
  }

  // Update Varible
  {
    uparams = (struct ns_var_update_params){
        .p      = db->p,
        .tx     = tx,
        .retr   = (struct var_retrieval){.type = VR_PG, .root = gparams.dest.var_root},
        .newpg  = iparams.root,
        .nbytes = gparams.dest.nbytes + ret,
    };
    WRAP_GOTO (ns_var_update (uparams, &db->e), failed_rollback);
  }

  ASSERT (ret % tsize == 0);
  ret /= tsize;

  AUTO_COMMIT (db, tx);
  return ret;

failed_rollback:
  ROLLBACK_PRESERVING_ERROR (db, tx);

failed:
  return error_trace (&db->e);
}

/******************************************************************************
 * SECTION: nsdb_read
 ******************************************************************************/

sb_size
nsdb_read (
    struct nsdb       *db,
    struct ns_txn     *tx,
    struct read_query *query,
    struct allocator  *alloc,
    struct stream     *dest
)
{
  sb_size                  ret;     // Return value
  t_size                   tsize;   // Size of  the variable
  b_size                   len;     // Length of the variable
  struct ns_var_get_params gparams; // Get operation
  struct ns_read_params    rparams; // Read operation
  struct stride            stride;  // Resolved stride

  db->e.cause_code = SUCCESS;
  db->e.cmlen      = 0;

  AUTO_BEGIN (db, tx);

  // Get variable
  {
    gparams = (struct ns_var_get_params){
        .p     = db->p,
        .tx    = tx,
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
      tx->tid,
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
        .p      = db->p,
        .dest   = dest,
        .tx     = tx,
        .root   = gparams.dest.rpt_root,
        .size   = tsize,
        .bofst  = tsize * stride.start,
        .stride = stride.stride,
        .nelem  = stride.nelems,
    };
    ret = ns_read (rparams, &db->e);
    WRAP_GOTO (ret, failed_rollback);
  }

  AUTO_COMMIT (db, tx);
  return ret;

failed_rollback:
  ROLLBACK_PRESERVING_ERROR (db, tx);

failed:
  return error_trace (&db->e);
}

sb_size
nsdb_remove (
    struct nsdb         *db,
    struct ns_txn       *tx,
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

  db->e.cause_code = SUCCESS;
  db->e.cmlen      = 0;

  // BEGIN TXN
  AUTO_BEGIN (db, tx);

  // GET VARIABLE
  {
    gparams = (struct ns_var_get_params){
        .p     = db->p,
        .tx    = tx,
        .vname = query->name,
        .alloc = alloc,
    };

    if (ns_var_get (&gparams, &db->e)) {
      goto failed_rollback;
    }
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
      tx->tid,
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
        .p      = db->p,
        .dest   = dest,
        .tx     = tx,
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
        .p      = db->p,
        .tx     = tx,
        .retr   = (struct var_retrieval){.type = VR_PG, .root = gparams.dest.var_root},
        .newpg  = rparams.root,
        .nbytes = gparams.dest.nbytes - (ret * tsize),
    };
    if (ns_var_update (uparams, &db->e) < 0) {
      goto failed_rollback;
    }
  }

  AUTO_COMMIT (db, tx);
  return ret;

failed_rollback:
  ROLLBACK_PRESERVING_ERROR (db, tx);

failed:
  return error_trace (&db->e);
}

sb_size
nsdb_write (
    struct nsdb        *db,
    struct ns_txn      *tx,
    struct write_query *query,
    struct allocator   *alloc,
    struct stream      *src
)
{
  sb_size                  ret;     // Return value
  t_size                   tsize;   // Size of  the variable
  b_size                   len;     // Length of the variable
  struct ns_var_get_params gparams; // Get or create operation
  struct ns_write_params   wparams; // Write operation
  struct stride            stride;  // Resolved stride

  db->e.cause_code = SUCCESS;
  db->e.cmlen      = 0;

  AUTO_BEGIN (db, tx);

  // GET VARIABLE
  {
    gparams = (struct ns_var_get_params){
        .p     = db->p,
        .tx    = tx,
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
      tx->tid,
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
        .p      = db->p,
        .src    = src,
        .tx     = tx,
        .root   = gparams.dest.rpt_root,
        .size   = tsize,
        .bofst  = tsize * stride.start,
        .stride = stride.stride,
        .nelem  = stride.nelems,
    };
    ret = ns_write (wparams, &db->e);
    WRAP_GOTO (ret, failed_rollback);
  }

  AUTO_COMMIT (db, tx);
  return ret;

failed_rollback:

  ROLLBACK_PRESERVING_ERROR (db, tx);

failed:
  return error_trace (&db->e);
}
