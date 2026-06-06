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

#ifndef LALLOC_H
#define LALLOC_H

#include <c_specx/assert.h>
#include <c_specx/error.h>
#include <c_specx/latch.h>
#include <c_specx/logging.h>
#include <c_specx/platform.h>
#include <c_specx/stdtypes.h>

////////////////////////////////////////////////////////////
// Local linear Allocator
//
// A Linear allocator takes a buffer and dishes out memory
// from this fixed sized buffer. It is not dynamic.
// It does not do any mallocs under the hood

struct lalloc
{
  latch latch;
  u32   used;
  u32   limit;
  u8   *data;
};

#define lalloc_create_from(buf) lalloc_create ((u8 *)buf, sizeof (buf))

struct lalloc lalloc_create (u8 *data, u32 limit);
u32           lalloc_get_state (struct lalloc *l);
void          lalloc_reset_to_state (struct lalloc *l, u32 state);
void         *lmalloc (struct lalloc *a, u32 req, u32 size, error *e);
void         *lcalloc (struct lalloc *a, u32 req, u32 size, error *e);
void          lalloc_reset (struct lalloc *a);

HEADER_FUNC void *
lmalloc_expect (struct lalloc *a, const u32 req, const u32 size)
{
  void *ret = lmalloc (a, req, size, NULL);
  ASSERT (ret);
  return ret;
}

////////////////////////////////////////////////////////////
// Chunk Allocator
//
// A chunk allocator allocates everything within a context
// using chunks - one free frees all memory that was allocated
// with it

/// A single chunk of memory in a chunk allocator chain
struct chunk
{
  struct lalloc alloc;  // Base allocator interface for this chunk
  struct chunk *next;   // Next chunk in the linked list, or NULL if tail
  u8            data[]; // Flexible array of chunk-owned bytes
};

/// Configuration settings for a chunk allocator
struct chunk_alloc_settings
{
  u32 max_alloc_size; // Maximum size of a single allocation in bytes (0 =
                      // unlimited)
  u32 max_total_size; // Maximum total memory across all chunks in bytes (0 =
                      // unlimited)
  float target_chunk_mult; // Multiplier applied to the requested size when
                           // sizing a new chunk (must be > 1)
  u32 min_chunk_size;      // Minimum size of a newly allocated chunk in bytes
  u32 max_chunk_size; // Maximum size of a newly allocated chunk in bytes (0 =
                      // unlimited)
  u32 max_chunks;     // Maximum number of chunks that may be allocated (0 =
                      // unlimited)
};

/// A chunk-based arena allocator
struct chunk_alloc
{
  latch latch; // Synchronization latch guarding this allocator
  struct chunk_alloc_settings settings;   // Configuration settings
  struct chunk               *head;       // Head of the chunk linked list
  u32                         num_chunks; // Current number of allocated chunks
  u32 total_allocated; // Total bytes allocated across all chunks
  u32 total_used;      // Total bytes currently in use
};

/// Initializes a chunk allocator with the given settings
void chunk_alloc_create (
    struct chunk_alloc         *dest, // Allocator to initialize
    struct chunk_alloc_settings settings
); // Settings to apply

/// Initializes a chunk allocator with default settings
void chunk_alloc_create_default (
    struct chunk_alloc *dest
); // Allocator to initialize

/// Frees all chunks and resets the allocator to its initial state
void chunk_alloc_free_all (struct chunk_alloc *ca); // Target allocator

/// Resets all chunk usage counters without freeing any memory, keeping chunks
/// available for reuse
void chunk_alloc_reset_all (struct chunk_alloc *ca); // Target allocator

/// Allocates uninitialized memory from the chunk allocator
void *chunk_malloc (
    struct chunk_alloc *ca,   // Target allocator
    u32                 req,  // Requested alignment in bytes
    u32                 size, // Number of bytes to allocate
    error              *e
); // The error object

/// Allocates zero-initialized memory from the chunk allocator
void *chunk_calloc (
    struct chunk_alloc *ca,   // Target allocator
    u32                 req,  // Requested alignment in bytes
    u32                 size, // Number of bytes to allocate
    error              *e
); // The error object

/// Copies memory from an external pointer into the chunk allocator and returns
/// a pointer to the copy
void *chunk_alloc_move_mem (
    struct chunk_alloc *ca,   // Target allocator
    const void         *ptr,  // Source data to copy
    u32                 size, // Number of bytes to copy
    error              *e
); // The error object

////////////////////////////////////////////////////////////
// Slab Allocator
//
// A slab allocator allocates fixed sized "slabs" it is dynamic
// because it can allocate an infinite number of these

struct slab;

struct slab_alloc
{
  struct slab *head;
  struct slab *current; // Cache slab with free space (hot path)
  latch        l;
  u32          size;
  u32          cap_per_slab;
};

void slab_alloc_init (struct slab_alloc *dest, u32 size, u32 cap_per_slab);
void slab_alloc_destroy (struct slab_alloc *alloc);

void *slab_alloc_alloc (struct slab_alloc *alloc, error *e);
void  slab_alloc_free (struct slab_alloc *alloc, void *ptr);

////////////////////////////////////////////////////////////
// Malloc Plan
//
// Does 2 phases of allocation:
// 1. Fake allocation - keep track of how many bytes you need
// 2. Real allocation - 1 malloc and 1 free

struct malloc_plan
{
  u32   size;
  u32   blen;
  void *buffer;

  enum
  {
    MP_PLANNING,
    MP_ALLOCING
  } mode;
};

HEADER_FUNC struct malloc_plan
malloc_plan_create (void)
{
  return (struct malloc_plan){
      .size   = 0,
      .blen   = 0,
      .buffer = NULL,
      .mode   = MP_PLANNING,
  };
}

HEADER_FUNC void *
malloc_plan_head (const struct malloc_plan *plan)
{
  switch (plan->mode)
  {
    case MP_PLANNING:
    {
      return NULL;
    }
    case MP_ALLOCING:
    {
      return (u8 *)plan->buffer + plan->blen;
    }
  }
  UNREACHABLE ();
}

// Allocate memory
void *malloc_plan_memcpy (struct malloc_plan *plan, const void *data, u32 len);

// Do the planning -> alloc swap
err_t malloc_plan_alloc (struct malloc_plan *plan, error *e);

#endif // LALLOC_H
