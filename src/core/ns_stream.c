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

#include "core/ns_stream.h"

#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_utils.h"
#include "core/testing/ns_testing.h"

#include <string.h>

/////////////////////////////////////////////////////////////////////
////// Stream

void
stream_init (struct stream *s, const struct stream_ops *ops, void *ctx)
{
  s->ops  = ops;
  s->ctx  = ctx;
  s->done = false;
}

#ifdef TESTING

static void
test_close_flag_fn (void *vctx)
{
  bool *flag = (bool *)vctx;
  *flag      = true;
}

static const struct stream_ops test_close_ops = {
    .pull  = NULL,
    .push  = NULL,
    .close = test_close_flag_fn,
};

static const struct stream_ops test_noclose_ops = {
    .pull  = NULL,
    .push  = NULL,
    .close = NULL,
};

TEST (stream_init)
{
  TEST_CASE ("sets ops, ctx, and done fields")
  {
    bool          dummy_ctx = false;
    struct stream s;

    stream_init (&s, &test_noclose_ops, &dummy_ctx);

    test_assert (s.ops == &test_noclose_ops);
    test_assert (s.ctx == &dummy_ctx);
    test_assert (!stream_isdone (&s));
  }
}

#endif // TESTING

void
stream_finish (struct stream *s)
{
  atomic_store_explicit (&s->done, true, memory_order_release);
}

#ifdef TESTING

TEST (stream_finish)
{
  TEST_CASE ("marks the stream done")
  {
    struct stream s;
    stream_init (&s, &test_noclose_ops, NULL);

    test_assert (!stream_isdone (&s));
    stream_finish (&s);
    test_assert (stream_isdone (&s));

    // Finishing an already-finished stream should be a harmless no-op.
    stream_finish (&s);
    test_assert (stream_isdone (&s));
  }
}

#endif // TESTING

bool
stream_isdone (const struct stream *s)
{
  return atomic_load_explicit (&s->done, memory_order_acquire);
}

#ifdef TESTING

TEST (stream_isdone)
{
  TEST_CASE ("reflects finish state")
  {
    struct stream s;
    stream_init (&s, &test_noclose_ops, NULL);

    test_assert (!stream_isdone (&s));
    stream_finish (&s);
    test_assert (stream_isdone (&s));
  }
}

#endif // TESTING

void
stream_close (const struct stream *s)
{
  if (s->ops->close) {
    s->ops->close (s->ctx);
  }
}

#ifdef TESTING

TEST (stream_close)
{
  TEST_CASE ("invokes close callback")
  {
    bool          flag = false;
    struct stream s;

    stream_init (&s, &test_close_ops, &flag);
    stream_close (&s);

    test_assert (flag);
  }

  TEST_CASE ("no-op when close is null")
  {
    struct stream s;

    stream_init (&s, &test_noclose_ops, NULL);

    // Should not crash even though close is NULL.
    stream_close (&s);
    test_assert (true);
  }
}

#endif // TESTING

i32
stream_bread (void *dest, const u32 size, const u32 n, struct stream *src, error *e)
{
  return src->ops->pull (src, src->ctx, dest, size, n, e);
}

#ifdef TESTING

struct test_pull_ctx
{
  const u8 *data;
  u32       len;
  u32       pos;
};

static i32
test_pull_fn (struct stream *s, void *vctx, void *dest, const u32 size, const u32 n, error *e)
{
  (void)e; // Unused
  struct test_pull_ctx *ctx   = (struct test_pull_ctx *)vctx;

  const u32             avail = ctx->len - ctx->pos;
  const u32             want  = size * n;
  const u32             next  = MIN (avail, want);

  if (next == 0) {
    stream_finish (s);
    return 0;
  }

  memcpy (dest, ctx->data + ctx->pos, next);
  ctx->pos += next;

  return (i32)(next / size);
}

static const struct stream_ops test_pull_ops = {
    .pull  = test_pull_fn,
    .push  = NULL,
    .close = NULL,
};

TEST (stream_bread)
{
  TEST_CASE ("reads requested data")
  {
    const u8             data[] = {1, 2, 3, 4, 5, 6, 7, 8};
    struct test_pull_ctx ctx    = {.data = data, .len = sizeof (data), .pos = 0};
    struct stream        src;

    stream_init (&src, &test_pull_ops, &ctx);

    u8    buf[4] = {0};
    error e      = error_create ();

    i32   got    = stream_bread (buf, 1, 4, &src, &e);

    test_assert_int_equal (got, 4);
    test_assert (memcmp (buf, data, 4) == 0);
    test_assert (!stream_isdone (&src));
  }

  TEST_CASE ("partial read then exhausted")
  {
    const u8             data[] = {9, 8, 7};
    struct test_pull_ctx ctx    = {.data = data, .len = sizeof (data), .pos = 0};
    struct stream        src;

    stream_init (&src, &test_pull_ops, &ctx);

    u8    buf[10] = {0};
    error e       = error_create ();

    i32   got     = stream_bread (buf, 1, 10, &src, &e);
    test_assert_int_equal (got, 3);
    test_assert (memcmp (buf, data, 3) == 0);

    got = stream_bread (buf, 1, 10, &src, &e);
    test_assert_int_equal (got, 0);
    test_assert (stream_isdone (&src));
  }
}

#endif // TESTING

i32
stream_bwrite (const void *buf, const u32 size, const u32 n, struct stream *dest, error *e)
{
  return dest->ops->push (dest, dest->ctx, buf, size, n, e);
}

#ifdef TESTING

struct test_push_ctx
{
  u8 *data;
  u32 cap;
  u32 pos;
};

static i32
test_push_fn (struct stream *s, void *vctx, const void *src, const u32 size, const u32 n, error *e)
{
  (void)e; // Unused
  struct test_push_ctx *ctx   = (struct test_push_ctx *)vctx;

  const u32             avail = ctx->cap - ctx->pos;
  const u32             want  = size * n;
  const u32             next  = MIN (avail, want);

  if (next == 0) {
    stream_finish (s);
    return 0;
  }

  memcpy (ctx->data + ctx->pos, src, next);
  ctx->pos += next;

  return (i32)(next / size);
}

static const struct stream_ops test_push_ops = {
    .pull  = NULL,
    .push  = test_push_fn,
    .close = NULL,
};

TEST (stream_bwrite)
{
  TEST_CASE ("writes requested data")
  {
    u8                   out[8] = {0};
    struct test_push_ctx ctx    = {.data = out, .cap = sizeof (out), .pos = 0};
    struct stream        dest;

    stream_init (&dest, &test_push_ops, &ctx);

    const u8 src[4] = {10, 20, 30, 40};
    error    e      = error_create ();

    i32      put    = stream_bwrite (src, 1, 4, &dest, &e);

    test_assert_int_equal (put, 4);
    test_assert (memcmp (out, src, 4) == 0);
    test_assert (!stream_isdone (&dest));
  }

  TEST_CASE ("partial write then exhausted")
  {
    u8                   out[3] = {0};
    struct test_push_ctx ctx    = {.data = out, .cap = sizeof (out), .pos = 0};
    struct stream        dest;

    stream_init (&dest, &test_push_ops, &ctx);

    const u8 src[5] = {1, 2, 3, 4, 5};
    error    e      = error_create ();

    i32      put    = stream_bwrite (src, 1, 5, &dest, &e);
    test_assert_int_equal (put, 3);
    test_assert (memcmp (out, src, 3) == 0);

    put = stream_bwrite (src, 1, 5, &dest, &e);
    test_assert_int_equal (put, 0);
    test_assert (stream_isdone (&dest));
  }
}

#endif // TESTING

i32
stream_read (struct stream *dest, const u32 size, const u32 n, struct stream *src, error *e)
{
  ASSERT (dest->ops->push);
  ASSERT (src->ops->pull);

  u8  buf[4096];
  u32 total     = 0;
  u32 remaining = n;
  u32 batch_max = sizeof (buf) / size;

  if (batch_max == 0) {
    batch_max = 1;
  }

  while (remaining > 0) {
    const u32 batch = remaining < batch_max ? remaining : batch_max;

    const i32 got   = src->ops->pull (src, src->ctx, buf, size, batch, e);
    if (got < 0) {
      return got;
    }
    if (got == 0) {
      break;
    }

    u32 pushed = 0;
    while (pushed < (u32)got) {
      const i32 w = dest->ops->push (dest, dest->ctx, buf + (pushed * size), size, got - pushed, e);
      if (w < 0) {
        return w;
      }
      if (w == 0) {
        break;
      }
      pushed += w;
    }

    total += pushed;
    remaining -= pushed;

    if (pushed < (u32)got) {
      break;
    }
  }

  return total;
}

#ifdef TESTING

TEST (stream_read)
{
  TEST_CASE ("full transfer")
  {
    const u8             data[6] = {1, 2, 3, 4, 5, 6};
    struct test_pull_ctx pctx    = {.data = data, .len = sizeof (data), .pos = 0};
    struct stream        src;

    stream_init (&src, &test_pull_ops, &pctx);

    u8                   out[6] = {0};
    struct test_push_ctx octx   = {.data = out, .cap = sizeof (out), .pos = 0};
    struct stream        dest;

    stream_init (&dest, &test_push_ops, &octx);

    error e     = error_create ();
    i32   total = stream_read (&dest, 1, 6, &src, &e);

    test_assert_int_equal (total, 6);
    test_assert (memcmp (out, data, 6) == 0);
  }

  TEST_CASE ("multi batch exceeds internal buffer")
  {
    // Forces stream_read's internal 4096-byte staging buffer to be
    // reused across multiple pull/push iterations.
    enum
    {
      N = 10000
    };

    static u8 data[N];
    static u8 out[N];

    for (u32 i = 0; i < N; i++) {
      data[i] = (u8)(i & 0xFF);
    }
    memset (out, 0, N);

    struct test_pull_ctx pctx = {.data = data, .len = N, .pos = 0};
    struct stream        src;
    stream_init (&src, &test_pull_ops, &pctx);

    struct test_push_ctx octx = {.data = out, .cap = N, .pos = 0};
    struct stream        dest;
    stream_init (&dest, &test_push_ops, &octx);

    error e     = error_create ();
    i32   total = stream_read (&dest, 1, N, &src, &e);

    test_assert_int_equal (total, N);
    test_assert (memcmp (out, data, N) == 0);
  }

  TEST_CASE ("source exhausts before n")
  {
    const u8             data[3] = {7, 8, 9};
    struct test_pull_ctx pctx    = {.data = data, .len = 3, .pos = 0};
    struct stream        src;

    stream_init (&src, &test_pull_ops, &pctx);

    u8                   out[10] = {0};
    struct test_push_ctx octx    = {.data = out, .cap = 10, .pos = 0};
    struct stream        dest;

    stream_init (&dest, &test_push_ops, &octx);

    error e     = error_create ();
    i32   total = stream_read (&dest, 1, 10, &src, &e);

    test_assert_int_equal (total, 3);
    test_assert (memcmp (out, data, 3) == 0);
    test_assert (stream_isdone (&src));
  }

  TEST_CASE ("dest has insufficient capacity")
  {
    const u8             data[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    struct test_pull_ctx pctx     = {.data = data, .len = 10, .pos = 0};
    struct stream        src;

    stream_init (&src, &test_pull_ops, &pctx);

    u8                   out[7] = {0};
    struct test_push_ctx octx   = {.data = out, .cap = 7, .pos = 0};
    struct stream        dest;

    stream_init (&dest, &test_push_ops, &octx);

    error e     = error_create ();
    i32   total = stream_read (&dest, 1, 10, &src, &e);

    // Dest only has room for 7 bytes, so the transfer stops short and
    // marks dest finished even though src still has data left.
    test_assert_int_equal (total, 7);
    test_assert (memcmp (out, data, 7) == 0);
    test_assert (stream_isdone (&dest));
    test_assert (!stream_isdone (&src));
  }
}

#endif // TESTING

static i32
stream_ibuf_pull (struct stream *s, void *vctx, void *dest, const u32 size, const u32 n, error *e)
{
  (void)e; // Unused
  struct stream_ibuf_ctx *ctx = (struct stream_ibuf_ctx *)vctx;

  u32                     avail;

  if (ctx->size == 0) {
    avail = size * n;
  } else {
    avail = ctx->size - ctx->pos;
  }

  u32 want = size * n;

  u32 next = MIN (avail, want);

  if (next == 0) {
    if (avail == 0) {
      stream_finish (s);
    }
    return 0;
  }

  memcpy (dest, ctx->buf + ctx->pos, next);
  ctx->pos += next;

  return next / size;
}

static i32
stream_obuf_push (
    struct stream *s,
    void          *vctx,
    const void    *src,
    const u32      size,
    const u32      n,
    error         *e
)
{
  (void)e; // Unused
  struct stream_obuf_ctx *ctx = (struct stream_obuf_ctx *)vctx;

  u32                     avail;

  if (ctx->cap == 0) {
    avail = size * n;
  } else {
    avail = ctx->cap - ctx->pos;
  }

  u32 want = size * n;

  u32 next = MIN (avail, want);

  if (next == 0) {
    if (avail == 0) {
      stream_finish (s);
    }
    return 0;
  }

  memcpy (ctx->buf + ctx->pos, src, next);
  ctx->pos += next;

  return next / size;
}

static i32
stream_dyn_obuf_push (
    struct stream *s,
    void          *vctx,
    const void    *src,
    const u32      size,
    const u32      n,
    error         *e
)
{
  (void)s; // Unused
  struct stream_dyn_obuf_ctx *ctx  = (struct stream_dyn_obuf_ctx *)vctx;

  // Minimum available space
  u32                         next = 0;

  if (ctx->limit == 0) {
    next = size * n;
  } else {
    next = MIN (size * n, ctx->limit - ctx->buffer.len);
  }

  // Do append
  if (ext_array_insert (&ctx->buffer, ctx->buffer.len, src, n * size, e) < 0) {
    return error_trace (e);
  }

  return next;
}

static const struct stream_ops stream_ibuf_ops = {
    .pull  = stream_ibuf_pull,
    .push  = NULL,
    .close = NULL,
};

static const struct stream_ops stream_obuf_ops = {
    .pull  = NULL,
    .push  = stream_obuf_push,
    .close = NULL,
};

static const struct stream_ops stream_dyn_obuf_ops = {
    .pull  = NULL,
    .push  = stream_dyn_obuf_push,
    .close = NULL,
};

void
stream_ibuf_init (struct stream *s, struct stream_ibuf_ctx *ctx, const void *buf, const u32 size)
{
  ctx->buf  = (const u8 *)buf;
  ctx->size = size;
  ctx->pos  = 0;
  stream_init (s, &stream_ibuf_ops, ctx);
}

#ifdef TESTING

TEST (stream_ibuf)
{
  TEST_CASE ("reads sequential chunks")
  {
    const u8               data[5] = {1, 2, 3, 4, 5};

    struct stream          s;
    struct stream_ibuf_ctx ctx;

    stream_ibuf_init (&s, &ctx, data, sizeof (data));

    u8    buf[2];
    error e   = error_create ();

    i32   got = stream_bread (buf, 1, 2, &s, &e);
    test_assert_int_equal (got, 2);
    test_assert (memcmp (buf, data, 2) == 0);
    test_assert (!stream_isdone (&s));

    got = stream_bread (buf, 1, 2, &s, &e);
    test_assert_int_equal (got, 2);
    test_assert (memcmp (buf, data + 2, 2) == 0);
    test_assert (!stream_isdone (&s));

    // Only one byte remains; should clamp and mark done.
    got = stream_bread (buf, 1, 2, &s, &e);
    test_assert_int_equal (got, 1);
    test_assert_int_equal (buf[0], data[4]);

    // not done until you do 1 more read - it
    // would be nice to change this
    test_assert (!stream_isdone (&s));
    got = stream_bread (buf, 1, 2, &s, &e);
    test_assert_int_equal (got, 0);
    test_assert (stream_isdone (&s));

    got = stream_bread (buf, 1, 2, &s, &e);
    test_assert_int_equal (got, 0);
  }

  TEST_CASE ("zero n request does not finish")
  {
    const u8               data[3] = {1, 2, 3};

    struct stream          s;
    struct stream_ibuf_ctx ctx;

    stream_ibuf_init (&s, &ctx, data, sizeof (data));

    u8    buf[1];
    error e   = error_create ();

    i32   got = stream_bread (buf, 1, 0, &s, &e);

    test_assert_int_equal (got, 0);
    test_assert (!stream_isdone (&s));
  }

  TEST_CASE ("size zero is treated as unbounded")
  {
    // NOTE: ctx->size == 0 is used internally as a sentinel meaning
    // "unbounded", not "empty". A buffer initialized with size 0 will
    // happily satisfy pulls up to whatever the real backing memory
    // allows; it never reports exhaustion on its own.
    static u8 data[16];
    for (u32 i = 0; i < 16; i++) {
      data[i] = (u8)(i + 1);
    }

    struct stream          s;
    struct stream_ibuf_ctx ctx;

    stream_ibuf_init (&s, &ctx, data, 0);

    u8    buf[16] = {0};
    error e       = error_create ();

    i32   got     = stream_bread (buf, 1, 16, &s, &e);

    test_assert_int_equal (got, 16);
    test_assert (memcmp (buf, data, 16) == 0);
    test_assert (!stream_isdone (&s));
  }
}

#endif // TESTING

void
stream_obuf_init (struct stream *s, struct stream_obuf_ctx *ctx, void *buf, const u32 cap)
{
  ctx->buf = buf;
  ctx->cap = cap;
  ctx->pos = 0;
  stream_init (s, &stream_obuf_ops, ctx);
}

#ifdef TESTING

TEST (stream_obuf)
{
  TEST_CASE ("writes sequential chunks")
  {
    u8                     out[5] = {0};

    struct stream          s;
    struct stream_obuf_ctx ctx;

    stream_obuf_init (&s, &ctx, out, sizeof (out));

    const u8 chunk1[2] = {1, 2};
    const u8 chunk2[2] = {3, 4};
    const u8 chunk3[2] = {5, 6}; // only 1 byte of capacity remains

    error    e         = error_create ();

    i32      put       = stream_bwrite (chunk1, 1, 2, &s, &e);
    test_assert_int_equal (put, 2);

    put = stream_bwrite (chunk2, 1, 2, &s, &e);
    test_assert_int_equal (put, 2);

    put = stream_bwrite (chunk3, 1, 2, &s, &e);
    test_assert_int_equal (put, 1);

    // One more 0 read
    test_assert (!stream_isdone (&s));
    put = stream_bwrite (chunk3, 1, 2, &s, &e);
    test_assert_int_equal (put, 0);
    test_assert (stream_isdone (&s));

    const u8 expected[5] = {1, 2, 3, 4, 5};
    test_assert (memcmp (out, expected, 5) == 0);
  }

  TEST_CASE ("exact fill then exhausted")
  {
    u8                     out[2] = {0};

    struct stream          s;
    struct stream_obuf_ctx ctx;

    stream_obuf_init (&s, &ctx, out, sizeof (out));

    const u8 chunk[2] = {9, 9};
    error    e        = error_create ();

    i32      put      = stream_bwrite (chunk, 1, 2, &s, &e);
    test_assert_int_equal (put, 2);
    test_assert (!stream_isdone (&s)); // exactly filled, not yet probed again

    put = stream_bwrite (chunk, 1, 1, &s, &e);
    test_assert_int_equal (put, 0);
    test_assert (stream_isdone (&s));
  }

  TEST_CASE ("cap zero is treated as unbounded")
  {
    // Same sentinel behavior as stream_ibuf: cap == 0 means
    // "unbounded", not "no capacity".
    static u8 out[16];
    memset (out, 0, sizeof (out));

    struct stream          s;
    struct stream_obuf_ctx ctx;

    stream_obuf_init (&s, &ctx, out, 0);

    u8 chunk[16];
    for (u32 i = 0; i < 16; i++) {
      chunk[i] = (u8)(i + 1);
    }

    error e   = error_create ();
    i32   put = stream_bwrite (chunk, 1, 16, &s, &e);

    test_assert_int_equal (put, 16);
    test_assert (memcmp (out, chunk, 16) == 0);
    test_assert (!stream_isdone (&s));
  }
}

TEST (stream_read_ibuf_to_obuf)
{
  TEST_CASE ("full pipe")
  {
    // Exercises stream_read driving a real ibuf source into a real
    // obuf destination end-to-end, rather than the bread/bwrite mocks
    // used above.
    const u8               data[6] = {1, 2, 3, 4, 5, 6};

    struct stream          ibuf_s;
    struct stream_ibuf_ctx ibuf_ctx;
    stream_ibuf_init (&ibuf_s, &ibuf_ctx, data, sizeof (data));

    u8                     out[6] = {0};
    struct stream          obuf_s;
    struct stream_obuf_ctx obuf_ctx;
    stream_obuf_init (&obuf_s, &obuf_ctx, out, sizeof (out));

    error e     = error_create ();
    i32   total = stream_read (&obuf_s, 1, 6, &ibuf_s, &e);

    test_assert_int_equal (total, 6);
    test_assert (memcmp (out, data, 6) == 0);

    test_assert (!stream_isdone (&ibuf_s));
    test_assert (!stream_isdone (&obuf_s));
    total = stream_read (&obuf_s, 1, 6, &ibuf_s, &e);
    test_assert_int_equal (total, 0);

    test_assert (stream_isdone (&ibuf_s));
  }

  TEST_CASE ("dest smaller than src")
  {
    // When the real obuf destination has less capacity than the
    // source has data, stream_read should stop short and mark dest
    // finished, mirroring stream_read's "dest has insufficient
    // capacity" test but through the real ibuf/obuf implementations.
    const u8               data[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    struct stream          ibuf_s;
    struct stream_ibuf_ctx ibuf_ctx;
    stream_ibuf_init (&ibuf_s, &ibuf_ctx, data, sizeof (data));

    u8                     out[4] = {0};
    struct stream          obuf_s;
    struct stream_obuf_ctx obuf_ctx;
    stream_obuf_init (&obuf_s, &obuf_ctx, out, sizeof (out));

    error e     = error_create ();
    i32   total = stream_read (&obuf_s, 1, 10, &ibuf_s, &e);

    test_assert_int_equal (total, 4);
    test_assert (memcmp (out, data, 4) == 0);
    test_assert (stream_isdone (&obuf_s));
    test_assert (!stream_isdone (&ibuf_s));
  }
}

#endif // TESTING

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
void
stream_dyn_obuf_init (struct stream *s, struct stream_dyn_obuf_ctx *ctx, u32 limit)
{
  ctx->buffer = ext_array_create ();
  ctx->limit  = limit;
  stream_init (s, &stream_dyn_obuf_ops, ctx);
}
