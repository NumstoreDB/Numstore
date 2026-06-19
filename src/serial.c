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

#include "serial.h"

#include "csx_assert.h"
#include "testing/testing.h"

/////////////////////////////////////////////////////////////////////
////// String

DEFINE_DBG_ASSERT (struct string, string, s, {
  ASSERT (s);
  ASSERT (s->data);
  ASSERT (s->len > 0);
})

DEFINE_DBG_ASSERT (struct string, cstring, s, {
  DBG_ASSERT (string, s);
  ASSERT (s->data[s->len] == 0);
})

struct string
strfcstr (const char *cstr)
{
  return (struct string){.data = cstr, .len = strlen (cstr)};
}

u64
line_length (const char *buf, const u64 max)
{
  ASSERT (buf);
  ASSERT (max > 0);

  const char *nl = memchr (buf, '\n', max);
  u64         ret;

  if (nl != NULL)
  {
    ret = (u64)(nl - buf);
  }
  else
  {
    ret = max;
  }

  return ret;
}

int
strings_all_unique (const struct string *strs, const u32 count)
{
  for (u32 i = 0; i < count; ++i)
  {
    for (u32 j = i + 1; j < count; ++j)
    {
      if (strs[i].len != strs[j].len)
      {
        continue;
      }
      if (memcmp (strs[i].data, strs[j].data, strs[i].len) == 0)
      {
        return 0;
      }
    }
  }
  return 1;
}

#ifdef TESTING
TEST (strings_all_unique)
{
  TEST_CASE ("empty array: trivially unique")
  {
    test_assert_int_equal (strings_all_unique (NULL, 0), 1);
  }

  TEST_CASE ("one string: unique")
  {
    char                data1[] = "hello";
    const struct string s1[]    = {{5, data1}};
    test_assert_int_equal (strings_all_unique (s1, 1), 1);
  }

  TEST_CASE ("two different strings, different lengths")
  {
    char                d1[] = "a";
    char                d2[] = "ab";
    const struct string s2[] = {{1, d1}, {2, d2}};
    test_assert_int_equal (strings_all_unique (s2, 2), 1);
  }

  TEST_CASE ("two different strings, same length")
  {
    char                e1[] = "ab";
    char                e2[] = "cd";
    const struct string s3[] = {{2, e1}, {2, e2}};
    test_assert_int_equal (strings_all_unique (s3, 2), 1);
  }

  TEST_CASE ("duplicate strings")
  {
    char                f1[] = "dup";
    char                f2[] = "dup";
    const struct string s4[] = {{3, f1}, {3, f2}};
    test_assert_int_equal (strings_all_unique (s4, 2), 0);
  }

  TEST_CASE ("multiple strings with one duplicate in the middle")
  {
    char                g1[] = "one";
    char                g2[] = "two";
    char                g3[] = "one";
    char                g4[] = "four";
    const struct string s5[] = {{3, g1}, {3, g2}, {3, g3}, {4, g4}};
    test_assert_int_equal (strings_all_unique (s5, 4), 0);
  }

  TEST_CASE ("all unique in larger set")
  {
    char                h1[] = "aa";
    char                h2[] = "bb";
    char                h3[] = "cc";
    char                h4[] = "dd";
    const struct string s6[] = {{2, h1}, {2, h2}, {2, h3}, {2, h4}};
    test_assert_int_equal (strings_all_unique (s6, 4), 1);
  }
}
#endif

bool
string_equal (const struct string s1, const struct string s2)
{
  if (s1.len != s2.len)
  {
    return false;
  }
  return strncmp (s1.data, s2.data, s1.len) == 0;
}

struct string
string_plus (
    const struct string left,
    const struct string right,
    struct lalloc      *alloc,
    error              *e
)
{
  const u32 len = left.len + right.len;
  ASSERT (len > 0);

  char *data = lmalloc (alloc, len, 1, e);

  if (data == NULL)
  {
    return (struct string){
        .data = NULL,
        .len  = 0,
    };
  }

  memcpy (data, left.data, left.len);
  memcpy (data + left.len, right.data, right.len);

  return (struct string){
      .data = data,
      .len  = len,
  };
}

const struct string *
strings_are_disjoint (
    const struct string *left,
    const u32            llen,
    const struct string *right,
    const u32            rlen
)
{
  for (u32 i = 0; i < llen; ++i)
  {
    for (u32 j = 0; j < rlen; ++j)
    {
      if (string_equal (left[i], right[j]))
      {
        return &left[i];
      }
    }
  }

  return NULL;
}

bool
string_contains (const struct string superset, const struct string subset)
{
  if (superset.len == 0 && subset.len == 0)
  {
    return true;
  }

  for (u32 i = 0; i < superset.len; ++i)
  {
    const u32 len = superset.len - i;
    if (len < subset.len)
    {
      return false;
    }

    const struct string _superset = {
        .data = &superset.data[i],
        .len  = subset.len,
    };
    if (string_equal (_superset, subset))
    {
      return true;
    }
  }

  return false;
}

#ifdef TESTING
TEST (string_contains)
{
  test_assert (!string_contains (strfcstr ("foo"), strfcstr ("foobar")));
  test_assert (string_contains (strfcstr ("foobar"), strfcstr ("foo")));
  test_assert (!string_contains (strfcstr ("fobar"), strfcstr ("foo")));
  test_assert (string_contains (strfcstr ("barfoo"), strfcstr ("foo")));
  test_assert (!string_contains (strfcstr ("barfo"), strfcstr ("foo")));
  test_assert (string_contains (strfcstr ("foo"), strfcstr ("")));
  test_assert (string_contains (strfcstr (""), strfcstr ("")));
}
#endif

bool
string_less_string (const struct string left, const struct string right)
{
  const u32 min_len = (left.len < right.len) ? left.len : right.len;
  const int cmp     = memcmp (left.data, right.data, min_len);
  if (cmp < 0)
  {
    return true;
  }
  if (cmp > 0)
  {
    return false;
  }
  return left.len < right.len;
}

bool
string_greater_string (const struct string left, const struct string right)
{
  const u32 min_len = (left.len < right.len) ? left.len : right.len;
  const int cmp     = memcmp (left.data, right.data, min_len);
  if (cmp > 0)
  {
    return true;
  }
  if (cmp < 0)
  {
    return false;
  }
  return left.len > right.len;
}

bool
string_less_equal_string (const struct string left, const struct string right)
{
  return !string_greater_string (left, right);
}

bool
string_greater_equal_string (
    const struct string left,
    const struct string right
)
{
  return !string_less_string (left, right);
}

err_t
string_copy (struct string *dest, struct string src, error *e)
{
  char *data = i_calloc (src.len + 1, 1, e);
  if (data == NULL)
  {
    return error_trace (e);
  }
  memcpy (data, src.data, src.len);
  dest->data = data;
  dest->len  = src.len;
  return SUCCESS;
}

DEFINE_DBG_ASSERT (struct serializer, serializer, s, {
  ASSERT (s);
  ASSERT (s->data);
  ASSERT (s->dcap > 0);
  ASSERT (s->dlen <= s->dcap);
})

/////////////////////////////////////////////////////////////////////
////// Serializer

struct serializer
srlizr_create (u8 *data, const u32 dcap)
{
  struct serializer ret = (struct serializer){
      .data = data,
      .dlen = 0,
      .dcap = dcap,
  };
  latch_init (&ret.latch);
  return ret;
}

bool
srlizr_write (struct serializer *dest, const void *src, const u32 len)
{
  latch_lock (&dest->latch);

  DBG_ASSERT (serializer, dest);

  if (dest->dlen + len > dest->dcap)
  {
    latch_unlock (&dest->latch);
    return false;
  }
  memcpy (dest->data + dest->dlen, src, len);
  dest->dlen += len;

  DBG_ASSERT (serializer, dest);

  latch_unlock (&dest->latch);

  return true;
}

/////////////////////////////////////////////////////////////////////
////// Deserializer

DEFINE_DBG_ASSERT (struct deserializer, deserializer, s, {
  ASSERT (s);
  ASSERT (s->data);
  ASSERT (s->dlen > 0);
  ASSERT (s->head <= s->dlen);
})

struct deserializer
dsrlizr_create (const u8 *data, const u32 dlen)
{
  struct deserializer ret = (struct deserializer){
      .data = data,
      .head = 0,
      .dlen = dlen,
  };
  latch_init (&ret.latch);
  return ret;
}

bool
dsrlizr_read (void *dest, const u32 dlen, struct deserializer *src)
{
  latch_lock (&src->latch);

  DBG_ASSERT (deserializer, src);

  if (src->head + dlen > src->dlen)
  {
    latch_unlock (&src->latch);
    return false;
  }
  memcpy (dest, src->data + src->head, dlen);
  src->head += dlen;

  DBG_ASSERT (deserializer, src);

  latch_unlock (&src->latch);

  return true;
}

/////////////////////////////////////////////////////////////////////
////// Stream

void
stream_init (struct stream *s, const struct stream_ops *ops, void *ctx)
{
  s->ops  = ops;
  s->ctx  = ctx;
  s->done = false;
}

void
stream_finish (struct stream *s)
{
  atomic_store_explicit (&s->done, true, memory_order_release);
}

bool
stream_isdone (const struct stream *s)
{
  return atomic_load_explicit (&s->done, memory_order_acquire);
}

void
stream_close (const struct stream *s)
{
  if (s->ops->close)
  {
    s->ops->close (s->ctx);
  }
}

i32 stream_read (
    struct stream *dest,
    u32            size,
    u32            n,
    struct stream *src,
    error         *e
);

i32
stream_bread (
    void          *dest,
    const u32      size,
    const u32      n,
    struct stream *src,
    error         *e
)
{
  return src->ops->pull (src, src->ctx, dest, size, n, e);
}

i32
stream_bwrite (
    const void    *buf,
    const u32      size,
    const u32      n,
    struct stream *dest,
    error         *e
)
{
  return dest->ops->push (dest, dest->ctx, buf, size, n, e);
}

i32
stream_read (
    struct stream *dest,
    const u32      size,
    const u32      n,
    struct stream *src,
    error         *e
)
{
  ASSERT (dest->ops->push);
  ASSERT (src->ops->pull);

  u8  buf[4096];
  u32 total     = 0;
  u32 remaining = n;
  u32 batch_max = sizeof (buf) / size;

  if (batch_max == 0)
  {
    batch_max = 1;
  }

  while (remaining > 0)
  {
    const u32 batch = remaining < batch_max ? remaining : batch_max;

    const i32 got = src->ops->pull (src, src->ctx, buf, size, batch, e);
    if (got < 0)
    {
      return got;
    }
    if (got == 0)
    {
      break;
    }

    u32 pushed = 0;
    while (pushed < (u32)got)
    {
      const i32 w = dest->ops->push (
          dest,
          dest->ctx,
          buf + (pushed * size),
          size,
          got - pushed,
          e
      );
      if (w < 0)
      {
        return w;
      }
      if (w == 0)
      {
        break;
      }
      pushed += w;
    }

    total += pushed;
    remaining -= pushed;

    if (pushed < (u32)got)
    {
      break;
    }
  }

  return total;
}

static i32
stream_ibuf_pull (
    struct stream *s,
    void          *vctx,
    void          *dest,
    const u32      size,
    const u32      n,
    error         *e
)
{
  struct stream_ibuf_ctx *ctx = (struct stream_ibuf_ctx *)vctx;

  const u32 avail = ctx->size - ctx->pos;
  const u32 want  = size * n;

  const u32 next = MIN (avail, want);

  if (next == 0)
  {
    if (avail == 0)
    {
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
  struct stream_obuf_ctx *ctx = (struct stream_obuf_ctx *)vctx;

  const u32 avail = ctx->cap - ctx->pos;
  const u32 want  = size * n;

  const u32 next = MIN (avail, want);

  if (next == 0)
  {
    if (avail == 0)
    {
      stream_finish (s);
    }
    return 0;
  }

  memcpy (ctx->buf + ctx->pos, src, next);
  ctx->pos += next;

  return next / size;
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

void
stream_ibuf_init (
    struct stream          *s,
    struct stream_ibuf_ctx *ctx,
    const void             *buf,
    const u32               size
)
{
  ctx->buf  = (const u8 *)buf;
  ctx->size = size;
  ctx->pos  = 0;
  stream_init (s, &stream_ibuf_ops, ctx);
}

void
stream_obuf_init (
    struct stream          *s,
    struct stream_obuf_ctx *ctx,
    void                   *buf,
    const u32               cap
)
{
  ctx->buf = buf;
  ctx->cap = cap;
  ctx->pos = 0;
  stream_init (s, &stream_obuf_ops, ctx);
}

static i32
stream_sink_push (
    struct stream *s,
    void          *vctx,
    const void    *src,
    u32            size,
    const u32      n,
    error         *e
)
{
  return n;
}

static const struct stream_ops stream_sink_ops = {
    .pull  = NULL,
    .push  = stream_sink_push,
    .close = NULL,
};

void
stream_sink_init (struct stream *s)
{
  stream_init (s, &stream_sink_ops, NULL);
}

static i32
stream_opsink_push (
    struct stream *s,
    void          *vctx,
    const void    *src,
    const u32      size,
    const u32      n,
    error         *e
)
{
  struct stream_opsink_ctx *ctx = (struct stream_opsink_ctx *)vctx;

  const u32 avail = ctx->size - ctx->pos;
  const u32 want  = size * n;

  const u32 next = MIN (avail, want);

  if (next == 0)
  {
    return 0;
  }

  memcpy ((u8 *)ctx->buf + ctx->pos, src, next);
  ctx->pos += next;

  if (ctx->pos == ctx->size)
  {
    ctx->op (ctx->buf);
    ctx->pos = 0;
  }

  return next / size;
}

static const struct stream_ops stream_opsink_ops = {
    .pull  = NULL,
    .push  = stream_opsink_push,
    .close = NULL,
};

void
stream_opsink_init (
    struct stream            *s,
    struct stream_opsink_ctx *ctx,
    const byte_op             op,
    void                     *buf,
    const u32                 size
)
{
  ctx->buf = buf, ctx->op = op, ctx->size = size;
  ctx->pos = 0;
  stream_init (s, &stream_opsink_ops, ctx);
}
static i32
stream_limit_pull (
    struct stream *s,
    void          *vctx,
    void          *buf,
    const u32      size,
    const u32      n,
    error         *e
)
{
  struct stream_limit_ctx *ctx = (struct stream_limit_ctx *)vctx;

  const u64 remaining = ctx->limit - ctx->consumed;
  if (remaining == 0)
  {
    stream_finish (s);
    return 0;
  }

  const u32 capped_n = (u32)(MIN ((u64)(size * n), remaining) / size);
  if (capped_n == 0)
  {
    stream_finish (s);
    return 0;
  }

  const i32 got = stream_bread (buf, size, capped_n, ctx->underlying, e);
  if (got < 0)
  {
    return got;
  }

  ctx->consumed += (u64)got * size;

  if (ctx->consumed >= ctx->limit || stream_isdone (ctx->underlying))
  {
    stream_finish (s);
  }

  return got;
}

static i32
stream_limit_push (
    struct stream *s,
    void          *vctx,
    const void    *buf,
    const u32      size,
    const u32      n,
    error         *e
)
{
  struct stream_limit_ctx *ctx = (struct stream_limit_ctx *)vctx;

  const u64 remaining = ctx->limit - ctx->consumed;
  if (remaining == 0)
  {
    stream_finish (s);
    return 0;
  }

  const u32 capped_n = (u32)(MIN ((u64)(size * n), remaining) / size);
  if (capped_n == 0)
  {
    stream_finish (s);
    return 0;
  }

  const i32 put = stream_bwrite (buf, size, capped_n, ctx->underlying, e);
  if (put < 0)
  {
    return put;
  }

  ctx->consumed += (u64)put * size;

  if (ctx->consumed >= ctx->limit || stream_isdone (ctx->underlying))
  {
    stream_finish (s);
  }

  return put;
}

static const struct stream_ops stream_limit_ops = {
    .pull  = stream_limit_pull,
    .push  = stream_limit_push,
    .close = NULL,
};

void
stream_limit_init (
    struct stream           *s,
    struct stream_limit_ctx *ctx,
    struct stream           *underlying,
    const u64                limit
)
{
  ctx->underlying = underlying;
  ctx->limit      = limit;
  ctx->consumed   = 0;
  stream_init (s, &stream_limit_ops, ctx);
}
