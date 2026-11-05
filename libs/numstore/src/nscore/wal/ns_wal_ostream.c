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
#include "nscore/wal/ns_wal.h"

#ifndef NDEBUG
#  include "core/ns_testing.h"
#endif

#include <stddef.h>

// [w->e] is deliberately not asserted clean: it is where a failed write or
// fsync is parked until the caller gets around to noticing (see walos_failed)
DEFINE_DBG_ASSERT (struct wal_ostream, wal_ostream, w, {
  ASSERT (w);
  DBG_ASSERT (cbuffer, &w->buffer);
  DBG_ASSERT (i_mem, &w->mem);
})

bool
walos_failed (const struct wal_ostream *w)
{
  ASSERT (w);
  return w->e.cause_code != SUCCESS;
}

err_t
walos_take_err (struct wal_ostream *w, error *e)
{
  DBG_ASSERT (wal_ostream, w);
  ASSERT (walos_failed (w));

  *e   = w->e;
  w->e = error_create ();

  return e->cause_code;
}

struct wal_ostream *
walos_open (const char *fname, struct i_mem mem, struct i_os os, lsn seek_to, error *e)
{
  // Allocate
  struct wal_ostream *ret = mem.table->malloc (mem.self, 1, sizeof *ret, e);
  if (ret == NULL) {
    return NULL;
  }

  if (os.table->open_file_w (os.self, &ret->fd, fname, e)) {
    mem.table->free (mem.self, ret);
    return NULL;
  }

  // The file may be longer than [seek_to] - a crash leaves the tail of a
  // half-flushed record behind, and the caller opens us exactly where the last
  // intact record ended. Those bytes are not a record and nothing will ever
  // overwrite all of them, so they would read back as a bad log type or a bad
  // CRC the next time around. Cut them off.
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

  if ((u64)size > seek_to && ret->fd.table->truncate (ret->fd.self, seek_to, e)) {
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
        "WAL Ostream opened to seek position: %" PRlsn " but could only seek to position: %" PRlsn,
        seek_to,
        pos
    );
    goto err_close;
  }

  // Initialize stuff
  latch_init (&ret->l);
  ret->buffer      = cbuffer_create (ret->_buffer, sizeof (ret->_buffer));
  ret->flushed_lsn = pos;
  ret->mem         = mem;
  ret->os          = os;

  // The stream carries its own error for the paths that have nowhere to report
  // one - see walos_failed, which is how a caller finds out about it
  ret->e           = error_create ();

  DBG_ASSERT (wal_ostream, ret);
  return ret;

err_close:
  os.table->close_file (os.self, ret->fd.self, e);
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
        if (wos) {
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
        if (wos) {
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

/// Pushes the buffer out and fsyncs it. Caller holds [w->l] and owns the
/// error: this neither latches a failure nor crashes, so the function that
/// took the latch can let go of it first.
static err_t
walos_flush_unsafe (struct wal_ostream *w, error *e)
{
  // Number of bytes to write
  const u32 towrite = cbuffer_len (&w->buffer);
  if (towrite == 0) {
    return SUCCESS;
  }

  // Just write the data to the file (don't update the buffer at all)
  if (cbuffer_write_to_file_1_expect (&w->fd, &w->buffer, towrite, e)) {
    return error_trace (e);
  }

  // Update the buffer
  cbuffer_write_to_file_2 (&w->buffer, towrite);

  // Run an fsync
  if (w->fd.table->fsync (w->fd.self, e)) {
    return error_trace (e);
  }

  // Update flushed lsn
  w->flushed_lsn += towrite;

  return SUCCESS;
}

#ifndef NDEBUG
TEST (walos_flush_unsafe)
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
    test_assert_int_equal (walos_flush_unsafe (wos, &ctx->e), SUCCESS);

    // Check that flushed lsn is 10
    test_assert (wos->flushed_lsn == 10);

    // Nothing buffered - still fine, still nothing written
    test_assert_int_equal (walos_flush_unsafe (wos, &ctx->e), SUCCESS);
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

        // Do test - the unsafe step reports, it never dies
        faulty_os_fail_after (ctx->faulty_os, fail_after);
        ret = walos_flush_unsafe (wos, &ctx->e);
        if (ret != SUCCESS) {
          // A flush that did not land leaves the lsn where it was
          test_assert_int_equal ((i32)wos->flushed_lsn, 0);
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
  }
}
#endif

err_t
walos_close (struct wal_ostream *w, error *e)
{
  latch_lock (&w->l);

  DBG_ASSERT (wal_ostream, w);

  // Closing has somewhere to report a failure, so a last flush that does not
  // land comes back as an error rather than killing the process. An error the
  // write path latched earlier is handed over the same way.
  if (walos_failed (w)) {
    walos_take_err (w, e);
  } else {
    walos_flush_unsafe (w, e);
  }

  w->os.table->close_file (w->os.self, w->fd.self, e);

  latch_unlock (&w->l);
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

/// Abandons the writer: the fd goes back to the OS and the allocation is
/// freed, but the buffer is dropped on the floor so nothing that was not
/// already flushed reaches the log.
err_t
walos_crash (struct wal_ostream *w, error *e)
{
  latch_lock (&w->l);

  DBG_ASSERT (wal_ostream, w);
  w->os.table->close_file (w->os.self, w->fd.self, e);

  latch_unlock (&w->l);
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

  // Once the log has a hole in it there is nothing useful left to push out
  if (!walos_failed (w)) {
    walos_flush_unsafe (w, &w->e);
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
    test_assert_int_equal (wos->flushed_lsn, 0);
    walos_flush_all (wos);
    test_assert_int_equal (wos->flushed_lsn, 10);

    // Flushing an empty buffer changes nothing
    walos_flush_all (wos);
    test_assert_int_equal (wos->flushed_lsn, 10);

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

        // Do test - the failure is latched, not thrown, and the latch is free
        // afterwards either way
        faulty_os_fail_after (ctx->faulty_os, fail_after);
        walos_flush_all (wos);
        ret = walos_failed (wos) ? ERR_IO : SUCCESS;
        test_assert (latch_trylock (&wos->l));
        latch_unlock (&wos->l);

        if (ret != SUCCESS) {
          // A failed stream stops touching the file
          const lsn stuck = wos->flushed_lsn;
          walos_flush_all (wos);
          test_assert_int_equal ((i32)wos->flushed_lsn, (i32)stuck);

          // And hands the error over once
          test_assert (walos_take_err (wos, &ctx->e) != SUCCESS);
          test_assert (!walos_failed (wos));
        }

        // Clean up
        faulty_os_reset (ctx->faulty_os);
        error_reset (&ctx->e);
        walos_close (wos, &ctx->e);
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

void
walos_write_all (struct wal_ostream *w, u32 *checksum, const void *data, u32 len)
{
  DBG_ASSERT (wal_ostream, w);

  if (checksum) {
    checksum_execute (checksum, data, len);
  }

  u32       written = 0;
  const u8 *src     = data;

  latch_lock (&w->l);

  // Maximum number of loops
  u32 loop_limit = len / WAL_BUFFER_CAP + 1;
  u32 loop       = 0;

  while (written < len && !walos_failed (w)) {
    // Flush on full
    if (cbuffer_avail (&w->buffer) == 0 && walos_flush_unsafe (w, &w->e)) {
      // The log now has a hole in it. Stop here and leave the failure parked
      // for the caller's top level - it is the one holding the latches.
      break;
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
    test_assert_int_equal (wos->flushed_lsn, 0);
    walos_write_all (wos, &checksum, data, 10);
    test_assert_int_equal (wos->flushed_lsn, 0);

    // Writing over buffer size, flush out
    walos_write_all (wos, &checksum, data, WAL_BUFFER_CAP);
    test_assert_int_equal (wos->flushed_lsn, WAL_BUFFER_CAP);

    // Writing exactly buffer size (unchaged)
    walos_write_all (wos, &checksum, data, WAL_BUFFER_CAP - 10);
    test_assert_int_equal (wos->flushed_lsn, WAL_BUFFER_CAP);

    // Writing 1 more byte
    walos_write_all (wos, &checksum, data, 1);
    test_assert_int_equal (wos->flushed_lsn, 2 * WAL_BUFFER_CAP);

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
        ctx->os.table->remove_quiet (ctx->os.self, "foo", &ctx->e);
        struct wal_ostream *wos = walos_open ("foo", ctx->mem, ctx->faulty_os, 0, &ctx->e);
        test_assert (wos != NULL);

        // Fill up the buffer - no flush yet, so no fault can fire
        u8 data[WAL_BUFFER_CAP] = {0};
        rand_bytes (data, WAL_BUFFER_CAP);
        walos_write_all (wos, NULL, data, WAL_BUFFER_CAP);
        test_assert (!walos_failed (wos));

        // The next write has to flush to make room
        faulty_os_fail_after (ctx->faulty_os, fail_after);
        walos_write_all (wos, NULL, data, WAL_BUFFER_CAP);
        ret = walos_failed (wos) ? ERR_IO : SUCCESS;

        // Whatever happened, the latch went back
        test_assert (latch_trylock (&wos->l));
        latch_unlock (&wos->l);

        // Clean up
        faulty_os_reset (ctx->faulty_os);
        error_reset (&ctx->e);
        walos_close (wos, &ctx->e);
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

lsn
walos_get_lsn (struct wal_ostream *w)
{
  latch_lock (&w->l);
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
  if (w->fd.table->truncate (w->fd.self, 0, e)) {
    goto theend;
  }
  if (w->fd.table->seek (w->fd.self, 0, I_SEEK_SET, e)) {
    goto theend;
  }

  w->flushed_lsn = 0;
  w->buffer      = cbuffer_create (w->_buffer, sizeof (w->_buffer));

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
    test_assert_int_equal ((i32)walos_test_file_size (ctx, "foo"), 10);

    // Truncate
    test_assert_int_equal (walos_truncate (wos, &ctx->e), SUCCESS);
    test_assert_int_equal ((i32)walos_test_file_size (ctx, "foo"), 0);

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
