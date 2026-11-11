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

#include "ns_csx_assert.h"
#include "ns_error.h"
#include "ns_page.h"
#include "ns_page_h.h"
#include "ns_page_inner_node.h"
#include "ns_stdtypes.h"
#include "ns_testing.h"
#include "pager/ns_page_fixture.h"
#include "pager/ns_pager.h"
#include "rope/ns_rope_algorithms.h"

#include <stdbool.h>

err_t
ns_rope_in_scan (struct pager *p, const page_h *cur, in_page_consumer consume, void *ctx, error *e)
{
  DBG_ASSERT (pager, p);
  DBG_ASSERT (page_h, cur);

  struct inner_node in;

  // Consume the first page
  in_get (&in, page_h_ro (cur));
  if (consume (&in, ctx, e) < 0) {
    return error_trace (e);
  }

  // Fetch next page
  page_h iter = page_h_create ();
  if (in.npg != PGNO_NULL) {
    if (pgr_get (&iter, PG_INNER_NODE, in.npg, p, e) < 0) {
      return error_trace (e);
    }
  }

  while (true) {
    // Done
    if (iter.mode == PHM_NONE) {
      return SUCCESS;
    }

    // Get data
    in_get (&in, page_h_ro (&iter));

    // Execute consumer
    if (consume (&in, ctx, e) < 0) {
      // Already failing; the first error is the one reported
      (void)pgr_release (p, &iter, PG_INNER_NODE, e);
      return error_trace (e);
    }

    if (pgr_release (p, &iter, PG_INNER_NODE, e) < 0) {
      return error_trace (e);
    }

    // Fetch next
    if (in.npg != PGNO_NULL) {
      if (pgr_get (&iter, PG_INNER_NODE, in.npg, p, e) < 0) {
        return error_trace (e);
      }
    }
  }
}

err_t
ns_rope_in_scan_pgno (struct pager *p, pgno pg, in_page_consumer consume, void *ctx, error *e)
{
  DBG_ASSERT (pager, p);

  page_h cur = page_h_create ();
  if (pgr_get (&cur, PG_INNER_NODE, pg, p, e) < 0) {
    return error_trace (e);
  }

  if (ns_rope_in_scan (p, &cur, consume, ctx, e) < 0) {
    // Already failing; the first error is the one reported
    (void)pgr_release (p, &cur, PG_INNER_NODE, e);
    return error_trace (e);
  }

  return pgr_release (p, &cur, PG_INNER_NODE, e);
}

////////////////// Validate

static err_t
in_validate_consumer (struct inner_node *in, void *p, error *e)
{
  for (p_size i = 0; i < in->plen; ++i) {
    struct in_pair         pair = in->pairs[i];

    // Get root page stats
    struct rope_page_stats stats;
    if (ns_rope_page_stats (p, &stats, pair.pg, e) < 0) {
      return error_trace (e);
    }

    // Validate that the tracked bytes are the same
    if (stats.tracked != pair.key) {
      return error_causef (
          e,
          ERR_CORRUPT,
          "Inner node tracks: %" PRb_size " bytes but actual page tracks: %" PRb_size " bytes\n",
          pair.key,
          stats.tracked
      );
    }
  }

  return SUCCESS;
}

err_t
ns_rope_in_validate_pgno (struct pager *p, pgno pg, error *e)
{
  DBG_ASSERT (pager, p);

  return ns_rope_in_scan_pgno (p, pg, in_validate_consumer, p, e);
}

#ifndef NDEBUG

struct in_scan_count_ctx
{
  u32 seen;
  u32 fail_on;
};

static err_t
in_scan_count (struct inner_node *in, void *vctx, error *e)
{
  (void)in;
  struct in_scan_count_ctx *c = vctx;
  c->seen++;
  if (c->fail_on != 0 && c->seen == c->fail_on) {
    return error_causef (e, ERR_CORRUPT, "consumer asked to stop at %" PRIu32, c->seen);
  }
  return SUCCESS;
}

static void
in_scan_build_chain (struct test_ctx *ctx, struct pgr_fixture *f, pgno out[3])
{
  page_h ph[3];

  for (u32 i = 0; i < 3; ++i) {
    ph[i] = page_h_create ();
    test_assert_int_equal (pgr_new (&ph[i], f->p, &f->tx, PG_INNER_NODE, &ctx->e), SUCCESS);
    out[i] = page_h_pgno (&ph[i]);

    in_make_valid (page_h_w (&ph[i]));
  }

  in_set_next (page_h_w (&ph[0]), out[1]);
  in_set_next (page_h_w (&ph[1]), out[2]);
  in_set_next (page_h_w (&ph[2]), PGNO_NULL);

  // Write them out and let go of the frames
  for (u32 i = 0; i < 3; ++i) {
    test_assert_int_equal (pgr_release_with_flush (f->p, &ph[i], PG_INNER_NODE, &ctx->e), SUCCESS);
  }

  for (u32 i = 0; i < 3; ++i) {
    page_h h = page_h_create ();
    test_assert_int_equal (pgr_get (&h, PG_INNER_NODE, out[i], f->p, &ctx->e), SUCCESS);
    test_assert_int_equal (pgr_release_with_evict (f->p, &h, PG_INNER_NODE, &ctx->e), SUCCESS);
  }
}

TEST (ns_rope_in_scan)
{
  TEST_CASE ("Green path")
  {
    // Set up
    struct pgr_fixture f;
    pgr_fixture_create (&f, &ctx->e);
    pgno pg[3];
    in_scan_build_chain (ctx, &f, pg);

    // Scan with no failure
    struct in_scan_count_ctx c = {.seen = 0, .fail_on = 0};
    test_assert_int_equal (ns_rope_in_scan_pgno (f.p, pg[0], in_scan_count, &c, &ctx->e), SUCCESS);
    test_assert_int_equal ((i32)c.seen, 3);

    // Clean up
    pgr_fixture_teardown (&f, &ctx->e);
  }

  TEST_CASE ("Fails mid scan")
  {
    // Set up
    struct pgr_fixture f;
    pgr_fixture_create (&f, &ctx->e);
    pgno pg[3];
    in_scan_build_chain (ctx, &f, pg);

    // Fail on the second page
    struct in_scan_count_ctx c   = {.seen = 0, .fail_on = 2};
    err_t                    ret = ns_rope_in_scan_pgno (f.p, pg[0], in_scan_count, &c, &ctx->e);
    test_assert_int_equal (ret, ERR_CORRUPT);
    error_reset (&ctx->e);
    test_assert_int_equal ((i32)c.seen, 2);

    // The frame was handed back, so it can be taken again
    page_h h = page_h_create ();
    test_assert_int_equal (pgr_get (&h, PG_INNER_NODE, pg[1], f.p, &ctx->e), SUCCESS);
    test_assert_int_equal (pgr_release (f.p, &h, PG_INNER_NODE, &ctx->e), SUCCESS);

    // Clean up
    pgr_fixture_teardown (&f, &ctx->e);
  }
}

#endif
