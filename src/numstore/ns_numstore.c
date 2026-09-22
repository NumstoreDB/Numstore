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
#include "nscore/algorithms/numstore/ns_numstore_algorithms.h"
#include "nscore/nsdb/ns_nsdb.h"
#include "nscore/types/ns_types.h"
#include "nscore/variables/ns_variables.h"
#include "numstore/numstore.h"

#include <string.h>

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

  // Initialize numstore database
  if (numstore_init_pager (ret->db->p, &ret->e)) {
    nsdb_close (ret->db, &ret->e);
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
    error_reset (&ns->e);
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

t_size
numstore_var_tsize (numstore_var_t *var)
{
  return type_byte_size (var->var.dtype);
}

sb_size
numstore_var_name (numstore_var_t *var, char *dest, size_t size)
{
  size_t needed = (size_t)var->var.vname.len + 1;

  // If you pass null to dest - return
  // the size (like snprintf)
  if (dest == NULL || size == 0) {
    return (sb_size)needed;
  }

  if (size < needed) {
    return ERR_NOMEM;
  }

  memcpy (dest, var->var.vname.data, var->var.vname.len);
  dest[var->var.vname.len] = '\0';
  return (sb_size)var->var.vname.len;
}

sb_size
numstore_var_type (numstore_var_t *var, char *dest, size_t size)
{
  size_t needed = type_get_string_size (var->var.dtype);

  // If you pass null to dest - return
  // the size (like snprintf)
  if (dest == NULL || size == 0) {
    return (sb_size)needed;
  }

  if (size < needed) {
    return ERR_NOMEM;
  }

  type_generate_string (dest, var->var.dtype);
  return (sb_size)strlen (dest);
}

void
numstore_var_free (numstore_var_t *var)
{
  nsdb_var_free (var);
}

#ifdef TESTING

/**
 * A small wrapper that just runs a query,
 * pass in data and dlen. Should be insert read remove
 * or write
 */
static inline sb_size
_numstore_fexecute_simple_with_data (
    numstore_t *ns,
    ns_txn_t   *txn,
    void       *data,
    b_size      dlen,
    const char *query,
    ...
)
{
  va_list ap;
  va_start (ap, query);

  struct numstore_plan plan = {.data = data, .dlen = dlen};
  sb_size              ret  = numstore_vexecute (ns, txn, &plan, query, ap);

  va_end (ap);
  return ret;
}

/**
 * A small wrapper that just runs a query that
 * returns a variable
 */
static inline err_t
_numstore_fexecute_simple_with_var (
    numstore_var_t **dest,
    numstore_t      *ns,
    ns_txn_t        *txn,
    const char      *query,
    ...
)
{
  va_list ap;
  va_start (ap, query);

  struct numstore_plan plan = {.options = NSDB_PLAN_OPT_CAPTURE_VAR};
  sb_size              ret  = numstore_vexecute (ns, txn, &plan, query, ap);

  va_end (ap);

  if (ret < 0) {
    return ret;
  }
  *dest = plan.var;

  return SUCCESS;
}

TEST (regression_cgd_test_create_delete_rollback_delete)
{
  sb_size res;

  // Clean re open database
  test_assert_int_equal (numstore_cleanup ("test"), 0);
  numstore_t *db = numstore_open ("test");
  test_assert (db != NULL);

  // Create the variable
  res = _numstore_fexecute_simple_with_data (
      db,
      NULL,
      NULL,
      0,
      "create n8Si3C union { tok6UW u32, YGhr cf128, LDzpWVm f16 }"
  );
  test_assert_int_equal (res, 0);

  // The culprit txn: delete the variable, then roll it back
  struct ns_txn *tx = numstore_begin (db);
  test_assert (tx != NULL);

  res = _numstore_fexecute_simple_with_data (db, tx, NULL, 0, "delete n8Si3C");
  test_assert_int_equal (res, 0);

  test_assert_int_equal (numstore_rollback (db, tx), 0);

  // Do something (seemingly unrelated)
  tx = numstore_begin (db);
  test_assert (tx != NULL);

  res = _numstore_fexecute_simple_with_data (
      db,
      tx,
      NULL,
      0,
      "create yJIF "
      "struct { sQf8W7t6 struct { ukc7C4 cf256, CHbmDuiD6 union { aVmHRo "
      "cf64, FeVvpnN u64 } } }"
  );
  test_assert_int_equal (res, 0);

  test_assert_int_equal (numstore_commit (db, tx), 0);

  // Delete the variable again
  //
  // This failed - it shouldn't because we roll'ed back our previous delete
  //      CAUSE:
  //          pgr_delete_and_release was setting the page in the fsm log
  //          to the page being released, not the fsm - this came from a
  //          refactor - I used to do that
  //          also it never included the bit in the log
  res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "delete n8Si3C");
  test_assert_int_equal (res, 0);

  // Close database
  test_assert_int_equal (numstore_close (db), 0);
}

TEST (regression_cgd_test_create_crash_close_delete)
{
  sb_size res;

  // Clean re open database
  test_assert_int_equal (numstore_cleanup ("test"), 0);
  numstore_t *db = numstore_open ("test");
  test_assert (db != NULL);

  // Create the variable
  res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create MkWMJ9a [8][9][3][3] i16");
  test_assert_int_equal (res, 0);

  // Crash, then re open
  test_assert_int_equal (numstore_crash (db), 0);
  db = numstore_open ("test");
  test_assert (db != NULL);

  // Cleanly close, then re open
  test_assert_int_equal (numstore_close (db), 0);
  db = numstore_open ("test");
  test_assert (db != NULL);

  // Delete the variable
  //
  // This Failed - it shouldnt
  //        CAUSE:
  //          The first log of fsm is a fsm update log. But the fsm page starts
  //          uninitialized, therefore it needs one upfront physical log first
  //          before it can be used - log a physical update log then continue on
  //          with fsm specific logs
  res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "delete MkWMJ9a");
  test_assert_int_equal (res, 0);

  // Close database
  test_assert_int_equal (numstore_close (db), 0);
}

TEST (regression_irwr_rollback_invalid_wal_header)
{
  sb_size        res;
  struct ns_txn *tx;

  // Clean re open database
  test_assert_int_equal (numstore_cleanup ("test"), 0);
  numstore_t *db = numstore_open ("test");
  test_assert (db != NULL);

  // TXN 1 (auto): create the variable
  res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create testvar u32");
  test_assert_int_equal (res, 0);

  // TXN 2: empty, rolled back
  tx = numstore_begin (db);
  test_assert (tx != NULL);
  test_assert_int_equal (numstore_rollback (db, tx), 0);

  // TXN 3: empty, committed
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

    res = _numstore_fexecute_simple_with_data (
        db,
        NULL,
        data,
        53797 * sizeof (u32),
        "insert testvar %d %d",
        0,
        53797
    );
    test_assert_int_equal (res, 53797);

    i_free (mem, data);
  }

  // TXN 5 (auto): WRITE start=23070 stride=7888 stop=54622 nelems=4
  {
    u32 data[4];
    for (int i = 0; i < 4; ++i) {
      data[i] = (u32)randu32 ();
    }

    res = _numstore_fexecute_simple_with_data (
        db,
        NULL,
        data,
        0,
        "write testvar[23070:54622:7888]"
    );
    test_assert_int_equal (res, 4);
  }

  // TXN 6: REMOVE start=5512 stride=13648 stop=32808 nelems=2 -> COMMIT
  tx = numstore_begin (db);
  test_assert (tx != NULL);
  {
    u32 removed[2];

    res = _numstore_fexecute_simple_with_data (
        db,
        tx,
        removed,
        0,
        "remove testvar[5512:32808:13648]"
    );
    test_assert_int_equal (res, 2);
  }
  test_assert_int_equal (numstore_commit (db, tx), 0);

  // TXN 7 (auto): WRITE start=50236 stride=283 stop=51085 nelems=3
  {
    u32 data[3];
    for (int i = 0; i < 3; ++i) {
      data[i] = (u32)randu32 ();
    }

    res = _numstore_fexecute_simple_with_data (db, NULL, data, 0, "write testvar[50236:51085:283]");
    test_assert_int_equal (res, 3);
  }

  // TXN 8: empty, rolled back
  tx = numstore_begin (db);
  test_assert (tx != NULL);
  test_assert_int_equal (numstore_rollback (db, tx), 0);

  // TXN 9 (auto): REMOVE start=51429 stride=1931 stop=55291 nelems=2
  {
    u32 removed[2];

    res = _numstore_fexecute_simple_with_data (
        db,
        NULL,
        removed,
        0,
        "remove testvar[51429:55291:1931]"
    );
    test_assert_int_equal (res, 2);
  }

  // TXN 10 (auto): READ start=1632 stride=9623 stop=20878 nelems=2
  {
    u32 buf[2];

    res = _numstore_fexecute_simple_with_data (db, NULL, buf, 0, "read testvar[1632:20878:9623]");
    test_assert_int_equal (res, 2);
  }

  // TXN 11 (auto): READ start=48723 stride=4036 stop=56795 nelems=2
  {
    u32 buf[2];

    res = _numstore_fexecute_simple_with_data (db, NULL, buf, 0, "read testvar[48723:56795:4036]");
    test_assert_int_equal (res, 2);
  }

  // TXN 12: empty, committed
  tx = numstore_begin (db);
  test_assert (tx != NULL);
  test_assert_int_equal (numstore_commit (db, tx), 0);

  // TXN 13: many operations, then ROLLBACK triggers the invalid wal header bug
  tx = numstore_begin (db);
  test_assert (tx != NULL);

  // WRITE start=49014 stride=3051 stop=52065 nelems=1
  {
    u32 data[1] = {(u32)randu32 ()};

    res = _numstore_fexecute_simple_with_data (db, tx, data, 0, "write testvar[49014:52065:3051]");
    test_assert_int_equal (res, 1);
  }

  // INSERT ofst=22727 nelem=73857
  {
    u32 *data = i_malloc (mem, 73857 * sizeof (u32), 1, NULL);
    test_assert (data != NULL);
    for (int i = 0; i < 73857; ++i) {
      data[i] = (u32)randu32 ();
    }

    res = _numstore_fexecute_simple_with_data (
        db,
        tx,
        data,
        0,
        "insert testvar %d %d",
        22727,
        73857
    );
    test_assert_int_equal (res, 73857);

    i_free (mem, data);
  }

  // REMOVE start=5509 stride=92363 stop=190235 nelems=2
  {
    u32 removed[2];

    res = _numstore_fexecute_simple_with_data (
        db,
        tx,
        removed,
        0,
        "remove testvar[5509:190235:92363]"
    );
    test_assert_int_equal (res, 2);
  }

  // INSERT ofst=8986 nelem=15959
  {
    u32 *data = i_malloc (mem, 15959 * sizeof (u32), 1, NULL);
    test_assert (data != NULL);
    for (int i = 0; i < 15959; ++i) {
      data[i] = (u32)randu32 ();
    }

    res = _numstore_fexecute_simple_with_data (
        db,
        tx,
        data,
        0,
        "insert testvar %d %d",
        8986,
        15959
    );
    test_assert_int_equal (res, 15959);

    i_free (mem, data);
  }

  // READ start=118059 stride=13676 stop=145411 nelems=2
  {
    u32 buf[2];

    res = _numstore_fexecute_simple_with_data (db, tx, buf, 0, "read testvar[118059:145411:13676]");
    test_assert_int_equal (res, 2);
  }

  // WRITE start=58530 stride=22447 stop=103424 nelems=2
  {
    u32 data[2];
    for (int i = 0; i < 2; ++i) {
      data[i] = (u32)randu32 ();
    }

    res = _numstore_fexecute_simple_with_data (
        db,
        tx,
        data,
        0,
        "write testvar[58530:103424:22447]"
    );
    test_assert_int_equal (res, 2);
  }

  // INSERT ofst=29193 nelem=27045
  {
    u32 *data = i_malloc (mem, 27045 * sizeof (u32), 1, NULL);
    test_assert (data != NULL);
    for (int i = 0; i < 27045; ++i) {
      data[i] = (u32)randu32 ();
    }

    res = _numstore_fexecute_simple_with_data (
        db,
        tx,
        data,
        0,
        "insert testvar %d %d",
        29193,
        27045
    );
    test_assert_int_equal (res, 27045);

    i_free (mem, data);
  }

  // READ start=39413 stride=49536 stop=88949 nelems=1
  {
    u32 buf[1];

    res = _numstore_fexecute_simple_with_data (db, tx, buf, 0, "read testvar[39413:88949:49536]");
    test_assert_int_equal (res, 1);
  }

  // Rollback the big transaction
  //
  // This failed -
  //      CAUSE:
  //          The threading logic was wrong - I just made the WAL single
  //          threaded instead
  test_assert_int_equal (numstore_rollback (db, tx), 0);

  // Close database
  test_assert_int_equal (numstore_close (db), 0);
}

#  define ITERS        10
#  define REOPEN_ITERS 20

TEST (numstore_create_txn)
{
  TEST_CASE ("Committing a created variable makes it persist across a database reopen")
  {
    sb_size res;
    err_t   err;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create a variable inside a transaction and commit it
    struct ns_txn *tx = numstore_begin (db);
    test_assert (tx != NULL);

    res = _numstore_fexecute_simple_with_data (db, tx, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    test_assert_int_equal (numstore_commit (db, tx), 0);

    // Close and re open the database
    test_assert_int_equal (numstore_close (db), 0);
    db = numstore_open ("test");
    test_assert (db != NULL);

    // The variable should still exist and be empty
    numstore_var_t *var;
    err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get if exists foo");
    test_assert_int_equal (err, SUCCESS);
    test_assert (var != NULL);
    test_assert_int_equal (numstore_var_len (var), 0);
    numstore_var_free (var);

    // Close database
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Rolling back a created variable leaves it not visible")
  {
    sb_size res;
    err_t   err;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Begin transaction
    struct ns_txn *tx = numstore_begin (db);
    test_assert (tx != NULL);

    // Create a new variable
    res = _numstore_fexecute_simple_with_data (db, tx, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    // Rollback
    test_assert_int_equal (numstore_rollback (db, tx), 0);

    // Get the variable (should fail)
    numstore_var_t *var;
    err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get foo");
    test_assert (err != SUCCESS);
    numstore_perror (db, "get foo");

    // Close database
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE (
      "Creating a variable succeeds after an earlier create of the same name was rolled back"
  )
  {
    sb_size res;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create a variable inside a transaction, then roll it back
    struct ns_txn *tx = numstore_begin (db);
    test_assert (tx != NULL);

    res = _numstore_fexecute_simple_with_data (db, tx, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    test_assert_int_equal (numstore_rollback (db, tx), 0);

    // Create a variable with the same name outside of a transaction
    res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    // Close database
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE (
      "Creating %d variables in separate committed transactions makes them all persist",
      ITERS
  )
  {
    sb_size res;
    err_t   err;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create a variable per transaction and commit each
    for (int i = 0; i < ITERS; ++i) {
      struct ns_txn *tx = numstore_begin (db);
      test_assert (tx != NULL);

      res = _numstore_fexecute_simple_with_data (db, tx, NULL, 0, "create var_%d u32", i);
      test_assert_int_equal (res, 0);

      test_assert_int_equal (numstore_commit (db, tx), 0);
    }

    // Check the variables
    for (int i = 0; i < ITERS; ++i) {
      numstore_var_t *var = NULL;
      err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get if exists foo");
      test_assert_int_equal (err, SUCCESS);
      test_assert (var->var.dtype == NULL);
    }

    // Close database
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE (
      "Only committed creates persist when %d transactions alternate commit and rollback",
      ITERS
  )
  {
    sb_size res;
    err_t   err;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create a variable per transaction, committing evens and rolling back odds
    for (int i = 0; i < ITERS; ++i) {
      struct ns_txn *tx = numstore_begin (db);
      test_assert (tx != NULL);

      res = _numstore_fexecute_simple_with_data (db, tx, NULL, 0, "create var_%d u32", i);
      test_assert_int_equal (res, 0);

      if (i % 2 == 0) {
        test_assert_int_equal (numstore_commit (db, tx), 0);
      } else {
        test_assert_int_equal (numstore_rollback (db, tx), 0);
      }
    }

    // NOTE: both branches of the original if/else did the exact same
    // thing ("get if exists foo", which never existed either way), so
    // this was collapsed into one unconditional check.
    for (int i = 0; i < ITERS; ++i) {
      numstore_var_t *var;
      err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get if exists foo");
      test_assert_int_equal (err, SUCCESS);
      test_assert (var->var.dtype == NULL);
    }

    // Close database
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("A newly created variable is always empty")
  {
    sb_size res;
    err_t   err;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i) {
      // Create a variable
      res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create var_%d u32", i);
      test_assert_int_equal (res, 0);

      // Check that it is empty
      numstore_var_t *var;
      err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get if exists foo");
      test_assert_int_equal (err, SUCCESS);
      test_assert (var->var.dtype == NULL);
    }

    // Close database
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Creating a variable that already exists fails")
  {
    sb_size res;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i) {
      // Create a variable
      res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create var_%d u32", i);
      test_assert_int_equal (res, 0);

      // Create the same variable again
      res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create var_%d u32", i);
      test_assert_int_equal (res, SUCCESS);
    }

    // Close database
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE (
      "Rolling back a create %d times and then committing it once leaves the variable empty",
      ITERS
  )
  {
    sb_size res;
    err_t   err;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create the same variable and roll it back, over and over
    for (int i = 0; i < ITERS; ++i) {
      struct ns_txn *tx = numstore_begin (db);
      test_assert (tx != NULL);

      res = _numstore_fexecute_simple_with_data (db, tx, NULL, 0, "create foo u32");
      test_assert_int_equal (res, 0);

      test_assert_int_equal (numstore_rollback (db, tx), 0);

      // The variable should not exist
      numstore_var_t *var;
      err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get if exists foo");
      test_assert_int_equal (err, SUCCESS);
      test_assert (var->var.dtype == NULL);
    }

    // Finally create the variable and commit it
    struct ns_txn *tx = numstore_begin (db);
    test_assert (tx != NULL);

    res = _numstore_fexecute_simple_with_data (db, tx, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    test_assert_int_equal (numstore_commit (db, tx), 0);

    // The variable should exist and be empty
    numstore_var_t *var;
    err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get if exists foo");
    test_assert_int_equal (err, SUCCESS);
    test_assert (var != NULL);
    test_assert_int_equal (numstore_var_len (var), 0);
    numstore_var_free (var);

    // Close database
    test_assert_int_equal (numstore_close (db), 0);
  }
}

TEST (numstore_delete_txn)
{
  TEST_CASE ("Rolling back a delete lets the variable be deleted again")
  {
    sb_size res;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    // Delete the variable inside a transaction, then roll it back
    struct ns_txn *tx = numstore_begin (db);
    test_assert (tx != NULL);

    res = _numstore_fexecute_simple_with_data (db, tx, NULL, 0, "delete foo");
    test_assert_int_equal (res, 0);

    test_assert_int_equal (numstore_rollback (db, tx), 0);

    // Delete the variable again, outside of a transaction
    res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "delete foo");
    test_assert_int_equal (res, 0);

    // Close database
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Deleting a variable inside a transaction makes it no longer visible")
  {
    sb_size res;
    err_t   err;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create ITERS variables
    for (int i = 0; i < ITERS; ++i) {
      res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create var_%d u32", i);
      test_assert_int_equal (res, 0);
    }

    // Delete a variable in a transaction, rolling back each time
    for (int i = 0; i < ITERS; ++i) {
      // Begin transaction
      struct ns_txn *tx = numstore_begin (db);
      test_assert (tx != NULL);

      // Delete the variable
      res = _numstore_fexecute_simple_with_data (db, tx, NULL, 0, "delete var");
      test_assert_int_equal (res, ERR_VARIABLE_NE);
      numstore_perror (db, "delete var");

      // Get the variable - should be non existent
      numstore_var_t *var;
      err = _numstore_fexecute_simple_with_var (&var, db, tx, "get var");
      test_assert (err != SUCCESS);
      numstore_perror (db, "delete var");

      // Rollback transaction
      test_assert_int_equal (numstore_rollback (db, tx), 0);
    }

    // Close database
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Rolling back a delete keeps both the variable and its data intact")
  {
    sb_size res;
    err_t   err;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create one variable
    res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    // Insert an array into the variable
    u32 src[ITERS];
    for (int i = 0; i < ITERS; ++i) {
      src[i] = (u32)randu32 ();
    }
    res = _numstore_fexecute_simple_with_data (db, NULL, src, 0, "insert foo %d %d", 0, ITERS);
    test_assert_int_equal (res, ITERS);

    for (int i = 0; i < ITERS; ++i) {
      // Delete the variable inside a transaction, then roll it back
      {
        struct ns_txn *tx = numstore_begin (db);
        test_assert (tx != NULL);

        res = _numstore_fexecute_simple_with_data (db, tx, NULL, 0, "delete foo");
        test_assert_int_equal (res, 0);

        test_assert_int_equal (numstore_rollback (db, tx), 0);
      }

      // The variable should still exist with the same length
      {
        numstore_var_t *var;
        err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get foo");
        test_assert_int_equal (err, SUCCESS);
        test_assert_int_equal (numstore_var_len (var), ITERS);
        numstore_var_free (var);
      }

      // The data should be unchanged
      {
        u32 dst[ITERS];
        res = _numstore_fexecute_simple_with_data (db, NULL, dst, 0, "read foo[:]");
        test_assert_int_equal (res, ITERS);
        for (int j = 0; j < ITERS; ++j) {
          test_assert_int_equal (dst[j], src[j]);
        }
      }
    }

    // Close database
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Deleting a variable that doesn't exist fails with ERR_VARIABLE_NE")
  {
    sb_size res;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i) {
      // Try to delete a non existent variable
      res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "delete var_%d", i);
      test_assert_int_equal (res, ERR_VARIABLE_NE);
      numstore_perror (db, "delete");
    }

    // Close database
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Deleting the same variable twice fails the second time")
  {
    sb_size res;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i) {
      // Create a variable
      res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create var_%d u32", i);
      test_assert_int_equal (res, 0);

      // Delete a non existent variable
      res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "delete var");
      test_assert_int_equal (res, ERR_VARIABLE_NE);
      numstore_perror (db, "delete");

      // Delete the variable we created
      res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "delete var_%d");
      test_assert_int_equal (res, SUCCESS);

      // Delete the variable again - should fail
      res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "delete var_%d");
      test_assert_int_equal (res, ERR_VARIABLE_NE);
      numstore_perror (db, "delete");
    }

    // Close database
    test_assert_int_equal (numstore_close (db), 0);
  }
}

TEST (numstore_insert_txn)
{
  TEST_CASE ("Committing an insert makes the data persist")
  {
    sb_size res;
    err_t   err;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    // Generate random data to insert
    u32 *src = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      src[i] = (u32)randu32 ();
    }

    // Insert the data inside a transaction and commit it
    struct ns_txn *tx = numstore_begin (db);
    test_assert (tx != NULL);

    res = _numstore_fexecute_simple_with_data (db, tx, src, 0, "insert foo %d %d", 0, ITERS);
    test_assert_int_equal (res, ITERS);

    test_assert_int_equal (numstore_commit (db, tx), 0);

    // The variable should have the new length
    numstore_var_t *var;
    err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get foo");
    test_assert_int_equal (err, SUCCESS);
    test_assert_int_equal (numstore_var_len (var), ITERS);
    numstore_var_free (var);

    // Read the data back and compare
    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    _numstore_fexecute_simple_with_data (db, NULL, dst, 0, "read foo[:]");
    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (dst[i], src[i]);
    }

    // Cleanup
    i_free (mem, src);
    i_free (mem, dst);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Rolling back an insert leaves the variable length unchanged")
  {
    sb_size res;
    err_t   err;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    // The variable starts out empty
    numstore_var_t *var;
    err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get foo");
    test_assert_int_equal (err, SUCCESS);
    test_assert_int_equal (numstore_var_len (var), 0);
    numstore_var_free (var);

    // Generate random data to insert
    u32 *src = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      src[i] = (u32)randu32 ();
    }

    // Insert the data inside a transaction, then roll it back
    struct ns_txn *tx = numstore_begin (db);
    test_assert (tx != NULL);

    res = _numstore_fexecute_simple_with_data (db, tx, src, 0, "insert foo %d %d", 0, ITERS);
    test_assert_int_equal (res, ITERS);

    test_assert_int_equal (numstore_rollback (db, tx), 0);

    // The variable should be empty again
    err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get foo");
    test_assert_int_equal (err, SUCCESS);
    test_assert_int_equal (numstore_var_len (var), 0);
    numstore_var_free (var);

    // Cleanup
    i_free (mem, src);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Rolling back an insert reverts the data to its earlier contents")
  {
    sb_size res;
    err_t   err;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    // Insert the initial data
    u32 *initial = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      initial[i] = (u32)randu32 ();
    }
    res = _numstore_fexecute_simple_with_data (db, NULL, initial, 0, "insert foo %d %d", 0, ITERS);
    test_assert_int_equal (res, ITERS);

    // Generate extra data to insert in a transaction
    u32 *extra = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      extra[i] = (u32)randu32 ();
    }

    // Append the extra data inside a transaction
    struct ns_txn *tx = numstore_begin (db);
    test_assert (tx != NULL);

    res = _numstore_fexecute_simple_with_data (db, tx, extra, 0, "insert foo %d %d", ITERS, ITERS);
    test_assert_int_equal (res, ITERS);

    // Inside the transaction the variable should have both sets of data
    numstore_var_t *var;
    err = _numstore_fexecute_simple_with_var (&var, db, tx, "get foo");
    test_assert_int_equal (err, SUCCESS);
    test_assert_int_equal (numstore_var_len (var), ITERS * 2);
    numstore_var_free (var);

    // Rollback
    test_assert_int_equal (numstore_rollback (db, tx), 0);

    // Only the initial data should remain
    err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get foo");
    test_assert_int_equal (err, SUCCESS);
    test_assert_int_equal (numstore_var_len (var), ITERS);
    numstore_var_free (var);

    // Read the data back and compare to the initial data
    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    _numstore_fexecute_simple_with_data (db, NULL, dst, 0, "read foo[:]");
    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (dst[i], initial[i]);
    }

    // Cleanup
    i_free (mem, initial);
    i_free (mem, extra);
    i_free (mem, dst);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Each single element insert returns one and grows the variable length by one")
  {
    sb_size res;
    err_t   err;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    for (int i = 0; i < ITERS; ++i) {
      // Append one element
      u32 val = (u32)randu32 ();
      res     = _numstore_fexecute_simple_with_data (db, NULL, &val, 0, "insert foo %d %d", i, 1);
      test_assert_int_equal (res, 1);

      // The length should have grown by one
      numstore_var_t *var;
      err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get foo");
      test_assert_int_equal (err, SUCCESS);
      test_assert_int_equal (numstore_var_len (var), i + 1);
      numstore_var_free (var);
    }

    // Close database
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Inserting at the front repeatedly preserves the intended element order")
  {
    sb_size res;
    err_t   err;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    // Generate the values we expect to end up with
    u32 *vals = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      vals[i] = (u32)randu32 ();
    }

    // Insert them one at a time at the front, last value first
    for (int i = ITERS - 1; i >= 0; --i) {
      res = _numstore_fexecute_simple_with_data (db, NULL, &vals[i], 0, "insert foo %d %d", 0, 1);
      test_assert_int_equal (res, 1);
    }

    // The variable should have the full length
    numstore_var_t *var;
    err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get foo");
    test_assert_int_equal (err, SUCCESS);
    test_assert_int_equal (numstore_var_len (var), ITERS);
    numstore_var_free (var);

    // Read the data back and compare
    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    _numstore_fexecute_simple_with_data (db, NULL, dst, 0, "read foo[:]");
    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (dst[i], vals[i]);
    }

    // Cleanup
    i_free (mem, vals);
    i_free (mem, dst);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Rolling back %d inserts one after another leaves the data stable", ITERS)
  {
    sb_size res;
    err_t   err;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    // Insert the initial data
    u32 *initial = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      initial[i] = (u32)randu32 ();
    }
    res = _numstore_fexecute_simple_with_data (db, NULL, initial, 0, "insert foo %d %d", 0, ITERS);
    test_assert_int_equal (res, ITERS);

    // Append one element in a transaction and roll it back, over and over
    u32 *extra = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      extra[i]          = (u32)randu32 ();

      struct ns_txn *tx = numstore_begin (db);
      test_assert (tx != NULL);

      res = _numstore_fexecute_simple_with_data (
          db,
          tx,
          &extra[i],
          0,
          "insert foo %d %d",
          ITERS,
          1
      );
      test_assert_int_equal (res, 1);

      test_assert_int_equal (numstore_rollback (db, tx), 0);

      // The length should be back to the initial length
      numstore_var_t *var;
      err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get foo");
      test_assert_int_equal (err, SUCCESS);
      test_assert_int_equal (numstore_var_len (var), ITERS);
      numstore_var_free (var);
    }

    // Read the data back and compare to the initial data
    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    _numstore_fexecute_simple_with_data (db, NULL, dst, 0, "read foo[:]");
    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (dst[i], initial[i]);
    }

    // Cleanup
    i_free (mem, initial);
    i_free (mem, extra);
    i_free (mem, dst);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Inserting into many variables keeps each variable independent")
  {
    sb_size res;
    err_t   err;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create ITERS variables, each with a single random element
    u32 *vals = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      vals[i] = (u32)randu32 ();

      res     = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create var_%d u32", i);
      test_assert_int_equal (res, 0);

      res = _numstore_fexecute_simple_with_data (
          db,
          NULL,
          &vals[i],
          0,
          "insert var_%d %d %d",
          i,
          0,
          1
      );
      test_assert_int_equal (res, 1);
    }

    // Each variable should hold only its own element
    for (int i = 0; i < ITERS; ++i) {
      // Check the length
      numstore_var_t *var;
      err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get var_%d", i);
      test_assert_int_equal (err, SUCCESS);
      test_assert_int_equal (numstore_var_len (var), 1);
      numstore_var_free (var);

      // Check the data
      u32 dst = 0;
      _numstore_fexecute_simple_with_data (db, NULL, &dst, 0, "read var_%d[:]", i);
      test_assert_int_equal (dst, vals[i]);
    }

    // Cleanup
    i_free (mem, vals);
    test_assert_int_equal (numstore_close (db), 0);
  }
}

TEST (numstore_write_txn)
{
  TEST_CASE ("Committing a write makes the new data persist")
  {
    sb_size res;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    // Generate the initial data and the data we will write over it
    u32 *initial = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    u32 *patch   = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      initial[i] = (u32)randu32 ();
      patch[i]   = (u32)randu32 ();
    }

    // Insert the initial data
    res = _numstore_fexecute_simple_with_data (db, NULL, initial, 0, "insert foo %d %d", 0, ITERS);
    test_assert_int_equal (res, ITERS);

    // Overwrite everything inside a transaction and commit it
    struct ns_txn *tx = numstore_begin (db);
    test_assert (tx != NULL);

    _numstore_fexecute_simple_with_data (db, tx, patch, 0, "write foo[0:%d:1]", ITERS);

    test_assert_int_equal (numstore_commit (db, tx), 0);

    // Read the data back and compare to the patch
    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    _numstore_fexecute_simple_with_data (db, NULL, dst, 0, "read foo[:]");
    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (dst[i], patch[i]);
    }

    // Cleanup
    i_free (mem, initial);
    i_free (mem, patch);
    i_free (mem, dst);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Rolling back a write reverts the data to its earlier contents")
  {
    sb_size res;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    // Generate the initial data and the data we will write over it
    u32 *initial = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    u32 *patch   = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      initial[i] = (u32)randu32 ();
      patch[i]   = (u32)randu32 ();
    }

    // Insert the initial data
    res = _numstore_fexecute_simple_with_data (db, NULL, initial, 0, "insert foo %d %d", 0, ITERS);
    test_assert_int_equal (res, ITERS);

    // Overwrite everything inside a transaction, then roll it back
    struct ns_txn *tx = numstore_begin (db);
    test_assert (tx != NULL);

    _numstore_fexecute_simple_with_data (db, tx, patch, 0, "write foo[0:%d:1]", ITERS);

    test_assert_int_equal (numstore_rollback (db, tx), 0);

    // Read the data back and compare to the initial data
    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    _numstore_fexecute_simple_with_data (db, NULL, dst, 0, "read foo[:]");
    for (int i = 0; i < ITERS; ++i) {
      test_assert_int_equal (dst[i], initial[i]);
    }

    // Cleanup
    i_free (mem, initial);
    i_free (mem, patch);
    i_free (mem, dst);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Writing to a variable does not change its length")
  {
    sb_size res;
    err_t   err;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    // Insert the initial data
    u32 *data = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      data[i] = (u32)randu32 ();
    }
    res = _numstore_fexecute_simple_with_data (db, NULL, data, 0, "insert foo %d %d", 0, ITERS);
    test_assert_int_equal (res, ITERS);
    i_free (mem, data);

    // Overwrite one element at a time
    for (int i = 0; i < ITERS; ++i) {
      u32 val = (u32)randu32 ();
      _numstore_fexecute_simple_with_data (db, NULL, &val, 0, "write foo[%d:%d:1]", i, i + 1);

      // The length should not have changed
      numstore_var_t *var;
      err = _numstore_fexecute_simple_with_var (&var, db, NULL, "get foo");
      test_assert_int_equal (err, SUCCESS);
      test_assert_int_equal (numstore_var_len (var), ITERS);
      numstore_var_free (var);
    }

    // Close database
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Writing a single element leaves all other elements unchanged")
  {
    sb_size res;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    // Insert the initial data - shadow tracks what the variable should contain
    u32 *shadow = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      shadow[i] = (u32)randu32 ();
    }
    res = _numstore_fexecute_simple_with_data (db, NULL, shadow, 0, "insert foo %d %d", 0, ITERS);
    test_assert_int_equal (res, ITERS);

    // Overwrite a random element, then compare everything against the shadow
    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      int idx     = randu32 () % ITERS;
      u32 val     = (u32)randu32 ();
      shadow[idx] = val;

      // Write the element
      _numstore_fexecute_simple_with_data (db, NULL, &val, 0, "write foo[%d:%d:1]", idx, idx + 1);

      // Read everything back and compare
      _numstore_fexecute_simple_with_data (db, NULL, dst, 0, "read foo[:]");
      for (int j = 0; j < ITERS; ++j) {
        test_assert_int_equal (dst[j], shadow[j]);
      }
    }

    // Cleanup
    i_free (mem, shadow);
    i_free (mem, dst);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Rolling back %d random single element writes leaves the data stable", ITERS)
  {
    sb_size res;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    // Insert the initial data
    u32 *initial = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      initial[i] = (u32)randu32 ();
    }
    res = _numstore_fexecute_simple_with_data (db, NULL, initial, 0, "insert foo %d %d", 0, ITERS);
    test_assert_int_equal (res, ITERS);

    // Overwrite a random element in a transaction and roll it back, over and over
    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      int            idx = randu32 () % ITERS;
      u32            val = (u32)randu32 ();

      // Write and rollback
      struct ns_txn *tx  = numstore_begin (db);
      test_assert (tx != NULL);

      _numstore_fexecute_simple_with_data (db, tx, &val, 0, "write foo[%d:%d:1]", idx, idx + 1);

      test_assert_int_equal (numstore_rollback (db, tx), 0);

      // Everything should still match the initial data
      _numstore_fexecute_simple_with_data (db, NULL, dst, 0, "read foo[:]");
      for (int j = 0; j < ITERS; ++j) {
        test_assert_int_equal (dst[j], initial[j]);
      }
    }

    // Cleanup
    i_free (mem, initial);
    i_free (mem, dst);
    test_assert_int_equal (numstore_close (db), 0);
  }

  TEST_CASE ("Committed writes persist across %d database reopens", REOPEN_ITERS)
  {
    sb_size res;

    // Clean re open database
    test_assert_int_equal (numstore_cleanup ("test"), 0);
    numstore_t *db = numstore_open ("test");
    test_assert (db != NULL);

    // Create the variable
    res = _numstore_fexecute_simple_with_data (db, NULL, NULL, 0, "create foo u32");
    test_assert_int_equal (res, 0);

    // Insert zeroed data - data tracks what the variable should contain
    u32 *data = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) {
      data[i] = 0;
    }
    res = _numstore_fexecute_simple_with_data (db, NULL, data, 0, "insert foo %d %d", 0, ITERS);
    test_assert_int_equal (res, ITERS);

    u32 *dst = i_malloc (mem, ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < REOPEN_ITERS; ++i) {
      u32 val           = (u32)randu32 ();
      int idx           = randu32 () % ITERS;
      data[idx]         = val;

      // Write a random element in a transaction and commit it
      struct ns_txn *tx = numstore_begin (db);
      test_assert (tx != NULL);

      _numstore_fexecute_simple_with_data (db, tx, &val, 0, "write foo[%d:%d:1]", idx, idx + 1);

      test_assert_int_equal (numstore_commit (db, tx), 0);

      // Close and re open the database
      test_assert_int_equal (numstore_close (db), 0);
      db = numstore_open ("test");
      test_assert (db != NULL);

      // Everything should still match after the reopen
      _numstore_fexecute_simple_with_data (db, NULL, dst, 0, "read foo[:]");
      for (int j = 0; j < ITERS; ++j) {
        test_assert_int_equal (dst[j], data[j]);
      }
    }

    // Cleanup
    i_free (mem, data);
    i_free (mem, dst);
    test_assert_int_equal (numstore_close (db), 0);
  }
}
#endif
