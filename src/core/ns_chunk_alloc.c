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

#include "core/ns_chunk_alloc.h"

#include <stdint.h>
#include <string.h>

#include "core/ns_bounds.h"
#include "core/ns_concurrency.h"
#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/os/ns_memory.h"

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
    error_causef (e, ERR_NOMEM, "linear alloc %d bytes: only %d remaining", total, avail);
    latch_unlock (&a->latch);
    return NULL;
  }

  void *ret = &a->data[a->used];
  a->used   = a->used + total;

  latch_unlock (&a->latch);

  return ret;
}

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
      ca->settings.max_chunk_size == 0 || ca->settings.max_chunk_size >= ca->settings.min_chunk_size
  );
  ASSERT (ca->head != NULL || ca->num_chunks == 0);
  ASSERT (ca->total_used <= ca->total_allocated);
  ASSERT (ca->settings.max_total_size == 0 || ca->total_allocated <= ca->settings.max_total_size);
  ASSERT (ca->settings.max_chunks == 0 || ca->num_chunks < ca->settings.max_chunks);

  u32 counted_chunks    = 0;
  u32 counted_allocated = 0;
  u32 counted_used      = 0;

  for (const struct chunk *c = ca->head; c != NULL; c = c->next)
  {
    DBG_ASSERT (chunk, c);

    counted_chunks++;
    counted_allocated += c->alloc.limit;
    counted_used += c->alloc.used;

    ASSERT (counted_chunks <= ca->settings.max_chunks || ca->settings.max_chunks == 0);
    ASSERT (counted_chunks <= 100000);
  }

  // Verify counts match
  ASSERT (counted_chunks == ca->num_chunks);
  ASSERT (counted_allocated == ca->total_allocated);
  ASSERT (counted_used == ca->total_used);
})

static struct chunk *
chunk_create (const u32 size, struct i_mem mem, error *e)
{
  struct chunk *ret = i_malloc (mem, sizeof (struct chunk) + size, 1, e);
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
chunk_alloc_create (struct chunk_alloc *dest, const struct chunk_alloc_settings settings)
{
  ASSERT (settings.target_chunk_mult >= 1.0f);
  ASSERT (settings.min_chunk_size > 0);
  ASSERT (settings.max_chunk_size == 0 || settings.max_chunk_size >= settings.min_chunk_size);

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

void
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
  if (ca->settings.max_chunk_size > 0 && new_chunk_size > ca->settings.max_chunk_size)
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

void
chunk_alloc_free_all (struct chunk_alloc *ca)
{
  latch_lock (&ca->latch);

  DBG_ASSERT (chunk_alloc, ca);

  struct chunk *cur = ca->head;
  while (cur != NULL)
  {
    struct chunk *next = cur->next;
    DBG_ASSERT (chunk, cur);
    i_free (ca->settings.mem, cur);
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
  ASSERT (ca->settings.max_chunk_size == 0 || size <= ca->settings.max_chunk_size);

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
  struct chunk *new_chunk = chunk_create (size, ca->settings.mem, e);
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

void *
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
  if (ca->settings.max_alloc_size > 0 && alloc_size > ca->settings.max_alloc_size)
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
