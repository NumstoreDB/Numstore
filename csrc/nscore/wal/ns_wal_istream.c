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

#include "nscore/wal/ns_wal_istream.h"

#include "core/ns_crc.h"
#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_latch.h"
#include "core/ns_stdtypes.h"
#include "core/ns_testing.h"
#include "core/os/ns_malloc.h"
#include "core/os/ns_os.h"

#include <stdbool.h>
#include <stddef.h>

DEFINE_DBG_ASSERT (struct wal_istream, wal_istream, w, {
  ASSERT (w);
  DBG_ASSERT (i_mem, &w->mem);
})

struct wal_istream *
walis_open (const char *fname, struct i_mem mem, struct i_os os, error *e)
{
  // Allocate memory
  struct wal_istream *dest = mem.table->malloc (mem.self, 1, sizeof *dest, e);
  if (dest == NULL) {
    return NULL;
  }

  // Open file in read only mode
  if (os.table->open_file_r (os.self, &dest->fd, fname, e)) {
    goto err_free;
  }

  // Seek to the start of the file
  if (dest->fd.table->seek (dest->fd.self, 0, I_SEEK_SET, e) < 0) {
    goto err_close;
  }

  dest->mem    = mem;
  dest->os     = os;
  dest->curlsn = 0;
  dest->lsnidx = 0;
  latch_init (&dest->l);

  DBG_ASSERT (wal_istream, dest);

  return dest;

err_close:
  os.table->close_file (os.self, dest->fd.self, e);
err_free:
  mem.table->free (mem.self, dest);
  return NULL;
}

#ifndef NDEBUG

/// Writes [len] bytes of [src] as the whole contents of the file [fname]
static void
walis_test_write (struct test_ctx *ctx, const char *fname, const void *src, const u32 len)
{
  ctx->os.table->remove_quiet (ctx->os.self, fname, &ctx->e);

  i_file f;
  test_assert_int_equal (ctx->os.table->open_file_w (ctx->os.self, &f, fname, &ctx->e), SUCCESS);
  test_assert_int_equal (f.table->write_all (f.self, src, len, &ctx->e), SUCCESS);
  test_assert_int_equal (ctx->os.table->close_file (ctx->os.self, f.self, &ctx->e), SUCCESS);
}

TEST (walis_open)
{
  TEST_CASE ("happy path")
  {
    ctx->os.table->remove_quiet (ctx->os.self, "foo", &ctx->e);
    i_file fp = {0};
    ctx->os.table->open_file_w (ctx->os.self, &fp, "foo", &ctx->e);
    ctx->os.table->close_file (ctx->os.self, fp.self, &ctx->e);

    struct wal_istream *wis = walis_open ("foo", ctx->mem, ctx->os, &ctx->e);
    test_assert (wis != NULL);
    walis_close (wis, &ctx->e);
    ctx->os.table->remove_quiet (ctx->os.self, "foo", &ctx->e);
  }

  TEST_CASE ("Flaky malloc")
  {
    int   fail_after = 0;
    err_t ret;

    do {
      TEST_CASE ("Failure flaky malloc. Fail after: %d", fail_after)
      {
        // Setup
        const u8 data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
        walis_test_write (ctx, "foo", data, sizeof data);

        // Do test
        faulty_mem_fail_after (ctx->faulty_mem, fail_after);
        struct wal_istream *wis = walis_open ("foo", ctx->faulty_mem, ctx->os, &ctx->e);
        ret                     = error_trace (&ctx->e);
        if (ret != SUCCESS) {
          test_assert (wis == NULL);
        }
        faulty_mem_reset (ctx->faulty_mem);
        error_reset (&ctx->e);

        // Reset
        if (wis) {
          walis_close (wis, &ctx->e);
        }
        ctx->os.table->remove_quiet (ctx->os.self, "foo", &ctx->e);

        // Next fail after
        fail_after++;
      }
    }
    while (ret != SUCCESS);
  }

  TEST_CASE ("Flaky os")
  {
    int   fail_after = 0;
    err_t ret;

    do {
      TEST_CASE ("Failure flaky os. Fail after: %d", fail_after)
      {
        // Setup
        const u8 data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
        walis_test_write (ctx, "foo", data, sizeof data);

        // Do test
        faulty_os_fail_after (ctx->faulty_os, fail_after);
        struct wal_istream *wis = walis_open ("foo", ctx->mem, ctx->faulty_os, &ctx->e);
        ret                     = error_trace (&ctx->e);
        if (ret != SUCCESS) {
          test_assert (wis == NULL);
        }
        faulty_os_reset (ctx->faulty_os);
        error_reset (&ctx->e);

        // Reset
        if (wis) {
          walis_close (wis, &ctx->e);
        }
        ctx->os.table->remove_quiet (ctx->os.self, "foo", &ctx->e);

        // Next fail after
        fail_after++;
      }
    }
    while (ret != SUCCESS);
  }
}
#endif

err_t
walis_close (struct wal_istream *w, error *e)
{
  latch_lock (&w->l);

  DBG_ASSERT (wal_istream, w);
  w->os.table->close_file (w->os.self, w->fd.self, e);
  w->mem.table->free (w->mem.self, w);

  return error_trace (e);
}

#ifndef NDEBUG
TEST (walis_close)
{
  const u8 payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};

  TEST_CASE ("happy path")
  {
    walis_test_write (ctx, "foo", payload, sizeof payload);

    struct wal_istream *wis = walis_open ("foo", ctx->mem, ctx->os, &ctx->e);
    test_assert (wis != NULL);
    test_assert_int_equal (walis_close (wis, &ctx->e), SUCCESS);

    ctx->os.table->remove_quiet (ctx->os.self, "foo", &ctx->e);
  }

  TEST_CASE ("Flaky os")
  {
    int   fail_after = 0;
    err_t ret;

    do {
      TEST_CASE ("Failure flaky os. Fail after: %d", fail_after)
      {
        // Setup
        walis_test_write (ctx, "foo", payload, sizeof payload);
        struct wal_istream *wis = walis_open ("foo", ctx->mem, ctx->faulty_os, &ctx->e);
        test_assert (wis != NULL);

        // Do test (the stream is freed regardless of the error)
        faulty_os_fail_after (ctx->faulty_os, fail_after);
        walis_close (wis, &ctx->e);
        ret = error_trace (&ctx->e);

        // Clean up
        faulty_os_reset (ctx->faulty_os);
        error_reset (&ctx->e);
        ctx->os.table->remove_quiet (ctx->os.self, "foo", &ctx->e);

        // Next fail after
        fail_after++;
      }
    }
    while (ret != SUCCESS);
  }
}
#endif

static err_t
walis_seek_unsafe (struct wal_istream *w, const lsn pos, error *e)
{
  DBG_ASSERT (wal_istream, w);

  const i64 res = w->fd.table->seek (w->fd.self, pos, I_SEEK_SET, e);
  if (res < 0) {
    return error_trace (e);
  }

  if ((u64)res != pos) {
    return error_causef (e, ERR_CORRUPT, "seek to invalid offset");
  }

  w->curlsn = pos;
  w->lsnidx = 0;

  return SUCCESS;
}

#ifndef NDEBUG
TEST (walis_seek_unsafe)
{
  const u8 payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};

  TEST_CASE ("happy path")
  {
    walis_test_write (ctx, "foo", payload, sizeof payload);
    struct wal_istream *wis = walis_open ("foo", ctx->mem, ctx->os, &ctx->e);
    test_assert (wis != NULL);

    // Consume some bytes so lsnidx is non zero
    u8 buf[4] = {0};
    walis_mark (wis);
    test_assert_int_equal (walis_read_all (wis, NULL, buf, sizeof buf, &ctx->e), SUCCESS);
    test_assert_int_equal ((i32)wis->lsnidx, 4);

    // Seek resets the position bookkeeping
    test_assert_int_equal (walis_seek_unsafe (wis, 2, &ctx->e), SUCCESS);
    test_assert_int_equal ((i32)wis->curlsn, 2);
    test_assert_int_equal ((i32)wis->lsnidx, 0);

    // The file position really moved
    u8 two[2] = {0};
    test_assert_int_equal (walis_read_all (wis, NULL, two, sizeof two, &ctx->e), SUCCESS);
    test_assert_memequal (two, payload + 2, sizeof two);

    walis_close (wis, &ctx->e);
    ctx->os.table->remove_quiet (ctx->os.self, "foo", &ctx->e);
  }

  TEST_CASE ("Flaky os")
  {
    int   fail_after = 0;
    err_t ret;

    do {
      TEST_CASE ("Failure flaky os. Fail after: %d", fail_after)
      {
        // Setup
        walis_test_write (ctx, "foo", payload, sizeof payload);
        struct wal_istream *wis = walis_open ("foo", ctx->mem, ctx->faulty_os, &ctx->e);
        test_assert (wis != NULL);

        // Do test
        faulty_os_fail_after (ctx->faulty_os, fail_after);
        walis_seek_unsafe (wis, 4, &ctx->e);
        ret = error_trace (&ctx->e);

        // Clean up
        faulty_os_reset (ctx->faulty_os);
        error_reset (&ctx->e);
        walis_close (wis, &ctx->e);
        ctx->os.table->remove_quiet (ctx->os.self, "foo", &ctx->e);

        // Next fail after
        fail_after++;
      }
    }
    while (ret != SUCCESS);
  }
}
#endif

int
walis_read_all (struct wal_istream *w, u32 *checksum, void *data, const u32 len, error *e)
{
  latch_lock (&w->l);

  DBG_ASSERT (wal_istream, w);

  // Read the expected record
  const i64 bread = w->fd.table->read_all (w->fd.self, data, len, e);
  if (bread < 0) {
    latch_unlock (&w->l);
    return error_trace (e);
  }

  // Didn't read everything - record is incomplete
  if (bread < len) {
    // Seek back so the file position is at the record start,
    // leaving it at the last fully-written record boundary
    if (bread > 0) {
      if (walis_seek_unsafe (w, w->curlsn, e)) {
        latch_unlock (&w->l);
        return error_trace (e);
      }
    }

    latch_unlock (&w->l);

    return WAL_EOF;
  }

  if (checksum) {
    checksum_execute (checksum, data, len);
  }

  w->lsnidx += len;

  latch_unlock (&w->l);

  return SUCCESS;
}

#ifndef NDEBUG
TEST (walis_read_all)
{
  const u8 payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};

  TEST_CASE ("happy path")
  {
    walis_test_write (ctx, "foo", payload, sizeof payload);
    struct wal_istream *wis = walis_open ("foo", ctx->mem, ctx->os, &ctx->e);
    test_assert (wis != NULL);

    u32 checksum = checksum_init ();
    u8  buf[4]   = {0};

    // Two full records back to back
    test_assert_int_equal (walis_read_all (wis, &checksum, buf, sizeof buf, &ctx->e), SUCCESS);
    test_assert_memequal (buf, payload, sizeof buf);
    test_assert_int_equal (walis_read_all (wis, &checksum, buf, sizeof buf, &ctx->e), SUCCESS);
    test_assert_memequal (buf, payload + 4, sizeof buf);

    // Checksum covers everything read
    u32 expected = checksum_init ();
    checksum_execute (&expected, payload, sizeof payload);
    test_assert_int_equal (checksum, expected);

    // Nothing left
    test_assert_int_equal (walis_read_all (wis, NULL, buf, sizeof buf, &ctx->e), WAL_EOF);

    // Incomplete record: EOF, and the position goes back to the last mark
    walis_mark (wis);
    walis_seek (wis, 4, &ctx->e);
    u8 big[8] = {0};
    test_assert_int_equal (walis_read_all (wis, NULL, big, sizeof big, &ctx->e), WAL_EOF);

    // ... so the same bytes can still be read as a complete record afterwards
    test_assert_int_equal (walis_read_all (wis, NULL, buf, sizeof buf, &ctx->e), SUCCESS);
    test_assert_memequal (buf, payload + 4, sizeof buf);

    walis_close (wis, &ctx->e);
    ctx->os.table->remove_quiet (ctx->os.self, "foo", &ctx->e);
  }

  TEST_CASE ("Flaky os")
  {
    int   fail_after = 0;
    err_t ret;

    do {
      TEST_CASE ("Failure flaky os. Fail after: %d", fail_after)
      {
        // Setup (4 byte file, asking for 8 so the seek back path runs too)
        walis_test_write (ctx, "foo", payload, 4);
        struct wal_istream *wis = walis_open ("foo", ctx->mem, ctx->faulty_os, &ctx->e);
        test_assert (wis != NULL);

        // Do test
        u8 buf[8] = {0};
        faulty_os_fail_after (ctx->faulty_os, fail_after);
        walis_read_all (wis, NULL, buf, sizeof buf, &ctx->e);
        ret = error_trace (&ctx->e);

        // Clean up
        faulty_os_reset (ctx->faulty_os);
        error_reset (&ctx->e);
        walis_close (wis, &ctx->e);
        ctx->os.table->remove_quiet (ctx->os.self, "foo", &ctx->e);

        // Next fail after
        fail_after++;
      }
    }
    while (ret != SUCCESS);
  }
}
#endif

err_t
walis_seek (struct wal_istream *w, const lsn pos, error *e)
{
  latch_lock (&w->l);

  if (walis_seek_unsafe (w, pos, e) < 0) {
    latch_unlock (&w->l);
    return error_trace (e);
  }

  latch_unlock (&w->l);
  return SUCCESS;
}

#ifndef NDEBUG
TEST (walis_seek)
{
  const u8 payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};

  TEST_CASE ("happy path")
  {
    walis_test_write (ctx, "foo", payload, sizeof payload);
    struct wal_istream *wis = walis_open ("foo", ctx->mem, ctx->os, &ctx->e);
    test_assert (wis != NULL);

    // Seek into the middle and read from there
    test_assert_int_equal (walis_seek (wis, 4, &ctx->e), SUCCESS);
    u8 buf[4] = {0};
    test_assert_int_equal (walis_read_all (wis, NULL, buf, sizeof buf, &ctx->e), SUCCESS);
    test_assert_memequal (buf, payload + 4, sizeof buf);

    // And back to the start
    test_assert_int_equal (walis_seek (wis, 0, &ctx->e), SUCCESS);
    test_assert_int_equal (walis_read_all (wis, NULL, buf, sizeof buf, &ctx->e), SUCCESS);
    test_assert_memequal (buf, payload, sizeof buf);

    walis_close (wis, &ctx->e);
    ctx->os.table->remove_quiet (ctx->os.self, "foo", &ctx->e);
  }

  TEST_CASE ("Flaky os")
  {
    int   fail_after = 0;
    err_t ret;

    do {
      TEST_CASE ("Failure flaky os. Fail after: %d", fail_after)
      {
        // Setup
        walis_test_write (ctx, "foo", payload, sizeof payload);
        struct wal_istream *wis = walis_open ("foo", ctx->mem, ctx->faulty_os, &ctx->e);
        test_assert (wis != NULL);

        // Do test
        faulty_os_fail_after (ctx->faulty_os, fail_after);
        walis_seek (wis, 4, &ctx->e);
        ret = error_trace (&ctx->e);

        // Clean up
        faulty_os_reset (ctx->faulty_os);
        error_reset (&ctx->e);
        walis_close (wis, &ctx->e);
        ctx->os.table->remove_quiet (ctx->os.self, "foo", &ctx->e);

        // Next fail after
        fail_after++;
      }
    }
    while (ret != SUCCESS);
  }
}
#endif

lsn
walis_mark (struct wal_istream *w)
{
  latch_lock (&w->l);

  w->curlsn += w->lsnidx;
  w->lsnidx  = 0;
  lsn curlsn = w->curlsn;

  latch_unlock (&w->l);

  return curlsn;
}

#ifndef NDEBUG
TEST (walis_mark)
{
  TEST_CASE ("happy path")
  {
    const u8 payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    walis_test_write (ctx, "foo", payload, sizeof payload);
    struct wal_istream *wis = walis_open ("foo", ctx->mem, ctx->os, &ctx->e);
    test_assert (wis != NULL);

    u8 buf[4] = {0};

    // Nothing read yet
    test_assert_int_equal ((i32)walis_mark (wis), 0);

    // mark returns the start of the record just consumed and commits its end
    test_assert_int_equal (walis_read_all (wis, NULL, buf, sizeof buf, &ctx->e), SUCCESS);
    test_assert_int_equal ((i32)walis_mark (wis), 4);
    test_assert_int_equal ((i32)wis->curlsn, 4);

    test_assert_int_equal (walis_read_all (wis, NULL, buf, sizeof buf, &ctx->e), SUCCESS);
    test_assert_int_equal ((i32)walis_mark (wis), 8);
    test_assert_int_equal ((i32)wis->curlsn, 8);

    // Marking again with nothing consumed returns the committed position
    test_assert_int_equal ((i32)walis_mark (wis), 8);

    // A half read record does not move the mark: the read rewinds to it
    test_assert_int_equal (walis_seek (wis, 4, &ctx->e), SUCCESS);
    u8 big[8] = {0};
    test_assert_int_equal (walis_read_all (wis, NULL, big, sizeof big, &ctx->e), WAL_EOF);
    test_assert_int_equal ((i32)walis_mark (wis), 4);

    walis_close (wis, &ctx->e);
    ctx->os.table->remove_quiet (ctx->os.self, "foo", &ctx->e);
  }
}
#endif

err_t
walis_crash (struct wal_istream *w, error *e)
{
  DBG_ASSERT (wal_istream, w);
  w->os.table->close_file (w->os.self, w->fd.self, e);
  w->mem.table->free (w->mem.self, w);
  return error_trace (e);
}

#ifndef NDEBUG

#  define WALIS_CRASH_T_FILE "walis_crash_t.wal"

/// Writes [len] bytes of [src] as the whole contents of the test wal
static void
walis_test_seed (struct test_ctx *ctx, const void *src, const u32 len)
{
  ctx->os.table->remove_quiet (ctx->os.self, WALIS_CRASH_T_FILE, &ctx->e);

  i_file f;
  test_assert_int_equal (
      ctx->os.table->open_file_w (ctx->os.self, &f, WALIS_CRASH_T_FILE, &ctx->e),
      SUCCESS
  );
  test_assert_int_equal (f.table->write_all (f.self, src, len, &ctx->e), SUCCESS);
  test_assert_int_equal (ctx->os.table->close_file (ctx->os.self, f.self, &ctx->e), SUCCESS);
}

/// walis_crash abandons a reader mid-stream: it has to release the file
/// handle and the allocation (a leaked fd trips the OS teardown assert in
/// test_ctx_free) and, unlike the writer's crash, it must not be able to
/// damage the log it was reading.
TEST (walis_crash)
{
  const u8 payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};

  TEST_CASE ("crashing part way through a record leaves the log alone")
  {
    walis_test_seed (ctx, payload, sizeof payload);

    struct wal_istream *wis = walis_open (WALIS_CRASH_T_FILE, ctx->mem, ctx->os, &ctx->e);
    test_assert (wis != NULL);

    // Read the first half of a record and die before marking its end, so the
    // committed lsn is still 0 while 4 bytes have been consumed
    lsn rlsn     = walis_mark (wis);

    u32 checksum = 0;
    u8  buf[4]   = {0};

    test_assert_int_equal (walis_read_all (wis, &checksum, buf, sizeof buf, &ctx->e), WAL_NOT_EOF);
    test_assert_int_equal ((i32)rlsn, 0);
    test_assert_memequal (buf, payload, sizeof buf);

    test_assert_int_equal (walis_crash (wis, &ctx->e), SUCCESS);

    // The reader held the only handle and it opened read only, so every byte
    // is still there for the next reader
    i_file f;
    test_assert_int_equal (
        ctx->os.table->open_file_r (ctx->os.self, &f, WALIS_CRASH_T_FILE, &ctx->e),
        SUCCESS
    );
    test_assert_int_equal ((i32)f.table->file_size (f.self, &ctx->e), (i32)sizeof payload);
    ctx->os.table->close_file (ctx->os.self, f.self, &ctx->e);

    ctx->os.table->remove_quiet (ctx->os.self, WALIS_CRASH_T_FILE, &ctx->e);
  }

  TEST_CASE ("a fresh reader starts over from the top")
  {
    walis_test_seed (ctx, payload, sizeof payload);

    struct wal_istream *wis = walis_open (WALIS_CRASH_T_FILE, ctx->mem, ctx->os, &ctx->e);
    test_assert (wis != NULL);

    u8 buf[4] = {0};
    walis_mark (wis);
    test_assert_int_equal (walis_read_all (wis, NULL, buf, sizeof buf, &ctx->e), WAL_NOT_EOF);
    test_assert_int_equal (walis_crash (wis, &ctx->e), SUCCESS);

    // No read position is persisted anywhere, so the replacement reader sees
    // the log from byte 0 again - the whole of it, in one read this time
    wis = walis_open (WALIS_CRASH_T_FILE, ctx->mem, ctx->os, &ctx->e);
    test_assert (wis != NULL);

    u8 all[8] = {0};
    walis_mark (wis);
    test_assert_int_equal (walis_read_all (wis, NULL, all, sizeof all, &ctx->e), WAL_NOT_EOF);
    test_assert_memequal (all, payload, sizeof payload);

    test_assert_int_equal (walis_close (wis, &ctx->e), SUCCESS);
    ctx->os.table->remove_quiet (ctx->os.self, WALIS_CRASH_T_FILE, &ctx->e);
  }

  TEST_CASE ("crashing straight after open")
  {
    walis_test_seed (ctx, payload, sizeof payload);

    struct wal_istream *wis = walis_open (WALIS_CRASH_T_FILE, ctx->mem, ctx->os, &ctx->e);
    test_assert (wis != NULL);
    test_assert_int_equal (walis_crash (wis, &ctx->e), SUCCESS);

    ctx->os.table->remove_quiet (ctx->os.self, WALIS_CRASH_T_FILE, &ctx->e);
  }

  TEST_CASE ("Flaky os")
  {
    int   fail_after = 0;
    err_t ret;

    do {
      TEST_CASE ("Failure flaky os. Fail after: %d", fail_after)
      {
        // Setup
        walis_test_seed (ctx, payload, sizeof payload);
        struct wal_istream
            *wis = walis_open (WALIS_CRASH_T_FILE, ctx->mem, ctx->faulty_os, &ctx->e);
        test_assert (wis != NULL);

        // Do test (the stream is freed regardless of the error)
        faulty_os_fail_after (ctx->faulty_os, fail_after);
        walis_crash (wis, &ctx->e);
        ret = error_trace (&ctx->e);

        // Clean up
        faulty_os_reset (ctx->faulty_os);
        error_reset (&ctx->e);
        ctx->os.table->remove_quiet (ctx->os.self, WALIS_CRASH_T_FILE, &ctx->e);

        // Next fail after
        fail_after++;
      }
    }
    while (ret != SUCCESS);
  }
}

#endif // TESTING
