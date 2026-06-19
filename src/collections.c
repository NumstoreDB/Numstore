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

#include "collections.h"

#include "csx_assert.h"
#include "testing/data_validator.h"
#include "testing/testing.h"

struct int_node
{
  int           value;
  struct llnode node;
};

#ifdef TESTING
TEST (llist)
{
  struct int_node nodes[10];
  nodes[0].value = 0;
  llnode_init (&nodes[0].node);

  struct llnode *head = &nodes[0].node;

  for (u32 i = 1; i < 10; ++i)
  {
    nodes[i].value = i;
    list_push (&head, &nodes[i].node);
  }

  for (u32 i = 10; i > 0; --i)
  {
    struct llnode         *ret  = list_pop (&head);
    const struct int_node *node = container_of (ret, struct int_node, node);
    test_assert_int_equal (node->value, i - 1);
  }

  const struct llnode *ret = list_pop (&head);
  test_assert_equal (ret, NULL);
  test_assert_equal (head, NULL);

  head = &nodes[0].node;
}
#endif

#include <string.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

struct cbuffer
cbuffer_create (void *data, const u32 cap)
{
  ASSERT (data);
  ASSERT (cap > 0);
  const struct cbuffer ret = (struct cbuffer){
      .head   = 0,
      .tail   = 0,
      .cap    = cap,
      .data   = data,
      .isfull = 0,
  };
  return ret;
}

struct cbuffer
cbuffer_create_with (void *data, const u32 cap, const u32 len)
{
  ASSERT (data);
  ASSERT (cap > 0);
  ASSERT (len <= cap);
  const struct cbuffer ret = (struct cbuffer){
      .head   = len % cap,
      .tail   = 0,
      .cap    = cap,
      .data   = data,
      .isfull = len == cap,
  };
  return ret;
}

////////////////////////////////////////////////////////////
// UTILS

#ifdef TESTING
TEST (cbuffer_isempty)
{
  u8             buf[1];
  struct cbuffer b = cbuffer_create (buf, 1);
  test_assert_int_equal (cbuffer_isempty (&b), 1);
  const u8 next = 0xFF;
  cbuffer_push_back (&next, 1, &b);
  test_assert_int_equal (cbuffer_isempty (&b), 0);
}

TEST (cbuffer_len)
{
  u8             buf[4];
  struct cbuffer b = cbuffer_create (buf, 4);
  test_assert_int_equal (cbuffer_len (&b), 0);
  cbuffer_pushb_back_expect (1, &b);
  cbuffer_pushb_back_expect (2, &b);
  test_assert_int_equal (cbuffer_len (&b), 2);
}

TEST (cbuffer_avail)
{
  u8             buf[4];
  struct cbuffer b = cbuffer_create (buf, 4);
  test_assert_int_equal (cbuffer_avail (&b), 4);
  cbuffer_pushb_back_expect (9, &b);
  test_assert_int_equal (cbuffer_avail (&b), 3);
}
#endif

void
cbuffer_discard_all (struct cbuffer *b)
{
  DBG_ASSERT (cbuffer, b);

  b->tail   = 0;
  b->head   = 0;
  b->isfull = 0;
}

struct bytes
cbuffer_get_next_data_bytes (const struct cbuffer *b)
{
  DBG_ASSERT (cbuffer, b);

  if (b->head == b->tail && !b->isfull)
  {
    return (struct bytes){
        .head = NULL,
        .len  = 0,
    };
  }

  if (b->head > b->tail)
  {
    return (struct bytes){
        .head = &b->data[b->tail],
        .len  = b->head - b->tail,
    };
  }
  else
  {
    return (struct bytes){
        .head = &b->data[b->tail],
        .len  = b->cap - b->tail,
    };
  }
}

#ifdef TESTING
TEST (cbuffer_get_next_data_bytes)
{
  u8             raw[16];
  struct cbuffer b = {
      .data   = raw,
      .cap    = sizeof (raw),
      .head   = 0,
      .tail   = 0,
      .isfull = false,
  };

  {
    const struct bytes out = cbuffer_get_next_data_bytes (&b);
    test_assert_ptr_equal (out.head, NULL);
    test_assert_int_equal (out.len, 0);
  }

  TEST_CASE ("Right half")
  {
    b.head   = 2;
    b.tail   = 6;
    b.isfull = false;

    const struct bytes out = cbuffer_get_next_data_bytes (&b);
    test_assert_ptr_equal (out.head, &raw[6]);
    test_assert_int_equal (out.len, 16 - 6);
  }

  TEST_CASE ("Middle")
  {
    b.head   = 12;
    b.tail   = 4;
    b.isfull = false;

    const struct bytes out = cbuffer_get_next_data_bytes (&b);
    test_assert_ptr_equal (out.head, &raw[4]);
    test_assert_int_equal (out.len, 12 - 4);
  }
}
#endif

struct bytes
cbuffer_get_next_avail_bytes (const struct cbuffer *b)
{
  DBG_ASSERT (cbuffer, b);

  if (b->isfull)
  {
    return (struct bytes){
        .head = NULL,
        .len  = 0,
    };
  }

  // [ _______ tail / head __________ ] (e.g. empty)
  // [ _______ tail Data Data head __ ]
  if (b->head >= b->tail)
  {
    return (struct bytes){
        .head = &b->data[b->head],
        .len  = b->cap - b->head,
    };
  }
  else
  {
    return (struct bytes){
        .head = &b->data[b->head],
        .len  = b->tail - b->head,
    };
  }
}

#ifdef TESTING
TEST (cbuffer_get_nbytes)
{
  u8             raw[16];
  struct cbuffer b = {
      .data   = raw,
      .cap    = sizeof (raw),
      .head   = 0,
      .tail   = 0,
      .isfull = false,
  };

  TEST_CASE ("Empty buffer")
  {
    const struct bytes out = cbuffer_get_next_avail_bytes (&b);
    test_assert_ptr_equal (out.head, &raw[0]);
    test_assert_int_equal (out.len, 16);
  }

  TEST_CASE ("Head < Tail, normal case")
  {
    b.head   = 2;
    b.tail   = 6;
    b.isfull = false;

    const struct bytes out = cbuffer_get_next_avail_bytes (&b);
    test_assert_ptr_equal (out.head, &raw[2]);
    test_assert_int_equal (out.len, 4); // 6 - 2
  }

  TEST_CASE ("Head > Tail, wraparound case")
  {
    b.head   = 12;
    b.tail   = 4;
    b.isfull = false;

    const struct bytes out = cbuffer_get_next_avail_bytes (&b);
    test_assert_ptr_equal (out.head, &raw[12]);
    test_assert_int_equal (out.len, 4); // cap - head = 16 - 12
  }

  TEST_CASE ("Full buffer")
  {
    b.head   = 5;
    b.tail   = 5;
    b.isfull = true;

    const struct bytes out = cbuffer_get_next_avail_bytes (&b);
    test_assert_ptr_equal (out.head, NULL);
    test_assert_int_equal (out.len, 0);
  }
}
#endif

void
cbuffer_fakewrite (struct cbuffer *b, const u32 nbytes)
{
  DBG_ASSERT (cbuffer, b);
  ASSERT (nbytes <= cbuffer_avail (b));

  b->head = (b->head + nbytes) % b->cap;
  if (nbytes > 0 && b->head == b->tail)
  {
    b->isfull = true;
  }
}

#ifdef TESTING
TEST (cbuffer_fakewrite)
{
  u8             raw[8];
  struct cbuffer b = cbuffer_create_from (raw);

  TEST_CASE ("empty buffer")
  {
    cbuffer_fakewrite (&b, 0);
    test_assert_int_equal (b.head, 0);
    test_assert_int_equal (b.tail, 0);
    test_assert (!b.isfull);
  }

  TEST_CASE ("normal")
  {
    b.head   = 0;
    b.tail   = 5;
    b.isfull = false;

    cbuffer_fakewrite (&b, 3);
    test_assert_int_equal (b.head, 3);
    test_assert_int_equal (b.tail, 5);
    test_assert (!b.isfull);
  }

  TEST_CASE ("wraparound")
  {
    b.head   = 6;
    b.tail   = 2;
    b.isfull = false;

    cbuffer_fakewrite (&b, 2);
    test_assert_int_equal (b.head, 0);
    test_assert_int_equal (b.tail, 2);
    test_assert (!b.isfull);
  }

  TEST_CASE ("reach tail")
  {
    b.head   = 2;
    b.tail   = 5;
    b.isfull = false;

    cbuffer_fakewrite (&b, 3);
    test_assert_int_equal (b.head, 5);
    test_assert (b.isfull);
  }

  TEST_CASE ("reach tail via wrap")
  {
    b.head   = 6;
    b.tail   = 2;
    b.isfull = false;

    cbuffer_fakewrite (&b, 4); // 6 + 4 == 10 % 8 == 2
    test_assert_int_equal (b.head, 2);
    test_assert (b.isfull);
  }

  TEST_CASE ("write 0 on full buffer does not change state")
  {
    b.head   = 3;
    b.tail   = 3;
    b.isfull = true;

    cbuffer_fakewrite (&b, 0);
    test_assert_int_equal (b.head, 3);
    test_assert_int_equal (b.tail, 3);
    test_assert (b.isfull);
  }
}
#endif

void
cbuffer_fakeread (struct cbuffer *b, const u32 nbytes)
{
  DBG_ASSERT (cbuffer, b);
  ASSERT (nbytes <= cbuffer_len (b));

  b->tail = (b->tail + nbytes) % b->cap;
  if (nbytes > 0)
  {
    b->isfull = false;
  }
}

#ifdef TESTING
TEST (cbuffer_fakeread)
{
  u8             raw[8];
  struct cbuffer b = cbuffer_create_from (raw);

  TEST_CASE ("empty buffer")
  {
    cbuffer_fakeread (&b, 0);
    test_assert_int_equal (b.head, 0);
    test_assert_int_equal (b.tail, 0);
    test_assert (!b.isfull);
  }

  TEST_CASE ("normal")
  {
    b.head   = 0;
    b.tail   = 5;
    b.isfull = false;

    cbuffer_fakeread (&b, 3);
    test_assert_int_equal (b.head, 0);
    test_assert_int_equal (b.tail, 0);
    test_assert (!b.isfull);
  }

  TEST_CASE ("wraparound")
  {
    b.head   = 6;
    b.tail   = 2;
    b.isfull = false;

    cbuffer_fakeread (&b, 2);
    test_assert_int_equal (b.head, 6);
    test_assert_int_equal (b.tail, 4);
    test_assert (!b.isfull);
  }

  {
    b.head   = 2;
    b.tail   = 5;
    b.isfull = false;

    cbuffer_fakeread (&b, 3);
    test_assert_int_equal (b.head, 2);
    test_assert_int_equal (b.tail, 0);
    test_assert (!b.isfull);
  }

  {
    b.head   = 6;
    b.tail   = 2;
    b.isfull = false;

    cbuffer_fakeread (&b, 4);
    test_assert_int_equal (b.head, 6);
    test_assert_int_equal (b.tail, 6);
    test_assert (!b.isfull);
  }

  {
    b.head   = 3;
    b.tail   = 3;
    b.isfull = true;

    cbuffer_fakeread (&b, 0);
    test_assert_int_equal (b.head, 3);
    test_assert_int_equal (b.tail, 3);
    test_assert (b.isfull);
  }
}
#endif

////////////////////////////////////////////////////////////
// RAW READ / WRITE

u32
cbuffer_read (void *dest, const u32 size, const u32 n, struct cbuffer *b)
{
  DBG_ASSERT (cbuffer, b);
  ASSERT (size > 0);
  ASSERT (n > 0);

  const u32 len     = cbuffer_len (b) / size;
  const u32 ntoread = (n < len) ? n : len;
  const u32 btoread = ntoread * size;
  u32       bread   = 0;

  u8 *output = NULL;
  if (dest)
  {
    output = (u8 *)dest;
  }

  while (bread < btoread)
  {
    u32 next;

    if (!b->isfull && b->head > b->tail)
    {
      next = MIN (b->head - b->tail, btoread - bread);
    }
    else
    {
      next = MIN (b->cap - b->tail, btoread - bread);
    }

    if (output)
    {
      memcpy (output + bread, b->data + b->tail, next);
    }
    b->tail = (b->tail + next) % b->cap;
    bread += next;
    b->isfull = 0;
  }

  ASSERT (ntoread * size == bread);

  return ntoread;
}

#ifdef TESTING
TEST (cbuffer_read)
{
  u8             buf[3];
  struct cbuffer b = cbuffer_create (buf, 3);
  u8             out[3];

  // read from empty: returns 0
  const u32 r1 = cbuffer_read (out, 1, 1, &b);
  test_assert_int_equal (r1, 0);

  // read after write
  const u8 src[3] = {7, 8, 9};
  cbuffer_write (src, 1, 3, &b);
  const u32 r2 = cbuffer_read (out, 1, 2, &b);
  test_assert_int_equal (r2, 2);
  test_assert_int_equal (out[0], 7);
  test_assert_int_equal (out[1], 8);
  test_assert_int_equal (cbuffer_len (&b), 1);
}
#endif

u32
cbuffer_copy (void *dest, const u32 size, const u32 n, const struct cbuffer *b)
{
  DBG_ASSERT (cbuffer, b);
  ASSERT (size > 0);
  ASSERT (n > 0);

  const u32 len     = cbuffer_len (b) / size;
  const u32 ntoread = (n < len) ? n : len;
  const u32 btoread = ntoread * size;
  u32       bread   = 0;

  u8 *output = NULL;
  if (dest)
  {
    output = (u8 *)dest;
  }

  // Literally the exact same as read but
  // use these temp variables instead
  // I was lazy
  u32  tail   = b->tail;
  bool isfull = b->isfull;

  while (bread < btoread)
  {
    u32 next;

    if (!isfull && b->head > tail)
    {
      next = MIN (b->head - tail, btoread - bread);
    }
    else
    {
      next = MIN (b->cap - tail, btoread - bread);
    }

    if (output)
    {
      memcpy (output + bread, b->data + tail, next);
    }
    tail = (tail + next) % b->cap;
    bread += next;
    isfull = 0;
  }

  ASSERT (ntoread * size == bread);
  return ntoread;
}

#ifdef TESTING
TEST (cbuffer_copy)
{
  u8             buf[3];
  struct cbuffer b = cbuffer_create (buf, 3);
  u8             out[3];
  u32            r2;

  TEST_CASE ("read from empty: returns 0")
  {
    const u32 r1 = cbuffer_copy (out, 1, 1, &b);
    test_assert_int_equal (r1, 0);
  }

  TEST_CASE ("read after write")
  {
    const u8 src[3] = {7, 8, 9};
    cbuffer_write (src, 1, 3, &b);
    r2 = cbuffer_copy (out, 1, 2, &b);
    test_assert_int_equal (r2, 2);
    test_assert_int_equal (out[0], 7);
    test_assert_int_equal (out[1], 8);
    test_assert_int_equal (cbuffer_len (&b), 3);
  }

  TEST_CASE ("Do it again and get the same results")
  {
    r2 = cbuffer_copy (out, 1, 2, &b);
    test_assert_int_equal (r2, 2);
    test_assert_int_equal (out[0], 7);
    test_assert_int_equal (out[1], 8);
    test_assert_int_equal (cbuffer_len (&b), 3);
  }
}
#endif

u32
cbuffer_write (const void *src, const u32 size, const u32 n, struct cbuffer *b)
{
  DBG_ASSERT (cbuffer, b);
  ASSERT (src);
  ASSERT (size > 0);
  ASSERT (n > 0);

  ASSERT (b->cap >= cbuffer_len (b));
  const u32 avail    = (b->cap - cbuffer_len (b)) / size;
  const u32 ntowrite = (n < avail) ? n : avail;
  const u32 btowrite = ntowrite * size;
  u32       bwrite   = 0;

  const u8 *input = (u8 *)src;

  while (bwrite < btowrite)
  {
    u32 next;

    ASSERT (btowrite > bwrite);
    if (b->head >= b->tail)
    {
      ASSERT (b->cap > b->head);
      next = MIN (b->cap - b->head, btowrite - bwrite);

      if (next == 0)
      {
        b->head = 0;
        next    = b->tail;
      }
    }
    else
    {
      ASSERT (b->tail > b->head);
      next = MIN (b->tail - b->head, btowrite - bwrite);
    }

    memcpy (b->data + b->head, input + bwrite, next);
    b->head = (b->head + next) % b->cap;
    bwrite += next;

    if (b->head == b->tail)
    {
      b->isfull = 1;
    }
  }

  ASSERT (ntowrite * size == bwrite);

  return ntowrite;
}

#ifdef TESTING
TEST (cbuffer_write)
{
  u8             buf[3];
  struct cbuffer b      = cbuffer_create (buf, 3);
  const u8       src[4] = {1, 2, 3, 4};

  TEST_CASE ("size > capacity: writes nothing")
  {
    const u32 w1 = cbuffer_write (src, 4, 1, &b);
    test_assert_int_equal (w1, 0);
  }

  TEST_CASE ("element size 1, too many elements: only 3 fit")
  {
    const u32 w2 = cbuffer_write (src, 1, 4, &b);
    test_assert_int_equal (w2, 3);
    test_assert_int_equal (cbuffer_len (&b), 3);
  }
}
#endif

////////////////////////////////////////////////////////////
// CBUFFER READ / WRITE

u32
cbuffer_cbuffer_move (
    struct cbuffer *dest,
    const u32       sz,
    const u32       cnt,
    struct cbuffer *src
)
{
  DBG_ASSERT (cbuffer, dest);
  DBG_ASSERT (cbuffer, src);
  ASSERT (sz > 0 && cnt > 0);

  // calculate number of elements to move
  const u32 src_elems = cbuffer_len (src) / sz;
  const u32 dst_space = cbuffer_avail (dest) / sz;
  u32       n;
  if (cnt < src_elems)
  {
    n = cnt;
  }
  else
  {
    n = src_elems;
  }
  if (dst_space < n)
  {
    n = dst_space;
  }
  if (n == 0)
  {
    return 0;
  }

  // total nbytes to transfer
  u32 nbytes = n * sz;

  // first chunk: from tail up to end of buffer
  u32 first;
  if (src->isfull || src->head <= src->tail)
  {
    first = src->cap - src->tail;
  }
  else
  {
    first = src->head - src->tail;
  }
  if (first > nbytes)
  {
    first = nbytes;
  }

  // write first chunk into dest
  cbuffer_write (src->data + src->tail, 1, first, dest);

  // advance tail and clear full flag
  src->tail   = (src->tail + first) % src->cap;
  src->isfull = false;
  nbytes -= first;

  // second chunk if any remains
  if (nbytes > 0)
  {
    cbuffer_write (src->data + src->tail, 1, nbytes, dest);
    src->tail = (src->tail + nbytes) % src->cap;
  }

  return n;
}

#ifdef TESTING
TEST (cbuffer_cbuffer_move)
{
  u8             buf_s[4];
  u8             buf_d[4];
  struct cbuffer src = cbuffer_create (buf_s, 4);
  struct cbuffer dst = cbuffer_create (buf_d, 4);
  u8             out[4];

  TEST_CASE ("empty source: should read 0 elements")
  {
    const u32 r0 = cbuffer_cbuffer_move (&dst, 1, 1, &src);
    test_assert_int_equal (r0, 0);
  }

  TEST_CASE ("fill source with 3 nbytes")
  {
    const u8 data2[3] = {4, 5, 6};
    cbuffer_write (data2, 1, 3, &src);
  }

  TEST_CASE ("read 2 elements into dst")
  {
    const u32 r1 = cbuffer_cbuffer_move (&dst, 1, 2, &src);
    test_assert_int_equal (r1, 2);
    test_assert_int_equal (cbuffer_len (&src), 1);
    test_assert_int_equal (cbuffer_len (&dst), 2);
  }

  TEST_CASE ("verify dst contents")
  {
    const u32 r2 = cbuffer_read (out, 1, 2, &dst);
    test_assert_int_equal (r2, 2);
    test_assert_int_equal (out[0], 4);
    test_assert_int_equal (out[1], 5);
  }
}
#endif

u32
cbuffer_cbuffer_copy (
    struct cbuffer       *dest,
    const u32             sz,
    const u32             cnt,
    const struct cbuffer *src
)
{
  DBG_ASSERT (cbuffer, dest);
  DBG_ASSERT (cbuffer, src);
  ASSERT (sz > 0 && cnt > 0);

  // calculate number of elements to copy
  const u32 src_elems = cbuffer_len (src) / sz;
  const u32 dst_space = cbuffer_avail (dest) / sz;
  u32       n;
  if (cnt < src_elems)
  {
    n = cnt;
  }
  else
  {
    n = src_elems;
  }
  if (dst_space < n)
  {
    n = dst_space;
  }
  if (n == 0)
  {
    return 0;
  }

  // total nbytes to copy
  u32 nbytes = n * sz;

  // local cursor and copy of full flag
  u32  pos  = src->tail;
  bool full = src->isfull;

  // first chunk: from pos up to end of buffer
  u32 first;
  if (full || src->head <= pos)
  {
    first = src->cap - pos;
  }
  else
  {
    first = src->head - pos;
  }
  if (first > nbytes)
  {
    first = nbytes;
  }

  // write first chunk into dest
  cbuffer_write (src->data + pos, 1, first, dest);

  // advance local cursor
  pos = (pos + first) % src->cap;
  nbytes -= first;

  // second chunk if any remains
  if (nbytes > 0)
  {
    cbuffer_write (src->data + pos, 1, nbytes, dest);
  }

  return n;
}
#ifdef TESTING
TEST (cbuffer_cbuffer_copy)
{
  u8             buf_s[4];
  u8             buf_d[4];
  struct cbuffer src = cbuffer_create (buf_s, 4);
  struct cbuffer dst = cbuffer_create (buf_d, 4);
  u8             out[4];

  TEST_CASE ("empty source: should copy 0 elements")
  {
    const u32 r0 = cbuffer_cbuffer_copy (&dst, 1, 1, &src);
    test_assert_int_equal (r0, 0);
  }

  TEST_CASE ("fill source with 3 nbytes")
  {
    const u8 data1[3] = {1, 2, 3};
    cbuffer_write (data1, 1, 3, &src);
  }

  TEST_CASE ("copy 2 elements into dst")
  {
    const u32 r1 = cbuffer_cbuffer_copy (&dst, 1, 2, &src);
    test_assert_int_equal (r1, 2);
    test_assert_int_equal (cbuffer_len (&src), 3);
    test_assert_int_equal (cbuffer_len (&dst), 2);
  }

  TEST_CASE ("verify dst contents")
  {
    const u32 r2 = cbuffer_read (out, 1, 2, &dst);
    test_assert_int_equal (r2, 2);
    test_assert_int_equal (out[0], 1);
    test_assert_int_equal (out[1], 2);
  }
}
#endif

////////////////////////////////////////////////////////////
// IO READ / WRITE

i32
cbuffer_read_from_file_1 (
    i_file               *src,
    const struct cbuffer *b,
    const u32             len,
    error                *e
)
{
  ASSERT (src);
  ASSERT (b);

  const u32    btowrite = MIN (cbuffer_avail (b), len);
  u32          bwrite   = 0;
  struct bytes iov[2];
  int          iovcnt  = 0;
  u32          newhead = b->head;

  while (bwrite < btowrite)
  {
    u32 next;
    if (newhead >= b->tail)
    {
      ASSERT (b->cap > newhead);
      next = MIN (b->cap - newhead, btowrite - bwrite);
    }
    else
    {
      ASSERT (b->tail > newhead);
      next = MIN (b->tail - newhead, btowrite - bwrite);
    }

    ASSERT (iovcnt < 2);
    iov[iovcnt].head = b->data + newhead;
    iov[iovcnt].len  = next;
    iovcnt++;

    newhead = (newhead + next) % b->cap;
    bwrite += next;
  }

  if (btowrite == 0)
  {
    return 0;
  }

  const i64 nread = i_readv_all (src, iov, iovcnt, e);
  if (nread < 0)
  {
    return error_trace (e);
  }

  return nread;
}

err_t
cbuffer_read_from_file_1_expect (
    i_file               *src,
    const struct cbuffer *b,
    const u32             len,
    error                *e
)
{
  const i32 read = cbuffer_read_from_file_1 (src, b, len, e);
  if (read < 0)
  {
    return read;
  }
  ASSERT ((u32)read == len);
  return SUCCESS;
}

void
cbuffer_read_from_file_2 (struct cbuffer *b, const u32 nread)
{
  DBG_ASSERT (cbuffer, b);
  ASSERT (nread <= cbuffer_avail (b));

  b->head = (b->head + nread) % b->cap;

  if (b->head == b->tail && nread > 0)
  {
    b->isfull = 1;
  }
}

i32
cbuffer_read_from_file (i_file *src, struct cbuffer *b, const u32 len, error *e)
{
  DBG_ASSERT (cbuffer, b);
  ASSERT (src);

  const i32 nread = cbuffer_read_from_file_1 (src, b, len, e);
  if (nread < 0)
  {
    return nread;
  }

  cbuffer_read_from_file_2 (b, (u32)nread);

  return nread;
}

i32
cbuffer_write_to_file_1 (
    i_file               *dest,
    const struct cbuffer *b,
    const u32             len,
    error                *e
)
{
  ASSERT (dest);
  ASSERT (b);

  const u32    btoread = MIN (cbuffer_len (b), len);
  u32          bread   = 0;
  struct bytes iov[2];
  int          iovcnt  = 0;
  u32          newtail = b->tail;

  while (bread < btoread)
  {
    u32 next;
    if (!b->isfull && b->head > newtail)
    {
      next = MIN (b->head - newtail, btoread - bread);
    }
    else
    {
      next = MIN (b->cap - newtail, btoread - bread);
    }

    ASSERT (iovcnt < 2);
    iov[iovcnt].head = b->data + newtail;
    iov[iovcnt].len  = next;
    iovcnt++;

    newtail = (newtail + next) % b->cap;
    bread += next;
  }

  if (btoread == 0)
  {
    return 0;
  }

  const err_t err = i_writev_all (dest, iov, iovcnt, e);
  if (err != SUCCESS)
  {
    return error_trace (e);
  }

  return btoread;
}

err_t
cbuffer_write_to_file_1_expect (
    i_file               *dest,
    const struct cbuffer *b,
    const u32             len,
    error                *e
)
{
  const i32 written = cbuffer_write_to_file_1 (dest, b, len, e);
  if (written < 0)
  {
    return written;
  }
  ASSERT ((u32)written == len);
  return SUCCESS;
}

void
cbuffer_write_to_file_2 (struct cbuffer *b, const u32 nwritten)
{
  DBG_ASSERT (cbuffer, b);
  ASSERT (nwritten <= cbuffer_len (b));

  b->tail = (b->tail + nwritten) % b->cap;

  if (nwritten > 0)
  {
    b->isfull = 0;
  }
}

i32
cbuffer_write_to_file (i_file *dest, struct cbuffer *b, const u32 len, error *e)
{
  DBG_ASSERT (cbuffer, b);
  ASSERT (dest);

  const i32 result = cbuffer_write_to_file_1 (dest, b, len, e);
  if (result < 0)
  {
    return result;
  }

  cbuffer_write_to_file_2 (b, (u32)result);

  return result;
}

////////////////////////////////////////////////////////////
// WORKING WITH SINGLE ELEMENTS
static u8
cbuffer_get_no_check (
    void                 *dest,
    const u32             size,
    const u32             idx,
    const struct cbuffer *b
)
{
  DBG_ASSERT (cbuffer, b);
  ASSERT (size > 0);
  ASSERT (idx * size < b->cap);

  const u32 len = cbuffer_len (b) / size;
  ASSERT (idx < len);

  if (dest)
  {
    const u32 offset = (b->tail + idx * size) % b->cap;

    if (offset + size <= b->cap)
    {
      memcpy (dest, b->data + offset, size);
    }
    else
    {
      const u32 first = b->cap - offset;
      memcpy (dest, b->data + offset, first);
      memcpy ((u8 *)dest + first, b->data, size - first);
    }
  }

  return 0;
}

#ifdef TESTING
TEST (cbuffer_get_no_check)
{
  TEST_CASE ("Normal behavior")
  {
    u8             buf[4];
    struct cbuffer b = cbuffer_create (buf, 4);
    u8             out[4];

    // write: [7, 8, 9]
    const u8 src[3] = {7, 8, 9};
    cbuffer_write (src, 1, 3, &b);

    // get(0) = 7
    cbuffer_get_no_check (out, 1, 0, &b);
    test_assert_int_equal (out[0], 7);

    // get(1) = 8
    cbuffer_get_no_check (out, 1, 1, &b);
    test_assert_int_equal (out[0], 8);

    // get(2) = 9
    cbuffer_get_no_check (out, 1, 2, &b);
    test_assert_int_equal (out[0], 9);

    // still full
    test_assert_int_equal (cbuffer_len (&b), 3);

    // test wraparound for size=2
    struct cbuffer b2      = cbuffer_create (buf, 4);
    const u8       src2[4] = {0x11, 0x22, 0x33, 0x44};
    cbuffer_write (src2, 2, 2, &b2); // two 2-byte elements

    u8 out2[2];
    cbuffer_get_no_check (out2, 2, 0, &b2);
    test_assert_int_equal (out2[0], 0x11);
    test_assert_int_equal (out2[1], 0x22);

    cbuffer_get_no_check (out2, 2, 1, &b2);
    test_assert_int_equal (out2[0], 0x33);
    test_assert_int_equal (out2[1], 0x44);

    test_assert_int_equal (cbuffer_len (&b2), 4);
  }

  TEST_CASE ("wraparound behavior")
  {
    u8             buf[4];
    struct cbuffer b = cbuffer_create (buf, 4);

    // Fill buffer completely
    const u8 src1[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    cbuffer_write (src1, 1, 4, &b); // buffer now full

    u8 temp[2];
    cbuffer_read (
        temp,
        1,
        2,
        &b
    ); // advance tail by 2 bytes // head = 0, tail = 2 now

    const u8 src2[2] = {0xEE, 0xFF};
    cbuffer_write (src2, 1, 2,
                   &b); // write at head (0,1), causes wrap

    // buffer now has: [0xEE, 0xFF, 0xCC, 0xDD]
    // tail = 2, head = 2, full circle
    // logical order: [0xCC, 0xDD, 0xEE, 0xFF]

    u8 out;
    cbuffer_get_no_check (&out, 1, 0, &b); // tail+0
    test_assert_int_equal (out, 0xCC);

    cbuffer_get_no_check (&out, 1, 1, &b); // tail+1
    test_assert_int_equal (out, 0xDD);

    cbuffer_get_no_check (&out, 1, 2, &b); // wrap to buf[0]
    test_assert_int_equal (out, 0xEE);

    cbuffer_get_no_check (&out, 1, 3, &b); // buf[1]
    test_assert_int_equal (out, 0xFF);
  }
}
#endif

bool
cbuffer_get (void *dest, const u32 size, const u32 idx, const struct cbuffer *b)
{
  DBG_ASSERT (cbuffer, b);
  ASSERT (dest);

  if (!b->isfull && (u32)idx >= (b->head + b->cap - b->tail) % b->cap)
  {
    return false;
  }

  cbuffer_get_no_check (dest, size, idx, b);

  return true;
}

#ifdef TESTING
TEST (cbuffer_get)
{
  u8             buf[2];
  struct cbuffer b = cbuffer_create (buf, 2);
  const u8       x = 5;
  cbuffer_push_back (&x, 1, &b);

  u8 v = 0xFF;
  // in-range index
  test_assert_int_equal (cbuffer_get (&v, 1, 0, &b), 1);
  test_assert_int_equal (v, 5);
  // out-of-range index
  test_assert_int_equal (cbuffer_get (&v, 1, 1, &b), 0);
}
#endif

bool
cbuffer_peek_back (void *dest, const u32 size, const struct cbuffer *b)
{
  ASSERT (dest);
  DBG_ASSERT (cbuffer, b);

  if (!b->isfull && b->head == b->tail)
  {
    return false;
  }

  return cbuffer_get (dest, size, 0, b);
}

#ifdef TESTING
TEST (cbuffer_peek_back)
{
  u8             buf1[1];
  struct cbuffer b1 = cbuffer_create (buf1, 1);
  u8             v;

  // peek on empty buffer fails
  test_assert_int_equal (cbuffer_peek_back (&v, 1, &b1), 0);

  // push_back then peek
  u8 x = 0xAA;
  cbuffer_push_back (&x, 1, &b1);
  test_assert_int_equal (cbuffer_peek_back (&v, 1, &b1), 1);
  test_assert_int_equal (v, 0xAA);
  test_assert_int_equal (cbuffer_len (&b1), 1);

  // pop_back clears it
  cbuffer_pop_back (&v, 1, &b1);
  test_assert_int_equal (cbuffer_peek_back (&v, 1, &b1), 0);

  // wraparound
  u8             buf2[3];
  struct cbuffer b2 = cbuffer_create (buf2, 3);
  x                 = 5;
  cbuffer_push_back (&x, 1, &b2);
  x = 6;
  cbuffer_push_back (&x, 1, &b2);
  x = 7;
  cbuffer_push_back (&x, 1, &b2);

  test_assert_int_equal (cbuffer_pop_back (&v, 1, &b2), 1);
  test_assert_int_equal (cbuffer_pop_back (&v, 1, &b2), 1);

  x = 8;
  cbuffer_push_back (&x, 1, &b2);

  test_assert_int_equal (cbuffer_peek_back (&v, 1, &b2), 1);
  test_assert_int_equal (v, 7);
}
#endif

bool
cbuffer_peek_front (void *dest, const u32 size, const struct cbuffer *b)
{
  ASSERT (dest);
  DBG_ASSERT (cbuffer, b);

  if (!b->isfull && b->head == b->tail)
  {
    return false;
  }

  return cbuffer_get (dest, size, 0, b);
}

#ifdef TESTING
TEST (cbuffer_peek_front)
{
  u8             buf1[1];
  struct cbuffer b1 = cbuffer_create (buf1, 1);
  u8             v;

  // peek from empty buffer fails
  test_assert_int_equal (cbuffer_peek_front (&v, 1, &b1), 0);

  // push_front then peek_front
  u8 x = 0xAB;
  cbuffer_push_front (&x, 1, &b1);
  test_assert_int_equal (cbuffer_peek_front (&v, 1, &b1), 1);
  test_assert_int_equal (v, 0xAB);
  test_assert_int_equal (cbuffer_len (&b1), 1);

  // pop_front clears it
  cbuffer_pop_front (&v, 1, &b1);
  test_assert_int_equal (cbuffer_peek_front (&v, 1, &b1), 0);

  // wraparound
  u8             buf2[3];
  struct cbuffer b2 = cbuffer_create (buf2, 3);
  x                 = 1;
  cbuffer_push_front (&x, 1, &b2);
  x = 2;
  cbuffer_push_front (&x, 1, &b2);
  x = 3;
  cbuffer_push_front (&x, 1, &b2);

  test_assert_int_equal (cbuffer_pop_front (&v, 1, &b2), 1);
  test_assert_int_equal (cbuffer_pop_front (&v, 1, &b2), 1);

  x = 4;
  cbuffer_push_front (&x, 1, &b2);

  test_assert_int_equal (cbuffer_peek_front (&v, 1, &b2), 1);
  test_assert_int_equal (v, 4);
}
#endif

bool
cbuffer_push_back (const void *src, const u32 size, struct cbuffer *b)
{
  DBG_ASSERT (cbuffer, b);
  ASSERT (size > 0);

  if (cbuffer_avail (b) < size)
  {
    return false;
  }

  const u32 offset = b->head;
  if (offset + size <= b->cap)
  {
    memcpy (b->data + offset, src, size);
  }
  else
  {
    const u32 first = b->cap - offset;
    memcpy (b->data + offset, src, first);
    memcpy (b->data, (u8 *)src + first, size - first);
  }

  b->head = (b->head + size) % b->cap;
  if (b->head == b->tail)
  {
    b->isfull = true;
  }

  return true;
}

#ifdef TESTING
TEST (cbuffer_push_back)
{
  u8             buf[2];
  struct cbuffer b = cbuffer_create (buf, 2);
  u8             v;

  u8 x = 0x11;
  test_assert_int_equal (cbuffer_push_back (&x, 1, &b), 1);

  x = 0x22;
  test_assert_int_equal (cbuffer_push_back (&x, 1, &b), 1);

  x = 0x33;
  test_assert_int_equal (cbuffer_push_back (&x, 1, &b), 0);

  test_assert_int_equal (cbuffer_pop_back (&v, 1, &b), 1);
  test_assert_int_equal (v, 0x11);

  x = 0x33;
  test_assert_int_equal (cbuffer_push_back (&x, 1, &b), 1);

  x = 0x44;
  test_assert_int_equal (cbuffer_push_back (&x, 1, &b), 0);
}
#endif

bool
cbuffer_push_front (const void *src, const u32 size, struct cbuffer *b)
{
  DBG_ASSERT (cbuffer, b);
  ASSERT (size > 0);

  if (cbuffer_avail (b) < size)
  {
    return false;
  }

  // Compute new tail position before wrapping
  const u32 new_tail = (b->tail + b->cap - size) % b->cap;

  if (new_tail + size <= b->cap)
  {
    memcpy (b->data + new_tail, src, size);
  }
  else
  {
    const u32 first = b->cap - new_tail;
    memcpy (b->data + new_tail, src, first);
    memcpy (b->data, (u8 *)src + first, size - first);
  }

  b->tail = new_tail;
  if (b->head == b->tail)
  {
    b->isfull = true;
  }

  return true;
}

#ifdef TESTING
TEST (cbuffer_push_front)
{
  u8             buf[3];
  struct cbuffer b = cbuffer_create (buf, 3);
  u8             v;

  // Push front 0x44
  u8 x = 0x44;
  test_assert_int_equal (cbuffer_push_front (&x, 1, &b), 1);

  // Push front 0x33
  x = 0x33;
  test_assert_int_equal (cbuffer_push_front (&x, 1, &b), 1);

  // Push front 0x22
  x = 0x22;
  test_assert_int_equal (cbuffer_push_front (&x, 1, &b), 1);

  // Buffer full: next push fails
  x = 0x11;
  test_assert_int_equal (cbuffer_push_front (&x, 1, &b), 0);

  // Pop front returns 0x22
  test_assert_int_equal (cbuffer_pop_front (&v, 1, &b), 1);
  test_assert_int_equal (v, 0x22);

  // Push front 0x11 now succeeds (wrap-around)
  x = 0x11;
  test_assert_int_equal (cbuffer_push_front (&x, 1, &b), 1);

  // Pop all remaining values in order: 0x11, 0x33, 0x44
  test_assert_int_equal (cbuffer_pop_front (&v, 1, &b), 1);
  test_assert_int_equal (v, 0x11);
  test_assert_int_equal (cbuffer_pop_front (&v, 1, &b), 1);
  test_assert_int_equal (v, 0x33);
  test_assert_int_equal (cbuffer_pop_front (&v, 1, &b), 1);
  test_assert_int_equal (v, 0x44);
}
#endif

bool
cbuffer_pop_back (void *dest, const u32 size, struct cbuffer *b)
{
  ASSERT (dest);
  DBG_ASSERT (cbuffer, b);
  ASSERT (size > 0);

  if (!b->isfull && b->head == b->tail)
  {
    return false;
  }

  const u32 offset = b->tail;
  if (offset + size <= b->cap)
  {
    memcpy (dest, b->data + offset, size);
  }
  else
  {
    const u32 first = b->cap - offset;
    memcpy (dest, b->data + offset, first);
    memcpy ((u8 *)dest + first, b->data, size - first);
  }

  b->tail   = (b->tail + size) % b->cap;
  b->isfull = false;

  return true;
}

#ifdef TESTING
TEST (cbuffer_pop_back)
{
  u8             buf1[1];
  struct cbuffer b1 = cbuffer_create (buf1, 1);
  u8             v;

  test_assert_int_equal (cbuffer_pop_back (&v, 1, &b1), 0);

  u8 x = 0x7F;
  cbuffer_push_back (&x, 1, &b1);
  test_assert_int_equal (cbuffer_pop_back (&v, 1, &b1), 1);
  test_assert_int_equal (v, 0x7F);

  test_assert_int_equal (cbuffer_pop_back (&v, 1, &b1), 0);

  u8             buf2[3];
  struct cbuffer b2 = cbuffer_create (buf2, 3);

  x = 1;
  cbuffer_push_back (&x, 1, &b2);
  x = 2;
  cbuffer_push_back (&x, 1, &b2);
  x = 3;
  cbuffer_push_back (&x, 1, &b2);

  test_assert_int_equal (cbuffer_pop_back (&v, 1, &b2), 1);
  test_assert_int_equal (v, 1);

  x = 4;
  cbuffer_push_back (&x, 1, &b2);

  test_assert_int_equal (cbuffer_pop_back (&v, 1, &b2), 1);
  test_assert_int_equal (v, 2);
  test_assert_int_equal (cbuffer_pop_back (&v, 1, &b2), 1);
  test_assert_int_equal (v, 3);
  test_assert_int_equal (cbuffer_pop_back (&v, 1, &b2), 1);
  test_assert_int_equal (v, 4);
}
#endif

bool
cbuffer_pop_front (void *dest, const u32 size, struct cbuffer *b)
{
  ASSERT (dest);
  DBG_ASSERT (cbuffer, b);
  ASSERT (size > 0);

  if (!b->isfull && b->head == b->tail)
  {
    return false;
  }

  const u32 offset = b->tail;
  if (offset + size <= b->cap)
  {
    memcpy (dest, b->data + offset, size);
  }
  else
  {
    const u32 first = b->cap - offset;
    memcpy (dest, b->data + offset, first);
    memcpy ((u8 *)dest + first, b->data, size - first);
  }

  b->tail   = (b->tail + size) % b->cap;
  b->isfull = false;

  return true;
}

#ifdef TESTING
TEST (cbuffer_pop_front)
{
  u8             buf1[1];
  struct cbuffer b1 = cbuffer_create (buf1, 1);
  u8             v;

  // pop from empty fails
  test_assert_int_equal (cbuffer_pop_front (&v, 1, &b1), 0);

  // single push_front and pop_front
  u8 x = 0xAB;
  cbuffer_push_front (&x, 1, &b1);
  test_assert_int_equal (cbuffer_pop_front (&v, 1, &b1), 1);
  test_assert_int_equal (v, 0xAB);

  test_assert_int_equal (cbuffer_pop_front (&v, 1, &b1), 0);

  // fill buffer with push_front in reverse order: [3, 2, 1]
  u8             buf2[3];
  struct cbuffer b2 = cbuffer_create (buf2, 3);

  x = 1;
  cbuffer_push_front (&x, 1, &b2);
  x = 2;
  cbuffer_push_front (&x, 1, &b2);
  x = 3;
  cbuffer_push_front (&x, 1, &b2);

  test_assert_int_equal (cbuffer_pop_front (&v, 1, &b2), 1); // gets 3
  test_assert_int_equal (v, 3);
  test_assert_int_equal (cbuffer_pop_front (&v, 1, &b2), 1); // gets 2
  test_assert_int_equal (v, 2);
  test_assert_int_equal (cbuffer_pop_front (&v, 1, &b2), 1); // gets 1
  test_assert_int_equal (v, 1);
}
#endif

#include <string.h>

DEFINE_DBG_ASSERT (struct dbl_buffer, dbl_buffer, d, {
  ASSERT (d);
  ASSERT (d->data);
  ASSERT (d->nelem <= d->nelem_cap);
})

err_t
dblb_create (
    struct dbl_buffer *dest,
    const u32          size,
    const u32          initial_cap,
    error             *e
)
{
  ASSERT (initial_cap > 0);
  ASSERT (size > 0);

  void *data = i_malloc (initial_cap, size, e);
  if (data == NULL)
  {
    return error_trace (e);
  }

  *dest = (struct dbl_buffer){
      .size      = size,
      .nelem     = 0,
      .nelem_cap = initial_cap,
      .data      = data,
  };

  DBG_ASSERT (dbl_buffer, dest);

  return SUCCESS;
}

err_t
dblb_append (struct dbl_buffer *d, const void *data, const u32 nelem, error *e)
{
  DBG_ASSERT (dbl_buffer, d);

  void *head = dblb_append_alloc (d, nelem, e);
  if (head == NULL)
  {
    return error_trace (e);
  }

  memcpy (head, data, d->size * nelem);

  return SUCCESS;
}

err_t
dblb_ensure_space (struct dbl_buffer *d, const u32 nelem, error *e)
{
  if (nelem >= d->nelem_cap)
  {
    void *newdata =
        i_realloc_right (d->data, d->nelem_cap, 2 * nelem, d->size, e);
    if (newdata == NULL)
    {
      return error_trace (e);
    }
    d->data      = newdata;
    d->nelem_cap = 2 * nelem;
  }
  return error_trace (e);
}

void *
dblb_append_alloc (struct dbl_buffer *d, const u32 nelem, error *e)
{
  DBG_ASSERT (dbl_buffer, d);

  const u32 newnelem_cap = d->nelem + nelem;

  if (dblb_ensure_space (d, newnelem_cap, e))
  {
    return NULL;
  }

  void *ret = (u8 *)d->data + d->nelem * d->size;
  d->nelem += nelem;

  return ret;
}

void
dblb_free (struct dbl_buffer *d)
{
  DBG_ASSERT (dbl_buffer, d);

  i_free (d->data);
  d->nelem_cap = 0;
  d->nelem     = 0;
}

#ifdef TESTING
TEST (dblb_create_basic)
{
  struct dbl_buffer db;
  error             e = error_create ();

  // Create buffer with size=4, initial_cap=2
  const err_t err = dblb_create (&db, 4, 2, &e);
  test_assert_int_equal (err, SUCCESS);
  test_assert_int_equal (db.size, 4);
  test_assert_int_equal (db.nelem, 0);
  test_assert_int_equal (db.nelem_cap, 2);
  test_assert (db.data != NULL);

  dblb_free (&db);
}

TEST (dblb_append_single)
{
  struct dbl_buffer db;
  error             e = error_create ();

  dblb_create (&db, sizeof (u32), 4, &e);

  // Append single element
  u32         val = 0x12345678;
  const err_t err = dblb_append (&db, &val, 1, &e);
  test_assert_int_equal (err, SUCCESS);
  test_assert_int_equal (db.nelem, 1);
  test_assert_int_equal (db.nelem_cap, 4);

  // Verify data
  const u32 *data = (u32 *)db.data;
  test_assert_int_equal (data[0], 0x12345678);

  dblb_free (&db);
}

TEST (dblb_append_multiple)
{
  struct dbl_buffer db;
  error             e = error_create ();

  dblb_create (&db, sizeof (u32), 2, &e);

  // Append multiple elements
  u32         vals[] = {0x11, 0x22, 0x33};
  const err_t err    = dblb_append (&db, vals, 3, &e);
  test_assert_int_equal (err, SUCCESS);
  test_assert_int_equal (db.nelem, 3);

  // Verify data
  const u32 *data = (u32 *)db.data;
  test_assert_int_equal (data[0], 0x11);
  test_assert_int_equal (data[1], 0x22);
  test_assert_int_equal (data[2], 0x33);

  dblb_free (&db);
}

TEST (dblb_append_triggers_realloc)
{
  struct dbl_buffer db;
  error             e = error_create ();

  // Start with capacity of 2
  dblb_create (&db, sizeof (u32), 2, &e);

  u32 val1 = 0xAA;
  dblb_append (&db, &val1, 1, &e);
  test_assert_int_equal (db.nelem, 1);
  test_assert_int_equal (db.nelem_cap, 2);

  u32 val2 = 0xBB;
  dblb_append (&db, &val2, 1, &e);
  test_assert_int_equal (db.nelem, 2);
  test_assert_int_equal (db.nelem_cap, 4);

  u32 val3 = 0xCC;
  dblb_append (&db, &val3, 1, &e);
  test_assert_int_equal (db.nelem, 3);
  test_assert_int_equal (db.nelem_cap, 4);

  u32 val4 = 0xDD;
  dblb_append (&db, &val4, 1, &e);
  test_assert_int_equal (db.nelem, 4);
  test_assert_int_equal (db.nelem_cap, 8);

  const u32 *data = (u32 *)db.data;
  test_assert_int_equal (data[0], 0xAA);
  test_assert_int_equal (data[1], 0xBB);
  test_assert_int_equal (data[2], 0xCC);
  test_assert_int_equal (data[3], 0xDD);

  dblb_free (&db);
}

TEST (dblb_append_alloc_basic)
{
  struct dbl_buffer db;
  error             e = error_create ();

  dblb_create (&db, sizeof (u32), 4, &e);

  // Allocate space for 2 elements
  void *ptr = dblb_append_alloc (&db, 2, &e);
  test_assert (ptr != NULL);
  test_assert_int_equal (db.nelem, 2);

  // Write directly to allocated space
  u32 *data = (u32 *)ptr;
  data[0]   = 0x1111;
  data[1]   = 0x2222;

  // Verify
  const u32 *base = (u32 *)db.data;
  test_assert_int_equal (base[0], 0x1111);
  test_assert_int_equal (base[1], 0x2222);

  dblb_free (&db);
}

TEST (dblb_append_alloc_sequential)
{
  struct dbl_buffer db;
  error             e = error_create ();

  dblb_create (&db, sizeof (u32), 8, &e);

  // First allocation
  u32 *ptr1 = (u32 *)dblb_append_alloc (&db, 2, &e);
  test_assert (ptr1 != NULL);
  ptr1[0] = 0xAA;
  ptr1[1] = 0xBB;

  // Second allocation - should be right after first
  u32 *ptr2 = (u32 *)dblb_append_alloc (&db, 2, &e);
  test_assert (ptr2 != NULL);
  test_assert_int_equal ((u8 *)ptr2 - (u8 *)ptr1, 2 * sizeof (u32));
  ptr2[0] = 0xCC;
  ptr2[1] = 0xDD;

  // Verify all data
  const u32 *base = (u32 *)db.data;
  test_assert_int_equal (base[0], 0xAA);
  test_assert_int_equal (base[1], 0xBB);
  test_assert_int_equal (base[2], 0xCC);
  test_assert_int_equal (base[3], 0xDD);
  test_assert_int_equal (db.nelem, 4);

  dblb_free (&db);
}

TEST (dblb_append_alloc_triggers_realloc)
{
  struct dbl_buffer db;
  error             e = error_create ();

  dblb_create (&db, sizeof (u32), 2, &e);

  // Fill to capacity
  dblb_append_alloc (&db, 2, &e);
  test_assert_int_equal (db.nelem_cap, 4);

  // This should trigger realloc
  const void *ptr = dblb_append_alloc (&db, 1, &e);
  test_assert (ptr != NULL);
  test_assert_int_equal (db.nelem, 3);
  test_assert_int_equal (db.nelem_cap, 4);

  dblb_free (&db);
}

TEST (dblb_different_element_sizes)
{
  struct dbl_buffer db;
  error             e = error_create ();

  // Test with u8
  dblb_create (&db, sizeof (u8), 4, &e);
  u8 byte = 0x42;
  dblb_append (&db, &byte, 1, &e);
  test_assert_int_equal (((u8 *)db.data)[0], 0x42);
  dblb_free (&db);

  // Test with u64
  dblb_create (&db, sizeof (u64), 4, &e);
  u64 large = 0x123456789ABCDEF0;
  dblb_append (&db, &large, 1, &e);
  test_assert_equal (((u64 *)db.data)[0], 0x123456789ABCDEF0);
  dblb_free (&db);
}

TEST (dblb_struct_elements)
{
  struct dbl_buffer db;
  error             e = error_create ();

  struct test_struct
  {
    u32 id;
    u32 value;
  };

  dblb_create (&db, sizeof (struct test_struct), 2, &e);

  struct test_struct s1 = {.id = 1, .value = 100};
  struct test_struct s2 = {.id = 2, .value = 200};

  dblb_append (&db, &s1, 1, &e);
  dblb_append (&db, &s2, 1, &e);

  const struct test_struct *data = (struct test_struct *)db.data;
  test_assert_int_equal (data[0].id, 1);
  test_assert_int_equal (data[0].value, 100);
  test_assert_int_equal (data[1].id, 2);
  test_assert_int_equal (data[1].value, 200);

  dblb_free (&db);
}

TEST (dblb_free_resets)
{
  struct dbl_buffer db;
  error             e = error_create ();

  dblb_create (&db, sizeof (u32), 4, &e);
  u32 val = 0x1234;
  dblb_append (&db, &val, 1, &e);

  dblb_free (&db);

  // Verify fields are reset
  test_assert_int_equal (db.nelem, 0);
  test_assert_int_equal (db.nelem_cap, 0);
}

TEST (dblb_large_append)
{
  struct dbl_buffer db;
  error             e = error_create ();

  dblb_create (&db, sizeof (u32), 2, &e);

  // Append many elements at once
  u32 vals[100];
  for (u32 i = 0; i < 100; i++)
  {
    vals[i] = i;
  }

  dblb_append (&db, vals, 100, &e);
  test_assert_int_equal (db.nelem, 100);

  // Verify data
  const u32 *data = (u32 *)db.data;
  for (u32 i = 0; i < 100; i++)
  {
    test_assert_int_equal (data[i], i);
  }

  dblb_free (&db);
}
#endif

#include <string.h>

struct ext_array
ext_array_create (void)
{
  return (struct ext_array){
      .data = NULL,
      .len  = 0,
      .cap  = 0,
  };
}

void
ext_array_free (struct ext_array *r)
{
  if (r->data)
  {
    i_free (r->data);
  }
  r->data = NULL;
  r->len  = 0;
  r->cap  = 0;
}

static err_t
ext_array_reserve (struct ext_array *r, const u32 cap, error *e)
{
  if (cap > r->cap)
  {
    u8 *data = i_realloc_right (r->data, r->cap, cap * 2, 1, e);
    if (data == NULL)
    {
      return error_trace (e);
    }
    r->data = data;
    r->cap  = cap * 2;
  }
  return SUCCESS;
}

i64
ext_array_insert (
    struct ext_array *r,
    const u32         ofst,
    const void       *src,
    const u32         slen,
    error            *e
)
{
  ASSERT (ofst <= r->len);
  if (ext_array_reserve (r, r->len + slen, e))
  {
    return error_trace (e);
  }

  const u32 tlen = r->len - ofst;
  if (tlen > 0)
  {
    u8 *tail = i_malloc (tlen, 1, e);
    if (tail == NULL)
    {
      return error_trace (e);
    }
    memcpy (tail, r->data + ofst, tlen);
    memcpy (r->data + ofst, src, slen);
    memcpy (r->data + ofst + slen, tail, tlen);
    i_free (tail);
  }
  else
  {
    memcpy (r->data + ofst, src, slen);
  }

  r->len += slen;
  return slen;
}

i64
ext_array_read (
    const struct ext_array *r,
    const struct stride     str,
    const u32               size,
    void                   *_dest,
    error                  *e
)
{
  u8 *dest       = _dest;
  u32 total_read = 0;
  u32 bidx       = str.start * size;

  while (total_read < str.nelems)
  {
    if (bidx + size > r->len)
    {
      return total_read;
    }
    memcpy (dest, r->data + bidx, size);
    dest += size;
    bidx += str.stride * size;
    total_read++;
  }

  return total_read;
}

i64
ext_array_write (
    const struct ext_array *r,
    const struct stride     str,
    const u32               size,
    const void             *_src,
    error                  *e
)
{
  const u8 *src           = _src;
  u32       total_written = 0;
  u32       bidx          = str.start * size;

  while (total_written < str.nelems)
  {
    if (bidx + size > r->len)
    {
      return total_written;
    }
    memcpy (r->data + bidx, src, size);
    src += size;
    bidx += str.stride * size;
    total_written++;
  }

  return total_written;
}

i64
ext_array_remove (
    struct ext_array   *r,
    const struct stride str,
    const u32           size,
    void               *_dest,
    error              *e
)
{
  u8 *dest          = _dest;
  u32 total_removed = 0;
  u32 wpos          = 0;
  u32 rpos          = 0;
  u32 next_remove   = str.start;

  while (rpos * size < r->len)
  {
    if (total_removed < str.nelems && rpos == next_remove)
    {
      if (dest)
      {
        memcpy (dest, r->data + rpos * size, size);
        dest += size;
      }
      total_removed++;
      next_remove += str.stride;
    }
    else
    {
      if (wpos != rpos)
      {
        memmove (r->data + wpos * size, r->data + rpos * size, size);
      }
      wpos++;
    }
    rpos++;
  }

  r->len = wpos * size;
  return total_removed;
}

u64
ext_array_get_len (const struct ext_array *r)
{
  return r->len;
}

static err_t
ext_array_insert_func (
    void       *ctx,
    const u32   ofst,
    const void *src,
    const u32   slen,
    error      *e
)
{
  struct ext_array *arr = ctx;
  return ext_array_insert (arr, ofst, src, slen, e);
}
static i64
ext_array_read_func (
    void               *ctx,
    const struct stride str,
    const u32           size,
    void               *dest,
    error              *e
)
{
  struct ext_array *arr = ctx;
  return ext_array_read (arr, str, size, dest, e);
}
static i64
ext_array_write_func (
    void               *ctx,
    const struct stride str,
    const u32           size,
    const void         *src,
    error              *e
)
{
  struct ext_array *arr = ctx;
  return ext_array_write (arr, str, size, src, e);
}
static i64
ext_array_remove_func (
    void               *ctx,
    const struct stride str,
    const u32           size,
    void               *dest,
    error              *e
)
{
  struct ext_array *arr = ctx;
  return ext_array_remove (arr, str, size, dest, e);
}
static i64
ext_array_getlen_func (void *ctx, error *e)
{
  struct ext_array *arr = ctx;
  return ext_array_get_len (arr);
}

static const struct data_writer_functions ext_array_funcs = {
    .insert = ext_array_insert_func,
    .read   = ext_array_read_func,
    .write  = ext_array_write_func,
    .remove = ext_array_remove_func,
    .getlen = ext_array_getlen_func,
};

void
ext_array_data_writer (struct data_writer *dest, struct ext_array *arr)
{
  dest->functions = ext_array_funcs;
  dest->ctx       = arr;
}

#ifdef TESTING
TEST (ext_array_insert_read)
{
  TEST_CASE ("basic sequential")
  {
    error            e = error_create ();
    struct ext_array a = ext_array_create ();

    const u32 src[] = {1, 2, 3, 4, 5};
    ext_array_insert (&a, 0, src, sizeof (src), &e);

    u32       dest[5] = {0};
    const i64 n       = ext_array_read (
        &a,
        (struct stride){
            .start  = 0,
            .stride = 1,
            .nelems = 5,
        },
        sizeof (u32),
        dest,
        &e
    );

    test_assert (n == 5);
    test_assert_memequal (src, dest, arrlen (src));
    ext_array_free (&a);
  }

  TEST_CASE ("insert at end (append)")
  {
    error            e = error_create ();
    struct ext_array a = ext_array_create ();

    const u32 first[]  = {1, 2, 3};
    const u32 second[] = {4, 5, 6};
    ext_array_insert (&a, 0, first, sizeof (first), &e);
    ext_array_insert (&a, sizeof (first), second, sizeof (second), &e);

    u32 dest[6] = {0};
    ext_array_read (
        &a,
        (struct stride){
            .start  = 0,
            .stride = 1,
            .nelems = 6,
        },
        sizeof (u32),
        dest,
        &e
    );

    const u32 expected[] = {1, 2, 3, 4, 5, 6};
    test_assert_memequal (expected, dest, arrlen (expected));
    ext_array_free (&a);
  }

  TEST_CASE ("insert in middle")
  {
    error            e = error_create ();
    struct ext_array a = ext_array_create ();

    const u32 initial[]  = {1, 3, 4};
    const u32 inserted[] = {2};
    ext_array_insert (&a, 0, initial, sizeof (initial), &e);
    ext_array_insert (&a, sizeof (u32), inserted, sizeof (inserted), &e);

    u32 dest[4] = {0};
    ext_array_read (
        &a,
        (struct stride){
            .start  = 0,
            .stride = 1,
            .nelems = 4,
        },
        sizeof (u32),
        dest,
        &e
    );

    const u32 expected[] = {1, 2, 3, 4};
    test_assert_memequal (expected, dest, arrlen (expected));
    ext_array_free (&a);
  }

  TEST_CASE ("read strided")
  {
    error            e = error_create ();
    struct ext_array a = ext_array_create ();

    const u32 src[] = {1, 2, 3, 4, 5, 6};
    ext_array_insert (&a, 0, src, sizeof (src), &e);

    u32       dest[3] = {0};
    const i64 n       = ext_array_read (
        &a,
        (struct stride){
            .start  = 0,
            .stride = 2,
            .nelems = 3,
        },
        sizeof (u32),
        dest,
        &e
    );

    test_assert (n == 3);
    const u32 expected[] = {1, 3, 5};
    test_assert_memequal (expected, dest, arrlen (expected));
    ext_array_free (&a);
  }

  TEST_CASE ("read past end")
  {
    error            e = error_create ();
    struct ext_array a = ext_array_create ();

    const u32 src[] = {10, 20, 30};
    ext_array_insert (&a, 0, src, sizeof (src), &e);

    u32       dest[10] = {0};
    const i64 n        = ext_array_read (
        &a,
        (struct stride){
            .start  = 1,
            .stride = 1,
            .nelems = 10,
        },
        sizeof (u32),
        dest,
        &e
    );

    test_assert (n == 2);
    const u32 expected[] = {20, 30};
    test_assert_memequal (expected, dest, 2);
    ext_array_free (&a);
  }
}

TEST (ext_array_write)
{
  TEST_CASE ("overwrite single middle")
  {
    error            e = error_create ();
    struct ext_array a = ext_array_create ();

    const u32 src[] = {1, 2, 3, 4, 5};
    ext_array_insert (&a, 0, src, sizeof (src), &e);

    const u32 patch[] = {99};
    const i64 n       = ext_array_write (
        &a,
        (struct stride){
            .start  = 2,
            .stride = 1,
            .nelems = 1,
        },
        sizeof (u32),
        patch,
        &e
    );

    test_assert (n == 1);

    u32 dest[5] = {0};
    ext_array_read (
        &a,
        (struct stride){.start = 0, .stride = 1, .nelems = 5},
        sizeof (u32),
        dest,
        &e
    );

    const u32 expected[] = {1, 2, 99, 4, 5};
    test_assert_memequal (expected, dest, arrlen (expected));
    ext_array_free (&a);
  }

  TEST_CASE ("overwrite strided")
  {
    error            e = error_create ();
    struct ext_array a = ext_array_create ();

    const u32 src[] = {1, 2, 3, 4, 5, 6};
    ext_array_insert (&a, 0, src, sizeof (src), &e);

    const u32 patch[] = {0, 0, 0};
    const i64 n       = ext_array_write (
        &a,
        (struct stride){
            .start  = 0,
            .stride = 2,
            .nelems = 3,
        },
        sizeof (u32),
        patch,
        &e
    );

    test_assert (n == 3);

    u32 dest[6] = {0};
    ext_array_read (
        &a,
        (struct stride){
            .start  = 0,
            .stride = 1,
            .nelems = 6,
        },
        sizeof (u32),
        dest,
        &e
    );

    const u32 expected[] = {0, 2, 0, 4, 0, 6};
    test_assert_memequal (expected, dest, arrlen (expected));
    ext_array_free (&a);
  }

  TEST_CASE ("write past end returns short count")
  {
    error            e = error_create ();
    struct ext_array a = ext_array_create ();

    const u32 src[] = {1, 2, 3};
    ext_array_insert (&a, 0, src, sizeof (src), &e);

    const u32 patch[] = {9, 9, 9};
    const i64 n       = ext_array_write (
        &a,
        (struct stride){
            .start  = 2,
            .stride = 1,
            .nelems = 3,
        },
        sizeof (u32),
        patch,
        &e
    );

    test_assert (n == 1);
    ext_array_free (&a);
  }
}

TEST (ext_array_remove)
{
  TEST_CASE ("remove from middle")
  {
    error            e = error_create ();
    struct ext_array a = ext_array_create ();

    const u32 src[] = {1, 2, 3, 4, 5};
    ext_array_insert (&a, 0, src, sizeof (src), &e);

    u32       removed = 0;
    const i64 n       = ext_array_remove (
        &a,
        (struct stride){
            .start  = 2,
            .stride = 1,
            .nelems = 1,
        },
        sizeof (u32),
        &removed,
        &e
    );

    test_assert (n == 1);
    u32 expected_removed = 3;
    test_assert_memequal (&expected_removed, &removed, 1);

    u32 dest[4] = {0};
    ext_array_read (
        &a,
        (struct stride){
            .start  = 0,
            .stride = 1,
            .nelems = 4,
        },
        sizeof (u32),
        dest,
        &e
    );

    const u32 expected[] = {1, 2, 4, 5};
    test_assert_memequal (expected, dest, arrlen (expected));
    ext_array_free (&a);
  }

  TEST_CASE ("remove first")
  {
    error            e = error_create ();
    struct ext_array a = ext_array_create ();

    const u32 src[] = {10, 20, 30};
    ext_array_insert (&a, 0, src, sizeof (src), &e);

    u32 removed = 0;
    ext_array_remove (
        &a,
        (struct stride){
            .start  = 0,
            .stride = 1,
            .nelems = 1,
        },
        sizeof (u32),
        &removed,
        &e
    );

    u32 dest[2] = {0};
    ext_array_read (
        &a,
        (struct stride){
            .start  = 0,
            .stride = 1,
            .nelems = 2,
        },
        sizeof (u32),
        dest,
        &e
    );

    const u32 expected[] = {20, 30};
    test_assert_memequal (expected, dest, arrlen (expected));
    ext_array_free (&a);
  }

  TEST_CASE ("remove last")
  {
    error            e = error_create ();
    struct ext_array a = ext_array_create ();

    const u32 src[] = {10, 20, 30};
    ext_array_insert (&a, 0, src, sizeof (src), &e);

    u32 removed = 0;
    ext_array_remove (
        &a,
        (struct stride){
            .start  = 2,
            .stride = 1,
            .nelems = 1,
        },
        sizeof (u32),
        &removed,
        &e
    );

    u32 dest[2] = {0};
    ext_array_read (
        &a,
        (struct stride){
            .start  = 0,
            .stride = 1,
            .nelems = 2,
        },
        sizeof (u32),
        dest,
        &e
    );

    const u32 expected[] = {10, 20};
    test_assert_memequal (expected, dest, arrlen (expected));
    ext_array_free (&a);
  }

  TEST_CASE ("remove strided")
  {
    error            e = error_create ();
    struct ext_array a = ext_array_create ();

    const u32 src[] = {1, 2, 3, 4, 5, 6};
    ext_array_insert (&a, 0, src, sizeof (src), &e);

    u32       removed[3] = {0};
    const i64 n          = ext_array_remove (
        &a,
        (struct stride){
            .start  = 0,
            .stride = 2,
            .nelems = 3,
        },
        sizeof (u32),
        removed,
        &e
    );

    test_assert (n == 3);
    const u32 expected_removed[] = {1, 3, 5};
    test_assert_memequal (expected_removed, removed, arrlen (expected_removed));

    u32 dest[3] = {0};
    ext_array_read (
        &a,
        (struct stride){
            .start  = 0,
            .stride = 1,
            .nelems = 3,
        },
        sizeof (u32),
        dest,
        &e
    );

    const u32 expected[] = {2, 4, 6};
    test_assert_memequal (expected, dest, arrlen (expected));
    ext_array_free (&a);
  }
}

TEST (ext_array_random)
{
  error e    = error_create ();
  u32   size = 1;

  /**
  struct ext_array ext_arr_1 = ext_array_create ();
  struct ext_array ext_arr_2 = ext_array_create ();

  struct data_writer ref;
  struct data_writer sut;

  ext_array_data_writer (&ref, &ext_arr_1);
  ext_array_data_writer (&sut, &ext_arr_2);

  struct dvalidtr d = {
  .sut = sut,
  .ref = ref,
  };

  dvalidtr_random_test (&d, 1, 1000, 10000, &e);

  ext_array_free (&ext_arr_1);

  ext_array_free (&ext_arr_2);
  */

  // Block sizes to test
  const u32 niters[] = {100, 100, 100, 100, 100, 100, 1000, 1000, 1000, 1000};

  for (u32 i = 0; i < arrlen (niters); ++i)
  {
    i_log_info ("Block random test: %d\n", i);

    struct ext_array ext_arr_1 = ext_array_create ();
    struct ext_array ext_arr_2 = ext_array_create ();

    struct data_writer ref;
    struct data_writer sut;

    ext_array_data_writer (&ref, &ext_arr_1);
    ext_array_data_writer (&sut, &ext_arr_2);

    struct dvalidtr d = {
        .sut     = sut,
        .ref     = ref,
        .isvalid = NULL,
    };

    dvalidtr_random_test (&d, 1, niters[i], 1000, &e);

    ext_array_free (&ext_arr_1);
    ext_array_free (&ext_arr_2);
  }
}
#endif

#include <stdlib.h>
#include <string.h>

struct block_array *
block_array_create (const u32 cap_per_node, error *e)
{
  ASSERT (cap_per_node > 0);

  struct block_array *ret =
      i_malloc (1, sizeof (struct block_array) + cap_per_node, e);
  if (ret == NULL)
  {
    return ret;
  }

  slab_alloc_init (
      &ret->block_alloc,
      sizeof (struct block) + cap_per_node,
      512
  );
  ret->cap_per_node = cap_per_node;
  ret->head         = NULL;
  ret->tlen         = 0;

  return ret;
}

struct block_array *
block_array_clone (const struct block_array *src, error *e)
{
  struct block_array *dst = block_array_create (src->cap_per_node, e);
  if (!dst)
  {
    return NULL;
  }

  u64 total_bytes = block_array_getlen (src);
  if (total_bytes == 0)
  {
    return dst;
  }

  void *buf = malloc ((size_t)total_bytes);
  if (!buf)
  {
    block_array_free (dst);
    return NULL;
  }

  block_array_read (
      src,
      (struct stride){.start = 0, .stride = 1, .nelems = total_bytes},
      1,
      buf
  );

  err_t err = block_array_insert (dst, 0, buf, (u32)total_bytes, e);
  free (buf);

  if (err != 0)
  {
    block_array_free (dst);
    return NULL;
  }

  return dst;
}

void
block_array_free (struct block_array *r)
{
  slab_alloc_destroy (&r->block_alloc);
  i_free (r);
}

static struct block *
block_alloc_empty (struct block_array *r, struct block *prev, error *e)
{
  struct block *ret = slab_alloc_alloc (&r->block_alloc, e);
  if (ret == NULL)
  {
    return NULL;
  }

  ret->len = 0;
  if (prev)
  {
    ret->next  = prev->next;
    prev->next = ret;
    if (ret->next)
    {
      ret->next->prev = ret;
    }
  }
  else
  {
    ret->next = NULL;
  }
  ret->prev = prev;

  return ret;
}

err_t
block_array_insert (
    struct block_array *r,
    u32                 ofst,
    const void         *_src,
    u32                 slen,
    error              *e
)
{
  ASSERT (slen > 0);

  const u8 *src = _src;

  // Allocate head if it's empty
  if (r->head == NULL)
  {
    r->head = block_alloc_empty (r, r->head, e);
    if (r->head == NULL)
    {
      panic ("ROLLBACK");
    }
  }

  // Seek
  struct block *cur = r->head;
  while (ofst > cur->len)
  {
    ASSERT (cur->next != NULL); // Don't allow buffer overflows
    ofst -= cur->len;
    cur = cur->next;
  }

  // Save the tail
  r->tlen = cur->len - ofst;
  if (r->tlen > 0)
  {
    memcpy (r->tail, cur->data + ofst, r->tlen);
    cur->len = ofst;
  }

  struct block *last = cur->next;

  // Write out source data
  while (slen > 0)
  {
    // Advance forward
    if (cur->len == r->cap_per_node)
    {
      cur = block_alloc_empty (r, cur, e);
      if (cur == NULL)
      {
        panic ("ROLLBACK");
      }
    }

    // Append to cur
    u32 next_write = r->cap_per_node - cur->len; // Writable
    next_write     = MIN (next_write, slen);     // Readable
    memcpy (&cur->data[cur->len], src, next_write);

    ASSERT (next_write <= slen);

    src += next_write;
    slen -= next_write;
    cur->len += next_write;
  }

  // Write out tail data
  u32 twritten = 0;
  while (twritten < r->tlen)
  {
    // Advance forward
    if (cur->len == r->cap_per_node)
    {
      cur = block_alloc_empty (r, cur, e);
      if (cur == NULL)
      {
        panic ("ROLLBACK");
      }
    }

    // Append to cur
    u32 next_write = r->cap_per_node - cur->len;           // Writable
    next_write     = MIN (next_write, r->tlen - twritten); // Readable
    memcpy (&cur->data[cur->len], &r->tail[twritten], next_write);

    twritten += next_write;
    cur->len += next_write;
  }

  // Link the last node
  cur->next = last;
  r->tlen   = 0;

  return SUCCESS;
}

struct stride_state
{
  u32  next;
  bool active;
};

u64
block_array_read (
    const struct block_array *r,
    const struct stride       str,
    const u32                 size,
    void                     *_dest
)
{
  u8 *dest = _dest;

  // Seek
  const struct block *cur  = r->head;
  u32                 bidx = str.start * size;

  while (true)
  {
    if (cur == NULL)
    {
      return 0;
    }
    else if (bidx >= cur->len)
    {
      bidx -= cur->len;
      cur = cur->next;
    }
    else
    {
      break;
    }
  }

  struct stride_state state = {
      .next   = size,
      .active = true,
  };

  u32 total_read = 0;

  while (total_read < str.nelems)
  {
    if (state.active)
    {
      const u32 next_read = MIN (state.next, cur->len - bidx);
      if (next_read > 0)
      {
        memcpy (dest, &cur->data[bidx], next_read);
        dest += next_read;
      }
      bidx += next_read;
      state.next -= next_read;

      if (state.next == 0)
      {
        total_read++;

        if (str.stride == 1)
        {
          state = (struct stride_state){
              .next   = size,
              .active = true,
          };
        }
        else
        {
          state = (struct stride_state){
              .next   = (str.stride - 1) * size,
              .active = false,
          };
        }
      }
    }
    else
    {
      const u32 next = MIN (state.next, cur->len - bidx);
      bidx += next;
      state.next -= next;

      if (state.next == 0)
      {
        state = (struct stride_state){.next = size, .active = true};
      }
    }

    if (bidx == cur->len)
    {
      cur  = cur->next;
      bidx = 0;
      if (cur == NULL)
      {
        return total_read;
      }
    }
  }

  return total_read;
}

u64
block_array_write (
    const struct block_array *r,
    const struct stride       str,
    const u32                 size,
    const void               *_src
)
{
  const u8 *src = _src;

  // Seek
  struct block *cur  = r->head;
  u32           bidx = str.start * size;

  while (true)
  {
    if (cur == NULL)
    {
      return 0;
    }
    else if (bidx >= cur->len)
    {
      bidx -= cur->len;
      cur = cur->next;
    }
    else
    {
      break;
    }
  }

  struct stride_state state = {
      .next   = size,
      .active = true,
  };

  u32 total_written = 0;

  while (total_written < str.nelems)
  {
    if (state.active)
    {
      const u32 next = MIN (state.next, cur->len - bidx);
      if (next > 0)
      {
        memcpy (&cur->data[bidx], src, next);
      }
      src += next;
      bidx += next;
      state.next -= next;

      if (state.next == 0)
      {
        total_written++;

        if (str.stride == 1)
        {
          state = (struct stride_state){
              .next   = size,
              .active = true,
          };
        }
        else
        {
          state = (struct stride_state){
              .next   = (str.stride - 1) * size,
              .active = false,
          };
        }
      }
    }
    else
    {
      const u32 next = MIN (state.next, cur->len - bidx);
      bidx += next;
      state.next -= next;

      if (state.next == 0)
      {
        state = (struct stride_state){.next = size, .active = true};
      }
    }

    if (bidx == cur->len)
    {
      cur  = cur->next;
      bidx = 0;
      if (cur == NULL)
      {
        return total_written;
      }
    }
  }

  return total_written;
}

i64
block_array_remove (
    struct block_array *r,
    const struct stride str,
    const u32           size,
    void               *_dest,
    error              *e
)
{
  u8 *dest = _dest;

  // Seek
  struct block *rcur  = r->head;          // Read block
  u32           rbidx = str.start * size; // Read idx

  while (true)
  {
    if (rcur == NULL)
    {
      return 0;
    }
    else if (rbidx >= rcur->len)
    {
      rbidx -= rcur->len;
      rcur = rcur->next;
    }
    else
    {
      break;
    }
  }

  struct block *wcur  = rcur;  // Write block
  u32           wbidx = rbidx; // Wride idx

  struct stride_state state = {
      .next   = size,
      .active = true,
  };
  u32 total_removed = 0;

  while (total_removed < str.nelems)
  {
    if (state.active)
    {
      const u32 next = MIN (state.next, rcur->len - rbidx);
      if (next > 0 && dest)
      {
        memcpy (dest, &rcur->data[rbidx], next);
        dest += next;
      }
      rbidx += next;
      state.next -= next;

      if (state.next == 0)
      {
        total_removed++;

        if (str.stride == 1)
        {
          state = (struct stride_state){
              .next   = size,
              .active = true,
          };
        }
        else
        {
          state = (struct stride_state){
              .next   = (str.stride - 1) * size,
              .active = false,
          };
        }
      }
    }
    else
    {
      u32 next = state.next;
      next     = MIN (next,
                      r->cap_per_node - wbidx); // Writable
      next     = MIN (next,
                      rcur->len - rbidx); // Readable

      if (next > 0)
      {
        memmove (&wcur->data[wbidx], &rcur->data[rbidx], next);
      }

      wbidx += next;
      rbidx += next;
      state.next -= next;

      // Write node is full
      if (wbidx == r->cap_per_node)
      {
        wcur->len = r->cap_per_node;
        wcur      = wcur->next;
        ASSERT (wcur == rcur);
        wbidx = 0;
      }

      if (state.next == 0)
      {
        state = (struct stride_state){
            .next   = size,
            .active = true,
        };
      }
    }

    if (rbidx == rcur->len)
    {
      struct block *next = rcur->next;

      if (rcur != wcur)
      {
        // Delete rcur
        if (rcur->prev)
        {
          rcur->prev->next = rcur->next;
        }
        if (rcur->next)
        {
          rcur->next->prev = rcur->prev;
        }

        slab_alloc_free (&r->block_alloc, rcur);
      }

      rcur  = next;
      rbidx = 0;
      if (rcur == NULL)
      {
        break;
      }
    }
  }

  if (rcur != NULL)
  {
    while (true)
    {
      // Basically the same as inactive block
      // except without state
      u32 next = r->cap_per_node - wbidx; // Writable
      next     = MIN (next,
                      rcur->len - rbidx); // Readable

      if (next > 0)
      {
        memmove (&wcur->data[wbidx], &rcur->data[rbidx], next);
      }

      wbidx += next;
      rbidx += next;

      // Write node is full
      if (wbidx == r->cap_per_node)
      {
        wcur->len = r->cap_per_node;
        wcur      = wcur->next;
        ASSERT (wcur == rcur);
        wbidx = 0;
      }

      if (rbidx == rcur->len)
      {
        if (rcur != wcur)
        {
          // Delete
          // rcur
          if (rcur->prev)
          {
            rcur->prev->next = rcur->next;
          }
          if (rcur->next)
          {
            rcur->next->prev = rcur->prev;
          }

          slab_alloc_free (&r->block_alloc, rcur);
        }
        break;
      }
    }
  }

  wcur->len = wbidx;

  return total_removed;
}

u64
block_array_getlen (const struct block_array *r)
{
  u64                 len = 0;
  const struct block *cur = r->head;
  while (cur != NULL)
  {
    len += cur->len;
    cur = cur->next;
  }
  return len;
}

static err_t
block_array_insert_func (
    void       *ctx,
    const u32   ofst,
    const void *src,
    const u32   slen,
    error      *e
)
{
  struct block_array *arr = ctx;
  return block_array_insert (arr, ofst, src, slen, e);
}
static i64
block_array_read_func (
    void               *ctx,
    const struct stride str,
    const u32           size,
    void               *dest,
    error              *e
)
{
  struct block_array *arr = ctx;
  return block_array_read (arr, str, size, dest);
}
static i64
block_array_write_func (
    void               *ctx,
    const struct stride str,
    const u32           size,
    const void         *src,
    error              *e
)
{
  struct block_array *arr = ctx;
  return block_array_write (arr, str, size, src);
}
static i64
block_array_remove_func (
    void               *ctx,
    const struct stride str,
    const u32           size,
    void               *dest,
    error              *e
)
{
  struct block_array *arr = ctx;
  return block_array_remove (arr, str, size, dest, e);
}
static i64
block_array_getlen_func (void *ctx, error *e)
{
  struct block_array *arr = ctx;
  return block_array_getlen (arr);
}

static const struct data_writer_functions block_array_funcs = {
    .insert = block_array_insert_func,
    .read   = block_array_read_func,
    .write  = block_array_write_func,
    .remove = block_array_remove_func,
    .getlen = block_array_getlen_func,
};

void
block_array_data_writer (struct data_writer *dest, struct block_array *arr)
{
  dest->functions = block_array_funcs;
  dest->ctx       = arr;
}

#ifdef TESTING
TEST (block_insert_read)
{
  TEST_CASE ("basic")
  {
    error               e = error_create ();
    struct block_array *b = block_array_create (2, &e);

    u32 src[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    block_array_insert (b, 0, src, sizeof (src), &e);

    u32 dest[arrlen (src)] = {0};
    block_array_read (
        b,
        (struct stride){
            .start  = 1,
            .stride = 2,
            .nelems = 3,
        },
        sizeof (u32),
        dest
    );

    u32 expected[] = {2, 4, 6};

    test_assert_memequal (expected, dest, arrlen (dest));

    block_array_free (b);
  }

  // Read all elements sequentially (stride=1, start=0)
  TEST_CASE ("block_array_read_all")
  {
    error               e = error_create ();
    struct block_array *b = block_array_create (2, &e);

    u32 src[] = {10, 20, 30, 40, 50};
    block_array_insert (b, 0, src, sizeof (src), &e);

    u32 dest[arrlen (src)] = {0};
    block_array_read (
        b,
        (struct stride){
            .start  = 0,
            .stride = 1,
            .nelems = 5,
        },
        sizeof (u32),
        dest
    );

    test_assert_memequal (src, dest, arrlen (dest));
    block_array_free (b);
  }

  // Read a single element from the middle
  TEST_CASE ("block_array_read_single_middle")
  {
    error               e = error_create ();
    struct block_array *b = block_array_create (4, &e);

    u32 src[] = {100, 200, 300, 400};
    block_array_insert (b, 0, src, sizeof (src), &e);

    u32 dest = 0;
    block_array_read (
        b,
        (struct stride){
            .start  = 2,
            .stride = 1,
            .nelems = 1,
        },
        sizeof (u32),
        &dest
    );

    u32 expected = 300;
    test_assert_memequal (&expected, &dest, 1);
    block_array_free (b);
  }

  // Read every 3rd element
  TEST_CASE ("block_array_read_stride3")
  {
    error               e = error_create ();
    struct block_array *b = block_array_create (2, &e);

    u32 src[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    block_array_insert (b, 0, src, sizeof (src), &e);

    u32 dest[3] = {0};
    block_array_read (
        b,
        (struct stride){
            .start  = 0,
            .stride = 3,
            .nelems = 3,
        },
        sizeof (u32),
        dest
    );

    u32 expected[] = {1, 4, 7};
    test_assert_memequal (expected, dest, arrlen (dest));
    block_array_free (b);
  }

  // Insert two disjoint chunks then read across both
  TEST_CASE ("block_array_insert_two_chunks")
  {
    error               e = error_create ();
    struct block_array *b = block_array_create (4, &e);

    u32 first[]  = {1, 2, 3};
    u32 second[] = {4, 5, 6};
    block_array_insert (b, 0, first, sizeof (first), &e);
    block_array_insert (b, sizeof (first), second, sizeof (second), &e);

    u32 dest[6] = {0};
    block_array_read (
        b,
        (struct stride){
            .start  = 0,
            .stride = 1,
            .nelems = 6,
        },
        sizeof (u32),
        dest
    );

    u32 expected[] = {1, 2, 3, 4, 5, 6};
    test_assert_memequal (expected, dest, arrlen (dest));
    block_array_free (b);
  }

  // Insert in the middle of existing data
  TEST_CASE ("block_array_insert_middle")
  {
    error               e = error_create ();
    struct block_array *b = block_array_create (2, &e);

    u32 initial[]  = {1, 3, 4};
    u32 inserted[] = {2};
    block_array_insert (b, 0, initial, sizeof (initial), &e);
    // Insert 2 after the first element (byte offset = sizeof u32)
    block_array_insert (b, sizeof (u32), inserted, sizeof (inserted), &e);

    u32 dest[4] = {0};
    block_array_read (
        b,
        (struct stride){
            .start  = 0,
            .stride = 1,
            .nelems = 4,
        },
        sizeof (u32),
        dest
    );

    u32 expected[] = {1, 2, 3, 4};
    test_assert_memequal (expected, dest, arrlen (dest));
    block_array_free (b);
  }

  // cap_per_node larger than the entire payload — single block case
  TEST_CASE ("block_array_single_block")
  {
    error               e = error_create ();
    struct block_array *b = block_array_create (256, &e);

    u32 src[] = {7, 14, 21};
    block_array_insert (b, 0, src, sizeof (src), &e);

    u32 dest[arrlen (src)] = {0};
    block_array_read (
        b,
        (struct stride){
            .start  = 0,
            .stride = 1,
            .nelems = 3,
        },
        sizeof (u32),
        dest
    );

    test_assert_memequal (src, dest, arrlen (dest));
    block_array_free (b);
  }

  // Read past end returns fewer elements than requested
  TEST_CASE ("block_array_read_past_end")
  {
    error               e = error_create ();
    struct block_array *b = block_array_create (2, &e);

    u32 src[] = {1, 2, 3};
    block_array_insert (b, 0, src, sizeof (src), &e);

    u32 dest[6] = {0};
    i64 nread   = block_array_read (
        b,
        (struct stride){
            .start  = 1,
            .stride = 1,
            .nelems = 6,
        },
        sizeof (u32),
        dest
    );

    // Only 2 elements remain after start=1
    test_assert (nread == 2);
    u32 expected[] = {2, 3};
    test_assert_memequal (expected, dest, 2);
    block_array_free (b);
  }

  // Randomised insert then full sequential read-back
  TEST_CASE ("block_array_random")
  {
    error e = error_create ();

    srand (12345);

    for (int trial = 0; trial < 64; trial++)
    {
      // cap_per_node: 1–15 bytes (forces many
      // different block-boundary patterns)
      u32                 cap = (rand () % 15) + 1;
      struct block_array *b   = block_array_create (cap, &e);

      // number of u32 elements: 1–127
      u32 nelems = (rand () % 127) + 1;
      u32 src[128];
      for (u32 i = 0; i < nelems; i++)
      {
        src[i] = (u32)rand ();
      }

      block_array_insert (b, 0, src, nelems * sizeof (u32), &e);

      u32 dest[128] = {0};
      i64 nread     = block_array_read (
          b,
          (struct stride){
              .start  = 0,
              .stride = 1,
              .nelems = nelems,
          },
          sizeof (u32),
          dest
      );

      test_assert (nread == (i64)nelems);
      test_assert_memequal (src, dest, nelems);

      block_array_free (b);
    }
  }
}

TEST (block_insert_remove_read)
{
  TEST_CASE ("remove_from_middle")
  {
    error               e = error_create ();
    struct block_array *b = block_array_create (4, &e);

    const u32 src[] = {1, 2, 3, 4, 5};
    block_array_insert (b, 0, src, sizeof (src), &e);

    // Remove the middle element (index 2)
    u32       removed = 0;
    const i64 n       = block_array_remove (
        b,
        (struct stride){.start = 2, .stride = 1, .nelems = 1},
        sizeof (u32),
        &removed,
        &e
    );

    test_assert (n == 1);
    u32 expected_removed = 3;
    test_assert_memequal (&expected_removed, &removed, 1);

    u32 dest[4] = {0};
    block_array_read (
        b,
        (struct stride){.start = 0, .stride = 1, .nelems = 4},
        sizeof (u32),
        dest
    );

    const u32 expected[] = {1, 2, 4, 5};
    test_assert_memequal (expected, dest, arrlen (expected));

    block_array_free (b);
  }

  TEST_CASE ("remove_first")
  {
    error               e = error_create ();
    struct block_array *b = block_array_create (2, &e);

    const u32 src[] = {10, 20, 30};
    block_array_insert (b, 0, src, sizeof (src), &e);

    u32       removed = 0;
    const i64 n       = block_array_remove (
        b,
        (struct stride){.start = 0, .stride = 1, .nelems = 1},
        sizeof (u32),
        &removed,
        &e
    );

    test_assert (n == 1);
    u32 expected_removed = 10;
    test_assert_memequal (&expected_removed, &removed, 1);

    u32 dest[2] = {0};
    block_array_read (
        b,
        (struct stride){.start = 0, .stride = 1, .nelems = 2},
        sizeof (u32),
        dest
    );

    const u32 expected[] = {20, 30};
    test_assert_memequal (expected, dest, arrlen (expected));

    block_array_free (b);
  }

  TEST_CASE ("remove_last")
  {
    error               e = error_create ();
    struct block_array *b = block_array_create (2, &e);

    const u32 src[] = {10, 20, 30};
    block_array_insert (b, 0, src, sizeof (src), &e);

    u32       removed = 0;
    const i64 n       = block_array_remove (
        b,
        (struct stride){.start = 2, .stride = 1, .nelems = 1},
        sizeof (u32),
        &removed,
        &e
    );

    test_assert (n == 1);
    u32 expected_removed = 30;
    test_assert_memequal (&expected_removed, &removed, 1);

    u32 dest[2] = {0};
    block_array_read (
        b,
        (struct stride){.start = 0, .stride = 1, .nelems = 2},
        sizeof (u32),
        dest
    );

    const u32 expected[] = {10, 20};
    test_assert_memequal (expected, dest, arrlen (expected));

    block_array_free (b);
  }

  TEST_CASE ("remove_strided")
  {
    error               e = error_create ();
    struct block_array *b = block_array_create (2, &e);

    // Remove every other element: 1, 3, 5 — leaving 2, 4, 6
    const u32 src[] = {1, 2, 3, 4, 5, 6};
    block_array_insert (b, 0, src, sizeof (src), &e);

    u32       removed[3] = {0};
    const i64 n          = block_array_remove (
        b,
        (struct stride){.start = 0, .stride = 2, .nelems = 3},
        sizeof (u32),
        removed,
        &e
    );

    test_assert (n == 3);
    const u32 expected_removed[] = {1, 3, 5};
    test_assert_memequal (expected_removed, removed, arrlen (expected_removed));

    u32 dest[3] = {0};
    block_array_read (
        b,
        (struct stride){.start = 0, .stride = 1, .nelems = 3},
        sizeof (u32),
        dest
    );

    const u32 expected[] = {2, 4, 6};
    test_assert_memequal (expected, dest, arrlen (expected));

    block_array_free (b);
  }
}

TEST (block_insert_write_read)
{
  TEST_CASE ("overwrite_middle")
  {
    error               e = error_create ();
    struct block_array *b = block_array_create (4, &e);

    const u32 src[] = {1, 2, 3, 4, 5};
    block_array_insert (b, 0, src, sizeof (src), &e);

    const u32 patch[] = {99};
    const i64 n       = block_array_write (
        b,
        (struct stride){
            .start  = 2,
            .stride = 1,
            .nelems = 1,
        },
        sizeof (u32),
        patch
    );

    test_assert (n == 1);

    u32 dest[5] = {0};
    block_array_read (
        b,
        (struct stride){
            .start  = 0,
            .stride = 1,
            .nelems = 5,
        },
        sizeof (u32),
        dest
    );

    const u32 expected[] = {1, 2, 99, 4, 5};
    test_assert_memequal (expected, dest, arrlen (expected));

    block_array_free (b);
  }

  TEST_CASE ("overwrite_all")
  {
    error               e = error_create ();
    struct block_array *b = block_array_create (2, &e);

    const u32 src[] = {0, 0, 0, 0};
    block_array_insert (b, 0, src, sizeof (src), &e);

    const u32 patch[] = {10, 20, 30, 40};
    const i64 n       = block_array_write (
        b,
        (struct stride){
            .start  = 0,
            .stride = 1,
            .nelems = 4,
        },
        sizeof (u32),
        patch
    );

    test_assert (n == 4);

    u32 dest[4] = {0};
    block_array_read (
        b,
        (struct stride){
            .start  = 0,
            .stride = 1,
            .nelems = 4,
        },
        sizeof (u32),
        dest
    );

    test_assert_memequal (patch, dest, arrlen (patch));

    block_array_free (b);
  }

  TEST_CASE ("overwrite_strided")
  {
    error               e = error_create ();
    struct block_array *b = block_array_create (2, &e);

    const u32 src[] = {1, 2, 3, 4, 5, 6};
    block_array_insert (b, 0, src, sizeof (src), &e);

    // Overwrite even indices (0, 2, 4) with 0
    const u32 patch[] = {0, 0, 0};
    const i64 n       = block_array_write (
        b,
        (struct stride){
            .start  = 0,
            .stride = 2,
            .nelems = 3,
        },
        sizeof (u32),
        patch
    );

    test_assert (n == 3);

    u32 dest[6] = {0};
    block_array_read (
        b,
        (struct stride){
            .start  = 0,
            .stride = 1,
            .nelems = 6,
        },
        sizeof (u32),
        dest
    );

    const u32 expected[] = {0, 2, 0, 4, 0, 6};
    test_assert_memequal (expected, dest, arrlen (expected));

    block_array_free (b);
  }

  TEST_CASE ("overwrite_last")
  {
    error               e = error_create ();
    struct block_array *b = block_array_create (2, &e);

    const u32 src[] = {1, 2, 3};
    block_array_insert (b, 0, src, sizeof (src), &e);

    const u32 patch[] = {42};
    const i64 n       = block_array_write (
        b,
        (struct stride){
            .start  = 2,
            .stride = 1,
            .nelems = 1,
        },
        sizeof (u32),
        patch
    );

    test_assert (n == 1);

    u32 dest[3] = {0};
    block_array_read (
        b,
        (struct stride){
            .start  = 0,
            .stride = 1,
            .nelems = 3,
        },
        sizeof (u32),
        dest
    );

    const u32 expected[] = {1, 2, 42};
    test_assert_memequal (expected, dest, arrlen (expected));

    block_array_free (b);
  }
}

TEST (block_random)
{
  error e = error_create ();

  // Block sizes to test
  const u32 sizes[] = {1, 2, 3, 4, 5, 10, 100, 500, 1000, 5000, 10000};
  const u32 niters[] =
      {100, 100, 100, 100, 100, 100, 1000, 1000, 1000, 1000, 10000};

  for (u32 i = 0; i < arrlen (sizes); ++i)
  {
    i_log_info ("Block random test: %d\n", i);

    struct ext_array    ext_arr   = ext_array_create ();
    struct block_array *block_arr = block_array_create (sizes[i], &e);
    struct data_writer  ref;
    struct data_writer  sut;

    ext_array_data_writer (&ref, &ext_arr);
    block_array_data_writer (&sut, block_arr);

    struct dvalidtr d = {
        .sut     = sut,
        .ref     = ref,
        .isvalid = NULL,
    };

    dvalidtr_random_test (&d, 1, niters[i], 1000, &e);

    ext_array_free (&ext_arr);
    block_array_free (block_arr);
  }
}
#endif

/******************************************************************************
 * SECTION: Byte Accessor
 ******************************************************************************/

static u32
ba_memcpy_from_recursive (u8 *dest, const u8 *src, struct byte_accessor *acc)
{
  switch (acc->type)
  {
    case TA_TAKE:
    {
      memcpy (dest, src, acc->src_size);
      return acc->src_size;
    }
    case TA_SELECT:
    {
      return ba_memcpy_from_recursive (
          dest,
          src + acc->select.bofst,
          acc->select.sub_ba
      );
    }
    case TA_RANGE:
    {
      u32 elem_size = acc->range.sub_ba->src_size;
      u32 pos       = acc->range.stride.start;
      u32 written   = 0;
      u32 i         = 0;

      while (i < acc->range.stride.nelems)
      {
        written += ba_memcpy_from_recursive (
            dest + written,
            src + pos * elem_size,
            acc->range.sub_ba
        );

        pos += acc->range.stride.stride;
        i++;
      }

      return written;
    }
  }
  UNREACHABLE ();
}

u32
ba_memcpy_from (u8 *dest, const u8 *src, struct byte_accessor *acc)
{
  return ba_memcpy_from_recursive (dest, src, acc);
}

#ifdef TESTING
TEST (ba_memcpy_from_basic)
{
  // struct { a int, b struct { b char, c [5]u16 } }
  u8 src[64];
  u8 dest[64];

  u8 test_data[] = {
      78,
      56,
      34,
      12,   // int a (little-endian)
      0xAB, // char b
      1,
      0, // u16 c[0] = 1
      2,
      0, // u16 c[1] = 2
      3,
      0, // u16 c[2] = 3
      4,
      0, // u16 c[3] = 4
      5,
      0, // u16 c[4] = 5
  };

  memcpy (src, test_data, sizeof (test_data));

  TEST_CASE ("[.a]")
  {
    struct byte_accessor acc = {
        .type      = TA_SELECT,
        .src_size  = 15,
        .dest_size = 4,
        .select    = {
            .bofst  = 0,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_TAKE,
                .src_size  = 4,
                .dest_size = 4,
            },
        },
    };

    u32 moved = ba_memcpy_from (dest, src, &acc);
    test_assert_int_equal (moved, 4);

    test_assert_int_equal (dest[0], 78);
    test_assert_int_equal (dest[1], 56);
    test_assert_int_equal (dest[2], 34);
    test_assert_int_equal (dest[3], 12);
  }

  TEST_CASE ("[.b]")
  {
    struct byte_accessor acc = {
        .type      = TA_SELECT,
        .src_size  = 15,
        .dest_size = 1,
        .select    = {
            .bofst  = 4,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_TAKE,
                .src_size  = 1,
                .dest_size = 1,
            },
        },
    };

    u32 moved = ba_memcpy_from (dest, src, &acc);
    test_assert_int_equal (moved, 1);

    test_assert_int_equal (dest[0], 0xAB);
  }

  TEST_CASE ("[.b, .a]")
  {
    struct byte_accessor dotb = {
        .type      = TA_SELECT,
        .src_size  = 15,
        .dest_size = 1,
        .select    = {
            .bofst  = 4,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_TAKE,
                .src_size  = 1,
                .dest_size = 1,
            },
        },
    };
    struct byte_accessor dota = {
        .type      = TA_SELECT,
        .src_size  = 15,
        .dest_size = 4,
        .select    = {
            .bofst  = 0,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_TAKE,
                .src_size  = 4,
                .dest_size = 4,
            },
        },
    };

    u32 moved = ba_memcpy_from (dest, src, &dotb);
    test_assert_int_equal (moved, 1);
    moved = ba_memcpy_from (dest + moved, src, &dota);
    test_assert_int_equal (moved, 4);

    test_assert_int_equal (dest[0], 0xAB);
    test_assert_int_equal (dest[1], 78);
    test_assert_int_equal (dest[2], 56);
    test_assert_int_equal (dest[3], 34);
    test_assert_int_equal (dest[4], 12);
  }

  TEST_CASE ("[.a, .b]")
  {
    struct byte_accessor dota = {
        .type      = TA_SELECT,
        .src_size  = 15,
        .dest_size = 4,
        .select    = {
            .bofst  = 0,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_TAKE,
                .src_size  = 4,
                .dest_size = 4,
            },
        },
    };
    struct byte_accessor dotb = {
        .type      = TA_SELECT,
        .src_size  = 15,
        .dest_size = 1,
        .select    = {
            .bofst  = 4,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_TAKE,
                .src_size  = 1,
                .dest_size = 1,
            },
        },
    };

    u32 moved = ba_memcpy_from (dest, src, &dota);
    test_assert_int_equal (moved, 4);
    moved = ba_memcpy_from (dest + moved, src, &dotb);
    test_assert_int_equal (moved, 1);

    test_assert_int_equal (dest[0], 78);
    test_assert_int_equal (dest[1], 56);
    test_assert_int_equal (dest[2], 34);
    test_assert_int_equal (dest[3], 12);
    test_assert_int_equal (dest[4], 0xAB);
  }

  TEST_CASE ("[.b.c[1:1:4]]")
  {
    // SELECT(4, SELECT(1, RANGE(start=1, stride=1, nelems=4,
    // TAKE(2)))) Gathers c[1], c[2], c[3] → 6 bytes
    struct byte_accessor acc = {
        .type      = TA_SELECT,
        .src_size  = 15,
        .dest_size = 6,
        .select    = {
            .bofst  = 4,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_SELECT,
                .src_size  = 11,
                .dest_size = 6,
                .select    = {
                    .bofst  = 1,
                    .sub_ba = &(struct byte_accessor){
                        .type      = TA_RANGE,
                        .src_size  = 10,
                        .dest_size = 8,
                        .range     = {
                            .sub_ba =
                                &(struct byte_accessor){
                                    .type      = TA_TAKE,
                                    .src_size  = 2,
                                    .dest_size = 2,
                                },
                            .stride = (struct stride){
                                .start  = 1,
                                .stride = 1,
                                .nelems = 3,
                            },
                        },
                    },
                },
            },
        },
    };

    u32 moved = ba_memcpy_from (dest, src, &acc);
    test_assert_int_equal (moved, 6);

    test_assert_int_equal (dest[0], 2); // c[1] = 2
    test_assert_int_equal (dest[1], 0);
    test_assert_int_equal (dest[2], 3); // c[2] = 3
    test_assert_int_equal (dest[3], 0);
    test_assert_int_equal (dest[4], 4); // c[3] = 4
    test_assert_int_equal (dest[5], 0);
  }

  TEST_CASE ("[.b.c[0:2:5]]")
  {
    // SELECT(4, SELECT(1, RANGE(start=0, stride=2, nelems=5,
    // TAKE(2)))) Gathers c[0], c[2], c[4] → 6 bytes
    struct byte_accessor acc = {
        .type      = TA_SELECT,
        .src_size  = 15,
        .dest_size = 6,
        .select    = {
            .bofst  = 4,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_SELECT,
                .src_size  = 11,
                .dest_size = 6,
                .select    = {
                    .bofst  = 1,
                    .sub_ba = &(struct byte_accessor){
                        .type      = TA_RANGE,
                        .src_size  = 10,
                        .dest_size = 6,
                        .range     = {
                            .sub_ba =
                                &(struct byte_accessor){
                                    .type      = TA_TAKE,
                                    .src_size  = 2,
                                    .dest_size = 2,
                                },
                            .stride = (struct stride){
                                .start  = 0,
                                .stride = 2,
                                .nelems = 3,
                            },
                        },
                    },
                },
            },
        },
    };

    u32 moved = ba_memcpy_from (dest, src, &acc);
    test_assert_int_equal (moved, 6);

    test_assert_int_equal (dest[0], 1); // c[0] = 1
    test_assert_int_equal (dest[1], 0);
    test_assert_int_equal (dest[2], 3); // c[2] = 3
    test_assert_int_equal (dest[3], 0);
    test_assert_int_equal (dest[4], 5); // c[4] = 5
    test_assert_int_equal (dest[5], 0);
  }

  TEST_CASE ("[.a, .a]")
  {
    u8 seq[4] = {0, 1, 2, 3};
    memcpy (src, seq, 4);

    struct byte_accessor dota = {
        .type      = TA_TAKE,
        .src_size  = 4,
        .dest_size = 4,
    };

    u32 moved = ba_memcpy_from (dest, src, &dota);
    test_assert_int_equal (moved, 4);
    moved = ba_memcpy_from (dest + moved, src, &dota);
    test_assert_int_equal (moved, 4);

    test_assert_int_equal (dest[0], 0);
    test_assert_int_equal (dest[1], 1);
    test_assert_int_equal (dest[2], 2);
    test_assert_int_equal (dest[3], 3);
    test_assert_int_equal (dest[4], 0);
    test_assert_int_equal (dest[5], 1);
    test_assert_int_equal (dest[6], 2);
    test_assert_int_equal (dest[7], 3);
  }
}
#endif

static u32
ba_memcpy_to_recursive (u8 *dest, const u8 *src, struct byte_accessor *acc)
{
  switch (acc->type)
  {
    case TA_TAKE:
    {
      memcpy (dest, src, acc->src_size);
      return acc->src_size;
    }
    case TA_SELECT:
    {
      return ba_memcpy_to_recursive (
          dest + acc->select.bofst,
          src,
          acc->select.sub_ba
      );
    }
    case TA_RANGE:
    {
      u32 elem_size = acc->range.sub_ba->src_size;
      u32 pos       = acc->range.stride.start;
      u32 read      = 0;

      while (pos < acc->range.stride.nelems)
      {
        read += ba_memcpy_to_recursive (
            dest + pos * elem_size,
            src + read,
            acc->range.sub_ba
        );
        pos += acc->range.stride.stride;
      }

      return read;
    }
  }
  UNREACHABLE ();
}

u32
ba_memcpy_to (u8 *dest, const u8 *src, struct byte_accessor *acc)
{
  return ba_memcpy_to_recursive (dest, src, acc);
}

#ifdef TESTING
TEST (ba_memcpy_to_basic)
{
  // struct { a int, b struct { b char, c [5]u16 } }
  u8 src[64];
  u8 dest[64];

  TEST_CASE ("[.a]")
  {
    u8 test_data[] = {78, 56, 34, 12};
    memcpy (src, test_data, sizeof (test_data));

    struct byte_accessor acc = {
        .type      = TA_SELECT,
        .src_size  = 4,
        .dest_size = 4,
        .select    = {
            .bofst  = 0,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_TAKE,
                .src_size  = 4,
                .dest_size = 4,
            },
        },
    };

    ba_memcpy_to (dest, src, &acc);

    test_assert_int_equal (dest[0], 78);
    test_assert_int_equal (dest[1], 56);
    test_assert_int_equal (dest[2], 34);
    test_assert_int_equal (dest[3], 12);
  }

  TEST_CASE ("[.b]")
  {
    u8 test_data[] = {0xAB};
    memcpy (src, test_data, sizeof (test_data));

    struct byte_accessor acc = {
        .type      = TA_SELECT,
        .src_size  = 1,
        .dest_size = 1,
        .select    = {
            .bofst  = 4,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_TAKE,
                .src_size  = 1,
                .dest_size = 1,
            },
        },
    };

    ba_memcpy_to (dest, src, &acc);

    test_assert_int_equal (dest[4], 0xAB);
  }

  TEST_CASE ("[.b, .a]")
  {
    u8 test_data[] = {
        0xAB, // .b
        78,
        56,
        34,
        12 // .a
    };
    memcpy (src, test_data, sizeof (test_data));

    struct byte_accessor dotb = {
        .type      = TA_SELECT,
        .src_size  = 1,
        .dest_size = 1,
        .select    = {
            .bofst  = 4,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_TAKE,
                .src_size  = 1,
                .dest_size = 1,
            },
        },
    };
    struct byte_accessor dota = {
        .type      = TA_SELECT,
        .src_size  = 4,
        .dest_size = 4,
        .select    = {
            .bofst  = 0,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_TAKE,
                .src_size  = 4,
                .dest_size = 4,
            },
        },
    };

    u32 moved = ba_memcpy_to (dest, src, &dotb); // 0xAB → dest[4]
    test_assert_int_equal (moved, 1);
    moved = ba_memcpy_to (
        dest,
        src + moved,
        &dota
    ); // {78,56,34,12} → dest[0..3]
    test_assert_int_equal (moved, 4);

    test_assert_int_equal (dest[0], 78);
    test_assert_int_equal (dest[1], 56);
    test_assert_int_equal (dest[2], 34);
    test_assert_int_equal (dest[3], 12);
    test_assert_int_equal (dest[4], 0xAB);
  }

  TEST_CASE ("[.a, .b]")
  {
    u8 test_data[] = {
        78,
        56,
        34,
        12,  // .a
        0xAB // .b
    };
    memcpy (src, test_data, sizeof (test_data));

    struct byte_accessor dota = {
        .type      = TA_SELECT,
        .src_size  = 4,
        .dest_size = 4,
        .select    = {
            .bofst  = 0,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_TAKE,
                .src_size  = 4,
                .dest_size = 4,
            },
        },
    };
    struct byte_accessor dotb = {
        .type      = TA_SELECT,
        .src_size  = 1,
        .dest_size = 1,
        .select    = {
            .bofst  = 4,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_TAKE,
                .src_size  = 1,
                .dest_size = 1,
            },
        },
    };

    u32 moved = ba_memcpy_to (dest, src, &dota);
    test_assert_int_equal (moved, 4);
    moved = ba_memcpy_to (dest, src + moved, &dotb);
    test_assert_int_equal (moved, 1);

    test_assert_int_equal (dest[0], 78);
    test_assert_int_equal (dest[1], 56);
    test_assert_int_equal (dest[2], 34);
    test_assert_int_equal (dest[3], 12);
    test_assert_int_equal (dest[4], 0xAB);
  }

  TEST_CASE ("[.b.c[1:1:4]]")
  {
    u8 test_data[] = {
        2,
        0, // c[1] = 2
        3,
        0, // c[2] = 3
        4,
        0, // c[3] = 4
    };
    memcpy (src, test_data, sizeof (test_data));

    // SELECT(bofst=4, SELECT(bofst=1, RANGE(start=1, stride=1,
    // nelems=4, TAKE(2))))
    struct byte_accessor acc = {
        .type      = TA_SELECT,
        .src_size  = 6,
        .dest_size = 6,
        .select    = {
            .bofst  = 4,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_SELECT,
                .src_size  = 6,
                .dest_size = 6,
                .select    = {
                    .bofst  = 1,
                    .sub_ba = &(struct byte_accessor){
                        .type      = TA_RANGE,
                        .src_size  = 6,
                        .dest_size = 6,
                        .range     = {
                            .sub_ba =
                                &(struct byte_accessor){
                                    .type      = TA_TAKE,
                                    .src_size  = 2,
                                    .dest_size = 2,
                                },
                            .stride = (struct stride){
                                .start  = 1,
                                .stride = 1,
                                .nelems = 4,
                            },
                        },
                    },
                },
            },
        },
    };

    u32 moved = ba_memcpy_to (dest, src, &acc);
    test_assert_int_equal (moved, 6);

    test_assert_int_equal (dest[7], 2); // c[1]
    test_assert_int_equal (dest[8], 0);
    test_assert_int_equal (dest[9], 3); // c[2]
    test_assert_int_equal (dest[10], 0);
    test_assert_int_equal (dest[11], 4); // c[3]
    test_assert_int_equal (dest[12], 0);
  }

  TEST_CASE ("[.b.c[0:2:5]]")
  {
    // Compact src: c[0], c[2], c[4] in linear order
    u8 test_data[] = {
        1,
        0, // c[0] = 1
        3,
        0, // c[2] = 3
        5,
        0, // c[4] = 5
    };
    memcpy (src, test_data, sizeof (test_data));

    // SELECT(bofst=4, SELECT(bofst=1, RANGE(start=0, stride=2,
    // nelems=5, TAKE(2)))) dest offsets: 4+1+(0*2)=5, 4+1+(2*2)=9,
    // 4+1+(4*2)=13
    struct byte_accessor acc = {
        .type      = TA_SELECT,
        .src_size  = 6,
        .dest_size = 6,
        .select    = {
            .bofst  = 4,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_SELECT,
                .src_size  = 6,
                .dest_size = 6,
                .select    = {
                    .bofst  = 1,
                    .sub_ba = &(struct byte_accessor){
                        .type      = TA_RANGE,
                        .src_size  = 6,
                        .dest_size = 6,
                        .range     = {
                            .sub_ba =
                                &(struct byte_accessor){
                                    .type      = TA_TAKE,
                                    .src_size  = 2,
                                    .dest_size = 2,
                                },
                            .stride = (struct stride){
                                .start  = 0,
                                .stride = 2,
                                .nelems = 5,
                            },
                        },
                    },
                },
            },
        },
    };

    u32 moved = ba_memcpy_to (dest, src, &acc);
    test_assert_int_equal (moved, 6);

    test_assert_int_equal (dest[5], 1); // c[0]
    test_assert_int_equal (dest[6], 0);
    test_assert_int_equal (dest[9], 3); // c[2]
    test_assert_int_equal (dest[10], 0);
    test_assert_int_equal (dest[13], 5); // c[4]
    test_assert_int_equal (dest[14], 0);
  }

  TEST_CASE ("[.a, .a]")
  {
    u8 test_data[] = {4, 5, 6, 7};
    memcpy (src, test_data, sizeof (test_data));

    struct byte_accessor dota = {
        .type   = TA_SELECT,
        .select = {
            .bofst  = 0,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_TAKE,
                .src_size  = 4,
                .dest_size = 4,
            },
        },
    };

    u32 moved = ba_memcpy_to (dest, src, &dota);
    test_assert_int_equal (moved, 4);
    moved = ba_memcpy_to (dest, src, &dota);
    test_assert_int_equal (moved, 4);

    test_assert_int_equal (dest[0], 4);
    test_assert_int_equal (dest[1], 5);
    test_assert_int_equal (dest[2], 6);
    test_assert_int_equal (dest[3], 7);
  }
}
#endif
