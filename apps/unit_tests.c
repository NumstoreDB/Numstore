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

int
main (const int argc, char **argv)
{
  const char *filter = (argc > 1) ? argv[1] : NULL;
  int         ntests = 0;

  error   e = error_create ();
  i_timer timer;
  if (i_timer_create (&timer, &e) != SUCCESS)
  {
    return -1;
  }

  int         failed = 0;
  const char *failed_names[286];

  //////////////////// /home/theo/Development/numstore/src/alloc.c:139 START
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
  //////////////////// /home/theo/Development/numstore/src/alloc.c:139 DONE

  //////////////////// /home/theo/Development/numstore/src/alloc.c:261 START
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
  //////////////////// /home/theo/Development/numstore/src/alloc.c:261 DONE

  //////////////////// /home/theo/Development/numstore/src/alloc.c:333 START
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
  //////////////////// /home/theo/Development/numstore/src/alloc.c:333 DONE

  //////////////////// /home/theo/Development/numstore/src/alloc.c:435 START
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
  //////////////////// /home/theo/Development/numstore/src/alloc.c:435 DONE

  //////////////////// /home/theo/Development/numstore/src/alloc.c:784 START
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
  //////////////////// /home/theo/Development/numstore/src/alloc.c:784 DONE

  //////////////////// /home/theo/Development/numstore/src/alloc.c:900 START
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
  //////////////////// /home/theo/Development/numstore/src/alloc.c:900 DONE

  //////////////////// /home/theo/Development/numstore/src/alloc.c:936 START
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
  //////////////////// /home/theo/Development/numstore/src/alloc.c:936 DONE

  //////////////////// /home/theo/Development/numstore/src/alloc.c:965 START
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
  //////////////////// /home/theo/Development/numstore/src/alloc.c:965 DONE

  //////////////////// /home/theo/Development/numstore/src/alloc.c:1008 START
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
  //////////////////// /home/theo/Development/numstore/src/alloc.c:1008 DONE

  //////////////////// /home/theo/Development/numstore/src/alloc.c:1060 START
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
  //////////////////// /home/theo/Development/numstore/src/alloc.c:1060 DONE

  //////////////////// /home/theo/Development/numstore/src/alloc.c:1104 START
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
  //////////////////// /home/theo/Development/numstore/src/alloc.c:1104 DONE

  //////////////////// /home/theo/Development/numstore/src/alloc.c:1150 START
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
  //////////////////// /home/theo/Development/numstore/src/alloc.c:1150 DONE

  //////////////////// /home/theo/Development/numstore/src/alloc.c:1179 START
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
  //////////////////// /home/theo/Development/numstore/src/alloc.c:1179 DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:28
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:28 DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:96
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:96 DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:106
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:106
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:116
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:116
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:166
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:166
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:239
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:239
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:306
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:306
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:393
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:393
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:514
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:514
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:586
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:586
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:675
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:675
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:769
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:769
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:878
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:878
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:1158
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:1158
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:1260
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:1260
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:1291
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:1291
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:1347
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:1347
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:1421
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:1421
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:1482
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:1482
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:1553
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:1553
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:1624
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:1624
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:1760
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:1760
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:1776
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:1776
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:1797
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:1797
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:1819
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:1819
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:1856
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:1856
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:1881
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:1881
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:1912
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:1912
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:1932
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:1932
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:1952
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:1952
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:1980
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:1980
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:1996
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:1996
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:2276
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:2276
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:2415
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:2415
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:2521
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:2521
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:2688
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:2688
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:3420
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:3420
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:3680
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:3680
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:3825
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:3825
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:3988
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:3988
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:4077
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:4077
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/collections.c:4407
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
  //////////////////// /home/theo/Development/numstore/src/collections.c:4407
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/compiler.c:687 START
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
  //////////////////// /home/theo/Development/numstore/src/compiler.c:687 DONE

  //////////////////// /home/theo/Development/numstore/src/compiler.c:713 START
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
  //////////////////// /home/theo/Development/numstore/src/compiler.c:713 DONE

  //////////////////// /home/theo/Development/numstore/src/compiler.c:741 START
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
  //////////////////// /home/theo/Development/numstore/src/compiler.c:741 DONE

  //////////////////// /home/theo/Development/numstore/src/compiler.c:754 START
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
  //////////////////// /home/theo/Development/numstore/src/compiler.c:754 DONE

  //////////////////// /home/theo/Development/numstore/src/compiler.c:768 START
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
  //////////////////// /home/theo/Development/numstore/src/compiler.c:768 DONE

  //////////////////// /home/theo/Development/numstore/src/compiler.c:784 START
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
  //////////////////// /home/theo/Development/numstore/src/compiler.c:784 DONE

  //////////////////// /home/theo/Development/numstore/src/compiler.c:803 START
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
  //////////////////// /home/theo/Development/numstore/src/compiler.c:803 DONE

  //////////////////// /home/theo/Development/numstore/src/compiler.c:823 START
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
  //////////////////// /home/theo/Development/numstore/src/compiler.c:823 DONE

  //////////////////// /home/theo/Development/numstore/src/compiler.c:841 START
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
  //////////////////// /home/theo/Development/numstore/src/compiler.c:841 DONE

  //////////////////// /home/theo/Development/numstore/src/compiler.c:863 START
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
  //////////////////// /home/theo/Development/numstore/src/compiler.c:863 DONE

  //////////////////// /home/theo/Development/numstore/src/compiler.c:879 START
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
  //////////////////// /home/theo/Development/numstore/src/compiler.c:879 DONE

  //////////////////// /home/theo/Development/numstore/src/compiler.c:891 START
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
  //////////////////// /home/theo/Development/numstore/src/compiler.c:891 DONE

  //////////////////// /home/theo/Development/numstore/src/compiler.c:902 START
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
  //////////////////// /home/theo/Development/numstore/src/compiler.c:902 DONE

  //////////////////// /home/theo/Development/numstore/src/compiler.c:1009 START
  if (!filter || strstr ("compile_user_stride_basic", filter))
  {
    extern void __test__compile_user_stride_basic (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "compile_user_stride_basic"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__compile_user_stride_basic ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "compile_user_stride_basic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "compile_user_stride_basic";
    }
    ntests++;
  }
  //////////////////// /home/theo/Development/numstore/src/compiler.c:1009 DONE

  //////////////////// /home/theo/Development/numstore/src/compiler.c:1043 START
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
  //////////////////// /home/theo/Development/numstore/src/compiler.c:1043 DONE

  //////////////////// /home/theo/Development/numstore/src/compiler.c:1370 START
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
  //////////////////// /home/theo/Development/numstore/src/compiler.c:1370 DONE

  //////////////////// /home/theo/Development/numstore/src/compiler.c:1613 START
  if (!filter || strstr ("compile_type_primitives", filter))
  {
    extern void __test__compile_type_primitives (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "compile_type_primitives"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__compile_type_primitives ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "compile_type_primitives");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "compile_type_primitives";
    }
    ntests++;
  }
  //////////////////// /home/theo/Development/numstore/src/compiler.c:1613 DONE

  //////////////////// /home/theo/Development/numstore/src/compiler.c:1682 START
  if (!filter || strstr ("compile_type_sarray", filter))
  {
    extern void __test__compile_type_sarray (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "compile_type_sarray"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__compile_type_sarray ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "compile_type_sarray");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "compile_type_sarray";
    }
    ntests++;
  }
  //////////////////// /home/theo/Development/numstore/src/compiler.c:1682 DONE

  //////////////////// /home/theo/Development/numstore/src/compiler.c:1725 START
  if (!filter || strstr ("compile_type_struct", filter))
  {
    extern void __test__compile_type_struct (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "compile_type_struct"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__compile_type_struct ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "compile_type_struct");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "compile_type_struct";
    }
    ntests++;
  }
  //////////////////// /home/theo/Development/numstore/src/compiler.c:1725 DONE

  //////////////////// /home/theo/Development/numstore/src/compiler.c:1761 START
  if (!filter || strstr ("compile_type_union", filter))
  {
    extern void __test__compile_type_union (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "compile_type_union"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__compile_type_union ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "compile_type_union");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "compile_type_union";
    }
    ntests++;
  }
  //////////////////// /home/theo/Development/numstore/src/compiler.c:1761 DONE

  //////////////////// /home/theo/Development/numstore/src/compiler.c:1785 START
  if (!filter || strstr ("compile_type_complex", filter))
  {
    extern void __test__compile_type_complex (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "compile_type_complex"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__compile_type_complex ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "compile_type_complex");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "compile_type_complex";
    }
    ntests++;
  }
  //////////////////// /home/theo/Development/numstore/src/compiler.c:1785 DONE

  //////////////////// /home/theo/Development/numstore/src/concurrency.c:55
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
  //////////////////// /home/theo/Development/numstore/src/concurrency.c:55 DONE

  //////////////////// /home/theo/Development/numstore/src/concurrency.c:100
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
  //////////////////// /home/theo/Development/numstore/src/concurrency.c:100
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/concurrency.c:146
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
  //////////////////// /home/theo/Development/numstore/src/concurrency.c:146
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/concurrency.c:379
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
  //////////////////// /home/theo/Development/numstore/src/concurrency.c:379
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/concurrency.c:477
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
  //////////////////// /home/theo/Development/numstore/src/concurrency.c:477
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/concurrency.c:636
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
  //////////////////// /home/theo/Development/numstore/src/concurrency.c:636
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/concurrency.c:653
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
  //////////////////// /home/theo/Development/numstore/src/concurrency.c:653
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/concurrency.c:709
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
  //////////////////// /home/theo/Development/numstore/src/concurrency.c:709
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/concurrency.c:899
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
  //////////////////// /home/theo/Development/numstore/src/concurrency.c:899
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/file_pager.c:124
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
  //////////////////// /home/theo/Development/numstore/src/file_pager.c:124 DONE

  //////////////////// /home/theo/Development/numstore/src/file_pager.c:233
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
  //////////////////// /home/theo/Development/numstore/src/file_pager.c:233 DONE

  //////////////////// /home/theo/Development/numstore/src/file_pager.c:412
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
  //////////////////// /home/theo/Development/numstore/src/file_pager.c:412 DONE

  //////////////////// /home/theo/Development/numstore/src/htable.c:198 START
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
  //////////////////// /home/theo/Development/numstore/src/htable.c:198 DONE

  //////////////////// /home/theo/Development/numstore/src/htable.c:273 START
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
  //////////////////// /home/theo/Development/numstore/src/htable.c:273 DONE

  //////////////////// /home/theo/Development/numstore/src/htable.c:281 START
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
  //////////////////// /home/theo/Development/numstore/src/htable.c:281 DONE

  //////////////////// /home/theo/Development/numstore/src/htable.c:290 START
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
  //////////////////// /home/theo/Development/numstore/src/htable.c:290 DONE

  //////////////////// /home/theo/Development/numstore/src/htable.c:298 START
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
  //////////////////// /home/theo/Development/numstore/src/htable.c:298 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/kvt_list_builder.c:199 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/kvt_list_builder.c:199 DONE

  //////////////////// /home/theo/Development/numstore/src/mem_vhmap.c:254 START
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
  //////////////////// /home/theo/Development/numstore/src/mem_vhmap.c:254 DONE

  //////////////////// /home/theo/Development/numstore/src/node_updates.c:225
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
  //////////////////// /home/theo/Development/numstore/src/node_updates.c:225
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/node_updates.c:298
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
  //////////////////// /home/theo/Development/numstore/src/node_updates.c:298
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/node_updates.c:414
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
  //////////////////// /home/theo/Development/numstore/src/node_updates.c:414
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/node_updates.c:664
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
  //////////////////// /home/theo/Development/numstore/src/node_updates.c:664
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/node_updates.c:864
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
  //////////////////// /home/theo/Development/numstore/src/node_updates.c:864
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/node_updates.c:1211
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
  //////////////////// /home/theo/Development/numstore/src/node_updates.c:1211
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/node_updates.c:1285
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
  //////////////////// /home/theo/Development/numstore/src/node_updates.c:1285
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/node_updates.c:1355
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
  //////////////////// /home/theo/Development/numstore/src/node_updates.c:1355
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/node_updates.c:1410
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
  //////////////////// /home/theo/Development/numstore/src/node_updates.c:1410
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/node_updates.c:1453
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
  //////////////////// /home/theo/Development/numstore/src/node_updates.c:1453
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/node_updates.c:1497
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
  //////////////////// /home/theo/Development/numstore/src/node_updates.c:1497
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/node_updates.c:1541
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
  //////////////////// /home/theo/Development/numstore/src/node_updates.c:1541
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/node_updates.c:1595
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
  //////////////////// /home/theo/Development/numstore/src/node_updates.c:1595
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/numerics.c:112 START
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
  //////////////////// /home/theo/Development/numstore/src/numerics.c:112 DONE

  //////////////////// /home/theo/Development/numstore/src/numerics.c:123 START
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
  //////////////////// /home/theo/Development/numstore/src/numerics.c:123 DONE

  //////////////////// /home/theo/Development/numstore/src/numerics.c:135 START
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
  //////////////////// /home/theo/Development/numstore/src/numerics.c:135 DONE

  //////////////////// /home/theo/Development/numstore/src/numerics.c:261 START
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
  //////////////////// /home/theo/Development/numstore/src/numerics.c:261 DONE

  //////////////////// /home/theo/Development/numstore/src/numerics.c:292 START
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
  //////////////////// /home/theo/Development/numstore/src/numerics.c:292 DONE

  //////////////////// /home/theo/Development/numstore/src/numerics.c:370 START
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
  //////////////////// /home/theo/Development/numstore/src/numerics.c:370 DONE

  //////////////////// /home/theo/Development/numstore/src/numerics.c:509 START
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
  //////////////////// /home/theo/Development/numstore/src/numerics.c:509 DONE

  //////////////////// /home/theo/Development/numstore/src/numerics.c:559 START
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
  //////////////////// /home/theo/Development/numstore/src/numerics.c:559 DONE

  //////////////////// /home/theo/Development/numstore/src/numerics.c:623 START
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
  //////////////////// /home/theo/Development/numstore/src/numerics.c:623 DONE

  //////////////////// /home/theo/Development/numstore/src/numerics.c:646 START
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
  //////////////////// /home/theo/Development/numstore/src/numerics.c:646 DONE

  //////////////////// /home/theo/Development/numstore/src/numerics.c:857 START
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
  //////////////////// /home/theo/Development/numstore/src/numerics.c:857 DONE

  //////////////////// /home/theo/Development/numstore/src/numerics.c:1008 START
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
  //////////////////// /home/theo/Development/numstore/src/numerics.c:1008 DONE

  //////////////////// /home/theo/Development/numstore/src/numerics.c:1048 START
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
  //////////////////// /home/theo/Development/numstore/src/numerics.c:1048 DONE

  //////////////////// /home/theo/Development/numstore/src/numerics.c:1085 START
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
  //////////////////// /home/theo/Development/numstore/src/numerics.c:1085 DONE

  //////////////////// /home/theo/Development/numstore/src/os.c:73 START
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
  //////////////////// /home/theo/Development/numstore/src/os.c:73 DONE

  //////////////////// /home/theo/Development/numstore/src/os.c:167 START
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
  //////////////////// /home/theo/Development/numstore/src/os.c:167 DONE

  //////////////////// /home/theo/Development/numstore/src/os.c:208 START
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
  //////////////////// /home/theo/Development/numstore/src/os.c:208 DONE

  //////////////////// /home/theo/Development/numstore/src/os.c:307 START
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
  //////////////////// /home/theo/Development/numstore/src/os.c:307 DONE

  //////////////////// /home/theo/Development/numstore/src/os.c:384 START
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
  //////////////////// /home/theo/Development/numstore/src/os.c:384 DONE

  //////////////////// /home/theo/Development/numstore/src/os.c:490 START
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
  //////////////////// /home/theo/Development/numstore/src/os.c:490 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pager/page_fixture.c:296 START
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
  ////home/theo/Development/numstore/src/pager/page_fixture.c:296 DONE

  //////////////////// /home/theo/Development/numstore/src/pager/pager.c:180
  /// START
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
  //////////////////// /home/theo/Development/numstore/src/pager/pager.c:180
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/pager/pager.c:234
  /// START
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
  //////////////////// /home/theo/Development/numstore/src/pager/pager.c:234
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/pager/pgr_close.c:74
  /// START
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
  //////////////////// /home/theo/Development/numstore/src/pager/pgr_close.c:74
  /// DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pager/pgr_delete_and_release.c:83 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/pager/pgr_delete_and_release.c:83 DONE

  //////////////////// /home/theo/Development/numstore/src/pager/pgr_get.c:112
  /// START
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
  //////////////////// /home/theo/Development/numstore/src/pager/pgr_get.c:112
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/pager/pgr_new.c:218
  /// START
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
  //////////////////// /home/theo/Development/numstore/src/pager/pgr_new.c:218
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/pager/pgr_open.c:277
  /// START
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
  //////////////////// /home/theo/Development/numstore/src/pager/pgr_open.c:277
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/pager/pgr_open.c:319
  /// START
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
  //////////////////// /home/theo/Development/numstore/src/pager/pgr_open.c:319
  /// DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pager/pgr_reserve_and_ctrl_lock.c:95
  /// START
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
  ////////////////////
  ////home/theo/Development/numstore/src/pager/pgr_reserve_and_ctrl_lock.c:95
  /// DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pager/pgr_rollback.c:157 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/pager/pgr_rollback.c:157 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pager/pgr_rollback.c:230 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/pager/pgr_rollback.c:230 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pager/pgr_rollback.c:309 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/pager/pgr_rollback.c:309 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pager/pgr_rollback.c:370 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/pager/pgr_rollback.c:370 DONE

  //////////////////// /home/theo/Development/numstore/src/pages/data_list.c:86
  /// START
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
  //////////////////// /home/theo/Development/numstore/src/pages/data_list.c:86
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/pages/data_list.c:168
  /// START
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
  //////////////////// /home/theo/Development/numstore/src/pages/data_list.c:168
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/pages/data_list.c:231
  /// START
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
  //////////////////// /home/theo/Development/numstore/src/pages/data_list.c:231
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/pages/data_list.c:410
  /// START
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
  //////////////////// /home/theo/Development/numstore/src/pages/data_list.c:410
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/pages/data_list.c:613
  /// START
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
  //////////////////// /home/theo/Development/numstore/src/pages/data_list.c:613
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/pages/data_list.c:711
  /// START
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
  //////////////////// /home/theo/Development/numstore/src/pages/data_list.c:711
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/pages/data_list.c:841
  /// START
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
  //////////////////// /home/theo/Development/numstore/src/pages/data_list.c:841
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/pages/data_list.c:918
  /// START
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
  //////////////////// /home/theo/Development/numstore/src/pages/data_list.c:918
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/pages/data_list.c:998
  /// START
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
  //////////////////// /home/theo/Development/numstore/src/pages/data_list.c:998
  /// DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pages/data_list.c:1088 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/pages/data_list.c:1088 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pages/data_list.c:1198 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/pages/data_list.c:1198 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pages/data_list.c:1231 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/pages/data_list.c:1231 DONE

  //////////////////// /home/theo/Development/numstore/src/pages/fsm_page.c:63
  /// START
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
  //////////////////// /home/theo/Development/numstore/src/pages/fsm_page.c:63
  /// DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:125 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:125 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:186 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:186 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:244 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:244 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:309 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:309 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:471 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:471 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:508 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:508 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:536 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:536 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:559 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:559 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:638 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:638 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:669 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:669 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:682 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:682 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:742 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:742 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:779 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:779 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:807 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:807 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:830 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:830 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:887 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:887 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:985 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:985 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:1023 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:1023 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:1041 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:1041 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:1054 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:1054 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:1126 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/pages/inner_node.c:1126 DONE

  //////////////////// /home/theo/Development/numstore/src/pages/page.c:153
  /// START
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
  //////////////////// /home/theo/Development/numstore/src/pages/page.c:153 DONE

  //////////////////// /home/theo/Development/numstore/src/pages/page.c:233
  /// START
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
  //////////////////// /home/theo/Development/numstore/src/pages/page.c:233 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/pages/var_hash_page.c:73 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/pages/var_hash_page.c:73 DONE

  //////////////////// /home/theo/Development/numstore/src/pages/var_page.c:41
  /// START
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
  //////////////////// /home/theo/Development/numstore/src/pages/var_page.c:41
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/pages/var_page.c:199
  /// START
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
  //////////////////// /home/theo/Development/numstore/src/pages/var_page.c:199
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/pages/var_page.c:297
  /// START
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
  //////////////////// /home/theo/Development/numstore/src/pages/var_page.c:297
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/pages/var_tail.c:26
  /// START
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
  //////////////////// /home/theo/Development/numstore/src/pages/var_tail.c:26
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/pages/var_tail.c:51
  /// START
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
  //////////////////// /home/theo/Development/numstore/src/pages/var_tail.c:51
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/pages/var_tail.c:86
  /// START
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
  //////////////////// /home/theo/Development/numstore/src/pages/var_tail.c:86
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/prim.c:48 START
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
  //////////////////// /home/theo/Development/numstore/src/prim.c:48 DONE

  //////////////////// /home/theo/Development/numstore/src/prim.c:130 START
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
  //////////////////// /home/theo/Development/numstore/src/prim.c:130 DONE

  //////////////////// /home/theo/Development/numstore/src/prim.c:197 START
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
  //////////////////// /home/theo/Development/numstore/src/prim.c:197 DONE

  //////////////////// /home/theo/Development/numstore/src/prim.c:209 START
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
  //////////////////// /home/theo/Development/numstore/src/prim.c:209 DONE

  //////////////////// /home/theo/Development/numstore/src/prim.c:247 START
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
  //////////////////// /home/theo/Development/numstore/src/prim.c:247 DONE

  //////////////////// /home/theo/Development/numstore/src/prim.c:273 START
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
  //////////////////// /home/theo/Development/numstore/src/prim.c:273 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/rope/ns_balance_and_release.c:68 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/rope/ns_balance_and_release.c:68 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/rope/ns_balance_and_release.c:248 START
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
  ////home/theo/Development/numstore/src/rope/ns_balance_and_release.c:248 DONE

  //////////////////// /home/theo/Development/numstore/src/sarray.c:119 START
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
  //////////////////// /home/theo/Development/numstore/src/sarray.c:119 DONE

  //////////////////// /home/theo/Development/numstore/src/sarray.c:157 START
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
  //////////////////// /home/theo/Development/numstore/src/sarray.c:157 DONE

  //////////////////// /home/theo/Development/numstore/src/sarray.c:186 START
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
  //////////////////// /home/theo/Development/numstore/src/sarray.c:186 DONE

  //////////////////// /home/theo/Development/numstore/src/sarray.c:222 START
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
  //////////////////// /home/theo/Development/numstore/src/sarray.c:222 DONE

  //////////////////// /home/theo/Development/numstore/src/sarray.c:315 START
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
  //////////////////// /home/theo/Development/numstore/src/sarray.c:315 DONE

  //////////////////// /home/theo/Development/numstore/src/sarray.c:350 START
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
  //////////////////// /home/theo/Development/numstore/src/sarray.c:350 DONE

  //////////////////// /home/theo/Development/numstore/src/sarray.c:556 START
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
  //////////////////// /home/theo/Development/numstore/src/sarray.c:556 DONE

  //////////////////// /home/theo/Development/numstore/src/serial.c:82 START
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
  //////////////////// /home/theo/Development/numstore/src/serial.c:82 DONE

  //////////////////// /home/theo/Development/numstore/src/serial.c:234 START
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
  //////////////////// /home/theo/Development/numstore/src/serial.c:234 DONE

  //////////////////// /home/theo/Development/numstore/src/stride.c:158 START
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
  //////////////////// /home/theo/Development/numstore/src/stride.c:158 DONE

  //////////////////// /home/theo/Development/numstore/src/struct.c:244 START
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
  //////////////////// /home/theo/Development/numstore/src/struct.c:244 DONE

  //////////////////// /home/theo/Development/numstore/src/struct.c:312 START
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
  //////////////////// /home/theo/Development/numstore/src/struct.c:312 DONE

  //////////////////// /home/theo/Development/numstore/src/struct.c:380 START
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
  //////////////////// /home/theo/Development/numstore/src/struct.c:380 DONE

  //////////////////// /home/theo/Development/numstore/src/struct.c:455 START
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
  //////////////////// /home/theo/Development/numstore/src/struct.c:455 DONE

  //////////////////// /home/theo/Development/numstore/src/struct.c:605 START
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
  //////////////////// /home/theo/Development/numstore/src/struct.c:605 DONE

  //////////////////// /home/theo/Development/numstore/src/struct.c:661 START
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
  //////////////////// /home/theo/Development/numstore/src/struct.c:661 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/aries_tests.c:20 START
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
  ////home/theo/Development/numstore/src/testing/aries_tests.c:20 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:54 START
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
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:54 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:73 START
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
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:73 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:80 START
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
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:80 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:112 START
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
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:112 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:166 START
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
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:166 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:201 START
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
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:201 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:232 START
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
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:232 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:271 START
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
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:271 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:295 START
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
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:295 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:329 START
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
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:329 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:366 START
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
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:366 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:388 START
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
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:388 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:397 START
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
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:397 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:410 START
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
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:410 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:429 START
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
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:429 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:448 START
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
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:448 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:523 START
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
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:523 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:564 START
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
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:564 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:621 START
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
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:621 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:659 START
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
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:659 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:728 START
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
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:728 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:779 START
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
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:779 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:816 START
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
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:816 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:875 START
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
  ////home/theo/Development/numstore/src/testing/core_extra_tests.c:875 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/dirty_page_table_tests.c:19
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
  ////home/theo/Development/numstore/src/testing/dirty_page_table_tests.c:19
  /// DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/dirty_page_table_tests.c:39
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
  ////home/theo/Development/numstore/src/testing/dirty_page_table_tests.c:39
  /// DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/dirty_page_table_tests.c:104
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
  ////home/theo/Development/numstore/src/testing/dirty_page_table_tests.c:104
  /// DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/dirty_page_table_tests.c:134
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
  ////home/theo/Development/numstore/src/testing/dirty_page_table_tests.c:134
  /// DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/dirty_page_table_tests.c:157
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
  ////home/theo/Development/numstore/src/testing/dirty_page_table_tests.c:157
  /// DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/dirty_page_table_tests.c:194
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
  ////home/theo/Development/numstore/src/testing/dirty_page_table_tests.c:194
  /// DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/dirty_page_table_tests.c:260
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
  ////home/theo/Development/numstore/src/testing/dirty_page_table_tests.c:260
  /// DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/dirty_page_table_tests.c:324
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
  ////home/theo/Development/numstore/src/testing/dirty_page_table_tests.c:324
  /// DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/dirty_page_table_tests.c:388
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
  ////home/theo/Development/numstore/src/testing/dirty_page_table_tests.c:388
  /// DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/dpgt_concurrency_tests.c:100
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
  ////home/theo/Development/numstore/src/testing/dpgt_concurrency_tests.c:100
  /// DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/numstore_regression_tests.c:21
  /// START
  if (!filter || strstr ("cgd_test_create_delete_rollback_delete", filter))
  {
    extern void __test__cgd_test_create_delete_rollback_delete (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "cgd_test_create_delete_rollback_delete"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__cgd_test_create_delete_rollback_delete ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "cgd_test_create_delete_rollback_delete");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cgd_test_create_delete_rollback_delete";
    }
    ntests++;
  }
  ////////////////////
  ////home/theo/Development/numstore/src/testing/numstore_regression_tests.c:21
  /// DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/numstore_regression_tests.c:67
  /// START
  if (!filter || strstr ("cgd_test_create_crash_close_delete", filter))
  {
    extern void __test__cgd_test_create_crash_close_delete (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "cgd_test_create_crash_close_delete"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__cgd_test_create_crash_close_delete ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "cgd_test_create_crash_close_delete");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cgd_test_create_crash_close_delete";
    }
    ntests++;
  }
  ////////////////////
  ////home/theo/Development/numstore/src/testing/numstore_regression_tests.c:67
  /// DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/numstore_regression_tests.c:97
  /// START
  if (!filter || strstr ("irwr_rollback_invalid_wal_header", filter))
  {
    extern void __test__irwr_rollback_invalid_wal_header (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "irwr_rollback_invalid_wal_header"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__irwr_rollback_invalid_wal_header ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "irwr_rollback_invalid_wal_header");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "irwr_rollback_invalid_wal_header";
    }
    ntests++;
  }
  ////////////////////
  ////home/theo/Development/numstore/src/testing/numstore_regression_tests.c:97
  /// DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/numstore_tests.c:24 START
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
  ////home/theo/Development/numstore/src/testing/numstore_tests.c:24 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/numstore_tests.c:179 START
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
  ////home/theo/Development/numstore/src/testing/numstore_tests.c:179 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/numstore_tests.c:287 START
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
  ////home/theo/Development/numstore/src/testing/numstore_tests.c:287 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/numstore_tests.c:503 START
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
  ////home/theo/Development/numstore/src/testing/numstore_tests.c:503 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/numstore_tests.c:762 START
  if (!filter || strstr ("nsdb_get_if_exists", filter))
  {
    extern void __test__nsdb_get_if_exists (void);
    i_log_info (
        "========================= TEST CASE: %s\n",
        "nsdb_get_if_exists"
    );
    int prev = test_ret;
    test_ret = 0;
    __test__nsdb_get_if_exists ();
    if (!test_ret)
    {
      i_log_passed ("%s\n", "nsdb_get_if_exists");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nsdb_get_if_exists";
    }
    ntests++;
  }
  ////////////////////
  ////home/theo/Development/numstore/src/testing/numstore_tests.c:762 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/robin_hood_ht_tests.c:27 START
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
  ////home/theo/Development/numstore/src/testing/robin_hood_ht_tests.c:27 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/robin_hood_ht_tests.c:105 START
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
  ////home/theo/Development/numstore/src/testing/robin_hood_ht_tests.c:105 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/smfile_data_writer.c:125 START
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
  ////home/theo/Development/numstore/src/testing/smfile_data_writer.c:125 DONE

  //////////////////// /home/theo/Development/numstore/src/testing/testing.c:38
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
  //////////////////// /home/theo/Development/numstore/src/testing/testing.c:38
  /// DONE

  //////////////////// /home/theo/Development/numstore/src/testing/testing.c:49
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
  //////////////////// /home/theo/Development/numstore/src/testing/testing.c:49
  /// DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/txnt_concurrency_tests.c:119
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
  ////home/theo/Development/numstore/src/testing/txnt_concurrency_tests.c:119
  /// DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/wal_tests.c:281 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/testing/wal_tests.c:281 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/testing/wal_tests.c:399 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/testing/wal_tests.c:399 DONE

  //////////////////// /home/theo/Development/numstore/src/txn_table.c:324 START
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
  //////////////////// /home/theo/Development/numstore/src/txn_table.c:324 DONE

  //////////////////// /home/theo/Development/numstore/src/txn_table.c:431 START
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
  //////////////////// /home/theo/Development/numstore/src/txn_table.c:431 DONE

  //////////////////// /home/theo/Development/numstore/src/txn_table.c:577 START
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
  //////////////////// /home/theo/Development/numstore/src/txn_table.c:577 DONE

  //////////////////// /home/theo/Development/numstore/src/txn_table.c:718 START
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
  //////////////////// /home/theo/Development/numstore/src/txn_table.c:718 DONE

  //////////////////// /home/theo/Development/numstore/src/txn_table.c:846 START
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
  //////////////////// /home/theo/Development/numstore/src/txn_table.c:846 DONE

  //////////////////// /home/theo/Development/numstore/src/txn_table.c:954 START
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
  //////////////////// /home/theo/Development/numstore/src/txn_table.c:954 DONE

  //////////////////// /home/theo/Development/numstore/src/txn_table.c:1011
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
  //////////////////// /home/theo/Development/numstore/src/txn_table.c:1011 DONE

  //////////////////// /home/theo/Development/numstore/src/txn_table.c:1164
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
  //////////////////// /home/theo/Development/numstore/src/txn_table.c:1164 DONE

  //////////////////// /home/theo/Development/numstore/src/txn_table.c:1348
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
  //////////////////// /home/theo/Development/numstore/src/txn_table.c:1348 DONE

  //////////////////// /home/theo/Development/numstore/src/txn_table.c:1568
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
  //////////////////// /home/theo/Development/numstore/src/txn_table.c:1568 DONE

  //////////////////// /home/theo/Development/numstore/src/txn_table.c:1702
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
  //////////////////// /home/theo/Development/numstore/src/txn_table.c:1702 DONE

  //////////////////// /home/theo/Development/numstore/src/types.c:285 START
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
  //////////////////// /home/theo/Development/numstore/src/types.c:285 DONE

  //////////////////// /home/theo/Development/numstore/src/types.c:968 START
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
  //////////////////// /home/theo/Development/numstore/src/types.c:968 DONE

  //////////////////// /home/theo/Development/numstore/src/types.c:1420 START
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
  //////////////////// /home/theo/Development/numstore/src/types.c:1420 DONE

  //////////////////// /home/theo/Development/numstore/src/types.c:1791 START
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
  //////////////////// /home/theo/Development/numstore/src/types.c:1791 DONE

  //////////////////// /home/theo/Development/numstore/src/union.c:243 START
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
  //////////////////// /home/theo/Development/numstore/src/union.c:243 DONE

  //////////////////// /home/theo/Development/numstore/src/union.c:314 START
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
  //////////////////// /home/theo/Development/numstore/src/union.c:314 DONE

  //////////////////// /home/theo/Development/numstore/src/union.c:382 START
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
  //////////////////// /home/theo/Development/numstore/src/union.c:382 DONE

  //////////////////// /home/theo/Development/numstore/src/union.c:457 START
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
  //////////////////// /home/theo/Development/numstore/src/union.c:457 DONE

  //////////////////// /home/theo/Development/numstore/src/union.c:603 START
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
  //////////////////// /home/theo/Development/numstore/src/union.c:603 DONE

  //////////////////// /home/theo/Development/numstore/src/union.c:659 START
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
  //////////////////// /home/theo/Development/numstore/src/union.c:659 DONE

  //////////////////// /home/theo/Development/numstore/src/utils.c:30 START
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
  //////////////////// /home/theo/Development/numstore/src/utils.c:30 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/var/ns_find_var_page.c:349 START
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
  ////////////////////
  ////home/theo/Development/numstore/src/var/ns_find_var_page.c:349 DONE

  ////////////////////
  ////home/theo/Development/numstore/src/var/ns_var_get_or_create.c:88 START
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
  ////home/theo/Development/numstore/src/var/ns_var_get_or_create.c:88 DONE

  //////////////////// /home/theo/Development/numstore/src/variables.c:274 START
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
  //////////////////// /home/theo/Development/numstore/src/variables.c:274 DONE

  //////////////////// /home/theo/Development/numstore/src/variables.c:291 START
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
  //////////////////// /home/theo/Development/numstore/src/variables.c:291 DONE

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
