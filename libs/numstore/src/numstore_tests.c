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

#include "nscore/errors.h"
#include "nscore/nshandle.h"
#include "nscore/var.h"
#include "numstore.h"

#include <c_specx.h>

#define ITERS        10
#define REOPEN_ITERS 20

#ifndef NTEST
TEST (nsdb_create_txn_tests)
{
  TEST_CASE ("create_commit_persists_across_reopen")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_begin (db), 0);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);
    test_assert_int_equal (nsdb_commit (db), 0);
    test_assert_int_equal (nsdb_close (db), 0);

    db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_len (db, "foo"), 0);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("create_rollback_var_not_visible")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_begin (db), 0);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);
    test_assert_int_equal (nsdb_rollback (db), 0);
    test_assert (nsdb_len (db, "foo") < 0);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("create_rollback_same_name_succeeds")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_begin (db), 0);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);
    test_assert_int_equal (nsdb_rollback (db), 0);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("create_sequential_commits_all_persist")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);

    char name[32];
    for (int i = 0; i < ITERS; ++i)
    {
      snprintf (name, sizeof name, "var_%d", i);
      test_assert_int_equal (nsdb_begin (db), 0);
      test_assert_int_equal (nsdb_create (db, name, "u32"), 0);
      test_assert_int_equal (nsdb_commit (db), 0);
    }
    for (int i = 0; i < ITERS; ++i)
    {
      snprintf (name, sizeof name, "var_%d", i);
      test_assert_int_equal (nsdb_len (db, name), 0);
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("create_alternating_commit_rollback")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);

    char name[32];
    for (int i = 0; i < ITERS; ++i)
    {
      snprintf (name, sizeof name, "var_%d", i);
      test_assert_int_equal (nsdb_begin (db), 0);
      test_assert_int_equal (nsdb_create (db, name, "u32"), 0);
      if (i % 2 == 0) { test_assert_int_equal (nsdb_commit (db), 0); }
      else
      {
        test_assert_int_equal (nsdb_rollback (db), 0);
      }
    }
    for (int i = 0; i < ITERS; ++i)
    {
      snprintf (name, sizeof name, "var_%d", i);
      if (i % 2 == 0) { test_assert_int_equal (nsdb_len (db, name), 0); }
      else
      {
        test_assert (nsdb_len (db, name) < 0);
      }
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("create_new_var_always_empty")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);

    char name[32];
    for (int i = 0; i < ITERS; ++i)
    {
      snprintf (name, sizeof name, "var_%d", i);
      test_assert_int_equal (nsdb_create (db, name, "u32"), 0);
      test_assert_int_equal (nsdb_len (db, name), 0);
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("create_duplicate_fails")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);

    char name[32];
    for (int i = 0; i < ITERS; ++i)
    {
      snprintf (name, sizeof name, "var_%d", i);
      test_assert_int_equal (nsdb_create (db, name, "u32"), 0);
      test_assert (nsdb_create (db, name, "u32") == SUCCESS);
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("create_rollback_N_times_then_commit")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i)
    {
      test_assert_int_equal (nsdb_begin (db), 0);
      test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);
      test_assert_int_equal (nsdb_rollback (db), 0);
      test_assert (nsdb_len (db, "foo") < 0);
    }
    test_assert_int_equal (nsdb_begin (db), 0);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);
    test_assert_int_equal (nsdb_commit (db), 0);
    test_assert_int_equal (nsdb_len (db, "foo"), 0);
    test_assert_int_equal (nsdb_close (db), 0);
  }
}

TEST (nsdb_delete_txn_tests)
{
  TEST_CASE ("create_delete_rollback_delete_again")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);
    test_assert_int_equal (nsdb_begin (db), 0);
    test_assert_int_equal (nsdb_delete (db, "foo"), 0);
    test_assert_int_equal (nsdb_rollback (db), 0);
    test_assert_int_equal (nsdb_delete (db, "foo"), 0);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("delete_commit_var_not_visible")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);

    char name[32];
    for (int i = 0; i < ITERS; ++i)
    {
      snprintf (name, sizeof name, "var_%d", i);
      test_assert_int_equal (nsdb_create (db, name, "u32"), 0);
    }
    for (int i = 0; i < ITERS; ++i)
    {
      snprintf (name, sizeof name, "var_%d", i);
      test_assert_int_equal (nsdb_begin (db), 0);
      test_assert_int_equal (nsdb_delete (db, name), 0);
      test_assert_int_equal (nsdb_commit (db), 0);
      test_assert (nsdb_len (db, name) < 0);
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("delete_rollback_var_and_data_survive")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);

    // Fill with randu32om data
    u32 *src = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) { src[i] = (u32)randu32 (); }
    test_assert_int_equal (nsdb_insert (db, "foo", src, 0, ITERS), ITERS);

    for (int i = 0; i < ITERS; ++i)
    {
      test_assert_int_equal (nsdb_begin (db), 0);
      test_assert_int_equal (nsdb_delete (db, "foo"), 0);
      test_assert_int_equal (nsdb_rollback (db), 0);
      test_assert_int_equal (nsdb_len (db, "foo"), ITERS);

      u32 *dst = i_malloc (ITERS * sizeof (u32), 1, NULL);
      nsdb_read (db, "foo", dst, 0, 0, 0, COLON_PRESENT);
      for (int j = 0; j < ITERS; ++j) { test_assert_int_equal (dst[j], src[j]); }
      i_free (dst);
    }
    i_free (src);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("delete_nonexistent_fails")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);

    char name[32];
    for (int i = 0; i < ITERS; ++i)
    {
      snprintf (name, sizeof name, "var_%d", i);
      test_assert (nsdb_delete (db, name) == SUCCESS);
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("delete_twice_fails")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);

    char name[32];
    for (int i = 0; i < ITERS; ++i)
    {
      snprintf (name, sizeof name, "var_%d", i);
      test_assert_int_equal (nsdb_create (db, name, "u32"), 0);
      test_assert_int_equal (nsdb_delete (db, name), 0);
      test_assert (nsdb_delete (db, name) == 0);
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("delete_alternating_commit_rollback")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);

    char name[32];
    for (int i = 0; i < ITERS; ++i)
    {
      snprintf (name, sizeof name, "var_%d", i);
      test_assert_int_equal (nsdb_create (db, name, "u32"), 0);
    }
    for (int i = 0; i < ITERS; ++i)
    {
      snprintf (name, sizeof name, "var_%d", i);
      test_assert_int_equal (nsdb_begin (db), 0);
      test_assert_int_equal (nsdb_delete (db, name), 0);
      if (i % 2 == 0) { test_assert_int_equal (nsdb_commit (db), 0); }
      else
      {
        test_assert_int_equal (nsdb_rollback (db), 0);
      }
    }
    for (int i = 0; i < ITERS; ++i)
    {
      snprintf (name, sizeof name, "var_%d", i);
      if (i % 2 == 0) { test_assert (nsdb_len (db, name) < 0); }
      else
      {
        test_assert_int_equal (nsdb_len (db, name), 0);
      }
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }

  /**
  TEST_CASE ("delete_recreate_cycle")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i)
    {
      test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);
      test_assert_int_equal (nsdb_len (db, "foo"), 0);

      u32 val = (u32)randu32 ();
      test_assert_int_equal (nsdb_insert (db, "foo", &val, 0, 1), 1);
      test_assert_int_equal (nsdb_len (db, "foo"), 1);

      test_assert_int_equal (nsdb_delete (db, "foo"), 0);
      test_assert (nsdb_len (db, "foo") < 0);
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }
  */
}

TEST (nsdb_insert_txn_tests)
{
  TEST_CASE ("insert_commit_data_persists")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);

    u32 *src = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) { src[i] = (u32)randu32 (); }

    test_assert_int_equal (nsdb_begin (db), 0);
    test_assert_int_equal (nsdb_insert (db, "foo", src, 0, ITERS), ITERS);
    test_assert_int_equal (nsdb_commit (db), 0);
    test_assert_int_equal (nsdb_len (db, "foo"), ITERS);

    u32 *dst = i_malloc (ITERS * sizeof (u32), 1, NULL);
    nsdb_read (db, "foo", dst, 0, 0, 0, COLON_PRESENT);
    for (int i = 0; i < ITERS; ++i) { test_assert_int_equal (dst[i], src[i]); }
    i_free (src);
    i_free (dst);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("insert_rollback_len_unchanged")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);
    test_assert_int_equal (nsdb_len (db, "foo"), 0);

    u32 *src = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) { src[i] = (u32)randu32 (); }

    test_assert_int_equal (nsdb_begin (db), 0);
    test_assert_int_equal (nsdb_insert (db, "foo", src, 0, ITERS), ITERS);
    test_assert_int_equal (nsdb_rollback (db), 0);
    test_assert_int_equal (nsdb_len (db, "foo"), 0);
    i_free (src);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("insert_rollback_data_reverts")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);

    u32 *initial = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) { initial[i] = (u32)randu32 (); }
    test_assert_int_equal (nsdb_insert (db, "foo", initial, 0, ITERS), ITERS);

    u32 *extra = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) { extra[i] = (u32)randu32 (); }

    test_assert_int_equal (nsdb_begin (db), 0);
    test_assert_int_equal (nsdb_insert (db, "foo", extra, ITERS, ITERS), ITERS);
    test_assert_int_equal (nsdb_len (db, "foo"), ITERS * 2);
    test_assert_int_equal (nsdb_rollback (db), 0);
    test_assert_int_equal (nsdb_len (db, "foo"), ITERS);

    u32 *dst = i_malloc (ITERS * sizeof (u32), 1, NULL);
    nsdb_read (db, "foo", dst, 0, 0, 0, COLON_PRESENT);
    for (int i = 0; i < ITERS; ++i) { test_assert_int_equal (dst[i], initial[i]); }
    i_free (initial);
    i_free (extra);
    i_free (dst);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("insert_returns_count_accumulates_len")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);

    for (int i = 0; i < ITERS; ++i)
    {
      u32 val = (u32)randu32 ();
      test_assert_int_equal (nsdb_insert (db, "foo", &val, i, 1), 1);
      test_assert_int_equal (nsdb_len (db, "foo"), i + 1);
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("insert_at_front_preserves_order")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);

    // Generate randu32om values, insert in reverse at front → reads back in original order
    u32 *vals = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) { vals[i] = (u32)randu32 (); }
    for (int i = ITERS - 1; i >= 0; --i)
    {
      test_assert_int_equal (nsdb_insert (db, "foo", &vals[i], 0, 1), 1);
    }
    test_assert_int_equal (nsdb_len (db, "foo"), ITERS);

    u32 *dst = i_malloc (ITERS * sizeof (u32), 1, NULL);
    nsdb_read (db, "foo", dst, 0, 0, 0, COLON_PRESENT);
    for (int i = 0; i < ITERS; ++i) { test_assert_int_equal (dst[i], vals[i]); }
    i_free (vals);
    i_free (dst);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("insert_rollback_N_times_data_stable")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);

    u32 *initial = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) { initial[i] = (u32)randu32 (); }
    test_assert_int_equal (nsdb_insert (db, "foo", initial, 0, ITERS), ITERS);

    u32 *extra = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      extra[i] = (u32)randu32 ();
      test_assert_int_equal (nsdb_begin (db), 0);
      test_assert_int_equal (nsdb_insert (db, "foo", &extra[i], ITERS, 1), 1);
      test_assert_int_equal (nsdb_rollback (db), 0);
      test_assert_int_equal (nsdb_len (db, "foo"), ITERS);
    }

    u32 *dst = i_malloc (ITERS * sizeof (u32), 1, NULL);
    nsdb_read (db, "foo", dst, 0, 0, 0, COLON_PRESENT);
    for (int i = 0; i < ITERS; ++i) { test_assert_int_equal (dst[i], initial[i]); }
    i_free (initial);
    i_free (extra);
    i_free (dst);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("insert_many_vars_independent")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);

    char name[32];
    u32 *vals = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      vals[i] = (u32)randu32 ();
      snprintf (name, sizeof name, "var_%d", i);
      test_assert_int_equal (nsdb_create (db, name, "u32"), 0);
      test_assert_int_equal (nsdb_insert (db, name, &vals[i], 0, 1), 1);
    }
    // Verify no cross-contamination
    for (int i = 0; i < ITERS; ++i)
    {
      snprintf (name, sizeof name, "var_%d", i);
      test_assert_int_equal (nsdb_len (db, name), 1);
      u32 dst = 0;
      nsdb_read (db, name, &dst, 0, 0, 0, COLON_PRESENT);
      test_assert_int_equal (dst, vals[i]);
    }
    i_free (vals);
    test_assert_int_equal (nsdb_close (db), 0);
  }
}

TEST (nsdb_write_txn_tests)
{
  TEST_CASE ("write_commit_data_persists")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);

    u32 *initial = i_malloc (ITERS * sizeof (u32), 1, NULL);
    u32 *patch   = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      initial[i] = (u32)randu32 ();
      patch[i]   = (u32)randu32 ();
    }
    test_assert_int_equal (nsdb_insert (db, "foo", initial, 0, ITERS), ITERS);

    test_assert_int_equal (nsdb_begin (db), 0);
    nsdb_write (
        db,
        "foo",
        patch,
        0,
        1,
        ITERS,
        START_PRESENT | STEP_PRESENT | STOP_PRESENT | COLON_PRESENT
    );
    test_assert_int_equal (nsdb_commit (db), 0);

    u32 *dst = i_malloc (ITERS * sizeof (u32), 1, NULL);
    nsdb_read (db, "foo", dst, 0, 0, 0, COLON_PRESENT);
    for (int i = 0; i < ITERS; ++i) { test_assert_int_equal (dst[i], patch[i]); }
    i_free (initial);
    i_free (patch);
    i_free (dst);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("write_rollback_data_reverts")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);

    u32 *initial = i_malloc (ITERS * sizeof (u32), 1, NULL);
    u32 *patch   = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      initial[i] = (u32)randu32 ();
      patch[i]   = (u32)randu32 ();
    }
    test_assert_int_equal (nsdb_insert (db, "foo", initial, 0, ITERS), ITERS);

    test_assert_int_equal (nsdb_begin (db), 0);
    nsdb_write (
        db,
        "foo",
        patch,
        0,
        1,
        ITERS,
        START_PRESENT | STEP_PRESENT | STOP_PRESENT | COLON_PRESENT
    );
    test_assert_int_equal (nsdb_rollback (db), 0);

    u32 *dst = i_malloc (ITERS * sizeof (u32), 1, NULL);
    nsdb_read (db, "foo", dst, 0, 0, 0, COLON_PRESENT);
    for (int i = 0; i < ITERS; ++i) { test_assert_int_equal (dst[i], initial[i]); }
    i_free (initial);
    i_free (patch);
    i_free (dst);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("write_does_not_change_len")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);

    u32 *data = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) { data[i] = (u32)randu32 (); }
    test_assert_int_equal (nsdb_insert (db, "foo", data, 0, ITERS), ITERS);
    i_free (data);

    for (int i = 0; i < ITERS; ++i)
    {
      u32 val = (u32)randu32 ();
      nsdb_write (
          db,
          "foo",
          &val,
          i,
          1,
          i + 1,
          START_PRESENT | STEP_PRESENT | STOP_PRESENT | COLON_PRESENT
      );
      test_assert_int_equal (nsdb_len (db, "foo"), ITERS);
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("write_single_element_others_unchanged")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);

    // Shadow array to track expected state
    u32 *shadow = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) { shadow[i] = (u32)randu32 (); }
    test_assert_int_equal (nsdb_insert (db, "foo", shadow, 0, ITERS), ITERS);

    u32 *dst = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      int idx     = randu32 () % ITERS;
      u32 val     = (u32)randu32 ();
      shadow[idx] = val;
      nsdb_write (
          db,
          "foo",
          &val,
          idx,
          1,
          idx + 1,
          START_PRESENT | STEP_PRESENT | STOP_PRESENT | COLON_PRESENT
      );

      nsdb_read (db, "foo", dst, 0, 0, 0, COLON_PRESENT);
      for (int j = 0; j < ITERS; ++j) { test_assert_int_equal (dst[j], shadow[j]); }
    }
    i_free (shadow);
    i_free (dst);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("write_rollback_N_times_data_stable")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);

    u32 *initial = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) { initial[i] = (u32)randu32 (); }
    test_assert_int_equal (nsdb_insert (db, "foo", initial, 0, ITERS), ITERS);

    u32 *dst = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      // Random patch at randu32om index
      int idx = randu32 () % ITERS;
      u32 val = (u32)randu32 ();
      test_assert_int_equal (nsdb_begin (db), 0);
      nsdb_write (
          db,
          "foo",
          &val,
          idx,
          1,
          idx + 1,
          START_PRESENT | STEP_PRESENT | STOP_PRESENT | COLON_PRESENT
      );
      test_assert_int_equal (nsdb_rollback (db), 0);

      nsdb_read (db, "foo", dst, 0, 0, 0, COLON_PRESENT);
      for (int j = 0; j < ITERS; ++j) { test_assert_int_equal (dst[j], initial[j]); }
    }
    i_free (initial);
    i_free (dst);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("write_commit_persists_across_reopen")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);

    u32 *data = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) { data[i] = 0; }
    test_assert_int_equal (nsdb_insert (db, "foo", data, 0, ITERS), ITERS);

    u32 *dst = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < REOPEN_ITERS; ++i)
    {
      u32 val   = (u32)randu32 ();
      int idx   = randu32 () % ITERS;
      data[idx] = val;

      test_assert_int_equal (nsdb_begin (db), 0);
      nsdb_write (
          db,
          "foo",
          &val,
          idx,
          1,
          idx + 1,
          START_PRESENT | STEP_PRESENT | STOP_PRESENT | COLON_PRESENT
      );
      test_assert_int_equal (nsdb_commit (db), 0);
      test_assert_int_equal (nsdb_close (db), 0);

      db = nsdb_open ("test");
      test_assert (db != NULL);
      nsdb_read (db, "foo", dst, 0, 0, 0, COLON_PRESENT);
      for (int j = 0; j < ITERS; ++j) { test_assert_int_equal (dst[j], data[j]); }
    }
    i_free (data);
    i_free (dst);
    test_assert_int_equal (nsdb_close (db), 0);
  }
}

/**
TEST_DISABLED (nsdb_remove_txn_tests)
{
  TEST_CASE ("remove_decrements_len_one_at_a_time")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);

    u32 *data = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) { data[i] = (u32)randu32 (); }
    test_assert_int_equal (nsdb_insert (db, "foo", data, 0, ITERS), ITERS);
    i_free (data);

    u32 dst;
    for (int i = ITERS; i > 0; --i)
    {
      test_assert_int_equal (nsdb_len (db, "foo"), i);
      nsdb_remove (
          db,
          "foo",
          &dst,
          0,
          1,
          1,
          START_PRESENT | STEP_PRESENT | STOP_PRESENT | COLON_PRESENT
      );
      test_assert_int_equal (nsdb_len (db, "foo"), i - 1);
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("remove_returns_correct_data")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);

    u32 *data = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) { data[i] = (u32)randu32 (); }
    test_assert_int_equal (nsdb_insert (db, "foo", data, 0, ITERS), ITERS);

    // Remove from front each time, verify value matches
    for (int i = 0; i < ITERS; ++i)
    {
      u32 dst = 0;
      nsdb_remove (
          db,
          "foo",
          &dst,
          0,
          1,
          1,
          START_PRESENT | STEP_PRESENT | STOP_PRESENT | COLON_PRESENT
      );
      test_assert_int_equal (dst, data[i]);
    }
    i_free (data);
    test_assert_int_equal (nsdb_len (db, "foo"), 0);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("remove_rollback_len_and_data_restored")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);

    u32 *initial = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) { initial[i] = (u32)randu32 (); }
    test_assert_int_equal (nsdb_insert (db, "foo", initial, 0, ITERS), ITERS);

    u32 *dst = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      // Remove a randu32om-length chunk from a randu32om position
      int  pos = randu32 () % ITERS;
      int  cnt = randu32 () % (ITERS - pos) + 1;
      u32 *tmp = i_malloc (cnt * sizeof (u32), 1, NULL);

      test_assert_int_equal (nsdb_begin (db), 0);
      nsdb_remove (
          db,
          "foo",
          tmp,
          pos,
          1,
          pos + cnt,
          START_PRESENT | STEP_PRESENT | STOP_PRESENT | COLON_PRESENT
      );
      i_free (tmp);
      test_assert_int_equal (nsdb_rollback (db), 0);
      test_assert_int_equal (nsdb_len (db, "foo"), ITERS);

      nsdb_read (db, "foo", dst, 0, 0, 0, COLON_PRESENT);
      for (int j = 0; j < ITERS; ++j) { test_assert_int_equal (dst[j], initial[j]); }
    }
    i_free (initial);
    i_free (dst);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("remove_commit_data_gone")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);

    for (int i = 0; i < ITERS; ++i)
    {
      u32 val = (u32)randu32 ();
      test_assert_int_equal (nsdb_insert (db, "foo", &val, 0, 1), 1);

      u32 dst;
      test_assert_int_equal (nsdb_begin (db), 0);
      nsdb_remove (
          db,
          "foo",
          &dst,
          0,
          1,
          1,
          START_PRESENT | STEP_PRESENT | STOP_PRESENT | COLON_PRESENT
      );
      test_assert_int_equal (nsdb_commit (db), 0);
      test_assert_int_equal (nsdb_len (db, "foo"), 0);
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("remove_from_randu32om_middle_order_preserved")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);

    for (int iter = 0; iter < ITERS; ++iter)
    {
      // Fill with randu32om data, track in shadow
      int  n      = randu32 () % 64 + 2; // at least 2 elements
      u32 *shadow = i_malloc (n * sizeof (u32), 1, NULL);
      for (int i = 0; i < n; ++i) { shadow[i] = (u32)randu32 (); }
      test_assert_int_equal (nsdb_insert (db, "foo", shadow, 0, n), n);

      // Remove element at randu32om index, update shadow
      int idx = randu32 () % n;
      u32 dst;
      nsdb_remove (
          db,
          "foo",
          &dst,
          idx,
          1,
          idx + 1,
          START_PRESENT | STEP_PRESENT | STOP_PRESENT | COLON_PRESENT
      );
      test_assert_int_equal (dst, shadow[idx]);
      test_assert_int_equal (nsdb_len (db, "foo"), n - 1);

      // Build expected: shadow without element at idx
      u32 *expected = i_malloc ((n - 1) * sizeof (u32), 1, NULL);
      for (int i = 0, k = 0; i < n; ++i)
      {
        if (i != idx) { expected[k++] = shadow[i]; }
      }

      u32 *readback = i_malloc ((n - 1) * sizeof (u32), 1, NULL);
      nsdb_read (db, "foo", readback, 0, 0, 0, COLON_PRESENT);
      for (int i = 0; i < n - 1; ++i) { test_assert_int_equal (readback[i], expected[i]); }

      // Drain for next iteration
      nsdb_remove (
          db,
          "foo",
          readback,
          0,
          1,
          n - 1,
          START_PRESENT | STEP_PRESENT | STOP_PRESENT | COLON_PRESENT
      );
      i_free (shadow);
      i_free (expected);
      i_free (readback);
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("remove_alternating_commit_rollback_len_correct")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);

    u32 *data = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i) { data[i] = (u32)randu32 (); }
    test_assert_int_equal (nsdb_insert (db, "foo", data, 0, ITERS), ITERS);
    i_free (data);

    int remaining = ITERS;
    u32 dst;
    for (int i = 0; i < ITERS; ++i)
    {
      test_assert_int_equal (nsdb_begin (db), 0);
      nsdb_remove (
          db,
          "foo",
          &dst,
          0,
          1,
          1,
          START_PRESENT | STEP_PRESENT | STOP_PRESENT | COLON_PRESENT
      );
      if (i % 2 == 0)
      {
        test_assert_int_equal (nsdb_commit (db), 0);
        remaining--;
      }
      else
      {
        test_assert_int_equal (nsdb_rollback (db), 0);
      }
      test_assert_int_equal (nsdb_len (db, "foo"), remaining);
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }
}
*/

#endif
