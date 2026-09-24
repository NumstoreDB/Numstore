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
smartfiles_remove (
    struct pager       *p,
    struct txn         *tx,
    struct stream      *dest,
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
    return error_causef (e, ERR_INVALID_ARGUMENT, "Cannot remove with stride == 0");
  }
  if (size == 0) {
    return error_causef (e, ERR_INVALID_ARGUMENT, "Cannot remove with size == 0");
  }
  if (nelem == 0) {
    return 0;
  }

  // GET VARIABLE
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
  b_size ofst = var_resolve_index (&gparams.dest, bofst);
  nelem       = var_resolve_nelem (&gparams.dest, ofst, nelem, size);
  if (nelem == 0) {
    return 0;
  }

  // REMOVE
  struct ns_remove_params rparams = {
      .p      = p,
      .dest   = dest,
      .tx     = tx,
      .root   = gparams.dest.rpt_root,
      .size   = size,
      .bofst  = ofst,
      .stride = stride,
      .nelem  = nelem,
  };
  sb_size ret = ns_remove (&rparams, e);
  if (ret < 0) {
    return error_trace (e);
  }

  // UPDATE VARIABLE
  struct ns_var_update_params uparams = {
      .p      = p,
      .tx     = tx,
      .retr   = (struct var_retrieval){.type = VR_PG, .root = gparams.dest.var_root},
      .newpg  = rparams.root,
      .nbytes = gparams.dest.nbytes - (ret * size),
  };
  if (ns_var_update (uparams, e)) {
    return error_trace (e);
  }

  return ret;
}

#ifdef TESTING
TEST (smartfiles_remove)
{
  struct pgr_fixture f;
  pgr_fixture_create (&f);
  smartfiles_init_pager (f.p, &f.e);

  ALLOC_INIT (temp);

  struct txn tx;
  pgr_begin_txn (&tx, f.p, &f.e);

  u8 buffer[16];
  for (u32 i = 0; i < sizeof (buffer); i++) {
    buffer[i] = (u8)i;
  }

  istream_create_from_array (input, buffer);
  smartfiles_insert (f.p, &tx, &input, 0, sizeof (buffer), &temp, &f.e);
  test_assert_equal (smartfiles_size (f.p, &tx, &temp, &f.e), sizeof (buffer));

  u8 out[16] = {0};

  ostream_create_from_array (output, out);
  sb_size n = smartfiles_remove (f.p, &tx, &output, 1, 0, 1, sizeof (buffer), &temp, &f.e);

  test_assert_equal (n, sizeof (buffer));
  test_assert (memcmp (out, buffer, sizeof (buffer)) == 0);
  test_assert_equal (smartfiles_size (f.p, &tx, &temp, &f.e), 0);

  pgr_commit (f.p, &tx, &f.e);

  ALLOC_CLOSE (temp);
}
#endif
