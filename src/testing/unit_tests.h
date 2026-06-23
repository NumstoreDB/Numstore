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

// AUTO GENERATED - DO NOT MODIFY

#include <stdio.h>
#include <string.h>

#include "error.h"
#include "logging.h"
#include "os.h"
#include "testing/testing.h"

HEADER_FUNC int
run_unit_tests (const char *filter)
{
  int ntests = 0;

  error   e = error_create ();
  i_timer timer;
  if (i_timer_create (&timer, &e) != SUCCESS)
  {
    return -1;
  }

  int         failed = 0;
  const char *failed_names[287];

  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:142 START
  if (!filter || strstr ("lalloc_edge_cases", filter))
  {
    extern void __test__lalloc_edge_cases (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "lalloc_edge_cases"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__lalloc_edge_cases ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "lalloc_edge_cases");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lalloc_edge_cases";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:142 DONE

  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:264 START
  if (!filter || strstr ("bobjp_create", filter))
  {
    extern void __test__bobjp_create (void);
    i_log_info ("========================= TEST CASE: %s\n", "bobjp_create");
    int prev = test_ret;
    test_ret = 0;
    __test__bobjp_create ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "bobjp_create");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "bobjp_create";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:264 DONE

  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:336 START
  if (!filter || strstr ("bobjp_destroy", filter))
  {
    extern void __test__bobjp_destroy (void);
    i_log_info ("========================= TEST CASE: %s\n", "bobjp_destroy");
    int prev = test_ret;
    test_ret = 0;
    __test__bobjp_destroy ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "bobjp_destroy");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "bobjp_destroy";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:336 DONE

  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:438 START
  if (!filter || strstr ("bobjp_alloc", filter))
  {
    extern void __test__bobjp_alloc (void);
    i_log_info ("========================= TEST CASE: %s\n", "bobjp_alloc");
    int prev = test_ret;
    test_ret = 0;
    __test__bobjp_alloc ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "bobjp_alloc");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "bobjp_alloc";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:438 DONE

  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:787 START
  if (!filter || strstr ("slab_alloc_simple", filter))
  {
    extern void __test__slab_alloc_simple (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "slab_alloc_simple"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__slab_alloc_simple ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "slab_alloc_simple");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "slab_alloc_simple";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:787 DONE

  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:903 START
  if (!filter || strstr ("slab_alloc_cap_one", filter))
  {
    extern void __test__slab_alloc_cap_one (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "slab_alloc_cap_one"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__slab_alloc_cap_one ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "slab_alloc_cap_one");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "slab_alloc_cap_one";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:903 DONE

  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:939 START
  if (!filter || strstr ("slab_alloc_no_duplicates", filter))
  {
    extern void __test__slab_alloc_no_duplicates (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "slab_alloc_no_duplicates"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__slab_alloc_no_duplicates ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "slab_alloc_no_duplicates");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "slab_alloc_no_duplicates";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:939 DONE

  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:968 START
  if (!filter || strstr ("slab_alloc_free_all_realloc", filter))
  {
    extern void __test__slab_alloc_free_all_realloc (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "slab_alloc_free_all_realloc"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__slab_alloc_free_all_realloc ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "slab_alloc_free_all_realloc");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "slab_alloc_free_all_realloc";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:968 DONE

  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:1011 START
  if (!filter || strstr ("slab_alloc_interleaved_patterns", filter))
  {
    extern void __test__slab_alloc_interleaved_patterns (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "slab_alloc_interleaved_patterns"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__slab_alloc_interleaved_patterns ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "slab_alloc_interleaved_patterns");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "slab_alloc_interleaved_patterns";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:1011 DONE

  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:1063 START
  if (!filter || strstr ("slab_alloc_free_head_slab", filter))
  {
    extern void __test__slab_alloc_free_head_slab (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "slab_alloc_free_head_slab"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__slab_alloc_free_head_slab ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "slab_alloc_free_head_slab");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "slab_alloc_free_head_slab";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:1063 DONE

  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:1107 START
  if (!filter || strstr ("slab_alloc_free_middle_slab", filter))
  {
    extern void __test__slab_alloc_free_middle_slab (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "slab_alloc_free_middle_slab"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__slab_alloc_free_middle_slab ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "slab_alloc_free_middle_slab");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "slab_alloc_free_middle_slab";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:1107 DONE

  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:1153 START
  if (!filter || strstr ("slab_alloc_minimum_size", filter))
  {
    extern void __test__slab_alloc_minimum_size (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "slab_alloc_minimum_size"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__slab_alloc_minimum_size ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "slab_alloc_minimum_size");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "slab_alloc_minimum_size";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:1153 DONE

  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:1182 START
  if (!filter || strstr ("slab_alloc_stress_random", filter))
  {
    extern void __test__slab_alloc_stress_random (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "slab_alloc_stress_random"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__slab_alloc_stress_random ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "slab_alloc_stress_random");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "slab_alloc_stress_random";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:1182 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:31
  /// START
  if (!filter || strstr ("llist", filter))
  {
    extern void __test__llist (void);
    i_log_info ("========================= TEST CASE: %s\n", "llist");
    int prev = test_ret;
    test_ret = 0;
    __test__llist ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "llist");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "llist";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:31
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:99
  /// START
  if (!filter || strstr ("cbuffer_isempty", filter))
  {
    extern void __test__cbuffer_isempty (void);
    i_log_info ("========================= TEST CASE: %s\n", "cbuffer_isempty");
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_isempty ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "cbuffer_isempty");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_isempty";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:99
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:109
  /// START
  if (!filter || strstr ("cbuffer_len", filter))
  {
    extern void __test__cbuffer_len (void);
    i_log_info ("========================= TEST CASE: %s\n", "cbuffer_len");
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_len ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "cbuffer_len");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_len";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:109
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:119
  /// START
  if (!filter || strstr ("cbuffer_avail", filter))
  {
    extern void __test__cbuffer_avail (void);
    i_log_info ("========================= TEST CASE: %s\n", "cbuffer_avail");
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_avail ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "cbuffer_avail");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_avail";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:119
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:169
  /// START
  if (!filter || strstr ("cbuffer_get_next_data_bytes", filter))
  {
    extern void __test__cbuffer_get_next_data_bytes (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "cbuffer_get_next_data_bytes"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_get_next_data_bytes ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "cbuffer_get_next_data_bytes");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_get_next_data_bytes";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:169
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:242
  /// START
  if (!filter || strstr ("cbuffer_get_nbytes", filter))
  {
    extern void __test__cbuffer_get_nbytes (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "cbuffer_get_nbytes"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_get_nbytes ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "cbuffer_get_nbytes");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_get_nbytes";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:242
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:309
  /// START
  if (!filter || strstr ("cbuffer_fakewrite", filter))
  {
    extern void __test__cbuffer_fakewrite (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "cbuffer_fakewrite"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_fakewrite ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "cbuffer_fakewrite");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_fakewrite";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:309
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:396
  /// START
  if (!filter || strstr ("cbuffer_fakeread", filter))
  {
    extern void __test__cbuffer_fakeread (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "cbuffer_fakeread"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_fakeread ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "cbuffer_fakeread");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_fakeread";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:396
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:517
  /// START
  if (!filter || strstr ("cbuffer_read", filter))
  {
    extern void __test__cbuffer_read (void);
    i_log_info ("========================= TEST CASE: %s\n", "cbuffer_read");
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_read ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "cbuffer_read");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_read";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:517
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:589
  /// START
  if (!filter || strstr ("cbuffer_copy", filter))
  {
    extern void __test__cbuffer_copy (void);
    i_log_info ("========================= TEST CASE: %s\n", "cbuffer_copy");
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_copy ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "cbuffer_copy");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_copy";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:589
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:678
  /// START
  if (!filter || strstr ("cbuffer_write", filter))
  {
    extern void __test__cbuffer_write (void);
    i_log_info ("========================= TEST CASE: %s\n", "cbuffer_write");
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_write ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "cbuffer_write");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_write";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:678
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:772
  /// START
  if (!filter || strstr ("cbuffer_cbuffer_move", filter))
  {
    extern void __test__cbuffer_cbuffer_move (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "cbuffer_cbuffer_move"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_cbuffer_move ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "cbuffer_cbuffer_move");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_cbuffer_move";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:772
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:881
  /// START
  if (!filter || strstr ("cbuffer_cbuffer_copy", filter))
  {
    extern void __test__cbuffer_cbuffer_copy (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "cbuffer_cbuffer_copy"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_cbuffer_copy ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "cbuffer_cbuffer_copy");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_cbuffer_copy";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:881
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1161
  /// START
  if (!filter || strstr ("cbuffer_get_no_check", filter))
  {
    extern void __test__cbuffer_get_no_check (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "cbuffer_get_no_check"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_get_no_check ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "cbuffer_get_no_check");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_get_no_check";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1161
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1263
  /// START
  if (!filter || strstr ("cbuffer_get", filter))
  {
    extern void __test__cbuffer_get (void);
    i_log_info ("========================= TEST CASE: %s\n", "cbuffer_get");
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_get ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "cbuffer_get");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_get";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1263
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1294
  /// START
  if (!filter || strstr ("cbuffer_peek_back", filter))
  {
    extern void __test__cbuffer_peek_back (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "cbuffer_peek_back"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_peek_back ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "cbuffer_peek_back");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_peek_back";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1294
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1350
  /// START
  if (!filter || strstr ("cbuffer_peek_front", filter))
  {
    extern void __test__cbuffer_peek_front (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "cbuffer_peek_front"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_peek_front ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "cbuffer_peek_front");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_peek_front";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1350
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1424
  /// START
  if (!filter || strstr ("cbuffer_push_back", filter))
  {
    extern void __test__cbuffer_push_back (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "cbuffer_push_back"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_push_back ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "cbuffer_push_back");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_push_back";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1424
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1485
  /// START
  if (!filter || strstr ("cbuffer_push_front", filter))
  {
    extern void __test__cbuffer_push_front (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "cbuffer_push_front"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_push_front ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "cbuffer_push_front");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_push_front";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1485
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1556
  /// START
  if (!filter || strstr ("cbuffer_pop_back", filter))
  {
    extern void __test__cbuffer_pop_back (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "cbuffer_pop_back"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_pop_back ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "cbuffer_pop_back");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_pop_back";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1556
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1627
  /// START
  if (!filter || strstr ("cbuffer_pop_front", filter))
  {
    extern void __test__cbuffer_pop_front (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "cbuffer_pop_front"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_pop_front ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "cbuffer_pop_front");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_pop_front";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1627
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1763
  /// START
  if (!filter || strstr ("dblb_create_basic", filter))
  {
    extern void __test__dblb_create_basic (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "dblb_create_basic"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__dblb_create_basic ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dblb_create_basic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_create_basic";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1763
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1779
  /// START
  if (!filter || strstr ("dblb_append_single", filter))
  {
    extern void __test__dblb_append_single (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "dblb_append_single"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__dblb_append_single ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dblb_append_single");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_append_single";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1779
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1800
  /// START
  if (!filter || strstr ("dblb_append_multiple", filter))
  {
    extern void __test__dblb_append_multiple (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "dblb_append_multiple"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__dblb_append_multiple ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dblb_append_multiple");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_append_multiple";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1800
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1822
  /// START
  if (!filter || strstr ("dblb_append_triggers_realloc", filter))
  {
    extern void __test__dblb_append_triggers_realloc (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "dblb_append_triggers_realloc"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__dblb_append_triggers_realloc ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dblb_append_triggers_realloc");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_append_triggers_realloc";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1822
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1859
  /// START
  if (!filter || strstr ("dblb_append_alloc_basic", filter))
  {
    extern void __test__dblb_append_alloc_basic (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "dblb_append_alloc_basic"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__dblb_append_alloc_basic ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dblb_append_alloc_basic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_append_alloc_basic";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1859
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1884
  /// START
  if (!filter || strstr ("dblb_append_alloc_sequential", filter))
  {
    extern void __test__dblb_append_alloc_sequential (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "dblb_append_alloc_sequential"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__dblb_append_alloc_sequential ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dblb_append_alloc_sequential");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_append_alloc_sequential";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1884
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1915
  /// START
  if (!filter || strstr ("dblb_append_alloc_triggers_realloc", filter))
  {
    extern void __test__dblb_append_alloc_triggers_realloc (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "dblb_append_alloc_triggers_realloc"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__dblb_append_alloc_triggers_realloc ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dblb_append_alloc_triggers_realloc");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_append_alloc_triggers_realloc";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1915
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1935
  /// START
  if (!filter || strstr ("dblb_different_element_sizes", filter))
  {
    extern void __test__dblb_different_element_sizes (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "dblb_different_element_sizes"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__dblb_different_element_sizes ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dblb_different_element_sizes");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_different_element_sizes";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1935
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1955
  /// START
  if (!filter || strstr ("dblb_struct_elements", filter))
  {
    extern void __test__dblb_struct_elements (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "dblb_struct_elements"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__dblb_struct_elements ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dblb_struct_elements");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_struct_elements";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1955
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1983
  /// START
  if (!filter || strstr ("dblb_free_resets", filter))
  {
    extern void __test__dblb_free_resets (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "dblb_free_resets"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__dblb_free_resets ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dblb_free_resets");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_free_resets";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1983
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1999
  /// START
  if (!filter || strstr ("dblb_large_append", filter))
  {
    extern void __test__dblb_large_append (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "dblb_large_append"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__dblb_large_append ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dblb_large_append");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_large_append";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1999
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:2279
  /// START
  if (!filter || strstr ("ext_array_insert_read", filter))
  {
    extern void __test__ext_array_insert_read (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "ext_array_insert_read"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__ext_array_insert_read ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "ext_array_insert_read");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ext_array_insert_read";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:2279
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:2418
  /// START
  if (!filter || strstr ("ext_array_write", filter))
  {
    extern void __test__ext_array_write (void);
    i_log_info ("========================= TEST CASE: %s\n", "ext_array_write");
    int prev = test_ret;
    test_ret = 0;
    __test__ext_array_write ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "ext_array_write");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ext_array_write";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:2418
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:2524
  /// START
  if (!filter || strstr ("ext_array_remove", filter))
  {
    extern void __test__ext_array_remove (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "ext_array_remove"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__ext_array_remove ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "ext_array_remove");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ext_array_remove";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:2524
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:2691
  /// START
  if (!filter || strstr ("ext_array_random", filter))
  {
    extern void __test__ext_array_random (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "ext_array_random"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__ext_array_random ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "ext_array_random");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ext_array_random";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:2691
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:3423
  /// START
  if (!filter || strstr ("block_insert_read", filter))
  {
    extern void __test__block_insert_read (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "block_insert_read"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__block_insert_read ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "block_insert_read");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "block_insert_read";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:3423
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:3683
  /// START
  if (!filter || strstr ("block_insert_remove_read", filter))
  {
    extern void __test__block_insert_remove_read (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "block_insert_remove_read"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__block_insert_remove_read ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "block_insert_remove_read");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "block_insert_remove_read";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:3683
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:3828
  /// START
  if (!filter || strstr ("block_insert_write_read", filter))
  {
    extern void __test__block_insert_write_read (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "block_insert_write_read"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__block_insert_write_read ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "block_insert_write_read");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "block_insert_write_read";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:3828
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:3991
  /// START
  if (!filter || strstr ("block_random", filter))
  {
    extern void __test__block_random (void);
    i_log_info ("========================= TEST CASE: %s\n", "block_random");
    int prev = test_ret;
    test_ret = 0;
    __test__block_random ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "block_random");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "block_random";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:3991
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:4080
  /// START
  if (!filter || strstr ("ba_memcpy_from_basic", filter))
  {
    extern void __test__ba_memcpy_from_basic (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "ba_memcpy_from_basic"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__ba_memcpy_from_basic ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "ba_memcpy_from_basic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ba_memcpy_from_basic";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:4080
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:4410
  /// START
  if (!filter || strstr ("ba_memcpy_to_basic", filter))
  {
    extern void __test__ba_memcpy_to_basic (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "ba_memcpy_to_basic"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__ba_memcpy_to_basic ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "ba_memcpy_to_basic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ba_memcpy_to_basic";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:4410
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:695 START
  if (!filter || strstr ("lexer_two_char_tokens", filter))
  {
    extern void __test__lexer_two_char_tokens (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "lexer_two_char_tokens"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__lexer_two_char_tokens ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "lexer_two_char_tokens");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_two_char_tokens";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:695 DONE

  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:721 START
  if (!filter || strstr ("lexer_single_char_operators", filter))
  {
    extern void __test__lexer_single_char_operators (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "lexer_single_char_operators"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__lexer_single_char_operators ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "lexer_single_char_operators");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_single_char_operators";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:721 DONE

  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:749 START
  if (!filter || strstr ("lexer_strings", filter))
  {
    extern void __test__lexer_strings (void);
    i_log_info ("========================= TEST CASE: %s\n", "lexer_strings");
    int prev = test_ret;
    test_ret = 0;
    __test__lexer_strings ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "lexer_strings");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_strings";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:749 DONE

  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:762 START
  if (!filter || strstr ("lexer_identifiers", filter))
  {
    extern void __test__lexer_identifiers (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "lexer_identifiers"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__lexer_identifiers ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "lexer_identifiers");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_identifiers";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:762 DONE

  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:776 START
  if (!filter || strstr ("lexer_numbers", filter))
  {
    extern void __test__lexer_numbers (void);
    i_log_info ("========================= TEST CASE: %s\n", "lexer_numbers");
    int prev = test_ret;
    test_ret = 0;
    __test__lexer_numbers ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "lexer_numbers");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_numbers";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:776 DONE

  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:792 START
  if (!filter || strstr ("lexer_keywords", filter))
  {
    extern void __test__lexer_keywords (void);
    i_log_info ("========================= TEST CASE: %s\n", "lexer_keywords");
    int prev = test_ret;
    test_ret = 0;
    __test__lexer_keywords ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "lexer_keywords");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_keywords";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:792 DONE

  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:815 START
  if (!filter || strstr ("lexer_primitives", filter))
  {
    extern void __test__lexer_primitives (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "lexer_primitives"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__lexer_primitives ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "lexer_primitives");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_primitives";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:815 DONE

  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:835 START
  if (!filter || strstr ("lexer_whitespace_handling", filter))
  {
    extern void __test__lexer_whitespace_handling (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "lexer_whitespace_handling"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__lexer_whitespace_handling ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "lexer_whitespace_handling");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_whitespace_handling";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:835 DONE

  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:853 START
  if (!filter || strstr ("lexer_complex_expression", filter))
  {
    extern void __test__lexer_complex_expression (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "lexer_complex_expression"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__lexer_complex_expression ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "lexer_complex_expression");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_complex_expression";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:853 DONE

  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:875 START
  if (!filter || strstr ("lexer_keyword_prefix", filter))
  {
    extern void __test__lexer_keyword_prefix (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "lexer_keyword_prefix"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__lexer_keyword_prefix ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "lexer_keyword_prefix");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_keyword_prefix";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:875 DONE

  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:891 START
  if (!filter || strstr ("lexer_errors", filter))
  {
    extern void __test__lexer_errors (void);
    i_log_info ("========================= TEST CASE: %s\n", "lexer_errors");
    int prev = test_ret;
    test_ret = 0;
    __test__lexer_errors ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "lexer_errors");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_errors";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:891 DONE

  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:903 START
  if (!filter || strstr ("lexer_empty_string", filter))
  {
    extern void __test__lexer_empty_string (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "lexer_empty_string"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__lexer_empty_string ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "lexer_empty_string");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_empty_string";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:903 DONE

  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:914 START
  if (!filter || strstr ("lexer_numbers_in_sequence", filter))
  {
    extern void __test__lexer_numbers_in_sequence (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "lexer_numbers_in_sequence"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__lexer_numbers_in_sequence ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "lexer_numbers_in_sequence");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_numbers_in_sequence";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:914 DONE

  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:58
  /// START
  if (!filter || strstr ("gr_lock_init", filter))
  {
    extern void __test__gr_lock_init (void);
    i_log_info ("========================= TEST CASE: %s\n", "gr_lock_init");
    int prev = test_ret;
    test_ret = 0;
    __test__gr_lock_init ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "gr_lock_init");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "gr_lock_init";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:58
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:103
  /// START
  if (!filter || strstr ("gr_lock_destroy", filter))
  {
    extern void __test__gr_lock_destroy (void);
    i_log_info ("========================= TEST CASE: %s\n", "gr_lock_destroy");
    int prev = test_ret;
    test_ret = 0;
    __test__gr_lock_destroy ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "gr_lock_destroy");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "gr_lock_destroy";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:103
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:149
  /// START
  if (!filter || strstr ("gr_lock_is_compatible", filter))
  {
    extern void __test__gr_lock_is_compatible (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "gr_lock_is_compatible"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__gr_lock_is_compatible ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "gr_lock_is_compatible");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "gr_lock_is_compatible";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:149
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:382
  /// START
  if (!filter || strstr ("gr_lock_unlock", filter))
  {
    extern void __test__gr_lock_unlock (void);
    i_log_info ("========================= TEST CASE: %s\n", "gr_lock_unlock");
    int prev = test_ret;
    test_ret = 0;
    __test__gr_lock_unlock ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "gr_lock_unlock");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "gr_lock_unlock";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:382
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:480
  /// START
  if (!filter || strstr ("gr_lock_mode_name", filter))
  {
    extern void __test__gr_lock_mode_name (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "gr_lock_mode_name"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__gr_lock_mode_name ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "gr_lock_mode_name");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "gr_lock_mode_name";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:480
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:639
  /// START
  if (!filter || strstr ("gr_lock_basic_sanity", filter))
  {
    extern void __test__gr_lock_basic_sanity (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "gr_lock_basic_sanity"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__gr_lock_basic_sanity ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "gr_lock_basic_sanity");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "gr_lock_basic_sanity";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:639
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:656
  /// START
  if (!filter || strstr ("gr_lock_is_is_compatible", filter))
  {
    extern void __test__gr_lock_is_is_compatible (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "gr_lock_is_is_compatible"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__gr_lock_is_is_compatible ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "gr_lock_is_is_compatible");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "gr_lock_is_is_compatible";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:656
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:712
  /// START
  if (!filter || strstr ("gr_lock_high_pressure_random", filter))
  {
    extern void __test__gr_lock_high_pressure_random (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "gr_lock_high_pressure_random"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__gr_lock_high_pressure_random ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "gr_lock_high_pressure_random");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "gr_lock_high_pressure_random";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:712
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:902
  /// START
  if (!filter || strstr ("latch", filter))
  {
    extern void __test__latch (void);
    i_log_info ("========================= TEST CASE: %s\n", "latch");
    int prev = test_ret;
    test_ret = 0;
    __test__latch ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "latch");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "latch";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:902
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/file_pager.c:127
  /// START
  if (!filter || strstr ("fpgr_open", filter))
  {
    extern void __test__fpgr_open (void);
    i_log_info ("========================= TEST CASE: %s\n", "fpgr_open");
    int prev = test_ret;
    test_ret = 0;
    __test__fpgr_open ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "fpgr_open");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "fpgr_open";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/file_pager.c:127
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/file_pager.c:236
  /// START
  if (!filter || strstr ("fpgr_new", filter))
  {
    extern void __test__fpgr_new (void);
    i_log_info ("========================= TEST CASE: %s\n", "fpgr_new");
    int prev = test_ret;
    test_ret = 0;
    __test__fpgr_new ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "fpgr_new");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "fpgr_new";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/file_pager.c:236
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/file_pager.c:415
  /// START
  if (!filter || strstr ("fpgr_read_write", filter))
  {
    extern void __test__fpgr_read_write (void);
    i_log_info ("========================= TEST CASE: %s\n", "fpgr_read_write");
    int prev = test_ret;
    test_ret = 0;
    __test__fpgr_read_write ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "fpgr_read_write");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "fpgr_read_write";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/file_pager.c:415
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/htable.c:201 START
  if (!filter || strstr ("htable", filter))
  {
    extern void __test__htable (void);
    i_log_info ("========================= TEST CASE: %s\n", "htable");
    int prev = test_ret;
    test_ret = 0;
    __test__htable ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "htable");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "htable";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/htable.c:201 DONE

  //////////////////// /Users/theo/Development/Numstore/src/htable.c:276 START
  if (!filter || strstr ("fnv1a_hash_empty", filter))
  {
    extern void __test__fnv1a_hash_empty (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "fnv1a_hash_empty"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__fnv1a_hash_empty ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "fnv1a_hash_empty");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "fnv1a_hash_empty";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/htable.c:276 DONE

  //////////////////// /Users/theo/Development/Numstore/src/htable.c:284 START
  if (!filter || strstr ("fnv1a_hash_single_char", filter))
  {
    extern void __test__fnv1a_hash_single_char (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "fnv1a_hash_single_char"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__fnv1a_hash_single_char ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "fnv1a_hash_single_char");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "fnv1a_hash_single_char";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/htable.c:284 DONE

  //////////////////// /Users/theo/Development/Numstore/src/htable.c:293 START
  if (!filter || strstr ("fnv1a_hash_known_value", filter))
  {
    extern void __test__fnv1a_hash_known_value (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "fnv1a_hash_known_value"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__fnv1a_hash_known_value ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "fnv1a_hash_known_value");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "fnv1a_hash_known_value";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/htable.c:293 DONE

  //////////////////// /Users/theo/Development/Numstore/src/htable.c:301 START
  if (!filter || strstr ("fnv1a_hash_deterministic", filter))
  {
    extern void __test__fnv1a_hash_deterministic (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "fnv1a_hash_deterministic"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__fnv1a_hash_deterministic ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "fnv1a_hash_deterministic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "fnv1a_hash_deterministic";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/htable.c:301 DONE

  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:227
  /// START
  if (!filter || strstr ("nupd_init", filter))
  {
    extern void __test__nupd_init (void);
    i_log_info ("========================= TEST CASE: %s\n", "nupd_init");
    int prev = test_ret;
    test_ret = 0;
    __test__nupd_init ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "nupd_init");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_init";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:227
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:300
  /// START
  if (!filter || strstr ("nupd_append_right", filter))
  {
    extern void __test__nupd_append_right (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "nupd_append_right"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__nupd_append_right ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "nupd_append_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_append_right";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:300
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:416
  /// START
  if (!filter || strstr ("nupd_append_left", filter))
  {
    extern void __test__nupd_append_left (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "nupd_append_left"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__nupd_append_left ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "nupd_append_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_append_left";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:416
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:666
  /// START
  if (!filter || strstr ("nupd_append_tip_right", filter))
  {
    extern void __test__nupd_append_tip_right (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "nupd_append_tip_right"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__nupd_append_tip_right ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "nupd_append_tip_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_append_tip_right";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:666
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:866
  /// START
  if (!filter || strstr ("nupd_append_tip_left", filter))
  {
    extern void __test__nupd_append_tip_left (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "nupd_append_tip_left"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__nupd_append_tip_left ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "nupd_append_tip_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_append_tip_left";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:866
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1213
  /// START
  if (!filter || strstr ("nupd_consume_right", filter))
  {
    extern void __test__nupd_consume_right (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "nupd_consume_right"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__nupd_consume_right ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "nupd_consume_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_consume_right";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1213
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1287
  /// START
  if (!filter || strstr ("nupd_consume_left", filter))
  {
    extern void __test__nupd_consume_left (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "nupd_consume_left"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__nupd_consume_left ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "nupd_consume_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_consume_left";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1287
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1357
  /// START
  if (!filter || strstr ("nupd_done_observing_left", filter))
  {
    extern void __test__nupd_done_observing_left (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "nupd_done_observing_left"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__nupd_done_observing_left ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "nupd_done_observing_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_done_observing_left";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1357
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1412
  /// START
  if (!filter || strstr ("nupd_done_observing_right", filter))
  {
    extern void __test__nupd_done_observing_right (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "nupd_done_observing_right"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__nupd_done_observing_right ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "nupd_done_observing_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_done_observing_right";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1412
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1455
  /// START
  if (!filter || strstr ("nupd_done_consuming_left", filter))
  {
    extern void __test__nupd_done_consuming_left (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "nupd_done_consuming_left"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__nupd_done_consuming_left ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "nupd_done_consuming_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_done_consuming_left";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1455
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1499
  /// START
  if (!filter || strstr ("nupd_done_consuming_right", filter))
  {
    extern void __test__nupd_done_consuming_right (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "nupd_done_consuming_right"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__nupd_done_consuming_right ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "nupd_done_consuming_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_done_consuming_right";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1499
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1543
  /// START
  if (!filter || strstr ("nupd_done_left", filter))
  {
    extern void __test__nupd_done_left (void);
    i_log_info ("========================= TEST CASE: %s\n", "nupd_done_left");
    int prev = test_ret;
    test_ret = 0;
    __test__nupd_done_left ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "nupd_done_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_done_left";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1543
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1597
  /// START
  if (!filter || strstr ("nupd_done_right", filter))
  {
    extern void __test__nupd_done_right (void);
    i_log_info ("========================= TEST CASE: %s\n", "nupd_done_right");
    int prev = test_ret;
    test_ret = 0;
    __test__nupd_done_right ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "nupd_done_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_done_right";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1597
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:115 START
  if (!filter || strstr ("checksum_execute_simple", filter))
  {
    extern void __test__checksum_execute_simple (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "checksum_execute_simple"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__checksum_execute_simple ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "checksum_execute_simple");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "checksum_execute_simple";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:115 DONE

  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:126 START
  if (!filter || strstr ("checksum_execute_deterministic", filter))
  {
    extern void __test__checksum_execute_deterministic (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "checksum_execute_deterministic"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__checksum_execute_deterministic ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "checksum_execute_deterministic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "checksum_execute_deterministic";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:126 DONE

  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:138 START
  if (!filter || strstr ("checksum_execute_incremental", filter))
  {
    extern void __test__checksum_execute_incremental (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "checksum_execute_incremental"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__checksum_execute_incremental ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "checksum_execute_incremental");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "checksum_execute_incremental";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:138 DONE

  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:251 START
  if (!filter || strstr ("randu32", filter))
  {
    extern void __test__randu32 (void);
    i_log_info ("========================= TEST CASE: %s\n", "randu32");
    int prev = test_ret;
    test_ret = 0;
    __test__randu32 ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "randu32");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "randu32";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:251 DONE

  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:281 START
  if (!filter || strstr ("randu32r", filter))
  {
    extern void __test__randu32r (void);
    i_log_info ("========================= TEST CASE: %s\n", "randu32r");
    int prev = test_ret;
    test_ret = 0;
    __test__randu32r ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "randu32r");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "randu32r";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:281 DONE

  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:357 START
  if (!filter || strstr ("randi32r", filter))
  {
    extern void __test__randi32r (void);
    i_log_info ("========================= TEST CASE: %s\n", "randi32r");
    int prev = test_ret;
    test_ret = 0;
    __test__randi32r ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "randi32r");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "randi32r";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:357 DONE

  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:495 START
  if (!filter || strstr ("randu64r", filter))
  {
    extern void __test__randu64r (void);
    i_log_info ("========================= TEST CASE: %s\n", "randu64r");
    int prev = test_ret;
    test_ret = 0;
    __test__randu64r ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "randu64r");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "randu64r";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:495 DONE

  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:544 START
  if (!filter || strstr ("randu64e", filter))
  {
    extern void __test__randu64e (void);
    i_log_info ("========================= TEST CASE: %s\n", "randu64e");
    int prev = test_ret;
    test_ret = 0;
    __test__randu64e ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "randu64e");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "randu64e";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:544 DONE

  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:606 START
  if (!filter || strstr ("randi64r", filter))
  {
    extern void __test__randi64r (void);
    i_log_info ("========================= TEST CASE: %s\n", "randi64r");
    int prev = test_ret;
    test_ret = 0;
    __test__randi64r ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "randi64r");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "randi64r";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:606 DONE

  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:628 START
  if (!filter || strstr ("randi64e", filter))
  {
    extern void __test__randi64e (void);
    i_log_info ("========================= TEST CASE: %s\n", "randi64e");
    int prev = test_ret;
    test_ret = 0;
    __test__randi64e ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "randi64e");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "randi64e";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:628 DONE

  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:837 START
  if (!filter || strstr ("parse_i32_expect", filter))
  {
    extern void __test__parse_i32_expect (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "parse_i32_expect"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__parse_i32_expect ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "parse_i32_expect");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "parse_i32_expect";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:837 DONE

  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:988 START
  if (!filter || strstr ("parse_f32_expect", filter))
  {
    extern void __test__parse_f32_expect (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "parse_f32_expect"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__parse_f32_expect ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "parse_f32_expect");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "parse_f32_expect";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:988 DONE

  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:1028
  /// START
  if (!filter || strstr ("py_mod_f32", filter))
  {
    extern void __test__py_mod_f32 (void);
    i_log_info ("========================= TEST CASE: %s\n", "py_mod_f32");
    int prev = test_ret;
    test_ret = 0;
    __test__py_mod_f32 ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "py_mod_f32");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "py_mod_f32";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:1028 DONE

  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:1065
  /// START
  if (!filter || strstr ("py_mod_i32", filter))
  {
    extern void __test__py_mod_i32 (void);
    i_log_info ("========================= TEST CASE: %s\n", "py_mod_i32");
    int prev = test_ret;
    test_ret = 0;
    __test__py_mod_i32 ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "py_mod_i32");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "py_mod_i32";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:1065 DONE

  //////////////////// /Users/theo/Development/Numstore/src/os_common.c:75 START
  if (!filter || strstr ("i_malloc_injection", filter))
  {
    extern void __test__i_malloc_injection (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "i_malloc_injection"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__i_malloc_injection ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "i_malloc_injection");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_malloc_injection";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/os_common.c:75 DONE

  //////////////////// /Users/theo/Development/Numstore/src/os_common.c:169
  /// START
  if (!filter || strstr ("i_realloc_basic", filter))
  {
    extern void __test__i_realloc_basic (void);
    i_log_info ("========================= TEST CASE: %s\n", "i_realloc_basic");
    int prev = test_ret;
    test_ret = 0;
    __test__i_realloc_basic ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "i_realloc_basic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_realloc_basic";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/os_common.c:169 DONE

  //////////////////// /Users/theo/Development/Numstore/src/os_common.c:210
  /// START
  if (!filter || strstr ("i_realloc_right", filter))
  {
    extern void __test__i_realloc_right (void);
    i_log_info ("========================= TEST CASE: %s\n", "i_realloc_right");
    int prev = test_ret;
    test_ret = 0;
    __test__i_realloc_right ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "i_realloc_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_realloc_right";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/os_common.c:210 DONE

  //////////////////// /Users/theo/Development/Numstore/src/os_common.c:309
  /// START
  if (!filter || strstr ("i_realloc_left", filter))
  {
    extern void __test__i_realloc_left (void);
    i_log_info ("========================= TEST CASE: %s\n", "i_realloc_left");
    int prev = test_ret;
    test_ret = 0;
    __test__i_realloc_left ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "i_realloc_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_realloc_left";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/os_common.c:309 DONE

  //////////////////// /Users/theo/Development/Numstore/src/os_common.c:386
  /// START
  if (!filter || strstr ("i_crealloc_right", filter))
  {
    extern void __test__i_crealloc_right (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "i_crealloc_right"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__i_crealloc_right ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "i_crealloc_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_crealloc_right";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/os_common.c:386 DONE

  //////////////////// /Users/theo/Development/Numstore/src/os_common.c:492
  /// START
  if (!filter || strstr ("i_crealloc_left", filter))
  {
    extern void __test__i_crealloc_left (void);
    i_log_info ("========================= TEST CASE: %s\n", "i_crealloc_left");
    int prev = test_ret;
    test_ret = 0;
    __test__i_crealloc_left ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "i_crealloc_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_crealloc_left";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/os_common.c:492 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:154 START
  if (!filter || strstr ("page_set_get_simple", filter))
  {
    extern void __test__page_set_get_simple (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "page_set_get_simple"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__page_set_get_simple ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "page_set_get_simple");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "page_set_get_simple";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:154 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:234 START
  if (!filter || strstr ("i_log_page", filter))
  {
    extern void __test__i_log_page (void);
    i_log_info ("========================= TEST CASE: %s\n", "i_log_page");
    int prev = test_ret;
    test_ret = 0;
    __test__i_log_page ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "i_log_page");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_log_page";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:234 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:310 START
  if (!filter || strstr ("i_log_fsm", filter))
  {
    extern void __test__i_log_fsm (void);
    i_log_info ("========================= TEST CASE: %s\n", "i_log_fsm");
    int prev = test_ret;
    test_ret = 0;
    __test__i_log_fsm ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "i_log_fsm");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_log_fsm";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:310 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:391 START
  if (!filter || strstr ("dl_validate", filter))
  {
    extern void __test__dl_validate (void);
    i_log_info ("========================= TEST CASE: %s\n", "dl_validate");
    int prev = test_ret;
    test_ret = 0;
    __test__dl_validate ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dl_validate");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_validate";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:391 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:473 START
  if (!filter || strstr ("dl_set_get", filter))
  {
    extern void __test__dl_set_get (void);
    i_log_info ("========================= TEST CASE: %s\n", "dl_set_get");
    int prev = test_ret;
    test_ret = 0;
    __test__dl_set_get ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dl_set_get");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_set_get";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:473 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:536 START
  if (!filter || strstr ("dl_read", filter))
  {
    extern void __test__dl_read (void);
    i_log_info ("========================= TEST CASE: %s\n", "dl_read");
    int prev = test_ret;
    test_ret = 0;
    __test__dl_read ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dl_read");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_read";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:536 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:715 START
  if (!filter || strstr ("dl_read_out_from", filter))
  {
    extern void __test__dl_read_out_from (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "dl_read_out_from"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__dl_read_out_from ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dl_read_out_from");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_read_out_from";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:715 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:918 START
  if (!filter || strstr ("dl_append", filter))
  {
    extern void __test__dl_append (void);
    i_log_info ("========================= TEST CASE: %s\n", "dl_append");
    int prev = test_ret;
    test_ret = 0;
    __test__dl_append ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dl_append");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_append";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:918 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:1016 START
  if (!filter || strstr ("dl_write", filter))
  {
    extern void __test__dl_write (void);
    i_log_info ("========================= TEST CASE: %s\n", "dl_write");
    int prev = test_ret;
    test_ret = 0;
    __test__dl_write ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dl_write");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_write";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:1016 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:1146 START
  if (!filter || strstr ("dl_memset", filter))
  {
    extern void __test__dl_memset (void);
    i_log_info ("========================= TEST CASE: %s\n", "dl_memset");
    int prev = test_ret;
    test_ret = 0;
    __test__dl_memset ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dl_memset");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_memset";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:1146 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:1223 START
  if (!filter || strstr ("dl_move_left", filter))
  {
    extern void __test__dl_move_left (void);
    i_log_info ("========================= TEST CASE: %s\n", "dl_move_left");
    int prev = test_ret;
    test_ret = 0;
    __test__dl_move_left ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dl_move_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_move_left";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:1223 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:1303 START
  if (!filter || strstr ("dl_shift_right", filter))
  {
    extern void __test__dl_shift_right (void);
    i_log_info ("========================= TEST CASE: %s\n", "dl_shift_right");
    int prev = test_ret;
    test_ret = 0;
    __test__dl_shift_right ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dl_shift_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_shift_right";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:1303 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:1393 START
  if (!filter || strstr ("dl_move_right", filter))
  {
    extern void __test__dl_move_right (void);
    i_log_info ("========================= TEST CASE: %s\n", "dl_move_right");
    int prev = test_ret;
    test_ret = 0;
    __test__dl_move_right ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dl_move_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_move_right";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:1393 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:1503 START
  if (!filter || strstr ("i_log_dl", filter))
  {
    extern void __test__i_log_dl (void);
    i_log_info ("========================= TEST CASE: %s\n", "i_log_dl");
    int prev = test_ret;
    test_ret = 0;
    __test__i_log_dl ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "i_log_dl");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_log_dl";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:1503 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:1536 START
  if (!filter || strstr ("dl_make_valid", filter))
  {
    extern void __test__dl_make_valid (void);
    i_log_info ("========================= TEST CASE: %s\n", "dl_make_valid");
    int prev = test_ret;
    test_ret = 0;
    __test__dl_make_valid ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dl_make_valid");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_make_valid";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:1536 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:1661 START
  if (!filter || strstr ("in_validate_for_db", filter))
  {
    extern void __test__in_validate_for_db (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "in_validate_for_db"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__in_validate_for_db ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "in_validate_for_db");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_validate_for_db";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:1661 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:1722 START
  if (!filter || strstr ("in_set_get_simple", filter))
  {
    extern void __test__in_set_get_simple (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "in_set_get_simple"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__in_set_get_simple ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "in_set_get_simple");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_set_get_simple";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:1722 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:1780 START
  if (!filter || strstr ("in_push_end", filter))
  {
    extern void __test__in_push_end (void);
    i_log_info ("========================= TEST CASE: %s\n", "in_push_end");
    int prev = test_ret;
    test_ret = 0;
    __test__in_push_end ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "in_push_end");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_push_end";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:1780 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:1845 START
  if (!filter || strstr ("in_memcpy", filter))
  {
    extern void __test__in_memcpy (void);
    i_log_info ("========================= TEST CASE: %s\n", "in_memcpy");
    int prev = test_ret;
    test_ret = 0;
    __test__in_memcpy ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "in_memcpy");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_memcpy";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:1845 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2007 START
  if (!filter || strstr ("in_move_left", filter))
  {
    extern void __test__in_move_left (void);
    i_log_info ("========================= TEST CASE: %s\n", "in_move_left");
    int prev = test_ret;
    test_ret = 0;
    __test__in_move_left ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "in_move_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_move_left";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2007 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2044 START
  if (!filter || strstr ("in_move_left_two_keys", filter))
  {
    extern void __test__in_move_left_two_keys (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "in_move_left_two_keys"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__in_move_left_two_keys ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "in_move_left_two_keys");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_move_left_two_keys";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2044 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2072 START
  if (!filter || strstr ("in_move_left_all_keys", filter))
  {
    extern void __test__in_move_left_all_keys (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "in_move_left_all_keys"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__in_move_left_all_keys ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "in_move_left_all_keys");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_move_left_all_keys";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2072 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2095 START
  if (!filter || strstr ("in_move_left_into_empty", filter))
  {
    extern void __test__in_move_left_into_empty (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "in_move_left_into_empty"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__in_move_left_into_empty ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "in_move_left_into_empty");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_move_left_into_empty";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2095 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2174 START
  if (!filter || strstr ("in_push_left", filter))
  {
    extern void __test__in_push_left (void);
    i_log_info ("========================= TEST CASE: %s\n", "in_push_left");
    int prev = test_ret;
    test_ret = 0;
    __test__in_push_left ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "in_push_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_push_left";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2174 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2205 START
  if (!filter || strstr ("in_push_left_into_empty", filter))
  {
    extern void __test__in_push_left_into_empty (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "in_push_left_into_empty"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__in_push_left_into_empty ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "in_push_left_into_empty");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_push_left_into_empty";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2205 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2218 START
  if (!filter || strstr ("in_push_left_to_full", filter))
  {
    extern void __test__in_push_left_to_full (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "in_push_left_to_full"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__in_push_left_to_full ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "in_push_left_to_full");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_push_left_to_full";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2218 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2278 START
  if (!filter || strstr ("in_move_right", filter))
  {
    extern void __test__in_move_right (void);
    i_log_info ("========================= TEST CASE: %s\n", "in_move_right");
    int prev = test_ret;
    test_ret = 0;
    __test__in_move_right ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "in_move_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_move_right";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2278 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2315 START
  if (!filter || strstr ("in_move_right_two_keys", filter))
  {
    extern void __test__in_move_right_two_keys (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "in_move_right_two_keys"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__in_move_right_two_keys ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "in_move_right_two_keys");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_move_right_two_keys";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2315 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2343 START
  if (!filter || strstr ("in_move_right_all_keys", filter))
  {
    extern void __test__in_move_right_all_keys (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "in_move_right_all_keys"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__in_move_right_all_keys ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "in_move_right_all_keys");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_move_right_all_keys";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2343 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2366 START
  if (!filter || strstr ("in_move_right_into_empty_right", filter))
  {
    extern void __test__in_move_right_into_empty_right (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "in_move_right_into_empty_right"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__in_move_right_into_empty_right ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "in_move_right_into_empty_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_move_right_into_empty_right";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2366 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2423 START
  if (!filter || strstr ("in_choose_lidx", filter))
  {
    extern void __test__in_choose_lidx (void);
    i_log_info ("========================= TEST CASE: %s\n", "in_choose_lidx");
    int prev = test_ret;
    test_ret = 0;
    __test__in_choose_lidx ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "in_choose_lidx");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_choose_lidx";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2423 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2521 START
  if (!filter || strstr ("in_cut_left", filter))
  {
    extern void __test__in_cut_left (void);
    i_log_info ("========================= TEST CASE: %s\n", "in_cut_left");
    int prev = test_ret;
    test_ret = 0;
    __test__in_cut_left ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "in_cut_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_cut_left";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2521 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2559 START
  if (!filter || strstr ("in_cut_left_all_at_once", filter))
  {
    extern void __test__in_cut_left_all_at_once (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "in_cut_left_all_at_once"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__in_cut_left_all_at_once ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "in_cut_left_all_at_once");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_cut_left_all_at_once";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2559 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2577 START
  if (!filter || strstr ("in_cut_left_from_empty", filter))
  {
    extern void __test__in_cut_left_from_empty (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "in_cut_left_from_empty"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__in_cut_left_from_empty ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "in_cut_left_from_empty");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_cut_left_from_empty";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2577 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2590 START
  if (!filter || strstr ("in_cut_left_to_one", filter))
  {
    extern void __test__in_cut_left_to_one (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "in_cut_left_to_one"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__in_cut_left_to_one ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "in_cut_left_to_one");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_cut_left_to_one";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2590 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2662 START
  if (!filter || strstr ("i_log_in", filter))
  {
    extern void __test__i_log_in (void);
    i_log_info ("========================= TEST CASE: %s\n", "i_log_in");
    int prev = test_ret;
    test_ret = 0;
    __test__i_log_in ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "i_log_in");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_log_in";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2662 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2740 START
  if (!filter || strstr ("i_log_vh", filter))
  {
    extern void __test__i_log_vh (void);
    i_log_info ("========================= TEST CASE: %s\n", "i_log_vh");
    int prev = test_ret;
    test_ret = 0;
    __test__i_log_vh ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "i_log_vh");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_log_vh";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2740 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2779 START
  if (!filter || strstr ("vp_init_empty", filter))
  {
    extern void __test__vp_init_empty (void);
    i_log_info ("========================= TEST CASE: %s\n", "vp_init_empty");
    int prev = test_ret;
    test_ret = 0;
    __test__vp_init_empty ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "vp_init_empty");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "vp_init_empty";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2779 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2937 START
  if (!filter || strstr ("vp_validate", filter))
  {
    extern void __test__vp_validate (void);
    i_log_info ("========================= TEST CASE: %s\n", "vp_validate");
    int prev = test_ret;
    test_ret = 0;
    __test__vp_validate ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "vp_validate");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "vp_validate";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2937 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:3035 START
  if (!filter || strstr ("i_log_vp", filter))
  {
    extern void __test__i_log_vp (void);
    i_log_info ("========================= TEST CASE: %s\n", "i_log_vp");
    int prev = test_ret;
    test_ret = 0;
    __test__i_log_vp ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "i_log_vp");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_log_vp";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:3035 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:3068 START
  if (!filter || strstr ("vt_init_empty", filter))
  {
    extern void __test__vt_init_empty (void);
    i_log_info ("========================= TEST CASE: %s\n", "vt_init_empty");
    int prev = test_ret;
    test_ret = 0;
    __test__vt_init_empty ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "vt_init_empty");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "vt_init_empty";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:3068 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:3093 START
  if (!filter || strstr ("vt_validate", filter))
  {
    extern void __test__vt_validate (void);
    i_log_info ("========================= TEST CASE: %s\n", "vt_validate");
    int prev = test_ret;
    test_ret = 0;
    __test__vt_validate ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "vt_validate");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "vt_validate";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:3093 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:3128 START
  if (!filter || strstr ("i_log_vt", filter))
  {
    extern void __test__i_log_vt (void);
    i_log_info ("========================= TEST CASE: %s\n", "i_log_vt");
    int prev = test_ret;
    test_ret = 0;
    __test__i_log_vt ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "i_log_vt");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_log_vt";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:3128 DONE

  //////////////////// /Users/theo/Development/Numstore/src/pager.c:219 START
  if (!filter || strstr ("pager_fill_ht", filter))
  {
    extern void __test__pager_fill_ht (void);
    i_log_info ("========================= TEST CASE: %s\n", "pager_fill_ht");
    int prev = test_ret;
    test_ret = 0;
    __test__pager_fill_ht ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "pager_fill_ht");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "pager_fill_ht";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/pager.c:219 DONE

  //////////////////// /Users/theo/Development/Numstore/src/pager.c:273 START
  if (!filter || strstr ("wal_int", filter))
  {
    extern void __test__wal_int (void);
    i_log_info ("========================= TEST CASE: %s\n", "wal_int");
    int prev = test_ret;
    test_ret = 0;
    __test__wal_int ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "wal_int");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "wal_int";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/pager.c:273 DONE

  //////////////////// /Users/theo/Development/Numstore/src/pager.c:1294 START
  if (!filter || strstr ("pager_open", filter))
  {
    extern void __test__pager_open (void);
    i_log_info ("========================= TEST CASE: %s\n", "pager_open");
    int prev = test_ret;
    test_ret = 0;
    __test__pager_open ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "pager_open");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "pager_open";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/pager.c:1294 DONE

  //////////////////// /Users/theo/Development/Numstore/src/pager.c:1336 START
  if (!filter || strstr ("pgr_open_basic", filter))
  {
    extern void __test__pgr_open_basic (void);
    i_log_info ("========================= TEST CASE: %s\n", "pgr_open_basic");
    int prev = test_ret;
    test_ret = 0;
    __test__pgr_open_basic ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "pgr_open_basic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "pgr_open_basic";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/pager.c:1336 DONE

  //////////////////// /Users/theo/Development/Numstore/src/pager.c:1430 START
  if (!filter || strstr ("pgr_close_success", filter))
  {
    extern void __test__pgr_close_success (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "pgr_close_success"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__pgr_close_success ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "pgr_close_success");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "pgr_close_success";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/pager.c:1430 DONE

  //////////////////// /Users/theo/Development/Numstore/src/pager.c:1537 START
  if (!filter || strstr ("pgr_delete", filter))
  {
    extern void __test__pgr_delete (void);
    i_log_info ("========================= TEST CASE: %s\n", "pgr_delete");
    int prev = test_ret;
    test_ret = 0;
    __test__pgr_delete ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "pgr_delete");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "pgr_delete";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/pager.c:1537 DONE

  //////////////////// /Users/theo/Development/Numstore/src/pager.c:1937 START
  if (!filter || strstr ("pgr_reserve_and_ctrl_lock_st", filter))
  {
    extern void __test__pgr_reserve_and_ctrl_lock_st (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "pgr_reserve_and_ctrl_lock_st"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__pgr_reserve_and_ctrl_lock_st ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "pgr_reserve_and_ctrl_lock_st");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "pgr_reserve_and_ctrl_lock_st";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/pager.c:1937 DONE

  //////////////////// /Users/theo/Development/Numstore/src/pager.c:2076 START
  if (!filter || strstr ("pgr_get_invalid_checksum", filter))
  {
    extern void __test__pgr_get_invalid_checksum (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "pgr_get_invalid_checksum"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__pgr_get_invalid_checksum ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "pgr_get_invalid_checksum");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "pgr_get_invalid_checksum";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/pager.c:2076 DONE

  //////////////////// /Users/theo/Development/Numstore/src/pager.c:2649 START
  if (!filter || strstr ("pgr_new_get_save", filter))
  {
    extern void __test__pgr_new_get_save (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "pgr_new_get_save"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__pgr_new_get_save ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "pgr_new_get_save");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "pgr_new_get_save";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/pager.c:2649 DONE

  //////////////////// /Users/theo/Development/Numstore/src/pager.c:2951 START
  if (!filter || strstr ("aries_rollback_basic", filter))
  {
    extern void __test__aries_rollback_basic (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "aries_rollback_basic"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__aries_rollback_basic ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "aries_rollback_basic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "aries_rollback_basic";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/pager.c:2951 DONE

  //////////////////// /Users/theo/Development/Numstore/src/pager.c:3024 START
  if (!filter || strstr ("aries_rollback_multiple_updates", filter))
  {
    extern void __test__aries_rollback_multiple_updates (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "aries_rollback_multiple_updates"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__aries_rollback_multiple_updates ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "aries_rollback_multiple_updates");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "aries_rollback_multiple_updates";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/pager.c:3024 DONE

  //////////////////// /Users/theo/Development/Numstore/src/pager.c:3103 START
  if (!filter || strstr ("aries_rollback_with_crash_recovery", filter))
  {
    extern void __test__aries_rollback_with_crash_recovery (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "aries_rollback_with_crash_recovery"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__aries_rollback_with_crash_recovery ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "aries_rollback_with_crash_recovery");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "aries_rollback_with_crash_recovery";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/pager.c:3103 DONE

  //////////////////// /Users/theo/Development/Numstore/src/pager.c:3164 START
  if (!filter || strstr ("aries_rollback_clr_not_undone", filter))
  {
    extern void __test__aries_rollback_clr_not_undone (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "aries_rollback_clr_not_undone"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__aries_rollback_clr_not_undone ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "aries_rollback_clr_not_undone");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "aries_rollback_clr_not_undone";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/pager.c:3164 DONE

  //////////////////// /Users/theo/Development/Numstore/src/parsers.c:206 START
  if (!filter || strstr ("compile_multi_user_stride", filter))
  {
    extern void __test__compile_multi_user_stride (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "compile_multi_user_stride"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__compile_multi_user_stride ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "compile_multi_user_stride");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "compile_multi_user_stride";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/parsers.c:206 DONE

  //////////////////// /Users/theo/Development/Numstore/src/parsers.c:1025 START
  if (!filter || strstr ("compile_query", filter))
  {
    extern void __test__compile_query (void);
    i_log_info ("========================= TEST CASE: %s\n", "compile_query");
    int prev = test_ret;
    test_ret = 0;
    __test__compile_query ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "compile_query");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "compile_query";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/parsers.c:1025 DONE

  //////////////////// /Users/theo/Development/Numstore/src/parsers.c:1684 START
  if (!filter || strstr ("compile_type", filter))
  {
    extern void __test__compile_type (void);
    i_log_info ("========================= TEST CASE: %s\n", "compile_type");
    int prev = test_ret;
    test_ret = 0;
    __test__compile_type ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "compile_type");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "compile_type";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/parsers.c:1684 DONE

  //////////////////// /Users/theo/Development/Numstore/src/parsers.c:2011 START
  if (!filter || strstr ("compile_type_ref", filter))
  {
    extern void __test__compile_type_ref (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "compile_type_ref"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__compile_type_ref ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "compile_type_ref");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "compile_type_ref";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/parsers.c:2011 DONE

  //////////////////// /Users/theo/Development/Numstore/src/parsers.c:2248 START
  if (!filter || strstr ("compile_user_stride", filter))
  {
    extern void __test__compile_user_stride (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "compile_user_stride"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__compile_user_stride ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "compile_user_stride");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "compile_user_stride";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/parsers.c:2248 DONE

  //////////////////// /Users/theo/Development/Numstore/src/rope_algorithms.c:90
  /// START
  if (!filter || strstr ("dlgt_balance_with_prev", filter))
  {
    extern void __test__dlgt_balance_with_prev (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "dlgt_balance_with_prev"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__dlgt_balance_with_prev ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dlgt_balance_with_prev");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dlgt_balance_with_prev";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/rope_algorithms.c:90
  /// DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/rope_algorithms.c:270 START
  if (!filter || strstr ("dlgt_balance_with_next", filter))
  {
    extern void __test__dlgt_balance_with_next (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "dlgt_balance_with_next"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__dlgt_balance_with_next ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dlgt_balance_with_next");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dlgt_balance_with_next";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/rope_algorithms.c:270 DONE

  //////////////////// /Users/theo/Development/Numstore/src/serial.c:85 START
  if (!filter || strstr ("strings_all_unique", filter))
  {
    extern void __test__strings_all_unique (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "strings_all_unique"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__strings_all_unique ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "strings_all_unique");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "strings_all_unique";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/serial.c:85 DONE

  //////////////////// /Users/theo/Development/Numstore/src/serial.c:237 START
  if (!filter || strstr ("string_contains", filter))
  {
    extern void __test__string_contains (void);
    i_log_info ("========================= TEST CASE: %s\n", "string_contains");
    int prev = test_ret;
    test_ret = 0;
    __test__string_contains ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "string_contains");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "string_contains";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/serial.c:237 DONE

  //////////////////// /Users/theo/Development/Numstore/src/stride.c:161 START
  if (!filter || strstr ("stride_resolve", filter))
  {
    extern void __test__stride_resolve (void);
    i_log_info ("========================= TEST CASE: %s\n", "stride_resolve");
    int prev = test_ret;
    test_ret = 0;
    __test__stride_resolve ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "stride_resolve");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "stride_resolve";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/stride.c:161 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/aries_tests.c:20 START
  if (!filter || strstr ("aries_crash", filter))
  {
    extern void __test__aries_crash (void);
    i_log_info ("========================= TEST CASE: %s\n", "aries_crash");
    int prev = test_ret;
    test_ret = 0;
    __test__aries_crash ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "aries_crash");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "aries_crash";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/aries_tests.c:20 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:54 START
  if (!filter || strstr ("f16_to_f32_normals_and_specials", filter))
  {
    extern void __test__f16_to_f32_normals_and_specials (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "f16_to_f32_normals_and_specials"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__f16_to_f32_normals_and_specials ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "f16_to_f32_normals_and_specials");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "f16_to_f32_normals_and_specials";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:54 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:73 START
  if (!filter || strstr ("f16_to_f32_nan_is_nan", filter))
  {
    extern void __test__f16_to_f32_nan_is_nan (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "f16_to_f32_nan_is_nan"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__f16_to_f32_nan_is_nan ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "f16_to_f32_nan_is_nan");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "f16_to_f32_nan_is_nan";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:73 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:80 START
  if (!filter || strstr ("f16_to_f32_smallest_subnormal_correct_value", filter))
  {
    extern void __test__f16_to_f32_smallest_subnormal_correct_value (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "f16_to_f32_smallest_subnormal_correct_value"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__f16_to_f32_smallest_subnormal_correct_value ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "f16_to_f32_smallest_subnormal_correct_value");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "f16_to_f32_smallest_subnormal_correct_value";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:80 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:112 START
  if (!filter || strstr ("parse_i32_boundary_values", filter))
  {
    extern void __test__parse_i32_boundary_values (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "parse_i32_boundary_values"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__parse_i32_boundary_values ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "parse_i32_boundary_values");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "parse_i32_boundary_values";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:112 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:166 START
  if (!filter || strstr ("parse_i64_boundary_values", filter))
  {
    extern void __test__parse_i64_boundary_values (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "parse_i64_boundary_values"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__parse_i64_boundary_values ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "parse_i64_boundary_values");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "parse_i64_boundary_values";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:166 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:201 START
  if (!filter || strstr ("ext_array_capacity_doubles_on_growth", filter))
  {
    extern void __test__ext_array_capacity_doubles_on_growth (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "ext_array_capacity_doubles_on_growth"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__ext_array_capacity_doubles_on_growth ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "ext_array_capacity_doubles_on_growth");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ext_array_capacity_doubles_on_growth";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:201 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:232 START
  if (!filter || strstr ("ext_array_remove_all_produces_empty", filter))
  {
    extern void __test__ext_array_remove_all_produces_empty (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "ext_array_remove_all_produces_empty"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__ext_array_remove_all_produces_empty ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "ext_array_remove_all_produces_empty");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ext_array_remove_all_produces_empty";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:232 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:271 START
  if (!filter || strstr ("llist_append_maintaififo_order", filter))
  {
    extern void __test__llist_append_maintaififo_order (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "llist_append_maintaififo_order"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__llist_append_maintaififo_order ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "llist_append_maintaififo_order");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "llist_append_maintaififo_order";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:271 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:295 START
  if (!filter || strstr ("llist_find_returnode_and_index", filter))
  {
    extern void __test__llist_find_returnode_and_index (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "llist_find_returnode_and_index"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__llist_find_returnode_and_index ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "llist_find_returnode_and_index");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "llist_find_returnode_and_index";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:295 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:329 START
  if (!filter || strstr ("llist_remove_from_head_middle_tail", filter))
  {
    extern void __test__llist_remove_from_head_middle_tail (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "llist_remove_from_head_middle_tail"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__llist_remove_from_head_middle_tail ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "llist_remove_from_head_middle_tail");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "llist_remove_from_head_middle_tail";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:329 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:366 START
  if (!filter || strstr ("llist_remove_absent_node_is_noop", filter))
  {
    extern void __test__llist_remove_absent_node_is_noop (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "llist_remove_absent_node_is_noop"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__llist_remove_absent_node_is_noop ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "llist_remove_absent_node_is_noop");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "llist_remove_absent_node_is_noop";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:366 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:388 START
  if (!filter || strstr ("checksum_known_crc32c_vector", filter))
  {
    extern void __test__checksum_known_crc32c_vector (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "checksum_known_crc32c_vector"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__checksum_known_crc32c_vector ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "checksum_known_crc32c_vector");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "checksum_known_crc32c_vector";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:388 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:397 START
  if (!filter || strstr ("checksum_distinct_bytes_differ", filter))
  {
    extern void __test__checksum_distinct_bytes_differ (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "checksum_distinct_bytes_differ"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__checksum_distinct_bytes_differ ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "checksum_distinct_bytes_differ");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "checksum_distinct_bytes_differ";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:397 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:410 START
  if (!filter || strstr ("serializer_write_at_capacity_then_overflow", filter))
  {
    extern void __test__serializer_write_at_capacity_then_overflow (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "serializer_write_at_capacity_then_overflow"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__serializer_write_at_capacity_then_overflow ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "serializer_write_at_capacity_then_overflow");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "serializer_write_at_capacity_then_overflow";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:410 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:429 START
  if (!filter || strstr ("serializer_incremental_write_overflow", filter))
  {
    extern void __test__serializer_incremental_write_overflow (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "serializer_incremental_write_overflow"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__serializer_incremental_write_overflow ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "serializer_incremental_write_overflow");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "serializer_incremental_write_overflow";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:429 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:448 START
  if (!filter || strstr ("stride_constructors_resolve_correctly", filter))
  {
    extern void __test__stride_constructors_resolve_correctly (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "stride_constructors_resolve_correctly"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__stride_constructors_resolve_correctly ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "stride_constructors_resolve_correctly");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "stride_constructors_resolve_correctly";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:448 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:523 START
  if (!filter || strstr ("string_ordering_operators", filter))
  {
    extern void __test__string_ordering_operators (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "string_ordering_operators"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__string_ordering_operators ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "string_ordering_operators");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "string_ordering_operators";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:523 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:564 START
  if (!filter || strstr ("line_length_newline_found", filter))
  {
    extern void __test__line_length_newline_found (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "line_length_newline_found"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__line_length_newline_found ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "line_length_newline_found");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "line_length_newline_found";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:564 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:621 START
  if (!filter || strstr ("string_equal_cases", filter))
  {
    extern void __test__string_equal_cases (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "string_equal_cases"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__string_equal_cases ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "string_equal_cases");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "string_equal_cases";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:621 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:659 START
  if (!filter || strstr ("strings_are_disjoint_cases", filter))
  {
    extern void __test__strings_are_disjoint_cases (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "strings_are_disjoint_cases"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__strings_are_disjoint_cases ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "strings_are_disjoint_cases");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "strings_are_disjoint_cases";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:659 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:728 START
  if (!filter || strstr ("string_plus_concatenates", filter))
  {
    extern void __test__string_plus_concatenates (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "string_plus_concatenates"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__string_plus_concatenates ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "string_plus_concatenates");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "string_plus_concatenates";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:728 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:779 START
  if (!filter || strstr ("cbuffer_discard_all_resets_state", filter))
  {
    extern void __test__cbuffer_discard_all_resets_state (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "cbuffer_discard_all_resets_state"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_discard_all_resets_state ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "cbuffer_discard_all_resets_state");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_discard_all_resets_state";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:779 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:816 START
  if (!filter || strstr ("cbuffer_read_write_wraparound", filter))
  {
    extern void __test__cbuffer_read_write_wraparound (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "cbuffer_read_write_wraparound"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_read_write_wraparound ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "cbuffer_read_write_wraparound");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_read_write_wraparound";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:816 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:875 START
  if (!filter || strstr ("cbuffer_cbuffer_move_transfers_bytes", filter))
  {
    extern void __test__cbuffer_cbuffer_move_transfers_bytes (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "cbuffer_cbuffer_move_transfers_bytes"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_cbuffer_move_transfers_bytes ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "cbuffer_cbuffer_move_transfers_bytes");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_cbuffer_move_transfers_bytes";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/core_extra_tests.c:875 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:19
  /// START
  if (!filter || strstr ("dpgt_open", filter))
  {
    extern void __test__dpgt_open (void);
    i_log_info ("========================= TEST CASE: %s\n", "dpgt_open");
    int prev = test_ret;
    test_ret = 0;
    __test__dpgt_open ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dpgt_open");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dpgt_open";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:19
  /// DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:39
  /// START
  if (!filter || strstr ("dpgt_merge_into", filter))
  {
    extern void __test__dpgt_merge_into (void);
    i_log_info ("========================= TEST CASE: %s\n", "dpgt_merge_into");
    int prev = test_ret;
    test_ret = 0;
    __test__dpgt_merge_into ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dpgt_merge_into");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dpgt_merge_into";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:39
  /// DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:104
  /// START
  if (!filter || strstr ("dpgt_min_rec_lsn", filter))
  {
    extern void __test__dpgt_min_rec_lsn (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "dpgt_min_rec_lsn"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__dpgt_min_rec_lsn ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dpgt_min_rec_lsn");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dpgt_min_rec_lsn";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:104
  /// DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:134
  /// START
  if (!filter || strstr ("dpgt_exists", filter))
  {
    extern void __test__dpgt_exists (void);
    i_log_info ("========================= TEST CASE: %s\n", "dpgt_exists");
    int prev = test_ret;
    test_ret = 0;
    __test__dpgt_exists ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dpgt_exists");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dpgt_exists";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:134
  /// DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:157
  /// START
  if (!filter || strstr ("dpgt_add", filter))
  {
    extern void __test__dpgt_add (void);
    i_log_info ("========================= TEST CASE: %s\n", "dpgt_add");
    int prev = test_ret;
    test_ret = 0;
    __test__dpgt_add ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dpgt_add");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dpgt_add";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:157
  /// DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:194
  /// START
  if (!filter || strstr ("dpgt_get", filter))
  {
    extern void __test__dpgt_get (void);
    i_log_info ("========================= TEST CASE: %s\n", "dpgt_get");
    int prev = test_ret;
    test_ret = 0;
    __test__dpgt_get ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dpgt_get");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dpgt_get";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:194
  /// DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:260
  /// START
  if (!filter || strstr ("dpgt_remove", filter))
  {
    extern void __test__dpgt_remove (void);
    i_log_info ("========================= TEST CASE: %s\n", "dpgt_remove");
    int prev = test_ret;
    test_ret = 0;
    __test__dpgt_remove ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dpgt_remove");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dpgt_remove";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:260
  /// DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:324
  /// START
  if (!filter || strstr ("dpgt_serialize", filter))
  {
    extern void __test__dpgt_serialize (void);
    i_log_info ("========================= TEST CASE: %s\n", "dpgt_serialize");
    int prev = test_ret;
    test_ret = 0;
    __test__dpgt_serialize ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dpgt_serialize");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dpgt_serialize";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:324
  /// DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:388
  /// START
  if (!filter || strstr ("dpgt_equal", filter))
  {
    extern void __test__dpgt_equal (void);
    i_log_info ("========================= TEST CASE: %s\n", "dpgt_equal");
    int prev = test_ret;
    test_ret = 0;
    __test__dpgt_equal ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dpgt_equal");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dpgt_equal";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:388
  /// DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/dpgt_concurrency_tests.c:100
  /// START
  if (!filter || strstr ("dpgt_concurrent", filter))
  {
    extern void __test__dpgt_concurrent (void);
    i_log_info ("========================= TEST CASE: %s\n", "dpgt_concurrent");
    int prev = test_ret;
    test_ret = 0;
    __test__dpgt_concurrent ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "dpgt_concurrent");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dpgt_concurrent";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/dpgt_concurrency_tests.c:100
  /// DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/mem_vhmap.c:254 START
  if (!filter || strstr ("mem_vhmap", filter))
  {
    extern void __test__mem_vhmap (void);
    i_log_info ("========================= TEST CASE: %s\n", "mem_vhmap");
    int prev = test_ret;
    test_ret = 0;
    __test__mem_vhmap ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "mem_vhmap");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "mem_vhmap";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/mem_vhmap.c:254 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/numstore_regression_tests.c:20
  /// START
  if (!filter
      || strstr ("regression_cgd_test_create_delete_rollback_delete", filter))
  {
    extern void __test__regression_cgd_test_create_delete_rollback_delete (
        void
    );
    i_log_info (
        "========================= TEST CASE: %s\n",
        "regression_cgd_test_create_delete_rollback_delete"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__regression_cgd_test_create_delete_rollback_delete ();
    if (!test_ret)
    {
      i_log_passed (
          "%s\n",
          "regression_cgd_test_create_delete_rollback_delete"
      );
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] =
          "regression_cgd_test_create_delete_rollback_delete";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/numstore_regression_tests.c:20
  /// DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/numstore_regression_tests.c:66
  /// START
  if (!filter
      || strstr ("regression_cgd_test_create_crash_close_delete", filter))
  {
    extern void __test__regression_cgd_test_create_crash_close_delete (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "regression_cgd_test_create_crash_close_delete"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__regression_cgd_test_create_crash_close_delete ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "regression_cgd_test_create_crash_close_delete");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "regression_cgd_test_create_crash_close_delete";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/numstore_regression_tests.c:66
  /// DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/numstore_regression_tests.c:99
  /// START
  if (!filter || strstr ("regression_irwr_rollback_invalid_wal_header", filter))
  {
    extern void __test__regression_irwr_rollback_invalid_wal_header (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "regression_irwr_rollback_invalid_wal_header"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__regression_irwr_rollback_invalid_wal_header ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "regression_irwr_rollback_invalid_wal_header");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "regression_irwr_rollback_invalid_wal_header";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/numstore_regression_tests.c:99
  /// DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/numstore_tests.c:24 START
  if (!filter || strstr ("nsdb_create_txn", filter))
  {
    extern void __test__nsdb_create_txn (void);
    i_log_info ("========================= TEST CASE: %s\n", "nsdb_create_txn");
    int prev = test_ret;
    test_ret = 0;
    __test__nsdb_create_txn ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "nsdb_create_txn");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nsdb_create_txn";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/numstore_tests.c:24 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/numstore_tests.c:207 START
  if (!filter || strstr ("nsdb_delete_txn", filter))
  {
    extern void __test__nsdb_delete_txn (void);
    i_log_info ("========================= TEST CASE: %s\n", "nsdb_delete_txn");
    int prev = test_ret;
    test_ret = 0;
    __test__nsdb_delete_txn ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "nsdb_delete_txn");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nsdb_delete_txn";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/numstore_tests.c:207 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/numstore_tests.c:322 START
  if (!filter || strstr ("nsdb_insert_txn", filter))
  {
    extern void __test__nsdb_insert_txn (void);
    i_log_info ("========================= TEST CASE: %s\n", "nsdb_insert_txn");
    int prev = test_ret;
    test_ret = 0;
    __test__nsdb_insert_txn ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "nsdb_insert_txn");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nsdb_insert_txn";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/numstore_tests.c:322 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/numstore_tests.c:584 START
  if (!filter || strstr ("nsdb_write_txn", filter))
  {
    extern void __test__nsdb_write_txn (void);
    i_log_info ("========================= TEST CASE: %s\n", "nsdb_write_txn");
    int prev = test_ret;
    test_ret = 0;
    __test__nsdb_write_txn ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "nsdb_write_txn");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nsdb_write_txn";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/numstore_tests.c:584 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/page_fixture.c:307 START
  if (!filter || strstr ("build_page_tree", filter))
  {
    extern void __test__build_page_tree (void);
    i_log_info ("========================= TEST CASE: %s\n", "build_page_tree");
    int prev = test_ret;
    test_ret = 0;
    __test__build_page_tree ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "build_page_tree");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "build_page_tree";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/page_fixture.c:307 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/robin_hood_ht_tests.c:27 START
  if (!filter || strstr ("ht_insert_idx_regression_trigger_swap", filter))
  {
    extern void __test__ht_insert_idx_regression_trigger_swap (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "ht_insert_idx_regression_trigger_swap"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__ht_insert_idx_regression_trigger_swap ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "ht_insert_idx_regression_trigger_swap");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ht_insert_idx_regression_trigger_swap";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/robin_hood_ht_tests.c:27 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/robin_hood_ht_tests.c:105
  /// START
  if (!filter || strstr ("robin_hood_ht", filter))
  {
    extern void __test__robin_hood_ht (void);
    i_log_info ("========================= TEST CASE: %s\n", "robin_hood_ht");
    int prev = test_ret;
    test_ret = 0;
    __test__robin_hood_ht ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "robin_hood_ht");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "robin_hood_ht";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/robin_hood_ht_tests.c:105 DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/smfile_data_writer.c:125 START
  if (!filter || strstr ("smfile_data_writer", filter))
  {
    extern void __test__smfile_data_writer (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "smfile_data_writer"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__smfile_data_writer ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "smfile_data_writer");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "smfile_data_writer";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/smfile_data_writer.c:125 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/testing.c:72
  /// START
  if (!filter || strstr ("test_mark_works", filter))
  {
    extern void __test__test_mark_works (void);
    i_log_info ("========================= TEST CASE: %s\n", "test_mark_works");
    int prev = test_ret;
    test_ret = 0;
    __test__test_mark_works ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "test_mark_works");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "test_mark_works";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/testing.c:72
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/testing.c:83
  /// START
  if (!filter || strstr ("test_mark_match", filter))
  {
    extern void __test__test_mark_match (void);
    i_log_info ("========================= TEST CASE: %s\n", "test_mark_match");
    int prev = test_ret;
    test_ret = 0;
    __test__test_mark_match ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "test_mark_match");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "test_mark_match";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/testing.c:83
  /// DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/txnt_concurrency_tests.c:119
  /// START
  if (!filter || strstr ("txnt_concurrent", filter))
  {
    extern void __test__txnt_concurrent (void);
    i_log_info ("========================= TEST CASE: %s\n", "txnt_concurrent");
    int prev = test_ret;
    test_ret = 0;
    __test__txnt_concurrent ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "txnt_concurrent");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_concurrent";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/testing/txnt_concurrency_tests.c:119
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:328
  /// START
  if (!filter || strstr ("txn_basic", filter))
  {
    extern void __test__txn_basic (void);
    i_log_info ("========================= TEST CASE: %s\n", "txn_basic");
    int prev = test_ret;
    test_ret = 0;
    __test__txn_basic ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "txn_basic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txn_basic";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:328 DONE

  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:435
  /// START
  if (!filter || strstr ("txnt_open", filter))
  {
    extern void __test__txnt_open (void);
    i_log_info ("========================= TEST CASE: %s\n", "txnt_open");
    int prev = test_ret;
    test_ret = 0;
    __test__txnt_open ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "txnt_open");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_open";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:435 DONE

  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:581
  /// START
  if (!filter || strstr ("txnt_merge_into", filter))
  {
    extern void __test__txnt_merge_into (void);
    i_log_info ("========================= TEST CASE: %s\n", "txnt_merge_into");
    int prev = test_ret;
    test_ret = 0;
    __test__txnt_merge_into ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "txnt_merge_into");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_merge_into";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:581 DONE

  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:722
  /// START
  if (!filter || strstr ("txnt_max_u_undo_lsn", filter))
  {
    extern void __test__txnt_max_u_undo_lsn (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "txnt_max_u_undo_lsn"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__txnt_max_u_undo_lsn ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "txnt_max_u_undo_lsn");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_max_u_undo_lsn";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:722 DONE

  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:850
  /// START
  if (!filter || strstr ("txnt_min_lsn", filter))
  {
    extern void __test__txnt_min_lsn (void);
    i_log_info ("========================= TEST CASE: %s\n", "txnt_min_lsn");
    int prev = test_ret;
    test_ret = 0;
    __test__txnt_min_lsn ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "txnt_min_lsn");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_min_lsn";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:850 DONE

  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:958
  /// START
  if (!filter || strstr ("txnt_exists", filter))
  {
    extern void __test__txnt_exists (void);
    i_log_info ("========================= TEST CASE: %s\n", "txnt_exists");
    int prev = test_ret;
    test_ret = 0;
    __test__txnt_exists ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "txnt_exists");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_exists";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:958 DONE

  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:1015
  /// START
  if (!filter || strstr ("txnt_insert", filter))
  {
    extern void __test__txnt_insert (void);
    i_log_info ("========================= TEST CASE: %s\n", "txnt_insert");
    int prev = test_ret;
    test_ret = 0;
    __test__txnt_insert ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "txnt_insert");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_insert";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:1015
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:1168
  /// START
  if (!filter || strstr ("txnt_get", filter))
  {
    extern void __test__txnt_get (void);
    i_log_info ("========================= TEST CASE: %s\n", "txnt_get");
    int prev = test_ret;
    test_ret = 0;
    __test__txnt_get ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "txnt_get");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_get";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:1168
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:1352
  /// START
  if (!filter || strstr ("txnt_remove", filter))
  {
    extern void __test__txnt_remove (void);
    i_log_info ("========================= TEST CASE: %s\n", "txnt_remove");
    int prev = test_ret;
    test_ret = 0;
    __test__txnt_remove ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "txnt_remove");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_remove";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:1352
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:1572
  /// START
  if (!filter || strstr ("txnt_serialize", filter))
  {
    extern void __test__txnt_serialize (void);
    i_log_info ("========================= TEST CASE: %s\n", "txnt_serialize");
    int prev = test_ret;
    test_ret = 0;
    __test__txnt_serialize ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "txnt_serialize");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_serialize";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:1572
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:1706
  /// START
  if (!filter || strstr ("txnt_equal_ignore_state", filter))
  {
    extern void __test__txnt_equal_ignore_state (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "txnt_equal_ignore_state"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__txnt_equal_ignore_state ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "txnt_equal_ignore_state");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_equal_ignore_state";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:1706
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:295 START
  if (!filter || strstr ("type_generate_string", filter))
  {
    extern void __test__type_generate_string (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "type_generate_string"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__type_generate_string ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "type_generate_string");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "type_generate_string";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:295 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:921 START
  if (!filter || strstr ("type_malloc_copy", filter))
  {
    extern void __test__type_malloc_copy (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "type_malloc_copy"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__type_malloc_copy ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "type_malloc_copy");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "type_malloc_copy";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:921 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:1072 START
  if (!filter || strstr ("prim_t_validate", filter))
  {
    extern void __test__prim_t_validate (void);
    i_log_info ("========================= TEST CASE: %s\n", "prim_t_validate");
    int prev = test_ret;
    test_ret = 0;
    __test__prim_t_validate ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "prim_t_validate");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "prim_t_validate";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:1072 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:1154 START
  if (!filter || strstr ("prim_t_snprintf", filter))
  {
    extern void __test__prim_t_snprintf (void);
    i_log_info ("========================= TEST CASE: %s\n", "prim_t_snprintf");
    int prev = test_ret;
    test_ret = 0;
    __test__prim_t_snprintf ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "prim_t_snprintf");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "prim_t_snprintf";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:1154 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:1224 START
  if (!filter || strstr ("prim_t_byte_size", filter))
  {
    extern void __test__prim_t_byte_size (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "prim_t_byte_size"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__prim_t_byte_size ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "prim_t_byte_size");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "prim_t_byte_size";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:1224 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:1236 START
  if (!filter || strstr ("prim_t_serialize", filter))
  {
    extern void __test__prim_t_serialize (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "prim_t_serialize"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__prim_t_serialize ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "prim_t_serialize");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "prim_t_serialize";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:1236 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:1274 START
  if (!filter || strstr ("prim_t_deserialize", filter))
  {
    extern void __test__prim_t_deserialize (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "prim_t_deserialize"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__prim_t_deserialize ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "prim_t_deserialize");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "prim_t_deserialize";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:1274 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:1300 START
  if (!filter || strstr ("prim_t_random", filter))
  {
    extern void __test__prim_t_random (void);
    i_log_info ("========================= TEST CASE: %s\n", "prim_t_random");
    int prev = test_ret;
    test_ret = 0;
    __test__prim_t_random ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "prim_t_random");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "prim_t_random";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:1300 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:1644 START
  if (!filter || strstr ("struct_t_snprintf", filter))
  {
    extern void __test__struct_t_snprintf (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "struct_t_snprintf"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__struct_t_snprintf ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "struct_t_snprintf");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "struct_t_snprintf";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:1644 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:1716 START
  if (!filter || strstr ("struct_t_byte_size", filter))
  {
    extern void __test__struct_t_byte_size (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "struct_t_byte_size"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__struct_t_byte_size ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "struct_t_byte_size");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "struct_t_byte_size";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:1716 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:1788 START
  if (!filter || strstr ("struct_t_get_serial_size", filter))
  {
    extern void __test__struct_t_get_serial_size (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "struct_t_get_serial_size"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__struct_t_get_serial_size ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "struct_t_get_serial_size");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "struct_t_get_serial_size";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:1788 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:1863 START
  if (!filter || strstr ("struct_t_serialize", filter))
  {
    extern void __test__struct_t_serialize (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "struct_t_serialize"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__struct_t_serialize ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "struct_t_serialize");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "struct_t_serialize";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:1863 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:2013 START
  if (!filter || strstr ("struct_t_deserialize_green_path", filter))
  {
    extern void __test__struct_t_deserialize_green_path (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "struct_t_deserialize_green_path"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__struct_t_deserialize_green_path ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "struct_t_deserialize_green_path");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "struct_t_deserialize_green_path";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:2013 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:2069 START
  if (!filter || strstr ("struct_t_deserialize_red_path", filter))
  {
    extern void __test__struct_t_deserialize_red_path (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "struct_t_deserialize_red_path"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__struct_t_deserialize_red_path ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "struct_t_deserialize_red_path");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "struct_t_deserialize_red_path";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:2069 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:2446 START
  if (!filter || strstr ("union_t_snprintf", filter))
  {
    extern void __test__union_t_snprintf (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "union_t_snprintf"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__union_t_snprintf ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "union_t_snprintf");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "union_t_snprintf";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:2446 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:2520 START
  if (!filter || strstr ("union_t_byte_size", filter))
  {
    extern void __test__union_t_byte_size (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "union_t_byte_size"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__union_t_byte_size ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "union_t_byte_size");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "union_t_byte_size";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:2520 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:2592 START
  if (!filter || strstr ("union_t_get_serial_size", filter))
  {
    extern void __test__union_t_get_serial_size (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "union_t_get_serial_size"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__union_t_get_serial_size ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "union_t_get_serial_size");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "union_t_get_serial_size";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:2592 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:2667 START
  if (!filter || strstr ("union_t_serialize", filter))
  {
    extern void __test__union_t_serialize (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "union_t_serialize"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__union_t_serialize ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "union_t_serialize");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "union_t_serialize";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:2667 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:2813 START
  if (!filter || strstr ("union_t_deserialize_green_path", filter))
  {
    extern void __test__union_t_deserialize_green_path (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "union_t_deserialize_green_path"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__union_t_deserialize_green_path ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "union_t_deserialize_green_path");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "union_t_deserialize_green_path";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:2813 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:2869 START
  if (!filter || strstr ("union_t_deserialize_red_path", filter))
  {
    extern void __test__union_t_deserialize_red_path (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "union_t_deserialize_red_path"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__union_t_deserialize_red_path ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "union_t_deserialize_red_path");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "union_t_deserialize_red_path";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:2869 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:3077 START
  if (!filter || strstr ("sarray_t_snprintf", filter))
  {
    extern void __test__sarray_t_snprintf (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "sarray_t_snprintf"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__sarray_t_snprintf ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "sarray_t_snprintf");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "sarray_t_snprintf";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:3077 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:3117 START
  if (!filter || strstr ("sarray_t_byte_size", filter))
  {
    extern void __test__sarray_t_byte_size (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "sarray_t_byte_size"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__sarray_t_byte_size ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "sarray_t_byte_size");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "sarray_t_byte_size";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:3117 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:3152 START
  if (!filter || strstr ("sarray_t_get_serial_size", filter))
  {
    extern void __test__sarray_t_get_serial_size (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "sarray_t_get_serial_size"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__sarray_t_get_serial_size ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "sarray_t_get_serial_size");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "sarray_t_get_serial_size";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:3152 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:3188 START
  if (!filter || strstr ("sarray_t_serialize", filter))
  {
    extern void __test__sarray_t_serialize (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "sarray_t_serialize"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__sarray_t_serialize ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "sarray_t_serialize");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "sarray_t_serialize";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:3188 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:3281 START
  if (!filter || strstr ("sarray_t_deserialize_green_path", filter))
  {
    extern void __test__sarray_t_deserialize_green_path (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "sarray_t_deserialize_green_path"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__sarray_t_deserialize_green_path ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "sarray_t_deserialize_green_path");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "sarray_t_deserialize_green_path";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:3281 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:3316 START
  if (!filter || strstr ("sarray_t_deserialize_red_path", filter))
  {
    extern void __test__sarray_t_deserialize_red_path (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "sarray_t_deserialize_red_path"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__sarray_t_deserialize_red_path ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "sarray_t_deserialize_red_path");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "sarray_t_deserialize_red_path";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:3316 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:3522 START
  if (!filter || strstr ("sarray_builder", filter))
  {
    extern void __test__sarray_builder (void);
    i_log_info ("========================= TEST CASE: %s\n", "sarray_builder");
    int prev = test_ret;
    test_ret = 0;
    __test__sarray_builder ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "sarray_builder");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "sarray_builder";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:3522 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:3757 START
  if (!filter || strstr ("kvt_list_builder", filter))
  {
    extern void __test__kvt_list_builder (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "kvt_list_builder"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__kvt_list_builder ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "kvt_list_builder");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "kvt_list_builder";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:3757 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:4346 START
  if (!filter || strstr ("ta_subtype", filter))
  {
    extern void __test__ta_subtype (void);
    i_log_info ("========================= TEST CASE: %s\n", "ta_subtype");
    int prev = test_ret;
    test_ret = 0;
    __test__ta_subtype ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "ta_subtype");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ta_subtype";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:4346 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:4717 START
  if (!filter || strstr ("type_accessor_builder", filter))
  {
    extern void __test__type_accessor_builder (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "type_accessor_builder"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__type_accessor_builder ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "type_accessor_builder");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "type_accessor_builder";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:4717 DONE

  //////////////////// /Users/theo/Development/Numstore/src/utils.c:33 START
  if (!filter || strstr ("file_basename", filter))
  {
    extern void __test__file_basename (void);
    i_log_info ("========================= TEST CASE: %s\n", "file_basename");
    int prev = test_ret;
    test_ret = 0;
    __test__file_basename ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "file_basename");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "file_basename";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/utils.c:33 DONE

  //////////////////// /Users/theo/Development/Numstore/src/var_algorithms.c:79
  /// START
  if (!filter || strstr ("ns_init_var_hash_map", filter))
  {
    extern void __test__ns_init_var_hash_map (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "ns_init_var_hash_map"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__ns_init_var_hash_map ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "ns_init_var_hash_map");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ns_init_var_hash_map";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/var_algorithms.c:79
  /// DONE

  //////////////////// /Users/theo/Development/Numstore/src/var_algorithms.c:471
  /// START
  if (!filter || strstr ("ns_find_var_page", filter))
  {
    extern void __test__ns_find_var_page (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "ns_find_var_page"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__ns_find_var_page ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "ns_find_var_page");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ns_find_var_page";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/var_algorithms.c:471
  /// DONE

  ////////////////////
  ////Users/theo/Development/Numstore/src/var_algorithms.c:1643 START
  if (!filter || strstr ("ns_var_get_or_create", filter))
  {
    extern void __test__ns_var_get_or_create (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "ns_var_get_or_create"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__ns_var_get_or_create ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "ns_var_get_or_create");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ns_var_get_or_create";
    }
    ntests++;
  }
  ////////////////////
  ////Users/theo/Development/Numstore/src/var_algorithms.c:1643 DONE

  //////////////////// /Users/theo/Development/Numstore/src/variables.c:277
  /// START
  if (!filter || strstr ("rand_varname_same_hash", filter))
  {
    extern void __test__rand_varname_same_hash (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "rand_varname_same_hash"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__rand_varname_same_hash ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "rand_varname_same_hash");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "rand_varname_same_hash";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/variables.c:277 DONE

  //////////////////// /Users/theo/Development/Numstore/src/variables.c:294
  /// START
  if (!filter || strstr ("rand_varname_different_hash", filter))
  {
    extern void __test__rand_varname_different_hash (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "rand_varname_different_hash"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__rand_varname_different_hash ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "rand_varname_different_hash");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "rand_varname_different_hash";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/variables.c:294 DONE

  //////////////////// /Users/theo/Development/Numstore/src/wal.c:72 START
  if (!filter || strstr ("walos_open", filter))
  {
    extern void __test__walos_open (void);
    i_log_info ("========================= TEST CASE: %s\n", "walos_open");
    int prev = test_ret;
    test_ret = 0;
    __test__walos_open ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "walos_open");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "walos_open";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/wal.c:72 DONE

  //////////////////// /Users/theo/Development/Numstore/src/wal.c:325 START
  if (!filter || strstr ("walis_open", filter))
  {
    extern void __test__walis_open (void);
    i_log_info ("========================= TEST CASE: %s\n", "walis_open");
    int prev = test_ret;
    test_ret = 0;
    __test__walis_open ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "walis_open");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "walis_open";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/wal.c:325 DONE

  //////////////////// /Users/theo/Development/Numstore/src/wal.c:1462 START
  if (!filter || strstr ("wal_rec_hdr_type_tostr", filter))
  {
    extern void __test__wal_rec_hdr_type_tostr (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "wal_rec_hdr_type_tostr"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__wal_rec_hdr_type_tostr ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "wal_rec_hdr_type_tostr");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "wal_rec_hdr_type_tostr";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/wal.c:1462 DONE

  //////////////////// /Users/theo/Development/Numstore/src/wal.c:2991 START
  if (!filter || strstr ("wal_multi_threaded", filter))
  {
    extern void __test__wal_multi_threaded (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "wal_multi_threaded"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__wal_multi_threaded ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "wal_multi_threaded");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "wal_multi_threaded";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/wal.c:2991 DONE

  //////////////////// /Users/theo/Development/Numstore/src/wal.c:3204 START
  if (!filter || strstr ("wal", filter))
  {
    extern void __test__wal (void);
    i_log_info ("========================= TEST CASE: %s\n", "wal");
    int prev = test_ret;
    test_ret = 0;
    __test__wal ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "wal");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "wal";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/wal.c:3204 DONE

  //////////////////// /Users/theo/Development/Numstore/src/wal.c:3322 START
  if (!filter || strstr ("wal_single_entry", filter))
  {
    extern void __test__wal_single_entry (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "wal_single_entry"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__wal_single_entry ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "wal_single_entry");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "wal_single_entry";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/wal.c:3322 DONE

  printf ("Time: %llu ms\n", (unsigned long long)i_timer_now_ms (&timer));
  i_timer_free (&timer);

  if (failed)
  {
    i_log_failure ("FAILED TESTS (%d):\n", failed);
    for (int i = 0; i < failed; i++)
    {
      i_log_failure ("  %s\n", failed_names[i]);
    }
  }
  else
  {
    i_log_passed ("ALL %d TESTS PASSED\n", ntests);
  }

  return test_ret;
}
