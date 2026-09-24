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
/**
 * @file
 * @brief Rope algorithms
 *
 * Various numstore rope algorithms
 */

#ifndef NS_ROPE_ALGORITHMS_H
#define NS_ROPE_ALGORITHMS_H

#include "core/ns_stream.h"
#include "nscore/pager/ns_pager.h"

enum stride_phase
{
  ACTIVE,
  SKIPPING,
};

struct ns_insert_params
{
  // Parameters
  struct pager  *p;
  struct stream *src;
  struct txn    *tx;
  pgno           root;
  b_size         bofst;
  b_size         bytes;
};

struct ns_write_params
{
  struct pager  *p;
  struct stream *src;
  struct txn    *tx;
  pgno           root;
  t_size         size;
  b_size         bofst;
  sb_size        stride;
  b_size         nelem;
};

struct ns_read_params
{
  struct pager  *p;
  struct stream *dest;
  struct txn    *tx;
  pgno           root;
  t_size         size;
  b_size         bofst;
  sb_size        stride;
  b_size         nelem;
};

struct ns_remove_params
{
  struct pager  *p;
  struct stream *dest;
  struct txn    *tx;
  pgno           root;
  b_size         size;
  b_size         bofst;
  sb_size        stride;
  b_size         nelem;
};

sb_size ns_insert (struct ns_insert_params *params, error *e);
sb_size ns_write (struct ns_write_params params, error *e);
sb_size ns_read (struct ns_read_params params, error *e);
sb_size ns_remove (struct ns_remove_params *params, error *e);

#endif // NS_ROPE_ALGORITHMS_H
