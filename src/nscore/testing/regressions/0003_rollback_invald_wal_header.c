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
#include "numstore/ns_numstore_internal.h"
#include "numstore/numstore.h"

#ifdef TESTING
#  include "core/testing/ns_testing.h"
#endif

#ifdef TESTING

TEST (0003_rollback_invalid_wal_header)
{
  sb_size     res;
  struct txn *tx;

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

#endif
