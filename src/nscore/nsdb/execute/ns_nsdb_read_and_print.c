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

#include "core/ns_arena_alloc.h"
#include "core/ns_error.h"
#include "core/ns_ext_array.h"
#include "core/ns_logging.h"
#include "core/ns_stream.h"
#include "core/ns_stride.h"
#include "nscore/algorithms/rope/ns_rope_algorithms.h"
#include "nscore/algorithms/var/ns_var_algorithms.h"
#include "nscore/nsdb/ns_nsdb.h"
#include "nscore/types/ns_query.h"
#include "nscore/types/ns_types.h"
#include "nscore/variables/ns_variables.h"

sb_size
nsdb_read_and_print (
    struct nsdb        *db,    // The database handle
    struct read_query  *query, // The query that got parsed
    struct arena_alloc *alloc, // Where to allocate stuff
    error              *e
)
{
  sb_size                  ret;     // Return value
  t_size                   tsize;   // Size of  the variable
  b_size                   len;     // Length of the variable
  struct ns_var_get_params gparams; // Get operation
  struct ns_read_params    rparams; // Read operation
  struct stride            stride;  // Resolved stride
  struct stream            dest;    // Output stream

  struct ns_txn           *tx = nsdb_begin (db, e);
  if (tx == NULL) {
    goto failed;
  }

  // GET VARIABLE
  {
    gparams = (struct ns_var_get_params){
        .p     = db->p,
        .tx    = tx,
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
    len   = gparams.dest.nbytes;

    // A consistent database has this be a multiple of tsize
    if (len % tsize != 0) {
      error_causef (e, ERR_CORRUPT, "Variable: %.*s has invalid byte size", strfmt (&query->name));
      goto failed_rollback;
    }
    len /= tsize;

    // Resolve length based on the stride
    if (stride_resolve (&stride, query->ustr, len, e)) {
      goto failed_rollback;
    }

    // Create the destination stream to print to the console
    if (type_stream_printer_init (&dest, gparams.dest.dtype, e)) {
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
        .dest   = &dest,
        .tx     = tx,
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
  if (nsdb_commit (db, tx, e)) {
    goto failed;
  }
  return ret;

failed_rollback:

  nsdb_rollback (db, tx, e);

failed:
  return error_trace (e);
}
