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
#include "core/ns_stdtypes.h"    // u32

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

#endif
