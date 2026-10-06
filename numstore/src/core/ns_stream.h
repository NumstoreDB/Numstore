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

struct stream;

typedef i32 (*stream_pull_fn) (struct stream *s, void *ctx, void *buf, u32 size, u32 n, error *e);
typedef i32 (*stream_push_fn) (
    struct stream *s,
    void          *ctx,
    const void    *buf,
    u32            size,
    u32            n,
    error         *e
);
typedef void (*stream_close_fn) (void *ctx);

struct stream_ops
{
  stream_pull_fn  pull;
  stream_push_fn  push;
  stream_close_fn close;
};

struct stream
{
  const struct stream_ops *ops;
  void                    *ctx;
  atomic_int               done;
};

void stream_init (struct stream *s, const struct stream_ops *ops, void *ctx);
void stream_close (const struct stream *s);
void stream_finish (struct stream *s);
bool stream_isdone (const struct stream *s);
i32 stream_read (struct stream *dest, u32 size, u32 n, struct stream *src, error *e);
i32 stream_bread (void *dest, u32 size, u32 n, struct stream *src, error *e);
i32 stream_bwrite (const void *buf, u32 size, u32 n, struct stream *dest, error *e);

struct stream_ibuf_ctx
{
  const u8 *buf;
  u32       size;
  u32       pos;
};

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

void stream_ibuf_init (struct stream *s, struct stream_ibuf_ctx *ctx, const void *buf, u32 size);
void stream_obuf_init (struct stream *s, struct stream_obuf_ctx *ctx, void *buf, u32 cap);
void stream_dyn_obuf_init (struct stream *s, struct stream_dyn_obuf_ctx *ctx, u32 limit);

#define istream_create_from(name, buffer, size) \
  struct stream          name;                  \
  struct stream_ibuf_ctx name##_ctx;            \
  stream_ibuf_init (&name, &name##_ctx, (buffer), (size))

#define istream_create_from_array(name, buffer) \
  istream_create_from (name, (buffer), sizeof (buffer))

#define ostream_create_from(name, buffer, size) \
  struct stream          name;                  \
  struct stream_obuf_ctx name##_ctx;            \
  stream_obuf_init (&name, &name##_ctx, (buffer), (size))

#define ostream_create_from_array(name, buffer) \
  ostream_create_from (name, (buffer), sizeof (buffer))

#endif
