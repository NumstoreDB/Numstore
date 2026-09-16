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

#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "core/os/ns_filesystem.h"
#include "core/os/ns_memory.h"
#include "nscore/nsdb/ns_nsdb.h"
#include "nscore/types/ns_types.h"
#include "nscore/variables/ns_variables.h"
#include "numstore/numstore.h"

#ifdef TESTING
#  include "core/testing/ns_testing.h"
#endif

numstore_t *
numstore_open (const char *path)
{
  error       e   = error_create ();
  numstore_t *ret = i_malloc (default_mem (), 1, sizeof *ret, &e);
  if (ret == NULL) {
    return NULL;
  }

  ret->e  = error_create ();
  ret->db = nsdb_open_with_resources (path, default_mem (), default_filesystem (), &ret->e);
  if (ret->db == NULL) {
    i_free (default_mem (), ret);
    return NULL;
  }

  return ret;
}

int
numstore_cleanup (const char *path)
{
  error e = error_create ();
  return nsdb_cleanup (path, &e);
}

int
numstore_close (numstore_t *ns)
{
  int ret = nsdb_close (ns->db, &ns->e);
  i_free (default_mem (), ns);
  return ret;
}

int
numstore_crash (numstore_t *ns)
{
  int ret = nsdb_crash (ns->db, &ns->e);
  i_free (default_mem (), ns);
  return ret;
}

const char *
numstore_strerror (numstore_t *ns)
{
  if (ns->e.cause_code < 0) {
    return ns->e.cause_msg;
  }
  return NULL;
}

int
numstore_perror (numstore_t *ns, const char *prefix)
{
  const char *err = numstore_strerror (ns);
  if (err) {
    return fprintf (stderr, "%s: %s\n", prefix, err);
  }
  return fprintf (stderr, "%s: success\n", prefix);
}

ns_txn_t *
numstore_begin (numstore_t *ns)
{
  return nsdb_begin (ns->db, &ns->e);
}

int
numstore_commit (numstore_t *ns, ns_txn_t *txn)
{
  return nsdb_commit (ns->db, txn, &ns->e);
}

int
numstore_rollback (numstore_t *ns, ns_txn_t *txn)
{
  return nsdb_rollback (ns->db, txn, &ns->e);
}

b_size
numstore_var_len (numstore_var_t *var)
{
  return var->var.nbytes / type_byte_size (var->var.dtype);
}

void
numstore_var_free (numstore_var_t *var)
{
  nsdb_var_free (var);
}

static inline bool
numstore_plan_has_option (const struct numstore_plan *plan, numstore_plan_opt_t opt)
{
  return (plan->options & opt) != 0;
}

static inline err_t
numstore_plan_validate (const struct numstore_plan *plan, error *e)
{
  if (numstore_plan_has_option (plan, NSDB_PLAN_OPT_ALLOCATE_DATA)) {
    if (plan->data != NULL || plan->dlen != 0) {
      return error_causef (
          e,
          ERR_INVALID_ARGUMENT,
          "data/dlen must be NULL/0 when allocate-data mode is enabled"
      );
    }
  }

  if (numstore_plan_has_option (plan, NSDB_PLAN_OPT_CAPTURE_VAR)) {
    if (plan->var != NULL) {
      return error_causef (
          e,
          ERR_INVALID_ARGUMENT,
          "var must be NULL when capture-var mode is enabled"
      );
    }
  }

  return SUCCESS;
}

int
numstore_plan_setopt (struct numstore_plan *plan, numstore_plan_opt_t flag)
{
  plan->options |= flag;
  error e = error_create ();
  return numstore_plan_validate (plan, &e);
}

#ifdef TESTING

// A wrapper to mimic legacy numstore_fexecute syntax.
// New call order: (db, tx, data, dlen, query, ...fmt_args)
// Implemented as a macro (not a function) because C can't forward a bare
// `...` into another variadic function without a va_list-based variant of
// numstore_fexecute — a macro just forwards the tokens directly.
#  define _numstore_fexecute(db, tx, _data, _dlen, query, ...)        \
    ({                                                                \
      struct numstore_plan _plan;                                     \
      memset (&_plan, 0, sizeof (_plan));                             \
      _plan.data = (_data);                                           \
      _plan.dlen = (_dlen);                                           \
      numstore_fexecute ((db), (tx), &_plan, (query), ##__VA_ARGS__); \
    })

TEST (regression_cgd_test_create_delete_rollback_delete)
{
  test_assert_int_equal (numstore_cleanup ("test"), 0);
  numstore_t *db = numstore_open ("test");
  test_assert (db != NULL);

  // Create the variable
  test_assert_int_equal (
      _numstore_fexecute (
          db,
          NULL,
          NULL,
          0,
          "create n8Si3C union { tok6UW u32, YGhr cf128, LDzpWVm f16 }"
      ),
      0
  );

  // The culprit txn
  struct ns_txn *tx = numstore_begin (db);
  test_assert (tx != NULL);
  test_assert_int_equal (_numstore_fexecute (db, tx, NULL, 0, "delete n8Si3C"), 0);
  test_assert_int_equal (numstore_rollback (db, tx), 0);

  // Do something (seemingly unrelated)
  tx = numstore_begin (db);
  test_assert (tx != NULL);
  test_assert_int_equal (
      _numstore_fexecute (
          db,
          tx,
          NULL,
          0,
          "create yJIF "
          "struct { sQf8W7t6 struct { ukc7C4 cf256, CHbmDuiD6 union { aVmHRo "
          "cf64, FeVvpnN u64 } } }"
      ),
      0
  );
  test_assert_int_equal (numstore_commit (db, tx), 0);

  // This failed - it shouldn't because we roll'ed back our previous delete
  //      CAUSE:
  //          pgr_delete_and_release was setting the page in the fsm log
  //          to the page being released, not the fsm - this came from a
  //          refactor - I used to do that
  //          also it never included the bit in the log
  test_assert (_numstore_fexecute (db, NULL, NULL, 0, "delete n8Si3C") == 0);

  test_assert_int_equal (numstore_close (db), 0);
}

TEST (regression_cgd_test_create_crash_close_delete)
{
  test_assert_int_equal (numstore_cleanup ("test"), 0);
  numstore_t *db = numstore_open ("test");
  test_assert (db != NULL);

  // Create
  test_assert_int_equal (
      _numstore_fexecute (db, NULL, NULL, 0, "create MkWMJ9a [8][9][3][3] i16"),
      0
  );

  // Crash
  test_assert_int_equal (numstore_crash (db), 0);
  db = numstore_open ("test");
  test_assert (db != NULL);

  // Close
  test_assert_int_equal (numstore_close (db), 0);
  db = numstore_open ("test");
  test_assert (db != NULL);

  // This Failed - it shouldnt
  //        CAUSE:
  //          The first log of fsm is a fsm update log. But the fsm page starts
  //          uninitialized, therefore it needs one upfront physical log first
  //          before it can be used - log a physical update log then continue on
  //          with fsm specific logs
  test_assert (_numstore_fexecute (db, NULL, NULL, 0, "delete MkWMJ9a") == 0);

  test_assert_int_equal (numstore_close (db), 0);
}

TEST (regression_irwr_rollback_invalid_wal_header)
{
  test_assert_int_equal (numstore_cleanup ("test"), 0);
  numstore_t *db = numstore_open ("test");
  test_assert (db != NULL);

  // TXN 1 (auto)
  test_assert_int_equal (_numstore_fexecute (db, NULL, NULL, 0, "create testvar u32"), 0);

  // TXN 2
  struct ns_txn *tx = numstore_begin (db);
  test_assert (tx != NULL);
  test_assert_int_equal (numstore_rollback (db, tx), 0);

  // TXN 3
  tx = numstore_begin (db);
  test_assert (tx != NULL);
  test_assert_int_equal (numstore_commit (db, tx), 0);

  // TXN 4 (auto): INSERT ofst=0 nelem=53797
  {
    u32 *data = i_malloc (mem, 53797 * sizeof (u32), 1, NULL);
    test_assert (data != NULL);
    for (int i = 0; i < 53797; ++i) {
      data[i] = (u32)randu32 ();
    }
    test_assert_int_equal (
        _numstore_fexecute (db, NULL, data, 0, "insert testvar %d %d", 0, 53797),
        53797
    );
    i_free (mem, data);
  }

  // TXN 5 (auto): WRITE start=23070 stride=7888 stop=54622 nelems=4
  {
    u32 data[4];
    for (int i = 0; i < 4; ++i) {
      data[i] = (u32)randu32 ();
    }
    test_assert_int_equal (
        _numstore_fexecute (db, NULL, data, 0, "write testvar[23070:54622:7888]"),
        4
    );
  }

  // TXN 6: REMOVE start=5512 stride=13648 stop=32808 nelems=2 → COMMIT
  tx = numstore_begin (db);
  test_assert (tx != NULL);
  {
    u32 removed[2];
    test_assert_int_equal (
        _numstore_fexecute (db, tx, removed, 0, "remove testvar[5512:32808:13648]"),
        2
    );
  }
  test_assert_int_equal (numstore_commit (db, tx), 0);

  // TXN 7 (auto): WRITE start=50236 stride=283 stop=51085 nelems=3
  {
    u32 data[3];
    for (int i = 0; i < 3; ++i) {
      data[i] = (u32)randu32 ();
    }
    test_assert_int_equal (
        _numstore_fexecute (db, NULL, data, 0, "write testvar[50236:51085:283]"),
        3
    );
  }

  // TXN 8
  tx = numstore_begin (db);
  test_assert (tx != NULL);
  test_assert_int_equal (numstore_rollback (db, tx), 0);

  // TXN 9 (auto): REMOVE start=51429 stride=1931 stop=55291 nelems=2
  {
    u32 removed[2];
    test_assert_int_equal (
        _numstore_fexecute (db, NULL, removed, 0, "remove testvar[51429:55291:1931]"),
        2
    );
  }

  // TXN 10 (auto): READ start=1632 stride=9623 stop=20878 nelems=2
  {
    u32 buf[2];
    test_assert_int_equal (
        _numstore_fexecute (db, NULL, buf, 0, "read testvar[1632:20878:9623]"),
        2
    );
  }

  // TXN 11 (auto): READ start=48723 stride=4036 stop=56795 nelems=2
  {
    u32 buf[2];
    test_assert_int_equal (
        _numstore_fexecute (db, NULL, buf, 0, "read testvar[48723:56795:4036]"),
        2
    );
  }

  // TXN 12
  tx = numstore_begin (db);
  test_assert (tx != NULL);
  test_assert_int_equal (numstore_commit (db, tx), 0);

  // TXN 13 → ROLLBACK triggers invalid wal header bug
  tx = numstore_begin (db);
  test_assert (tx != NULL);

  // WRITE start=49014 stride=3051 stop=52065 nelems=1
  {
    u32 data[1] = {(u32)randu32 ()};
    test_assert_int_equal (
        _numstore_fexecute (db, tx, data, 0, "write testvar[49014:52065:3051]"),
        1
    );
  }

  // INSERT ofst=22727 nelem=73857
  {
    u32 *data = i_malloc (mem, 73857 * sizeof (u32), 1, NULL);
    test_assert (data != NULL);
    for (int i = 0; i < 73857; ++i) {
      data[i] = (u32)randu32 ();
    }
    test_assert_int_equal (
        _numstore_fexecute (db, tx, data, 0, "insert testvar %d %d", 22727, 73857),
        73857
    );
    i_free (mem, data);
  }

  // REMOVE start=5509 stride=92363 stop=190235 nelems=2
  {
    u32 removed[2];
    test_assert_int_equal (
        _numstore_fexecute (db, tx, removed, 0, "remove testvar[5509:190235:92363]"),
        2
    );
  }

  // INSERT ofst=8986 nelem=15959
  {
    u32 *data = i_malloc (mem, 15959 * sizeof (u32), 1, NULL);
    test_assert (data != NULL);
    for (int i = 0; i < 15959; ++i) {
      data[i] = (u32)randu32 ();
    }
    test_assert_int_equal (
        _numstore_fexecute (db, tx, data, 0, "insert testvar %d %d", 8986, 15959),
        15959
    );
    i_free (mem, data);
  }

  // READ start=118059 stride=13676 stop=145411 nelems=2
  {
    u32 buf[2];
    test_assert_int_equal (
        _numstore_fexecute (db, tx, buf, 0, "read testvar[118059:145411:13676]"),
        2
    );
  }

  // WRITE start=58530 stride=22447 stop=103424 nelems=2
  {
    u32 data[2];
    for (int i = 0; i < 2; ++i) {
      data[i] = (u32)randu32 ();
    }
    test_assert_int_equal (
        _numstore_fexecute (db, tx, data, 0, "write testvar[58530:103424:22447]"),
        2
    );
  }

  // INSERT ofst=29193 nelem=27045
  {
    u32 *data = i_malloc (mem, 27045 * sizeof (u32), 1, NULL);
    test_assert (data != NULL);
    for (int i = 0; i < 27045; ++i) {
      data[i] = (u32)randu32 ();
    }
    test_assert_int_equal (
        _numstore_fexecute (db, tx, data, 0, "insert testvar %d %d", 29193, 27045),
        27045
    );
    i_free (mem, data);
  }

  // READ start=39413 stride=49536 stop=88949 nelems=1
  {
    u32 buf[1];
    test_assert_int_equal (
        _numstore_fexecute (db, tx, buf, 0, "read testvar[39413:88949:49536]"),
        1
    );
  }

  // This failed -
  //      CAUSE:
  //          The threading logic was wrong - I just made the WAL single
  //          threaded instead
  test_assert_int_equal (numstore_rollback (db, tx), 0);

  test_assert_int_equal (numstore_close (db), 0);
}

#  define ITERS        10
#  define REOPEN_ITERS 20

TEST (numstore_create_txn)
{
  TEST_CASE ("create_commit_persists_across_reopen")
  {
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);
    struct ns_txn *tx = numstore_begin (db);
    test_assert (tx != NULL);
    test_assert_int_equal (_numstore_fexecute (db, tx, NULL, 0, "create foo u32"), 0);
    test_assert_int_equal (numstore_commit (db, tx), 0);
    test_assert_int_equal (numstore_close (db), 0);

    db = numstore_open ("test");
    test_assert (db != NULL);
    numstore_var_t *var;
    test_assert_int_equal (_numstore_fexecute (db, NULL, &var, 0, "get if exists foo"), 0);
    test_assert_int_equal (numstore_var_len (var), 0);
    numstore_var_free (var);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("create_rollback_var_not_visible")
  {
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);
    struct ns_txn *tx = numstore_begin (db);
    test_assert (tx != NULL);
    test_assert_int_equal (_numstore_fexecute (db, tx, NULL, 0, "create foo u32"), 0);
    test_assert_int_equal (numstore_rollback (db, tx), 0);
    numstore_var_t *var;
    test_assert (_numstore_fexecute (db, NULL, &var, 0, "get foo") != 0);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("create_rollback_same_name_succeeds")
  {
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);
    struct ns_txn *tx = numstore_begin (db);
    test_assert (tx != NULL);
    test_assert_int_equal (_numstore_fexecute (db, tx, NULL, 0, "create foo u32"), 0);
    test_assert_int_equal (numstore_rollback (db, tx), 0);
    test_assert_int_equal (_numstore_fexecute (db, NULL, NULL, 0, "create foo u32"), 0);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("create_sequential_commits_all_persist")
  {
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i) {
      struct ns_txn *tx = numstore_begin (db);
      test_assert (tx != NULL);
      test_assert_int_equal (_numstore_fexecute (db, tx, NULL, 0, "create var_%d u32", i), 0);
      test_assert_int_equal (numstore_commit (db, tx), 0);
    }
    for (int i = 0; i < ITERS; ++i) {
      numstore_var_t *var = NULL;
      test_assert_int_equal (_numstore_fexecute (db, NULL, &var, 0, "get if exists foo"), 0);
      test_assert (var == NULL);
    }
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("create_alternating_commit_rollback")
  {
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i) {
      struct ns_txn *tx = numstore_begin (db);
      test_assert (tx != NULL);
      test_assert_int_equal (_numstore_fexecute (db, tx, NULL, 0, "create var_%d u32", i), 0);
      if (i % 2 == 0) {
        test_assert_int_equal (numstore_commit (db, tx), 0);
      } else {
        test_assert_int_equal (numstore_rollback (db, tx), 0);
      }
    }
    for (int i = 0; i < ITERS; ++i) {
      numstore_var_t *var;
      if (i % 2 == 0) {
        test_assert_int_equal (
            _numstore_fexecute (db, NULL, &var, 0, "get if exists foo"),
            SUCCESS
        );
        test_assert (var == NULL);
      } else {
        test_assert_int_equal (
            _numstore_fexecute (db, NULL, &var, 0, "get if exists foo"),
            SUCCESS
        );
        test_assert (var == NULL);
      }
    }
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("create_new_var_always_empty")
  {
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (_numstore_fexecute (db, NULL, NULL, 0, "create var_%d u32", i), 0);
      numstore_var_t *var;
      test_assert_int_equal (_numstore_fexecute (db, NULL, &var, 0, "get if exists foo"), SUCCESS);
      test_assert (var == NULL);
    }
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("create_duplicate_fails")
  {
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (_numstore_fexecute (db, NULL, NULL, 0, "create var_%d u32", i), 0);
      test_assert (_numstore_fexecute (db, NULL, NULL, 0, "create var_%d u32", i) == SUCCESS);
    }
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("create_rollback_N_times_then_commit")
  {
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i) {
      struct ns_txn *tx = numstore_begin (db);
      test_assert (tx != NULL);
      test_assert_int_equal (_numstore_fexecute (db, tx, NULL, 0, "create foo u32"), 0);
      test_assert_int_equal (numstore_rollback (db, tx), 0);
      numstore_var_t *var;
      test_assert_int_equal (_numstore_fexecute (db, NULL, &var, 0, "get if exists foo"), SUCCESS);
      test_assert (var == NULL);
    }
    struct ns_txn *tx = numstore_begin (db);
    test_assert (tx != NULL);
    test_assert_int_equal (_numstore_fexecute (db, tx, NULL, 0, "create foo u32"), 0);
    test_assert_int_equal (numstore_commit (db, tx), 0);
    numstore_var_t *var;
    test_assert_int_equal (_numstore_fexecute (db, NULL, &var, 0, "get if exists foo"), 0);
    test_assert_int_equal (numstore_var_len (var), 0);
    numstore_var_free (var);
    test_assert_int_equal (numstore_close (db), 0);
  }
}

TEST (numstore_delete_txn)
{
  TEST_CASE ("create_delete_rollback_delete_again")
  {
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (_numstore_fexecute (db, NULL, NULL, 0, "create foo u32"), 0);
    struct ns_txn *tx = numstore_begin (db);
    test_assert (tx != NULL);
    test_assert_int_equal (_numstore_fexecute (db, tx, NULL, 0, "delete foo"), 0);
    test_assert_int_equal (numstore_rollback (db, tx), 0);
    test_assert_int_equal (_numstore_fexecute (db, NULL, NULL, 0, "delete foo"), 0);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("delete_commit_var_not_visible")
  {
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (_numstore_fexecute (db, NULL, NULL, 0, "create var_%d u32", i), 0);
    }
    for (int i = 0; i < ITERS; ++i) {
      struct ns_txn *tx = numstore_begin (db);
      test_assert (tx != NULL);
      test_assert_int_equal (_numstore_fexecute (db, tx, NULL, 0, "delete var"), ERR_VARIABLE_NE);
      numstore_var_t *var;
      test_assert (_numstore_fexecute (db, tx, &var, 0, "get var") != 0);
      test_assert_int_equal (numstore_rollback (db, tx), 0);
    }
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("delete_rollback_var_and_data_survive")
  {
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (_numstore_fexecute (db, NULL, NULL, 0, "create foo u32"), 0);

    u32 *src = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      src[i] = (u32)randu32 ();
    }
    test_assert_int_equal (
        _numstore_fexecute (db, NULL, src, 0, "insert foo %d %d", 0, ITERS),
        ITERS
    );

    for (int i = 0; i < ITERS; ++i) {
      struct ns_txn *tx = numstore_begin (db);
      test_assert (tx != NULL);
      test_assert_int_equal (_numstore_fexecute (db, tx, NULL, 0, "delete foo"), 0);
      test_assert_int_equal (numstore_rollback (db, tx), 0);

      numstore_var_t *var;
      test_assert_int_equal (_numstore_fexecute (db, NULL, &var, 0, "get foo"), 0);
      test_assert_int_equal (numstore_var_len (var), ITERS);
      numstore_var_free (var);

      u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
      _numstore_fexecute (db, NULL, dst, 0, "read foo[:]");
      for (int j = 0; j < ITERS; ++j) {
        test_assert_int_equal (dst[j], src[j]);
      }
      i_free (mem, dst);
    }
    i_free (mem, src);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("delete_nonexistent_fails")
  {
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (
          _numstore_fexecute (db, NULL, NULL, 0, "delete var_%d", i),
          ERR_VARIABLE_NE
      );
    }
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("delete_twice_fails")
  {
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (_numstore_fexecute (db, NULL, NULL, 0, "create var_%d u32", i), 0);
      test_assert (_numstore_fexecute (db, NULL, NULL, 0, "delete var") == ERR_VARIABLE_NE);
    }
    test_assert_int_equal (numstore_close (db), 0);
  }
}

TEST (numstore_insert_txn)
{
  TEST_CASE ("insert_commit_data_persists")
  {
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (_numstore_fexecute (db, NULL, NULL, 0, "create foo u32"), 0);

    u32 *src = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      src[i] = (u32)randu32 ();
    }

    struct ns_txn *tx = numstore_begin (db);
    test_assert (tx != NULL);
    test_assert_int_equal (
        _numstore_fexecute (db, tx, src, 0, "insert foo %d %d", 0, ITERS),
        ITERS
    );
    test_assert_int_equal (numstore_commit (db, tx), 0);

    numstore_var_t *var;
    test_assert_int_equal (_numstore_fexecute (db, NULL, &var, 0, "get foo"), 0);
    test_assert_int_equal (numstore_var_len (var), ITERS);
    numstore_var_free (var);

    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    _numstore_fexecute (db, NULL, dst, 0, "read foo[:]");
    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (dst[i], src[i]);
    }
    i_free (mem, src);
    i_free (mem, dst);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("insert_rollback_len_unchanged")
  {
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (_numstore_fexecute (db, NULL, NULL, 0, "create foo u32"), 0);

    numstore_var_t *var;
    test_assert_int_equal (_numstore_fexecute (db, NULL, &var, 0, "get foo"), 0);
    test_assert_int_equal (numstore_var_len (var), 0);
    numstore_var_free (var);

    u32 *src = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      src[i] = (u32)randu32 ();
    }

    struct ns_txn *tx = numstore_begin (db);
    test_assert (tx != NULL);
    test_assert_int_equal (
        _numstore_fexecute (db, tx, src, 0, "insert foo %d %d", 0, ITERS),
        ITERS
    );
    test_assert_int_equal (numstore_rollback (db, tx), 0);

    test_assert_int_equal (_numstore_fexecute (db, NULL, &var, 0, "get foo"), 0);
    test_assert_int_equal (numstore_var_len (var), 0);
    numstore_var_free (var);

    i_free (mem, src);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("insert_rollback_data_reverts")
  {
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (_numstore_fexecute (db, NULL, NULL, 0, "create foo u32"), 0);

    u32 *initial = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      initial[i] = (u32)randu32 ();
    }
    test_assert_int_equal (
        _numstore_fexecute (db, NULL, initial, 0, "insert foo %d %d", 0, ITERS),
        ITERS
    );

    u32 *extra = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      extra[i] = (u32)randu32 ();
    }

    struct ns_txn *tx = numstore_begin (db);
    test_assert (tx != NULL);
    test_assert_int_equal (
        _numstore_fexecute (db, tx, extra, 0, "insert foo %d %d", ITERS, ITERS),
        ITERS
    );

    numstore_var_t *var;
    test_assert_int_equal (_numstore_fexecute (db, tx, &var, 0, "get foo"), 0);
    test_assert_int_equal (numstore_var_len (var), ITERS * 2);
    numstore_var_free (var);

    test_assert_int_equal (numstore_rollback (db, tx), 0);

    test_assert_int_equal (_numstore_fexecute (db, NULL, &var, 0, "get foo"), 0);
    test_assert_int_equal (numstore_var_len (var), ITERS);
    numstore_var_free (var);

    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    _numstore_fexecute (db, NULL, dst, 0, "read foo[:]");
    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (dst[i], initial[i]);
    }
    i_free (mem, initial);
    i_free (mem, extra);
    i_free (mem, dst);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("insert_returns_count_accumulates_len")
  {
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (_numstore_fexecute (db, NULL, NULL, 0, "create foo u32"), 0);

    for (int i = 0; i < ITERS; ++i) {
      u32 val = (u32)randu32 ();
      test_assert_int_equal (_numstore_fexecute (db, NULL, &val, 0, "insert foo %d %d", i, 1), 1);
      numstore_var_t *var;
      test_assert_int_equal (_numstore_fexecute (db, NULL, &var, 0, "get foo"), 0);
      test_assert_int_equal (numstore_var_len (var), i + 1);
      numstore_var_free (var);
    }
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("insert_at_front_preserves_order")
  {
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (_numstore_fexecute (db, NULL, NULL, 0, "create foo u32"), 0);

    u32 *vals = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      vals[i] = (u32)randu32 ();
    }
    for (int i = ITERS - 1; i >= 0; --i) {
      test_assert_int_equal (
          _numstore_fexecute (db, NULL, &vals[i], 0, "insert foo %d %d", 0, 1),
          1
      );
    }

    numstore_var_t *var;
    test_assert_int_equal (_numstore_fexecute (db, NULL, &var, 0, "get foo"), 0);
    test_assert_int_equal (numstore_var_len (var), ITERS);
    numstore_var_free (var);

    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    _numstore_fexecute (db, NULL, dst, 0, "read foo[:]");
    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (dst[i], vals[i]);
    }
    i_free (mem, vals);
    i_free (mem, dst);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("insert_rollback_N_times_data_stable")
  {
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (_numstore_fexecute (db, NULL, NULL, 0, "create foo u32"), 0);

    u32 *initial = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      initial[i] = (u32)randu32 ();
    }
    test_assert_int_equal (
        _numstore_fexecute (db, NULL, initial, 0, "insert foo %d %d", 0, ITERS),
        ITERS
    );

    u32 *extra = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      extra[i]          = (u32)randu32 ();
      struct ns_txn *tx = numstore_begin (db);
      test_assert (tx != NULL);
      test_assert_int_equal (
          _numstore_fexecute (db, tx, &extra[i], 0, "insert foo %d %d", ITERS, 1),
          1
      );
      test_assert_int_equal (numstore_rollback (db, tx), 0);

      numstore_var_t *var;
      test_assert_int_equal (_numstore_fexecute (db, NULL, &var, 0, "get foo"), 0);
      test_assert_int_equal (numstore_var_len (var), ITERS);
      numstore_var_free (var);
    }

    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    _numstore_fexecute (db, NULL, dst, 0, "read foo[:]");
    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (dst[i], initial[i]);
    }
    i_free (mem, initial);
    i_free (mem, extra);
    i_free (mem, dst);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("insert_many_vars_independent")
  {
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    u32 *vals = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      vals[i] = (u32)randu32 ();
      test_assert_int_equal (_numstore_fexecute (db, NULL, NULL, 0, "create var_%d u32", i), 0);
      test_assert_int_equal (
          _numstore_fexecute (db, NULL, &vals[i], 0, "insert var_%d %d %d", i, 0, 1),
          1
      );
    }
    for (int i = 0; i < ITERS; ++i) {
      numstore_var_t *var;
      test_assert_int_equal (_numstore_fexecute (db, NULL, &var, 0, "get var_%d", i), 0);
      test_assert_int_equal (numstore_var_len (var), 1);
      numstore_var_free (var);

      u32 dst = 0;
      _numstore_fexecute (db, NULL, &dst, 0, "read var_%d[:]", i);
      test_assert_int_equal (dst, vals[i]);
    }
    i_free (mem, vals);
    test_assert_int_equal (numstore_close (db), 0);
  }
}

TEST (numstore_write_txn)
{
  TEST_CASE ("write_commit_data_persists")
  {
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (_numstore_fexecute (db, NULL, NULL, 0, "create foo u32"), 0);

    u32 *initial = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    u32 *patch   = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      initial[i] = (u32)randu32 ();
      patch[i]   = (u32)randu32 ();
    }
    test_assert_int_equal (
        _numstore_fexecute (db, NULL, initial, 0, "insert foo %d %d", 0, ITERS),
        ITERS
    );

    struct ns_txn *tx = numstore_begin (db);
    test_assert (tx != NULL);
    _numstore_fexecute (db, tx, patch, 0, "write foo[0:%d:1]", ITERS);
    test_assert_int_equal (numstore_commit (db, tx), 0);

    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    _numstore_fexecute (db, NULL, dst, 0, "read foo[:]");
    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (dst[i], patch[i]);
    }
    i_free (mem, initial);
    i_free (mem, patch);
    i_free (mem, dst);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("write_rollback_data_reverts")
  {
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (_numstore_fexecute (db, NULL, NULL, 0, "create foo u32"), 0);

    u32 *initial = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    u32 *patch   = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      initial[i] = (u32)randu32 ();
      patch[i]   = (u32)randu32 ();
    }
    test_assert_int_equal (
        _numstore_fexecute (db, NULL, initial, 0, "insert foo %d %d", 0, ITERS),
        ITERS
    );

    struct ns_txn *tx = numstore_begin (db);
    test_assert (tx != NULL);
    _numstore_fexecute (db, tx, patch, 0, "write foo[0:%d:1]", ITERS);
    test_assert_int_equal (numstore_rollback (db, tx), 0);

    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    _numstore_fexecute (db, NULL, dst, 0, "read foo[:]");
    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (dst[i], initial[i]);
    }
    i_free (mem, initial);
    i_free (mem, patch);
    i_free (mem, dst);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("write_does_not_change_len")
  {
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (_numstore_fexecute (db, NULL, NULL, 0, "create foo u32"), 0);

    u32 *data = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      data[i] = (u32)randu32 ();
    }
    test_assert_int_equal (
        _numstore_fexecute (db, NULL, data, 0, "insert foo %d %d", 0, ITERS),
        ITERS
    );
    i_free (mem, data);

    for (int i = 0; i < ITERS; ++i) {
      u32 val = (u32)randu32 ();
      _numstore_fexecute (db, NULL, &val, 0, "write foo[%d:%d:1]", i, i + 1);

      numstore_var_t *var;
      test_assert_int_equal (_numstore_fexecute (db, NULL, &var, 0, "get foo"), 0);
      test_assert_int_equal (numstore_var_len (var), ITERS);
      numstore_var_free (var);
    }
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("write_single_element_others_unchanged")
  {
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (_numstore_fexecute (db, NULL, NULL, 0, "create foo u32"), 0);

    u32 *shadow = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      shadow[i] = (u32)randu32 ();
    }
    test_assert_int_equal (
        _numstore_fexecute (db, NULL, shadow, 0, "insert foo %d %d", 0, ITERS),
        ITERS
    );

    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      int idx     = randu32 () % ITERS;
      u32 val     = (u32)randu32 ();
      shadow[idx] = val;
      _numstore_fexecute (db, NULL, &val, 0, "write foo[%d:%d:1]", idx, idx + 1);
      _numstore_fexecute (db, NULL, dst, 0, "read foo[:]");
      for (int j = 0; j < ITERS; ++j) {
        test_assert_int_equal (dst[j], shadow[j]);
      }
    }
    i_free (mem, shadow);
    i_free (mem, dst);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("write_rollback_N_times_data_stable")
  {
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (_numstore_fexecute (db, NULL, NULL, 0, "create foo u32"), 0);

    u32 *initial = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      initial[i] = (u32)randu32 ();
    }
    test_assert_int_equal (
        _numstore_fexecute (db, NULL, initial, 0, "insert foo %d %d", 0, ITERS),
        ITERS
    );

    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      int            idx = randu32 () % ITERS;
      u32            val = (u32)randu32 ();
      struct ns_txn *tx  = numstore_begin (db);
      test_assert (tx != NULL);
      _numstore_fexecute (db, tx, &val, 0, "write foo[%d:%d:1]", idx, idx + 1);
      test_assert_int_equal (numstore_rollback (db, tx), 0);
      _numstore_fexecute (db, NULL, dst, 0, "read foo[:]");
      for (int j = 0; j < ITERS; ++j) {
        test_assert_int_equal (dst[j], initial[j]);
      }
    }
    i_free (mem, initial);
    i_free (mem, dst);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("write_commit_persists_across_reopen")
  {
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (_numstore_fexecute (db, NULL, NULL, 0, "create foo u32"), 0);

    u32 *data = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      data[i] = 0;
    }
    test_assert_int_equal (
        _numstore_fexecute (db, NULL, data, 0, "insert foo %d %d", 0, ITERS),
        ITERS
    );

    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < REOPEN_ITERS; ++i) {
      u32 val           = (u32)randu32 ();
      int idx           = randu32 () % ITERS;
      data[idx]         = val;

      struct ns_txn *tx = numstore_begin (db);
      test_assert (tx != NULL);
      _numstore_fexecute (db, tx, &val, 0, "write foo[%d:%d:1]", idx, idx + 1);
      test_assert_int_equal (numstore_commit (db, tx), 0);
      test_assert_int_equal (numstore_close (db), 0);

      db = numstore_open ("test");
      test_assert (db != NULL);
      _numstore_fexecute (db, NULL, dst, 0, "read foo[:]");
      for (int j = 0; j < ITERS; ++j) {
        test_assert_int_equal (dst[j], data[j]);
      }
    }
    i_free (mem, data);
    i_free (mem, dst);
    test_assert_int_equal (numstore_close (db), 0);
  }
}
#endif
