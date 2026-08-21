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

#ifndef NS_NODE_UPDATES_H
#define NS_NODE_UPDATES_H

#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "core/os/ns_memory.h"
#include "nscore/page/ns_page_h.h"
#include "nscore/page/ns_page_inner_node.h"

/**
 * A buffer meant to mimic the structure of a chain of inner nodes
 * in memory - used specifically for re writing inner node layers.
 *
 * Layout is a left right and pivot of inner node keys (keys and values)
 *
 * [ ...  left  ... ] PIVOT [ ... right ... ]
 *
 * Both sides track three counters:
 *
 * Each side tracks three counters, cons <= obs <= len:
 *  - len:   total number of entries currently stored
 *
 *  - obs:   to update inner node layers, you observe inner nodes
 *           in order to ensure that there are no duplicates
 *           and no missed pages. This is the maximum observed
 *           node. We can't pass obs with cons, or else we would
 *           be appending new keys without ensuring that they are
 *           duplicates
 *
 *  - cons:  entries popped off the node updates and written into the
 *           new tree
 *
 *    cons <= obs <= len
 *
 * [ ...  left  ... ] PIVOT [ ... right ... ]
 *   ^      ^   ^              ^     ^    ^
 *  len    obs cons           cons  obs  len
 */

#define pgh_unravel(pg) page_h_pgno (pg), dlgt_get_size (page_h_ro (pg))
struct node_updates *nupd_init (pgno pg, b_size size, struct i_mem mem, error *e);
struct node_updates *nupd_random_from (
    pgno        *left,
    u32          llen,
    pgno         pivot,
    pgno        *right,
    u32          rlen,
    struct i_mem mem,
    error       *e
);

void nupd_reset (struct node_updates *ret, pgno pg, b_size size);
void nupd_free (struct node_updates *n);
pgno nupd_pivot_pg (const struct node_updates *n);

/*
 * Commit(pg, size):
 *   Sets prev if it's available, otherwise, just
 *   appends. We keep track of prev because various
 *   algorithms hold onto nodes and continue forward
 *
 *      if prev != NULL:
 *          ASSERT(prev->pg == pg);
 *          prev->key = size;
 *          prev = NULL;
 *      else:
 *          append(pg, size);
 */
err_t nupd_commit_1st_right (struct node_updates *s, pgno pg, b_size size, error *e);
err_t nupd_commit_1st_left (struct node_updates *s, pgno pg, b_size size, error *e);

/*
 * Append(pg, size)
 *    Adds a single entry. If nothing has been added to this
 *    side yet and pg matches the pivot, it's really just an
 *    update to the pivot's key, not a new entry.
 *
 *      if len == 0 and pg == pivot.pg:
 *          pivot.key = size;
 *          return &pivot;
 *      else:
 *          push(pg, size);   // len++
 *
 * Append_2nd(pg1, size1, pg2, size2)
 *    Append 2 entries, but the first might
 *    already be pending from a previous call
 *
 *      if prev != NULL:
 *          ASSERT(prev->pg == pg1);
 *      else:
 *          prev = append(pg1, size1);
 *      append(pg2, size2);
 *
 * Append Tip(prev, cur, next)
 *    Consumes a 3 page window - which
 *    are adjacent in the tree
 *
 *      commit(cur.pg, cur.key)
 *      if prev != NULL:
 *          entry = search "backwards" until entry->pg == prev.pg;
 *          if entry != NULL:
 *              entry->key = prev.key;
 *          else:
 *              append_<opposite direction>(prev.pg, prev.key);
 *      if next != NULL:
 *          append_<direction>(next.pg, next.key);
 */
p_size nupd_append_maximally_left (struct node_updates *n, const page_h *pg, p_size rlen);
p_size nupd_append_maximally_right (struct node_updates *n, const page_h *pg, p_size llen);
p_size nupd_append_maximally_right_then_left (struct node_updates *n, page_h *pg);
err_t nupd_append_tip_right (struct node_updates *s, struct three_in_pair output, error *e);
err_t nupd_append_tip_left (struct node_updates *s, struct three_in_pair output, error *e);
err_t nupd_append_2nd_right (
    struct node_updates *s,
    pgno                 pg1,
    b_size               size1,
    pgno                 pg2,
    b_size               size2,
    error               *e
);
err_t nupd_append_2nd_left (
    struct node_updates *s,
    pgno                 pg1,
    b_size               size1,
    pgno                 pg2,
    b_size               size2,
    error               *e
);

/**
 * Observe(pg, key)
 *    Reconciles one page against what's already stored, walking
 *    forward from wherever obs left off last time. If it matches
 *    the pivot before anything's been observed, there's nothing
 *    to do. Otherwise scan the unobserved tail for a duplicate;
 *    if none is found, it's a new page and gets appended.
 *
 *      if obs == 0 and pivot.pg == pg:
 *          return;
 *      while obs < len:
 *          if get(obs)->pg == pg:
 *              return;
 *          obs++;
 *      push(pg, key);
 *      obs++;
 */
err_t nupd_observe_pivot (struct node_updates *s, page_h *pg, p_size lidx, error *e);
err_t nupd_observe_right_from (struct node_updates *s, const page_h *pg, p_size lidx, error *e);
err_t nupd_observe_left_from (struct node_updates *s, const page_h *pg, p_size lidx, error *e);
err_t nupd_observe_all_right (struct node_updates *s, const page_h *pg, error *e);
err_t nupd_observe_all_left (struct node_updates *s, const page_h *pg, error *e);

/**
 * Consume()
 *    Pops the next observed-but-unconsumed entry, in the same
 *    order it was appended/observed (closest to the pivot first).
 *
 *      ASSERT(cons < obs);
 *      return get(cons++);
 */
struct in_pair nupd_consume_right (struct node_updates *s);
struct in_pair nupd_consume_left (struct node_updates *s);

/**
 * Completeness checks
 *
 * "done observing" means obs == len (nothing left unobserved).
 * "done consuming" means cons == obs (nothing observed left to consume).
 * "done" (left/right) is both at once - fully observed and fully
 * consumed, i.e. that side is entirely spent and safe to move on from.
 */
bool nupd_done_observing_left (const struct node_updates *s);
bool nupd_done_observing_right (const struct node_updates *s);
bool nupd_done_consuming_left (const struct node_updates *s);
bool nupd_done_consuming_right (const struct node_updates *s);
bool nupd_done_left (struct node_updates *s);
bool nupd_done_right (struct node_updates *s);

#endif // NS_NODE_UPDATES_H
