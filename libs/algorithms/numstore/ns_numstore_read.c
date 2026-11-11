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

#include "ns_error.h"
#include "numstore/ns_numstore_algorithms.h"
#include "rope/ns_rope_algorithms.h"

sb_size
numstore_read (
    struct pager      *p,
    struct txn        *tx,
    struct variable   *var,
    struct user_stride ustr,
    struct stream     *dest,
    error             *e
)
{
  DBG_ASSERT (pager, p);
  DBG_ASSERT (ns_txn, tx);
  DBG_ASSERT (variable, var);
  DBG_ASSERT (user_stride, &ustr);
  DBG_ASSERT (stream, dest);
  DBG_ASSERT (clean_error, e);

  // Resolve sizes
  t_size tsize = type_byte_size (var->dtype);
  ASSERT (var->nbytes % tsize == 0); // Variable is valid (from assert)
  b_size        len = var->nbytes / tsize;

  // Resolve length based on the stride
  struct stride stride; // Resolved stride
  if (stride_resolve (&stride, ustr, len, e) < 0) {
    return error_trace (e);
  }

  // The step started as a positive i64, so it fits in sb_size
  ASSERT (stride.stride <= (u64)I64_MAX);

  // An empty range - the rope takes nelem == 0 as "no limit"
  if (stride.nelems == 0) {
    return 0;
  }

  return ns_rope_read (
      (struct ns_read_params){
          .p      = p,
          .dest   = dest,
          .tx     = tx,
          .root   = var->rpt_root,
          .size   = tsize,
          .bofst  = tsize * stride.start,
          .stride = (sb_size)stride.stride,
          .nelem  = stride.nelems,
      },
      e
  );
}
