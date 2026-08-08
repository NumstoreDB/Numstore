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

#ifndef ROPE_H
#define ROPE_H

#include "node_updates.h" // node_updates
#include "numstore.h"     // pgno
#include "pager.h"        // page_h
#include "pager.h"        // pager

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

struct seek_v
{
  page_h pg;
  p_size lidx;
};

struct ns_seek_params
{
  struct pager *p;
  struct txn   *tx;
  pgno          root;
  b_size        bofst;
  bool          save_stack;
  struct seek_v pstack[20];
  u32           sp;
  page_h        pg;
  p_size        lidx;
};

err_t ns_seek (struct ns_seek_params *a, error *e);

struct root_update
{
  pgno root;
  bool isroot;
};

struct ns_balance_and_release_params
{
  struct pager         *p;
  struct txn           *tx;
  struct three_in_pair *output;
  struct root_update   *root;
  page_h               *prev;
  page_h               *cur;
  page_h               *next;
};

err_t ns_balance_and_release (
    struct ns_balance_and_release_params params,
    error                               *e
);

struct ns_rebalance_params
{
  struct pager        *p;
  struct txn          *tx;
  pgno                 root;
  struct seek_v       *pstack;
  u32                  sp;
  struct node_updates *input;
  struct node_updates *output;
  struct root_update   layer_root;
  page_h               cur;
  page_h               limit;
  p_size               lidx;
};

err_t ns_rebalance (struct ns_rebalance_params *params, error *e);

// UNTESTED (ALL)
i32 ns_get_number_of_layers (struct pager *p, pgno root, error *e);
/**
i32 ns_get_length_to_the_right_of (struct pager *p, pgno pg, error *e);
i32 ns_get_length_to_the_left_of (struct pager *p, pgno pg, error *e);
i32 ns_get_length_of_layer_that_contains_node (struct pager *p, pgno pg, error
*e);
*/

#endif // ROPE_H
