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

#include "nshandle.h"        // nshandle
#include "numstore.h"        // nsdb
#include "testing/testing.h" // TEST

#ifdef TESTING
TEST (regression_cgd_test_create_delete_rollback_delete)
{
  test_assert_int_equal (nsh_cleanup ("test"), 0);
  nsdb_t *db = nsdb_open ("test");
  test_assert (db != NULL);

  // Create the variable
  test_assert_int_equal (
      nsdb_execute (
          db,
          "create n8Si3C union { tok6UW u32, YGhr cf128, LDzpWVm f16 }",
          NULL
      ),
      0
  );

  // The culprit txn
  test_assert_int_equal (nsdb_begin (db), 0);
  test_assert_int_equal (nsdb_execute (db, "delete n8Si3C", NULL), 0);
  test_assert_int_equal (nsdb_rollback (db), 0);

  // Do something (seemingly unrelated)
  test_assert_int_equal (nsdb_begin (db), 0);
  test_assert_int_equal (
      nsdb_execute (
          db,
          "create yJIF "
          "struct { sQf8W7t6 struct { ukc7C4 cf256, CHbmDuiD6 union { aVmHRo "
          "cf64, FeVvpnN u64 } } }",
          NULL
      ),
      0
  );
  test_assert_int_equal (nsdb_commit (db), 0);

  // This failed - it shouldn't because we roll'ed back our previous delete
  //      CAUSE:
  //          pgr_delete_and_release was setting the page in the fsm log
  //          to the page being released, not the fsm - this came from a
  //          refactor - I used to do that
  //          also it never included the bit in the log
  test_assert (nsdb_execute (db, "delete n8Si3C", NULL) == 0);

  test_assert_int_equal (nsdb_close (db), 0);
}

TEST (regression_cgd_test_create_crash_close_delete)
{
  test_assert_int_equal (nsh_cleanup ("test"), 0);
  nsdb_t *db = nsdb_open ("test");
  test_assert (db != NULL);

  // Create
  test_assert_int_equal (
      nsdb_execute (db, "create MkWMJ9a [8][9][3][3] i16", NULL),
      0
  );

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
  test_assert (nsdb_execute (db, "delete MkWMJ9a", NULL) == 0);

  test_assert_int_equal (nsdb_close (db), 0);
}

TEST (regression_irwr_rollback_invalid_wal_header)
{
  test_assert_int_equal (nsh_cleanup ("test"), 0);
  nsdb_t *db = nsdb_open ("test");
  test_assert (db != NULL);

  // TXN 1 (auto)
  test_assert_int_equal (nsdb_execute (db, "create testvar u32", NULL), 0);

  // TXN 2
  test_assert_int_equal (nsdb_begin (db), 0);
  test_assert_int_equal (nsdb_rollback (db), 0);

  // TXN 3
  test_assert_int_equal (nsdb_begin (db), 0);
  test_assert_int_equal (nsdb_commit (db), 0);

  // TXN 4 (auto): INSERT ofst=0 nelem=53797
  {
    u32 *data = i_malloc (53797 * sizeof (u32), 1, NULL);
    test_assert (data != NULL);
    for (int i = 0; i < 53797; ++i)
    {
      data[i] = (u32)randu32 ();
    }
    test_assert_int_equal (
        nsdb_execute (db, "insert testvar %d %d", data, 0, 53797),
        53797
    );
    i_free (data);
  }

  // TXN 5 (auto): WRITE start=23070 stride=7888 stop=54622 nelems=4
  {
    u32 data[4];
    for (int i = 0; i < 4; ++i)
    {
      data[i] = (u32)randu32 ();
    }
    test_assert_int_equal (
        nsdb_execute (db, "write testvar[23070:54622:7888]", data),
        4
    );
  }

  // TXN 6: REMOVE start=5512 stride=13648 stop=32808 nelems=2 → COMMIT
  test_assert_int_equal (nsdb_begin (db), 0);
  {
    u32 removed[2];
    test_assert_int_equal (
        nsdb_execute (db, "remove testvar[5512:32808:13648]", removed),
        2
    );
  }
  test_assert_int_equal (nsdb_commit (db), 0);

  // TXN 7 (auto): WRITE start=50236 stride=283 stop=51085 nelems=3
  {
    u32 data[3];
    for (int i = 0; i < 3; ++i)
    {
      data[i] = (u32)randu32 ();
    }
    test_assert_int_equal (
        nsdb_execute (db, "write testvar[50236:51085:283]", data),
        3
    );
  }

  // TXN 8
  test_assert_int_equal (nsdb_begin (db), 0);
  test_assert_int_equal (nsdb_rollback (db), 0);

  // TXN 9 (auto): REMOVE start=51429 stride=1931 stop=55291 nelems=2
  {
    u32 removed[2];
    test_assert_int_equal (
        nsdb_execute (db, "remove testvar[51429:55291:1931]", removed),
        2
    );
  }

  // TXN 10 (auto): READ start=1632 stride=9623 stop=20878 nelems=2
  {
    u32 buf[2];
    test_assert_int_equal (
        nsdb_execute (db, "read testvar[1632:20878:9623]", buf),
        2
    );
  }

  // TXN 11 (auto): READ start=48723 stride=4036 stop=56795 nelems=2
  {
    u32 buf[2];
    test_assert_int_equal (
        nsdb_execute (db, "read testvar[48723:56795:4036]", buf),
        2
    );
  }

  // TXN 12
  test_assert_int_equal (nsdb_begin (db), 0);
  test_assert_int_equal (nsdb_commit (db), 0);

  // TXN 13 → ROLLBACK triggers invalid wal header bug
  test_assert_int_equal (nsdb_begin (db), 0);

  // WRITE start=49014 stride=3051 stop=52065 nelems=1
  {
    u32 data[1] = {(u32)randu32 ()};
    test_assert_int_equal (
        nsdb_execute (db, "write testvar[49014:52065:3051]", data),
        1
    );
  }

  // INSERT ofst=22727 nelem=73857
  {
    u32 *data = i_malloc (73857 * sizeof (u32), 1, NULL);
    test_assert (data != NULL);
    for (int i = 0; i < 73857; ++i)
    {
      data[i] = (u32)randu32 ();
    }
    test_assert_int_equal (
        nsdb_execute (db, "insert testvar %d %d", data, 22727, 73857),
        73857
    );
    i_free (data);
  }

  // REMOVE start=5509 stride=92363 stop=190235 nelems=2
  {
    u32 removed[2];
    test_assert_int_equal (
        nsdb_execute (db, "remove testvar[5509:190235:92363]", removed),
        2
    );
  }

  // INSERT ofst=8986 nelem=15959
  {
    u32 *data = i_malloc (15959 * sizeof (u32), 1, NULL);
    test_assert (data != NULL);
    for (int i = 0; i < 15959; ++i)
    {
      data[i] = (u32)randu32 ();
    }
    test_assert_int_equal (
        nsdb_execute (db, "insert testvar %d %d", data, 8986, 15959),
        15959
    );
    i_free (data);
  }

  // READ start=118059 stride=13676 stop=145411 nelems=2
  {
    u32 buf[2];
    test_assert_int_equal (
        nsdb_execute (db, "read testvar[118059:145411:13676]", buf),
        2
    );
  }

  // WRITE start=58530 stride=22447 stop=103424 nelems=2
  {
    u32 data[2];
    for (int i = 0; i < 2; ++i)
    {
      data[i] = (u32)randu32 ();
    }
    test_assert_int_equal (
        nsdb_execute (db, "write testvar[58530:103424:22447]", data),
        2
    );
  }

  // INSERT ofst=29193 nelem=27045
  {
    u32 *data = i_malloc (27045 * sizeof (u32), 1, NULL);
    test_assert (data != NULL);
    for (int i = 0; i < 27045; ++i)
    {
      data[i] = (u32)randu32 ();
    }
    test_assert_int_equal (
        nsdb_execute (db, "insert testvar %d %d", data, 29193, 27045),
        27045
    );
    i_free (data);
  }

  // READ start=39413 stride=49536 stop=88949 nelems=1
  {
    u32 buf[1];
    test_assert_int_equal (
        nsdb_execute (db, "read testvar[39413:88949:49536]", buf),
        1
    );
  }

  // This failed -
  //      CAUSE:
  //          The threading logic was wrong - I just made the WAL single
  //          threaded instead
  test_assert_int_equal (nsdb_rollback (db), 0);

  test_assert_int_equal (nsdb_close (db), 0);
}
#endif
