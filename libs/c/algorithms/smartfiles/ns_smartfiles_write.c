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

#include "ns_arena_alloc.h"
#include "ns_bounds.h"
#include "ns_error.h"
#include "ns_stdtypes.h"
#include "ns_stream.h"
#include "ns_string.h"
#include "ns_testing.h"
#include "ns_utils.h"
#include "pager/ns_page_fixture.h"
#include "pager/ns_pager.h"
#include "rope/ns_rope_algorithms.h"
#include "smartfiles/ns_smartfiles_algorithms.h"
#include "types/ns_types.h"
#include "types/ns_variables.h"
#include "var/ns_var_algorithms.h"

sb_size
smartfiles_write (
    struct pager       *p,
    struct txn         *tx,
    struct stream      *src,
    t_size              size,
    sb_size             bofst,
    sb_size             stride,
    b_size              nelem,
    struct arena_alloc *alloc,
    error              *e
)
{
  DBG_ASSERT (pager, p);
  DBG_ASSERT (ns_txn, tx);

  ASSERT (tx != NULL);

  if (stride < 0) {
    return error_causef (e, ERR_INVALID_ARGUMENT, "Negative strides aren't supported yet");
  }
  if (stride == 0) {
    return error_causef (e, ERR_INVALID_ARGUMENT, "Cannot write with stride == 0");
  }
  if (size == 0) {
    return error_causef (e, ERR_INVALID_ARGUMENT, "Cannot write with size == 0");
  }
  if (nelem == 0) {
    return 0;
  }

  // GET OR CREATE VARIABLE
  struct ns_var_get_params gparams = {
      .p     = p,
      .tx    = tx,
      .vname = strfcstr (DEFAULT_VARIABLE),
      .alloc = alloc,
  };
  if (ns_var_get (&gparams, e) < 0) {
    return error_trace (e);
  }

  // Resolve sizes
  //
  // A stride of 1 is one byte range, so split it by bytes: what is inside
  // the file is overwritten and the rest is appended. Splitting by elements
  // would skip the element that straddles the end of the file
  b_size ofst        = var_resolve_index (&gparams.dest, bofst);
  t_size write_size  = size;
  b_size write_nelem = var_resolve_nelem (&gparams.dest, ofst, nelem, size, stride);
  bool   append      = false;

  if (stride == 1) {
    b_size nbytes;
    if (!safe_mul_u64 (&nbytes, nelem, size)) {
      return error_causef (
          e,
          ERR_ARITH,
          "write of %" PRb_size " elements of %" PRt_size " bytes: overflow",
          nelem,
          size
      );
    }
    write_size  = 1;
    write_nelem = MIN (nbytes, gparams.dest.nbytes - ofst);
    append      = write_nelem < nbytes;
  } else if (write_nelem < nelem) {
    return error_causef (e, ERR_INVALID_ARGUMENT, "Cannot write past end with stride != 1");
  }

  // WRITE
  struct ns_write_params wparams = {
      .p      = p,
      .src    = src,
      .tx     = tx,
      .root   = gparams.dest.rpt_root,
      .size   = write_size,
      .bofst  = ofst,
      .stride = stride,
      .nelem  = write_nelem,
  };

  // ns_rope_write reads nelem == 0 as "no limit", so skip it
  sb_size written = 0;
  if (write_nelem > 0) {
    written = ns_rope_write (wparams, e);
    if (written < 0) {
      return error_trace (e);
    }
  }

  // A stride of 1 counted bytes - back to elements of the caller's size
  sb_size ret = stride == 1 ? written / (sb_size)size : written;

  // INSERT REMAINDER
  // src is sequential: ns_rope_write consumed the bytes inside the file,
  // so ns_insert continues reading from where it left off.
  if (append) {
    struct ns_insert_params iparams = {
        .p     = p,
        .src   = src,
        .tx    = tx,
        .root  = wparams.root,
        .bofst = gparams.dest.nbytes, // Append
    };
    sb_size inserted = ns_insert (&iparams, e);
    if (inserted < 0) {
      return error_trace (e);
    }
    ret = (written + inserted) / (sb_size)size;

    // UPDATE VARIABLE
    if (ns_var_update_by_var_root (
            p,
            tx,
            gparams.dest.var_root,
            iparams.root,
            gparams.dest.nbytes + inserted,
            e
        )
        < 0) {
      return error_trace (e);
    }
  }

  return ret;
}

#ifndef NDEBUG
TEST (smartfiles_pwrite)
{
  struct pgr_fixture f;
  pgr_fixture_create (&f, &ctx->e);
  smartfiles_init_pager (f.p, &ctx->e);

  struct txn tx;
  pgr_begin_txn (&tx, f.p, &ctx->e);

  u8 buffer[8] = {1, 2, 3, 4, 5, 6, 7, 8};

  istream_create_from_array (in_init, buffer);
  smartfiles_insert (f.p, &tx, &in_init, 0, sizeof (buffer), &ctx->alloc, &ctx->e);

  // Overwrite the first 4 bytes in place.
  u8 overwrite[4] = {9, 9, 9, 9};
  istream_create_from_array (in_over, overwrite);
  sb_size
      n = smartfiles_write (f.p, &tx, &in_over, 1, 0, 1, sizeof (overwrite), &ctx->alloc, &ctx->e);

  test_assert_equal (n, sizeof (overwrite));
  test_assert_equal (smartfiles_size (f.p, &tx, &ctx->alloc, &ctx->e), sizeof (buffer));

  u8 out[8] = {0};

  ostream_create_from_array (output, out);
  smartfiles_read (f.p, &tx, &output, 1, 0, 1, sizeof (out), &ctx->alloc, &ctx->e);

  u8 expected[8] = {9, 9, 9, 9, 5, 6, 7, 8};
  test_assert (memcmp (out, expected, sizeof (expected)) == 0);

  // Writing past the end should append (insert the remainder).
  u8 append[4] = {11, 12, 13, 14};
  istream_create_from_array (in_append, append);
  n = smartfiles_write (
      f.p,
      &tx,
      &in_append,
      1,
      sizeof (buffer),
      1,
      sizeof (append),
      &ctx->alloc,
      &ctx->e
  );

  test_assert_equal (n, sizeof (append));
  test_assert_equal (
      smartfiles_size (f.p, &tx, &ctx->alloc, &ctx->e),
      sizeof (buffer) + sizeof (append)
  );

  pgr_commit (f.p, &tx, &ctx->e);

  arena_alloc_free_all (&ctx->alloc);
  pgr_fixture_teardown (&f, &ctx->e);
}
#endif
