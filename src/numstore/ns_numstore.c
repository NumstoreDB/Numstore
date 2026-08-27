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

#include "core/ns_alloc.h"
#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_numerics.h"
#include "core/ns_stdtypes.h"
#include "core/ns_string.h"
#include "core/os/ns_filesystem.h"
#include "core/os/ns_memory.h"
#include "core/testing/ns_testing.h"
#include "nscore/compiler/ns_compiler.h"
#include "nscore/nsdb/ns_nsdb.h"
#include "nscore/nsdb/ns_nsdb_execute.h"
#include "nscore/types/ns_query.h"
#include "nscore/types/ns_types.h"
#include "nscore/types/ns_variables.h"
#include "numstore/numstore.h"

#include <stdarg.h>
#include <stdio.h>

struct nsdb *
nsdb_open (const char *path)
{
  return nsdb_open_with_resources (path, default_mem (), default_filesystem ());
}

sb_size
nsdb_fexecute (nsdb_t *nh, ns_txn_t *txn, const char *query, void *data, ...)
{
  // Initialize a local allocator
  ALLOC_INIT (alloc);

  sb_size ret; // return variable
  va_list ap, ap2;

  // Reset errors before proceeding
  nh->e.cause_code = 0;
  nh->e.cmlen      = 0;

  // Compute the length the formatted query needs, without writing anything.
  va_start (ap, data);
  va_copy (ap2, ap);
  i32 qlen = vsnprintf (NULL, 0, query, ap);
  va_end (ap);
  if (qlen < 0) {
    va_end (ap2);
    ret = error_causef (&nh->e, ERR_INVALID_ARGUMENT, "Invalid printf argument");
    goto theend;
  }

  // Allocate buffer for the query
  char *buf = allocate (&alloc, (size_t)qlen + 1, 1, &nh->e);
  if (!buf) {
    va_end (ap2);
    ret = error_trace (&nh->e);
    goto theend;
  }

  // Actually write the formatted query into buf.
  qlen = vsnprintf (buf, (size_t)qlen + 1, query, ap2);
  ASSERT (qlen >= 0);
  va_end (ap2);

  // Compile the query
  struct query q; // The AST
  if (compile_query (&q, buf, &alloc, &nh->e)) {
    ret = error_trace (&nh->e);
    goto theend;
  }
  ret = nsdb_execute_on_buffer (nh, txn, &q, data, &alloc);

theend:
  ALLOC_CLOSE (alloc);
  return ret;
}

#ifdef TESTING
TEST (nsdb_fexecute)
{
  ALLOC_INIT (alloc);
  error e = error_create ();

  nsdb_cleanup ("test");
  struct nsdb     *db  = nsdb_open ("test");
  struct nsdb_var *var = NULL;

  nsdb_fexecute (db, NULL, "get %s", &var, "a");
  test_assert_equal (var, NULL);

  nsdb_fexecute (db, NULL, "create %s %s", NULL, "a", "u32");
  nsdb_fexecute (db, NULL, "get %s", &var, "a");
  test_assert (var != NULL);

  test_assert (string_equal (var->var->vname, strfcstr ("a")));
  test_assert (type_equal (var->var->dtype, compile_type_alloc ("u32", &alloc, &e)));
  test_assert_equal (nsdb_var_len (var), 0);
  nsdb_var_free (db, var);

  nsdb_close (db);
  ALLOC_CLOSE (alloc);
}
#endif

b_size
nsdb_var_len (nsdb_var_t *var)
{
  return var->var->nbytes / type_byte_size (var->var->dtype);
}

void
nsdb_var_free (nsdb_t *db, nsdb_var_t *var)
{
  struct allocator *alloc = var->alloc;
  allocator_free (alloc);
  i_free (db->mem, alloc);
}

#ifdef TESTING
TEST (regression_cgd_test_create_delete_rollback_delete)
{
  test_assert_int_equal (nsdb_cleanup ("test"), 0);
  nsdb_t *db = nsdb_open ("test");
  test_assert (db != NULL);

  // Create the variable
  test_assert_int_equal (
      nsdb_fexecute (db, NULL, "create n8Si3C union { tok6UW u32, YGhr cf128, LDzpWVm f16 }", NULL),
      0
  );

  // The culprit txn
  struct ns_txn *tx = nsdb_begin (db);
  test_assert (tx != NULL);
  test_assert_int_equal (nsdb_fexecute (db, tx, "delete n8Si3C", NULL), 0);
  test_assert_int_equal (nsdb_rollback (db, tx), 0);

  // Do something (seemingly unrelated)
  tx = nsdb_begin (db);
  test_assert (tx != NULL);
  test_assert_int_equal (
      nsdb_fexecute (
          db,
          tx,
          "create yJIF "
          "struct { sQf8W7t6 struct { ukc7C4 cf256, CHbmDuiD6 union { aVmHRo "
          "cf64, FeVvpnN u64 } } }",
          NULL
      ),
      0
  );
  test_assert_int_equal (nsdb_commit (db, tx), 0);

  // This failed - it shouldn't because we roll'ed back our previous delete
  //      CAUSE:
  //          pgr_delete_and_release was setting the page in the fsm log
  //          to the page being released, not the fsm - this came from a
  //          refactor - I used to do that
  //          also it never included the bit in the log
  test_assert (nsdb_fexecute (db, NULL, "delete n8Si3C", NULL) == 0);

  test_assert_int_equal (nsdb_close (db), 0);
}

TEST (regression_cgd_test_create_crash_close_delete)
{
  test_assert_int_equal (nsdb_cleanup ("test"), 0);
  nsdb_t *db = nsdb_open ("test");
  test_assert (db != NULL);

  // Create
  test_assert_int_equal (nsdb_fexecute (db, NULL, "create MkWMJ9a [8][9][3][3] i16", NULL), 0);

  // Crash
  test_assert_int_equal (nsdb_crash (db), 0);
  db = nsdb_open ("test");
  test_assert (db != NULL);

  // Close
  test_assert_int_equal (nsdb_close (db), 0);
  db = nsdb_open ("test");
  test_assert (db != NULL);

  // This Failed - it shouldnt
  //        CAUSE:
  //          The first log of fsm is a fsm update log. But the fsm page starts
  //          uninitialized, therefore it needs one upfront physical log first
  //          before it can be used - log a physical update log then continue on
  //          with fsm specific logs
  test_assert (nsdb_fexecute (db, NULL, "delete MkWMJ9a", NULL) == 0);

  test_assert_int_equal (nsdb_close (db), 0);
}

TEST (regression_irwr_rollback_invalid_wal_header)
{
  test_assert_int_equal (nsdb_cleanup ("test"), 0);
  nsdb_t *db = nsdb_open ("test");
  test_assert (db != NULL);

  // TXN 1 (auto)
  test_assert_int_equal (nsdb_fexecute (db, NULL, "create testvar u32", NULL), 0);

  // TXN 2
  struct ns_txn *tx = nsdb_begin (db);
  test_assert (tx != NULL);
  test_assert_int_equal (nsdb_rollback (db, tx), 0);

  // TXN 3
  tx = nsdb_begin (db);
  test_assert (tx != NULL);
  test_assert_int_equal (nsdb_commit (db, tx), 0);

  // TXN 4 (auto): INSERT ofst=0 nelem=53797
  {
    u32 *data = i_malloc (mem, 53797 * sizeof (u32), 1, NULL);
    test_assert (data != NULL);
    for (int i = 0; i < 53797; ++i) {
      data[i] = (u32)randu32 ();
    }
    test_assert_int_equal (nsdb_fexecute (db, NULL, "insert testvar %d %d", data, 0, 53797), 53797);
    i_free (mem, data);
  }

  // TXN 5 (auto): WRITE start=23070 stride=7888 stop=54622 nelems=4
  {
    u32 data[4];
    for (int i = 0; i < 4; ++i) {
      data[i] = (u32)randu32 ();
    }
    test_assert_int_equal (nsdb_fexecute (db, NULL, "write testvar[23070:54622:7888]", data), 4);
  }

  // TXN 6: REMOVE start=5512 stride=13648 stop=32808 nelems=2 → COMMIT
  tx = nsdb_begin (db);
  test_assert (tx != NULL);
  {
    u32 removed[2];
    test_assert_int_equal (nsdb_fexecute (db, tx, "remove testvar[5512:32808:13648]", removed), 2);
  }
  test_assert_int_equal (nsdb_commit (db, tx), 0);

  // TXN 7 (auto): WRITE start=50236 stride=283 stop=51085 nelems=3
  {
    u32 data[3];
    for (int i = 0; i < 3; ++i) {
      data[i] = (u32)randu32 ();
    }
    test_assert_int_equal (nsdb_fexecute (db, NULL, "write testvar[50236:51085:283]", data), 3);
  }

  // TXN 8
  tx = nsdb_begin (db);
  test_assert (tx != NULL);
  test_assert_int_equal (nsdb_rollback (db, tx), 0);

  // TXN 9 (auto): REMOVE start=51429 stride=1931 stop=55291 nelems=2
  {
    u32 removed[2];
    test_assert_int_equal (
        nsdb_fexecute (db, NULL, "remove testvar[51429:55291:1931]", removed),
        2
    );
  }

  // TXN 10 (auto): READ start=1632 stride=9623 stop=20878 nelems=2
  {
    u32 buf[2];
    test_assert_int_equal (nsdb_fexecute (db, NULL, "read testvar[1632:20878:9623]", buf), 2);
  }

  // TXN 11 (auto): READ start=48723 stride=4036 stop=56795 nelems=2
  {
    u32 buf[2];
    test_assert_int_equal (nsdb_fexecute (db, NULL, "read testvar[48723:56795:4036]", buf), 2);
  }

  // TXN 12
  tx = nsdb_begin (db);
  test_assert (tx != NULL);
  test_assert_int_equal (nsdb_commit (db, tx), 0);

  // TXN 13 → ROLLBACK triggers invalid wal header bug
  tx = nsdb_begin (db);
  test_assert (tx != NULL);

  // WRITE start=49014 stride=3051 stop=52065 nelems=1
  {
    u32 data[1] = {(u32)randu32 ()};
    test_assert_int_equal (nsdb_fexecute (db, tx, "write testvar[49014:52065:3051]", data), 1);
  }

  // INSERT ofst=22727 nelem=73857
  {
    u32 *data = i_malloc (mem, 73857 * sizeof (u32), 1, NULL);
    test_assert (data != NULL);
    for (int i = 0; i < 73857; ++i) {
      data[i] = (u32)randu32 ();
    }
    test_assert_int_equal (
        nsdb_fexecute (db, tx, "insert testvar %d %d", data, 22727, 73857),
        73857
    );
    i_free (mem, data);
  }

  // REMOVE start=5509 stride=92363 stop=190235 nelems=2
  {
    u32 removed[2];
    test_assert_int_equal (nsdb_fexecute (db, tx, "remove testvar[5509:190235:92363]", removed), 2);
  }

  // INSERT ofst=8986 nelem=15959
  {
    u32 *data = i_malloc (mem, 15959 * sizeof (u32), 1, NULL);
    test_assert (data != NULL);
    for (int i = 0; i < 15959; ++i) {
      data[i] = (u32)randu32 ();
    }
    test_assert_int_equal (
        nsdb_fexecute (db, tx, "insert testvar %d %d", data, 8986, 15959),
        15959
    );
    i_free (mem, data);
  }

  // READ start=118059 stride=13676 stop=145411 nelems=2
  {
    u32 buf[2];
    test_assert_int_equal (nsdb_fexecute (db, tx, "read testvar[118059:145411:13676]", buf), 2);
  }

  // WRITE start=58530 stride=22447 stop=103424 nelems=2
  {
    u32 data[2];
    for (int i = 0; i < 2; ++i) {
      data[i] = (u32)randu32 ();
    }
    test_assert_int_equal (nsdb_fexecute (db, tx, "write testvar[58530:103424:22447]", data), 2);
  }

  // INSERT ofst=29193 nelem=27045
  {
    u32 *data = i_malloc (mem, 27045 * sizeof (u32), 1, NULL);
    test_assert (data != NULL);
    for (int i = 0; i < 27045; ++i) {
      data[i] = (u32)randu32 ();
    }
    test_assert_int_equal (
        nsdb_fexecute (db, tx, "insert testvar %d %d", data, 29193, 27045),
        27045
    );
    i_free (mem, data);
  }

  // READ start=39413 stride=49536 stop=88949 nelems=1
  {
    u32 buf[1];
    test_assert_int_equal (nsdb_fexecute (db, tx, "read testvar[39413:88949:49536]", buf), 1);
  }

  // This failed -
  //      CAUSE:
  //          The threading logic was wrong - I just made the WAL single
  //          threaded instead
  test_assert_int_equal (nsdb_rollback (db, tx), 0);

  test_assert_int_equal (nsdb_close (db), 0);
}

#endif

#ifdef TESTING
#  define ITERS        10
#  define REOPEN_ITERS 20

TEST (nsdb_create_txn)
{
  TEST_CASE ("create_commit_persists_across_reopen")
  {
    test_assert_int_equal (nsdb_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    struct ns_txn *tx = nsdb_begin (db);
    test_assert (tx != NULL);
    test_assert_int_equal (nsdb_fexecute (db, tx, "create foo u32", NULL), 0);
    test_assert_int_equal (nsdb_commit (db, tx), 0);
    test_assert_int_equal (nsdb_close (db), 0);

    db = nsdb_open ("test");
    test_assert (db != NULL);
    nsdb_var_t *var;
    test_assert_int_equal (nsdb_fexecute (db, NULL, "get if exists foo", &var), 0);
    test_assert_int_equal (nsdb_var_len (var), 0);
    nsdb_var_free (db, var);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("create_rollback_var_not_visible")
  {
    test_assert_int_equal (nsdb_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    struct ns_txn *tx = nsdb_begin (db);
    test_assert (tx != NULL);
    test_assert_int_equal (nsdb_fexecute (db, tx, "create foo u32", NULL), 0);
    test_assert_int_equal (nsdb_rollback (db, tx), 0);
    nsdb_var_t *var;
    test_assert (nsdb_fexecute (db, NULL, "get foo", &var) != 0);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("create_rollback_same_name_succeeds")
  {
    test_assert_int_equal (nsdb_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    struct ns_txn *tx = nsdb_begin (db);
    test_assert (tx != NULL);
    test_assert_int_equal (nsdb_fexecute (db, tx, "create foo u32", NULL), 0);
    test_assert_int_equal (nsdb_rollback (db, tx), 0);
    test_assert_int_equal (nsdb_fexecute (db, NULL, "create foo u32", NULL), 0);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("create_sequential_commits_all_persist")
  {
    test_assert_int_equal (nsdb_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i) {
      struct ns_txn *tx = nsdb_begin (db);
      test_assert (tx != NULL);
      test_assert_int_equal (nsdb_fexecute (db, tx, "create var_%d u32", NULL, i), 0);
      test_assert_int_equal (nsdb_commit (db, tx), 0);
    }
    for (int i = 0; i < ITERS; ++i) {
      nsdb_var_t *var = NULL;
      test_assert_int_equal (nsdb_fexecute (db, NULL, "get if exists foo", &var), 0);
      test_assert (var == NULL);
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("create_alternating_commit_rollback")
  {
    test_assert_int_equal (nsdb_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i) {
      struct ns_txn *tx = nsdb_begin (db);
      test_assert (tx != NULL);
      test_assert_int_equal (nsdb_fexecute (db, tx, "create var_%d u32", NULL, i), 0);
      if (i % 2 == 0) {
        test_assert_int_equal (nsdb_commit (db, tx), 0);
      } else {
        test_assert_int_equal (nsdb_rollback (db, tx), 0);
      }
    }
    for (int i = 0; i < ITERS; ++i) {
      nsdb_var_t *var;
      if (i % 2 == 0) {
        test_assert_int_equal (nsdb_fexecute (db, NULL, "get if exists foo", &var), SUCCESS);
        test_assert (var == NULL);
      } else {
        test_assert_int_equal (nsdb_fexecute (db, NULL, "get if exists foo", &var), SUCCESS);
        test_assert (var == NULL);
      }
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("create_new_var_always_empty")
  {
    test_assert_int_equal (nsdb_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (nsdb_fexecute (db, NULL, "create var_%d u32", NULL, i), 0);
      nsdb_var_t *var;
      test_assert_int_equal (nsdb_fexecute (db, NULL, "get if exists foo", &var), SUCCESS);
      test_assert (var == NULL);
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("create_duplicate_fails")
  {
    test_assert_int_equal (nsdb_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (nsdb_fexecute (db, NULL, "create var_%d u32", NULL, i), 0);
      test_assert (nsdb_fexecute (db, NULL, "create var_%d u32", NULL, i) == SUCCESS);
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("create_rollback_N_times_then_commit")
  {
    test_assert_int_equal (nsdb_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i) {
      struct ns_txn *tx = nsdb_begin (db);
      test_assert (tx != NULL);
      test_assert_int_equal (nsdb_fexecute (db, tx, "create foo u32", NULL), 0);
      test_assert_int_equal (nsdb_rollback (db, tx), 0);
      nsdb_var_t *var;
      test_assert_int_equal (nsdb_fexecute (db, NULL, "get if exists foo", &var), SUCCESS);
      test_assert (var == NULL);
    }
    struct ns_txn *tx = nsdb_begin (db);
    test_assert (tx != NULL);
    test_assert_int_equal (nsdb_fexecute (db, tx, "create foo u32", NULL), 0);
    test_assert_int_equal (nsdb_commit (db, tx), 0);
    nsdb_var_t *var;
    test_assert_int_equal (nsdb_fexecute (db, NULL, "get if exists foo", &var), 0);
    test_assert_int_equal (nsdb_var_len (var), 0);
    nsdb_var_free (db, var);
    test_assert_int_equal (nsdb_close (db), 0);
  }
}

TEST (nsdb_delete_txn)
{
  TEST_CASE ("create_delete_rollback_delete_again")
  {
    test_assert_int_equal (nsdb_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_fexecute (db, NULL, "create foo u32", NULL), 0);
    struct ns_txn *tx = nsdb_begin (db);
    test_assert (tx != NULL);
    test_assert_int_equal (nsdb_fexecute (db, tx, "delete foo", NULL), 0);
    test_assert_int_equal (nsdb_rollback (db, tx), 0);
    test_assert_int_equal (nsdb_fexecute (db, NULL, "delete foo", NULL), 0);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("delete_commit_var_not_visible")
  {
    test_assert_int_equal (nsdb_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (nsdb_fexecute (db, NULL, "create var_%d u32", NULL, i), 0);
    }
    for (int i = 0; i < ITERS; ++i) {
      struct ns_txn *tx = nsdb_begin (db);
      test_assert (tx != NULL);
      test_assert_int_equal (nsdb_fexecute (db, tx, "delete var", NULL), ERR_VARIABLE_NE);
      nsdb_var_t *var;
      test_assert (nsdb_fexecute (db, tx, "get var", &var) != 0);
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("delete_rollback_var_and_data_survive")
  {
    test_assert_int_equal (nsdb_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_fexecute (db, NULL, "create foo u32", NULL), 0);

    u32 *src = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      src[i] = (u32)randu32 ();
    }
    test_assert_int_equal (nsdb_fexecute (db, NULL, "insert foo %d %d", src, 0, ITERS), ITERS);

    for (int i = 0; i < ITERS; ++i) {
      struct ns_txn *tx = nsdb_begin (db);
      test_assert (tx != NULL);
      test_assert_int_equal (nsdb_fexecute (db, tx, "delete foo", NULL), 0);
      test_assert_int_equal (nsdb_rollback (db, tx), 0);

      nsdb_var_t *var;
      test_assert_int_equal (nsdb_fexecute (db, NULL, "get foo", &var), 0);
      test_assert_int_equal (nsdb_var_len (var), ITERS);
      nsdb_var_free (db, var);

      u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
      nsdb_fexecute (db, NULL, "read foo[:]", dst);
      for (int j = 0; j < ITERS; ++j) {
        test_assert_int_equal (dst[j], src[j]);
      }
      i_free (mem, dst);
    }
    i_free (mem, src);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("delete_nonexistent_fails")
  {
    test_assert_int_equal (nsdb_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (nsdb_fexecute (db, NULL, "delete var_%d", NULL, i), ERR_VARIABLE_NE);
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("delete_twice_fails")
  {
    test_assert_int_equal (nsdb_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (nsdb_fexecute (db, NULL, "create var_%d u32", NULL, i), 0);
      test_assert (nsdb_fexecute (db, NULL, "delete var", NULL) == ERR_VARIABLE_NE);
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }
}

TEST (nsdb_insert_txn)
{
  TEST_CASE ("insert_commit_data_persists")
  {
    test_assert_int_equal (nsdb_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_fexecute (db, NULL, "create foo u32", NULL), 0);

    u32 *src = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      src[i] = (u32)randu32 ();
    }

    struct ns_txn *tx = nsdb_begin (db);
    test_assert (tx != NULL);
    test_assert_int_equal (nsdb_fexecute (db, tx, "insert foo %d %d", src, 0, ITERS), ITERS);
    test_assert_int_equal (nsdb_commit (db, tx), 0);

    nsdb_var_t *var;
    test_assert_int_equal (nsdb_fexecute (db, NULL, "get foo", &var), 0);
    test_assert_int_equal (nsdb_var_len (var), ITERS);
    nsdb_var_free (db, var);

    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    nsdb_fexecute (db, NULL, "read foo[:]", dst);
    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (dst[i], src[i]);
    }
    i_free (mem, src);
    i_free (mem, dst);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("insert_rollback_len_unchanged")
  {
    test_assert_int_equal (nsdb_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_fexecute (db, NULL, "create foo u32", NULL), 0);

    nsdb_var_t *var;
    test_assert_int_equal (nsdb_fexecute (db, NULL, "get foo", &var), 0);
    test_assert_int_equal (nsdb_var_len (var), 0);
    nsdb_var_free (db, var);

    u32 *src = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      src[i] = (u32)randu32 ();
    }

    struct ns_txn *tx = nsdb_begin (db);
    test_assert (tx != NULL);
    test_assert_int_equal (nsdb_fexecute (db, tx, "insert foo %d %d", src, 0, ITERS), ITERS);
    test_assert_int_equal (nsdb_rollback (db, tx), 0);

    test_assert_int_equal (nsdb_fexecute (db, NULL, "get foo", &var), 0);
    test_assert_int_equal (nsdb_var_len (var), 0);
    nsdb_var_free (db, var);

    i_free (mem, src);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("insert_rollback_data_reverts")
  {
    test_assert_int_equal (nsdb_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_fexecute (db, NULL, "create foo u32", NULL), 0);

    u32 *initial = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      initial[i] = (u32)randu32 ();
    }
    test_assert_int_equal (nsdb_fexecute (db, NULL, "insert foo %d %d", initial, 0, ITERS), ITERS);

    u32 *extra = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      extra[i] = (u32)randu32 ();
    }

    struct ns_txn *tx = nsdb_begin (db);
    test_assert (tx != NULL);
    test_assert_int_equal (nsdb_fexecute (db, tx, "insert foo %d %d", extra, ITERS, ITERS), ITERS);

    nsdb_var_t *var;
    test_assert_int_equal (nsdb_fexecute (db, tx, "get foo", &var), 0);
    test_assert_int_equal (nsdb_var_len (var), ITERS * 2);
    nsdb_var_free (db, var);

    test_assert_int_equal (nsdb_rollback (db, tx), 0);

    test_assert_int_equal (nsdb_fexecute (db, NULL, "get foo", &var), 0);
    test_assert_int_equal (nsdb_var_len (var), ITERS);
    nsdb_var_free (db, var);

    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    nsdb_fexecute (db, NULL, "read foo[:]", dst);
    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (dst[i], initial[i]);
    }
    i_free (mem, initial);
    i_free (mem, extra);
    i_free (mem, dst);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("insert_returns_count_accumulates_len")
  {
    test_assert_int_equal (nsdb_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_fexecute (db, NULL, "create foo u32", NULL), 0);

    for (int i = 0; i < ITERS; ++i) {
      u32 val = (u32)randu32 ();
      test_assert_int_equal (nsdb_fexecute (db, NULL, "insert foo %d %d", &val, i, 1), 1);
      nsdb_var_t *var;
      test_assert_int_equal (nsdb_fexecute (db, NULL, "get foo", &var), 0);
      test_assert_int_equal (nsdb_var_len (var), i + 1);
      nsdb_var_free (db, var);
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("insert_at_front_preserves_order")
  {
    test_assert_int_equal (nsdb_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_fexecute (db, NULL, "create foo u32", NULL), 0);

    u32 *vals = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      vals[i] = (u32)randu32 ();
    }
    for (int i = ITERS - 1; i >= 0; --i) {
      test_assert_int_equal (nsdb_fexecute (db, NULL, "insert foo %d %d", &vals[i], 0, 1), 1);
    }

    nsdb_var_t *var;
    test_assert_int_equal (nsdb_fexecute (db, NULL, "get foo", &var), 0);
    test_assert_int_equal (nsdb_var_len (var), ITERS);
    nsdb_var_free (db, var);

    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    nsdb_fexecute (db, NULL, "read foo[:]", dst);
    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (dst[i], vals[i]);
    }
    i_free (mem, vals);
    i_free (mem, dst);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("insert_rollback_N_times_data_stable")
  {
    test_assert_int_equal (nsdb_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_fexecute (db, NULL, "create foo u32", NULL), 0);

    u32 *initial = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      initial[i] = (u32)randu32 ();
    }
    test_assert_int_equal (nsdb_fexecute (db, NULL, "insert foo %d %d", initial, 0, ITERS), ITERS);

    u32 *extra = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      extra[i]          = (u32)randu32 ();
      struct ns_txn *tx = nsdb_begin (db);
      test_assert (tx != NULL);
      test_assert_int_equal (nsdb_fexecute (db, tx, "insert foo %d %d", &extra[i], ITERS, 1), 1);
      test_assert_int_equal (nsdb_rollback (db, tx), 0);

      nsdb_var_t *var;
      test_assert_int_equal (nsdb_fexecute (db, NULL, "get foo", &var), 0);
      test_assert_int_equal (nsdb_var_len (var), ITERS);
      nsdb_var_free (db, var);
    }

    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    nsdb_fexecute (db, NULL, "read foo[:]", dst);
    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (dst[i], initial[i]);
    }
    i_free (mem, initial);
    i_free (mem, extra);
    i_free (mem, dst);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("insert_many_vars_independent")
  {
    test_assert_int_equal (nsdb_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);

    u32 *vals = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      vals[i] = (u32)randu32 ();
      test_assert_int_equal (nsdb_fexecute (db, NULL, "create var_%d u32", NULL, i), 0);
      test_assert_int_equal (nsdb_fexecute (db, NULL, "insert var_%d %d %d", &vals[i], i, 0, 1), 1);
    }
    for (int i = 0; i < ITERS; ++i) {
      nsdb_var_t *var;
      test_assert_int_equal (nsdb_fexecute (db, NULL, "get var_%d", &var, i), 0);
      test_assert_int_equal (nsdb_var_len (var), 1);
      nsdb_var_free (db, var);

      u32 dst = 0;
      nsdb_fexecute (db, NULL, "read var_%d[:]", &dst, i);
      test_assert_int_equal (dst, vals[i]);
    }
    i_free (mem, vals);
    test_assert_int_equal (nsdb_close (db), 0);
  }
}

TEST (nsdb_write_txn)
{
  TEST_CASE ("write_commit_data_persists")
  {
    test_assert_int_equal (nsdb_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_fexecute (db, NULL, "create foo u32", NULL), 0);

    u32 *initial = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    u32 *patch   = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      initial[i] = (u32)randu32 ();
      patch[i]   = (u32)randu32 ();
    }
    test_assert_int_equal (nsdb_fexecute (db, NULL, "insert foo %d %d", initial, 0, ITERS), ITERS);

    struct ns_txn *tx = nsdb_begin (db);
    test_assert (tx != NULL);
    nsdb_fexecute (db, tx, "write foo[0:%d:1]", patch, ITERS);
    test_assert_int_equal (nsdb_commit (db, tx), 0);

    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    nsdb_fexecute (db, NULL, "read foo[:]", dst);
    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (dst[i], patch[i]);
    }
    i_free (mem, initial);
    i_free (mem, patch);
    i_free (mem, dst);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("write_rollback_data_reverts")
  {
    test_assert_int_equal (nsdb_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_fexecute (db, NULL, "create foo u32", NULL), 0);

    u32 *initial = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    u32 *patch   = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      initial[i] = (u32)randu32 ();
      patch[i]   = (u32)randu32 ();
    }
    test_assert_int_equal (nsdb_fexecute (db, NULL, "insert foo %d %d", initial, 0, ITERS), ITERS);

    struct ns_txn *tx = nsdb_begin (db);
    test_assert (tx != NULL);
    nsdb_fexecute (db, tx, "write foo[0:%d:1]", patch, ITERS);
    test_assert_int_equal (nsdb_rollback (db, tx), 0);

    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    nsdb_fexecute (db, NULL, "read foo[:]", dst);
    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (dst[i], initial[i]);
    }
    i_free (mem, initial);
    i_free (mem, patch);
    i_free (mem, dst);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("write_does_not_change_len")
  {
    test_assert_int_equal (nsdb_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_fexecute (db, NULL, "create foo u32", NULL), 0);

    u32 *data = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      data[i] = (u32)randu32 ();
    }
    test_assert_int_equal (nsdb_fexecute (db, NULL, "insert foo %d %d", data, 0, ITERS), ITERS);
    i_free (mem, data);

    for (int i = 0; i < ITERS; ++i) {
      u32 val = (u32)randu32 ();
      nsdb_fexecute (db, NULL, "write foo[%d:%d:1]", &val, i, i + 1);

      nsdb_var_t *var;
      test_assert_int_equal (nsdb_fexecute (db, NULL, "get foo", &var), 0);
      test_assert_int_equal (nsdb_var_len (var), ITERS);
      nsdb_var_free (db, var);
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("write_single_element_others_unchanged")
  {
    test_assert_int_equal (nsdb_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_fexecute (db, NULL, "create foo u32", NULL), 0);

    u32 *shadow = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      shadow[i] = (u32)randu32 ();
    }
    test_assert_int_equal (nsdb_fexecute (db, NULL, "insert foo %d %d", shadow, 0, ITERS), ITERS);

    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      int idx     = randu32 () % ITERS;
      u32 val     = (u32)randu32 ();
      shadow[idx] = val;
      nsdb_fexecute (db, NULL, "write foo[%d:%d:1]", &val, idx, idx + 1);
      nsdb_fexecute (db, NULL, "read foo[:]", dst);
      for (int j = 0; j < ITERS; ++j) {
        test_assert_int_equal (dst[j], shadow[j]);
      }
    }
    i_free (mem, shadow);
    i_free (mem, dst);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("write_rollback_N_times_data_stable")
  {
    test_assert_int_equal (nsdb_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_fexecute (db, NULL, "create foo u32", NULL), 0);

    u32 *initial = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      initial[i] = (u32)randu32 ();
    }
    test_assert_int_equal (nsdb_fexecute (db, NULL, "insert foo %d %d", initial, 0, ITERS), ITERS);

    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      int            idx = randu32 () % ITERS;
      u32            val = (u32)randu32 ();
      struct ns_txn *tx  = nsdb_begin (db);
      test_assert (tx != NULL);
      nsdb_fexecute (db, tx, "write foo[%d:%d:1]", &val, idx, idx + 1);
      test_assert_int_equal (nsdb_rollback (db, tx), 0);
      nsdb_fexecute (db, NULL, "read foo[:]", dst);
      for (int j = 0; j < ITERS; ++j) {
        test_assert_int_equal (dst[j], initial[j]);
      }
    }
    i_free (mem, initial);
    i_free (mem, dst);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("write_commit_persists_across_reopen")
  {
    test_assert_int_equal (nsdb_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_fexecute (db, NULL, "create foo u32", NULL), 0);

    u32 *data = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      data[i] = 0;
    }
    test_assert_int_equal (nsdb_fexecute (db, NULL, "insert foo %d %d", data, 0, ITERS), ITERS);

    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < REOPEN_ITERS; ++i) {
      u32 val           = (u32)randu32 ();
      int idx           = randu32 () % ITERS;
      data[idx]         = val;

      struct ns_txn *tx = nsdb_begin (db);
      test_assert (tx != NULL);
      nsdb_fexecute (db, tx, "write foo[%d:%d:1]", &val, idx, idx + 1);
      test_assert_int_equal (nsdb_commit (db, tx), 0);
      test_assert_int_equal (nsdb_close (db), 0);

      db = nsdb_open ("test");
      test_assert (db != NULL);
      nsdb_fexecute (db, NULL, "read foo[:]", dst);
      for (int j = 0; j < ITERS; ++j) {
        test_assert_int_equal (dst[j], data[j]);
      }
    }
    i_free (mem, data);
    i_free (mem, dst);
    test_assert_int_equal (nsdb_close (db), 0);
  }
}
#endif
