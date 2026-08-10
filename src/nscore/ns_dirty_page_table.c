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

#include "nscore/ns_dirty_page_table.h"

#include <stddef.h>

#include "core/ns_concurrency.h"
#include "core/ns_csx_assert.h"
#include "core/ns_htable.h"
#include "core/ns_utils.h"
#include "core/os/ns_os.h"
#include "core/testing/ns_testing.h"

/*
 * Dirty page table entry.
 *
 * rec_lsn is the LSN of the first WAL record that dirtied this page (the
 * Recovery LSN).  The minimum rec_lsn across all DPT entries determines
 * the redo start point during ARIES recovery: any WAL record before that
 * LSN cannot affect a page that is still dirty, so it can be skipped.
 *
 * The latch protects rec_lsn and pg for concurrent access.  hnode embeds
 * the hash-table intrusion pointer; pg also doubles as the hash key.
 */
struct dpg_entry
{
  lsn          rec_lsn;
  pgno         pg;
  struct hnode node;
  latch        l;
};

#define DPGT_SERIAL_UNIT (sizeof (pgno) + sizeof (lsn))

static void
dpge_key_init (struct dpg_entry *dest, const pgno pg)
{
  dest->pg = pg;
  latch_init (&dest->l);
  hnode_init (&dest->node, pg);
}

static void
dpge_init (struct dpg_entry *dest, const pgno pg, const lsn rec_lsn)
{
  dest->pg      = pg;
  dest->rec_lsn = rec_lsn;
  latch_init (&dest->l);
  hnode_init (&dest->node, pg);
}

static bool
dpge_equals (const struct hnode *left, const struct hnode *right)
{
  // Might have passed the exact same ref as exists in the htable
  if (left == right)
  {
    return true;
  }

  // Otherwise, passed a key with just relevant information
  else
  {
    struct dpg_entry *_left  = container_of (left, struct dpg_entry, node);
    struct dpg_entry *_right = container_of (right, struct dpg_entry, node);

    latch_lock (&_left->l);
    latch_lock (&_right->l);

    bool ret = _left->pg == _right->pg;

    latch_unlock (&_right->l);
    latch_unlock (&_left->l);

    return ret;
  }
}

DEFINE_DBG_ASSERT (struct dpg_table, dirty_pg_table, d, { ASSERT (d); })

// Lifecycle
struct dpg_table *
dpgt_open (error *e)
{
  struct dpg_table *dest = i_malloc (1, sizeof *dest, e);
  if (dest == NULL)
  {
    goto failed;
  }
  slab_alloc_init (&dest->alloc, sizeof (struct dpg_entry), 1000);

  dest->t = htable_create (512, e);
  if (dest->t == NULL)
  {
    goto dest_failed;
  }

  return dest;

dest_failed:
  i_free (dest);
failed:
  return NULL;
}

void
dpgt_close (struct dpg_table *t)
{
  DBG_ASSERT (dirty_pg_table, t);
  slab_alloc_destroy (&t->alloc);
  htable_free (t->t);
  i_free (t);
}

struct dpgt_merge_ctx
{
  struct dpg_table *dest;
  error            *e;
};

static void
merge_dpge (const pgno pg, const lsn rec_lsn, void *vctx)
{
  const struct dpgt_merge_ctx *ctx = vctx;

  if (ctx->e->cause_code)
  {
    return;
  }

  if (dpgt_add (ctx->dest, pg, rec_lsn, ctx->e))
  {
    return;
  }
}

err_t
dpgt_merge_into (struct dpg_table *dest, struct dpg_table *src, error *e)
{
  struct dpgt_merge_ctx ctx = {
      .dest = dest,
      .e    = e,
  };

  dpgt_foreach (src, merge_dpge, &ctx);

  return ctx.e->cause_code;
}

static void
dpge_max (pgno pg, const lsn rec_lsn, void *ctx)
{
  lsn *min = ctx;

  if (rec_lsn < *min)
  {
    *min = rec_lsn;
  }
}

lsn
dpgt_min_rec_lsn (struct dpg_table *d)
{
  ASSERT (dpgt_get_size (d) > 0);
  lsn min = (lsn)-1;

  dpgt_foreach (d, dpge_max, &min);

  return min;
}

struct dpgt_foreach_ctx
{
  void (*action) (pgno pg, lsn rec_lsn, void *ctx);
  void *ctx;
};

static void
dpgt_hnode_foreach (struct hnode *node, void *ctx)
{
  const struct dpgt_foreach_ctx *_ctx  = ctx;
  struct dpg_entry              *entry = container_of (node, struct dpg_entry, node);

  latch_lock (&entry->l);

  pgno pg = entry->pg;
  lsn  l  = entry->rec_lsn;

  latch_unlock (&entry->l);

  _ctx->action (pg, l, _ctx->ctx);
}

void
dpgt_foreach (
    const struct dpg_table *t,
    void (*action) (pgno pg, lsn rec_lsn, void *ctx),
    void *ctx
)
{
  struct dpgt_foreach_ctx _ctx = {
      .action = action,
      .ctx    = ctx,
  };
  htable_foreach (t->t, dpgt_hnode_foreach, &_ctx);
}

u32
dpgt_get_size (const struct dpg_table *d)
{
  return htable_size (d->t);
}

bool
dpgt_exists (const struct dpg_table *t, const pgno pg)
{
  struct dpg_entry entry;
  dpge_key_init (&entry, pg);

  struct hnode **ret = htable_lookup (t->t, &entry.node, dpge_equals);

  return ret != NULL;
}

err_t
dpgt_add (struct dpg_table *t, const pgno pg, const lsn rec_lsn, error *e)
{
  DBG_ASSERT (dirty_pg_table, t);

  struct dpg_entry *v = slab_alloc_alloc (&t->alloc, e);
  if (v == NULL)
  {
    goto theend;
  }

  dpge_init (v, pg, rec_lsn);

  htable_insert (t->t, &v->node);

theend:
  return error_trace (e);
}

err_t
dpgt_add_if_ne (struct dpg_table *t, pgno pg, lsn rec_lsn, error *e)
{
  if (!dpgt_exists (t, pg))
  {
    return dpgt_add (t, pg, rec_lsn, e);
  }
  return SUCCESS;
}

bool
dpgt_get (lsn *dest, struct dpg_table *t, const pgno pg)
{
  DBG_ASSERT (dirty_pg_table, t);

  struct dpg_entry key;
  dpge_key_init (&key, pg);

  struct hnode **node = htable_lookup (t->t, &key.node, dpge_equals);
  if (node)
  {
    *dest = container_of (*node, struct dpg_entry, node)->rec_lsn;
  }

  return node != NULL;
}

void
dpgt_remove (bool *exists, struct dpg_table *t, const pgno pg)
{
  DBG_ASSERT (dirty_pg_table, t);

  struct dpg_entry key;
  dpge_key_init (&key, pg);

  struct hnode **node = htable_lookup (t->t, &key.node, dpge_equals);

  if (node == NULL)
  {
    *exists = false;
    return;
  }

  *exists = true;

  htable_delete (t->t, node);
}

void
dpgt_remove_expect (struct dpg_table *t, const pgno pg)
{
  DBG_ASSERT (dirty_pg_table, t);

  struct dpg_entry key;
  dpge_key_init (&key, pg);

  struct hnode **node = htable_lookup (t->t, &key.node, dpge_equals);
  ASSERT (node != NULL);
  htable_delete (t->t, node);
}

void
dpgt_update (struct dpg_table *t, const pgno pg, const lsn new_rec_lsn)
{
  struct dpg_entry key;
  dpge_key_init (&key, pg);

  {
    DBG_ASSERT (dirty_pg_table, t);

    struct hnode **node = htable_lookup (t->t, &key.node, dpge_equals);
    ASSERT (node != NULL);
    struct dpg_entry *entry = container_of (*node, struct dpg_entry, node);

    latch_lock (&entry->l);
    entry->rec_lsn = new_rec_lsn;
    latch_unlock (&entry->l);
  }
}

struct dpgt_eq_ctx
{
  struct dpg_table *other;
  bool              ret;
};

static void
dpgt_eq_foreach (struct hnode *node, void *_ctx)
{
  struct dpgt_eq_ctx *ctx = _ctx;
  if (ctx->ret == false)
  {
    return;
  }

  struct dpg_entry *entry = container_of (node, struct dpg_entry, node);
  struct dpg_entry  candidate;

  latch_lock (&entry->l);
  {
    dpge_key_init (&candidate, entry->pg);

    struct hnode **other_node = htable_lookup (ctx->other->t, &candidate.node, dpge_equals);

    if (other_node == NULL)
    {
      ctx->ret = false;
      goto theend;
    }

    struct dpg_entry *other = container_of (*other_node, struct dpg_entry, node);

    latch_lock (&other->l);
    {
      ASSERT (other->pg == entry->pg);
      ctx->ret = other->rec_lsn == entry->rec_lsn;
    }
    latch_unlock (&other->l);
  }

theend:
  latch_unlock (&entry->l);
}

bool
dpgt_equal (struct dpg_table *left, struct dpg_table *right)
{
  bool ret = false;

  if (htable_size (left->t) != htable_size (right->t))
  {
    goto theend;
  }

  struct dpgt_eq_ctx ctx = {
      .other = right,
      .ret   = true,
  };
  htable_foreach (left->t, dpgt_eq_foreach, &ctx);
  ret = ctx.ret;

theend:

  return ret;
}

void
dpgt_crash (struct dpg_table *t)
{
  DBG_ASSERT (dirty_pg_table, t);
  htable_free (t->t);
  slab_alloc_destroy (&t->alloc);
  i_free (t);
}

#ifdef TESTING
TEST (dpgt_open)
{
  TEST_CASE ("basic")
  {
    error             e = error_create ();
    struct dpg_table *t = dpgt_open (&e);
    dpgt_close (t);
  }

  TEST_CASE ("open multiple")
  {
    error e = error_create ();
    for (int i = 0; i < 4; ++i)
    {
      struct dpg_table *t = dpgt_open (&e);
      dpgt_close (t);
    }
  }
}

TEST (dpgt_merge_into)
{
  TEST_CASE ("empty to empty")
  {
    error             e      = error_create ();
    struct dpg_table *src    = dpgt_open (&e);
    struct dpg_table *dest   = dpgt_open (&e);
    const err_t       result = dpgt_merge_into (dest, src, &e);
    test_assert (result == SUCCESS);

    dpgt_close (dest);
    dpgt_close (src);
  }

  TEST_CASE ("data")
  {
    error             e    = error_create ();
    struct dpg_table *dest = dpgt_open (&e);
    struct dpg_table *src  = dpgt_open (&e);
    // Add to dest (pages 1-5)
    for (pgno pg = 1; pg <= 5; pg++)
    {
      dpgt_add (dest, pg, pg * 10, &e);
    }

    // Add to src (pages 6-10)
    for (pgno pg = 6; pg <= 10; pg++)
    {
      dpgt_add (src, pg, pg * 10, &e);
    }

    const err_t result = dpgt_merge_into (dest, src, &e);
    test_assert (result == SUCCESS);

    // Verify all pages exist in dest
    for (pgno pg = 1; pg <= 10; pg++)
    {
      test_assert (dpgt_exists (dest, pg));
    }

    dpgt_close (dest);
    dpgt_close (src);
  }

  TEST_CASE ("dest gets new rec_lsn on collision")
  {
    error             e    = error_create ();
    struct dpg_table *dest = dpgt_open (&e);
    struct dpg_table *src  = dpgt_open (&e);
    // Same page in both with different rec_lsn
    dpgt_add (dest, 42, 100, &e);
    dpgt_add (src, 42, 200, &e);

    dpgt_merge_into (dest, src, &e);

    lsn  rec_lsn;
    bool found = dpgt_get (&rec_lsn, dest, 42);
    test_assert (found);
    test_assert_int_equal (rec_lsn, 200);

    dpgt_close (dest);
    dpgt_close (src);
  }
}

TEST (dpgt_min_rec_lsn)
{
  TEST_CASE ("single entry")
  {
    error             e = error_create ();
    struct dpg_table *t = dpgt_open (&e);
    dpgt_add (t, 1, 50, &e);

    const lsn min = dpgt_min_rec_lsn (t);
    test_assert_int_equal (min, 50);

    dpgt_close (t);
  }

  TEST_CASE ("multiple entries")
  {
    error             e = error_create ();
    struct dpg_table *t = dpgt_open (&e);
    dpgt_add (t, 1, 100, &e);
    dpgt_add (t, 2, 25, &e);
    dpgt_add (t, 3, 75, &e);
    dpgt_add (t, 4, 50, &e);

    const lsn min = dpgt_min_rec_lsn (t);
    test_assert (min == 25);

    dpgt_close (t);
  }
}

TEST (dpgt_exists)
{
  TEST_CASE ("nonexistent returns false")
  {
    error             e = error_create ();
    struct dpg_table *t = dpgt_open (&e);
    test_assert (!dpgt_exists (t, 9999));

    dpgt_close (t);
  }

  TEST_CASE ("exists after add")
  {
    error             e = error_create ();
    struct dpg_table *t = dpgt_open (&e);
    test_assert (!dpgt_exists (t, 1100));
    dpgt_add (t, 1100, 500, &e);
    test_assert (dpgt_exists (t, 1100));

    dpgt_close (t);
  }
}

TEST (dpgt_add)
{
  TEST_CASE ("new entry")
  {
    error             e = error_create ();
    struct dpg_table *t = dpgt_open (&e);
    dpgt_add (t, 900, 100, &e);

    lsn  rec_lsn;
    bool found = dpgt_get (&rec_lsn, t, 900);
    test_assert (found);
    test_assert_int_equal (rec_lsn, 100);

    dpgt_close (t);
  }

  TEST_CASE ("multiple entries different pages")
  {
    error             e = error_create ();
    struct dpg_table *t = dpgt_open (&e);
    for (pgno pg = 1; pg <= 5; pg++)
    {
      dpgt_add (t, pg, pg * 10, &e);
    }

    for (pgno pg = 1; pg <= 5; pg++)
    {
      lsn  rec_lsn;
      bool found = dpgt_get (&rec_lsn, t, pg);
      test_assert (found);
      test_assert_int_equal (rec_lsn, pg * 10);
    }

    dpgt_close (t);
  }
}

TEST (dpgt_get)
{
  TEST_CASE ("nonexistent returns false")
  {
    error             e = error_create ();
    struct dpg_table *t = dpgt_open (&e);
    lsn               rec_lsn;
    bool              found = dpgt_get (&rec_lsn, t, 9999);
    test_assert (!found);

    dpgt_close (t);
  }

  TEST_CASE ("get rec_lsn")
  {
    error             e = error_create ();
    struct dpg_table *t = dpgt_open (&e);
    dpgt_add (t, 100, 50, &e);

    lsn  rec_lsn;
    bool found = dpgt_get (&rec_lsn, t, 100);
    test_assert (found);
    test_assert_int_equal (rec_lsn, 50);

    dpgt_close (t);
  }

  TEST_CASE ("update rec_lsn")
  {
    error             e = error_create ();
    struct dpg_table *t = dpgt_open (&e);
    dpgt_add (t, 600, 100, &e);

    dpgt_update (t, 600, 200);

    lsn  rec_lsn;
    bool found = dpgt_get (&rec_lsn, t, 600);
    test_assert (found);
    test_assert_int_equal (rec_lsn, 200);

    dpgt_close (t);
  }

  TEST_CASE ("multiple pages independent")
  {
    error             e = error_create ();
    struct dpg_table *t = dpgt_open (&e);
    dpgt_add (t, 1, 10, &e);
    dpgt_add (t, 2, 20, &e);
    dpgt_add (t, 3, 300, &e);

    lsn rec_lsn;

    dpgt_get (&rec_lsn, t, 1);
    test_assert_int_equal (rec_lsn, 10);

    dpgt_get (&rec_lsn, t, 2);
    test_assert_int_equal (rec_lsn, 20);

    dpgt_get (&rec_lsn, t, 3);
    test_assert_int_equal (rec_lsn, 300);

    dpgt_close (t);
  }
}

TEST (dpgt_remove)
{
  TEST_CASE ("remove existing")
  {
    error             e = error_create ();
    struct dpg_table *t = dpgt_open (&e);
    dpgt_add (t, 400, 100, &e);

    bool removed;
    dpgt_remove (&removed, t, 400);
    test_assert (removed);

    lsn  rec_lsn;
    bool found = dpgt_get (&rec_lsn, t, 400);
    test_assert (!found);

    dpgt_close (t);
  }

  TEST_CASE ("remove nonexistent")
  {
    error             e = error_create ();
    struct dpg_table *t = dpgt_open (&e);
    bool              removed;
    dpgt_remove (&removed, t, 500);
    test_assert (!removed);

    dpgt_close (t);
  }

  TEST_CASE ("double remove")
  {
    error             e = error_create ();
    struct dpg_table *t = dpgt_open (&e);
    dpgt_add (t, 100, 50, &e);

    bool removed;
    dpgt_remove (&removed, t, 100);
    test_assert (removed);

    dpgt_remove (&removed, t, 100);
    test_assert (!removed);

    dpgt_close (t);
  }

  TEST_CASE ("get fails after remove")
  {
    error             e = error_create ();
    struct dpg_table *t = dpgt_open (&e);
    dpgt_add (t, 200, 50, &e);

    bool removed;
    dpgt_remove (&removed, t, 200);
    test_assert (removed);

    lsn  rec_lsn;
    bool found = dpgt_get (&rec_lsn, t, 200);
    test_assert (!found);

    dpgt_close (t);
  }
}

TEST (dpgt_equal)
{
  TEST_CASE ("empty tables")
  {
    error             e  = error_create ();
    struct dpg_table *t1 = dpgt_open (&e);
    struct dpg_table *t2 = dpgt_open (&e);
    test_assert (dpgt_equal (t1, t2));

    dpgt_close (t1);
    dpgt_close (t2);
  }

  TEST_CASE ("same content")
  {
    error             e  = error_create ();
    struct dpg_table *t1 = dpgt_open (&e);
    struct dpg_table *t2 = dpgt_open (&e);
    for (pgno pg = 1; pg <= 5; pg++)
    {
      dpgt_add (t1, pg, pg * 10, &e);
      dpgt_add (t2, pg, pg * 10, &e);
    }

    test_assert (dpgt_equal (t1, t2));

    dpgt_close (t1);
    dpgt_close (t2);
  }

  TEST_CASE ("different rec_lsn")
  {
    error             e  = error_create ();
    struct dpg_table *t1 = dpgt_open (&e);
    struct dpg_table *t2 = dpgt_open (&e);
    dpgt_add (t1, 1, 10, &e);
    dpgt_add (t2, 1, 20, &e);

    test_assert (!dpgt_equal (t1, t2));

    dpgt_close (t1);
    dpgt_close (t2);
  }

  TEST_CASE ("different sizes")
  {
    error             e  = error_create ();
    struct dpg_table *t1 = dpgt_open (&e);
    struct dpg_table *t2 = dpgt_open (&e);
    dpgt_add (t1, 1, 10, &e);
    dpgt_add (t1, 2, 20, &e);
    dpgt_add (t2, 1, 10, &e);

    test_assert (!dpgt_equal (t1, t2));

    dpgt_close (t1);
    dpgt_close (t2);
  }

  TEST_CASE ("different pages same rec_lsn")
  {
    error             e  = error_create ();
    struct dpg_table *t1 = dpgt_open (&e);
    struct dpg_table *t2 = dpgt_open (&e);
    dpgt_add (t1, 1, 10, &e);
    dpgt_add (t2, 2, 10, &e);

    test_assert (!dpgt_equal (t1, t2));

    dpgt_close (t1);
    dpgt_close (t2);
  }
}
#endif

#ifdef TESTING
struct dpgt_thread_ctx
{
  struct dpg_table *table;
  volatile int      counter;
  pgno              start_pg;
  int               count;
};

static void *
dpgt_insert_thread (void *arg)
{
  struct dpgt_thread_ctx *ctx = arg;
  error                   e   = error_create ();

  for (int i = 0; i < ctx->count; i++)
  {
    const pgno pg      = ctx->start_pg + i;
    const lsn  rec_lsn = ctx->start_pg + i;

    if (dpgt_add (ctx->table, pg, rec_lsn, &e) == SUCCESS)
    {
      ctx->counter += 1;
    }
  }
  return NULL;
}

static void *
dpgt_reader_thread (void *arg)
{
  struct dpgt_thread_ctx *ctx = arg;

  for (int i = 0; i < ctx->count; i++)
  {
    lsn rec_lsn;
    if (dpgt_get (&rec_lsn, ctx->table, ctx->start_pg + i))
    {
      ctx->counter += 1;
    }
  }
  return NULL;
}

static void *
dpgt_updater_thread (void *arg)
{
  struct dpgt_thread_ctx *ctx = arg;

  for (int i = 0; i < ctx->count; i++)
  {
    const pgno pg = ctx->start_pg + i;

    if (dpgt_exists (ctx->table, pg))
    {
      dpgt_update (ctx->table, pg, ctx->start_pg + i + 1000);
      ctx->counter += 1;
    }
  }

  return NULL;
}

static void *
dpgt_remove_thread (void *arg)
{
  struct dpgt_thread_ctx *ctx = arg;

  for (int i = 0; i < ctx->count; i++)
  {
    bool removed;
    dpgt_remove (&removed, ctx->table, ctx->start_pg + i);

    if (removed)
    {
      ctx->counter += 1;
    }
  }

  return NULL;
}

TEST (dpgt_concurrent)
{
  TEST_CASE ("concurrent inserts")
  {
    error                  e    = error_create ();
    struct dpg_table      *t    = dpgt_open (&e);
    struct dpgt_thread_ctx ctx1 = {
        .table    = t,
        .start_pg = 0,
        .count    = 100,
        .counter  = 0,
    };
    struct dpgt_thread_ctx ctx2 = {
        .table    = t,
        .start_pg = 100,
        .count    = 100,
        .counter  = 0,
    };
    struct dpgt_thread_ctx ctx3 = {
        .table    = t,
        .start_pg = 200,
        .count    = 100,
        .counter  = 0,
    };

    i_thread t1, t2, t3;
    test_assert_equal (i_thread_create (&t1, dpgt_insert_thread, &ctx1, &e), SUCCESS);
    test_assert_equal (i_thread_create (&t2, dpgt_insert_thread, &ctx2, &e), SUCCESS);
    test_assert_equal (i_thread_create (&t3, dpgt_insert_thread, &ctx3, &e), SUCCESS);

    i_thread_join (&t1, &e);
    i_thread_join (&t2, &e);
    i_thread_join (&t3, &e);

    int total_inserts = ctx1.counter + ctx2.counter + ctx3.counter;
    test_assert_equal (total_inserts, 300);

    for (pgno pg = 0; pg < 300; pg++)
    {
      test_assert (dpgt_exists (t, pg));
    }

    dpgt_close (t);
  }

  TEST_CASE ("concurrent readers")
  {
    error             e = error_create ();
    struct dpg_table *t = dpgt_open (&e);
    // Pre-populate
    for (pgno pg = 0; pg < 200; pg++)
    {
      dpgt_add (t, pg, pg * 10, &e);
    }

    struct dpgt_thread_ctx ctx1 = {
        .table    = t,
        .start_pg = 0,
        .count    = 100,
        .counter  = 0,
    };
    struct dpgt_thread_ctx ctx2 = {
        .table    = t,
        .start_pg = 50,
        .count    = 100,
        .counter  = 0,
    };
    struct dpgt_thread_ctx ctx3 = {
        .table    = t,
        .start_pg = 100,
        .count    = 100,
        .counter  = 0,
    };

    i_thread t1, t2, t3;
    test_assert_equal (i_thread_create (&t1, dpgt_reader_thread, &ctx1, &e), SUCCESS);
    test_assert_equal (i_thread_create (&t2, dpgt_reader_thread, &ctx2, &e), SUCCESS);
    test_assert_equal (i_thread_create (&t3, dpgt_reader_thread, &ctx3, &e), SUCCESS);

    i_thread_join (&t1, &e);
    i_thread_join (&t2, &e);
    i_thread_join (&t3, &e);

    int total_reads = ctx1.counter + ctx2.counter + ctx3.counter;
    test_assert_equal (total_reads, 300);

    dpgt_close (t);
  }

  TEST_CASE ("concurrent updates")
  {
    error             e = error_create ();
    struct dpg_table *t = dpgt_open (&e);
    // Pre-populate
    for (pgno pg = 0; pg < 300; pg++)
    {
      dpgt_add (t, pg, pg * 10, &e);
    }

    struct dpgt_thread_ctx ctx1 = {
        .table    = t,
        .start_pg = 0,
        .count    = 100,
        .counter  = 0,
    };
    struct dpgt_thread_ctx ctx2 = {
        .table    = t,
        .start_pg = 100,
        .count    = 100,
        .counter  = 0,
    };
    struct dpgt_thread_ctx ctx3 = {
        .table    = t,
        .start_pg = 200,
        .count    = 100,
        .counter  = 0,
    };

    i_thread t1, t2, t3;
    test_assert_equal (i_thread_create (&t1, dpgt_updater_thread, &ctx1, &e), SUCCESS);
    test_assert_equal (i_thread_create (&t2, dpgt_updater_thread, &ctx2, &e), SUCCESS);
    test_assert_equal (i_thread_create (&t3, dpgt_updater_thread, &ctx3, &e), SUCCESS);

    i_thread_join (&t1, &e);
    i_thread_join (&t2, &e);
    i_thread_join (&t3, &e);

    int total_updates = ctx1.counter + ctx2.counter + ctx3.counter;
    test_assert_equal (total_updates, 300);

    // Verify updates
    for (pgno pg = 0; pg < 300; pg++)
    {
      lsn rec_lsn;
      test_assert (dpgt_get (&rec_lsn, t, pg));
      test_assert_equal (rec_lsn, pg + 1000);
    }

    dpgt_close (t);
  }

  TEST_CASE ("concurrent removes")
  {
    error             e = error_create ();
    struct dpg_table *t = dpgt_open (&e);
    // Pre-populate
    for (pgno pg = 0; pg < 300; pg++)
    {
      dpgt_add (t, pg, pg * 10, &e);
    }

    struct dpgt_thread_ctx ctx1 = {
        .table    = t,
        .start_pg = 0,
        .count    = 100,
        .counter  = 0,
    };
    struct dpgt_thread_ctx ctx2 = {
        .table    = t,
        .start_pg = 100,
        .count    = 100,
        .counter  = 0,
    };
    struct dpgt_thread_ctx ctx3 = {
        .table    = t,
        .start_pg = 200,
        .count    = 100,
        .counter  = 0,
    };

    i_thread t1, t2, t3;
    test_assert_equal (i_thread_create (&t1, dpgt_remove_thread, &ctx1, &e), SUCCESS);
    test_assert_equal (i_thread_create (&t2, dpgt_remove_thread, &ctx2, &e), SUCCESS);
    test_assert_equal (i_thread_create (&t3, dpgt_remove_thread, &ctx3, &e), SUCCESS);

    i_thread_join (&t1, &e);
    i_thread_join (&t2, &e);
    i_thread_join (&t3, &e);

    int total_removes = ctx1.counter + ctx2.counter + ctx3.counter;
    test_assert_equal (total_removes, 300);

    for (pgno pg = 0; pg < 300; pg++)
    {
      test_assert (!dpgt_exists (t, pg));
    }

    dpgt_close (t);
  }

  TEST_CASE ("concurrent insert and read")
  {
    error             e = error_create ();
    struct dpg_table *t = dpgt_open (&e);
    // Pre-populate half so readers have something to find
    for (pgno pg = 0; pg < 100; pg++)
    {
      dpgt_add (t, pg, pg * 10, &e);
    }

    struct dpgt_thread_ctx insert_ctx = {
        .table    = t,
        .start_pg = 100,
        .count    = 100,
        .counter  = 0,
    };
    struct dpgt_thread_ctx read_ctx1 = {
        .table    = t,
        .start_pg = 0,
        .count    = 100,
        .counter  = 0,
    };
    struct dpgt_thread_ctx read_ctx2 = {
        .table    = t,
        .start_pg = 0,
        .count    = 100,
        .counter  = 0,
    };

    i_thread t1, t2, t3;
    test_assert_equal (i_thread_create (&t1, dpgt_insert_thread, &insert_ctx, &e), SUCCESS);
    test_assert_equal (i_thread_create (&t2, dpgt_reader_thread, &read_ctx1, &e), SUCCESS);
    test_assert_equal (i_thread_create (&t3, dpgt_reader_thread, &read_ctx2, &e), SUCCESS);

    i_thread_join (&t1, &e);
    i_thread_join (&t2, &e);
    i_thread_join (&t3, &e);

    // All inserts must have succeeded
    test_assert_equal (insert_ctx.counter, 100);

    // All pre-populated pages must still be present
    for (pgno pg = 0; pg < 100; pg++)
    {
      test_assert (dpgt_exists (t, pg));
    }

    // All inserted pages must be present
    for (pgno pg = 100; pg < 200; pg++)
    {
      test_assert (dpgt_exists (t, pg));
    }

    dpgt_close (t);
  }
}
#endif
