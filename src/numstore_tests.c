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

#include <c_specx.h>

#include "nscore/errors.h"
#include "nscore/nshandle.h"
#include "nscore/var.h"
#include "numstore.h"

#define ITERS        10
#define REOPEN_ITERS 20

#ifndef NTEST
TEST (nsdb_create_txn)
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
    nsdb_var_t *foo = nsdb_get (db, "foo");
    test_assert_int_equal (nsdb_len (db, "foo"), 0);
    nsdb_free (foo);
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
      test_assert_int_equal (nsdb_len (db, "foo"), ERR_VARIABLE_NE);
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
      if (i % 2 == 0)
      {
        test_assert_int_equal (nsdb_commit (db), 0);
      }
      else
      {
        test_assert_int_equal (nsdb_rollback (db), 0);
      }
    }
    for (int i = 0; i < ITERS; ++i)
    {
      snprintf (name, sizeof name, "var_%d", i);
      if (i % 2 == 0)
      {
        test_assert_int_equal (nsdb_len (db, "foo"), ERR_VARIABLE_NE);
      }
      else
      {
        test_assert (nsdb_len (db, "foo") < 0);
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
      test_assert_int_equal (nsdb_len (db, "foo"), ERR_VARIABLE_NE);
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

TEST (nsdb_delete_txn)
{
  TEST_CASE ("create_delete_rollback_delete_again")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);
    nsdb_var_t *foo = nsdb_get (db, "foo");
    test_assert_int_equal (nsdb_begin (db), 0);
    test_assert_int_equal (nsdb_delete (db, "foo"), 0);
    test_assert_int_equal (nsdb_rollback (db), 0);
    test_assert_int_equal (nsdb_delete (db, "foo"), 0);
    nsdb_free (foo);
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
      nsdb_var_t *var = nsdb_get (db, name);
      test_assert_int_equal (nsdb_begin (db), 0);
      test_assert_int_equal (nsdb_delete (db, "var"), ERR_VARIABLE_NE);
      test_assert (nsdb_len (db, "var") < 0);
      nsdb_free (var);
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("delete_rollback_var_and_data_survive")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);
    nsdb_var_t *foo = nsdb_get (db, "foo");

    u32 *src = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      src[i] = (u32)randu32 ();
    }
    test_assert_int_equal (nsdb_insert (db, foo, src, 0, ITERS), ITERS);

    for (int i = 0; i < ITERS; ++i)
    {
      test_assert_int_equal (nsdb_begin (db), 0);
      test_assert_int_equal (nsdb_delete (db, "foo"), 0);
      test_assert_int_equal (nsdb_rollback (db), 0);
      test_assert_int_equal (nsdb_len (db, "foo"), ITERS);

      u32 *dst = i_malloc (ITERS * sizeof (u32), 1, NULL);
      nsdb_read (db, foo, dst, 0, 0, 0, COLON_PRESENT);
      for (int j = 0; j < ITERS; ++j)
      {
        test_assert_int_equal (dst[j], src[j]);
      }
      i_free (dst);
    }
    nsdb_free (foo);
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
      test_assert_int_equal (nsdb_delete (db, name), ERR_VARIABLE_NE);
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
      test_assert (nsdb_delete (db, "var") == ERR_VARIABLE_NE);
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }
}

TEST (nsdb_insert_txn)
{
  TEST_CASE ("insert_commit_data_persists")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);

    u32 *src = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      src[i] = (u32)randu32 ();
    }

    nsdb_var_t *foo = nsdb_get (db, "foo");
    test_assert_int_equal (nsdb_begin (db), 0);
    test_assert_int_equal (nsdb_insert (db, foo, src, 0, ITERS), ITERS);
    test_assert_int_equal (nsdb_commit (db), 0);
    test_assert_int_equal (nsdb_len (db, "foo"), ITERS);

    u32 *dst = i_malloc (ITERS * sizeof (u32), 1, NULL);
    nsdb_read (db, foo, dst, 0, 0, 0, COLON_PRESENT);
    for (int i = 0; i < ITERS; ++i)
    {
      test_assert_int_equal (dst[i], src[i]);
    }
    nsdb_free (foo);
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
    nsdb_var_t *foo = nsdb_get (db, "foo");
    test_assert_int_equal (nsdb_len (db, "foo"), 0);

    u32 *src = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      src[i] = (u32)randu32 ();
    }

    test_assert_int_equal (nsdb_begin (db), 0);
    test_assert_int_equal (nsdb_insert (db, foo, src, 0, ITERS), ITERS);
    test_assert_int_equal (nsdb_rollback (db), 0);
    test_assert_int_equal (nsdb_len (db, "foo"), 0);
    nsdb_free (foo);
    i_free (src);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("insert_rollback_data_reverts")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);
    nsdb_var_t *foo = nsdb_get (db, "foo");

    u32 *initial = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      initial[i] = (u32)randu32 ();
    }
    test_assert_int_equal (nsdb_insert (db, foo, initial, 0, ITERS), ITERS);

    u32 *extra = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      extra[i] = (u32)randu32 ();
    }

    test_assert_int_equal (nsdb_begin (db), 0);
    test_assert_int_equal (nsdb_insert (db, foo, extra, ITERS, ITERS), ITERS);
    test_assert_int_equal (nsdb_len (db, "foo"), ITERS * 2);
    test_assert_int_equal (nsdb_rollback (db), 0);
    test_assert_int_equal (nsdb_len (db, "foo"), ITERS);

    u32 *dst = i_malloc (ITERS * sizeof (u32), 1, NULL);
    nsdb_read (db, foo, dst, 0, 0, 0, COLON_PRESENT);
    for (int i = 0; i < ITERS; ++i)
    {
      test_assert_int_equal (dst[i], initial[i]);
    }
    nsdb_free (foo);
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
    nsdb_var_t *foo = nsdb_get (db, "foo");

    for (int i = 0; i < ITERS; ++i)
    {
      u32 val = (u32)randu32 ();
      test_assert_int_equal (nsdb_insert (db, foo, &val, i, 1), 1);
      test_assert_int_equal (nsdb_len (db, "foo"), i + 1);
    }
    nsdb_free (foo);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("insert_at_front_preserves_order")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);
    nsdb_var_t *foo = nsdb_get (db, "foo");

    u32 *vals = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      vals[i] = (u32)randu32 ();
    }
    for (int i = ITERS - 1; i >= 0; --i)
    {
      test_assert_int_equal (nsdb_insert (db, foo, &vals[i], 0, 1), 1);
    }
    test_assert_int_equal (nsdb_len (db, "foo"), ITERS);

    u32 *dst = i_malloc (ITERS * sizeof (u32), 1, NULL);
    nsdb_read (db, foo, dst, 0, 0, 0, COLON_PRESENT);
    for (int i = 0; i < ITERS; ++i)
    {
      test_assert_int_equal (dst[i], vals[i]);
    }
    nsdb_free (foo);
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
    nsdb_var_t *foo = nsdb_get (db, "foo");

    u32 *initial = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      initial[i] = (u32)randu32 ();
    }
    test_assert_int_equal (nsdb_insert (db, foo, initial, 0, ITERS), ITERS);

    u32 *extra = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      extra[i] = (u32)randu32 ();
      test_assert_int_equal (nsdb_begin (db), 0);
      test_assert_int_equal (nsdb_insert (db, foo, &extra[i], ITERS, 1), 1);
      test_assert_int_equal (nsdb_rollback (db), 0);
      test_assert_int_equal (nsdb_len (db, "foo"), ITERS);
    }

    u32 *dst = i_malloc (ITERS * sizeof (u32), 1, NULL);
    nsdb_read (db, foo, dst, 0, 0, 0, COLON_PRESENT);
    for (int i = 0; i < ITERS; ++i)
    {
      test_assert_int_equal (dst[i], initial[i]);
    }
    nsdb_free (foo);
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
      nsdb_var_t *var = nsdb_get (db, name);
      test_assert_int_equal (nsdb_insert (db, var, &vals[i], 0, 1), 1);
      nsdb_free (var);
    }
    for (int i = 0; i < ITERS; ++i)
    {
      snprintf (name, sizeof name, "var_%d", i);
      nsdb_var_t *var = nsdb_get (db, name);
      test_assert_int_equal (nsdb_len (db, name), 1);
      u32 dst = 0;
      nsdb_read (db, var, &dst, 0, 0, 0, COLON_PRESENT);
      test_assert_int_equal (dst, vals[i]);
      nsdb_free (var);
    }
    i_free (vals);
    test_assert_int_equal (nsdb_close (db), 0);
  }
}

TEST (nsdb_write_txn)
{
  TEST_CASE ("write_commit_data_persists")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);
    nsdb_var_t *foo = nsdb_get (db, "foo");

    u32 *initial = i_malloc (ITERS * sizeof (u32), 1, NULL);
    u32 *patch   = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      initial[i] = (u32)randu32 ();
      patch[i]   = (u32)randu32 ();
    }
    test_assert_int_equal (nsdb_insert (db, foo, initial, 0, ITERS), ITERS);

    test_assert_int_equal (nsdb_begin (db), 0);
    nsdb_write (
        db,
        foo,
        patch,
        0,
        1,
        ITERS,
        START_PRESENT | STEP_PRESENT | STOP_PRESENT | COLON_PRESENT
    );
    test_assert_int_equal (nsdb_commit (db), 0);

    u32 *dst = i_malloc (ITERS * sizeof (u32), 1, NULL);
    nsdb_read (db, foo, dst, 0, 0, 0, COLON_PRESENT);
    for (int i = 0; i < ITERS; ++i)
    {
      test_assert_int_equal (dst[i], patch[i]);
    }
    nsdb_free (foo);
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
    nsdb_var_t *foo = nsdb_get (db, "foo");

    u32 *initial = i_malloc (ITERS * sizeof (u32), 1, NULL);
    u32 *patch   = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      initial[i] = (u32)randu32 ();
      patch[i]   = (u32)randu32 ();
    }
    test_assert_int_equal (nsdb_insert (db, foo, initial, 0, ITERS), ITERS);

    test_assert_int_equal (nsdb_begin (db), 0);
    nsdb_write (
        db,
        foo,
        patch,
        0,
        1,
        ITERS,
        START_PRESENT | STEP_PRESENT | STOP_PRESENT | COLON_PRESENT
    );
    test_assert_int_equal (nsdb_rollback (db), 0);

    u32 *dst = i_malloc (ITERS * sizeof (u32), 1, NULL);
    nsdb_read (db, foo, dst, 0, 0, 0, COLON_PRESENT);
    for (int i = 0; i < ITERS; ++i)
    {
      test_assert_int_equal (dst[i], initial[i]);
    }
    nsdb_free (foo);
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
    nsdb_var_t *foo = nsdb_get (db, "foo");

    u32 *data = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      data[i] = (u32)randu32 ();
    }
    test_assert_int_equal (nsdb_insert (db, foo, data, 0, ITERS), ITERS);
    i_free (data);

    for (int i = 0; i < ITERS; ++i)
    {
      u32 val = (u32)randu32 ();
      nsdb_write (
          db,
          foo,
          &val,
          i,
          1,
          i + 1,
          START_PRESENT | STEP_PRESENT | STOP_PRESENT | COLON_PRESENT
      );
      test_assert_int_equal (nsdb_len (db, "foo"), ITERS);
    }
    nsdb_free (foo);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("write_single_element_others_unchanged")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);
    nsdb_var_t *foo = nsdb_get (db, "foo");

    u32 *shadow = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      shadow[i] = (u32)randu32 ();
    }
    test_assert_int_equal (nsdb_insert (db, foo, shadow, 0, ITERS), ITERS);

    u32 *dst = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      int idx     = randu32 () % ITERS;
      u32 val     = (u32)randu32 ();
      shadow[idx] = val;
      nsdb_write (
          db,
          foo,
          &val,
          idx,
          1,
          idx + 1,
          START_PRESENT | STEP_PRESENT | STOP_PRESENT | COLON_PRESENT
      );
      nsdb_read (db, foo, dst, 0, 0, 0, COLON_PRESENT);
      for (int j = 0; j < ITERS; ++j)
      {
        test_assert_int_equal (dst[j], shadow[j]);
      }
    }
    nsdb_free (foo);
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
    nsdb_var_t *foo = nsdb_get (db, "foo");

    u32 *initial = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      initial[i] = (u32)randu32 ();
    }
    test_assert_int_equal (nsdb_insert (db, foo, initial, 0, ITERS), ITERS);

    u32 *dst = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      int idx = randu32 () % ITERS;
      u32 val = (u32)randu32 ();
      test_assert_int_equal (nsdb_begin (db), 0);
      nsdb_write (
          db,
          foo,
          &val,
          idx,
          1,
          idx + 1,
          START_PRESENT | STEP_PRESENT | STOP_PRESENT | COLON_PRESENT
      );
      test_assert_int_equal (nsdb_rollback (db), 0);
      nsdb_read (db, foo, dst, 0, 0, 0, COLON_PRESENT);
      for (int j = 0; j < ITERS; ++j)
      {
        test_assert_int_equal (dst[j], initial[j]);
      }
    }
    nsdb_free (foo);
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
    nsdb_var_t *foo = nsdb_get (db, "foo");

    u32 *data = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      data[i] = 0;
    }
    test_assert_int_equal (nsdb_insert (db, foo, data, 0, ITERS), ITERS);
    nsdb_free (foo);

    u32 *dst = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < REOPEN_ITERS; ++i)
    {
      u32 val   = (u32)randu32 ();
      int idx   = randu32 () % ITERS;
      data[idx] = val;

      foo = nsdb_get (db, "foo");
      test_assert_int_equal (nsdb_begin (db), 0);
      nsdb_write (
          db,
          foo,
          &val,
          idx,
          1,
          idx + 1,
          START_PRESENT | STEP_PRESENT | STOP_PRESENT | COLON_PRESENT
      );
      test_assert_int_equal (nsdb_commit (db), 0);
      nsdb_free (foo);
      test_assert_int_equal (nsdb_close (db), 0);

      db = nsdb_open ("test");
      test_assert (db != NULL);
      foo = nsdb_get (db, "foo");
      nsdb_read (db, foo, dst, 0, 0, 0, COLON_PRESENT);
      for (int j = 0; j < ITERS; ++j)
      {
        test_assert_int_equal (dst[j], data[j]);
      }
      nsdb_free (foo);
    }
    i_free (data);
    i_free (dst);
    test_assert_int_equal (nsdb_close (db), 0);
  }
}

TEST (nsdb_get_if_exists)
{
  TEST_CASE ("smoke test green path")
  {
    // Clean up the db
    test_assert_int_equal (nsh_cleanup ("test"), 0);

    // Open it
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);

    nsdb_var_t *var;

    // Get if exists (doesn't) - ok
    test_assert (nsdb_get_if_exists (db, &var, "foo") == SUCCESS);
    test_assert (var == NULL);

    // Create
    test_assert_int_equal (nsdb_create (db, "foo", "u32"), 0);

    // Get if exists (does) - ok
    test_assert (nsdb_get_if_exists (db, &var, "foo") == SUCCESS);
    test_assert (var != NULL);

    nsdb_free (var);

    nsdb_close (db);
  }
}
#endif
