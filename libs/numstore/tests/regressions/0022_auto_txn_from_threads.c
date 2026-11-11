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

#include "ns_stdtypes.h"
#include "numstore.h"
#include "os/ns_os.h"

#include <stdlib.h>
#include <string.h>

#ifndef NDEBUG
#  include "ns_testing.h"
#endif

#ifndef NDEBUG
// FIXED - both causes below are gone
//
// WHAT HAPPENED:
//    Several threads call the API on one nsdb_t with a NULL transaction
//    (here "insert" with tx == NULL, each thread on its own variable). It
//    died in different places from run to run, in about half the runs
//
// CAUSE 1: one auto transaction per database
//    A NULL transaction makes nsdb run the call in an "auto transaction":
//    nsdb_auto_begin starts one, nsdb_auto_finish commits it or rolls it
//    back. There was exactly one of those per database - the struct txn
//    lived in the handle (db->auto_tx.tx, with an in_auto_txn flag next to
//    it). Two threads in an auto transaction at the same time shared that
//    one struct txn: both ran pgr_begin_txn on it, both added locks to it,
//    both committed it. Seen from it:
//
//      ASSERT (l->holder_counts[mode] > 0)       ns_gr_lock.c (gr_unlock)
//      ASSERT (a->current == NULL || a->head)    ns_slab_alloc.c
//
//    FIX: the auto transaction belongs to the call. nsdb_auto_begin takes
//    it from the slab allocator, the same as nsdb_begin, and
//    nsdb_auto_finish gives it back. A call is in an auto transaction when
//    the caller's transaction is NULL, so the shared flag is gone too
//
// CAUSE 2: one error per database
//    The public API kept one error object per handle (nsdb_t.e, set up in
//    ns_open). Every ns_* call started with error_reset (ns->e) and handed
//    ns->e down as the error of that call. Two calls at the same time
//    worked on the same object:
//
//      - A call that fails part way returns error_trace (e). If the other
//        thread had reset e in between, that read SUCCESS and the caller
//        carried on with half built state:
//          ASSERT (p->src)   ns_parser.h - the lexer failed, the parser ran
//      - A call saw the other call's error and failed for no reason of its
//        own. The log showed errors with no message that started in places
//        that can't fail, like dblb_ensure_space and fpgr_read
//      - ASSERT (e->cause_code == SUCCESS)   ns_error.h (clean_error)
//
//    With a NULL transaction the query is compiled before the database is
//    locked, so two threads were in there together all the time
//
//    FIX: the error belongs to the call. Each call works on an error on its
//    own stack. When it fails, that error is set on its transaction
//    (struct nsdb_txn has an error of its own) and copied to the
//    database's error under the database latch (nsdb_report_error in
//    ns_nsdb.c). Nothing resets the database's error any more - a caller
//    takes it with ns_strerror / ns_txn_strerror. On a handle that several
//    threads use, ns_strerror gives the last error of any thread and
//    ns_txn_strerror the error of your own transaction
//
// WHY THE TEST IS WRITTEN THIS WAY:
//    Each thread only touches its own variable, so there is nothing to
//    conflict on - under the global database lock and under hierarchical
//    locks alike.
//
//    Every few inserts a thread also makes a call that fails, to put the
//    error path under the same load. Those fail in the compiler, before
//    any transaction, which is where the threads really overlap
enum
{
  AT_NTHREADS = 4,
  AT_NINSERTS = 200,
};

struct at_worker
{
  nsdb_t *db;
  u32     id;
  u32     nfailed;
};

static void *
at_worker_run (void *arg)
{
  struct at_worker *w = arg;

  for (u32 i = 0; i < AT_NINSERTS; ++i) {
    if (ns_write (w->db, NULL, &i, sizeof i, "insert v%u %u 1", w->id, i) != 1) {
      w->nfailed++;
    }

    if (i % 8 == 0) {
      // Must fail, and must not disturb anybody's insert
      if (ns_execute (w->db, NULL, "thread %u says this is not a query", w->id) >= 0) {
        w->nfailed++;
      }

      // Some thread's error, or nobody's if another thread took it first.
      // Either way it is a whole string of ours
      char *msg = (char *)ns_strerror (w->db, true);
      if (msg != NULL) {
        if (strnlen (msg, MAX_ERRSTR_LEN) == 0) {
          w->nfailed++;
        }
        free (msg);
      }
    }
  }

  return NULL;
}

TEST (0022_auto_txn_from_threads)
{
  test_assert_int_equal (ns_cleanup ("test"), 0);
  nsdb_t *db = ns_open ("test");
  test_assert (db != NULL);

  for (u32 i = 0; i < AT_NTHREADS; ++i) {
    test_assert_int_equal (ns_execute (db, NULL, "create v%u u32", i), 0);
  }

  // Every thread appends to its own variable, all with tx == NULL
  struct at_worker workers[AT_NTHREADS];
  i_thread         threads[AT_NTHREADS];
  for (u32 i = 0; i < AT_NTHREADS; ++i) {
    workers[i] = (struct at_worker){.db = db, .id = i};
    test_assert_int_equal (
        ctx->os.table
            ->thread_create (ctx->os.self, &threads[i], at_worker_run, &workers[i], &ctx->e),
        SUCCESS
    );
  }
  for (u32 i = 0; i < AT_NTHREADS; ++i) {
    ctx->os.table->thread_join (ctx->os.self, threads[i].self);
  }

  // Every variable holds 0, 1, 2 ... in order
  for (u32 i = 0; i < AT_NTHREADS; ++i) {
    test_assert_int_equal (workers[i].nfailed, 0);

    u32 back[AT_NINSERTS];
    test_assert_int_equal (ns_read (db, NULL, back, sizeof back, "read v%u", i), AT_NINSERTS);
    for (u32 k = 0; k < AT_NINSERTS; ++k) {
      test_assert_int_equal (back[k], k);
    }
  }

  test_assert_int_equal (ns_close (db), 0);
}
#endif
