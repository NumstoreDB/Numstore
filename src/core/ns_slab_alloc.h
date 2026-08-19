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

#include "core/ns_concurrency.h" // latch
#include "core/ns_error.h"
#include "core/ns_stdtypes.h" // u32
#include "core/os/ns_memory.h"

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
  struct i_mem      mem;
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
void slab_alloc_init (struct slab_alloc *dest, struct i_mem mem, u32 size, u32 cap_per_slab);

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

#endif
