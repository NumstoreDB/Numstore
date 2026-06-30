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

// clang-format off

// AUTO GENERATED - DO NOT MODIFY

#include <stdio.h>
#include <string.h>

#include "error.h"
#include "logging.h"
#include "os.h"
#include "testing/testing.h"

HEADER_FUNC int
run_unit_tests (const char* filter)
{
  int ntests = 0;

  error   e = error_create ();
  i_timer timer;
  if (i_timer_create (&timer, &e) != SUCCESS)
  {
    return -1;
  }

  int         failed = 0;
  const char *failed_names[292];

  
  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:376 START
  if (!filter || strstr("slab_alloc_simple", filter))
  {
    extern void __test__slab_alloc_simple(void);
    i_log_info("========================= TEST CASE: %s\n", "slab_alloc_simple");
    int prev = test_ret;
    test_ret = 0;
    __test__slab_alloc_simple();
    if (!test_ret)
    {
      i_log_passed("%s\n", "slab_alloc_simple");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "slab_alloc_simple";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:376 DONE

  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:492 START
  if (!filter || strstr("slab_alloc_cap_one", filter))
  {
    extern void __test__slab_alloc_cap_one(void);
    i_log_info("========================= TEST CASE: %s\n", "slab_alloc_cap_one");
    int prev = test_ret;
    test_ret = 0;
    __test__slab_alloc_cap_one();
    if (!test_ret)
    {
      i_log_passed("%s\n", "slab_alloc_cap_one");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "slab_alloc_cap_one";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:492 DONE

  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:528 START
  if (!filter || strstr("slab_alloc_no_duplicates", filter))
  {
    extern void __test__slab_alloc_no_duplicates(void);
    i_log_info("========================= TEST CASE: %s\n", "slab_alloc_no_duplicates");
    int prev = test_ret;
    test_ret = 0;
    __test__slab_alloc_no_duplicates();
    if (!test_ret)
    {
      i_log_passed("%s\n", "slab_alloc_no_duplicates");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "slab_alloc_no_duplicates";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:528 DONE

  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:557 START
  if (!filter || strstr("slab_alloc_free_all_realloc", filter))
  {
    extern void __test__slab_alloc_free_all_realloc(void);
    i_log_info("========================= TEST CASE: %s\n", "slab_alloc_free_all_realloc");
    int prev = test_ret;
    test_ret = 0;
    __test__slab_alloc_free_all_realloc();
    if (!test_ret)
    {
      i_log_passed("%s\n", "slab_alloc_free_all_realloc");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "slab_alloc_free_all_realloc";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:557 DONE

  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:600 START
  if (!filter || strstr("slab_alloc_interleaved_patterns", filter))
  {
    extern void __test__slab_alloc_interleaved_patterns(void);
    i_log_info("========================= TEST CASE: %s\n", "slab_alloc_interleaved_patterns");
    int prev = test_ret;
    test_ret = 0;
    __test__slab_alloc_interleaved_patterns();
    if (!test_ret)
    {
      i_log_passed("%s\n", "slab_alloc_interleaved_patterns");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "slab_alloc_interleaved_patterns";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:600 DONE

  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:652 START
  if (!filter || strstr("slab_alloc_free_head_slab", filter))
  {
    extern void __test__slab_alloc_free_head_slab(void);
    i_log_info("========================= TEST CASE: %s\n", "slab_alloc_free_head_slab");
    int prev = test_ret;
    test_ret = 0;
    __test__slab_alloc_free_head_slab();
    if (!test_ret)
    {
      i_log_passed("%s\n", "slab_alloc_free_head_slab");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "slab_alloc_free_head_slab";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:652 DONE

  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:696 START
  if (!filter || strstr("slab_alloc_free_middle_slab", filter))
  {
    extern void __test__slab_alloc_free_middle_slab(void);
    i_log_info("========================= TEST CASE: %s\n", "slab_alloc_free_middle_slab");
    int prev = test_ret;
    test_ret = 0;
    __test__slab_alloc_free_middle_slab();
    if (!test_ret)
    {
      i_log_passed("%s\n", "slab_alloc_free_middle_slab");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "slab_alloc_free_middle_slab";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:696 DONE

  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:742 START
  if (!filter || strstr("slab_alloc_minimum_size", filter))
  {
    extern void __test__slab_alloc_minimum_size(void);
    i_log_info("========================= TEST CASE: %s\n", "slab_alloc_minimum_size");
    int prev = test_ret;
    test_ret = 0;
    __test__slab_alloc_minimum_size();
    if (!test_ret)
    {
      i_log_passed("%s\n", "slab_alloc_minimum_size");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "slab_alloc_minimum_size";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:742 DONE

  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:771 START
  if (!filter || strstr("slab_alloc_stress_random", filter))
  {
    extern void __test__slab_alloc_stress_random(void);
    i_log_info("========================= TEST CASE: %s\n", "slab_alloc_stress_random");
    int prev = test_ret;
    test_ret = 0;
    __test__slab_alloc_stress_random();
    if (!test_ret)
    {
      i_log_passed("%s\n", "slab_alloc_stress_random");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "slab_alloc_stress_random";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/alloc.c:771 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:35 START
  if (!filter || strstr("llist", filter))
  {
    extern void __test__llist(void);
    i_log_info("========================= TEST CASE: %s\n", "llist");
    int prev = test_ret;
    test_ret = 0;
    __test__llist();
    if (!test_ret)
    {
      i_log_passed("%s\n", "llist");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "llist";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:35 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:87 START
  if (!filter || strstr("cbuffer_isempty", filter))
  {
    extern void __test__cbuffer_isempty(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_isempty");
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_isempty();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_isempty");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_isempty";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:87 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:97 START
  if (!filter || strstr("cbuffer_len", filter))
  {
    extern void __test__cbuffer_len(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_len");
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_len();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_len");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_len";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:97 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:107 START
  if (!filter || strstr("cbuffer_avail", filter))
  {
    extern void __test__cbuffer_avail(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_avail");
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_avail();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_avail");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_avail";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:107 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:157 START
  if (!filter || strstr("cbuffer_get_next_data_bytes", filter))
  {
    extern void __test__cbuffer_get_next_data_bytes(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_get_next_data_bytes");
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_get_next_data_bytes();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_get_next_data_bytes");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_get_next_data_bytes";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:157 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:230 START
  if (!filter || strstr("cbuffer_get_nbytes", filter))
  {
    extern void __test__cbuffer_get_nbytes(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_get_nbytes");
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_get_nbytes();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_get_nbytes");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_get_nbytes";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:230 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:297 START
  if (!filter || strstr("cbuffer_fakewrite", filter))
  {
    extern void __test__cbuffer_fakewrite(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_fakewrite");
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_fakewrite();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_fakewrite");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_fakewrite";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:297 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:384 START
  if (!filter || strstr("cbuffer_fakeread", filter))
  {
    extern void __test__cbuffer_fakeread(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_fakeread");
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_fakeread();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_fakeread");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_fakeread";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:384 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:505 START
  if (!filter || strstr("cbuffer_read", filter))
  {
    extern void __test__cbuffer_read(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_read");
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_read();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_read");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_read";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:505 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:577 START
  if (!filter || strstr("cbuffer_copy", filter))
  {
    extern void __test__cbuffer_copy(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_copy");
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_copy();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_copy");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_copy";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:577 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:666 START
  if (!filter || strstr("cbuffer_write", filter))
  {
    extern void __test__cbuffer_write(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_write");
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_write();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_write");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_write";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:666 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:760 START
  if (!filter || strstr("cbuffer_cbuffer_move", filter))
  {
    extern void __test__cbuffer_cbuffer_move(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_cbuffer_move");
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_cbuffer_move();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_cbuffer_move");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_cbuffer_move";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:760 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:869 START
  if (!filter || strstr("cbuffer_cbuffer_copy", filter))
  {
    extern void __test__cbuffer_cbuffer_copy(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_cbuffer_copy");
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_cbuffer_copy();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_cbuffer_copy");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_cbuffer_copy";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:869 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1030 START
  if (!filter || strstr("cbuffer_get_no_check", filter))
  {
    extern void __test__cbuffer_get_no_check(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_get_no_check");
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_get_no_check();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_get_no_check");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_get_no_check";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1030 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1132 START
  if (!filter || strstr("cbuffer_get", filter))
  {
    extern void __test__cbuffer_get(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_get");
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_get();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_get");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_get";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1132 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1163 START
  if (!filter || strstr("cbuffer_peek_back", filter))
  {
    extern void __test__cbuffer_peek_back(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_peek_back");
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_peek_back();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_peek_back");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_peek_back";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1163 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1219 START
  if (!filter || strstr("cbuffer_peek_front", filter))
  {
    extern void __test__cbuffer_peek_front(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_peek_front");
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_peek_front();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_peek_front");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_peek_front";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1219 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1293 START
  if (!filter || strstr("cbuffer_push_back", filter))
  {
    extern void __test__cbuffer_push_back(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_push_back");
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_push_back();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_push_back");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_push_back";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1293 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1354 START
  if (!filter || strstr("cbuffer_push_front", filter))
  {
    extern void __test__cbuffer_push_front(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_push_front");
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_push_front();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_push_front");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_push_front";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1354 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1425 START
  if (!filter || strstr("cbuffer_pop_back", filter))
  {
    extern void __test__cbuffer_pop_back(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_pop_back");
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_pop_back();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_pop_back");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_pop_back";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1425 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1496 START
  if (!filter || strstr("cbuffer_pop_front", filter))
  {
    extern void __test__cbuffer_pop_front(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_pop_front");
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_pop_front();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_pop_front");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_pop_front";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1496 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1633 START
  if (!filter || strstr("dblb_create_basic", filter))
  {
    extern void __test__dblb_create_basic(void);
    i_log_info("========================= TEST CASE: %s\n", "dblb_create_basic");
    int prev = test_ret;
    test_ret = 0;
    __test__dblb_create_basic();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dblb_create_basic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_create_basic";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1633 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1651 START
  if (!filter || strstr("dblb_append_single", filter))
  {
    extern void __test__dblb_append_single(void);
    i_log_info("========================= TEST CASE: %s\n", "dblb_append_single");
    int prev = test_ret;
    test_ret = 0;
    __test__dblb_append_single();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dblb_append_single");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_append_single";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1651 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1674 START
  if (!filter || strstr("dblb_append_multiple", filter))
  {
    extern void __test__dblb_append_multiple(void);
    i_log_info("========================= TEST CASE: %s\n", "dblb_append_multiple");
    int prev = test_ret;
    test_ret = 0;
    __test__dblb_append_multiple();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dblb_append_multiple");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_append_multiple";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1674 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1698 START
  if (!filter || strstr("dblb_append_triggers_realloc", filter))
  {
    extern void __test__dblb_append_triggers_realloc(void);
    i_log_info("========================= TEST CASE: %s\n", "dblb_append_triggers_realloc");
    int prev = test_ret;
    test_ret = 0;
    __test__dblb_append_triggers_realloc();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dblb_append_triggers_realloc");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_append_triggers_realloc";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1698 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1737 START
  if (!filter || strstr("dblb_append_alloc_basic", filter))
  {
    extern void __test__dblb_append_alloc_basic(void);
    i_log_info("========================= TEST CASE: %s\n", "dblb_append_alloc_basic");
    int prev = test_ret;
    test_ret = 0;
    __test__dblb_append_alloc_basic();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dblb_append_alloc_basic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_append_alloc_basic";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1737 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1764 START
  if (!filter || strstr("dblb_append_alloc_sequential", filter))
  {
    extern void __test__dblb_append_alloc_sequential(void);
    i_log_info("========================= TEST CASE: %s\n", "dblb_append_alloc_sequential");
    int prev = test_ret;
    test_ret = 0;
    __test__dblb_append_alloc_sequential();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dblb_append_alloc_sequential");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_append_alloc_sequential";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1764 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1797 START
  if (!filter || strstr("dblb_append_alloc_triggers_realloc", filter))
  {
    extern void __test__dblb_append_alloc_triggers_realloc(void);
    i_log_info("========================= TEST CASE: %s\n", "dblb_append_alloc_triggers_realloc");
    int prev = test_ret;
    test_ret = 0;
    __test__dblb_append_alloc_triggers_realloc();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dblb_append_alloc_triggers_realloc");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_append_alloc_triggers_realloc";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1797 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1819 START
  if (!filter || strstr("dblb_different_element_sizes", filter))
  {
    extern void __test__dblb_different_element_sizes(void);
    i_log_info("========================= TEST CASE: %s\n", "dblb_different_element_sizes");
    int prev = test_ret;
    test_ret = 0;
    __test__dblb_different_element_sizes();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dblb_different_element_sizes");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_different_element_sizes";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1819 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1840 START
  if (!filter || strstr("dblb_struct_elements", filter))
  {
    extern void __test__dblb_struct_elements(void);
    i_log_info("========================= TEST CASE: %s\n", "dblb_struct_elements");
    int prev = test_ret;
    test_ret = 0;
    __test__dblb_struct_elements();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dblb_struct_elements");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_struct_elements";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1840 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1870 START
  if (!filter || strstr("dblb_free_resets", filter))
  {
    extern void __test__dblb_free_resets(void);
    i_log_info("========================= TEST CASE: %s\n", "dblb_free_resets");
    int prev = test_ret;
    test_ret = 0;
    __test__dblb_free_resets();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dblb_free_resets");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_free_resets";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1870 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1888 START
  if (!filter || strstr("dblb_large_append", filter))
  {
    extern void __test__dblb_large_append(void);
    i_log_info("========================= TEST CASE: %s\n", "dblb_large_append");
    int prev = test_ret;
    test_ret = 0;
    __test__dblb_large_append();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dblb_large_append");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_large_append";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:1888 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:2170 START
  if (!filter || strstr("ext_array_insert_read", filter))
  {
    extern void __test__ext_array_insert_read(void);
    i_log_info("========================= TEST CASE: %s\n", "ext_array_insert_read");
    int prev = test_ret;
    test_ret = 0;
    __test__ext_array_insert_read();
    if (!test_ret)
    {
      i_log_passed("%s\n", "ext_array_insert_read");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ext_array_insert_read";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:2170 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:2309 START
  if (!filter || strstr("ext_array_write", filter))
  {
    extern void __test__ext_array_write(void);
    i_log_info("========================= TEST CASE: %s\n", "ext_array_write");
    int prev = test_ret;
    test_ret = 0;
    __test__ext_array_write();
    if (!test_ret)
    {
      i_log_passed("%s\n", "ext_array_write");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ext_array_write";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:2309 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:2415 START
  if (!filter || strstr("ext_array_remove", filter))
  {
    extern void __test__ext_array_remove(void);
    i_log_info("========================= TEST CASE: %s\n", "ext_array_remove");
    int prev = test_ret;
    test_ret = 0;
    __test__ext_array_remove();
    if (!test_ret)
    {
      i_log_passed("%s\n", "ext_array_remove");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ext_array_remove";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:2415 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:2582 START
  if (!filter || strstr("ext_array_random", filter))
  {
    extern void __test__ext_array_random(void);
    i_log_info("========================= TEST CASE: %s\n", "ext_array_random");
    int prev = test_ret;
    test_ret = 0;
    __test__ext_array_random();
    if (!test_ret)
    {
      i_log_passed("%s\n", "ext_array_random");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ext_array_random";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:2582 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:3311 START
  if (!filter || strstr("block_insert_read", filter))
  {
    extern void __test__block_insert_read(void);
    i_log_info("========================= TEST CASE: %s\n", "block_insert_read");
    int prev = test_ret;
    test_ret = 0;
    __test__block_insert_read();
    if (!test_ret)
    {
      i_log_passed("%s\n", "block_insert_read");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "block_insert_read";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:3311 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:3571 START
  if (!filter || strstr("block_insert_remove_read", filter))
  {
    extern void __test__block_insert_remove_read(void);
    i_log_info("========================= TEST CASE: %s\n", "block_insert_remove_read");
    int prev = test_ret;
    test_ret = 0;
    __test__block_insert_remove_read();
    if (!test_ret)
    {
      i_log_passed("%s\n", "block_insert_remove_read");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "block_insert_remove_read";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:3571 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:3716 START
  if (!filter || strstr("block_insert_write_read", filter))
  {
    extern void __test__block_insert_write_read(void);
    i_log_info("========================= TEST CASE: %s\n", "block_insert_write_read");
    int prev = test_ret;
    test_ret = 0;
    __test__block_insert_write_read();
    if (!test_ret)
    {
      i_log_passed("%s\n", "block_insert_write_read");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "block_insert_write_read";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:3716 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:3879 START
  if (!filter || strstr("block_random", filter))
  {
    extern void __test__block_random(void);
    i_log_info("========================= TEST CASE: %s\n", "block_random");
    int prev = test_ret;
    test_ret = 0;
    __test__block_random();
    if (!test_ret)
    {
      i_log_passed("%s\n", "block_random");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "block_random";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:3879 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:3968 START
  if (!filter || strstr("ba_memcpy_from_basic", filter))
  {
    extern void __test__ba_memcpy_from_basic(void);
    i_log_info("========================= TEST CASE: %s\n", "ba_memcpy_from_basic");
    int prev = test_ret;
    test_ret = 0;
    __test__ba_memcpy_from_basic();
    if (!test_ret)
    {
      i_log_passed("%s\n", "ba_memcpy_from_basic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ba_memcpy_from_basic";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:3968 DONE

  //////////////////// /Users/theo/Development/Numstore/src/collections.c:4298 START
  if (!filter || strstr("ba_memcpy_to_basic", filter))
  {
    extern void __test__ba_memcpy_to_basic(void);
    i_log_info("========================= TEST CASE: %s\n", "ba_memcpy_to_basic");
    int prev = test_ret;
    test_ret = 0;
    __test__ba_memcpy_to_basic();
    if (!test_ret)
    {
      i_log_passed("%s\n", "ba_memcpy_to_basic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ba_memcpy_to_basic";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/collections.c:4298 DONE

  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:708 START
  if (!filter || strstr("lexer_two_char_tokens", filter))
  {
    extern void __test__lexer_two_char_tokens(void);
    i_log_info("========================= TEST CASE: %s\n", "lexer_two_char_tokens");
    int prev = test_ret;
    test_ret = 0;
    __test__lexer_two_char_tokens();
    if (!test_ret)
    {
      i_log_passed("%s\n", "lexer_two_char_tokens");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_two_char_tokens";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:708 DONE

  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:734 START
  if (!filter || strstr("lexer_single_char_operators", filter))
  {
    extern void __test__lexer_single_char_operators(void);
    i_log_info("========================= TEST CASE: %s\n", "lexer_single_char_operators");
    int prev = test_ret;
    test_ret = 0;
    __test__lexer_single_char_operators();
    if (!test_ret)
    {
      i_log_passed("%s\n", "lexer_single_char_operators");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_single_char_operators";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:734 DONE

  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:762 START
  if (!filter || strstr("lexer_strings", filter))
  {
    extern void __test__lexer_strings(void);
    i_log_info("========================= TEST CASE: %s\n", "lexer_strings");
    int prev = test_ret;
    test_ret = 0;
    __test__lexer_strings();
    if (!test_ret)
    {
      i_log_passed("%s\n", "lexer_strings");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_strings";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:762 DONE

  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:775 START
  if (!filter || strstr("lexer_identifiers", filter))
  {
    extern void __test__lexer_identifiers(void);
    i_log_info("========================= TEST CASE: %s\n", "lexer_identifiers");
    int prev = test_ret;
    test_ret = 0;
    __test__lexer_identifiers();
    if (!test_ret)
    {
      i_log_passed("%s\n", "lexer_identifiers");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_identifiers";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:775 DONE

  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:789 START
  if (!filter || strstr("lexer_numbers", filter))
  {
    extern void __test__lexer_numbers(void);
    i_log_info("========================= TEST CASE: %s\n", "lexer_numbers");
    int prev = test_ret;
    test_ret = 0;
    __test__lexer_numbers();
    if (!test_ret)
    {
      i_log_passed("%s\n", "lexer_numbers");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_numbers";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:789 DONE

  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:805 START
  if (!filter || strstr("lexer_keywords", filter))
  {
    extern void __test__lexer_keywords(void);
    i_log_info("========================= TEST CASE: %s\n", "lexer_keywords");
    int prev = test_ret;
    test_ret = 0;
    __test__lexer_keywords();
    if (!test_ret)
    {
      i_log_passed("%s\n", "lexer_keywords");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_keywords";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:805 DONE

  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:832 START
  if (!filter || strstr("lexer_primitives", filter))
  {
    extern void __test__lexer_primitives(void);
    i_log_info("========================= TEST CASE: %s\n", "lexer_primitives");
    int prev = test_ret;
    test_ret = 0;
    __test__lexer_primitives();
    if (!test_ret)
    {
      i_log_passed("%s\n", "lexer_primitives");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_primitives";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:832 DONE

  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:852 START
  if (!filter || strstr("lexer_whitespace_handling", filter))
  {
    extern void __test__lexer_whitespace_handling(void);
    i_log_info("========================= TEST CASE: %s\n", "lexer_whitespace_handling");
    int prev = test_ret;
    test_ret = 0;
    __test__lexer_whitespace_handling();
    if (!test_ret)
    {
      i_log_passed("%s\n", "lexer_whitespace_handling");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_whitespace_handling";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:852 DONE

  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:870 START
  if (!filter || strstr("lexer_complex_expression", filter))
  {
    extern void __test__lexer_complex_expression(void);
    i_log_info("========================= TEST CASE: %s\n", "lexer_complex_expression");
    int prev = test_ret;
    test_ret = 0;
    __test__lexer_complex_expression();
    if (!test_ret)
    {
      i_log_passed("%s\n", "lexer_complex_expression");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_complex_expression";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:870 DONE

  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:892 START
  if (!filter || strstr("lexer_keyword_prefix", filter))
  {
    extern void __test__lexer_keyword_prefix(void);
    i_log_info("========================= TEST CASE: %s\n", "lexer_keyword_prefix");
    int prev = test_ret;
    test_ret = 0;
    __test__lexer_keyword_prefix();
    if (!test_ret)
    {
      i_log_passed("%s\n", "lexer_keyword_prefix");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_keyword_prefix";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:892 DONE

  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:908 START
  if (!filter || strstr("lexer_errors", filter))
  {
    extern void __test__lexer_errors(void);
    i_log_info("========================= TEST CASE: %s\n", "lexer_errors");
    int prev = test_ret;
    test_ret = 0;
    __test__lexer_errors();
    if (!test_ret)
    {
      i_log_passed("%s\n", "lexer_errors");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_errors";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:908 DONE

  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:920 START
  if (!filter || strstr("lexer_empty_string", filter))
  {
    extern void __test__lexer_empty_string(void);
    i_log_info("========================= TEST CASE: %s\n", "lexer_empty_string");
    int prev = test_ret;
    test_ret = 0;
    __test__lexer_empty_string();
    if (!test_ret)
    {
      i_log_passed("%s\n", "lexer_empty_string");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_empty_string";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:920 DONE

  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:931 START
  if (!filter || strstr("lexer_numbers_in_sequence", filter))
  {
    extern void __test__lexer_numbers_in_sequence(void);
    i_log_info("========================= TEST CASE: %s\n", "lexer_numbers_in_sequence");
    int prev = test_ret;
    test_ret = 0;
    __test__lexer_numbers_in_sequence();
    if (!test_ret)
    {
      i_log_passed("%s\n", "lexer_numbers_in_sequence");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_numbers_in_sequence";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/compiler.c:931 DONE

  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:58 START
  if (!filter || strstr("gr_lock_init", filter))
  {
    extern void __test__gr_lock_init(void);
    i_log_info("========================= TEST CASE: %s\n", "gr_lock_init");
    int prev = test_ret;
    test_ret = 0;
    __test__gr_lock_init();
    if (!test_ret)
    {
      i_log_passed("%s\n", "gr_lock_init");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "gr_lock_init";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:58 DONE

  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:103 START
  if (!filter || strstr("gr_lock_destroy", filter))
  {
    extern void __test__gr_lock_destroy(void);
    i_log_info("========================= TEST CASE: %s\n", "gr_lock_destroy");
    int prev = test_ret;
    test_ret = 0;
    __test__gr_lock_destroy();
    if (!test_ret)
    {
      i_log_passed("%s\n", "gr_lock_destroy");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "gr_lock_destroy";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:103 DONE

  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:149 START
  if (!filter || strstr("gr_lock_is_compatible", filter))
  {
    extern void __test__gr_lock_is_compatible(void);
    i_log_info("========================= TEST CASE: %s\n", "gr_lock_is_compatible");
    int prev = test_ret;
    test_ret = 0;
    __test__gr_lock_is_compatible();
    if (!test_ret)
    {
      i_log_passed("%s\n", "gr_lock_is_compatible");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "gr_lock_is_compatible";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:149 DONE

  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:359 START
  if (!filter || strstr("gr_lock_unlock", filter))
  {
    extern void __test__gr_lock_unlock(void);
    i_log_info("========================= TEST CASE: %s\n", "gr_lock_unlock");
    int prev = test_ret;
    test_ret = 0;
    __test__gr_lock_unlock();
    if (!test_ret)
    {
      i_log_passed("%s\n", "gr_lock_unlock");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "gr_lock_unlock";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:359 DONE

  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:457 START
  if (!filter || strstr("gr_lock_mode_name", filter))
  {
    extern void __test__gr_lock_mode_name(void);
    i_log_info("========================= TEST CASE: %s\n", "gr_lock_mode_name");
    int prev = test_ret;
    test_ret = 0;
    __test__gr_lock_mode_name();
    if (!test_ret)
    {
      i_log_passed("%s\n", "gr_lock_mode_name");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "gr_lock_mode_name";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:457 DONE

  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:616 START
  if (!filter || strstr("gr_lock_basic_sanity", filter))
  {
    extern void __test__gr_lock_basic_sanity(void);
    i_log_info("========================= TEST CASE: %s\n", "gr_lock_basic_sanity");
    int prev = test_ret;
    test_ret = 0;
    __test__gr_lock_basic_sanity();
    if (!test_ret)
    {
      i_log_passed("%s\n", "gr_lock_basic_sanity");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "gr_lock_basic_sanity";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:616 DONE

  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:633 START
  if (!filter || strstr("gr_lock_is_is_compatible", filter))
  {
    extern void __test__gr_lock_is_is_compatible(void);
    i_log_info("========================= TEST CASE: %s\n", "gr_lock_is_is_compatible");
    int prev = test_ret;
    test_ret = 0;
    __test__gr_lock_is_is_compatible();
    if (!test_ret)
    {
      i_log_passed("%s\n", "gr_lock_is_is_compatible");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "gr_lock_is_is_compatible";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:633 DONE

  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:689 START
  if (!filter || strstr("gr_lock_high_pressure_random", filter))
  {
    extern void __test__gr_lock_high_pressure_random(void);
    i_log_info("========================= TEST CASE: %s\n", "gr_lock_high_pressure_random");
    int prev = test_ret;
    test_ret = 0;
    __test__gr_lock_high_pressure_random();
    if (!test_ret)
    {
      i_log_passed("%s\n", "gr_lock_high_pressure_random");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "gr_lock_high_pressure_random";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:689 DONE

  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:879 START
  if (!filter || strstr("latch", filter))
  {
    extern void __test__latch(void);
    i_log_info("========================= TEST CASE: %s\n", "latch");
    int prev = test_ret;
    test_ret = 0;
    __test__latch();
    if (!test_ret)
    {
      i_log_passed("%s\n", "latch");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "latch";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/concurrency.c:879 DONE

  //////////////////// /Users/theo/Development/Numstore/src/file_pager.c:126 START
  if (!filter || strstr("fpgr_open", filter))
  {
    extern void __test__fpgr_open(void);
    i_log_info("========================= TEST CASE: %s\n", "fpgr_open");
    int prev = test_ret;
    test_ret = 0;
    __test__fpgr_open();
    if (!test_ret)
    {
      i_log_passed("%s\n", "fpgr_open");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "fpgr_open";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/file_pager.c:126 DONE

  //////////////////// /Users/theo/Development/Numstore/src/file_pager.c:235 START
  if (!filter || strstr("fpgr_new", filter))
  {
    extern void __test__fpgr_new(void);
    i_log_info("========================= TEST CASE: %s\n", "fpgr_new");
    int prev = test_ret;
    test_ret = 0;
    __test__fpgr_new();
    if (!test_ret)
    {
      i_log_passed("%s\n", "fpgr_new");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "fpgr_new";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/file_pager.c:235 DONE

  //////////////////// /Users/theo/Development/Numstore/src/file_pager.c:414 START
  if (!filter || strstr("fpgr_read_write", filter))
  {
    extern void __test__fpgr_read_write(void);
    i_log_info("========================= TEST CASE: %s\n", "fpgr_read_write");
    int prev = test_ret;
    test_ret = 0;
    __test__fpgr_read_write();
    if (!test_ret)
    {
      i_log_passed("%s\n", "fpgr_read_write");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "fpgr_read_write";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/file_pager.c:414 DONE

  //////////////////// /Users/theo/Development/Numstore/src/htable.c:191 START
  if (!filter || strstr("htable", filter))
  {
    extern void __test__htable(void);
    i_log_info("========================= TEST CASE: %s\n", "htable");
    int prev = test_ret;
    test_ret = 0;
    __test__htable();
    if (!test_ret)
    {
      i_log_passed("%s\n", "htable");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "htable";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/htable.c:191 DONE

  //////////////////// /Users/theo/Development/Numstore/src/htable.c:266 START
  if (!filter || strstr("fnv1a_hash_empty", filter))
  {
    extern void __test__fnv1a_hash_empty(void);
    i_log_info("========================= TEST CASE: %s\n", "fnv1a_hash_empty");
    int prev = test_ret;
    test_ret = 0;
    __test__fnv1a_hash_empty();
    if (!test_ret)
    {
      i_log_passed("%s\n", "fnv1a_hash_empty");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "fnv1a_hash_empty";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/htable.c:266 DONE

  //////////////////// /Users/theo/Development/Numstore/src/htable.c:274 START
  if (!filter || strstr("fnv1a_hash_single_char", filter))
  {
    extern void __test__fnv1a_hash_single_char(void);
    i_log_info("========================= TEST CASE: %s\n", "fnv1a_hash_single_char");
    int prev = test_ret;
    test_ret = 0;
    __test__fnv1a_hash_single_char();
    if (!test_ret)
    {
      i_log_passed("%s\n", "fnv1a_hash_single_char");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "fnv1a_hash_single_char";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/htable.c:274 DONE

  //////////////////// /Users/theo/Development/Numstore/src/htable.c:283 START
  if (!filter || strstr("fnv1a_hash_known_value", filter))
  {
    extern void __test__fnv1a_hash_known_value(void);
    i_log_info("========================= TEST CASE: %s\n", "fnv1a_hash_known_value");
    int prev = test_ret;
    test_ret = 0;
    __test__fnv1a_hash_known_value();
    if (!test_ret)
    {
      i_log_passed("%s\n", "fnv1a_hash_known_value");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "fnv1a_hash_known_value";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/htable.c:283 DONE

  //////////////////// /Users/theo/Development/Numstore/src/htable.c:291 START
  if (!filter || strstr("fnv1a_hash_deterministic", filter))
  {
    extern void __test__fnv1a_hash_deterministic(void);
    i_log_info("========================= TEST CASE: %s\n", "fnv1a_hash_deterministic");
    int prev = test_ret;
    test_ret = 0;
    __test__fnv1a_hash_deterministic();
    if (!test_ret)
    {
      i_log_passed("%s\n", "fnv1a_hash_deterministic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "fnv1a_hash_deterministic";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/htable.c:291 DONE

  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:227 START
  if (!filter || strstr("nupd_init", filter))
  {
    extern void __test__nupd_init(void);
    i_log_info("========================= TEST CASE: %s\n", "nupd_init");
    int prev = test_ret;
    test_ret = 0;
    __test__nupd_init();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nupd_init");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_init";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:227 DONE

  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:300 START
  if (!filter || strstr("nupd_append_right", filter))
  {
    extern void __test__nupd_append_right(void);
    i_log_info("========================= TEST CASE: %s\n", "nupd_append_right");
    int prev = test_ret;
    test_ret = 0;
    __test__nupd_append_right();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nupd_append_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_append_right";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:300 DONE

  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:416 START
  if (!filter || strstr("nupd_append_left", filter))
  {
    extern void __test__nupd_append_left(void);
    i_log_info("========================= TEST CASE: %s\n", "nupd_append_left");
    int prev = test_ret;
    test_ret = 0;
    __test__nupd_append_left();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nupd_append_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_append_left";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:416 DONE

  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:666 START
  if (!filter || strstr("nupd_append_tip_right", filter))
  {
    extern void __test__nupd_append_tip_right(void);
    i_log_info("========================= TEST CASE: %s\n", "nupd_append_tip_right");
    int prev = test_ret;
    test_ret = 0;
    __test__nupd_append_tip_right();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nupd_append_tip_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_append_tip_right";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:666 DONE

  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:866 START
  if (!filter || strstr("nupd_append_tip_left", filter))
  {
    extern void __test__nupd_append_tip_left(void);
    i_log_info("========================= TEST CASE: %s\n", "nupd_append_tip_left");
    int prev = test_ret;
    test_ret = 0;
    __test__nupd_append_tip_left();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nupd_append_tip_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_append_tip_left";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:866 DONE

  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1213 START
  if (!filter || strstr("nupd_consume_right", filter))
  {
    extern void __test__nupd_consume_right(void);
    i_log_info("========================= TEST CASE: %s\n", "nupd_consume_right");
    int prev = test_ret;
    test_ret = 0;
    __test__nupd_consume_right();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nupd_consume_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_consume_right";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1213 DONE

  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1287 START
  if (!filter || strstr("nupd_consume_left", filter))
  {
    extern void __test__nupd_consume_left(void);
    i_log_info("========================= TEST CASE: %s\n", "nupd_consume_left");
    int prev = test_ret;
    test_ret = 0;
    __test__nupd_consume_left();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nupd_consume_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_consume_left";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1287 DONE

  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1357 START
  if (!filter || strstr("nupd_done_observing_left", filter))
  {
    extern void __test__nupd_done_observing_left(void);
    i_log_info("========================= TEST CASE: %s\n", "nupd_done_observing_left");
    int prev = test_ret;
    test_ret = 0;
    __test__nupd_done_observing_left();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nupd_done_observing_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_done_observing_left";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1357 DONE

  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1412 START
  if (!filter || strstr("nupd_done_observing_right", filter))
  {
    extern void __test__nupd_done_observing_right(void);
    i_log_info("========================= TEST CASE: %s\n", "nupd_done_observing_right");
    int prev = test_ret;
    test_ret = 0;
    __test__nupd_done_observing_right();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nupd_done_observing_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_done_observing_right";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1412 DONE

  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1455 START
  if (!filter || strstr("nupd_done_consuming_left", filter))
  {
    extern void __test__nupd_done_consuming_left(void);
    i_log_info("========================= TEST CASE: %s\n", "nupd_done_consuming_left");
    int prev = test_ret;
    test_ret = 0;
    __test__nupd_done_consuming_left();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nupd_done_consuming_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_done_consuming_left";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1455 DONE

  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1499 START
  if (!filter || strstr("nupd_done_consuming_right", filter))
  {
    extern void __test__nupd_done_consuming_right(void);
    i_log_info("========================= TEST CASE: %s\n", "nupd_done_consuming_right");
    int prev = test_ret;
    test_ret = 0;
    __test__nupd_done_consuming_right();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nupd_done_consuming_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_done_consuming_right";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1499 DONE

  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1543 START
  if (!filter || strstr("nupd_done_left", filter))
  {
    extern void __test__nupd_done_left(void);
    i_log_info("========================= TEST CASE: %s\n", "nupd_done_left");
    int prev = test_ret;
    test_ret = 0;
    __test__nupd_done_left();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nupd_done_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_done_left";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1543 DONE

  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1597 START
  if (!filter || strstr("nupd_done_right", filter))
  {
    extern void __test__nupd_done_right(void);
    i_log_info("========================= TEST CASE: %s\n", "nupd_done_right");
    int prev = test_ret;
    test_ret = 0;
    __test__nupd_done_right();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nupd_done_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_done_right";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/node_updates.c:1597 DONE

  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:117 START
  if (!filter || strstr("checksum_execute_simple", filter))
  {
    extern void __test__checksum_execute_simple(void);
    i_log_info("========================= TEST CASE: %s\n", "checksum_execute_simple");
    int prev = test_ret;
    test_ret = 0;
    __test__checksum_execute_simple();
    if (!test_ret)
    {
      i_log_passed("%s\n", "checksum_execute_simple");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "checksum_execute_simple";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:117 DONE

  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:128 START
  if (!filter || strstr("checksum_execute_deterministic", filter))
  {
    extern void __test__checksum_execute_deterministic(void);
    i_log_info("========================= TEST CASE: %s\n", "checksum_execute_deterministic");
    int prev = test_ret;
    test_ret = 0;
    __test__checksum_execute_deterministic();
    if (!test_ret)
    {
      i_log_passed("%s\n", "checksum_execute_deterministic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "checksum_execute_deterministic";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:128 DONE

  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:140 START
  if (!filter || strstr("checksum_execute_incremental", filter))
  {
    extern void __test__checksum_execute_incremental(void);
    i_log_info("========================= TEST CASE: %s\n", "checksum_execute_incremental");
    int prev = test_ret;
    test_ret = 0;
    __test__checksum_execute_incremental();
    if (!test_ret)
    {
      i_log_passed("%s\n", "checksum_execute_incremental");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "checksum_execute_incremental";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:140 DONE

  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:172 START
  if (!filter || strstr("randu32", filter))
  {
    extern void __test__randu32(void);
    i_log_info("========================= TEST CASE: %s\n", "randu32");
    int prev = test_ret;
    test_ret = 0;
    __test__randu32();
    if (!test_ret)
    {
      i_log_passed("%s\n", "randu32");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "randu32";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:172 DONE

  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:196 START
  if (!filter || strstr("randu32r", filter))
  {
    extern void __test__randu32r(void);
    i_log_info("========================= TEST CASE: %s\n", "randu32r");
    int prev = test_ret;
    test_ret = 0;
    __test__randu32r();
    if (!test_ret)
    {
      i_log_passed("%s\n", "randu32r");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "randu32r";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:196 DONE

  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:258 START
  if (!filter || strstr("randi32r", filter))
  {
    extern void __test__randi32r(void);
    i_log_info("========================= TEST CASE: %s\n", "randi32r");
    int prev = test_ret;
    test_ret = 0;
    __test__randi32r();
    if (!test_ret)
    {
      i_log_passed("%s\n", "randi32r");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "randi32r";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:258 DONE

  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:390 START
  if (!filter || strstr("randu64r", filter))
  {
    extern void __test__randu64r(void);
    i_log_info("========================= TEST CASE: %s\n", "randu64r");
    int prev = test_ret;
    test_ret = 0;
    __test__randu64r();
    if (!test_ret)
    {
      i_log_passed("%s\n", "randu64r");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "randu64r";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:390 DONE

  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:439 START
  if (!filter || strstr("randu64e", filter))
  {
    extern void __test__randu64e(void);
    i_log_info("========================= TEST CASE: %s\n", "randu64e");
    int prev = test_ret;
    test_ret = 0;
    __test__randu64e();
    if (!test_ret)
    {
      i_log_passed("%s\n", "randu64e");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "randu64e";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:439 DONE

  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:501 START
  if (!filter || strstr("randi64r", filter))
  {
    extern void __test__randi64r(void);
    i_log_info("========================= TEST CASE: %s\n", "randi64r");
    int prev = test_ret;
    test_ret = 0;
    __test__randi64r();
    if (!test_ret)
    {
      i_log_passed("%s\n", "randi64r");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "randi64r";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:501 DONE

  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:523 START
  if (!filter || strstr("randi64e", filter))
  {
    extern void __test__randi64e(void);
    i_log_info("========================= TEST CASE: %s\n", "randi64e");
    int prev = test_ret;
    test_ret = 0;
    __test__randi64e();
    if (!test_ret)
    {
      i_log_passed("%s\n", "randi64e");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "randi64e";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:523 DONE

  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:562 START
  if (!filter || strstr("randf", filter))
  {
    extern void __test__randf(void);
    i_log_info("========================= TEST CASE: %s\n", "randf");
    int prev = test_ret;
    test_ret = 0;
    __test__randf();
    if (!test_ret)
    {
      i_log_passed("%s\n", "randf");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "randf";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:562 DONE

  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:692 START
  if (!filter || strstr("parse_i32_expect", filter))
  {
    extern void __test__parse_i32_expect(void);
    i_log_info("========================= TEST CASE: %s\n", "parse_i32_expect");
    int prev = test_ret;
    test_ret = 0;
    __test__parse_i32_expect();
    if (!test_ret)
    {
      i_log_passed("%s\n", "parse_i32_expect");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "parse_i32_expect";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:692 DONE

  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:843 START
  if (!filter || strstr("parse_f32_expect", filter))
  {
    extern void __test__parse_f32_expect(void);
    i_log_info("========================= TEST CASE: %s\n", "parse_f32_expect");
    int prev = test_ret;
    test_ret = 0;
    __test__parse_f32_expect();
    if (!test_ret)
    {
      i_log_passed("%s\n", "parse_f32_expect");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "parse_f32_expect";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:843 DONE

  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:883 START
  if (!filter || strstr("py_mod_f32", filter))
  {
    extern void __test__py_mod_f32(void);
    i_log_info("========================= TEST CASE: %s\n", "py_mod_f32");
    int prev = test_ret;
    test_ret = 0;
    __test__py_mod_f32();
    if (!test_ret)
    {
      i_log_passed("%s\n", "py_mod_f32");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "py_mod_f32";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:883 DONE

  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:920 START
  if (!filter || strstr("py_mod_i32", filter))
  {
    extern void __test__py_mod_i32(void);
    i_log_info("========================= TEST CASE: %s\n", "py_mod_i32");
    int prev = test_ret;
    test_ret = 0;
    __test__py_mod_i32();
    if (!test_ret)
    {
      i_log_passed("%s\n", "py_mod_i32");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "py_mod_i32";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/numerics.c:920 DONE

  //////////////////// /Users/theo/Development/Numstore/src/os_common.c:76 START
  if (!filter || strstr("i_malloc_injection", filter))
  {
    extern void __test__i_malloc_injection(void);
    i_log_info("========================= TEST CASE: %s\n", "i_malloc_injection");
    int prev = test_ret;
    test_ret = 0;
    __test__i_malloc_injection();
    if (!test_ret)
    {
      i_log_passed("%s\n", "i_malloc_injection");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_malloc_injection";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/os_common.c:76 DONE

  //////////////////// /Users/theo/Development/Numstore/src/os_common.c:170 START
  if (!filter || strstr("i_realloc_basic", filter))
  {
    extern void __test__i_realloc_basic(void);
    i_log_info("========================= TEST CASE: %s\n", "i_realloc_basic");
    int prev = test_ret;
    test_ret = 0;
    __test__i_realloc_basic();
    if (!test_ret)
    {
      i_log_passed("%s\n", "i_realloc_basic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_realloc_basic";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/os_common.c:170 DONE

  //////////////////// /Users/theo/Development/Numstore/src/os_common.c:211 START
  if (!filter || strstr("i_realloc_right", filter))
  {
    extern void __test__i_realloc_right(void);
    i_log_info("========================= TEST CASE: %s\n", "i_realloc_right");
    int prev = test_ret;
    test_ret = 0;
    __test__i_realloc_right();
    if (!test_ret)
    {
      i_log_passed("%s\n", "i_realloc_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_realloc_right";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/os_common.c:211 DONE

  //////////////////// /Users/theo/Development/Numstore/src/os_common.c:310 START
  if (!filter || strstr("i_realloc_left", filter))
  {
    extern void __test__i_realloc_left(void);
    i_log_info("========================= TEST CASE: %s\n", "i_realloc_left");
    int prev = test_ret;
    test_ret = 0;
    __test__i_realloc_left();
    if (!test_ret)
    {
      i_log_passed("%s\n", "i_realloc_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_realloc_left";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/os_common.c:310 DONE

  //////////////////// /Users/theo/Development/Numstore/src/os_common.c:387 START
  if (!filter || strstr("i_crealloc_right", filter))
  {
    extern void __test__i_crealloc_right(void);
    i_log_info("========================= TEST CASE: %s\n", "i_crealloc_right");
    int prev = test_ret;
    test_ret = 0;
    __test__i_crealloc_right();
    if (!test_ret)
    {
      i_log_passed("%s\n", "i_crealloc_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_crealloc_right";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/os_common.c:387 DONE

  //////////////////// /Users/theo/Development/Numstore/src/os_common.c:493 START
  if (!filter || strstr("i_crealloc_left", filter))
  {
    extern void __test__i_crealloc_left(void);
    i_log_info("========================= TEST CASE: %s\n", "i_crealloc_left");
    int prev = test_ret;
    test_ret = 0;
    __test__i_crealloc_left();
    if (!test_ret)
    {
      i_log_passed("%s\n", "i_crealloc_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_crealloc_left";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/os_common.c:493 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:150 START
  if (!filter || strstr("page_set_get_simple", filter))
  {
    extern void __test__page_set_get_simple(void);
    i_log_info("========================= TEST CASE: %s\n", "page_set_get_simple");
    int prev = test_ret;
    test_ret = 0;
    __test__page_set_get_simple();
    if (!test_ret)
    {
      i_log_passed("%s\n", "page_set_get_simple");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "page_set_get_simple";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:150 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:230 START
  if (!filter || strstr("i_log_page", filter))
  {
    extern void __test__i_log_page(void);
    i_log_info("========================= TEST CASE: %s\n", "i_log_page");
    int prev = test_ret;
    test_ret = 0;
    __test__i_log_page();
    if (!test_ret)
    {
      i_log_passed("%s\n", "i_log_page");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_log_page";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:230 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:306 START
  if (!filter || strstr("i_log_fsm", filter))
  {
    extern void __test__i_log_fsm(void);
    i_log_info("========================= TEST CASE: %s\n", "i_log_fsm");
    int prev = test_ret;
    test_ret = 0;
    __test__i_log_fsm();
    if (!test_ret)
    {
      i_log_passed("%s\n", "i_log_fsm");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_log_fsm";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:306 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:387 START
  if (!filter || strstr("dl_validate", filter))
  {
    extern void __test__dl_validate(void);
    i_log_info("========================= TEST CASE: %s\n", "dl_validate");
    int prev = test_ret;
    test_ret = 0;
    __test__dl_validate();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dl_validate");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_validate";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:387 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:469 START
  if (!filter || strstr("dl_set_get", filter))
  {
    extern void __test__dl_set_get(void);
    i_log_info("========================= TEST CASE: %s\n", "dl_set_get");
    int prev = test_ret;
    test_ret = 0;
    __test__dl_set_get();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dl_set_get");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_set_get";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:469 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:532 START
  if (!filter || strstr("dl_read", filter))
  {
    extern void __test__dl_read(void);
    i_log_info("========================= TEST CASE: %s\n", "dl_read");
    int prev = test_ret;
    test_ret = 0;
    __test__dl_read();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dl_read");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_read";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:532 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:660 START
  if (!filter || strstr("dl_read_out_from", filter))
  {
    extern void __test__dl_read_out_from(void);
    i_log_info("========================= TEST CASE: %s\n", "dl_read_out_from");
    int prev = test_ret;
    test_ret = 0;
    __test__dl_read_out_from();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dl_read_out_from");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_read_out_from";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:660 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:846 START
  if (!filter || strstr("dl_append", filter))
  {
    extern void __test__dl_append(void);
    i_log_info("========================= TEST CASE: %s\n", "dl_append");
    int prev = test_ret;
    test_ret = 0;
    __test__dl_append();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dl_append");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_append";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:846 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:944 START
  if (!filter || strstr("dl_write", filter))
  {
    extern void __test__dl_write(void);
    i_log_info("========================= TEST CASE: %s\n", "dl_write");
    int prev = test_ret;
    test_ret = 0;
    __test__dl_write();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dl_write");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_write";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:944 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:1024 START
  if (!filter || strstr("dl_memset", filter))
  {
    extern void __test__dl_memset(void);
    i_log_info("========================= TEST CASE: %s\n", "dl_memset");
    int prev = test_ret;
    test_ret = 0;
    __test__dl_memset();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dl_memset");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_memset";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:1024 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:1101 START
  if (!filter || strstr("dl_move_left", filter))
  {
    extern void __test__dl_move_left(void);
    i_log_info("========================= TEST CASE: %s\n", "dl_move_left");
    int prev = test_ret;
    test_ret = 0;
    __test__dl_move_left();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dl_move_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_move_left";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:1101 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:1181 START
  if (!filter || strstr("dl_shift_right", filter))
  {
    extern void __test__dl_shift_right(void);
    i_log_info("========================= TEST CASE: %s\n", "dl_shift_right");
    int prev = test_ret;
    test_ret = 0;
    __test__dl_shift_right();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dl_shift_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_shift_right";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:1181 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:1271 START
  if (!filter || strstr("dl_move_right", filter))
  {
    extern void __test__dl_move_right(void);
    i_log_info("========================= TEST CASE: %s\n", "dl_move_right");
    int prev = test_ret;
    test_ret = 0;
    __test__dl_move_right();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dl_move_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_move_right";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:1271 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:1381 START
  if (!filter || strstr("i_log_dl", filter))
  {
    extern void __test__i_log_dl(void);
    i_log_info("========================= TEST CASE: %s\n", "i_log_dl");
    int prev = test_ret;
    test_ret = 0;
    __test__i_log_dl();
    if (!test_ret)
    {
      i_log_passed("%s\n", "i_log_dl");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_log_dl";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:1381 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:1414 START
  if (!filter || strstr("dl_make_valid", filter))
  {
    extern void __test__dl_make_valid(void);
    i_log_info("========================= TEST CASE: %s\n", "dl_make_valid");
    int prev = test_ret;
    test_ret = 0;
    __test__dl_make_valid();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dl_make_valid");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_make_valid";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:1414 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:1539 START
  if (!filter || strstr("in_validate_for_db", filter))
  {
    extern void __test__in_validate_for_db(void);
    i_log_info("========================= TEST CASE: %s\n", "in_validate_for_db");
    int prev = test_ret;
    test_ret = 0;
    __test__in_validate_for_db();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_validate_for_db");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_validate_for_db";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:1539 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:1600 START
  if (!filter || strstr("in_set_get_simple", filter))
  {
    extern void __test__in_set_get_simple(void);
    i_log_info("========================= TEST CASE: %s\n", "in_set_get_simple");
    int prev = test_ret;
    test_ret = 0;
    __test__in_set_get_simple();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_set_get_simple");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_set_get_simple";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:1600 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:1658 START
  if (!filter || strstr("in_push_end", filter))
  {
    extern void __test__in_push_end(void);
    i_log_info("========================= TEST CASE: %s\n", "in_push_end");
    int prev = test_ret;
    test_ret = 0;
    __test__in_push_end();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_push_end");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_push_end";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:1658 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:1723 START
  if (!filter || strstr("in_memcpy", filter))
  {
    extern void __test__in_memcpy(void);
    i_log_info("========================= TEST CASE: %s\n", "in_memcpy");
    int prev = test_ret;
    test_ret = 0;
    __test__in_memcpy();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_memcpy");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_memcpy";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:1723 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:1854 START
  if (!filter || strstr("in_move_left", filter))
  {
    extern void __test__in_move_left(void);
    i_log_info("========================= TEST CASE: %s\n", "in_move_left");
    int prev = test_ret;
    test_ret = 0;
    __test__in_move_left();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_move_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_move_left";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:1854 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:1891 START
  if (!filter || strstr("in_move_left_two_keys", filter))
  {
    extern void __test__in_move_left_two_keys(void);
    i_log_info("========================= TEST CASE: %s\n", "in_move_left_two_keys");
    int prev = test_ret;
    test_ret = 0;
    __test__in_move_left_two_keys();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_move_left_two_keys");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_move_left_two_keys";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:1891 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:1919 START
  if (!filter || strstr("in_move_left_all_keys", filter))
  {
    extern void __test__in_move_left_all_keys(void);
    i_log_info("========================= TEST CASE: %s\n", "in_move_left_all_keys");
    int prev = test_ret;
    test_ret = 0;
    __test__in_move_left_all_keys();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_move_left_all_keys");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_move_left_all_keys";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:1919 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:1942 START
  if (!filter || strstr("in_move_left_into_empty", filter))
  {
    extern void __test__in_move_left_into_empty(void);
    i_log_info("========================= TEST CASE: %s\n", "in_move_left_into_empty");
    int prev = test_ret;
    test_ret = 0;
    __test__in_move_left_into_empty();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_move_left_into_empty");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_move_left_into_empty";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:1942 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2001 START
  if (!filter || strstr("in_push_left", filter))
  {
    extern void __test__in_push_left(void);
    i_log_info("========================= TEST CASE: %s\n", "in_push_left");
    int prev = test_ret;
    test_ret = 0;
    __test__in_push_left();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_push_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_push_left";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2001 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2032 START
  if (!filter || strstr("in_push_left_into_empty", filter))
  {
    extern void __test__in_push_left_into_empty(void);
    i_log_info("========================= TEST CASE: %s\n", "in_push_left_into_empty");
    int prev = test_ret;
    test_ret = 0;
    __test__in_push_left_into_empty();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_push_left_into_empty");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_push_left_into_empty";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2032 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2045 START
  if (!filter || strstr("in_push_left_to_full", filter))
  {
    extern void __test__in_push_left_to_full(void);
    i_log_info("========================= TEST CASE: %s\n", "in_push_left_to_full");
    int prev = test_ret;
    test_ret = 0;
    __test__in_push_left_to_full();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_push_left_to_full");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_push_left_to_full";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2045 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2105 START
  if (!filter || strstr("in_move_right", filter))
  {
    extern void __test__in_move_right(void);
    i_log_info("========================= TEST CASE: %s\n", "in_move_right");
    int prev = test_ret;
    test_ret = 0;
    __test__in_move_right();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_move_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_move_right";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2105 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2142 START
  if (!filter || strstr("in_move_right_two_keys", filter))
  {
    extern void __test__in_move_right_two_keys(void);
    i_log_info("========================= TEST CASE: %s\n", "in_move_right_two_keys");
    int prev = test_ret;
    test_ret = 0;
    __test__in_move_right_two_keys();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_move_right_two_keys");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_move_right_two_keys";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2142 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2170 START
  if (!filter || strstr("in_move_right_all_keys", filter))
  {
    extern void __test__in_move_right_all_keys(void);
    i_log_info("========================= TEST CASE: %s\n", "in_move_right_all_keys");
    int prev = test_ret;
    test_ret = 0;
    __test__in_move_right_all_keys();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_move_right_all_keys");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_move_right_all_keys";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2170 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2193 START
  if (!filter || strstr("in_move_right_into_empty_right", filter))
  {
    extern void __test__in_move_right_into_empty_right(void);
    i_log_info("========================= TEST CASE: %s\n", "in_move_right_into_empty_right");
    int prev = test_ret;
    test_ret = 0;
    __test__in_move_right_into_empty_right();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_move_right_into_empty_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_move_right_into_empty_right";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2193 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2250 START
  if (!filter || strstr("in_choose_lidx", filter))
  {
    extern void __test__in_choose_lidx(void);
    i_log_info("========================= TEST CASE: %s\n", "in_choose_lidx");
    int prev = test_ret;
    test_ret = 0;
    __test__in_choose_lidx();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_choose_lidx");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_choose_lidx";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2250 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2348 START
  if (!filter || strstr("in_cut_left", filter))
  {
    extern void __test__in_cut_left(void);
    i_log_info("========================= TEST CASE: %s\n", "in_cut_left");
    int prev = test_ret;
    test_ret = 0;
    __test__in_cut_left();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_cut_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_cut_left";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2348 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2386 START
  if (!filter || strstr("in_cut_left_all_at_once", filter))
  {
    extern void __test__in_cut_left_all_at_once(void);
    i_log_info("========================= TEST CASE: %s\n", "in_cut_left_all_at_once");
    int prev = test_ret;
    test_ret = 0;
    __test__in_cut_left_all_at_once();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_cut_left_all_at_once");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_cut_left_all_at_once";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2386 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2404 START
  if (!filter || strstr("in_cut_left_from_empty", filter))
  {
    extern void __test__in_cut_left_from_empty(void);
    i_log_info("========================= TEST CASE: %s\n", "in_cut_left_from_empty");
    int prev = test_ret;
    test_ret = 0;
    __test__in_cut_left_from_empty();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_cut_left_from_empty");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_cut_left_from_empty";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2404 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2417 START
  if (!filter || strstr("in_cut_left_to_one", filter))
  {
    extern void __test__in_cut_left_to_one(void);
    i_log_info("========================= TEST CASE: %s\n", "in_cut_left_to_one");
    int prev = test_ret;
    test_ret = 0;
    __test__in_cut_left_to_one();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_cut_left_to_one");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_cut_left_to_one";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2417 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2489 START
  if (!filter || strstr("i_log_in", filter))
  {
    extern void __test__i_log_in(void);
    i_log_info("========================= TEST CASE: %s\n", "i_log_in");
    int prev = test_ret;
    test_ret = 0;
    __test__i_log_in();
    if (!test_ret)
    {
      i_log_passed("%s\n", "i_log_in");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_log_in";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2489 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2567 START
  if (!filter || strstr("i_log_vh", filter))
  {
    extern void __test__i_log_vh(void);
    i_log_info("========================= TEST CASE: %s\n", "i_log_vh");
    int prev = test_ret;
    test_ret = 0;
    __test__i_log_vh();
    if (!test_ret)
    {
      i_log_passed("%s\n", "i_log_vh");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_log_vh";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2567 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2606 START
  if (!filter || strstr("vp_init_empty", filter))
  {
    extern void __test__vp_init_empty(void);
    i_log_info("========================= TEST CASE: %s\n", "vp_init_empty");
    int prev = test_ret;
    test_ret = 0;
    __test__vp_init_empty();
    if (!test_ret)
    {
      i_log_passed("%s\n", "vp_init_empty");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "vp_init_empty";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2606 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2764 START
  if (!filter || strstr("vp_validate", filter))
  {
    extern void __test__vp_validate(void);
    i_log_info("========================= TEST CASE: %s\n", "vp_validate");
    int prev = test_ret;
    test_ret = 0;
    __test__vp_validate();
    if (!test_ret)
    {
      i_log_passed("%s\n", "vp_validate");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "vp_validate";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2764 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2862 START
  if (!filter || strstr("i_log_vp", filter))
  {
    extern void __test__i_log_vp(void);
    i_log_info("========================= TEST CASE: %s\n", "i_log_vp");
    int prev = test_ret;
    test_ret = 0;
    __test__i_log_vp();
    if (!test_ret)
    {
      i_log_passed("%s\n", "i_log_vp");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_log_vp";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2862 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2895 START
  if (!filter || strstr("vt_init_empty", filter))
  {
    extern void __test__vt_init_empty(void);
    i_log_info("========================= TEST CASE: %s\n", "vt_init_empty");
    int prev = test_ret;
    test_ret = 0;
    __test__vt_init_empty();
    if (!test_ret)
    {
      i_log_passed("%s\n", "vt_init_empty");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "vt_init_empty";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2895 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2920 START
  if (!filter || strstr("vt_validate", filter))
  {
    extern void __test__vt_validate(void);
    i_log_info("========================= TEST CASE: %s\n", "vt_validate");
    int prev = test_ret;
    test_ret = 0;
    __test__vt_validate();
    if (!test_ret)
    {
      i_log_passed("%s\n", "vt_validate");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "vt_validate";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2920 DONE

  //////////////////// /Users/theo/Development/Numstore/src/page.c:2955 START
  if (!filter || strstr("i_log_vt", filter))
  {
    extern void __test__i_log_vt(void);
    i_log_info("========================= TEST CASE: %s\n", "i_log_vt");
    int prev = test_ret;
    test_ret = 0;
    __test__i_log_vt();
    if (!test_ret)
    {
      i_log_passed("%s\n", "i_log_vt");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_log_vt";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/page.c:2955 DONE

  //////////////////// /Users/theo/Development/Numstore/src/pager.c:224 START
  if (!filter || strstr("pager_fill_ht", filter))
  {
    extern void __test__pager_fill_ht(void);
    i_log_info("========================= TEST CASE: %s\n", "pager_fill_ht");
    int prev = test_ret;
    test_ret = 0;
    __test__pager_fill_ht();
    if (!test_ret)
    {
      i_log_passed("%s\n", "pager_fill_ht");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "pager_fill_ht";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/pager.c:224 DONE

  //////////////////// /Users/theo/Development/Numstore/src/pager.c:278 START
  if (!filter || strstr("wal_int", filter))
  {
    extern void __test__wal_int(void);
    i_log_info("========================= TEST CASE: %s\n", "wal_int");
    int prev = test_ret;
    test_ret = 0;
    __test__wal_int();
    if (!test_ret)
    {
      i_log_passed("%s\n", "wal_int");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "wal_int";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/pager.c:278 DONE

  //////////////////// /Users/theo/Development/Numstore/src/pager.c:333 START
  if (!filter || strstr("i_log_page_table", filter))
  {
    extern void __test__i_log_page_table(void);
    i_log_info("========================= TEST CASE: %s\n", "i_log_page_table");
    int prev = test_ret;
    test_ret = 0;
    __test__i_log_page_table();
    if (!test_ret)
    {
      i_log_passed("%s\n", "i_log_page_table");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_log_page_table";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/pager.c:333 DONE

  //////////////////// /Users/theo/Development/Numstore/src/pager.c:1323 START
  if (!filter || strstr("pager_open", filter))
  {
    extern void __test__pager_open(void);
    i_log_info("========================= TEST CASE: %s\n", "pager_open");
    int prev = test_ret;
    test_ret = 0;
    __test__pager_open();
    if (!test_ret)
    {
      i_log_passed("%s\n", "pager_open");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "pager_open";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/pager.c:1323 DONE

  //////////////////// /Users/theo/Development/Numstore/src/pager.c:1365 START
  if (!filter || strstr("pgr_open_basic", filter))
  {
    extern void __test__pgr_open_basic(void);
    i_log_info("========================= TEST CASE: %s\n", "pgr_open_basic");
    int prev = test_ret;
    test_ret = 0;
    __test__pgr_open_basic();
    if (!test_ret)
    {
      i_log_passed("%s\n", "pgr_open_basic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "pgr_open_basic";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/pager.c:1365 DONE

  //////////////////// /Users/theo/Development/Numstore/src/pager.c:1459 START
  if (!filter || strstr("pgr_close_success", filter))
  {
    extern void __test__pgr_close_success(void);
    i_log_info("========================= TEST CASE: %s\n", "pgr_close_success");
    int prev = test_ret;
    test_ret = 0;
    __test__pgr_close_success();
    if (!test_ret)
    {
      i_log_passed("%s\n", "pgr_close_success");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "pgr_close_success";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/pager.c:1459 DONE

  //////////////////// /Users/theo/Development/Numstore/src/pager.c:1566 START
  if (!filter || strstr("pgr_delete", filter))
  {
    extern void __test__pgr_delete(void);
    i_log_info("========================= TEST CASE: %s\n", "pgr_delete");
    int prev = test_ret;
    test_ret = 0;
    __test__pgr_delete();
    if (!test_ret)
    {
      i_log_passed("%s\n", "pgr_delete");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "pgr_delete";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/pager.c:1566 DONE

  //////////////////// /Users/theo/Development/Numstore/src/pager.c:1966 START
  if (!filter || strstr("pgr_reserve_and_ctrl_lock_st", filter))
  {
    extern void __test__pgr_reserve_and_ctrl_lock_st(void);
    i_log_info("========================= TEST CASE: %s\n", "pgr_reserve_and_ctrl_lock_st");
    int prev = test_ret;
    test_ret = 0;
    __test__pgr_reserve_and_ctrl_lock_st();
    if (!test_ret)
    {
      i_log_passed("%s\n", "pgr_reserve_and_ctrl_lock_st");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "pgr_reserve_and_ctrl_lock_st";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/pager.c:1966 DONE

  //////////////////// /Users/theo/Development/Numstore/src/pager.c:2105 START
  if (!filter || strstr("pgr_get_invalid_checksum", filter))
  {
    extern void __test__pgr_get_invalid_checksum(void);
    i_log_info("========================= TEST CASE: %s\n", "pgr_get_invalid_checksum");
    int prev = test_ret;
    test_ret = 0;
    __test__pgr_get_invalid_checksum();
    if (!test_ret)
    {
      i_log_passed("%s\n", "pgr_get_invalid_checksum");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "pgr_get_invalid_checksum";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/pager.c:2105 DONE

  //////////////////// /Users/theo/Development/Numstore/src/pager.c:2678 START
  if (!filter || strstr("pgr_new_get_save", filter))
  {
    extern void __test__pgr_new_get_save(void);
    i_log_info("========================= TEST CASE: %s\n", "pgr_new_get_save");
    int prev = test_ret;
    test_ret = 0;
    __test__pgr_new_get_save();
    if (!test_ret)
    {
      i_log_passed("%s\n", "pgr_new_get_save");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "pgr_new_get_save";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/pager.c:2678 DONE

  //////////////////// /Users/theo/Development/Numstore/src/pager.c:2759 START
  if (!filter || strstr("pgr_checkpoint", filter))
  {
    extern void __test__pgr_checkpoint(void);
    i_log_info("========================= TEST CASE: %s\n", "pgr_checkpoint");
    int prev = test_ret;
    test_ret = 0;
    __test__pgr_checkpoint();
    if (!test_ret)
    {
      i_log_passed("%s\n", "pgr_checkpoint");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "pgr_checkpoint";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/pager.c:2759 DONE

  //////////////////// /Users/theo/Development/Numstore/src/pager.c:3036 START
  if (!filter || strstr("aries_rollback_basic", filter))
  {
    extern void __test__aries_rollback_basic(void);
    i_log_info("========================= TEST CASE: %s\n", "aries_rollback_basic");
    int prev = test_ret;
    test_ret = 0;
    __test__aries_rollback_basic();
    if (!test_ret)
    {
      i_log_passed("%s\n", "aries_rollback_basic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "aries_rollback_basic";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/pager.c:3036 DONE

  //////////////////// /Users/theo/Development/Numstore/src/pager.c:3109 START
  if (!filter || strstr("aries_rollback_multiple_updates", filter))
  {
    extern void __test__aries_rollback_multiple_updates(void);
    i_log_info("========================= TEST CASE: %s\n", "aries_rollback_multiple_updates");
    int prev = test_ret;
    test_ret = 0;
    __test__aries_rollback_multiple_updates();
    if (!test_ret)
    {
      i_log_passed("%s\n", "aries_rollback_multiple_updates");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "aries_rollback_multiple_updates";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/pager.c:3109 DONE

  //////////////////// /Users/theo/Development/Numstore/src/pager.c:3188 START
  if (!filter || strstr("aries_rollback_with_crash_recovery", filter))
  {
    extern void __test__aries_rollback_with_crash_recovery(void);
    i_log_info("========================= TEST CASE: %s\n", "aries_rollback_with_crash_recovery");
    int prev = test_ret;
    test_ret = 0;
    __test__aries_rollback_with_crash_recovery();
    if (!test_ret)
    {
      i_log_passed("%s\n", "aries_rollback_with_crash_recovery");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "aries_rollback_with_crash_recovery";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/pager.c:3188 DONE

  //////////////////// /Users/theo/Development/Numstore/src/pager.c:3249 START
  if (!filter || strstr("aries_rollback_clr_not_undone", filter))
  {
    extern void __test__aries_rollback_clr_not_undone(void);
    i_log_info("========================= TEST CASE: %s\n", "aries_rollback_clr_not_undone");
    int prev = test_ret;
    test_ret = 0;
    __test__aries_rollback_clr_not_undone();
    if (!test_ret)
    {
      i_log_passed("%s\n", "aries_rollback_clr_not_undone");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "aries_rollback_clr_not_undone";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/pager.c:3249 DONE

  //////////////////// /Users/theo/Development/Numstore/src/parsers.c:166 START
  if (!filter || strstr("compile_user_stride", filter))
  {
    extern void __test__compile_user_stride(void);
    i_log_info("========================= TEST CASE: %s\n", "compile_user_stride");
    int prev = test_ret;
    test_ret = 0;
    __test__compile_user_stride();
    if (!test_ret)
    {
      i_log_passed("%s\n", "compile_user_stride");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "compile_user_stride";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/parsers.c:166 DONE

  //////////////////// /Users/theo/Development/Numstore/src/parsers.c:375 START
  if (!filter || strstr("compile_multi_user_stride", filter))
  {
    extern void __test__compile_multi_user_stride(void);
    i_log_info("========================= TEST CASE: %s\n", "compile_multi_user_stride");
    int prev = test_ret;
    test_ret = 0;
    __test__compile_multi_user_stride();
    if (!test_ret)
    {
      i_log_passed("%s\n", "compile_multi_user_stride");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "compile_multi_user_stride";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/parsers.c:375 DONE

  //////////////////// /Users/theo/Development/Numstore/src/parsers.c:1001 START
  if (!filter || strstr("compile_type", filter))
  {
    extern void __test__compile_type(void);
    i_log_info("========================= TEST CASE: %s\n", "compile_type");
    int prev = test_ret;
    test_ret = 0;
    __test__compile_type();
    if (!test_ret)
    {
      i_log_passed("%s\n", "compile_type");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "compile_type";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/parsers.c:1001 DONE

  //////////////////// /Users/theo/Development/Numstore/src/parsers.c:1663 START
  if (!filter || strstr("compile_query", filter))
  {
    extern void __test__compile_query(void);
    i_log_info("========================= TEST CASE: %s\n", "compile_query");
    int prev = test_ret;
    test_ret = 0;
    __test__compile_query();
    if (!test_ret)
    {
      i_log_passed("%s\n", "compile_query");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "compile_query";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/parsers.c:1663 DONE

  //////////////////// /Users/theo/Development/Numstore/src/parsers.c:2440 START
  if (!filter || strstr("compile_type_ref", filter))
  {
    extern void __test__compile_type_ref(void);
    i_log_info("========================= TEST CASE: %s\n", "compile_type_ref");
    int prev = test_ret;
    test_ret = 0;
    __test__compile_type_ref();
    if (!test_ret)
    {
      i_log_passed("%s\n", "compile_type_ref");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "compile_type_ref";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/parsers.c:2440 DONE

  //////////////////// /Users/theo/Development/Numstore/src/rope_algorithms.c:92 START
  if (!filter || strstr("dlgt_balance_with_prev", filter))
  {
    extern void __test__dlgt_balance_with_prev(void);
    i_log_info("========================= TEST CASE: %s\n", "dlgt_balance_with_prev");
    int prev = test_ret;
    test_ret = 0;
    __test__dlgt_balance_with_prev();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dlgt_balance_with_prev");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dlgt_balance_with_prev";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/rope_algorithms.c:92 DONE

  //////////////////// /Users/theo/Development/Numstore/src/rope_algorithms.c:272 START
  if (!filter || strstr("dlgt_balance_with_next", filter))
  {
    extern void __test__dlgt_balance_with_next(void);
    i_log_info("========================= TEST CASE: %s\n", "dlgt_balance_with_next");
    int prev = test_ret;
    test_ret = 0;
    __test__dlgt_balance_with_next();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dlgt_balance_with_next");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dlgt_balance_with_next";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/rope_algorithms.c:272 DONE

  //////////////////// /Users/theo/Development/Numstore/src/rope_algorithms.c:888 START
  if (!filter || strstr("ns_insert", filter))
  {
    extern void __test__ns_insert(void);
    i_log_info("========================= TEST CASE: %s\n", "ns_insert");
    int prev = test_ret;
    test_ret = 0;
    __test__ns_insert();
    if (!test_ret)
    {
      i_log_passed("%s\n", "ns_insert");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ns_insert";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/rope_algorithms.c:888 DONE

  //////////////////// /Users/theo/Development/Numstore/src/serial.c:85 START
  if (!filter || strstr("strings_all_unique", filter))
  {
    extern void __test__strings_all_unique(void);
    i_log_info("========================= TEST CASE: %s\n", "strings_all_unique");
    int prev = test_ret;
    test_ret = 0;
    __test__strings_all_unique();
    if (!test_ret)
    {
      i_log_passed("%s\n", "strings_all_unique");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "strings_all_unique";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/serial.c:85 DONE

  //////////////////// /Users/theo/Development/Numstore/src/serial.c:207 START
  if (!filter || strstr("string_contains", filter))
  {
    extern void __test__string_contains(void);
    i_log_info("========================= TEST CASE: %s\n", "string_contains");
    int prev = test_ret;
    test_ret = 0;
    __test__string_contains();
    if (!test_ret)
    {
      i_log_passed("%s\n", "string_contains");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "string_contains";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/serial.c:207 DONE

  //////////////////// /Users/theo/Development/Numstore/src/stride.c:163 START
  if (!filter || strstr("stride_resolve", filter))
  {
    extern void __test__stride_resolve(void);
    i_log_info("========================= TEST CASE: %s\n", "stride_resolve");
    int prev = test_ret;
    test_ret = 0;
    __test__stride_resolve();
    if (!test_ret)
    {
      i_log_passed("%s\n", "stride_resolve");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "stride_resolve";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/stride.c:163 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/aries_tests.c:21 START
  if (!filter || strstr("aries_crash", filter))
  {
    extern void __test__aries_crash(void);
    i_log_info("========================= TEST CASE: %s\n", "aries_crash");
    int prev = test_ret;
    test_ret = 0;
    __test__aries_crash();
    if (!test_ret)
    {
      i_log_passed("%s\n", "aries_crash");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "aries_crash";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/aries_tests.c:21 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:53 START
  if (!filter || strstr("f16_to_f32_normals_and_specials", filter))
  {
    extern void __test__f16_to_f32_normals_and_specials(void);
    i_log_info("========================= TEST CASE: %s\n", "f16_to_f32_normals_and_specials");
    int prev = test_ret;
    test_ret = 0;
    __test__f16_to_f32_normals_and_specials();
    if (!test_ret)
    {
      i_log_passed("%s\n", "f16_to_f32_normals_and_specials");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "f16_to_f32_normals_and_specials";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:53 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:72 START
  if (!filter || strstr("f16_to_f32_nan_is_nan", filter))
  {
    extern void __test__f16_to_f32_nan_is_nan(void);
    i_log_info("========================= TEST CASE: %s\n", "f16_to_f32_nan_is_nan");
    int prev = test_ret;
    test_ret = 0;
    __test__f16_to_f32_nan_is_nan();
    if (!test_ret)
    {
      i_log_passed("%s\n", "f16_to_f32_nan_is_nan");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "f16_to_f32_nan_is_nan";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:72 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:79 START
  if (!filter || strstr("f16_to_f32_smallest_subnormal_correct_value", filter))
  {
    extern void __test__f16_to_f32_smallest_subnormal_correct_value(void);
    i_log_info("========================= TEST CASE: %s\n", "f16_to_f32_smallest_subnormal_correct_value");
    int prev = test_ret;
    test_ret = 0;
    __test__f16_to_f32_smallest_subnormal_correct_value();
    if (!test_ret)
    {
      i_log_passed("%s\n", "f16_to_f32_smallest_subnormal_correct_value");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "f16_to_f32_smallest_subnormal_correct_value";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:79 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:111 START
  if (!filter || strstr("parse_i32_boundary_values", filter))
  {
    extern void __test__parse_i32_boundary_values(void);
    i_log_info("========================= TEST CASE: %s\n", "parse_i32_boundary_values");
    int prev = test_ret;
    test_ret = 0;
    __test__parse_i32_boundary_values();
    if (!test_ret)
    {
      i_log_passed("%s\n", "parse_i32_boundary_values");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "parse_i32_boundary_values";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:111 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:165 START
  if (!filter || strstr("parse_i64_boundary_values", filter))
  {
    extern void __test__parse_i64_boundary_values(void);
    i_log_info("========================= TEST CASE: %s\n", "parse_i64_boundary_values");
    int prev = test_ret;
    test_ret = 0;
    __test__parse_i64_boundary_values();
    if (!test_ret)
    {
      i_log_passed("%s\n", "parse_i64_boundary_values");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "parse_i64_boundary_values";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:165 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:200 START
  if (!filter || strstr("ext_array_capacity_doubles_on_growth", filter))
  {
    extern void __test__ext_array_capacity_doubles_on_growth(void);
    i_log_info("========================= TEST CASE: %s\n", "ext_array_capacity_doubles_on_growth");
    int prev = test_ret;
    test_ret = 0;
    __test__ext_array_capacity_doubles_on_growth();
    if (!test_ret)
    {
      i_log_passed("%s\n", "ext_array_capacity_doubles_on_growth");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ext_array_capacity_doubles_on_growth";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:200 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:231 START
  if (!filter || strstr("ext_array_remove_all_produces_empty", filter))
  {
    extern void __test__ext_array_remove_all_produces_empty(void);
    i_log_info("========================= TEST CASE: %s\n", "ext_array_remove_all_produces_empty");
    int prev = test_ret;
    test_ret = 0;
    __test__ext_array_remove_all_produces_empty();
    if (!test_ret)
    {
      i_log_passed("%s\n", "ext_array_remove_all_produces_empty");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ext_array_remove_all_produces_empty";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:231 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:270 START
  if (!filter || strstr("llist_append_maintaififo_order", filter))
  {
    extern void __test__llist_append_maintaififo_order(void);
    i_log_info("========================= TEST CASE: %s\n", "llist_append_maintaififo_order");
    int prev = test_ret;
    test_ret = 0;
    __test__llist_append_maintaififo_order();
    if (!test_ret)
    {
      i_log_passed("%s\n", "llist_append_maintaififo_order");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "llist_append_maintaififo_order";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:270 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:294 START
  if (!filter || strstr("llist_find_returnode_and_index", filter))
  {
    extern void __test__llist_find_returnode_and_index(void);
    i_log_info("========================= TEST CASE: %s\n", "llist_find_returnode_and_index");
    int prev = test_ret;
    test_ret = 0;
    __test__llist_find_returnode_and_index();
    if (!test_ret)
    {
      i_log_passed("%s\n", "llist_find_returnode_and_index");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "llist_find_returnode_and_index";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:294 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:328 START
  if (!filter || strstr("llist_remove_from_head_middle_tail", filter))
  {
    extern void __test__llist_remove_from_head_middle_tail(void);
    i_log_info("========================= TEST CASE: %s\n", "llist_remove_from_head_middle_tail");
    int prev = test_ret;
    test_ret = 0;
    __test__llist_remove_from_head_middle_tail();
    if (!test_ret)
    {
      i_log_passed("%s\n", "llist_remove_from_head_middle_tail");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "llist_remove_from_head_middle_tail";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:328 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:365 START
  if (!filter || strstr("llist_remove_absent_node_is_noop", filter))
  {
    extern void __test__llist_remove_absent_node_is_noop(void);
    i_log_info("========================= TEST CASE: %s\n", "llist_remove_absent_node_is_noop");
    int prev = test_ret;
    test_ret = 0;
    __test__llist_remove_absent_node_is_noop();
    if (!test_ret)
    {
      i_log_passed("%s\n", "llist_remove_absent_node_is_noop");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "llist_remove_absent_node_is_noop";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:365 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:387 START
  if (!filter || strstr("checksum_known_crc32c_vector", filter))
  {
    extern void __test__checksum_known_crc32c_vector(void);
    i_log_info("========================= TEST CASE: %s\n", "checksum_known_crc32c_vector");
    int prev = test_ret;
    test_ret = 0;
    __test__checksum_known_crc32c_vector();
    if (!test_ret)
    {
      i_log_passed("%s\n", "checksum_known_crc32c_vector");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "checksum_known_crc32c_vector";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:387 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:396 START
  if (!filter || strstr("checksum_distinct_bytes_differ", filter))
  {
    extern void __test__checksum_distinct_bytes_differ(void);
    i_log_info("========================= TEST CASE: %s\n", "checksum_distinct_bytes_differ");
    int prev = test_ret;
    test_ret = 0;
    __test__checksum_distinct_bytes_differ();
    if (!test_ret)
    {
      i_log_passed("%s\n", "checksum_distinct_bytes_differ");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "checksum_distinct_bytes_differ";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:396 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:409 START
  if (!filter || strstr("serializer_write_at_capacity_then_overflow", filter))
  {
    extern void __test__serializer_write_at_capacity_then_overflow(void);
    i_log_info("========================= TEST CASE: %s\n", "serializer_write_at_capacity_then_overflow");
    int prev = test_ret;
    test_ret = 0;
    __test__serializer_write_at_capacity_then_overflow();
    if (!test_ret)
    {
      i_log_passed("%s\n", "serializer_write_at_capacity_then_overflow");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "serializer_write_at_capacity_then_overflow";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:409 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:428 START
  if (!filter || strstr("serializer_incremental_write_overflow", filter))
  {
    extern void __test__serializer_incremental_write_overflow(void);
    i_log_info("========================= TEST CASE: %s\n", "serializer_incremental_write_overflow");
    int prev = test_ret;
    test_ret = 0;
    __test__serializer_incremental_write_overflow();
    if (!test_ret)
    {
      i_log_passed("%s\n", "serializer_incremental_write_overflow");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "serializer_incremental_write_overflow";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:428 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:447 START
  if (!filter || strstr("stride_constructors_resolve_correctly", filter))
  {
    extern void __test__stride_constructors_resolve_correctly(void);
    i_log_info("========================= TEST CASE: %s\n", "stride_constructors_resolve_correctly");
    int prev = test_ret;
    test_ret = 0;
    __test__stride_constructors_resolve_correctly();
    if (!test_ret)
    {
      i_log_passed("%s\n", "stride_constructors_resolve_correctly");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "stride_constructors_resolve_correctly";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:447 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:522 START
  if (!filter || strstr("string_ordering_operators", filter))
  {
    extern void __test__string_ordering_operators(void);
    i_log_info("========================= TEST CASE: %s\n", "string_ordering_operators");
    int prev = test_ret;
    test_ret = 0;
    __test__string_ordering_operators();
    if (!test_ret)
    {
      i_log_passed("%s\n", "string_ordering_operators");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "string_ordering_operators";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:522 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:563 START
  if (!filter || strstr("line_length_newline_found", filter))
  {
    extern void __test__line_length_newline_found(void);
    i_log_info("========================= TEST CASE: %s\n", "line_length_newline_found");
    int prev = test_ret;
    test_ret = 0;
    __test__line_length_newline_found();
    if (!test_ret)
    {
      i_log_passed("%s\n", "line_length_newline_found");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "line_length_newline_found";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:563 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:620 START
  if (!filter || strstr("string_equal_cases", filter))
  {
    extern void __test__string_equal_cases(void);
    i_log_info("========================= TEST CASE: %s\n", "string_equal_cases");
    int prev = test_ret;
    test_ret = 0;
    __test__string_equal_cases();
    if (!test_ret)
    {
      i_log_passed("%s\n", "string_equal_cases");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "string_equal_cases";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:620 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:658 START
  if (!filter || strstr("strings_are_disjoint_cases", filter))
  {
    extern void __test__strings_are_disjoint_cases(void);
    i_log_info("========================= TEST CASE: %s\n", "strings_are_disjoint_cases");
    int prev = test_ret;
    test_ret = 0;
    __test__strings_are_disjoint_cases();
    if (!test_ret)
    {
      i_log_passed("%s\n", "strings_are_disjoint_cases");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "strings_are_disjoint_cases";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:658 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:727 START
  if (!filter || strstr("cbuffer_discard_all_resets_state", filter))
  {
    extern void __test__cbuffer_discard_all_resets_state(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_discard_all_resets_state");
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_discard_all_resets_state();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_discard_all_resets_state");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_discard_all_resets_state";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:727 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:764 START
  if (!filter || strstr("cbuffer_read_write_wraparound", filter))
  {
    extern void __test__cbuffer_read_write_wraparound(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_read_write_wraparound");
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_read_write_wraparound();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_read_write_wraparound");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_read_write_wraparound";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:764 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:823 START
  if (!filter || strstr("cbuffer_cbuffer_move_transfers_bytes", filter))
  {
    extern void __test__cbuffer_cbuffer_move_transfers_bytes(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_cbuffer_move_transfers_bytes");
    int prev = test_ret;
    test_ret = 0;
    __test__cbuffer_cbuffer_move_transfers_bytes();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_cbuffer_move_transfers_bytes");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_cbuffer_move_transfers_bytes";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/core_extra_tests.c:823 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:19 START
  if (!filter || strstr("dpgt_open", filter))
  {
    extern void __test__dpgt_open(void);
    i_log_info("========================= TEST CASE: %s\n", "dpgt_open");
    int prev = test_ret;
    test_ret = 0;
    __test__dpgt_open();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dpgt_open");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dpgt_open";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:19 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:39 START
  if (!filter || strstr("dpgt_merge_into", filter))
  {
    extern void __test__dpgt_merge_into(void);
    i_log_info("========================= TEST CASE: %s\n", "dpgt_merge_into");
    int prev = test_ret;
    test_ret = 0;
    __test__dpgt_merge_into();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dpgt_merge_into");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dpgt_merge_into";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:39 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:104 START
  if (!filter || strstr("dpgt_min_rec_lsn", filter))
  {
    extern void __test__dpgt_min_rec_lsn(void);
    i_log_info("========================= TEST CASE: %s\n", "dpgt_min_rec_lsn");
    int prev = test_ret;
    test_ret = 0;
    __test__dpgt_min_rec_lsn();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dpgt_min_rec_lsn");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dpgt_min_rec_lsn";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:104 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:134 START
  if (!filter || strstr("dpgt_exists", filter))
  {
    extern void __test__dpgt_exists(void);
    i_log_info("========================= TEST CASE: %s\n", "dpgt_exists");
    int prev = test_ret;
    test_ret = 0;
    __test__dpgt_exists();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dpgt_exists");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dpgt_exists";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:134 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:157 START
  if (!filter || strstr("dpgt_add", filter))
  {
    extern void __test__dpgt_add(void);
    i_log_info("========================= TEST CASE: %s\n", "dpgt_add");
    int prev = test_ret;
    test_ret = 0;
    __test__dpgt_add();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dpgt_add");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dpgt_add";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:157 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:194 START
  if (!filter || strstr("dpgt_get", filter))
  {
    extern void __test__dpgt_get(void);
    i_log_info("========================= TEST CASE: %s\n", "dpgt_get");
    int prev = test_ret;
    test_ret = 0;
    __test__dpgt_get();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dpgt_get");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dpgt_get";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:194 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:260 START
  if (!filter || strstr("dpgt_remove", filter))
  {
    extern void __test__dpgt_remove(void);
    i_log_info("========================= TEST CASE: %s\n", "dpgt_remove");
    int prev = test_ret;
    test_ret = 0;
    __test__dpgt_remove();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dpgt_remove");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dpgt_remove";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:260 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:324 START
  if (!filter || strstr("dpgt_equal", filter))
  {
    extern void __test__dpgt_equal(void);
    i_log_info("========================= TEST CASE: %s\n", "dpgt_equal");
    int prev = test_ret;
    test_ret = 0;
    __test__dpgt_equal();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dpgt_equal");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dpgt_equal";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/dirty_page_table_tests.c:324 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/dpgt_concurrency_tests.c:100 START
  if (!filter || strstr("dpgt_concurrent", filter))
  {
    extern void __test__dpgt_concurrent(void);
    i_log_info("========================= TEST CASE: %s\n", "dpgt_concurrent");
    int prev = test_ret;
    test_ret = 0;
    __test__dpgt_concurrent();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dpgt_concurrent");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dpgt_concurrent";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/dpgt_concurrency_tests.c:100 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/mem_vhmap.c:255 START
  if (!filter || strstr("mem_vhmap", filter))
  {
    extern void __test__mem_vhmap(void);
    i_log_info("========================= TEST CASE: %s\n", "mem_vhmap");
    int prev = test_ret;
    test_ret = 0;
    __test__mem_vhmap();
    if (!test_ret)
    {
      i_log_passed("%s\n", "mem_vhmap");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "mem_vhmap";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/mem_vhmap.c:255 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/numstore_regression_tests.c:20 START
  if (!filter || strstr("regression_cgd_test_create_delete_rollback_delete", filter))
  {
    extern void __test__regression_cgd_test_create_delete_rollback_delete(void);
    i_log_info("========================= TEST CASE: %s\n", "regression_cgd_test_create_delete_rollback_delete");
    int prev = test_ret;
    test_ret = 0;
    __test__regression_cgd_test_create_delete_rollback_delete();
    if (!test_ret)
    {
      i_log_passed("%s\n", "regression_cgd_test_create_delete_rollback_delete");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "regression_cgd_test_create_delete_rollback_delete";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/numstore_regression_tests.c:20 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/numstore_regression_tests.c:66 START
  if (!filter || strstr("regression_cgd_test_create_crash_close_delete", filter))
  {
    extern void __test__regression_cgd_test_create_crash_close_delete(void);
    i_log_info("========================= TEST CASE: %s\n", "regression_cgd_test_create_crash_close_delete");
    int prev = test_ret;
    test_ret = 0;
    __test__regression_cgd_test_create_crash_close_delete();
    if (!test_ret)
    {
      i_log_passed("%s\n", "regression_cgd_test_create_crash_close_delete");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "regression_cgd_test_create_crash_close_delete";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/numstore_regression_tests.c:66 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/numstore_regression_tests.c:99 START
  if (!filter || strstr("regression_irwr_rollback_invalid_wal_header", filter))
  {
    extern void __test__regression_irwr_rollback_invalid_wal_header(void);
    i_log_info("========================= TEST CASE: %s\n", "regression_irwr_rollback_invalid_wal_header");
    int prev = test_ret;
    test_ret = 0;
    __test__regression_irwr_rollback_invalid_wal_header();
    if (!test_ret)
    {
      i_log_passed("%s\n", "regression_irwr_rollback_invalid_wal_header");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "regression_irwr_rollback_invalid_wal_header";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/numstore_regression_tests.c:99 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/numstore_tests.c:24 START
  if (!filter || strstr("nsdb_create_txn", filter))
  {
    extern void __test__nsdb_create_txn(void);
    i_log_info("========================= TEST CASE: %s\n", "nsdb_create_txn");
    int prev = test_ret;
    test_ret = 0;
    __test__nsdb_create_txn();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nsdb_create_txn");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nsdb_create_txn";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/numstore_tests.c:24 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/numstore_tests.c:207 START
  if (!filter || strstr("nsdb_delete_txn", filter))
  {
    extern void __test__nsdb_delete_txn(void);
    i_log_info("========================= TEST CASE: %s\n", "nsdb_delete_txn");
    int prev = test_ret;
    test_ret = 0;
    __test__nsdb_delete_txn();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nsdb_delete_txn");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nsdb_delete_txn";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/numstore_tests.c:207 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/numstore_tests.c:322 START
  if (!filter || strstr("nsdb_insert_txn", filter))
  {
    extern void __test__nsdb_insert_txn(void);
    i_log_info("========================= TEST CASE: %s\n", "nsdb_insert_txn");
    int prev = test_ret;
    test_ret = 0;
    __test__nsdb_insert_txn();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nsdb_insert_txn");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nsdb_insert_txn";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/numstore_tests.c:322 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/numstore_tests.c:584 START
  if (!filter || strstr("nsdb_write_txn", filter))
  {
    extern void __test__nsdb_write_txn(void);
    i_log_info("========================= TEST CASE: %s\n", "nsdb_write_txn");
    int prev = test_ret;
    test_ret = 0;
    __test__nsdb_write_txn();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nsdb_write_txn");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nsdb_write_txn";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/numstore_tests.c:584 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/page_fixture.c:307 START
  if (!filter || strstr("build_page_tree", filter))
  {
    extern void __test__build_page_tree(void);
    i_log_info("========================= TEST CASE: %s\n", "build_page_tree");
    int prev = test_ret;
    test_ret = 0;
    __test__build_page_tree();
    if (!test_ret)
    {
      i_log_passed("%s\n", "build_page_tree");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "build_page_tree";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/page_fixture.c:307 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/robin_hood_ht_tests.c:27 START
  if (!filter || strstr("ht_insert_idx_regression_trigger_swap", filter))
  {
    extern void __test__ht_insert_idx_regression_trigger_swap(void);
    i_log_info("========================= TEST CASE: %s\n", "ht_insert_idx_regression_trigger_swap");
    int prev = test_ret;
    test_ret = 0;
    __test__ht_insert_idx_regression_trigger_swap();
    if (!test_ret)
    {
      i_log_passed("%s\n", "ht_insert_idx_regression_trigger_swap");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ht_insert_idx_regression_trigger_swap";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/robin_hood_ht_tests.c:27 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/robin_hood_ht_tests.c:105 START
  if (!filter || strstr("robin_hood_ht", filter))
  {
    extern void __test__robin_hood_ht(void);
    i_log_info("========================= TEST CASE: %s\n", "robin_hood_ht");
    int prev = test_ret;
    test_ret = 0;
    __test__robin_hood_ht();
    if (!test_ret)
    {
      i_log_passed("%s\n", "robin_hood_ht");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "robin_hood_ht";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/robin_hood_ht_tests.c:105 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/smfile_data_writer.c:126 START
  if (!filter || strstr("smfile_data_writer", filter))
  {
    extern void __test__smfile_data_writer(void);
    i_log_info("========================= TEST CASE: %s\n", "smfile_data_writer");
    int prev = test_ret;
    test_ret = 0;
    __test__smfile_data_writer();
    if (!test_ret)
    {
      i_log_passed("%s\n", "smfile_data_writer");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "smfile_data_writer";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/smfile_data_writer.c:126 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/testing.c:72 START
  if (!filter || strstr("test_mark_works", filter))
  {
    extern void __test__test_mark_works(void);
    i_log_info("========================= TEST CASE: %s\n", "test_mark_works");
    int prev = test_ret;
    test_ret = 0;
    __test__test_mark_works();
    if (!test_ret)
    {
      i_log_passed("%s\n", "test_mark_works");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "test_mark_works";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/testing.c:72 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/testing.c:83 START
  if (!filter || strstr("test_mark_match", filter))
  {
    extern void __test__test_mark_match(void);
    i_log_info("========================= TEST CASE: %s\n", "test_mark_match");
    int prev = test_ret;
    test_ret = 0;
    __test__test_mark_match();
    if (!test_ret)
    {
      i_log_passed("%s\n", "test_mark_match");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "test_mark_match";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/testing.c:83 DONE

  //////////////////// /Users/theo/Development/Numstore/src/testing/txnt_concurrency_tests.c:119 START
  if (!filter || strstr("txnt_concurrent", filter))
  {
    extern void __test__txnt_concurrent(void);
    i_log_info("========================= TEST CASE: %s\n", "txnt_concurrent");
    int prev = test_ret;
    test_ret = 0;
    __test__txnt_concurrent();
    if (!test_ret)
    {
      i_log_passed("%s\n", "txnt_concurrent");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_concurrent";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/testing/txnt_concurrency_tests.c:119 DONE

  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:324 START
  if (!filter || strstr("txn_basic", filter))
  {
    extern void __test__txn_basic(void);
    i_log_info("========================= TEST CASE: %s\n", "txn_basic");
    int prev = test_ret;
    test_ret = 0;
    __test__txn_basic();
    if (!test_ret)
    {
      i_log_passed("%s\n", "txn_basic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txn_basic";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:324 DONE

  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:431 START
  if (!filter || strstr("txnt_open", filter))
  {
    extern void __test__txnt_open(void);
    i_log_info("========================= TEST CASE: %s\n", "txnt_open");
    int prev = test_ret;
    test_ret = 0;
    __test__txnt_open();
    if (!test_ret)
    {
      i_log_passed("%s\n", "txnt_open");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_open";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:431 DONE

  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:577 START
  if (!filter || strstr("txnt_merge_into", filter))
  {
    extern void __test__txnt_merge_into(void);
    i_log_info("========================= TEST CASE: %s\n", "txnt_merge_into");
    int prev = test_ret;
    test_ret = 0;
    __test__txnt_merge_into();
    if (!test_ret)
    {
      i_log_passed("%s\n", "txnt_merge_into");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_merge_into";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:577 DONE

  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:718 START
  if (!filter || strstr("txnt_max_u_undo_lsn", filter))
  {
    extern void __test__txnt_max_u_undo_lsn(void);
    i_log_info("========================= TEST CASE: %s\n", "txnt_max_u_undo_lsn");
    int prev = test_ret;
    test_ret = 0;
    __test__txnt_max_u_undo_lsn();
    if (!test_ret)
    {
      i_log_passed("%s\n", "txnt_max_u_undo_lsn");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_max_u_undo_lsn";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:718 DONE

  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:846 START
  if (!filter || strstr("txnt_min_lsn", filter))
  {
    extern void __test__txnt_min_lsn(void);
    i_log_info("========================= TEST CASE: %s\n", "txnt_min_lsn");
    int prev = test_ret;
    test_ret = 0;
    __test__txnt_min_lsn();
    if (!test_ret)
    {
      i_log_passed("%s\n", "txnt_min_lsn");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_min_lsn";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:846 DONE

  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:954 START
  if (!filter || strstr("txnt_exists", filter))
  {
    extern void __test__txnt_exists(void);
    i_log_info("========================= TEST CASE: %s\n", "txnt_exists");
    int prev = test_ret;
    test_ret = 0;
    __test__txnt_exists();
    if (!test_ret)
    {
      i_log_passed("%s\n", "txnt_exists");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_exists";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:954 DONE

  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:1011 START
  if (!filter || strstr("txnt_insert", filter))
  {
    extern void __test__txnt_insert(void);
    i_log_info("========================= TEST CASE: %s\n", "txnt_insert");
    int prev = test_ret;
    test_ret = 0;
    __test__txnt_insert();
    if (!test_ret)
    {
      i_log_passed("%s\n", "txnt_insert");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_insert";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:1011 DONE

  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:1164 START
  if (!filter || strstr("txnt_get", filter))
  {
    extern void __test__txnt_get(void);
    i_log_info("========================= TEST CASE: %s\n", "txnt_get");
    int prev = test_ret;
    test_ret = 0;
    __test__txnt_get();
    if (!test_ret)
    {
      i_log_passed("%s\n", "txnt_get");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_get";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:1164 DONE

  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:1348 START
  if (!filter || strstr("txnt_remove", filter))
  {
    extern void __test__txnt_remove(void);
    i_log_info("========================= TEST CASE: %s\n", "txnt_remove");
    int prev = test_ret;
    test_ret = 0;
    __test__txnt_remove();
    if (!test_ret)
    {
      i_log_passed("%s\n", "txnt_remove");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_remove";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:1348 DONE

  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:1568 START
  if (!filter || strstr("txnt_serialize", filter))
  {
    extern void __test__txnt_serialize(void);
    i_log_info("========================= TEST CASE: %s\n", "txnt_serialize");
    int prev = test_ret;
    test_ret = 0;
    __test__txnt_serialize();
    if (!test_ret)
    {
      i_log_passed("%s\n", "txnt_serialize");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_serialize";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:1568 DONE

  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:1702 START
  if (!filter || strstr("txnt_equal_ignore_state", filter))
  {
    extern void __test__txnt_equal_ignore_state(void);
    i_log_info("========================= TEST CASE: %s\n", "txnt_equal_ignore_state");
    int prev = test_ret;
    test_ret = 0;
    __test__txnt_equal_ignore_state();
    if (!test_ret)
    {
      i_log_passed("%s\n", "txnt_equal_ignore_state");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_equal_ignore_state";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/txn_table.c:1702 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:71 START
  if (!filter || strstr("prim_t_validate", filter))
  {
    extern void __test__prim_t_validate(void);
    i_log_info("========================= TEST CASE: %s\n", "prim_t_validate");
    int prev = test_ret;
    test_ret = 0;
    __test__prim_t_validate();
    if (!test_ret)
    {
      i_log_passed("%s\n", "prim_t_validate");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "prim_t_validate";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:71 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:374 START
  if (!filter || strstr("prim_t_snprintf", filter))
  {
    extern void __test__prim_t_snprintf(void);
    i_log_info("========================= TEST CASE: %s\n", "prim_t_snprintf");
    int prev = test_ret;
    test_ret = 0;
    __test__prim_t_snprintf();
    if (!test_ret)
    {
      i_log_passed("%s\n", "prim_t_snprintf");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "prim_t_snprintf";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:374 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:494 START
  if (!filter || strstr("struct_t_snprintf", filter))
  {
    extern void __test__struct_t_snprintf(void);
    i_log_info("========================= TEST CASE: %s\n", "struct_t_snprintf");
    int prev = test_ret;
    test_ret = 0;
    __test__struct_t_snprintf();
    if (!test_ret)
    {
      i_log_passed("%s\n", "struct_t_snprintf");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "struct_t_snprintf";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:494 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:654 START
  if (!filter || strstr("union_t_snprintf", filter))
  {
    extern void __test__union_t_snprintf(void);
    i_log_info("========================= TEST CASE: %s\n", "union_t_snprintf");
    int prev = test_ret;
    test_ret = 0;
    __test__union_t_snprintf();
    if (!test_ret)
    {
      i_log_passed("%s\n", "union_t_snprintf");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "union_t_snprintf";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:654 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:752 START
  if (!filter || strstr("sarray_t_snprintf", filter))
  {
    extern void __test__sarray_t_snprintf(void);
    i_log_info("========================= TEST CASE: %s\n", "sarray_t_snprintf");
    int prev = test_ret;
    test_ret = 0;
    __test__sarray_t_snprintf();
    if (!test_ret)
    {
      i_log_passed("%s\n", "sarray_t_snprintf");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "sarray_t_snprintf";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:752 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:878 START
  if (!filter || strstr("prim_t_byte_size", filter))
  {
    extern void __test__prim_t_byte_size(void);
    i_log_info("========================= TEST CASE: %s\n", "prim_t_byte_size");
    int prev = test_ret;
    test_ret = 0;
    __test__prim_t_byte_size();
    if (!test_ret)
    {
      i_log_passed("%s\n", "prim_t_byte_size");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "prim_t_byte_size";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:878 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:907 START
  if (!filter || strstr("struct_t_byte_size", filter))
  {
    extern void __test__struct_t_byte_size(void);
    i_log_info("========================= TEST CASE: %s\n", "struct_t_byte_size");
    int prev = test_ret;
    test_ret = 0;
    __test__struct_t_byte_size();
    if (!test_ret)
    {
      i_log_passed("%s\n", "struct_t_byte_size");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "struct_t_byte_size";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:907 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:978 START
  if (!filter || strstr("union_t_byte_size", filter))
  {
    extern void __test__union_t_byte_size(void);
    i_log_info("========================= TEST CASE: %s\n", "union_t_byte_size");
    int prev = test_ret;
    test_ret = 0;
    __test__union_t_byte_size();
    if (!test_ret)
    {
      i_log_passed("%s\n", "union_t_byte_size");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "union_t_byte_size";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:978 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:1046 START
  if (!filter || strstr("sarray_t_byte_size", filter))
  {
    extern void __test__sarray_t_byte_size(void);
    i_log_info("========================= TEST CASE: %s\n", "sarray_t_byte_size");
    int prev = test_ret;
    test_ret = 0;
    __test__sarray_t_byte_size();
    if (!test_ret)
    {
      i_log_passed("%s\n", "sarray_t_byte_size");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "sarray_t_byte_size";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:1046 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:1237 START
  if (!filter || strstr("type_generate_string", filter))
  {
    extern void __test__type_generate_string(void);
    i_log_info("========================= TEST CASE: %s\n", "type_generate_string");
    int prev = test_ret;
    test_ret = 0;
    __test__type_generate_string();
    if (!test_ret)
    {
      i_log_passed("%s\n", "type_generate_string");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "type_generate_string";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:1237 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:1397 START
  if (!filter || strstr("struct_t_get_serial_size", filter))
  {
    extern void __test__struct_t_get_serial_size(void);
    i_log_info("========================= TEST CASE: %s\n", "struct_t_get_serial_size");
    int prev = test_ret;
    test_ret = 0;
    __test__struct_t_get_serial_size();
    if (!test_ret)
    {
      i_log_passed("%s\n", "struct_t_get_serial_size");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "struct_t_get_serial_size";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:1397 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:1465 START
  if (!filter || strstr("union_t_get_serial_size", filter))
  {
    extern void __test__union_t_get_serial_size(void);
    i_log_info("========================= TEST CASE: %s\n", "union_t_get_serial_size");
    int prev = test_ret;
    test_ret = 0;
    __test__union_t_get_serial_size();
    if (!test_ret)
    {
      i_log_passed("%s\n", "union_t_get_serial_size");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "union_t_get_serial_size";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:1465 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:1528 START
  if (!filter || strstr("sarray_t_get_serial_size", filter))
  {
    extern void __test__sarray_t_get_serial_size(void);
    i_log_info("========================= TEST CASE: %s\n", "sarray_t_get_serial_size");
    int prev = test_ret;
    test_ret = 0;
    __test__sarray_t_get_serial_size();
    if (!test_ret)
    {
      i_log_passed("%s\n", "sarray_t_get_serial_size");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "sarray_t_get_serial_size";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:1528 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:1589 START
  if (!filter || strstr("prim_t_serialize", filter))
  {
    extern void __test__prim_t_serialize(void);
    i_log_info("========================= TEST CASE: %s\n", "prim_t_serialize");
    int prev = test_ret;
    test_ret = 0;
    __test__prim_t_serialize();
    if (!test_ret)
    {
      i_log_passed("%s\n", "prim_t_serialize");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "prim_t_serialize";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:1589 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:1629 START
  if (!filter || strstr("struct_t_serialize", filter))
  {
    extern void __test__struct_t_serialize(void);
    i_log_info("========================= TEST CASE: %s\n", "struct_t_serialize");
    int prev = test_ret;
    test_ret = 0;
    __test__struct_t_serialize();
    if (!test_ret)
    {
      i_log_passed("%s\n", "struct_t_serialize");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "struct_t_serialize";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:1629 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:1722 START
  if (!filter || strstr("union_t_serialize", filter))
  {
    extern void __test__union_t_serialize(void);
    i_log_info("========================= TEST CASE: %s\n", "union_t_serialize");
    int prev = test_ret;
    test_ret = 0;
    __test__union_t_serialize();
    if (!test_ret)
    {
      i_log_passed("%s\n", "union_t_serialize");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "union_t_serialize";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:1722 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:1810 START
  if (!filter || strstr("sarray_t_serialize", filter))
  {
    extern void __test__sarray_t_serialize(void);
    i_log_info("========================= TEST CASE: %s\n", "sarray_t_serialize");
    int prev = test_ret;
    test_ret = 0;
    __test__sarray_t_serialize();
    if (!test_ret)
    {
      i_log_passed("%s\n", "sarray_t_serialize");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "sarray_t_serialize";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:1810 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:1909 START
  if (!filter || strstr("prim_t_deserialize", filter))
  {
    extern void __test__prim_t_deserialize(void);
    i_log_info("========================= TEST CASE: %s\n", "prim_t_deserialize");
    int prev = test_ret;
    test_ret = 0;
    __test__prim_t_deserialize();
    if (!test_ret)
    {
      i_log_passed("%s\n", "prim_t_deserialize");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "prim_t_deserialize";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:1909 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:2009 START
  if (!filter || strstr("struct_t_deserialize_green_path", filter))
  {
    extern void __test__struct_t_deserialize_green_path(void);
    i_log_info("========================= TEST CASE: %s\n", "struct_t_deserialize_green_path");
    int prev = test_ret;
    test_ret = 0;
    __test__struct_t_deserialize_green_path();
    if (!test_ret)
    {
      i_log_passed("%s\n", "struct_t_deserialize_green_path");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "struct_t_deserialize_green_path";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:2009 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:2064 START
  if (!filter || strstr("struct_t_deserialize_red_path", filter))
  {
    extern void __test__struct_t_deserialize_red_path(void);
    i_log_info("========================= TEST CASE: %s\n", "struct_t_deserialize_red_path");
    int prev = test_ret;
    test_ret = 0;
    __test__struct_t_deserialize_red_path();
    if (!test_ret)
    {
      i_log_passed("%s\n", "struct_t_deserialize_red_path");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "struct_t_deserialize_red_path";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:2064 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:2205 START
  if (!filter || strstr("union_t_deserialize_green_path", filter))
  {
    extern void __test__union_t_deserialize_green_path(void);
    i_log_info("========================= TEST CASE: %s\n", "union_t_deserialize_green_path");
    int prev = test_ret;
    test_ret = 0;
    __test__union_t_deserialize_green_path();
    if (!test_ret)
    {
      i_log_passed("%s\n", "union_t_deserialize_green_path");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "union_t_deserialize_green_path";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:2205 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:2260 START
  if (!filter || strstr("union_t_deserialize_red_path", filter))
  {
    extern void __test__union_t_deserialize_red_path(void);
    i_log_info("========================= TEST CASE: %s\n", "union_t_deserialize_red_path");
    int prev = test_ret;
    test_ret = 0;
    __test__union_t_deserialize_red_path();
    if (!test_ret)
    {
      i_log_passed("%s\n", "union_t_deserialize_red_path");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "union_t_deserialize_red_path";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:2260 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:2354 START
  if (!filter || strstr("sarray_t_deserialize_green_path", filter))
  {
    extern void __test__sarray_t_deserialize_green_path(void);
    i_log_info("========================= TEST CASE: %s\n", "sarray_t_deserialize_green_path");
    int prev = test_ret;
    test_ret = 0;
    __test__sarray_t_deserialize_green_path();
    if (!test_ret)
    {
      i_log_passed("%s\n", "sarray_t_deserialize_green_path");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "sarray_t_deserialize_green_path";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:2354 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:2388 START
  if (!filter || strstr("sarray_t_deserialize_red_path", filter))
  {
    extern void __test__sarray_t_deserialize_red_path(void);
    i_log_info("========================= TEST CASE: %s\n", "sarray_t_deserialize_red_path");
    int prev = test_ret;
    test_ret = 0;
    __test__sarray_t_deserialize_red_path();
    if (!test_ret)
    {
      i_log_passed("%s\n", "sarray_t_deserialize_red_path");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "sarray_t_deserialize_red_path";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:2388 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:2483 START
  if (!filter || strstr("prim_t_random", filter))
  {
    extern void __test__prim_t_random(void);
    i_log_info("========================= TEST CASE: %s\n", "prim_t_random");
    int prev = test_ret;
    test_ret = 0;
    __test__prim_t_random();
    if (!test_ret)
    {
      i_log_passed("%s\n", "prim_t_random");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "prim_t_random";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:2483 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:3297 START
  if (!filter || strstr("sarray_builder", filter))
  {
    extern void __test__sarray_builder(void);
    i_log_info("========================= TEST CASE: %s\n", "sarray_builder");
    int prev = test_ret;
    test_ret = 0;
    __test__sarray_builder();
    if (!test_ret)
    {
      i_log_passed("%s\n", "sarray_builder");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "sarray_builder";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:3297 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:3527 START
  if (!filter || strstr("kvt_list_builder", filter))
  {
    extern void __test__kvt_list_builder(void);
    i_log_info("========================= TEST CASE: %s\n", "kvt_list_builder");
    int prev = test_ret;
    test_ret = 0;
    __test__kvt_list_builder();
    if (!test_ret)
    {
      i_log_passed("%s\n", "kvt_list_builder");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "kvt_list_builder";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:3527 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:4107 START
  if (!filter || strstr("ta_subtype", filter))
  {
    extern void __test__ta_subtype(void);
    i_log_info("========================= TEST CASE: %s\n", "ta_subtype");
    int prev = test_ret;
    test_ret = 0;
    __test__ta_subtype();
    if (!test_ret)
    {
      i_log_passed("%s\n", "ta_subtype");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ta_subtype";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:4107 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:4472 START
  if (!filter || strstr("type_accessor_builder", filter))
  {
    extern void __test__type_accessor_builder(void);
    i_log_info("========================= TEST CASE: %s\n", "type_accessor_builder");
    int prev = test_ret;
    test_ret = 0;
    __test__type_accessor_builder();
    if (!test_ret)
    {
      i_log_passed("%s\n", "type_accessor_builder");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "type_accessor_builder";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:4472 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:4715 START
  if (!filter || strstr("print_indent", filter))
  {
    extern void __test__print_indent(void);
    i_log_info("========================= TEST CASE: %s\n", "print_indent");
    int prev = test_ret;
    test_ret = 0;
    __test__print_indent();
    if (!test_ret)
    {
      i_log_passed("%s\n", "print_indent");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "print_indent";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:4715 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:4941 START
  if (!filter || strstr("print_prim_value", filter))
  {
    extern void __test__print_prim_value(void);
    i_log_info("========================= TEST CASE: %s\n", "print_prim_value");
    int prev = test_ret;
    test_ret = 0;
    __test__print_prim_value();
    if (!test_ret)
    {
      i_log_passed("%s\n", "print_prim_value");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "print_prim_value";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:4941 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:5084 START
  if (!filter || strstr("sarray_sub_size", filter))
  {
    extern void __test__sarray_sub_size(void);
    i_log_info("========================= TEST CASE: %s\n", "sarray_sub_size");
    int prev = test_ret;
    test_ret = 0;
    __test__sarray_sub_size();
    if (!test_ret)
    {
      i_log_passed("%s\n", "sarray_sub_size");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "sarray_sub_size";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:5084 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:5170 START
  if (!filter || strstr("print_sarray_dim", filter))
  {
    extern void __test__print_sarray_dim(void);
    i_log_info("========================= TEST CASE: %s\n", "print_sarray_dim");
    int prev = test_ret;
    test_ret = 0;
    __test__print_sarray_dim();
    if (!test_ret)
    {
      i_log_passed("%s\n", "print_sarray_dim");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "print_sarray_dim";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:5170 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:5266 START
  if (!filter || strstr("print_type_inner", filter))
  {
    extern void __test__print_type_inner(void);
    i_log_info("========================= TEST CASE: %s\n", "print_type_inner");
    int prev = test_ret;
    test_ret = 0;
    __test__print_type_inner();
    if (!test_ret)
    {
      i_log_passed("%s\n", "print_type_inner");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "print_type_inner";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:5266 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:5290 START
  if (!filter || strstr("type_print_data", filter))
  {
    extern void __test__type_print_data(void);
    i_log_info("========================= TEST CASE: %s\n", "type_print_data");
    int prev = test_ret;
    test_ret = 0;
    __test__type_print_data();
    if (!test_ret)
    {
      i_log_passed("%s\n", "type_print_data");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "type_print_data";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:5290 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:5436 START
  if (!filter || strstr("type_print_os_sink", filter))
  {
    extern void __test__type_print_os_sink(void);
    i_log_info("========================= TEST CASE: %s\n", "type_print_os_sink");
    int prev = test_ret;
    test_ret = 0;
    __test__type_print_os_sink();
    if (!test_ret)
    {
      i_log_passed("%s\n", "type_print_os_sink");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "type_print_os_sink";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:5436 DONE

  //////////////////// /Users/theo/Development/Numstore/src/types.c:5462 START
  if (!filter || strstr("type_print_os_close", filter))
  {
    extern void __test__type_print_os_close(void);
    i_log_info("========================= TEST CASE: %s\n", "type_print_os_close");
    int prev = test_ret;
    test_ret = 0;
    __test__type_print_os_close();
    if (!test_ret)
    {
      i_log_passed("%s\n", "type_print_os_close");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "type_print_os_close";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/types.c:5462 DONE

  //////////////////// /Users/theo/Development/Numstore/src/utils.c:33 START
  if (!filter || strstr("file_basename", filter))
  {
    extern void __test__file_basename(void);
    i_log_info("========================= TEST CASE: %s\n", "file_basename");
    int prev = test_ret;
    test_ret = 0;
    __test__file_basename();
    if (!test_ret)
    {
      i_log_passed("%s\n", "file_basename");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "file_basename";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/utils.c:33 DONE

  //////////////////// /Users/theo/Development/Numstore/src/var_algorithms.c:80 START
  if (!filter || strstr("ns_init_var_hash_map", filter))
  {
    extern void __test__ns_init_var_hash_map(void);
    i_log_info("========================= TEST CASE: %s\n", "ns_init_var_hash_map");
    int prev = test_ret;
    test_ret = 0;
    __test__ns_init_var_hash_map();
    if (!test_ret)
    {
      i_log_passed("%s\n", "ns_init_var_hash_map");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ns_init_var_hash_map";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/var_algorithms.c:80 DONE

  //////////////////// /Users/theo/Development/Numstore/src/var_algorithms.c:471 START
  if (!filter || strstr("ns_find_var_page", filter))
  {
    extern void __test__ns_find_var_page(void);
    i_log_info("========================= TEST CASE: %s\n", "ns_find_var_page");
    int prev = test_ret;
    test_ret = 0;
    __test__ns_find_var_page();
    if (!test_ret)
    {
      i_log_passed("%s\n", "ns_find_var_page");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ns_find_var_page";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/var_algorithms.c:471 DONE

  //////////////////// /Users/theo/Development/Numstore/src/var_algorithms.c:1638 START
  if (!filter || strstr("ns_var_get_or_create", filter))
  {
    extern void __test__ns_var_get_or_create(void);
    i_log_info("========================= TEST CASE: %s\n", "ns_var_get_or_create");
    int prev = test_ret;
    test_ret = 0;
    __test__ns_var_get_or_create();
    if (!test_ret)
    {
      i_log_passed("%s\n", "ns_var_get_or_create");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ns_var_get_or_create";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/var_algorithms.c:1638 DONE

  //////////////////// /Users/theo/Development/Numstore/src/variables.c:292 START
  if (!filter || strstr("rand_varname_same_hash", filter))
  {
    extern void __test__rand_varname_same_hash(void);
    i_log_info("========================= TEST CASE: %s\n", "rand_varname_same_hash");
    int prev = test_ret;
    test_ret = 0;
    __test__rand_varname_same_hash();
    if (!test_ret)
    {
      i_log_passed("%s\n", "rand_varname_same_hash");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "rand_varname_same_hash";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/variables.c:292 DONE

  //////////////////// /Users/theo/Development/Numstore/src/variables.c:309 START
  if (!filter || strstr("rand_varname_different_hash", filter))
  {
    extern void __test__rand_varname_different_hash(void);
    i_log_info("========================= TEST CASE: %s\n", "rand_varname_different_hash");
    int prev = test_ret;
    test_ret = 0;
    __test__rand_varname_different_hash();
    if (!test_ret)
    {
      i_log_passed("%s\n", "rand_varname_different_hash");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "rand_varname_different_hash";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/variables.c:309 DONE

  //////////////////// /Users/theo/Development/Numstore/src/wal.c:73 START
  if (!filter || strstr("walos_open", filter))
  {
    extern void __test__walos_open(void);
    i_log_info("========================= TEST CASE: %s\n", "walos_open");
    int prev = test_ret;
    test_ret = 0;
    __test__walos_open();
    if (!test_ret)
    {
      i_log_passed("%s\n", "walos_open");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "walos_open";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/wal.c:73 DONE

  //////////////////// /Users/theo/Development/Numstore/src/wal.c:326 START
  if (!filter || strstr("walis_open", filter))
  {
    extern void __test__walis_open(void);
    i_log_info("========================= TEST CASE: %s\n", "walis_open");
    int prev = test_ret;
    test_ret = 0;
    __test__walis_open();
    if (!test_ret)
    {
      i_log_passed("%s\n", "walis_open");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "walis_open";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/wal.c:326 DONE

  //////////////////// /Users/theo/Development/Numstore/src/wal.c:1463 START
  if (!filter || strstr("wal_rec_hdr_type_tostr", filter))
  {
    extern void __test__wal_rec_hdr_type_tostr(void);
    i_log_info("========================= TEST CASE: %s\n", "wal_rec_hdr_type_tostr");
    int prev = test_ret;
    test_ret = 0;
    __test__wal_rec_hdr_type_tostr();
    if (!test_ret)
    {
      i_log_passed("%s\n", "wal_rec_hdr_type_tostr");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "wal_rec_hdr_type_tostr";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/wal.c:1463 DONE

  //////////////////// /Users/theo/Development/Numstore/src/wal.c:2992 START
  if (!filter || strstr("wal_multi_threaded", filter))
  {
    extern void __test__wal_multi_threaded(void);
    i_log_info("========================= TEST CASE: %s\n", "wal_multi_threaded");
    int prev = test_ret;
    test_ret = 0;
    __test__wal_multi_threaded();
    if (!test_ret)
    {
      i_log_passed("%s\n", "wal_multi_threaded");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "wal_multi_threaded";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/wal.c:2992 DONE

  //////////////////// /Users/theo/Development/Numstore/src/wal.c:3205 START
  if (!filter || strstr("wal", filter))
  {
    extern void __test__wal(void);
    i_log_info("========================= TEST CASE: %s\n", "wal");
    int prev = test_ret;
    test_ret = 0;
    __test__wal();
    if (!test_ret)
    {
      i_log_passed("%s\n", "wal");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "wal";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/wal.c:3205 DONE

  //////////////////// /Users/theo/Development/Numstore/src/wal.c:3323 START
  if (!filter || strstr("wal_single_entry", filter))
  {
    extern void __test__wal_single_entry(void);
    i_log_info("========================= TEST CASE: %s\n", "wal_single_entry");
    int prev = test_ret;
    test_ret = 0;
    __test__wal_single_entry();
    if (!test_ret)
    {
      i_log_passed("%s\n", "wal_single_entry");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "wal_single_entry";
    }
    ntests++;
  }
  //////////////////// /Users/theo/Development/Numstore/src/wal.c:3323 DONE


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

// clang-format on
