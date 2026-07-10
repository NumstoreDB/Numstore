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

#include "lock_table.h"

#include "txn_table.h"

#ifdef TESTING
#  include "testing/testing.h"
#endif

/******************************************************************************
 * SECTION: Lt Lock
 ******************************************************************************/

u32
lt_lock_key (const struct lt_lock lock)
{
  // The containing buffer
  char     hcode[sizeof (u8)];
  u32      hcodelen = 0;
  const u8 _type    = lock.type;

  memcpy (&hcode[hcodelen], &_type, sizeof (_type));
  hcodelen += sizeof (_type);

  switch (lock.type)
  {
    case LOCK_DB:
    {
      break;
    }
  }

  const struct string lock_type_hcode = {
      .data = hcode,
      .len  = hcodelen,
  };

  return fnv1a_hash (lock_type_hcode);
}

bool
lt_lock_equal (const struct lt_lock left, const struct lt_lock right)
{
  if (left.type != right.type)
  {
    return false;
  }

  switch (left.type)
  {
    case LOCK_DB:
    {
      return true;
    }
  }
  UNREACHABLE (); // LCOV_EXCL_LINE
}

bool
get_parent (struct lt_lock *parent, const struct lt_lock lock)
{
  switch (lock.type)
  {
    case LOCK_DB:
    {
      return false;
    }
  }

  UNREACHABLE (); // LCOV_EXCL_LINE
}

/******************************************************************************
 * SECTION: Lock Table
 ******************************************************************************/

struct lockt_frame
{
  struct lt_lock key;
  struct gr_lock lock;
  struct hnode   node;
  int            refcount;
};

static err_t
lockt_frame_init (struct lockt_frame *dest, const struct lt_lock key, error *e)
{
  WRAP (gr_lock_init (&dest->lock, e));

  dest->key      = key;
  dest->refcount = 0;
  hnode_init (&dest->node, lt_lock_key (key));

  return SUCCESS;
}

static void
lockt_frame_destroy (struct lockt_frame *dest)
{
  gr_lock_destroy (&dest->lock);
}

static void
lockt_frame_init_key (struct lockt_frame *dest, const struct lt_lock key)
{
  dest->key = key;
  hnode_init (&dest->node, lt_lock_key (key));
}

static bool
lockt_frame_eq (const struct hnode *left, const struct hnode *right)
{
  const struct lockt_frame *_left =
      container_of (left, struct lockt_frame, node);
  const struct lockt_frame *_right =
      container_of (right, struct lockt_frame, node);
  return lt_lock_equal (_left->key, _right->key);
}

static void
frame_ref (struct lockt_frame *frame)
{
  frame->refcount++;
}

static void
frame_unref (struct lockt *t, struct lockt_frame *frame)
{
  ASSERT (frame->refcount > 0);
  frame->refcount--;

  if (frame->refcount == 0)
  {
    struct hnode **found =
        htable_lookup (t->table, &frame->node, lockt_frame_eq);
    htable_delete (t->table, found);
    lockt_frame_destroy (frame);
    slab_alloc_free (&t->lock_alloc, frame);
  }
}

err_t
lockt_init (struct lockt *t, error *e)
{
  slab_alloc_init (&t->lock_alloc, sizeof (struct lockt_frame), 1000);

  t->table = htable_create (1000, e);
  if (t->table == NULL)
  {
    return error_trace (e);
  }

  latch_init (&t->l);

  return SUCCESS;
}

void
lockt_destroy (struct lockt *t)
{
  slab_alloc_destroy (&t->lock_alloc);
  htable_free (t->table);
}

static err_t
lockt_lock_once (
    struct lockt        *t,
    const struct lt_lock lock,
    const enum lock_mode mode,
    struct txn          *tx,
    error               *e
)
{
  /**
  // Fast path: if this transaction already holds this lock, skip.
  if (tx && txn_haslock (tx, lock))
    {
      return SUCCESS;
    }
  */

  latch_lock (&t->l);

  // Find the existing lock
  struct lockt_frame key;
  lockt_frame_init_key (&key, lock);
  struct hnode **node = htable_lookup (t->table, &key.node, lockt_frame_eq);

  // Result pointer
  struct lockt_frame *frame;

  if (node != NULL)
  {
    // Existing frame
    frame = container_of (*node, struct lockt_frame, node);
  }
  else
  {
    // Allocate and insert a new frame
    frame = slab_alloc_alloc (&t->lock_alloc, e);
    if (e->cause_code != SUCCESS)
    {
      latch_unlock (&t->l);
      return error_trace (e);
    }

    if (lockt_frame_init (frame, lock, e) != SUCCESS)
    {
      slab_alloc_free (&t->lock_alloc, frame);
      latch_unlock (&t->l);
      return error_trace (e);
    }

    htable_insert (t->table, &frame->node);
  }

  frame_ref (frame);

  // Record in the transaction while we still hold the latch.
  if (tx)
  {
    if (txn_newlock (tx, lock, mode, e) != SUCCESS)
    {
      frame_unref (t, frame);
      latch_unlock (&t->l);
      return error_trace (e);
    }
  }

  latch_unlock (&t->l);

  // Call the lock function - this is the big stuff
  if (gr_lock (&frame->lock, mode, e) != SUCCESS)
  {
    latch_lock (&t->l);
    frame_unref (t, frame);
    latch_unlock (&t->l);
    return error_trace (e);
  }

  return SUCCESS;
}

err_t
lockt_lock (
    struct lockt        *t,
    const struct lt_lock lock,
    const enum lock_mode mode,
    struct txn          *tx,
    error               *e
)
{
  // Fetch and lock the parent lock first if there is one
  struct lt_lock parent;
  if (get_parent (&parent, lock))
  {
    const enum lock_mode pmode = get_parent_mode (mode);
    // TODO - error handling here for failed locks
    WRAP (lockt_lock (t, parent, pmode, tx, e));
  }

  return lockt_lock_once (t, lock, mode, tx, e);
}

static void
lockt_unlock_once (
    struct lockt        *t,
    const struct lt_lock lock,
    const enum lock_mode mode
)
{
  latch_lock (&t->l);
  {
    // Search for the node
    struct lockt_frame key;
    lockt_frame_init_key (&key, lock);
    struct hnode **found = htable_lookup (t->table, &key.node, lockt_frame_eq);
    ASSERT (found);
    struct lockt_frame *frame = container_of (*found, struct lockt_frame, node);

    // Unlock it
    gr_unlock (&frame->lock, mode);

    // Unreference it
    frame_unref (t, frame);
  }
  latch_unlock (&t->l);
}

err_t
lockt_unlock (
    struct lockt        *t,
    const struct lt_lock lock,
    const enum lock_mode mode,
    error               *e
)
{
  // Unlock the child first (bottom-up).
  lockt_unlock_once (t, lock, mode);

  // Then unlock the parent.
  struct lt_lock parent;
  if (get_parent (&parent, lock))
  {
    const enum lock_mode pmode = get_parent_mode (mode);
    WRAP (lockt_unlock (t, parent, pmode, e));
  }

  return SUCCESS;
}

struct unlock_ctx
{
  struct lockt *t;
};

static void
unlock_single_lock (
    const struct lt_lock lock,
    const enum lock_mode mode,
    void                *ctx
)
{
  const struct unlock_ctx *c = ctx;
  struct lockt            *t = c->t;

  struct lockt_frame key;
  lockt_frame_init_key (&key, lock);

  struct hnode **found = htable_lookup (t->table, &key.node, lockt_frame_eq);
  ASSERT (found);

  struct lockt_frame *frame = container_of (*found, struct lockt_frame, node);

  gr_unlock (&frame->lock, mode);
  frame_unref (t, frame);
}

void
lockt_unlock_tx (struct lockt *t, struct txn *tx)
{
  ASSERT (t);
  ASSERT (tx);

  latch_lock (&t->l);

  struct unlock_ctx ctx = {.t = t};
  txn_foreach_lock (tx, unlock_single_lock, &ctx);

  latch_unlock (&t->l);
  txn_close (tx);
}
