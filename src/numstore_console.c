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

#include "alloc.h"
#include "csx_assert.h"
#include "error.h"
#include "logging.h"
#include "nsdb.h"
#include "numstore.h"
#include "query.h"
#include "rope_algorithms.h"
#include "serial.h"
#include "types.h"
#include "var_algorithms.h"
#include "variables.h"

/******************************************************************************
 * SECTION: nsdb_get_and_print
 ******************************************************************************/

err_t
nsdb_get_and_print (
    struct nsdb        *db,
    struct get_query   *query,
    struct chunk_alloc *alloc
)
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
    if (query->if_exists && err == ERR_VARIABLE_NE)
    {
      db->e.cause_code = SUCCESS;
      db->e.cmlen      = 0;
      fprintf (stderr, "Variable: %.*s doesn't exist\n", strfmt (&query->name));
      goto commit;
    }
    WRAP_GOTO (err, failed_rollback);

    i_print_variable (&gparams.dest, &db->e);
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
    struct nsdb        *db,    // The database handle
    struct read_query  *query, // The query that got parsed
    struct chunk_alloc *alloc  // Where to allocate stuff
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

    // Create the destination stream to print to the console
    if (type_stream_printer_init (&dest, gparams.dest.dtype, &db->e))
    {
      goto failed_rollback;
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
 * SECTION: nsdb_execute on buffer
 ******************************************************************************/

err_t
nsdb_execute_in_console (
    struct nsdb        *ns,
    struct query       *q,
    struct chunk_alloc *alc
)
{
  sb_size ret = SUCCESS;

  switch (q->type)
  {
    case QT_READ:
    {
      ret = nsdb_read_and_print (ns, &q->read, alc);
      if (ret < 0)
      {
        goto failed;
      }

      break;
    }
    case QT_WRITE:
    {
      break;
    }
    case QT_REMOVE:
    {
      break;
    }
    case QT_INSERT:
    {
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
      break;
    }
    case QT_GET:
    {
      ret = nsdb_get_and_print (ns, &q->get, alc);
      if (ret < 0)
      {
        goto failed;
      }
      break;
    }

    case QT_EXIT:
    {
      break;
    }

    case QT_HELP:
    {
      break;
    }
  }

  return ret;

failed:

  return error_trace (&ns->e);
}
