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
 * @brief Functions and data structures on bytes or strings
 */

#ifndef NS_STREAM_H
#define NS_STREAM_H

#include "core/ns_error.h"
#include "core/ns_ext_array.h"
#include "core/ns_stdtypes.h"

#include <stdatomic.h>
#include <stdbool.h>

/******************************************************************************
 * SECTION: Stream
 * ----------------------------------------------------------------------------
 * @brief A polymorphic byte-oriented I/O interface like C++ "stream"
 *
 * A stream wraps a (pull, push, close) vtable
 * The `done` flag signals end-of-data; a stream sets it via
 * stream_finish() when it has no more bytes to produce or accept.  Callers
 * test it with stream_isdone() to decide when to stop reading or writing.
 *
 *   stream_bread(dest, size, n, src)  - pull up to n elements of [size] bytes
 *                                       from src into the dest buffer.
 *   stream_bwrite(buf, size, n, dest) - push n elements of [size] bytes from
 *                                       buf into dest.
 *   stream_read(dest, size, n, src)   - stream-to-stream copy.
 *
 * All three return the number of elements transferred (>= 0) or a negative
 * error code.  A return value smaller than n does not indicate an error;
 * the caller should check stream_isdone() to distinguish short-read from
 * error.
 *
 * Concrete stream implementations included here:
 *   stream_ibuf    - pulls from a fixed const byte buffer (read source).
 *   stream_obuf    - pushes into a fixed mutable byte buffer (write sink).
 ******************************************************************************/

struct stream;

/**
 * @fn i32 (*stream_pull_fn)(struct stream *s, void *ctx, void *buf, u32 size,
 * u32 n, error *e)
 * @brief Pull function pointer interface.
 *
 * @param s The stream being read.
 * @param ctx Implementation-defined context.
 * @param buf Destination buffer to receive the data.
 * @param size Size of each element in bytes.
 * @param n Maximum number of elements to pull.
 * @param e The error object.
 * @return Number of elements successfully pulled.
 */
typedef i32 (*stream_pull_fn) (struct stream *s, void *ctx, void *buf, u32 size, u32 n, error *e);

/**
 * @fn i32 (*stream_push_fn)(struct stream *s, void *ctx, const void *buf, u32
 * size, u32 n, error *e)
 * @brief Function pointer type for pushing bytes from a caller buffer into a
 * stream.
 *
 * @param s The stream being written.
 * @param ctx Implementation-defined context.
 * @param buf Source buffer containing the data to push.
 * @param size Size of each element in bytes.
 * @param n Number of elements to push.
 * @param e The error object.
 * @return Number of elements successfully pushed.
 */
typedef i32 (*stream_push_fn) (
    struct stream *s,
    void          *ctx,
    const void    *buf,
    u32            size,
    u32            n,
    error         *e
);

/**
 * @fn void (*stream_close_fn)(void *ctx)
 * @brief Function pointer type for releasing any resources held by a stream
 * implementation.
 *
 * @param ctx Implementation-defined context.
 */
typedef void (*stream_close_fn) (void *ctx);

/**
 * @struct stream_ops
 * @brief Vtable of operations backing a stream.
 *
 * @var stream_ops::pull
 * @brief Pull bytes out of the stream (may be NULL for write-only streams).
 * @var stream_ops::push
 * @brief Push bytes into the stream (may be NULL for read-only streams).
 * @var stream_ops::close
 * @brief Release resources held by the stream (may be NULL).
 */
struct stream_ops
{
  stream_pull_fn  pull;
  stream_push_fn  push;
  stream_close_fn close;
};

/**
 * @struct stream
 * @brief A polymorphic byte-oriented I/O stream.
 *
 * @var stream::ops
 * @brief Vtable of stream operations.
 * @var stream::ctx
 * @brief Opaque context passed to every vtable call.
 * @var stream::done
 * @brief Non-zero once the stream has no more data to produce or accept.
 */
struct stream
{
  const struct stream_ops *ops;
  void                    *ctx;
  atomic_int               done;
};

/**
 * @fn void stream_init(struct stream *s, const struct stream_ops *ops, void
 * *ctx)
 * @brief Initializes a stream with a given vtable and context.
 *
 * @param s Stream to initialize.
 * @param ops Vtable to attach.
 * @param ctx Opaque context to attach.
 */
void stream_init (struct stream *s, const struct stream_ops *ops, void *ctx);

/**
 * @fn void stream_close(const struct stream *s)
 * @brief Calls the stream's close function and releases any implementation
 * resources.
 *
 * @param s Stream to close.
 */
void stream_close (const struct stream *s);

/**
 * @fn void stream_finish(struct stream *s)
 * @brief Marks a stream as done, signaling to callers that no more data will be
 * produced or accepted.
 *
 * @param s Stream to mark done.
 */
void stream_finish (struct stream *s);

/**
 * @fn bool stream_isdone(const struct stream *s)
 * @brief Returns true if the stream has been marked done via stream_finish().
 *
 * @param s Stream to test.
 */
bool stream_isdone (const struct stream *s);

/**
 * @brief Copies up to n elements of size bytes from src to dest via their
 * stream interfaces.
 *
 * @param dest Destination stream to push into.
 * @param size Size of each element in bytes.
 * @param n Maximum number of elements to transfer.
 * @param src Source stream to pull from.
 * @param e The error object.
 * @return Number of elements successfully transferred.
 */
i32 stream_read (struct stream *dest, u32 size, u32 n, struct stream *src, error *e);

/**
 * @fn i32 stream_bread(void *dest, u32 size, u32 n, struct stream *src, error
 * *e)
 * @brief Pulls up to n elements of size bytes from src into a raw buffer.
 *
 * @param dest Destination buffer to receive the data.
 * @param size Size of each element in bytes.
 * @param n Maximum number of elements to pull.
 * @param src Source stream to pull from.
 * @param e The error object.
 * @return Number of elements successfully pulled.
 */
i32 stream_bread (void *dest, u32 size, u32 n, struct stream *src, error *e);

/**
 * @fn i32 stream_bwrite(const void *buf, u32 size, u32 n, struct stream *dest,
 * error *e)
 * @brief Pushes n elements of size bytes from a raw buffer into dest.
 *
 * @param buf Source buffer containing the data to push.
 * @param size Size of each element in bytes.
 * @param n Number of elements to push.
 * @param dest Destination stream to push into.
 * @param e The error object.
 * @return Number of elements successfully pushed.
 */
i32 stream_bwrite (const void *buf, u32 size, u32 n, struct stream *dest, error *e);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Special Streams
 * @brief Commonly used streams
 *
 *
 * 1. ibuf - An input buffer stream that reads from a byte buffer
 * 2. obuf - An output buffer stream that writes to a byte buffer
 * 3. sink - Just ignores bytes
 *----------------------------------------------------------------------------*/

/**
 * @struct stream_ibuf_ctx
 * @brief Context for a read-only stream backed by a fixed const byte buffer.
 *
 * @var stream_ibuf_ctx::buf
 * @brief Pointer to the source buffer.
 * @var stream_ibuf_ctx::size
 * @brief Total number of bytes in buf.
 * @var stream_ibuf_ctx::pos
 * @brief Current read position in bytes.
 */
struct stream_ibuf_ctx
{
  const u8 *buf;
  u32       size;
  u32       pos;
};

/**
 * @brief Context for a write-only stream that writes into a fixed mutable byte
 * buffer.
 *
 * @var stream_obuf_ctx::buf
 * @brief Pointer to the destination buffer.
 * @var stream_obuf_ctx::cap
 * @brief Total capacity of buf in bytes.
 * @var stream_obuf_ctx::pos
 * @brief Current write position in bytes.
 */
struct stream_obuf_ctx
{
  u8 *buf;
  u32 cap;
  u32 pos;
};

struct stream_dyn_obuf_ctx
{
  struct ext_array buffer;
  u32              limit;
};

/**
 * @fn void stream_ibuf_init(struct stream *s, struct stream_ibuf_ctx *ctx,
 * const void *buf, u32 size)
 * @brief Initializes a read-only stream that pulls from a fixed const byte
 * buffer.
 *
 * @param s Stream to initialize.
 * @param ctx Context to initialize and attach.
 * @param buf Source buffer to read from.
 * @param size Number of bytes in buf.
 */
void stream_ibuf_init (struct stream *s, struct stream_ibuf_ctx *ctx, const void *buf, u32 size);

/**
 * @fn void stream_obuf_init(struct stream *s, struct stream_obuf_ctx *ctx, void
 * *buf, u32 cap)
 * @brief Initializes a write-only stream that pushes into a fixed mutable byte
 * buffer.
 *
 * @param s Stream to initialize.
 * @param ctx Context to initialize and attach.
 * @param buf Destination buffer to write into.
 * @param cap Capacity of buf in bytes.
 */
void stream_obuf_init (struct stream *s, struct stream_obuf_ctx *ctx, void *buf, u32 cap);

/**
 * @fn void stream_obuf_init(struct stream *s, struct stream_obuf_ctx *ctx, void
 * *buf, u32 cap)
 * @brief Initializes a write-only stream that pushes into a fixed mutable byte
 * buffer.
 *
 * @param s Stream to initialize.
 * @param ctx Context to initialize and attach.
 * @param buf Destination buffer to write into.
 * @param cap Capacity of buf in bytes.
 */
void stream_dyn_obuf_init (struct stream *s, struct stream_dyn_obuf_ctx *ctx, u32 limit);

#endif
