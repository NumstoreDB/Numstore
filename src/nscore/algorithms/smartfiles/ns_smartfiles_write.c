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
#include "core/ns_stdtypes.h"
#include "core/ns_stream.h"
#include "core/ns_string.h"
#include "core/testing/ns_testing.h"
#include "nscore/algorithms/rope/ns_rope_algorithms.h"
#include "nscore/algorithms/smartfiles/ns_smartfiles_algorithms.h"
#include "nscore/algorithms/var/ns_var_algorithms.h"
#include "nscore/page/ns_page_fixture.h"
#include "nscore/pager/ns_pager.h"
#include "nscore/types/ns_types.h"
#include "nscore/variables/ns_variables.h"

sb_size
smartfiles_write (
    struct pager       *p,
    struct ns_txn      *tx,
    struct stream      *src,
    t_size              size,
    sb_size             bofst,
    sb_size             stride,
    b_size              nelem,
    struct arena_alloc *alloc,
    error              *e
)
{
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
  if (ns_var_get (&gparams, e)) {
    return error_trace (e);
  }

  // Resolve sizes
  b_size ofst         = var_resolve_index (&gparams.dest, bofst);
  b_size write_nelem  = var_resolve_nelem (&gparams.dest, ofst, nelem, size);
  b_size insert_nelem = nelem - write_nelem;
  if (insert_nelem > 0 && stride != 1) {
    return error_causef (e, ERR_INVALID_ARGUMENT, "Cannot write past end with stride != 1");
  }

  // WRITE
  struct ns_write_params wparams = {
      .p      = p,
      .src    = src,
      .tx     = tx,
      .root   = gparams.dest.rpt_root,
      .size   = size,
      .bofst  = ofst,
      .stride = stride,
      .nelem  = write_nelem,
  };
  sb_size ret = ns_write (wparams, e);
  if (ret < 0) {
    return error_trace (e);
  }

  // INSERT REMAINDER
  // src is sequential: ns_write consumed the first write_nelem elements,
  // so ns_insert continues reading from where it left off.
  if (insert_nelem > 0) {
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
    ret += inserted / size;

    struct ns_var_update_params uparams = {
        .p      = p,
        .tx     = tx,
        .retr   = (struct var_retrieval){.type = VR_PG, .root = gparams.dest.var_root},
        .newpg  = iparams.root,
        .nbytes = gparams.dest.nbytes + inserted,
    };
    if (ns_var_update (uparams, e)) {
      return error_trace (e);
    }
  }

  return ret;
}

#ifdef TESTING
TEST (smartfiles_pwrite)
{
  struct pgr_fixture f;
  pgr_fixture_create (&f);
  smartfiles_init_pager (f.p, &f.e);

  ALLOC_INIT (temp);

  struct ns_txn tx;
  pgr_begin_txn (&tx, f.p, &f.e);

  u8 buffer[8] = {1, 2, 3, 4, 5, 6, 7, 8};

  istream_create_from_array (in_init, buffer);
  smartfiles_insert (f.p, &tx, &in_init, 0, sizeof (buffer), &temp, &f.e);

  // Overwrite the first 4 bytes in place.
  u8 overwrite[4] = {9, 9, 9, 9};
  istream_create_from_array (in_over, overwrite);
  sb_size n = smartfiles_write (f.p, &tx, &in_over, 1, 0, 1, sizeof (overwrite), &temp, &f.e);

  test_assert_equal (n, sizeof (overwrite));
  test_assert_equal (smartfiles_size (f.p, &tx, &temp, &f.e), sizeof (buffer));

  u8 out[8] = {0};

  ostream_create_from_array (output, out);
  smartfiles_read (f.p, &tx, &output, 1, 0, 1, sizeof (out), &temp, &f.e);

  u8 expected[8] = {9, 9, 9, 9, 5, 6, 7, 8};
  test_assert (memcmp (out, expected, sizeof (expected)) == 0);

  // Writing past the end should append (insert the remainder).
  u8 append[4] = {11, 12, 13, 14};
  istream_create_from_array (in_append, append);
  n = smartfiles_write (f.p, &tx, &in_append, 1, sizeof (buffer), 1, sizeof (append), &temp, &f.e);

  test_assert_equal (n, sizeof (append));
  test_assert_equal (smartfiles_size (f.p, &tx, &temp, &f.e), sizeof (buffer) + sizeof (append));

  pgr_commit (f.p, &tx, &f.e);

  ALLOC_CLOSE (temp);
}
#endif
