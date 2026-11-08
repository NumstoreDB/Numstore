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

#include "nscore/wal/ns_wal_ostream.h"

#include "core/ns_cbuffer.h"
#include "core/ns_crash.h"
#include "core/ns_crc.h"
#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_latch.h"
#include "core/ns_numerics.h"
#include "core/ns_stdtypes.h"
#include "core/ns_utils.h"
#include "core/os/ns_malloc.h"
#include "core/os/ns_os.h"

#ifndef NDEBUG
#  include "core/ns_testing.h"
#endif

#include <stddef.h>

DEFINE_DBG_ASSERT (struct wal_ostream, wal_ostream, w, {
  ASSERT (w);
  DBG_ASSERT (cbuffer, &w->buffer);
  DBG_ASSERT (i_mem, &w->mem);
  ASSERT (w->file_size >= w->flushed_lsn);
})

struct wal_ostream *
walos_open (const char *fname, struct i_mem mem, struct i_os os, lsn seek_to, error *e)
{
  struct wal_ostream *ret = mem.table->malloc (mem.self, 1, sizeof *ret, e);
  if (ret == NULL) {
    return NULL;
  }

  if (os.table->open_file_w (os.self, &ret->fd, fname, e) < 0) {
    mem.table->free (mem.self, ret);
    return NULL;
  }

  const i64 size = ret->fd.table->file_size (ret->fd.self, e);
  if (size < 0) {
    goto err_close;
  }

  if ((u64)size < seek_to) {
    error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "WAL Ostream opened to seek position: %" PRlsn " past the end of a %" PRId64 " byte log",
        seek_to,
        size
    );
    goto err_close;
  }

  // Seek to where the log is to continue
  i64 pos = ret->fd.table->seek (ret->fd.self, seek_to, I_SEEK_SET, e);

  // Failed to seek
  if (pos < 0) {
    goto err_close;
  }

  // Failed to seek to desired spot (invalid seek_to)
  if ((u64)pos != seek_to) {
    error_causef (
        e,
        ERR_INVALID_ARGUMENT,
        "WAL Ostream opened to seek position: %" PRlsn " but could only seek to position: %" PRId64,
        seek_to,
        pos
    );
    goto err_close;
  }

  ret->mem = mem;
  ret->os  = os;
  latch_init (&ret->l);
  ret->flushed_lsn = seek_to;
  ret->file_size   = (lsn)size;
  ret->e           = error_create ();
  ret->buffer      = cbuffer_create (ret->_buffer, sizeof ret->_buffer);

  DBG_ASSERT (wal_ostream, ret);
  return ret;

err_close:
  // Already failing - a close failure adds nothing the caller can act on
  (void)os.table->close_file (os.self, ret->fd.self, e);
  mem.table->free (mem.self, ret);
  return NULL;
}

#ifndef NDEBUG
TEST (walos_open)
{
  TEST_CASE ("happy path")
  {
    ctx->os.table->remove_quiet (ctx->os.self, "foo", &ctx->e);
    struct wal_ostream *wos = walos_open ("foo", ctx->mem, ctx->os, 0, &ctx->e);
    test_assert (wos != NULL);
    walos_close (wos, &ctx->e);
    ctx->os.table->remove_quiet (ctx->os.self, "foo", &ctx->e);
  }

  TEST_CASE ("Flaky malloc")
  {
    int   fail_after = 0;
    err_t ret;

    do {
      TEST_CASE ("Failure flaky malloc. Fail after: %d", fail_after)
      {
        // Do test
        faulty_mem_fail_after (ctx->faulty_mem, fail_after);
        struct wal_ostream *wos = walos_open ("foo", ctx->faulty_mem, ctx->os, 0, &ctx->e);
        ret                     = error_trace (&ctx->e);
        if (ret != SUCCESS) {
          test_assert (wos == NULL);
        }
        faulty_mem_reset (ctx->faulty_mem);
        error_reset (&ctx->e);

        // Reset
        if (wos != NULL) {
          walos_close (wos, &ctx->e);
        }

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
        // Do test
        faulty_os_fail_after (ctx->faulty_os, fail_after);
        struct wal_ostream *wos = walos_open ("foo", ctx->mem, ctx->faulty_os, 0, &ctx->e);
        ret                     = error_trace (&ctx->e);
        if (ret != SUCCESS) {
          test_assert (wos == NULL);
        }
        faulty_os_reset (ctx->faulty_os);
        error_reset (&ctx->e);

        // Reset
        if (wos != NULL) {
          walos_close (wos, &ctx->e);
        }

        // Next fail after
        fail_after++;
      }
    }
    while (ret != SUCCESS);
  }
}
#endif

static err_t
walos_flush_impl (struct wal_ostream *w, error *e)
{
  const u32 towrite = cbuffer_len (&w->buffer);
  if (towrite == 0) {
    return SUCCESS;
  }

  // Just write the data to the file (don't update the buffer at all)
  if (cbuffer_write_to_file_1_expect (&w->fd, &w->buffer, towrite, e) < 0) {
    return error_trace (e);
  }

  cbuffer_write_to_file_2 (&w->buffer, towrite);

  if (w->fd.table->fsync (w->fd.self, e) < 0) {
    return error_trace (e);
  }

  w->flushed_lsn += towrite;

  // Sometimes, a flushed lsn happens before the file size.
  //
  // Imagine
  //  write(partial update log)
  //  crash
  //  open
  //  write(begin log)
  //
  // A fully flushed begin log is smaller than a partially written
  // update log, so the file size stays the same, but flushed lsn grows
  w->file_size = MAX (w->file_size, w->flushed_lsn);

  return SUCCESS;
}

#ifndef NDEBUG
TEST (walos_flush_impl)
{
  TEST_CASE ("happy path")
  {
    // Set up
    ctx->os.table->remove_quiet (ctx->os.self, "foo", &ctx->e);
    struct wal_ostream *wos = walos_open ("foo", ctx->mem, ctx->os, 0, &ctx->e);
    test_assert (wos != NULL);

    // Flush
    u8 data[10] = {0};
    walos_write_all (wos, NULL, data, 10);
    test_assert (wos->flushed_lsn == 0);
    walos_flush_impl (wos, &ctx->e);

    // Check that flushed lsn is 10
    test_assert (wos->flushed_lsn == 10);

    // Clean up
    walos_close (wos, &ctx->e);
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
        ctx->os.table->remove_quiet (ctx->os.self, "foo", &ctx->e);
        struct wal_ostream *wos = walos_open ("foo", ctx->mem, ctx->faulty_os, 0, &ctx->e);
        test_assert (wos != NULL);

        u8 data[10] = {0};
        walos_write_all (wos, NULL, data, 10);

        // Do test - flush_impl reports through the error it was handed, so a
        // failure here is visible to the caller that passed &ctx->e
        faulty_os_fail_after (ctx->faulty_os, fail_after);
        ret = walos_flush_impl (wos, &ctx->e);
        if (ret != SUCCESS) {
          test_assert (ctx->e.cause_code < 0);
        }

        // Clean up
        faulty_os_reset (ctx->faulty_os);
        error_reset (&ctx->e);
        walos_close (wos, &ctx->e);
        ctx->os.table->remove_quiet (ctx->os.self, "foo", &ctx->e);

        // Next fail after
        fail_after++;
      }
    }
    while (ret != SUCCESS);

    // The sweep has to reach past the first injection point. It used to set
    // ret = SUCCESS unconditionally, which ended the loop after fail_after 0
    // and left every os call but the first untested.
    test_assert (fail_after > 1);
  }
}
#endif

err_t
walos_close (struct wal_ostream *w, error *e)
{
  latch_lock (&w->l);

  DBG_ASSERT (wal_ostream, w);

  // Unlike walos_flush_all, close can report - the caller is handed an error
  // rather than a dead process. Close and free regardless, the stream is gone
  // either way.
  (void)walos_flush_impl (w, e);
  (void)w->os.table->close_file (w->os.self, w->fd.self, e);
  w->mem.table->free (w->mem.self, w);

  return error_trace (e);
}

#ifndef NDEBUG
TEST (walos_close)
{
  TEST_CASE ("happy path")
  {
    // Setup
    ctx->os.table->remove_quiet (ctx->os.self, "foo", &ctx->e);
    struct wal_ostream *wos = walos_open ("foo", ctx->mem, ctx->os, 0, &ctx->e);
    test_assert (wos != NULL);

    u8 data[10] = {0};
    walos_write_all (wos, NULL, data, 10);

    // Close
    test_assert_int_equal (walos_close (wos, &ctx->e), SUCCESS);

    // Close flushed the buffered data
    i_file f;
    test_assert_int_equal (ctx->os.table->open_file_r (ctx->os.self, &f, "foo", &ctx->e), SUCCESS);
    test_assert_int_equal ((i32)f.table->file_size (f.self, &ctx->e), 10);
    ctx->os.table->close_file (ctx->os.self, f.self, &ctx->e);

    // Clean up
    ctx->os.table->remove_quiet (ctx->os.self, "foo", &ctx->e);
  }
}
#endif

err_t
walos_crash (struct wal_ostream *w, error *e)
{
  latch_lock (&w->l);

  DBG_ASSERT (wal_ostream, w);
  // A crash frees the stream either way. A close failure stays in [e].
  (void)w->os.table->close_file (w->os.self, w->fd.self, e);
  w->mem.table->free (w->mem.self, w);

  return error_trace (e);
}

#ifndef NDEBUG
static inline i64
walos_test_file_size (struct test_ctx *ctx, const char *fname)
{
  i_file f;
  if (ctx->os.table->open_file_r (ctx->os.self, &f, fname, &ctx->e) != SUCCESS) {
    return -1;
  }
  const i64 size = f.table->file_size (f.self, &ctx->e);
  ctx->os.table->close_file (ctx->os.self, f.self, &ctx->e);
  return size;
}

TEST (walos_crash)
{
  const u8 payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};

  TEST_CASE ("unflushed writes never reach the file")
  {
    ctx->os.table->remove_quiet (ctx->os.self, "walos_crash_t.wal", &ctx->e);

    struct wal_ostream *wos = walos_open ("walos_crash_t.wal", ctx->mem, ctx->os, 0, &ctx->e);
    test_assert (wos != NULL);

    walos_write_all (wos, NULL, payload, sizeof payload);

    // Buffered only: the next lsn moved, the flushed one did not
    test_assert_int_equal ((i32)walos_get_lsn (wos), (i32)sizeof payload);
    test_assert_int_equal ((i32)wos->flushed_lsn, 0);

    test_assert_int_equal (walos_crash (wos, &ctx->e), SUCCESS);

    test_assert_int_equal ((i32)walos_test_file_size (ctx, "walos_crash_t.wal"), 0);

    ctx->os.table->remove_quiet (ctx->os.self, "walos_crash_t.wal", &ctx->e);
  }

  TEST_CASE ("flushed writes survive, later ones do not")
  {
    ctx->os.table->remove_quiet (ctx->os.self, "walos_crash_t.wal", &ctx->e);

    struct wal_ostream *wos = walos_open ("walos_crash_t.wal", ctx->mem, ctx->os, 0, &ctx->e);
    test_assert (wos != NULL);

    walos_write_all (wos, NULL, payload, sizeof payload);
    walos_flush_all (wos);
    test_assert_int_equal ((i32)wos->flushed_lsn, (i32)sizeof payload);

    // A second write that is still sitting in the buffer when we die
    walos_write_all (wos, NULL, payload, sizeof payload);
    test_assert_int_equal ((i32)walos_get_lsn (wos), 2 * (i32)sizeof payload);

    test_assert_int_equal (walos_crash (wos, &ctx->e), SUCCESS);

    // Exactly the flushed prefix is on disk
    test_assert_int_equal (
        (i32)walos_test_file_size (ctx, "walos_crash_t.wal"),
        (i32)sizeof payload
    );

    i_file f;
    test_assert_int_equal (
        ctx->os.table->open_file_r (ctx->os.self, &f, "walos_crash_t.wal", &ctx->e),
        SUCCESS
    );
    u8 readback[8] = {0};
    test_assert_int_equal (
        (i32)f.table->read_all (f.self, readback, sizeof readback, &ctx->e),
        (i32)sizeof readback
    );
    test_assert_memequal (readback, payload, sizeof payload);
    ctx->os.table->close_file (ctx->os.self, f.self, &ctx->e);

    ctx->os.table->remove_quiet (ctx->os.self, "walos_crash_t.wal", &ctx->e);
  }

  TEST_CASE ("crashing straight after open leaves an empty wal behind")
  {
    ctx->os.table->remove_quiet (ctx->os.self, "walos_crash_t.wal", &ctx->e);

    struct wal_ostream *wos = walos_open ("walos_crash_t.wal", ctx->mem, ctx->os, 0, &ctx->e);
    test_assert (wos != NULL);
    test_assert_int_equal (walos_crash (wos, &ctx->e), SUCCESS);

    // walos_open created the file, so it exists and is empty - not missing
    test_assert_int_equal ((i32)walos_test_file_size (ctx, "walos_crash_t.wal"), 0);

    ctx->os.table->remove_quiet (ctx->os.self, "walos_crash_t.wal", &ctx->e);
  }

  TEST_CASE ("Flaky os")
  {
    int   fail_after = 0;
    err_t ret;

    do {
      TEST_CASE ("Failure flaky os. Fail after: %d", fail_after)
      {
        // Setup
        ctx->os.table->remove_quiet (ctx->os.self, "walos_crash_t.wal", &ctx->e);
        struct wal_ostream
            *wos = walos_open ("walos_crash_t.wal", ctx->mem, ctx->faulty_os, 0, &ctx->e);
        test_assert (wos != NULL);

        // Do test
        faulty_os_fail_after (ctx->faulty_os, fail_after);
        walos_crash (wos, &ctx->e);
        ret = error_trace (&ctx->e);

        // Clean up (the stream is freed regardless of the error)
        faulty_os_reset (ctx->faulty_os);
        error_reset (&ctx->e);
        ctx->os.table->remove_quiet (ctx->os.self, "walos_crash_t.wal", &ctx->e);

        // Next fail after
        fail_after++;
      }
    }
    while (ret != SUCCESS);
  }
}

#endif

void
walos_flush_all (struct wal_ostream *w)
{
  DBG_ASSERT (wal_ostream, w);
  latch_lock (&w->l);

  // A log that cannot be written is the end of the line - there is nowhere to
  // report it, the caller is about to be told its logs are durable. Unlock
  // first: the crash is a longjmp in a debug build, so nothing would hand the
  // latch back. Same shape as walos_write_all.
  if (walos_flush_impl (w, &w->e) < 0) {
    latch_unlock (&w->l);
    intentionally_crash ();
  }

  latch_unlock (&w->l);
}

#ifndef NDEBUG
TEST (walos_flush_all)
{
  TEST_CASE ("happy path")
  {
    // Setup
    ctx->os.table->remove_quiet (ctx->os.self, "foo", &ctx->e);
    struct wal_ostream *wos = walos_open ("foo", ctx->mem, ctx->os, 0, &ctx->e);
    test_assert (wos != NULL);

    // Flush buffered data
    u8 data[10] = {0};
    walos_write_all (wos, NULL, data, 10);
    test_assert_type_equal (wos->flushed_lsn, (lsn)(0), lsn, PRlsn);
    walos_flush_all (wos);
    test_assert_type_equal (wos->flushed_lsn, (lsn)(10), lsn, PRlsn);

    // Flushing an empty buffer changes nothing
    walos_flush_all (wos);
    test_assert_type_equal (wos->flushed_lsn, (lsn)(10), lsn, PRlsn);

    // Clean up
    walos_close (wos, &ctx->e);
    ctx->os.table->remove_quiet (ctx->os.self, "foo", &ctx->e);
  }

  TEST_CASE ("Flaky os")
  {
    int   fail_after = 0;
    err_t ret;
    bool  crashed = false;

    do {
      TEST_CASE ("Failure flaky os. Fail after: %d", fail_after)
      {
        ret = SUCCESS;

        // Setup
        ctx->os.table->remove_quiet (ctx->os.self, "foo", &ctx->e);
        struct wal_ostream *wos = walos_open ("foo", ctx->mem, ctx->faulty_os, 0, &ctx->e);
        test_assert (wos != NULL);

        u8 data[10] = {0};
        walos_write_all (wos, NULL, data, 10);

        // Do test
        faulty_os_fail_after (ctx->faulty_os, fail_after);
        capture_intentional_crash (&ret, &ctx->e)
        {
          walos_flush_all (wos);
        }

        // Clean up
        faulty_os_reset (ctx->faulty_os);
        error_reset (&ctx->e);
        if (ret != SUCCESS) {
          crashed = true;
          // During a crash - lock could still be held
          latch_unlock (&wos->l);
        }
        walos_close (wos, &ctx->e);
        ctx->os.table->remove_quiet (ctx->os.self, "foo", &ctx->e);

        // Next fail after
        fail_after++;
      }
    }
    while (ret != SUCCESS);

    // A flush has nowhere to report a failure - the caller is about to be told
    // its logs are durable - so it has to take the process down. If nothing
    // ever crashed here, the failure was swallowed.
    test_assert (crashed);
  }
}
#endif

void
walos_write_all (struct wal_ostream *w, u32 *checksum, const void *data, u32 len)
{
  DBG_ASSERT (wal_ostream, w);

  if (checksum != NULL) {
    checksum_execute (checksum, data, len);
  }

  u32       written = 0;
  const u8 *src     = data;

  latch_lock (&w->l);

  // Maximum number of loops
  u32 loop_limit = (len / WAL_BUFFER_CAP) + 1;
  u32 loop       = 0;

  while (written < len) {
    // Flush on full
    if (cbuffer_avail (&w->buffer) == 0 && walos_flush_impl (w, &w->e) < 0) {
      latch_unlock (&w->l);
      intentionally_crash ();
    }

    // Write maximally to the buffer
    u32 towrite = MIN (len - written, cbuffer_avail (&w->buffer));
    if (towrite > 0) {
      cbuffer_write_expect (src + written, 1, towrite, &w->buffer);
      written += towrite;
    }

    ASSERT (loop <= loop_limit);
    loop += 1;
  }

  latch_unlock (&w->l);
}

#ifndef NDEBUG
TEST (walos_write_all)
{
  TEST_CASE ("happy path")
  {
    // Setup
    ctx->os.table->remove_quiet (ctx->os.self, "foo", &ctx->e);
    struct wal_ostream *wos = walos_open ("foo", ctx->mem, ctx->os, 0, &ctx->e);
    test_assert (wos != NULL);

    u32 checksum             = checksum_init ();
    u8  data[WAL_BUFFER_CAP] = {0};
    rand_bytes (data, WAL_BUFFER_CAP);

    // Under buffer size, flushed lsn should be unchanged
    test_assert_type_equal (wos->flushed_lsn, (lsn)(0), lsn, PRlsn);
    walos_write_all (wos, &checksum, data, 10);
    test_assert_type_equal (wos->flushed_lsn, (lsn)(0), lsn, PRlsn);

    // Writing over buffer size, flush out
    walos_write_all (wos, &checksum, data, WAL_BUFFER_CAP);
    test_assert_type_equal (wos->flushed_lsn, (lsn)(WAL_BUFFER_CAP), lsn, PRlsn);

    // Writing exactly buffer size (unchaged)
    walos_write_all (wos, &checksum, data, WAL_BUFFER_CAP - 10);
    test_assert_type_equal (wos->flushed_lsn, (lsn)(WAL_BUFFER_CAP), lsn, PRlsn);

    // Writing 1 more byte
    walos_write_all (wos, &checksum, data, 1);
    test_assert_type_equal (wos->flushed_lsn, (lsn)(2 * WAL_BUFFER_CAP), lsn, PRlsn);

    u32 actual_checksum = checksum_init ();
    checksum_execute (&actual_checksum, data, 10);
    checksum_execute (&actual_checksum, data, WAL_BUFFER_CAP);
    checksum_execute (&actual_checksum, data, WAL_BUFFER_CAP - 10);
    checksum_execute (&actual_checksum, data, 1);

    test_assert_int_equal (checksum, actual_checksum);

    // Clean up
    walos_close (wos, &ctx->e);
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
        struct wal_ostream *wos = walos_open ("foo", ctx->faulty_mem, ctx->os, 0, &ctx->e);
        ret                     = error_trace (&ctx->e);

        faulty_os_fail_after (ctx->faulty_os, fail_after);

        // Fill up the buffer
        u8 data[WAL_BUFFER_CAP] = {0};
        rand_bytes (data, WAL_BUFFER_CAP);
        walos_write_all (wos, NULL, data, WAL_BUFFER_CAP);

        // Next write flushes
        capture_intentional_crash (&ret, &ctx->e)
        {
          walos_write_all (wos, NULL, data, WAL_BUFFER_CAP);
        }

        // Clean up
        faulty_os_reset (ctx->faulty_os);
        error_reset (&ctx->e);
        walos_close (wos, &ctx->e);
        fail_after++;
      }
    }
    while (ret != SUCCESS);
  }
}
#endif

err_t
walos_zero_tail (struct wal_ostream *w, const void *zeros, u32 len, error *e)
{
  DBG_ASSERT (wal_ostream, w);
  ASSERT (zeros);

  latch_lock (&w->l);

  ASSERT (w->file_size >= w->flushed_lsn); // From DBG_ASSERT
  lsn n = w->file_size - w->flushed_lsn;
  if (n == 0) {
    goto theend;
  }

  // The caller passes in a buffer, we assume the caller knows what they
  // are doing and passes a buffer big enough. WAL passes in a buffer of
  // maximum size wal log size, but we don't know that. If the remainder
  // is too much to write, we fail.
  //
  // An invariant of the ostream is that we never have to do "infinite" writes
  // where we loop until the end of the file. We just have to do one write
  if (n > len) {
    error_causef (
        e,
        ERR_CORRUPT,
        "WAL has %" PRlsn " bytes past its last intact log at %" PRlsn
        " - more than one log (%" PRlsn "), so this is not a torn tail",
        n,
        w->flushed_lsn,
        (lsn)len
    );
    goto theend;
  }

  if (w->fd.table->pwrite_all (w->fd.self, zeros, n, w->flushed_lsn, e) < 0) {
    goto theend;
  }
  // An fsync failure stays in [e] and is returned below
  (void)w->fd.table->fsync (w->fd.self, e);

theend:
  latch_unlock (&w->l);
  return error_trace (e);
}

lsn
walos_get_lsn (struct wal_ostream *w)
{
  latch_lock (&w->l);
  DBG_ASSERT (wal_ostream, w);
  lsn ret = w->flushed_lsn + cbuffer_len (&w->buffer);
  latch_unlock (&w->l);
  return ret;
}

#ifndef NDEBUG
TEST (walos_get_lsn)
{
  TEST_CASE ("happy path")
  {
    // Setup
    ctx->os.table->remove_quiet (ctx->os.self, "foo", &ctx->e);
    struct wal_ostream *wos = walos_open ("foo", ctx->mem, ctx->os, 0, &ctx->e);
    test_assert (wos != NULL);

    // Fresh stream
    test_assert_int_equal ((i32)walos_get_lsn (wos), 0);

    // Buffered bytes count towards the lsn
    u8 data[10] = {0};
    walos_write_all (wos, NULL, data, 10);
    test_assert_int_equal ((i32)walos_get_lsn (wos), 10);

    // Flushing doesn't change the lsn
    walos_flush_all (wos);
    test_assert_int_equal ((i32)walos_get_lsn (wos), 10);

    // Clean up
    walos_close (wos, &ctx->e);
    ctx->os.table->remove_quiet (ctx->os.self, "foo", &ctx->e);
  }
}
#endif

slsn
walos_truncate (struct wal_ostream *w, error *e)
{
  latch_lock (&w->l);
  DBG_ASSERT (wal_ostream, w);

  if (w->fd.table->truncate (w->fd.self, 0, e) < 0) {
    goto theend;
  }
  if (w->fd.table->seek (w->fd.self, 0, I_SEEK_SET, e) < 0) {
    goto theend;
  }

  w->flushed_lsn = 0;
  w->file_size   = 0;
  w->buffer      = cbuffer_create (w->_buffer, sizeof w->_buffer);

theend:
  latch_unlock (&w->l);
  return error_trace (e);
}

#ifndef NDEBUG
TEST (walos_truncate)
{
  TEST_CASE ("happy path")
  {
    // Setup
    ctx->os.table->remove_quiet (ctx->os.self, "foo", &ctx->e);
    struct wal_ostream *wos = walos_open ("foo", ctx->mem, ctx->os, 0, &ctx->e);
    test_assert (wos != NULL);

    u8 data[10] = {0};
    walos_write_all (wos, NULL, data, 10);
    walos_flush_all (wos);
    test_assert_type_equal (walos_test_file_size (ctx, "foo"), (slsn)10, slsn, PRId64);

    // Truncate
    test_assert_int_equal (walos_truncate (wos, &ctx->e), SUCCESS);
    test_assert_type_equal (walos_test_file_size (ctx, "foo"), (slsn)0, slsn, PRId64);

    // Clean up
    walos_close (wos, &ctx->e);
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
        ctx->os.table->remove_quiet (ctx->os.self, "foo", &ctx->e);
        struct wal_ostream *wos = walos_open ("foo", ctx->mem, ctx->faulty_os, 0, &ctx->e);
        test_assert (wos != NULL);

        // Do test
        faulty_os_fail_after (ctx->faulty_os, fail_after);
        walos_truncate (wos, &ctx->e);
        ret = error_trace (&ctx->e);

        // Clean up
        faulty_os_reset (ctx->faulty_os);
        error_reset (&ctx->e);
        walos_close (wos, &ctx->e);
        ctx->os.table->remove_quiet (ctx->os.self, "foo", &ctx->e);

        // Next fail after
        fail_after++;
      }
    }
    while (ret != SUCCESS);
  }
}
#endif
