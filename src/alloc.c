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

#include "alloc.h"

#include <stdatomic.h>
#include <string.h>

#include "concurrency.h"
#include "csx_assert.h"
#include "error.h"
#include "numerics.h"
#include "os.h"

#ifdef TESTING
#  include "testing/testing.h"
#endif

/******************************************************************************
 * SECTION: Local Linear Allocator
 ******************************************************************************/

/**
 * @struct lalloc
 * @brief A local arena allocator
 *
 * An allocator that allocates from a fixed size buffer
 * provided by the user
 *
 * @var lalloc::latch
 * @brief The latch to maintain thread safety
 *
 * @var lalloc::used
 * @brief How many bytes have been used
 *
 * @var lalloc::limit
 * @brief The maximum number of bytes available
 *
 * @var lalloc::data
 * @brief The buffer that holds all the data
 */
struct lalloc
{
  latch latch;
  u32   used;
  u32   limit;
  u8   *data;
};

DEFINE_DBG_ASSERT (struct lalloc, lalloc, l, {
  ASSERT (l);
  ASSERT (l->data);
  ASSERT (l->used <= l->limit);
})

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static struct lalloc
lalloc_create (u8 *data, const u32 limit)
{
  ASSERT (limit > 0);
  struct lalloc ret = {
      .used  = 0,
      .limit = limit,
      .data  = data,
  };
  latch_init (&ret.latch);
  DBG_ASSERT (lalloc, &ret);
  return ret;
}

static void *
lmalloc (struct lalloc *a, const u32 req, const u32 size, error *e)
{
  latch_lock (&a->latch);

  DBG_ASSERT (lalloc, a);
  ASSERT (req > 0);
  ASSERT (size > 0);

  u32 total;
  if (!safe_mul_u32 (&total, req, size))
  {
    error_causef (e, ERR_NOMEM, "alloc %d*%d: overflow", req, size);
    latch_unlock (&a->latch);
    return NULL;
  }

  const u32 avail = a->limit - a->used;
  if (avail <= total)
  {
    error_causef (
        e,
        ERR_NOMEM,
        "linear alloc %d bytes: only %d remaining",
        total,
        avail
    );
    latch_unlock (&a->latch);
    return NULL;
  }

  void *ret = &a->data[a->used];
  a->used   = a->used + total;

  latch_unlock (&a->latch);

  return ret;
}

/******************************************************************************
 * SECTION: Slab Allocator
 ******************************************************************************/

struct slab
{
  void        *freelist;
  struct slab *next;
  struct slab *prev;
  u32          used;
  u32          total_size;
  u8           data[];
};

void
slab_alloc_init (struct slab_alloc *dest, u32 size, const u32 cap_per_slab)
{
  ASSERT (size >= sizeof (void *));
  ASSERT (cap_per_slab > 0);

  // Align size to pointer boundary for better performance
  size = (size + sizeof (void *) - 1) & ~(sizeof (void *) - 1);

  *dest = (struct slab_alloc){
      .head         = NULL,
      .current      = NULL,
      .size         = size,
      .cap_per_slab = cap_per_slab,
  };
  latch_init (&dest->l);
}

void
slab_alloc_destroy (struct slab_alloc *alloc)
{
  ASSERT (alloc);

  struct slab *s = alloc->head;
  while (s)
  {
    struct slab *next = s->next;
    i_free (s);
    s = next;
  }

  alloc->head    = NULL;
  alloc->current = NULL;
}

static struct slab *
slab_alloc_extend (struct slab_alloc *alloc, error *e)
{
  const u32 data_size  = alloc->size * alloc->cap_per_slab;
  const u32 total_size = data_size + sizeof (struct slab);

  struct slab *slab = i_malloc (1, total_size, e);
  if (slab == NULL)
  {
    return NULL;
  }

  slab->total_size = total_size;

  // Link at head
  slab->prev = NULL;
  slab->next = alloc->head;
  if (alloc->head)
  {
    ASSERT (alloc->head->prev == NULL);
    alloc->head->prev = slab;
  }
  alloc->head = slab;
  slab->used  = 0;

  // Initialize freelist
  slab->freelist = slab->data;
  u8 *cur        = slab->data;
  for (u32 i = 0; i < alloc->cap_per_slab - 1; ++i)
  {
    u8 *next      = cur + alloc->size;
    *(void **)cur = next;
    cur           = next;
  }
  *(void **)cur = NULL;

  return slab;
}

void *
slab_alloc_alloc (struct slab_alloc *alloc, error *e)
{
  ASSERT (alloc);

  void *ret = NULL;
  latch_lock (&alloc->l);

  struct slab *s = alloc->current;

  // HOT PATH: Try cached current slab first
  if (s && s->freelist)
  {
    ret         = s->freelist;
    s->freelist = *(void **)ret;
    s->used++;
    goto theend;
  }

  // SLOW PATH: Find or create slab with space
  s = alloc->head;
  while (s && !s->freelist)
  {
    s = s->next;
  }

  if (!s)
  {
    s = slab_alloc_extend (alloc, e);
    if (s == NULL)
    {
      goto theend;
    }
  }

  // Update cache
  alloc->current = s;

  ret         = s->freelist;
  s->freelist = *(void **)ret;
  s->used++;

theend:
  latch_unlock (&alloc->l);
  return ret;
}

static bool
slab_contains (const struct slab_alloc *alloc, struct slab *s, const void *ptr)
{
  void       *start = s->data;
  const void *end   = (u8 *)start + (alloc->cap_per_slab * alloc->size);
  return ptr >= start && ptr < end;
}

static struct slab *
slab_from_ptr (struct slab_alloc *alloc, void *ptr)
{
  // Try the cached slab
  struct slab *s = alloc->current;
  if (s && slab_contains (alloc, s, ptr))
  {
    return s;
  }

  // Try remaining slabs
  s = alloc->head;
  while (s)
  {
    if (slab_contains (alloc, s, ptr))
    {
      return s;
    }
    s = s->next;
  }

  UNREACHABLE (); // LCOV_EXCL_LINE
}

void
slab_alloc_free (struct slab_alloc *alloc, void *ptr)
{
  ASSERT (alloc);
  ASSERT (ptr);

  latch_lock (&alloc->l);

  struct slab *s = slab_from_ptr (alloc, ptr);
  ASSERT (s);

  // Add to freelist
  ASSERT (s->used > 0);
  *(void **)ptr = s->freelist;
  s->freelist   = ptr;
  s->used--;

  // Update current cache if this slab now has space
  if (alloc->current == NULL || alloc->current->freelist == NULL)
  {
    alloc->current = s;
  }

  // Free empty slabs
  if (s->used == 0)
  {
    if (s->next || s->prev)
    {
      // Clear cache if we're freeing it
      if (alloc->current == s)
      {
        alloc->current = NULL;
      }

      // Update head if we're freeing it
      if (s == alloc->head)
      {
        alloc->head = s->next;
      }

      if (s->prev)
      {
        s->prev->next = s->next;
      }
      if (s->next)
      {
        s->next->prev = s->prev;
      }

      i_free (s);
    }
  }

  latch_unlock (&alloc->l);
}

#ifdef TESTING

struct test_item
{
  i32  a;
  u64  b;
  char data[10];
};

static void
test_item_init (struct test_item *item, const i32 value)
{
  item->a = value;
  item->b = (u64)value * 1000;
  for (int i = 0; i < 10; i++)
  {
    item->data[i] = (char)(value + i);
  }
}

static void
test_item_verify (const struct test_item *item, const i32 expected)
{
  test_assert_equal (item->a, expected);
  test_assert_equal (item->b, (u64)expected * 1000);
  for (int i = 0; i < 10; i++)
  {
    test_assert_equal (item->data[i], (char)(expected + i));
  }
}

TEST (slab_alloc_simple)
{
  struct slab_alloc alloc;
  error             e = error_create ();

  slab_alloc_init (&alloc, sizeof (struct test_item), 5);

  // Allocate 20 items (will span 4 slabs)
  struct test_item *items[20];
  for (int i = 0; i < 20; i++)
  {
    items[i] = slab_alloc_alloc (&alloc, &e);
    test_assert (items[i] != NULL);
    test_item_init (items[i], i);
  }

  // Verify all items
  for (int i = 0; i < 20; i++)
  {
    test_item_verify (items[i], i);
  }

  // Free every other item (indices 0, 2, 4, ... 18)
  for (int i = 0; i < 20; i += 2)
  {
    slab_alloc_free (&alloc, items[i]);
    items[i] = NULL;
  }

  // Verify remaining items (indices 1, 3, 5, ... 19)
  for (int i = 1; i < 20; i += 2)
  {
    test_item_verify (items[i], i);
  }

  // Allocate 10 new items (should reuse freed slots)
  struct test_item *new_items[10];
  for (int i = 0; i < 10; i++)
  {
    new_items[i] = slab_alloc_alloc (&alloc, &e);
    test_assert (new_items[i] != NULL);
    test_item_init (new_items[i], 100 + i);
  }

  // Verify old items still intact
  for (int i = 1; i < 20; i += 2)
  {
    test_item_verify (items[i], i);
  }

  // Verify new items
  for (int i = 0; i < 10; i++)
  {
    test_item_verify (new_items[i], 100 + i);
  }

  // Free first half of new items (indices 0-4)
  for (int i = 0; i < 5; i++)
  {
    slab_alloc_free (&alloc, new_items[i]);
    new_items[i] = NULL;
  }

  // Verify remaining new items (indices 5-9)
  for (int i = 5; i < 10; i++)
  {
    test_item_verify (new_items[i], 100 + i);
  }

  // Verify old items still intact
  for (int i = 1; i < 20; i += 2)
  {
    test_item_verify (items[i], i);
  }

  // Allocate another batch
  struct test_item *batch3[15];
  for (int i = 0; i < 15; i++)
  {
    batch3[i] = slab_alloc_alloc (&alloc, &e);
    test_assert (batch3[i] != NULL);
    test_item_init (batch3[i], 200 + i);
  }

  // Verify all three batches
  for (int i = 1; i < 20; i += 2)
  {
    test_item_verify (items[i], i);
  }
  for (int i = 5; i < 10; i++)
  {
    test_item_verify (new_items[i], 100 + i);
  }
  for (int i = 0; i < 15; i++)
  {
    test_item_verify (batch3[i], 200 + i);
  }

  // Free everything
  for (int i = 1; i < 20; i += 2)
  {
    slab_alloc_free (&alloc, items[i]);
  }
  for (int i = 5; i < 10; i++)
  {
    slab_alloc_free (&alloc, new_items[i]);
  }
  for (int i = 0; i < 15; i++)
  {
    slab_alloc_free (&alloc, batch3[i]);
  }

  slab_alloc_destroy (&alloc);
}

// Test: cap_per_slab = 1 (edge case, one item per slab)
TEST (slab_alloc_cap_one)
{
  struct slab_alloc alloc;
  error             e = error_create ();

  slab_alloc_init (&alloc, sizeof (struct test_item), 1);

  struct test_item *a = slab_alloc_alloc (&alloc, &e);
  test_assert (a != NULL);
  test_item_init (a, 42);

  struct test_item *b = slab_alloc_alloc (&alloc, &e);
  test_assert (b != NULL);
  test_assert (a != b);
  test_item_init (b, 43);

  test_item_verify (a, 42);
  test_item_verify (b, 43);

  slab_alloc_free (&alloc, a);
  test_item_verify (b, 43);

  // Realloc should reuse the freed slot
  struct test_item *c = slab_alloc_alloc (&alloc, &e);
  test_assert (c != NULL);
  test_item_init (c, 44);

  test_item_verify (b, 43);
  test_item_verify (c, 44);

  slab_alloc_free (&alloc, b);
  slab_alloc_free (&alloc, c);
  slab_alloc_destroy (&alloc);
}

// Test: all pointers are unique (detects freelist corruption)
TEST (slab_alloc_no_duplicates)
{
  struct slab_alloc alloc;
  error             e = error_create ();
  const int         N = 100;

  slab_alloc_init (&alloc, sizeof (struct test_item), 7);

  void *ptrs[100];
  for (int i = 0; i < N; i++)
  {
    ptrs[i] = slab_alloc_alloc (&alloc, &e);
    test_assert (ptrs[i] != NULL);

    // Verify no duplicate with any prior pointer
    for (int j = 0; j < i; j++)
    {
      test_assert (ptrs[i] != ptrs[j]);
    }
  }

  for (int i = 0; i < N; i++)
  {
    slab_alloc_free (&alloc, ptrs[i]);
  }
  slab_alloc_destroy (&alloc);
}

// Test: free all then realloc (slab reclamation + regrowth)
TEST (slab_alloc_free_all_realloc)
{
  struct slab_alloc alloc;
  error             e = error_create ();

  slab_alloc_init (&alloc, sizeof (struct test_item), 4);

  // Fill 3 slabs (12 items)
  struct test_item *ptrs[12];
  for (int i = 0; i < 12; i++)
  {
    ptrs[i] = slab_alloc_alloc (&alloc, &e);
    test_assert (ptrs[i] != NULL);
    test_item_init (ptrs[i], i);
  }

  // Free all — should reclaim slabs down to 1
  for (int i = 0; i < 12; i++)
  {
    slab_alloc_free (&alloc, ptrs[i]);
  }

  // Reallocate — should work, extending as needed
  for (int i = 0; i < 12; i++)
  {
    ptrs[i] = slab_alloc_alloc (&alloc, &e);
    test_assert (ptrs[i] != NULL);
    test_item_init (ptrs[i], 500 + i);
  }

  for (int i = 0; i < 12; i++)
  {
    test_item_verify (ptrs[i], 500 + i);
  }

  for (int i = 0; i < 12; i++)
  {
    slab_alloc_free (&alloc, ptrs[i]);
  }
  slab_alloc_destroy (&alloc);
}

// Test: interleaved alloc/free in LIFO, FIFO, and random order
TEST (slab_alloc_interleaved_patterns)
{
  struct slab_alloc alloc;
  error             e = error_create ();

  slab_alloc_init (&alloc, sizeof (struct test_item), 4);

  // Pattern 1: alloc 8, free LIFO (stack order)
  struct test_item *ptrs[8];
  for (int i = 0; i < 8; i++)
  {
    ptrs[i] = slab_alloc_alloc (&alloc, &e);
    test_assert (ptrs[i] != NULL);
    test_item_init (ptrs[i], i);
  }
  for (int i = 7; i >= 0; i--)
  {
    test_item_verify (ptrs[i], i);
    slab_alloc_free (&alloc, ptrs[i]);
  }

  // Pattern 2: alloc 8, free FIFO (queue order)
  for (int i = 0; i < 8; i++)
  {
    ptrs[i] = slab_alloc_alloc (&alloc, &e);
    test_assert (ptrs[i] != NULL);
    test_item_init (ptrs[i], 100 + i);
  }
  for (int i = 0; i < 8; i++)
  {
    test_item_verify (ptrs[i], 100 + i);
    slab_alloc_free (&alloc, ptrs[i]);
  }

  // Pattern 3: alternating alloc-free-alloc-free
  struct test_item *a = slab_alloc_alloc (&alloc, &e);
  test_item_init (a, 1);
  struct test_item *b = slab_alloc_alloc (&alloc, &e);
  test_item_init (b, 2);
  slab_alloc_free (&alloc, a);
  struct test_item *c = slab_alloc_alloc (&alloc, &e);
  test_item_init (c, 3);
  test_item_verify (b, 2);
  test_item_verify (c, 3);
  slab_alloc_free (&alloc, b);
  slab_alloc_free (&alloc, c);

  slab_alloc_destroy (&alloc);
}

// Test: free order triggers slab reclamation correctly
// Specifically: free an entire slab that is the HEAD of the list
TEST (slab_alloc_free_head_slab)
{
  struct slab_alloc alloc;
  error             e = error_create ();

  slab_alloc_init (&alloc, sizeof (struct test_item), 2);

  // slab1 created (becomes head)
  struct test_item *a = slab_alloc_alloc (&alloc, &e);
  struct test_item *b = slab_alloc_alloc (&alloc, &e);

  // slab2 created (becomes new head, slab2->next = slab1)
  struct test_item *c = slab_alloc_alloc (&alloc, &e);
  struct test_item *d = slab_alloc_alloc (&alloc, &e);

  test_item_init (a, 1);
  test_item_init (b, 2);
  test_item_init (c, 3);
  test_item_init (d, 4);

  // Free both items from slab2 (the head) — should reclaim it
  slab_alloc_free (&alloc, c);
  slab_alloc_free (&alloc, d);

  // slab1 items should still be fine
  test_item_verify (a, 1);
  test_item_verify (b, 2);

  // Should still be able to allocate (extends or reuses slab1 if space)
  struct test_item *f = slab_alloc_alloc (&alloc, &e);
  test_assert (f != NULL);
  test_item_init (f, 5);

  test_item_verify (a, 1);
  test_item_verify (b, 2);
  test_item_verify (f, 5);

  slab_alloc_free (&alloc, a);
  slab_alloc_free (&alloc, b);
  slab_alloc_free (&alloc, f);
  slab_alloc_destroy (&alloc);
}

// Test: free middle slab — verifies doubly-linked list surgery
TEST (slab_alloc_free_middle_slab)
{
  struct slab_alloc alloc;
  error             e = error_create ();

  slab_alloc_init (&alloc, sizeof (struct test_item), 2);

  // Create 3 slabs: head -> slab3 -> slab2 -> slab1
  struct test_item *s1[2], *s2[2], *s3[2];
  s1[0] = slab_alloc_alloc (&alloc, &e);
  s1[1] = slab_alloc_alloc (&alloc, &e);
  s2[0] = slab_alloc_alloc (&alloc, &e);
  s2[1] = slab_alloc_alloc (&alloc, &e);
  s3[0] = slab_alloc_alloc (&alloc, &e);
  s3[1] = slab_alloc_alloc (&alloc, &e);

  for (int i = 0; i < 2; i++)
  {
    test_item_init (s1[i], 10 + i);
    test_item_init (s2[i], 20 + i);
    test_item_init (s3[i], 30 + i);
  }

  // Free middle slab (slab2)
  slab_alloc_free (&alloc, s2[0]);
  slab_alloc_free (&alloc, s2[1]);

  // Verify others survive
  test_item_verify (s1[0], 10);
  test_item_verify (s1[1], 11);
  test_item_verify (s3[0], 30);
  test_item_verify (s3[1], 31);

  // Alloc more to confirm list is intact
  struct test_item *x = slab_alloc_alloc (&alloc, &e);
  test_assert (x != NULL);

  slab_alloc_free (&alloc, s1[0]);
  slab_alloc_free (&alloc, s1[1]);
  slab_alloc_free (&alloc, s3[0]);
  slab_alloc_free (&alloc, s3[1]);
  slab_alloc_free (&alloc, x);
  slab_alloc_destroy (&alloc);
}

// Test: minimum size (sizeof(void*)) works
TEST (slab_alloc_minimum_size)
{
  struct slab_alloc alloc;
  error             e = error_create ();

  slab_alloc_init (&alloc, sizeof (void *), 10);

  void *ptrs[20];
  for (int i = 0; i < 20; i++)
  {
    ptrs[i] = slab_alloc_alloc (&alloc, &e);
    test_assert (ptrs[i] != NULL);
    // Write a known pattern to detect corruption
    *(uintptr_t *)ptrs[i] = (uintptr_t)(0xDEAD0000 + i);
  }

  for (int i = 0; i < 20; i++)
  {
    test_assert_equal (*(uintptr_t *)ptrs[i], (uintptr_t)(0xDEAD0000 + i));
  }

  for (int i = 0; i < 20; i++)
  {
    slab_alloc_free (&alloc, ptrs[i]);
  }
  slab_alloc_destroy (&alloc);
}

// Stress: random alloc/free churn to shake out corruption
TEST (slab_alloc_stress_random)
{
  struct slab_alloc alloc;
  error             e    = error_create ();
  const int         POOL = 256;

  slab_alloc_init (&alloc, sizeof (struct test_item), 8);

  struct test_item *pool[256]   = {0};
  i32               values[256] = {0};
  int               active      = 0;

  for (int round = 0; round < 5000; round++)
  {
    bool do_alloc = (active == 0) || (active < POOL && randu32r (0, 2) != 0);

    if (do_alloc)
    {
      // Find free slot
      int idx = randu32r (0, POOL + 1);
      for (int i = 0; i < POOL; i++)
      {
        if (pool[(idx + i) % POOL] == NULL)
        {
          idx = (idx + i) % POOL;
          break;
        }
      }
      if (pool[idx] != NULL)
      {
        continue;
      }

      pool[idx] = slab_alloc_alloc (&alloc, &e);
      test_assert (pool[idx] != NULL);
      values[idx] = round;
      test_item_init (pool[idx], round);
      active++;
    }
    else
    {
      // Find active slot
      int idx = randu32r (0, POOL + 1);
      for (int i = 0; i < POOL; i++)
      {
        if (pool[(idx + i) % POOL] != NULL)
        {
          idx = (idx + i) % POOL;
          break;
        }
      }
      if (pool[idx] == NULL)
      {
        continue;
      }

      test_item_verify (pool[idx], values[idx]);
      slab_alloc_free (&alloc, pool[idx]);
      pool[idx] = NULL;
      active--;
    }

    // Periodic full verification
    if (round % 500 == 0)
    {
      for (int i = 0; i < POOL; i++)
      {
        if (pool[i])
        {
          test_item_verify (pool[i], values[i]);
        }
      }
    }
  }

  // Cleanup
  for (int i = 0; i < POOL; i++)
  {
    if (pool[i])
    {
      slab_alloc_free (&alloc, pool[i]);
    }
  }
  slab_alloc_destroy (&alloc);
}
#endif

/******************************************************************************
 * SECTION: Chunk Allocator
 ******************************************************************************/

/**
 * @struct chunk
 * @brief Single link block within a chunk allocator chain
 *
 * Wraps a standard local linear allocator instance alongside a flexible data
 * array which handles the payload tracking for this specific segment.
 *
 * @var chunk::alloc
 * @brief The internal linear allocator wrapper riding on top of the chunk data.
 * @var chunk::next
 * @brief Pointer to the subsequent chunk link in the chain or NULL if tail.
 * @var chunk::data
 * @brief Inline flexible array handling the raw bytes owned by this block.
 */
struct chunk
{
  struct lalloc alloc;  // Base allocator interface for this chunk
  struct chunk *next;   // Next chunk in the linked list, or NULL if tail
  u8            data[]; // Flexible array of chunk-owned bytes
};

DEFINE_DBG_ASSERT (struct chunk, chunk, c, {
  ASSERT (c);
  ASSERT (c->alloc.data == c->data);
  ASSERT (c->alloc.used <= c->alloc.limit);
  ASSERT (c->alloc.limit > 0);
  ASSERT (c->alloc.data == (u8 *)c + sizeof (struct chunk));
})

DEFINE_DBG_ASSERT (struct chunk_alloc, chunk_alloc, ca, {
  ASSERT (ca);
  ASSERT (ca->settings.target_chunk_mult >= 1.0f);
  ASSERT (ca->settings.min_chunk_size > 0);
  ASSERT (
      ca->settings.max_chunk_size == 0
      || ca->settings.max_chunk_size >= ca->settings.min_chunk_size
  );
  ASSERT (ca->head != NULL || ca->num_chunks == 0);
  ASSERT (ca->total_used <= ca->total_allocated);
  ASSERT (
      ca->settings.max_total_size == 0
      || ca->total_allocated <= ca->settings.max_total_size
  );
  ASSERT (
      ca->settings.max_chunks == 0 || ca->num_chunks < ca->settings.max_chunks
  );

  u32 counted_chunks    = 0;
  u32 counted_allocated = 0;
  u32 counted_used      = 0;

  for (const struct chunk *c = ca->head; c != NULL; c = c->next)
  {
    DBG_ASSERT (chunk, c);

    counted_chunks++;
    counted_allocated += c->alloc.limit;
    counted_used += c->alloc.used;

    ASSERT (
        counted_chunks <= ca->settings.max_chunks
        || ca->settings.max_chunks == 0
    );
    ASSERT (counted_chunks <= 100000);
  }

  // Verify counts match
  ASSERT (counted_chunks == ca->num_chunks);
  ASSERT (counted_allocated == ca->total_allocated);
  ASSERT (counted_used == ca->total_used);
})

static struct chunk *
chunk_create (const u32 size, error *e)
{
  struct chunk *ret = i_malloc (sizeof (struct chunk) + size, 1, e);
  if (ret == NULL)
  {
    return NULL;
  }
  ret->alloc = lalloc_create (ret->data, size);
  ret->next  = NULL;
  DBG_ASSERT (chunk, ret);
  return ret;
}

static void
chunk_alloc_create (
    struct chunk_alloc               *dest,
    const struct chunk_alloc_settings settings
)
{
  ASSERT (settings.target_chunk_mult >= 1.0f);
  ASSERT (settings.min_chunk_size > 0);
  ASSERT (
      settings.max_chunk_size == 0
      || settings.max_chunk_size >= settings.min_chunk_size
  );

  *dest = (struct chunk_alloc){
      .settings        = settings,
      .head            = NULL,
      .num_chunks      = 0,
      .total_allocated = 0,
      .total_used      = 0,
  };

  latch_init (&dest->latch);

  DBG_ASSERT (chunk_alloc, dest);
}

static void
chunk_alloc_create_default (struct chunk_alloc *dest)
{
  chunk_alloc_create (
      dest,
      (struct chunk_alloc_settings){
          .max_alloc_size    = 0,
          .max_total_size    = 0,
          .target_chunk_mult = 10,
          .min_chunk_size    = 10,
          .max_chunk_size    = 0,
          .max_chunks        = 0,
      }
  );
}

static u32
compute_new_chunk_size (const struct chunk_alloc *ca, const u32 alloc_size)
{
  DBG_ASSERT (chunk_alloc, ca);

  // Target chunk size based on multiplier
  u32 new_chunk_size = (u32)(alloc_size * ca->settings.target_chunk_mult);

  // Clamp to minimum
  if (new_chunk_size < ca->settings.min_chunk_size)
  {
    new_chunk_size = ca->settings.min_chunk_size;
  }

  // Clamp to maximum
  if (ca->settings.max_chunk_size > 0
      && new_chunk_size > ca->settings.max_chunk_size)
  {
    new_chunk_size = ca->settings.max_chunk_size;
  }

  // Ensure it fits the current allocation
  if (new_chunk_size < alloc_size)
  {
    new_chunk_size = alloc_size;
  }

  return new_chunk_size;
}

static void
chunk_alloc_free_all (struct chunk_alloc *ca)
{
  latch_lock (&ca->latch);

  DBG_ASSERT (chunk_alloc, ca);

  struct chunk *cur = ca->head;
  while (cur != NULL)
  {
    struct chunk *next = cur->next;
    DBG_ASSERT (chunk, cur);
    i_free (cur);
    cur = next;
  }

  ca->head            = NULL;
  ca->num_chunks      = 0;
  ca->total_allocated = 0;
  ca->total_used      = 0;

  latch_unlock (&ca->latch);
}

static err_t
chunk_alloc_add_new_chunk (struct chunk_alloc *ca, const u32 size, error *e)
{
  DBG_ASSERT (chunk_alloc, ca);

  // Check chunk count limit
  if (ca->settings.max_chunks > 0 && ca->num_chunks >= ca->settings.max_chunks)
  {
    return error_causef (
        e,
        ERR_NOMEM,
        "chunk limit reached (%u/%u chunks)",
        ca->num_chunks,
        ca->settings.max_chunks
    );
  }

  // Verify size constraints (internal assertions)
  ASSERT (size >= ca->settings.min_chunk_size);
  ASSERT (
      ca->settings.max_chunk_size == 0 || size <= ca->settings.max_chunk_size
  );

  // Check total memory limit
  if (ca->settings.max_total_size > 0)
  {
    if (ca->total_allocated + size > ca->settings.max_total_size)
    {
      return error_causef (
          e,
          ERR_NOMEM,
          "alloc %u bytes would exceed %u "
          "byte limit (%u allocated)",
          size,
          ca->settings.max_total_size,
          ca->total_allocated
      );
    }
  }

  // Create chunk
  struct chunk *new_chunk = chunk_create (size, e);
  if (new_chunk == NULL)
  {
    return error_trace (e);
  }

  // Add to front of list
  new_chunk->next = ca->head;
  ca->head        = new_chunk;
  ca->num_chunks++;
  ca->total_allocated += size;

  return SUCCESS;
}

static void *
chunk_malloc (struct chunk_alloc *ca, const u32 req, const u32 size, error *e)
{
  latch_lock (&ca->latch);

  DBG_ASSERT (chunk_alloc, ca);

  // Check for overflow
  if (req > 0 && size > UINT32_MAX / req)
  {
    error_causef (e, ERR_NOMEM, "alloc overflow: %u * %u", req, size);
    latch_unlock (&ca->latch);
    return NULL;
  }

  const u32 alloc_size = req * size;

  // Check single allocation limit
  if (ca->settings.max_alloc_size > 0
      && alloc_size > ca->settings.max_alloc_size)
  {
    error_causef (
        e,
        ERR_NOMEM,
        "alloc %u bytes exceeds max %u",
        alloc_size,
        ca->settings.max_alloc_size
    );
    latch_unlock (&ca->latch);
    return NULL;
  }

  // Try current chunk first
  if (ca->head != NULL)
  {
    void *ptr = lmalloc (&ca->head->alloc, req, size, NULL);
    if (ptr != NULL)
    {
      ca->total_used += alloc_size;
      latch_unlock (&ca->latch);
      return ptr;
    }
  }

  // Need a new chunk - calculate size
  const u32 new_chunk_size = compute_new_chunk_size (ca, alloc_size);

  // Create new chunk
  if (chunk_alloc_add_new_chunk (ca, new_chunk_size, e) != SUCCESS)
  {
    latch_unlock (&ca->latch);
    return NULL;
  }

  // Allocate from new chunk
  void *ptr = lmalloc (&ca->head->alloc, req, size, e);
  if (ptr != NULL)
  {
    ca->total_used += alloc_size;
  }

  latch_unlock (&ca->latch);

  return ptr;
}

/******************************************************************************
 * SECTION: Generic Allocator
 ******************************************************************************/

void
create_default_allocator (struct allocator *alloc)
{
  alloc->type = AT_CHUNK_ALLOCATOR;
  chunk_alloc_create_default (&alloc->calloc);
}

void *
allocate (struct allocator *alloc, u32 nelem, u32 size, error *e)
{
  ASSERT (alloc);
  switch (alloc->type)
  {
    case AT_CHUNK_ALLOCATOR:
    {
      return chunk_malloc (&alloc->calloc, nelem, size, e);
    }
    default:
    {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
}

void *
allocator_copy (struct allocator *alloc, const void *ptr, u32 size, error *e)
{
  void *dest = allocate (alloc, size, 1, e);

  if (dest == NULL)
  {
    return NULL;
  }

  memcpy (dest, ptr, size);

  return dest;
}

void
allocator_free (struct allocator *alloc)
{
  switch (alloc->type)
  {
    case AT_CHUNK_ALLOCATOR:
    {
      chunk_alloc_free_all (&alloc->calloc);
      return;
    }
    default:
    {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
  UNREACHABLE ();
}

/******************************************************************************
 * SECTION: Builder Pattern
 ******************************************************************************/

void
builder_init (struct builder *b, struct allocator *alloc)
{
  b->persistent = alloc;
  create_default_allocator (&b->temp);
}

void *
builder_malloc_temp (struct builder *b, u32 nelem, u32 size, error *e)
{
  return allocate (&b->temp, nelem, size, e);
}

void *
builder_malloc_persist (struct builder *b, u32 nelem, u32 size, error *e)
{
  return allocate (b->persistent, nelem, size, e);
}

void
builder_free (struct builder *b)
{
  allocator_free (&b->temp);
}
