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
#include "nscore/nsdb/ns_nsdb.h"
#include "nscore/pager/ns_pager.h"
#include "nscore/types/ns_query.h"
#include "nscore/types/ns_types.h"
#include "nscore/types/ns_variables.h"
#include "numstore/numstore.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

sb_size
nsdb_execute_on_buffer (
    struct nsdb      *ns,
    struct nstxn_t   *txn,
    struct query     *q,
    void             *data,
    struct allocator *alc
)
{
  sb_size                ret = SUCCESS;
  struct variable       *var;

  struct stream          stream;
  struct stream_obuf_ctx octx;
  struct stream_ibuf_ctx ictx;

  switch (q->type) {
    case QT_READ: {
      // Destination pointer is required
      if (data == NULL) {
        error_causef (&ns->e, ERR_INVALID_ARGUMENT, "data is required for a read operation");
        goto failed;
      }

      if (q->read.limit && q->read.blimit) {
        stream_obuf_init (&stream, &octx, data, q->read.limit);
      } else {
        stream_obuf_init (&stream, &octx, data, 0);
      }
      ret = nsdb_read (ns, &q->read, alc, &stream);
      if (ret < 0) {
        goto failed;
      }

      break;
    }
    case QT_WRITE: {
      // Source pointer is required
      if (data == NULL) {
        error_causef (&ns->e, ERR_INVALID_ARGUMENT, "data is required for a write operation");
        goto failed;
      }

      if (q->write.limit && q->write.blimit) {
        stream_ibuf_init (&stream, &ictx, data, q->write.limit);
      } else {
        stream_ibuf_init (&stream, &ictx, data, 0);
      }
      ret = nsdb_write (ns, &q->write, alc, &stream);
      if (ret < 0) {
        goto failed;
      }

      break;
    }
    case QT_REMOVE: {
      if (data) {
        if (q->remove.limit && q->remove.blimit) {
          stream_obuf_init (&stream, &octx, data, q->remove.limit);
        } else {
          stream_obuf_init (&stream, &octx, data, 0);
        }

        ret = nsdb_remove (ns, &q->remove, alc, &stream);
      } else {
        ret = nsdb_remove (ns, &q->remove, alc, NULL);
      }
      if (ret < 0) {
        goto failed;
      }

      break;
    }
    case QT_INSERT: {
      // Source pointer is required
      if (data == NULL) {
        error_causef (&ns->e, ERR_INVALID_ARGUMENT, "data is required for a insert operation");
        goto failed;
      }

      stream_ibuf_init (&stream, &ictx, data, 0);
      ret = nsdb_insert (ns, &q->insert, alc, &stream);
      if (ret < 0) {
        goto failed;
      }

      break;
    }

    case QT_CREATE: {
      if (nsdb_create (ns, alc, q->create.name, q->create.type)) {
        goto failed;
      }

      ret = SUCCESS;

      break;
    }
    case QT_DELETE: {
      if (nsdb_delete (ns, &q->delete)) {
        goto failed;
      }

      ret = SUCCESS;

      break;
    }
    case QT_GET: {
      struct nsdb_var **_data = data;

      // Destination pointer is required
      if (data == NULL) {
        error_causef (&ns->e, ERR_INVALID_ARGUMENT, "data is required for a get operation");
        goto failed;
      }

      // Variables get their own allocator
      // context that gets freed on nsdb_var_free
      struct allocator *valloc = i_malloc (default_mem (), 1, sizeof *valloc, &ns->e);
      if (valloc == NULL) {
        goto failed;
      }
      create_default_allocator (valloc);

      // Get the variable
      if (nsdb_get (ns, &q->get, valloc, &var) < 0) {
        allocator_free (valloc);
        i_free (default_mem (), valloc);
        goto failed;
      }

      if (var == NULL) {
        *_data = NULL;
        allocator_free (valloc);
        i_free (default_mem (), valloc);
        ret = SUCCESS;
        break;
      }

      // Transfer over to a variable handle (that can be free'd)
      *_data = allocate (valloc, 1, sizeof (struct nsdb_var), &ns->e);

      if (*_data == NULL) {
        allocator_free (valloc);
        i_free (default_mem (), valloc);
        goto failed;
      }

      (*_data)->var   = var;
      (*_data)->alloc = valloc;

      ret             = SUCCESS;

      break;
    }

    case QT_EXIT: {
      ret = SUCCESS;
      break;
    }

    case QT_HELP: {
      ret = SUCCESS;
      break;
    }
  }

  return ret;

failed:

  return error_trace (&ns->e);
}

/******************************************************************************
 * SECTION: nsdb_get_and_print
 ******************************************************************************/

err_t
nsdb_get_and_print (struct nsdb *db, struct get_query *query, struct allocator *alloc)
{
  struct ns_var_get_params gparams; // Get or create operation

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
      fprintf (stderr, "Variable: %.*s doesn't exist\n", strfmt (&query->name));
      goto commit;
    }
    WRAP_GOTO (err, failed_rollback);
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
 * SECTION: nsdb_read_and_print
 ******************************************************************************/

sb_size
nsdb_read_and_print (
    struct nsdb       *db,    // The database handle
    struct read_query *query, // The query that got parsed
    struct allocator  *alloc  // Where to allocate stuff
)
{
  sb_size                  ret;     // Return value
  t_size                   tsize;   // Size of  the variable
  b_size                   len;     // Length of the variable
  struct ns_var_get_params gparams; // Get operation
  struct ns_read_params    rparams; // Read operation
  struct stride            stride;  // Resolved stride
  struct stream            dest;    // Output stream

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

    // Create the destination stream to print to the console
    if (type_stream_printer_init (&dest, gparams.dest.dtype, &db->e)) {
      goto failed_rollback;
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
        .dest   = &dest,
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
 * SECTION: nsdb_execute on console
 ******************************************************************************/

static err_t
nsdb_execute_in_console (struct nsdb *ns, struct query *q, struct allocator *alc)
{
  sb_size ret = SUCCESS;

  switch (q->type) {
    case QT_READ: {
      ret = nsdb_read_and_print (ns, &q->read, alc);
      if (ret < 0) {
        goto failed;
      }

      break;
    }
    case QT_WRITE: {
      break;
    }
    case QT_REMOVE: {
      break;
    }
    case QT_INSERT: {
      break;
    }

    case QT_CREATE: {
      if (nsdb_create (ns, alc, q->create.name, q->create.type)) {
        goto failed;
      }

      printf ("{ \"Status\" : \"Ok\" }\n");

      ret = SUCCESS;

      break;
    }
    case QT_DELETE: {
      break;
    }
    case QT_GET: {
      ret = nsdb_get_and_print (ns, &q->get, alc);
      if (ret < 0) {
        goto failed;
      }
      break;
    }

    case QT_EXIT: {
      break;
    }

    case QT_HELP: {
      break;
    }
  }

  return ret;

failed:

  return error_trace (&ns->e);
}
