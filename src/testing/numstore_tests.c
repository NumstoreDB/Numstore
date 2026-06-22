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

#include "error.h"
#include "nshandle.h"
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
    test_assert_int_equal (nsdb_execute (db, "create foo u32", NULL), 0);
    test_assert_int_equal (nsdb_commit (db), 0);
    test_assert_int_equal (nsdb_close (db), 0);

    db = nsdb_open ("test");
    test_assert (db != NULL);
    nsdb_var_t *var;
    test_assert_int_equal (nsdb_execute (db, "get foo", &var), 0);
    test_assert_int_equal (nsdb_var_len (var), 0);
    nsdb_var_free (var);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("create_rollback_var_not_visible")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_begin (db), 0);
    test_assert_int_equal (nsdb_execute (db, "create foo u32", NULL), 0);
    test_assert_int_equal (nsdb_rollback (db), 0);
    nsdb_var_t *var;
    test_assert (nsdb_execute (db, "get foo", &var) != 0);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("create_rollback_same_name_succeeds")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_begin (db), 0);
    test_assert_int_equal (nsdb_execute (db, "create foo u32", NULL), 0);
    test_assert_int_equal (nsdb_rollback (db), 0);
    test_assert_int_equal (nsdb_execute (db, "create foo u32", NULL), 0);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("create_sequential_commits_all_persist")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i)
    {
      test_assert_int_equal (nsdb_begin (db), 0);
      test_assert_int_equal (
          nsdb_execute (db, "create var_%d u32", NULL, i),
          0
      );
      test_assert_int_equal (nsdb_commit (db), 0);
    }
    for (int i = 0; i < ITERS; ++i)
    {
      nsdb_var_t *var;
      test_assert_int_equal (
          nsdb_execute (db, "get foo", &var),
          ERR_VARIABLE_NE
      );
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("create_alternating_commit_rollback")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i)
    {
      test_assert_int_equal (nsdb_begin (db), 0);
      test_assert_int_equal (
          nsdb_execute (db, "create var_%d u32", NULL, i),
          0
      );
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
      nsdb_var_t *var;
      if (i % 2 == 0)
      {
        test_assert_int_equal (
            nsdb_execute (db, "get foo", &var),
            ERR_VARIABLE_NE
        );
      }
      else
      {
        test_assert (nsdb_execute (db, "get foo", &var) != 0);
      }
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("create_new_var_always_empty")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i)
    {
      test_assert_int_equal (
          nsdb_execute (db, "create var_%d u32", NULL, i),
          0
      );
      nsdb_var_t *var;
      test_assert_int_equal (
          nsdb_execute (db, "get foo", &var),
          ERR_VARIABLE_NE
      );
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("create_duplicate_fails")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i)
    {
      test_assert_int_equal (
          nsdb_execute (db, "create var_%d u32", NULL, i),
          0
      );
      test_assert (nsdb_execute (db, "create var_%d u32", NULL, i) == SUCCESS);
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
      test_assert_int_equal (nsdb_execute (db, "create foo u32", NULL), 0);
      test_assert_int_equal (nsdb_rollback (db), 0);
      nsdb_var_t *var;
      test_assert (nsdb_execute (db, "get foo", &var) != 0);
    }
    test_assert_int_equal (nsdb_begin (db), 0);
    test_assert_int_equal (nsdb_execute (db, "create foo u32", NULL), 0);
    test_assert_int_equal (nsdb_commit (db), 0);
    nsdb_var_t *var;
    test_assert_int_equal (nsdb_execute (db, "get foo", &var), 0);
    test_assert_int_equal (nsdb_var_len (var), 0);
    nsdb_var_free (var);
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
    test_assert_int_equal (nsdb_execute (db, "create foo u32", NULL), 0);
    test_assert_int_equal (nsdb_begin (db), 0);
    test_assert_int_equal (nsdb_execute (db, "delete foo", NULL), 0);
    test_assert_int_equal (nsdb_rollback (db), 0);
    test_assert_int_equal (nsdb_execute (db, "delete foo", NULL), 0);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("delete_commit_var_not_visible")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i)
    {
      test_assert_int_equal (
          nsdb_execute (db, "create var_%d u32", NULL, i),
          0
      );
    }
    for (int i = 0; i < ITERS; ++i)
    {
      test_assert_int_equal (nsdb_begin (db), 0);
      test_assert_int_equal (
          nsdb_execute (db, "delete var", NULL),
          ERR_VARIABLE_NE
      );
      nsdb_var_t *var;
      test_assert (nsdb_execute (db, "get var", &var) != 0);
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("delete_rollback_var_and_data_survive")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_execute (db, "create foo u32", NULL), 0);

    u32 *src = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      src[i] = (u32)randu32 ();
    }
    test_assert_int_equal (
        nsdb_execute (db, "insert foo %d %d", src, 0, ITERS),
        ITERS
    );

    for (int i = 0; i < ITERS; ++i)
    {
      test_assert_int_equal (nsdb_begin (db), 0);
      test_assert_int_equal (nsdb_execute (db, "delete foo", NULL), 0);
      test_assert_int_equal (nsdb_rollback (db), 0);

      nsdb_var_t *var;
      test_assert_int_equal (nsdb_execute (db, "get foo", &var), 0);
      test_assert_int_equal (nsdb_var_len (var), ITERS);
      nsdb_var_free (var);

      u32 *dst = i_malloc (ITERS * sizeof (u32), 1, NULL);
      nsdb_execute (db, "read foo[:]", dst);
      for (int j = 0; j < ITERS; ++j)
      {
        test_assert_int_equal (dst[j], src[j]);
      }
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

    for (int i = 0; i < ITERS; ++i)
    {
      test_assert_int_equal (
          nsdb_execute (db, "delete var_%d", NULL, i),
          ERR_VARIABLE_NE
      );
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("delete_twice_fails")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);

    for (int i = 0; i < ITERS; ++i)
    {
      test_assert_int_equal (
          nsdb_execute (db, "create var_%d u32", NULL, i),
          0
      );
      test_assert (nsdb_execute (db, "delete var", NULL) == ERR_VARIABLE_NE);
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
    test_assert_int_equal (nsdb_execute (db, "create foo u32", NULL), 0);

    u32 *src = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      src[i] = (u32)randu32 ();
    }

    test_assert_int_equal (nsdb_begin (db), 0);
    test_assert_int_equal (
        nsdb_execute (db, "insert foo %d %d", src, 0, ITERS),
        ITERS
    );
    test_assert_int_equal (nsdb_commit (db), 0);

    nsdb_var_t *var;
    test_assert_int_equal (nsdb_execute (db, "get foo", &var), 0);
    test_assert_int_equal (nsdb_var_len (var), ITERS);
    nsdb_var_free (var);

    u32 *dst = i_malloc (ITERS * sizeof (u32), 1, NULL);
    nsdb_execute (db, "read foo[:]", dst);
    for (int i = 0; i < ITERS; ++i)
    {
      test_assert_int_equal (dst[i], src[i]);
    }
    i_free (src);
    i_free (dst);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("insert_rollback_len_unchanged")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_execute (db, "create foo u32", NULL), 0);

    nsdb_var_t *var;
    test_assert_int_equal (nsdb_execute (db, "get foo", &var), 0);
    test_assert_int_equal (nsdb_var_len (var), 0);
    nsdb_var_free (var);

    u32 *src = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      src[i] = (u32)randu32 ();
    }

    test_assert_int_equal (nsdb_begin (db), 0);
    test_assert_int_equal (
        nsdb_execute (db, "insert foo %d %d", src, 0, ITERS),
        ITERS
    );
    test_assert_int_equal (nsdb_rollback (db), 0);

    test_assert_int_equal (nsdb_execute (db, "get foo", &var), 0);
    test_assert_int_equal (nsdb_var_len (var), 0);
    nsdb_var_free (var);

    i_free (src);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("insert_rollback_data_reverts")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_execute (db, "create foo u32", NULL), 0);

    u32 *initial = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      initial[i] = (u32)randu32 ();
    }
    test_assert_int_equal (
        nsdb_execute (db, "insert foo %d %d", initial, 0, ITERS),
        ITERS
    );

    u32 *extra = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      extra[i] = (u32)randu32 ();
    }

    test_assert_int_equal (nsdb_begin (db), 0);
    test_assert_int_equal (
        nsdb_execute (db, "insert foo %d %d", extra, ITERS, ITERS),
        ITERS
    );

    nsdb_var_t *var;
    test_assert_int_equal (nsdb_execute (db, "get foo", &var), 0);
    test_assert_int_equal (nsdb_var_len (var), ITERS * 2);
    nsdb_var_free (var);

    test_assert_int_equal (nsdb_rollback (db), 0);

    test_assert_int_equal (nsdb_execute (db, "get foo", &var), 0);
    test_assert_int_equal (nsdb_var_len (var), ITERS);
    nsdb_var_free (var);

    u32 *dst = i_malloc (ITERS * sizeof (u32), 1, NULL);
    nsdb_execute (db, "read foo[:]", dst);
    for (int i = 0; i < ITERS; ++i)
    {
      test_assert_int_equal (dst[i], initial[i]);
    }
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
    test_assert_int_equal (nsdb_execute (db, "create foo u32", NULL), 0);

    for (int i = 0; i < ITERS; ++i)
    {
      u32 val = (u32)randu32 ();
      test_assert_int_equal (
          nsdb_execute (db, "insert foo %d %d", &val, i, 1),
          1
      );
      nsdb_var_t *var;
      test_assert_int_equal (nsdb_execute (db, "get foo", &var), 0);
      test_assert_int_equal (nsdb_var_len (var), i + 1);
      nsdb_var_free (var);
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("insert_at_front_preserves_order")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_execute (db, "create foo u32", NULL), 0);

    u32 *vals = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      vals[i] = (u32)randu32 ();
    }
    for (int i = ITERS - 1; i >= 0; --i)
    {
      test_assert_int_equal (
          nsdb_execute (db, "insert foo %d %d", &vals[i], 0, 1),
          1
      );
    }

    nsdb_var_t *var;
    test_assert_int_equal (nsdb_execute (db, "get foo", &var), 0);
    test_assert_int_equal (nsdb_var_len (var), ITERS);
    nsdb_var_free (var);

    u32 *dst = i_malloc (ITERS * sizeof (u32), 1, NULL);
    nsdb_execute (db, "read foo[:]", dst);
    for (int i = 0; i < ITERS; ++i)
    {
      test_assert_int_equal (dst[i], vals[i]);
    }
    i_free (vals);
    i_free (dst);
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("insert_rollback_N_times_data_stable")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_execute (db, "create foo u32", NULL), 0);

    u32 *initial = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      initial[i] = (u32)randu32 ();
    }
    test_assert_int_equal (
        nsdb_execute (db, "insert foo %d %d", initial, 0, ITERS),
        ITERS
    );

    u32 *extra = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      extra[i] = (u32)randu32 ();
      test_assert_int_equal (nsdb_begin (db), 0);
      test_assert_int_equal (
          nsdb_execute (db, "insert foo %d %d", &extra[i], ITERS, 1),
          1
      );
      test_assert_int_equal (nsdb_rollback (db), 0);

      nsdb_var_t *var;
      test_assert_int_equal (nsdb_execute (db, "get foo", &var), 0);
      test_assert_int_equal (nsdb_var_len (var), ITERS);
      nsdb_var_free (var);
    }

    u32 *dst = i_malloc (ITERS * sizeof (u32), 1, NULL);
    nsdb_execute (db, "read foo[:]", dst);
    for (int i = 0; i < ITERS; ++i)
    {
      test_assert_int_equal (dst[i], initial[i]);
    }
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

    u32 *vals = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      vals[i] = (u32)randu32 ();
      test_assert_int_equal (
          nsdb_execute (db, "create var_%d u32", NULL, i),
          0
      );
      test_assert_int_equal (
          nsdb_execute (db, "insert var_%d %d %d", &vals[i], i, 0, 1),
          1
      );
    }
    for (int i = 0; i < ITERS; ++i)
    {
      nsdb_var_t *var;
      test_assert_int_equal (nsdb_execute (db, "get var_%d", &var, i), 0);
      test_assert_int_equal (nsdb_var_len (var), 1);
      nsdb_var_free (var);

      u32 dst = 0;
      nsdb_execute (db, "read var_%d[:]", &dst, i);
      test_assert_int_equal (dst, vals[i]);
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
    test_assert_int_equal (nsdb_execute (db, "create foo u32", NULL), 0);

    u32 *initial = i_malloc (ITERS * sizeof (u32), 1, NULL);
    u32 *patch   = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      initial[i] = (u32)randu32 ();
      patch[i]   = (u32)randu32 ();
    }
    test_assert_int_equal (
        nsdb_execute (db, "insert foo %d %d", initial, 0, ITERS),
        ITERS
    );

    test_assert_int_equal (nsdb_begin (db), 0);
    nsdb_execute (db, "write foo[0:%d:1]", patch, ITERS);
    test_assert_int_equal (nsdb_commit (db), 0);

    u32 *dst = i_malloc (ITERS * sizeof (u32), 1, NULL);
    nsdb_execute (db, "read foo[:]", dst);
    for (int i = 0; i < ITERS; ++i)
    {
      test_assert_int_equal (dst[i], patch[i]);
    }
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
    test_assert_int_equal (nsdb_execute (db, "create foo u32", NULL), 0);

    u32 *initial = i_malloc (ITERS * sizeof (u32), 1, NULL);
    u32 *patch   = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      initial[i] = (u32)randu32 ();
      patch[i]   = (u32)randu32 ();
    }
    test_assert_int_equal (
        nsdb_execute (db, "insert foo %d %d", initial, 0, ITERS),
        ITERS
    );

    test_assert_int_equal (nsdb_begin (db), 0);
    nsdb_execute (db, "write foo[0:%d:1]", patch, ITERS);
    test_assert_int_equal (nsdb_rollback (db), 0);

    u32 *dst = i_malloc (ITERS * sizeof (u32), 1, NULL);
    nsdb_execute (db, "read foo[:]", dst);
    for (int i = 0; i < ITERS; ++i)
    {
      test_assert_int_equal (dst[i], initial[i]);
    }
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
    test_assert_int_equal (nsdb_execute (db, "create foo u32", NULL), 0);

    u32 *data = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      data[i] = (u32)randu32 ();
    }
    test_assert_int_equal (
        nsdb_execute (db, "insert foo %d %d", data, 0, ITERS),
        ITERS
    );
    i_free (data);

    for (int i = 0; i < ITERS; ++i)
    {
      u32 val = (u32)randu32 ();
      nsdb_execute (db, "write foo[%d:%d:1]", &val, i, i + 1);

      nsdb_var_t *var;
      test_assert_int_equal (nsdb_execute (db, "get foo", &var), 0);
      test_assert_int_equal (nsdb_var_len (var), ITERS);
      nsdb_var_free (var);
    }
    test_assert_int_equal (nsdb_close (db), 0);
  }

  TEST_CASE ("write_single_element_others_unchanged")
  {
    test_assert_int_equal (nsh_cleanup ("test"), 0);
    nsdb_t *db = nsdb_open ("test");
    test_assert (db != NULL);
    test_assert_int_equal (nsdb_execute (db, "create foo u32", NULL), 0);

    u32 *shadow = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      shadow[i] = (u32)randu32 ();
    }
    test_assert_int_equal (
        nsdb_execute (db, "insert foo %d %d", shadow, 0, ITERS),
        ITERS
    );

    u32 *dst = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      int idx     = randu32 () % ITERS;
      u32 val     = (u32)randu32 ();
      shadow[idx] = val;
      nsdb_execute (db, "write foo[%d:%d:1]", &val, idx, idx + 1);
      nsdb_execute (db, "read foo[:]", dst);
      for (int j = 0; j < ITERS; ++j)
      {
        test_assert_int_equal (dst[j], shadow[j]);
      }
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
    test_assert_int_equal (nsdb_execute (db, "create foo u32", NULL), 0);

    u32 *initial = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      initial[i] = (u32)randu32 ();
    }
    test_assert_int_equal (
        nsdb_execute (db, "insert foo %d %d", initial, 0, ITERS),
        ITERS
    );

    u32 *dst = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      int idx = randu32 () % ITERS;
      u32 val = (u32)randu32 ();
      test_assert_int_equal (nsdb_begin (db), 0);
      nsdb_execute (db, "write foo[%d:%d:1]", &val, idx, idx + 1);
      test_assert_int_equal (nsdb_rollback (db), 0);
      nsdb_execute (db, "read foo[:]", dst);
      for (int j = 0; j < ITERS; ++j)
      {
        test_assert_int_equal (dst[j], initial[j]);
      }
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
    test_assert_int_equal (nsdb_execute (db, "create foo u32", NULL), 0);

    u32 *data = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < ITERS; ++i)
    {
      data[i] = 0;
    }
    test_assert_int_equal (
        nsdb_execute (db, "insert foo %d %d", data, 0, ITERS),
        ITERS
    );

    u32 *dst = i_malloc (ITERS * sizeof (u32), 1, NULL);
    for (int i = 0; i < REOPEN_ITERS; ++i)
    {
      u32 val   = (u32)randu32 ();
      int idx   = randu32 () % ITERS;
      data[idx] = val;

      test_assert_int_equal (nsdb_begin (db), 0);
      nsdb_execute (db, "write foo[%d:%d:1]", &val, idx, idx + 1);
      test_assert_int_equal (nsdb_commit (db), 0);
      test_assert_int_equal (nsdb_close (db), 0);

      db = nsdb_open ("test");
      test_assert (db != NULL);
      nsdb_execute (db, "read foo[:]", dst);
      for (int j = 0; j < ITERS; ++j)
      {
        test_assert_int_equal (dst[j], data[j]);
      }
    }
    i_free (data);
    i_free (dst);
    test_assert_int_equal (nsdb_close (db), 0);
  }
}
#endif
