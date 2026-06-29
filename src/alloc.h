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
  struct slab      *head;
  struct slab      *current;
  latch             l;
  u32               size;
  u32               cap_per_slab;
  struct allocator *alloc;
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

/******************************************************************************
 * SECTION: Generic Allocator
 * ----------------------------------------------------------------------------
 * @brief Just a container for any generic allocator
 ******************************************************************************/

struct allocator
{
  enum
  {
    AT_CHUNK_ALLOCATOR,
  } type;

  union {
    struct chunk_alloc calloc;
  };
};

void  create_default_allocator (struct allocator *alloc);
void *allocate (struct allocator *alloc, u32 nelem, u32 size, error *e);
void *
allocator_copy (struct allocator *alloc, const void *ptr, u32 size, error *e);
void allocator_free (struct allocator *alloc);

#define ALLOC_INIT(name) \
  struct allocator name; \
  create_default_allocator (&name)

#define ALLOC_CLOSE(name) allocator_free (&name)
#define ALLOC_RESET(name) \
  allocator_free (&name); \
  create_default_allocator (&name)

/******************************************************************************
 * SECTION: Builder Pattern
 * ----------------------------------------------------------------------------
 * @brief Contains two allocators - a persistent allocator and a temp
 ******************************************************************************/

struct builder
{
  struct allocator *persistent;
  struct allocator  temp;
};

void  builder_init (struct builder *b, struct allocator *alloc);
void *builder_malloc_temp (struct builder *b, u32 nelem, u32 size, error *e);
void *builder_malloc_persist (struct builder *b, u32 nelem, u32 size, error *e);
void  builder_free (struct builder *b);

#define BUILDER_INIT(name, alloc) \
  struct builder name;            \
  builder_init (&name, alloc)

#define BUILDER_CLOSE(name) builder_free (&name)

#endif // LALLOC_H
