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

#include <stdatomic.h>
#include <string.h>

#include "concurrency.h"
#include "os.h"
#include "wal.h"

#ifndef NTEST

/**
 * @struct wal_queue
 * @brief Parameters for the wal_multi_threaded test
 *
 * @var wal_queue::sync
 * @brief The "start" signal so all threads start at the same time
 *
 * @var wal_queue::ww
 * @brief The shared wal writer
 *
 * @var wal_queue::idx
 * @brief The next log to write - this is atomically added to for each writer
 *
 * @var wal_queue::read
 * @brief The list of log records to write (as read to make it easier to build)
 *
 * @var wal_queue::len
 * @brief Length of [read]
 */
struct wal_queue
{
  _Atomic u32 sync;
  struct wal *ww;
  atomic_int  idx;

  struct wal_rec_hdr_read *read;
  const int                len;
};

static void *
wal_thread (void *ctx)
{
  error             e = error_create ();
  struct wal_queue *q = ctx;

  while (atomic_load (&q->sync) > 0)
  {
    spin_pause ();
  }

  while (true)
  {
    const int idx = atomic_fetch_add (&q->idx, 1);
    if (idx >= q->len)
    {
      return NULL;
    }

    struct wal_rec_hdr_write write = wrhw_from_wrhr (&q->read[idx]);

    const slsn l = wal_append_log (q->ww, &write, &e);
    if (l < 0)
    {
      panic ("Failed to write log");
    }

    if (wal_flush_all (q->ww, &e))
    {
      panic ("Failed to flush wal");
    }
  }
}

TEST (wal_multi_threaded)
{
  error e = error_create ();
  i_remove_quiet ("test.wal", &e);
  struct wal *ww = wal_open ("test.wal", &e);
  wal_write_start_lsn (ww, 0, &e);

  const u32 N = 5000;

  struct wal_rec_hdr_read *read = i_malloc (N, sizeof *read, &e);

  for (u32 i = 0; i < N; ++i)
  {
    wal_rec_hdr_read_random (&read[i]);
  }

  struct wal_queue ctx = {
      .sync = 1,
      .ww   = ww,
      .idx  = 0,
      .read = read,
      .len  = N,
  };

  u32      nthreads;
  i_thread threads[10];
  for (nthreads = 0; nthreads < arrlen (threads); ++nthreads)
  {
    i_thread_create (&threads[nthreads], wal_thread, &ctx, &e);
  }

  // launch
  atomic_store (&ctx.sync, 0);

  i_log_info ("Threads active\n");

  for (; nthreads > 0; --nthreads)
  {
    i_thread_join (&threads[nthreads - 1], &e);
  }

  // To speed up searches, keep a "finger" which is "near" the
  // most recent found log
  u32 finger   = 0;
  lsn read_lsn = 0;

  for (u32 i = 0; i < N; ++i)
  {
    struct wal_rec_hdr_read *actual = wal_read_next (ww, &read_lsn, &e);
    i_print_wal_rec_hdr_read_light (LOG_INFO, actual, read_lsn);
    test_assert (actual->type != WL_EOF);

    // Search through all records to ensure it's there
    bool found = false;
    for (u32 k = 0; k < N; ++k)
    {
      const u32 idx = (finger + k) % N;
      if (wal_rec_hdr_read_equal (actual, &read[idx]))
      {
        finger         = (idx + 1) % N;
        read[idx].type = WL_EOF;
        found          = true;
        break;
      }
    }
    test_assert (found);
  }

  const struct wal_rec_hdr_read *actual = wal_read_next (ww, &read_lsn, &e);
  test_assert_int_equal (actual->type, WL_EOF);

  wal_close (ww, &e);
  i_free (read);
}

struct wal_test_params
{
  const char              *fname;
  struct wal_rec_hdr_read *batch1;
  u32                      batch1_len;
  struct wal_rec_hdr_read *batch2;
  u32                      batch2_len;
};

static void
wal_test_fill_batch (struct wal_rec_hdr_read *batch, const u32 len, error *e)
{
  for (u32 i = 0; i < len; i++)
  {
    struct wal_rec_hdr_read *r = &batch[i];

    switch (r->type)
    {
      case WL_UPDATE:
      {
        rand_bytes (r->update.phys.undo, NS_PAGE_SIZE);
        rand_bytes (r->update.phys.redo, NS_PAGE_SIZE);
        break;
      }
      case WL_CLR:
      {
        rand_bytes (r->clr.phys.redo, NS_PAGE_SIZE);
        break;
      }
      default:
      {
        break;
      }
    }
  }
}

static void
wal_test_free_batch (const struct wal_rec_hdr_read *batch, const u32 len)
{
  for (u32 i = 0; i < len; i++)
  {
    const struct wal_rec_hdr_read *r = &batch[i];
  }
}

static void
run_wal_test (const struct wal_test_params *p)
{
  error e = error_create ();

  i_remove_quiet (p->fname, &e);
  struct wal *ww = wal_open (p->fname, &e);
  wal_write_start_lsn (ww, 0, &e);
  /**
   * Write all the input logs
   */
  {
    slsn l = -1;
    for (u32 i = 0; i < p->batch1_len; i++)
    {
      struct wal_rec_hdr_write out   = wrhw_from_wrhr (&p->batch1[i]);
      slsn                     nextl = wal_append_log (ww, &out, &e);
      test_assert (nextl >= 0);
      test_assert (nextl > l);
      l = nextl;
    }
    wal_flush_all (ww, &e);
  }

  /**
   * Read all the input logs and expect
   * that they are the same as the
   * first batch written ones
   */
  {
    for (u32 i = 0; i < p->batch1_len; i++)
    {
      lsn                      read_lsn;
      struct wal_rec_hdr_read *next = NULL;
      if (i == 0)
      {
        next = wal_read_first (ww, &e);
      }
      else
      {
        next = wal_read_next (ww, &read_lsn, &e);
      }
      test_assert (wal_rec_hdr_read_equal (next, &p->batch1[i]));
    }
  }

  /**
   * Write a second batch of input logs
   */
  {
    slsn l = 0;
    for (u32 i = 0; i < p->batch2_len; i++)
    {
      struct wal_rec_hdr_write out = wrhw_from_wrhr (&p->batch2[i]);
      l                            = wal_append_log (ww, &out, &e);
    }
    wal_flush_all (ww, &e);
  }

  /**
   * Read from the start and confirm all the logs
   */
  {
    for (u32 i = 0; i < p->batch1_len; i++)
    {
      lsn                      read_lsn;
      struct wal_rec_hdr_read *next = NULL;
      if (i == 0)
      {
        next = wal_read_first (ww, &e);
      }
      else
      {
        next = wal_read_next (ww, &read_lsn, &e);
      }
      test_assert (wal_rec_hdr_read_equal (next, &p->batch1[i]));
    }

    for (u32 i = 0; i < p->batch2_len; i++)
    {
      lsn                      read_lsn;
      struct wal_rec_hdr_read *next = wal_read_next (ww, &read_lsn, &e);
      test_assert (wal_rec_hdr_read_equal (next, &p->batch2[i]));
    }
  }

  wal_close (ww, &e);
}

////////////////////////////////////////////////////////////
// WAL test cases

TEST (wal)
{
  error e = error_create ();

  struct wal_rec_hdr_read batch1_full[] = {
      {.type = WL_BEGIN, .begin = {.tid = 1}},
      {.type = WL_COMMIT, .commit = {.tid = 3, .prev = 20}},
      {.type = WL_END, .end = {.tid = 4, .prev = 30}},
      {
          .type = WL_UPDATE,
          .update =
              {
                  .type = WUP_PHYSICAL,
                  .tid  = 5,
                  .prev = 40,
                  .phys = {.pg = 111},
              },
      },
      {
          .type = WL_CLR,
          .clr =
              {
                  .type      = WCLR_PHYSICAL,
                  .tid       = 6,
                  .prev      = 50,
                  .undo_next = 42,
                  .phys      = {.pg = 222},
              },
      },
  };

  struct wal_rec_hdr_read batch2_full[] = {
      {.type = WL_BEGIN, .begin = {.tid = 2}},
      {
          .type = WL_UPDATE,
          .update =
              {
                  .type = WUP_PHYSICAL,
                  .tid  = 6,
                  .prev = 41,
                  .phys = {.pg = 112},
              },
      },
  };

  struct wal_rec_hdr_read batch1_begin_only[] = {
      {.type = WL_BEGIN, .begin = {.tid = 1}},
  };

  struct wal_rec_hdr_read batch2_begin_only[] = {
      {.type = WL_BEGIN, .begin = {.tid = 2}},
  };

  struct wal_rec_hdr_read batch1_no_ckpt[] = {
      {.type = WL_BEGIN, .begin = {.tid = 1}},
      {.type = WL_COMMIT, .commit = {.tid = 3, .prev = 20}},
      {.type = WL_END, .end = {.tid = 4, .prev = 30}},
      {
          .type = WL_UPDATE,
          .update =
              {
                  .type = WUP_PHYSICAL,
                  .tid  = 5,
                  .prev = 40,
                  .phys = {.pg = 111},
              },
      },
      {
          .type = WL_CLR,
          .clr =
              {
                  .type      = WCLR_PHYSICAL,
                  .tid       = 6,
                  .prev      = 50,
                  .undo_next = 42,
                  .phys      = {.pg = 222},
              },
      },
  };

  struct wal_test_params cases[] = {
      {
          .fname      = "test_full.wal",
          .batch1     = batch1_full,
          .batch1_len = arrlen (batch1_full),
          .batch2     = batch2_full,
          .batch2_len = arrlen (batch2_full),
      },
      {
          .fname      = "test_begin_only.wal",
          .batch1     = batch1_begin_only,
          .batch1_len = arrlen (batch1_begin_only),
          .batch2     = batch2_begin_only,
          .batch2_len = arrlen (batch2_begin_only),
      },
      {
          .fname      = "test_no_ckpt.wal",
          .batch1     = batch1_no_ckpt,
          .batch1_len = arrlen (batch1_no_ckpt),
          .batch2     = batch2_full,
          .batch2_len = arrlen (batch2_full),
      },
  };

  for (u32 i = 0; i < arrlen (cases); i++)
  {
    TEST_CASE ("Wal: %d", i)
    {
      const struct wal_test_params *c = &cases[i];

      wal_test_fill_batch (c->batch1, c->batch1_len, &e);
      wal_test_fill_batch (c->batch2, c->batch2_len, &e);

      run_wal_test (c);

      wal_test_free_batch (c->batch1, c->batch1_len);
      wal_test_free_batch (c->batch2, c->batch2_len);
    }
  }
}

TEST (wal_single_entry)
{
  error e = error_create ();

  struct wal_rec_hdr_read cases[] = {
      {.type = WL_BEGIN, .begin = {.tid = 1}},
      {.type = WL_COMMIT, .commit = {.tid = 2, .prev = 10}},
      {.type = WL_END, .end = {.tid = 3, .prev = 20}},
      {.type = WL_UPDATE,
       .update =
           {.type = WUP_PHYSICAL, .tid = 4, .prev = 30, .phys = {.pg = 111}}},
      {.type = WL_CLR,
       .clr =
           {.type      = WCLR_PHYSICAL,
            .tid       = 5,
            .prev      = 40,
            .undo_next = 42,
            .phys      = {.pg = 222}}},
  };

  for (u32 i = 0; i < arrlen (cases); i++)
  {
    TEST_CASE ("wal_single_entry: %d", i)
    {
      struct wal_rec_hdr_read *c = &cases[i];

      wal_test_fill_batch (c, 1, &e);

      i_remove_quiet ("test_single_entry.wal", &e);
      struct wal *ww = wal_open ("test_single_entry.wal", &e);
      wal_write_start_lsn (ww, 0, &e);

      // WRITE
      struct wal_rec_hdr_write out = wrhw_from_wrhr (c);
      const slsn               l   = wal_append_log (ww, &out, &e);
      test_assert (l >= 0);

      wal_flush_all (ww, &e);

      // READ
      struct wal_rec_hdr_read *next = wal_read_first (ww, &e);
      test_assert (wal_rec_hdr_read_equal (next, c));

      wal_close (ww, &e);

      wal_test_free_batch (c, 1);
    }
  }
}

#endif
