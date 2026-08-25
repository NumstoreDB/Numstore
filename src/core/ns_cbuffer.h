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

#ifndef NS_CBUFFER_H
#define NS_CBUFFER_H

#include "core/ns_bytes.h"
#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_platform.h"
#include "core/ns_stdtypes.h"
#include "core/os/ns_file.h"

#include <stdbool.h>

/******************************************************************************
 * SECTION: Circular Buffer
 * ----------------------------------------------------------------------------
 *
 * @brief A ring buffer
 *
 *
 * A cbuffer wraps a fixed-size byte array and maintains head/tail pointers
 * to implement a FIFO queue. No heap allocation is performed - the caller
 * owns the backing memory and decides its lifetime.
 *
 * Layout:
 * @code
 * [-------------++++++++++++++++------------]
 * ^tail          ^head
 * @endcode
 *
 * Data occupies [tail, head). When the buffer is full, head == tail and
 * isfull is true.
 *
 * @par Usage:
 * @code
 *
 * TODO
 *
 * @endcode
 ******************************************************************************/

/**
 * @struct cbuffer
 * @brief Circular ring buffer tracking byte array sequences.
 *
 * @var cbuffer::data
 * @brief Pointer to the caller-supplied backing array.
 *
 * @var cbuffer::cap
 * @brief Total capacity of the backing array in bytes.
 *
 * @var cbuffer::head
 * @brief Write cursor - next byte is written here.
 *
 * @var cbuffer::tail
 * @brief Read cursor - next byte is read from here.
 *
 * @var cbuffer::isfull
 * @brief True when head == tail and the buffer is full (not empty).
 */
struct cbuffer
{
  u8  *data;
  u32  cap;
  u32  head;
  u32  tail;
  bool isfull;
};

/*-----------------------------------------------------------------------------
 * SUBSECTION: cbuffer Creation
 * @brief Creating cbuffers
 *----------------------------------------------------------------------------*/

/**
 * @def cbuffer_create_from
 * @brief Creates a cbuffer over an existing array with zero initial length.
 * @param data Pointer to the backing array.
 */
#define cbuffer_create_from(data) cbuffer_create (data, sizeof data)

/**
 * @def cbuffer_create_full_from
 * @brief Creates a cbuffer over an existing array, treating it as full.
 * @param data Pointer to the backing array (already filled).
 */
#define cbuffer_create_full_from(data) cbuffer_create_with (data, sizeof data, sizeof data)

/**
 * @def cbuffer_create_from_cstr
 * @brief Creates a cbuffer from a C string, treating the string bytes as data.
 * @param cstr Null-terminated string to wrap (length is strlen(cstr)).
 */
#define cbuffer_create_from_cstr(cstr) cbuffer_create_with (cstr, strlen (cstr), strlen (cstr))

/**
 * @fn struct cbuffer cbuffer_create(void *data, u32 cap)
 * @brief Creates an empty cbuffer over a caller-supplied array.
 *
 * @param data Pointer to the backing array.
 * @param cap Size of the backing array in bytes.
 * @return Initialized cbuffer with no data.
 */
struct cbuffer cbuffer_create (void *data, u32 cap);

/**
 * @fn struct cbuffer cbuffer_create_with(void *data, u32 cap, u32 len)
 * @brief Creates a cbuffer with an initial data length already present.
 *
 * @param data Pointer to the backing array (first len bytes are considered
 * data).
 * @param cap Total size of the backing array in bytes.
 * @param len Number of bytes already present in the buffer.
 * @return Initialized cbuffer with head advanced by len.
 */
struct cbuffer cbuffer_create_with (void *data, u32 cap, u32 len);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Properties
 * @brief Properties on cbuffers
 *----------------------------------------------------------------------------*/

/**
 * @fn u32 cbuffer_len(const struct cbuffer *b)
 * @brief Returns the number of bytes currently in the buffer.
 *
 * @param b The cbuffer (must not be NULL).
 * @return Number of bytes available to read.
 */
HEADER_FUNC u32
cbuffer_len (const struct cbuffer *b)
{
  u32 len;
  if (b->isfull) {
    len = b->cap;
  } else if (b->head >= b->tail) {
    len = b->head - b->tail;
  } else {
    len = b->cap - (b->tail - b->head);
  }
  return len;
}

DEFINE_DBG_ASSERT (struct cbuffer, cbuffer, b, {
  ASSERT (b);
  ASSERT (b->cap > 0);
  ASSERT (b->data);
  if (b->isfull) {
    ASSERT (b->tail == b->head);
  }
  ASSERT (cbuffer_len (b) <= b->cap);
})

/**
 * @fn bool cbuffer_isempty(const struct cbuffer *b)
 * @brief Returns true if the buffer contains no data.
 *
 * @param b The cbuffer (must not be NULL).
 * @return True if empty, false otherwise.
 */
HEADER_FUNC bool
cbuffer_isempty (const struct cbuffer *b)
{
  DBG_ASSERT (cbuffer, b);
  return (!b->isfull && b->head == b->tail);
}

/**
 * @fn u32 cbuffer_slen(const struct cbuffer *b, const u32 size)
 * @brief Returns the number of elements of size bytes currently in the buffer.
 *
 * @param b The cbuffer.
 * @param size Element size in bytes - must evenly divide the current length.
 * @return Number of whole elements present.
 */
HEADER_FUNC u32
cbuffer_slen (const struct cbuffer *b, const u32 size)
{
  const u32 len = cbuffer_len (b);
  ASSERT (len % size == 0);
  return len / size;
}

/**
 * @fn u32 cbuffer_avail(const struct cbuffer *b)
 * @brief Returns the number of bytes available for writing.
 *
 * @param b The cbuffer (must not be NULL).
 * @return Bytes of free space remaining.
 */
HEADER_FUNC u32
cbuffer_avail (const struct cbuffer *b)
{
  DBG_ASSERT (cbuffer, b);
  const u32 len = cbuffer_len (b);
  ASSERT (b->cap >= len);
  return b->cap - len;
}

/**
 * @fn u32 cbuffer_savail(const struct cbuffer *b, const u32 size)
 * @brief Returns the number of elements of size bytes that can still be
 * written.
 *
 * @param b The cbuffer (must not be NULL).
 * @param size Element size in bytes - must evenly divide the current length.
 * @return Number of whole elements that fit in the remaining space.
 */
HEADER_FUNC u32
cbuffer_savail (const struct cbuffer *b, const u32 size)
{
  DBG_ASSERT (cbuffer, b);
  const u32 len = cbuffer_len (b);
  ASSERT (b->cap >= len);
  ASSERT (len % size == 0);
  return (b->cap - len) / size;
}

/**
 * @fn void cbuffer_discard_all(struct cbuffer *b)
 * @brief Resets the buffer to empty, discarding all data.
 *
 * @param b The cbuffer to reset.
 */
void cbuffer_discard_all (struct cbuffer *b);

/**
 * @fn struct bytes cbuffer_get_next_avail_bytes(const struct cbuffer *b)
 * @brief Returns a bytes view of the next contiguous free region in the backing
 * array.
 *
 * @param b The cbuffer context.
 * @return Fragmented view of free continuous memory.
 */
struct bytes cbuffer_get_next_avail_bytes (const struct cbuffer *b);

/**
 * @fn struct bytes cbuffer_get_next_data_bytes(const struct cbuffer *b)
 * @brief Returns a bytes view of the next contiguous data region in the backing
 * array.
 *
 * @param b The cbuffer context.
 * @return Fragmented view of active data contiguous memory.
 */
struct bytes cbuffer_get_next_data_bytes (const struct cbuffer *b);

/**
 * @fn void cbuffer_fakeread(struct cbuffer *b, u32 bytes)
 * @brief Advances the tail pointer by bytes, as if that many bytes were read.
 *
 * @param b The target cbuffer.
 * @param bytes Number of byte indices to advance.
 */
void cbuffer_fakeread (struct cbuffer *b, u32 bytes);

/**
 * @fn void cbuffer_fakewrite(struct cbuffer *b, u32 bytes)
 * @brief Advances the head pointer by bytes, as if that many bytes were
 * written.
 *
 * @param b The target cbuffer.
 * @param bytes Number of byte indices to advance.
 */
void cbuffer_fakewrite (struct cbuffer *b, u32 bytes);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Raw Read / Write from bytes
 * @brief Reading and writing to cbuffers from / to bytes
 *----------------------------------------------------------------------------*/

/**
 * @fn u32 cbuffer_read(void *dest, u32 size, u32 n, struct cbuffer *b)
 * @brief Consumes elements out of the ring buffer into a destination buffer.
 *
 * @param dest Memory target destination.
 * @param size Unit size of elements.
 * @param n Explicit quantity of elements to transfer.
 * @param b The source ring buffer.
 * @return Elements successfully processed.
 */
u32 cbuffer_read (void *dest, u32 size, u32 n, struct cbuffer *b);

/**
 * @fn u32 cbuffer_copy(void *dest, u32 size, u32 n, const struct cbuffer *b)
 * @brief Inspects elements out of the ring buffer without mutating tracking
 * pointers.
 *
 * @param dest Memory target destination.
 * @param size Unit size of elements.
 * @param n Explicit quantity of elements to view.
 * @param b The source ring buffer.
 * @return Elements successfully copied.
 */
u32 cbuffer_copy (void *dest, u32 size, u32 n, const struct cbuffer *b);

/**
 * @fn u32 cbuffer_write(const void *src, u32 size, u32 n, struct cbuffer *b)
 * @brief Appends elements from an external slice onto the ring buffer payload.
 *
 * @param src Memory source data pointer.
 * @param size Unit size of elements.
 * @param n Explicit quantity of elements to add.
 * @param b The target ring buffer.
 * @return Elements successfully written.
 */
u32 cbuffer_write (const void *src, u32 size, u32 n, struct cbuffer *b);

/**
 * @def cbuffer_read_expect
 * @brief Reads exactly n elements - ASSERTs if the buffer does not have enough
 * data.
 */
#define cbuffer_read_expect(dest, size, n, b)     \
  do {                                            \
    u32 __read = cbuffer_read (dest, size, n, b); \
    ASSERT (__read == n);                         \
  }                                               \
  while (0)

/**
 * @def cbuffer_write_expect
 * @brief Writes exactly n elements - ASSERTs if the buffer does not have enough
 * space.
 */
#define cbuffer_write_expect(src, size, n, b)        \
  do {                                               \
    u32 __written = cbuffer_write (src, size, n, b); \
    ASSERT (__written == n);                         \
  }                                                  \
  while (0)

/*-----------------------------------------------------------------------------
 * SUBSECTION: Raw Read / Write from other cbuffers
 * @brief Reading and writing to cbuffers from / to other cbuffers
 *----------------------------------------------------------------------------*/

/**
 * @fn u32 cbuffer_cbuffer_move(struct cbuffer *dest, u32 size, u32 n, struct
 * cbuffer *src)
 * @brief Dequeues items out of a source buffer and pushes them directly onto a
 * destination buffer.
 *
 * @param dest Target ring buffer destination.
 * @param size Sizing dimensions of items.
 * @param n Explicit quantity to transfer.
 * @param src Source ring buffer generator.
 * @return Total elements moved.
 */
u32 cbuffer_cbuffer_move (struct cbuffer *dest, u32 size, u32 n, struct cbuffer *src);

/**
 * @fn u32 cbuffer_cbuffer_copy(struct cbuffer *dest, u32 size, u32 n, const
 * struct cbuffer *src)
 * @brief Copies items out of a source buffer and pushes them directly onto a
 * destination buffer without eviction.
 *
 * @param dest Target ring buffer destination.
 * @param size Sizing dimensions of items.
 * @param n Explicit quantity to clone.
 * @param src Source ring buffer reference container.
 * @return Total elements copied.
 */
u32 cbuffer_cbuffer_copy (struct cbuffer *dest, u32 size, u32 n, const struct cbuffer *src);

/**
 * @def cbuffer_cbuffer_move_max
 * @brief Evicts and moves all tracked active data elements safely between
 * contexts.
 */
#define cbuffer_cbuffer_move_max(dest, src) cbuffer_cbuffer_move (dest, 1, cbuffer_len (src), src)

/**
 * @def cbuffer_cbuffer_copy_max
 * @brief Copies all tracked active data elements safely between contexts.
 */
#define cbuffer_cbuffer_copy_max(dest, src) cbuffer_cbuffer_copy (dest, 1, cbuffer_len (src), src)

/*-----------------------------------------------------------------------------
 * SUBSECTION: IO Read / Writing
 * @brief Reading and writing to cbuffers from / to files
 *----------------------------------------------------------------------------*/

/**
 * @fn i32 cbuffer_write_to_file_1(i_file *dest, const struct cbuffer *b, u32
 * len, error *e)
 * @brief Stage-one pipeline flush writing content blocks directly to descriptor
 * files.
 *
 * @param dest Destination file handle.
 * @param b Source ring buffer containing elements.
 * @param len Exact metrics representing byte transfer lengths.
 * @param e Error reporting instance container.
 * @return Tracked metric status representing processed fields.
 */
i32 cbuffer_write_to_file_1 (i_file *dest, const struct cbuffer *b, u32 len, error *e);

/**
 * @fn err_t cbuffer_write_to_file_1_expect(i_file *dest, const struct cbuffer
 * *b, u32 len, error *e)
 * @brief Stage-one pipeline file flush asserting that errors do not populate.
 *
 * @param dest Destination file handle.
 * @param b Source ring buffer containing elements.
 * @param len Exact metrics representing byte transfer lengths.
 * @param e Error reporting instance container.
 * @return Code validation metrics verifying the operation.
 */
err_t cbuffer_write_to_file_1_expect (i_file *dest, const struct cbuffer *b, u32 len, error *e);

/**
 * @fn void cbuffer_write_to_file_2(struct cbuffer *b, u32 nwritten)
 * @brief Stage-two pipeline handler tracking written blocks and updating read
 * offsets.
 *
 * @param b Target operational buffer container.
 * @param nwritten Completed total byte outputs processed.
 */
void cbuffer_write_to_file_2 (struct cbuffer *b, u32 nwritten);

/**
 * @fn i32 cbuffer_write_to_file(i_file *dest, struct cbuffer *b, u32 len, error
 * *e)
 * @brief Consolidated write utility piping data blocks directly onto disk
 * structures.
 *
 * @param dest Target storage stream container file.
 * @param b Target operational buffer container.
 * @param len Desired scale length requested for conversion.
 * @param e Tracker catching operational framework faults.
 * @return Output indicator metrics.
 */
i32 cbuffer_write_to_file (i_file *dest, struct cbuffer *b, u32 len, error *e);

/**
 * @fn i32 cbuffer_read_from_file_1(i_file *src, const struct cbuffer *b, u32
 * len, error *e)
 * @brief Stage-one storage system call tracking input read sizes from
 * descriptor objects.
 *
 * @param src Source tracking file descriptor container.
 * @param b Target ring buffer receiving fields.
 * @param len Target processing byte boundary constraints.
 * @param e Error tracking storage.
 * @return Read verification data loops.
 */
i32 cbuffer_read_from_file_1 (i_file *src, const struct cbuffer *b, u32 len, error *e);

/**
 * @fn err_t cbuffer_read_from_file_1_expect(i_file *src, const struct cbuffer
 * *b, u32 len, error *e)
 * @brief Stage-one pipeline read checking that descriptor fetches pass
 * constraints.
 *
 * @param src Source tracking file descriptor container.
 * @param b Target ring buffer receiving fields.
 * @param len Target processing byte boundary constraints.
 * @param e Error tracking storage.
 * @return Validation context verification fields.
 */
err_t cbuffer_read_from_file_1_expect (i_file *src, const struct cbuffer *b, u32 len, error *e);

/**
 * @fn void cbuffer_read_from_file_2(struct cbuffer *b, u32 nread)
 * @brief Stage-two tracking step modifying internal write offsets after reading
 * from disk.
 *
 * @param b Target buffer managing tracking indices.
 * @param nread Concrete verified elements count extracted.
 */
void cbuffer_read_from_file_2 (struct cbuffer *b, u32 nread);

/**
 * @fn i32 cbuffer_read_from_file(i_file *src, struct cbuffer *b, u32 len, error
 * *e)
 * @brief Consolidated pipeline action piping files explicitly back into
 * operational storage pools.
 *
 * @param src Source input track file.
 * @param b Target system receiver ring buffer context.
 * @param len Limit constraints evaluating operational boundaries.
 * @param e Tracker logging environment runtime validation exceptions.
 * @return State verification markers.
 */
i32 cbuffer_read_from_file (i_file *src, struct cbuffer *b, u32 len, error *e);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Single Element Read / Write
 * @brief Writing single elements
 *----------------------------------------------------------------------------*/

/**
 * @fn bool cbuffer_get(void *dest, u32 size, u32 idx, const struct cbuffer *b)
 * @brief Indexing accessor pulling elements safely from specific index markers.
 *
 * @param dest Storage location reference target.
 * @param size Uniform dimension scaling metrics.
 * @param idx Position tracking variable target.
 * @param b Targeted buffer data repository context.
 * @return True if valid match found, false otherwise.
 */
bool cbuffer_get (void *dest, u32 size, u32 idx, const struct cbuffer *b);

/**
 * @fn bool cbuffer_push_back(const void *src, u32 size, struct cbuffer *b)
 * @brief Pushes a single item onto the trailing frame edge of the ring array.
 *
 * @param src Element item source locator.
 * @param size Byte spacing metrics.
 * @param b Target tracking context receiver.
 * @return Operational verification flag.
 */
bool cbuffer_push_back (const void *src, u32 size, struct cbuffer *b);

/**
 * @fn bool cbuffer_push_front(const void *src, u32 size, struct cbuffer *b)
 * @brief Pushes a single item onto the leading boundary edge of the ring array.
 *
 * @param src Element item source locator.
 * @param size Byte spacing metrics.
 * @param b Target tracking context receiver.
 * @return Operational verification flag.
 */
bool cbuffer_push_front (const void *src, u32 size, struct cbuffer *b);

/**
 * @fn bool cbuffer_pop_back(void *dest, u32 size, struct cbuffer *b)
 * @brief Pops an item out from the trailing frame boundary edge safely.
 *
 * @param dest Storage verification target interface pointer.
 * @param size Element footprint constraints metrics.
 * @param b Context model tracking values.
 * @return Validation confirmation indicator flags.
 */
bool cbuffer_pop_back (void *dest, u32 size, struct cbuffer *b);

/**
 * @fn bool cbuffer_pop_front(void *dest, u32 size, struct cbuffer *b)
 * @brief Pops an item out from the leading boundary line edge safely.
 *
 * @param dest Storage verification target interface pointer.
 * @param size Element footprint constraints metrics.
 * @param b Context model tracking values.
 * @return Validation confirmation indicator flags.
 */
bool cbuffer_pop_front (void *dest, u32 size, struct cbuffer *b);

/**
 * @fn bool cbuffer_peek_back(void *dest, u32 size, const struct cbuffer *b)
 * @brief Non-destructively clones contents residing at trailing array offsets.
 *
 * @param dest Output data mirror workspace.
 * @param size Explicit element allocation thresholds.
 * @param b Constant system tracking state context block.
 * @return Verification tracking output indicators.
 */
bool cbuffer_peek_back (void *dest, u32 size, const struct cbuffer *b);

/**
 * @fn bool cbuffer_peek_front(void *dest, u32 size, const struct cbuffer *b)
 * @brief Non-destructively clones contents residing at leading head pointer
 * locations.
 *
 * @param dest Output data mirror workspace.
 * @param size Explicit element allocation thresholds.
 * @param b Constant system tracking state context block.
 * @return Verification tracking output indicators.
 */
bool cbuffer_peek_front (void *dest, u32 size, const struct cbuffer *b);

/**
 * @def cbuffer_push_back_expect
 * @brief Pushes an item to the trailing boundary edge - ASSERTs if full.
 */
#define cbuffer_push_back_expect(src, size, b)     \
  do {                                             \
    bool __ret = cbuffer_push_back (src, size, b); \
    ASSERT (__ret);                                \
  }                                                \
  while (0)

/**
 * @def cbuffer_push_front_expect
 * @brief Pushes an item to the leading boundary edge - ASSERTs if full.
 */
#define cbuffer_push_front_expect(src, size, b)     \
  do {                                              \
    bool __ret = cbuffer_push_front (src, size, b); \
    ASSERT (__ret);                                 \
  }                                                 \
  while (0)

/**
 * @def cbuffer_pop_back_expect
 * @brief Pops an item from the trailing boundary edge - ASSERTs if empty.
 */
#define cbuffer_pop_back_expect(dest, size, b)     \
  do {                                             \
    bool __ret = cbuffer_pop_back (dest, size, b); \
    ASSERT (__ret);                                \
  }                                                \
  while (0)

/**
 * @def cbuffer_pop_front_expect
 * @brief Pops an item from the leading boundary edge - ASSERTs if empty.
 */
#define cbuffer_pop_front_expect(dest, size, b)     \
  do {                                              \
    bool __ret = cbuffer_pop_front (dest, size, b); \
    ASSERT (__ret);                                 \
  }                                                 \
  while (0)

/**
 * @def cbuffer_peek_back_expect
 * @brief Peeks at the trailing boundary edge - ASSERTs if empty.
 */
#define cbuffer_peek_back_expect(dest, size, b)     \
  do {                                              \
    bool __ret = cbuffer_peek_back (dest, size, b); \
    ASSERT (__ret);                                 \
  }                                                 \
  while (0)

/**
 * @def cbuffer_peek_front_expect
 * @brief Peeks at the leading boundary edge - ASSERTs if empty.
 */
#define cbuffer_peek_front_expect(dest, size, b)     \
  do {                                               \
    bool __ret = cbuffer_peek_front (dest, size, b); \
    ASSERT (__ret);                                  \
  }                                                  \
  while (0)

/**
 * @def cbuffer_pushb_back_expect
 * @brief Explicitly pushes a single raw 8-bit byte value onto the back of the
 * buffer.
 */
#define cbuffer_pushb_back_expect(src, b)         \
  do {                                            \
    u8   _src  = src;                             \
    bool __ret = cbuffer_push_back (&_src, 1, b); \
    ASSERT (__ret);                               \
  }                                               \
  while (0)

/**
 * @def cbuffer_pushb_front_expect
 * @brief Explicitly pushes a single raw 8-bit byte value onto the front of the
 * buffer.
 */
#define cbuffer_pushb_front_expect(src, b)         \
  do {                                             \
    u8   _src  = src;                              \
    bool __ret = cbuffer_push_front (&_src, 1, b); \
    ASSERT (__ret);                                \
  }                                                \
  while (0)

#endif
