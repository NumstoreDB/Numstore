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
 * @brief Allocators and memory allocation patterns
 *
 * Alloc.h contains specialized allocators used in numstore
 * such as:
 * - Local Allocator - a standard arena allocator that is fixed in size
 * - Chunk Allocator - an allocator that offers 1 free for all attached allocs
 * - Slab Allocator  - an allocator that allocates fixed size slabs
 * - Malloc Plan     - a two phase malloc - defining the size - then one malloc
 */

#ifndef LALLOC_H
#define LALLOC_H

#include "concurrency.h" // latch
#include "csx_assert.h"  // UNREACHABLE
#include "platform.h"    // HEADER_FUNC
#include "stdtypes.h"    // u32

/******************************************************************************
 * SECTION: Local Linear Allocator
 * ----------------------------------------------------------------------------
 *
 * @brief A fixed sized memory arena
 *
 * A Linear allocator takes a buffer and dishes out memory
 * from this fixed sized buffer. It is not dynamic.
 * It does not do any mallocs under the hood.
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

/**
 * @brief Creates a new local allocator using the user supplied buffer
 *
 * No memory allocations because the allocator is provided via the user
 *
 * @param  data The buffer to hold all the data
 * @param  limit The length of [data]
 * @return A new stack allocated local allocator
 */
struct lalloc lalloc_create (u8 *data, u32 limit);

/**
 * @brief Creates an allocator from a fixed size buffer
 *
 * Shorthand for an allocator from a stack allocated buffer
 *
 * do something like:
 * u8 data[1065];
 * struct lalloc l = lalloc_create_from(data)
 *
 *
 * @param buf The stack allocated buffer - needs sizeof
 * @return A new local allocator
 */
#define lalloc_create_from(buf) lalloc_create ((u8 *)buf, sizeof (buf))

/**
 * @brief Gets how many bytes are being used
 *
 * Useful for setting state - doing a bunch of allocations,
 * then calling lalloc_reset_to_state to reset back to the
 * recorded state
 *
 * @param l The local allocator
 * @return A u32 - opaque, but internally represents the number
 * of bytes used
 */
u32 lalloc_get_state (struct lalloc *l);

/**
 * @brief Resets to the state obtained via [lalloc_get_state]
 *
 * Resets state so that any allocation that happened between [lalloc_get_state]
 * and now is ignored
 *
 * @param l The allocator
 * @param state The state obtained from [lalloc_get_state]
 */
void lalloc_reset_to_state (struct lalloc *l, u32 state);

/**
 * @brief Primary allocator for local linear allocator
 *
 * Performs a thread-safe bump allocation from the underlying arena buffer.
 * Memory returned is uninitialized and aligned according to alignment
 * constraints.
 *
 * @param a Pointer to the local linear allocator instance.
 * @param req Requested memory alignment boundary in bytes.
 * @param size Total number of contiguous bytes to allocate.
 * @param e Pointer to an error container tracking out-of-memory states.
 * @return void* Pointer to the allocated memory block, or NULL on failure.
 */
void *lmalloc (struct lalloc *a, u32 req, u32 size, error *e);

/**
 * @brief Primary zero-initializing allocator for local linear allocator
 *
 * Allocates a block of memory exactly like lmalloc, but zeroes out the memory
 * block before passing the pointer back to the caller.
 *
 * @param a Pointer to the local linear allocator instance.
 * @param req Requested memory alignment boundary in bytes.
 * @param size Total number of contiguous bytes to allocate.
 * @param e Pointer to an error container tracking out-of-memory states.
 * @return void* Pointer to the zeroed memory block, or NULL on failure.
 */
void *lcalloc (struct lalloc *a, u32 req, u32 size, error *e);

/**
 * @brief Total reset of local linear allocator consumption tracking
 *
 * Thread-safely resets the internal used counter back to zero. This completely
 * invalidates all existing allocations made out of this allocator instance.
 *
 * @param a Pointer to the target linear allocator instance.
 */
void lalloc_reset (struct lalloc *a);

/**
 * @brief Inline allocator helper asserting on allocation exhaustion
 *
 * Wraps around standard lmalloc, bypassing external error parameters and
 * forcing a hard system runtime panic if an out-of-memory event triggers.
 *
 * @param a Pointer to the local linear allocator instance.
 * @param req Requested memory alignment boundary in bytes.
 * @param size Total number of contiguous bytes to allocate.
 * @return void* Guaranteed non-NULL pointer to the newly allocated memory
 * block.
 */
HEADER_FUNC void *
lmalloc_expect (struct lalloc *a, const u32 req, const u32 size)
{
  void *ret = lmalloc (a, req, size, NULL);
  ASSERT (ret);
  return ret;
}

/******************************************************************************
 * SECTION: Chunk Allocator
 * ----------------------------------------------------------------------------
 *
 * @brief Coupled allocations - one free
 *
 * A chunk allocator allocates everything within a context
 * using chunks - one free frees all memory that was allocated
 * with it.
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

/**
 * @struct chunk_alloc_settings
 * @brief Threshold and behavioral settings for a chunk-based arena
 *
 * @var chunk_alloc_settings::max_alloc_size
 * @brief Maximum size constraint for an individual allocation block (0 =
 * unlimited).
 *
 * @var chunk_alloc_settings::max_total_size
 * @brief Maximum collective allocation limit allowed across the entire chain (0
 * = unlimited).
 *
 * @var chunk_alloc_settings::target_chunk_mult
 * @brief Scale multiplier applied to allocations to calculate new chunk head
 * dimensions (> 1).
 *
 * @var chunk_alloc_settings::min_chunk_size
 * @brief Lower bound ceiling for newly minted chunk buffers.
 *
 * @var chunk_alloc_settings::max_chunk_size
 * @brief Upper bound ceiling for newly minted chunk buffers (0 = unlimited).
 *
 * @var chunk_alloc_settings::max_chunks
 * @brief Ceiling count for the maximum allowed number of chunks in the chain (0
 * = unlimited).
 */
struct chunk_alloc_settings
{
  u32   max_alloc_size;
  u32   max_total_size;
  float target_chunk_mult;
  u32   min_chunk_size;
  u32   max_chunk_size;
  u32   max_chunks;
};

/**
 * @struct chunk_alloc
 * @brief Main context controller handling a chunk-based arena allocation chain.
 *
 * @var chunk_alloc::latch
 * @brief Thread synchronization lock protecting operations across the chunk
 * chain.
 *
 * @var chunk_alloc::settings
 * @brief The immutable threshold metrics guiding new chunk calculations.
 *
 * @var chunk_alloc::head
 * @brief Root link pointing to the first chunk within the allocation list.
 *
 * @var chunk_alloc::num_chunks
 * @brief Active index counting how many individual chunks are chained.
 *
 * @var chunk_alloc::total_allocated
 * @brief Aggregated metric tracking every byte allocated down the system chain.
 *
 * @var chunk_alloc::total_used
 * @brief Aggregated metric tracking active bytes utilized inside the systems
 * payload.
 */
struct chunk_alloc
{
  latch                       latch;
  struct chunk_alloc_settings settings;
  struct chunk               *head;
  u32                         num_chunks;
  u32                         total_allocated;
  u32                         total_used;
};

/**
 * @brief Initializes a chunk allocation controller with explicit custom
 * metrics.
 *
 * Sets up the root context control blocks, clears tracking metadata counters,
 * and bakes in the unique runtime limitations specified by the caller.
 *
 * @param dest Pointer to the uninitialized controller destination memory.
 * @param  settings  The immutable operational constraints for the arena
 * chain.
 */
void chunk_alloc_create (
    struct chunk_alloc         *dest,
    struct chunk_alloc_settings settings
);

/**
 * @brief Initializes a chunk allocation controller using baseline platform
 * presets.
 *
 * Convenience wrapper with sensible defaults
 *
 * @param dest Pointer to the uninitialized controller destination memory.
 */
void chunk_alloc_create_default (struct chunk_alloc *dest);

/**
 * @brief Destroys the allocation list and frees all memory segments
 *
 * Iterates through every node linked to the root node, releases backing
 * host memory frames, and fully clears control metrics back to zero.
 *
 * @param ca Target allocator context
 */
void chunk_alloc_free_all (struct chunk_alloc *ca);

/**
 * @brief Resets tracking offsets to zero without releasing underlying system
 * segments.
 *
 * Wipes out utilized tracking metrics to allow memory reuse across the chain,
 * eliminating the CPU penalty of recurring system-level allocation calls.
 *
 * @param ca Target allocator context being recycled.
 */
void chunk_alloc_reset_all (struct chunk_alloc *ca);

/**
 * @brief Reserves a contiguous raw segment of memory out of the managed arena.
 *
 * Evaluates the active block space, dynamically spawning an underlying node
 * bridge if current payload capacities cannot fit the multiplied payload matrix
 * size.
 *
 * @param ca   Target allocator context handling the request.
 * @param req  Number of elements requested.
 * @param size Size of each individual element in bytes.
 * @param e    Error context
 *
 * @return void* Pointer to the newly carved block segment, or NULL on failure.
 */
void *chunk_malloc (struct chunk_alloc *ca, u32 req, u32 size, error *e);

/**
 * @brief Reserves a contiguous block segment out of the arena and
 * zero-initializes it.
 *
 * Executes identical block evaluation processes as standard allocation
 * routines, but explicitly forces bitwise zero clearing across the entire
 * returning block payload.
 *
 * @param ca   Target allocator context handling the request.
 * @param req  Number of elements requested.
 * @param size Size of each individual element in bytes.
 * @param e    Error context
 *
 * @return void* Pointer to the cleared block segment, or NULL on failure.
 */
void *chunk_calloc (struct chunk_alloc *ca, u32 req, u32 size, error *e);

/**
 * @brief Copies external memory blocks inside a newly reserved arena payload
 * area.
 *
 * Allocates fresh space inside the active chain matching the source volume,
 * copies the existing buffer content over, and returns the managed target block
 * address.
 *
 * @param ca   Target allocator context receiving the content.
 * @param ptr  Pointer to the external source data block to be copied.
 * @param size Total size in bytes of the external memory footprint.
 * @param e    Error context
 *
 * @return void* Pointer to the localized deep-copied buffer, or NULL on
 * failure.
 */
void *chunk_alloc_move_mem (
    struct chunk_alloc *ca,
    const void         *ptr,
    u32                 size,
    error              *e
);

/******************************************************************************
 * SECTION: Slab Allocator
 * ----------------------------------------------------------------------------
 *
 * @brief Allocates fixed size blocks
 *
 * A slab allocator allocates fixed sized "slabs" it is dynamic
 * because it can allocate an infinite number of these
 ******************************************************************************/

/**
 * @brief A single slab in a slab allocator
 */
struct slab;

/**
 * @struct slab_alloc
 * @brief Controller handling fixed-size object allocation containers
 *
 * @var slab_alloc::head
 * @brief Base tracking node pointing to the start of the slab chain list.
 *
 * @var slab_alloc::current
 * @brief Hot cache pointer pointing directly to a slab node with immediate
 * vacancy.
 *
 * @var slab_alloc::l
 * @brief Isolation lock safeguarding thread transitions during slab allocation
 * blocks.
 *
 * @var slab_alloc::size
 * @brief Uniform explicit byte size configuration for every object stored.
 *
 * @var slab_alloc::cap_per_slab
 * @brief Capacity metric tracking how many objects an individual slab block
 * holds.
 */
struct slab_alloc
{
  struct slab *head;
  struct slab *current;
  latch        l;
  u32          size;
  u32          cap_per_slab;
};

/**
 * @brief Initializes a target slab allocator control context
 *
 * Sets standard tracking parameters defining object sizing layouts and
 * capacities.
 *
 * @param dest Out pointer pointing to the target slab controller instance.
 * @param size Uniform byte boundary size matching all items handled.
 * @param cap_per_slab Target threshold limit tracking maximum items stored per
 * slab.
 */
void slab_alloc_init (struct slab_alloc *dest, u32 size, u32 cap_per_slab);

/**
 * @brief Releases every individual backing slab pool assigned to an allocator
 *
 * Iterates down the slab chain, making system-level frees to scrub all
 * structures.
 *
 * @param alloc Pointer to the targeted active slab allocator context.
 */
void slab_alloc_destroy (struct slab_alloc *alloc);

/**
 * @brief Fetches an uninitialized fixed-size slot from the hot slab track
 *
 * @param alloc Pointer to the active slab allocator pool.
 * @param e Out error container tracking block execution faults.
 * @return void* Valid pointer targeting a free slot, or NULL if exhausted.
 */
void *slab_alloc_alloc (struct slab_alloc *alloc, error *e);

/**
 * @brief Returns an individual active object block slot back to its home slab
 * pool
 *
 * @param alloc Pointer to the active slab allocator pool.
 * @param ptr Target memory location to return back to the vacancy pool.
 */
void slab_alloc_free (struct slab_alloc *alloc, void *ptr);

/******************************************************************************
 * SECTION: Malloc Plan
 * ----------------------------------------------------------------------------
 *
 * @brief Does 2 phases of allocation - a planning and a memcpy
 *
 * 1. Fake allocation - keep track of how many bytes you need
 * 2. Real allocation - 1 malloc and 1 free
 ******************************************************************************/

/**
 * @struct malloc_plan
 * @brief State container managing a two-phase structured execution pipeline
 *
 * This tracking context logs target structural limits safely first before
 * shifting state into active allocation mode to pass down slices from one
 * consolidated buffer.
 *
 * @var malloc_plan::size
 * @brief Accumulated size metrics tracked during the preliminary plan phase.
 *
 * @var malloc_plan::blen
 * @brief Offset cursor tracking bytes sliced away during the active allocation
 * phase.
 *
 * @var malloc_plan::buffer
 * @brief Monolithic root pointer tracking the memory buffer block.
 *
 * @var malloc_plan::mode
 * @brief Current operational phase context.
 */
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

/**
 * @brief Zero initializes a new execution tracking plan block
 *
 * @return struct malloc_plan Cleanly initialized planning tracking state
 * struct.
 */
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

/**
 * @brief Retrieves the write tracking cursor offset location from an active
 * allocation context
 *
 * @param plan Const tracking pointer capturing the active plan state.
 * @return void* Pointer to the current offset write head, or NULL if planning.
 */
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

/**
 * @brief Dual-behavior planning tool or copy executor tracking a target chunk
 * state
 *
 * If in MP_PLANNING mode, this simply increments internal size constraints by
 * len. If in MP_ALLOCING mode, this performs a physical copy of data into the
 * plan buffer.
 *
 * @param plan Tracking state coordinator pointer.
 * @param data Source pointer targeting an unmanaged structure data block.
 * @param len Exact byte distance bounds tracked for copying.
 *
 * @return void* Destination pointer offset where data will reside, or NULL if
 * planning.
 */
void *malloc_plan_memcpy (struct malloc_plan *plan, const void *data, u32 len);

/**
 * @brief Transitions a plan state from planning constraints over to raw buffer
 * initialization
 *
 * Allocates the single consolidated memory chunk computed during the tracking
 * phase.
 *
 * @param plan Target structural manager context to evaluate and initialize.
 * @param e Tracking parameter to capture standard execution faults.
 *
 * @return err_t Custom error response metrics tracking execution success.
 */
err_t malloc_plan_alloc (struct malloc_plan *plan, error *e);

#endif // LALLOC_H
