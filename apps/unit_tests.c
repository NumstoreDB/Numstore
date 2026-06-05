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

#include <c_specx.h>
#include <stdio.h>
#include <string.h>

int
main (const int argc, char **argv)
{
  const char *filter = (argc > 1) ? argv[1] : NULL;

  error   e = error_create ();
  i_timer timer;
  if (i_timer_create (&timer, &e) != SUCCESS)
  {
    return -1;
  }

  int         failed = 0;
  const char *failed_names[268];

  
  //////////////////// C:\Users\tlincke\dev\numstore\src\aries_tests.c:24 START
  if (!filter || strstr("aries_crash", filter))
  {
    extern void test_aries_crash(void);
    i_log_info("========================= TEST CASE: %s\n", "aries_crash");
    int prev = test_ret;
    test_ret = 0;
    test_aries_crash();
    if (!test_ret)
    {
      i_log_passed("%s\n", "aries_crash");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "aries_crash";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\aries_tests.c:24 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\block_array.c:691 START
  if (!filter || strstr("block_insert_read", filter))
  {
    extern void test_block_insert_read(void);
    i_log_info("========================= TEST CASE: %s\n", "block_insert_read");
    int prev = test_ret;
    test_ret = 0;
    test_block_insert_read();
    if (!test_ret)
    {
      i_log_passed("%s\n", "block_insert_read");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "block_insert_read";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\block_array.c:691 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\block_array.c:951 START
  if (!filter || strstr("block_insert_remove_read", filter))
  {
    extern void test_block_insert_remove_read(void);
    i_log_info("========================= TEST CASE: %s\n", "block_insert_remove_read");
    int prev = test_ret;
    test_ret = 0;
    test_block_insert_remove_read();
    if (!test_ret)
    {
      i_log_passed("%s\n", "block_insert_remove_read");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "block_insert_remove_read";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\block_array.c:951 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\block_array.c:1096 START
  if (!filter || strstr("block_insert_write_read", filter))
  {
    extern void test_block_insert_write_read(void);
    i_log_info("========================= TEST CASE: %s\n", "block_insert_write_read");
    int prev = test_ret;
    test_ret = 0;
    test_block_insert_write_read();
    if (!test_ret)
    {
      i_log_passed("%s\n", "block_insert_write_read");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "block_insert_write_read";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\block_array.c:1096 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\block_array.c:1259 START
  if (!filter || strstr("block_random", filter))
  {
    extern void test_block_random(void);
    i_log_info("========================= TEST CASE: %s\n", "block_random");
    int prev = test_ret;
    test_ret = 0;
    test_block_random();
    if (!test_ret)
    {
      i_log_passed("%s\n", "block_random");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "block_random";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\block_array.c:1259 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\byte_accessor.c:67 START
  if (!filter || strstr("ba_memcpy_from_basic", filter))
  {
    extern void test_ba_memcpy_from_basic(void);
    i_log_info("========================= TEST CASE: %s\n", "ba_memcpy_from_basic");
    int prev = test_ret;
    test_ret = 0;
    test_ba_memcpy_from_basic();
    if (!test_ret)
    {
      i_log_passed("%s\n", "ba_memcpy_from_basic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ba_memcpy_from_basic";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\byte_accessor.c:67 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\byte_accessor.c:397 START
  if (!filter || strstr("ba_memcpy_to_basic", filter))
  {
    extern void test_ba_memcpy_to_basic(void);
    i_log_info("========================= TEST CASE: %s\n", "ba_memcpy_to_basic");
    int prev = test_ret;
    test_ret = 0;
    test_ba_memcpy_to_basic();
    if (!test_ret)
    {
      i_log_passed("%s\n", "ba_memcpy_to_basic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ba_memcpy_to_basic";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\byte_accessor.c:397 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:55 START
  if (!filter || strstr("cbuffer_isempty", filter))
  {
    extern void test_cbuffer_isempty(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_isempty");
    int prev = test_ret;
    test_ret = 0;
    test_cbuffer_isempty();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_isempty");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_isempty";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:55 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:65 START
  if (!filter || strstr("cbuffer_len", filter))
  {
    extern void test_cbuffer_len(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_len");
    int prev = test_ret;
    test_ret = 0;
    test_cbuffer_len();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_len");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_len";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:65 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:75 START
  if (!filter || strstr("cbuffer_avail", filter))
  {
    extern void test_cbuffer_avail(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_avail");
    int prev = test_ret;
    test_ret = 0;
    test_cbuffer_avail();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_avail");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_avail";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:75 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:125 START
  if (!filter || strstr("cbuffer_get_next_data_bytes", filter))
  {
    extern void test_cbuffer_get_next_data_bytes(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_get_next_data_bytes");
    int prev = test_ret;
    test_ret = 0;
    test_cbuffer_get_next_data_bytes();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_get_next_data_bytes");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_get_next_data_bytes";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:125 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:198 START
  if (!filter || strstr("cbuffer_get_nbytes", filter))
  {
    extern void test_cbuffer_get_nbytes(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_get_nbytes");
    int prev = test_ret;
    test_ret = 0;
    test_cbuffer_get_nbytes();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_get_nbytes");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_get_nbytes";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:198 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:265 START
  if (!filter || strstr("cbuffer_fakewrite", filter))
  {
    extern void test_cbuffer_fakewrite(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_fakewrite");
    int prev = test_ret;
    test_ret = 0;
    test_cbuffer_fakewrite();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_fakewrite");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_fakewrite";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:265 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:352 START
  if (!filter || strstr("cbuffer_fakeread", filter))
  {
    extern void test_cbuffer_fakeread(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_fakeread");
    int prev = test_ret;
    test_ret = 0;
    test_cbuffer_fakeread();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_fakeread");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_fakeread";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:352 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:473 START
  if (!filter || strstr("cbuffer_read", filter))
  {
    extern void test_cbuffer_read(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_read");
    int prev = test_ret;
    test_ret = 0;
    test_cbuffer_read();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_read");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_read";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:473 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:545 START
  if (!filter || strstr("cbuffer_copy", filter))
  {
    extern void test_cbuffer_copy(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_copy");
    int prev = test_ret;
    test_ret = 0;
    test_cbuffer_copy();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_copy");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_copy";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:545 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:634 START
  if (!filter || strstr("cbuffer_write", filter))
  {
    extern void test_cbuffer_write(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_write");
    int prev = test_ret;
    test_ret = 0;
    test_cbuffer_write();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_write");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_write";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:634 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:728 START
  if (!filter || strstr("cbuffer_cbuffer_move", filter))
  {
    extern void test_cbuffer_cbuffer_move(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_cbuffer_move");
    int prev = test_ret;
    test_ret = 0;
    test_cbuffer_cbuffer_move();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_cbuffer_move");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_cbuffer_move";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:728 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:837 START
  if (!filter || strstr("cbuffer_cbuffer_copy", filter))
  {
    extern void test_cbuffer_cbuffer_copy(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_cbuffer_copy");
    int prev = test_ret;
    test_ret = 0;
    test_cbuffer_cbuffer_copy();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_cbuffer_copy");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_cbuffer_copy";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:837 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:1117 START
  if (!filter || strstr("cbuffer_get_no_check", filter))
  {
    extern void test_cbuffer_get_no_check(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_get_no_check");
    int prev = test_ret;
    test_ret = 0;
    test_cbuffer_get_no_check();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_get_no_check");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_get_no_check";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:1117 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:1219 START
  if (!filter || strstr("cbuffer_get", filter))
  {
    extern void test_cbuffer_get(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_get");
    int prev = test_ret;
    test_ret = 0;
    test_cbuffer_get();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_get");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_get";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:1219 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:1250 START
  if (!filter || strstr("cbuffer_peek_back", filter))
  {
    extern void test_cbuffer_peek_back(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_peek_back");
    int prev = test_ret;
    test_ret = 0;
    test_cbuffer_peek_back();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_peek_back");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_peek_back";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:1250 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:1306 START
  if (!filter || strstr("cbuffer_peek_front", filter))
  {
    extern void test_cbuffer_peek_front(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_peek_front");
    int prev = test_ret;
    test_ret = 0;
    test_cbuffer_peek_front();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_peek_front");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_peek_front";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:1306 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:1380 START
  if (!filter || strstr("cbuffer_push_back", filter))
  {
    extern void test_cbuffer_push_back(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_push_back");
    int prev = test_ret;
    test_ret = 0;
    test_cbuffer_push_back();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_push_back");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_push_back";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:1380 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:1441 START
  if (!filter || strstr("cbuffer_push_front", filter))
  {
    extern void test_cbuffer_push_front(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_push_front");
    int prev = test_ret;
    test_ret = 0;
    test_cbuffer_push_front();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_push_front");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_push_front";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:1441 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:1512 START
  if (!filter || strstr("cbuffer_pop_back", filter))
  {
    extern void test_cbuffer_pop_back(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_pop_back");
    int prev = test_ret;
    test_ret = 0;
    test_cbuffer_pop_back();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_pop_back");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_pop_back";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:1512 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:1583 START
  if (!filter || strstr("cbuffer_pop_front", filter))
  {
    extern void test_cbuffer_pop_front(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_pop_front");
    int prev = test_ret;
    test_ret = 0;
    test_cbuffer_pop_front();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_pop_front");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_pop_front";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\cbuffer.c:1583 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\checksums.c:62 START
  if (!filter || strstr("checksum_execute_simple", filter))
  {
    extern void test_checksum_execute_simple(void);
    i_log_info("========================= TEST CASE: %s\n", "checksum_execute_simple");
    int prev = test_ret;
    test_ret = 0;
    test_checksum_execute_simple();
    if (!test_ret)
    {
      i_log_passed("%s\n", "checksum_execute_simple");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "checksum_execute_simple";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\checksums.c:62 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\checksums.c:73 START
  if (!filter || strstr("checksum_execute_deterministic", filter))
  {
    extern void test_checksum_execute_deterministic(void);
    i_log_info("========================= TEST CASE: %s\n", "checksum_execute_deterministic");
    int prev = test_ret;
    test_ret = 0;
    test_checksum_execute_deterministic();
    if (!test_ret)
    {
      i_log_passed("%s\n", "checksum_execute_deterministic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "checksum_execute_deterministic";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\checksums.c:73 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\checksums.c:85 START
  if (!filter || strstr("checksum_execute_incremental", filter))
  {
    extern void test_checksum_execute_incremental(void);
    i_log_info("========================= TEST CASE: %s\n", "checksum_execute_incremental");
    int prev = test_ret;
    test_ret = 0;
    test_checksum_execute_incremental();
    if (!test_ret)
    {
      i_log_passed("%s\n", "checksum_execute_incremental");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "checksum_execute_incremental";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\checksums.c:85 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\compiler.c:114 START
  if (!filter || strstr("compile_user_stride_basic", filter))
  {
    extern void test_compile_user_stride_basic(void);
    i_log_info("========================= TEST CASE: %s\n", "compile_user_stride_basic");
    int prev = test_ret;
    test_ret = 0;
    test_compile_user_stride_basic();
    if (!test_ret)
    {
      i_log_passed("%s\n", "compile_user_stride_basic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "compile_user_stride_basic";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\compiler.c:114 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\compiler.c:148 START
  if (!filter || strstr("compile_type_primitives", filter))
  {
    extern void test_compile_type_primitives(void);
    i_log_info("========================= TEST CASE: %s\n", "compile_type_primitives");
    int prev = test_ret;
    test_ret = 0;
    test_compile_type_primitives();
    if (!test_ret)
    {
      i_log_passed("%s\n", "compile_type_primitives");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "compile_type_primitives";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\compiler.c:148 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\compiler.c:217 START
  if (!filter || strstr("compile_type_sarray", filter))
  {
    extern void test_compile_type_sarray(void);
    i_log_info("========================= TEST CASE: %s\n", "compile_type_sarray");
    int prev = test_ret;
    test_ret = 0;
    test_compile_type_sarray();
    if (!test_ret)
    {
      i_log_passed("%s\n", "compile_type_sarray");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "compile_type_sarray";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\compiler.c:217 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\compiler.c:260 START
  if (!filter || strstr("compile_type_struct", filter))
  {
    extern void test_compile_type_struct(void);
    i_log_info("========================= TEST CASE: %s\n", "compile_type_struct");
    int prev = test_ret;
    test_ret = 0;
    test_compile_type_struct();
    if (!test_ret)
    {
      i_log_passed("%s\n", "compile_type_struct");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "compile_type_struct";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\compiler.c:260 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\compiler.c:296 START
  if (!filter || strstr("compile_type_union", filter))
  {
    extern void test_compile_type_union(void);
    i_log_info("========================= TEST CASE: %s\n", "compile_type_union");
    int prev = test_ret;
    test_ret = 0;
    test_compile_type_union();
    if (!test_ret)
    {
      i_log_passed("%s\n", "compile_type_union");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "compile_type_union";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\compiler.c:296 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\compiler.c:320 START
  if (!filter || strstr("compile_type_complex", filter))
  {
    extern void test_compile_type_complex(void);
    i_log_info("========================= TEST CASE: %s\n", "compile_type_complex");
    int prev = test_ret;
    test_ret = 0;
    test_compile_type_complex();
    if (!test_ret)
    {
      i_log_passed("%s\n", "compile_type_complex");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "compile_type_complex";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\compiler.c:320 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:51 START
  if (!filter || strstr("f16_to_f32_normals_and_specials", filter))
  {
    extern void test_f16_to_f32_normals_and_specials(void);
    i_log_info("========================= TEST CASE: %s\n", "f16_to_f32_normals_and_specials");
    int prev = test_ret;
    test_ret = 0;
    test_f16_to_f32_normals_and_specials();
    if (!test_ret)
    {
      i_log_passed("%s\n", "f16_to_f32_normals_and_specials");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "f16_to_f32_normals_and_specials";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:51 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:70 START
  if (!filter || strstr("f16_to_f32_nan_is_nan", filter))
  {
    extern void test_f16_to_f32_nan_is_nan(void);
    i_log_info("========================= TEST CASE: %s\n", "f16_to_f32_nan_is_nan");
    int prev = test_ret;
    test_ret = 0;
    test_f16_to_f32_nan_is_nan();
    if (!test_ret)
    {
      i_log_passed("%s\n", "f16_to_f32_nan_is_nan");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "f16_to_f32_nan_is_nan";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:70 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:77 START
  if (!filter || strstr("f16_to_f32_smallest_subnormal_correct_value", filter))
  {
    extern void test_f16_to_f32_smallest_subnormal_correct_value(void);
    i_log_info("========================= TEST CASE: %s\n", "f16_to_f32_smallest_subnormal_correct_value");
    int prev = test_ret;
    test_ret = 0;
    test_f16_to_f32_smallest_subnormal_correct_value();
    if (!test_ret)
    {
      i_log_passed("%s\n", "f16_to_f32_smallest_subnormal_correct_value");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "f16_to_f32_smallest_subnormal_correct_value";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:77 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:109 START
  if (!filter || strstr("parse_i32_boundary_values", filter))
  {
    extern void test_parse_i32_boundary_values(void);
    i_log_info("========================= TEST CASE: %s\n", "parse_i32_boundary_values");
    int prev = test_ret;
    test_ret = 0;
    test_parse_i32_boundary_values();
    if (!test_ret)
    {
      i_log_passed("%s\n", "parse_i32_boundary_values");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "parse_i32_boundary_values";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:109 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:163 START
  if (!filter || strstr("parse_i64_boundary_values", filter))
  {
    extern void test_parse_i64_boundary_values(void);
    i_log_info("========================= TEST CASE: %s\n", "parse_i64_boundary_values");
    int prev = test_ret;
    test_ret = 0;
    test_parse_i64_boundary_values();
    if (!test_ret)
    {
      i_log_passed("%s\n", "parse_i64_boundary_values");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "parse_i64_boundary_values";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:163 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:198 START
  if (!filter || strstr("ext_array_capacity_doubles_on_growth", filter))
  {
    extern void test_ext_array_capacity_doubles_on_growth(void);
    i_log_info("========================= TEST CASE: %s\n", "ext_array_capacity_doubles_on_growth");
    int prev = test_ret;
    test_ret = 0;
    test_ext_array_capacity_doubles_on_growth();
    if (!test_ret)
    {
      i_log_passed("%s\n", "ext_array_capacity_doubles_on_growth");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ext_array_capacity_doubles_on_growth";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:198 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:229 START
  if (!filter || strstr("ext_array_remove_all_produces_empty", filter))
  {
    extern void test_ext_array_remove_all_produces_empty(void);
    i_log_info("========================= TEST CASE: %s\n", "ext_array_remove_all_produces_empty");
    int prev = test_ret;
    test_ret = 0;
    test_ext_array_remove_all_produces_empty();
    if (!test_ret)
    {
      i_log_passed("%s\n", "ext_array_remove_all_produces_empty");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ext_array_remove_all_produces_empty";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:229 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:268 START
  if (!filter || strstr("llist_append_maintaififo_order", filter))
  {
    extern void test_llist_append_maintaififo_order(void);
    i_log_info("========================= TEST CASE: %s\n", "llist_append_maintaififo_order");
    int prev = test_ret;
    test_ret = 0;
    test_llist_append_maintaififo_order();
    if (!test_ret)
    {
      i_log_passed("%s\n", "llist_append_maintaififo_order");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "llist_append_maintaififo_order";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:268 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:292 START
  if (!filter || strstr("llist_find_returnode_and_index", filter))
  {
    extern void test_llist_find_returnode_and_index(void);
    i_log_info("========================= TEST CASE: %s\n", "llist_find_returnode_and_index");
    int prev = test_ret;
    test_ret = 0;
    test_llist_find_returnode_and_index();
    if (!test_ret)
    {
      i_log_passed("%s\n", "llist_find_returnode_and_index");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "llist_find_returnode_and_index";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:292 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:326 START
  if (!filter || strstr("llist_remove_from_head_middle_tail", filter))
  {
    extern void test_llist_remove_from_head_middle_tail(void);
    i_log_info("========================= TEST CASE: %s\n", "llist_remove_from_head_middle_tail");
    int prev = test_ret;
    test_ret = 0;
    test_llist_remove_from_head_middle_tail();
    if (!test_ret)
    {
      i_log_passed("%s\n", "llist_remove_from_head_middle_tail");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "llist_remove_from_head_middle_tail";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:326 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:363 START
  if (!filter || strstr("llist_remove_absent_node_is_noop", filter))
  {
    extern void test_llist_remove_absent_node_is_noop(void);
    i_log_info("========================= TEST CASE: %s\n", "llist_remove_absent_node_is_noop");
    int prev = test_ret;
    test_ret = 0;
    test_llist_remove_absent_node_is_noop();
    if (!test_ret)
    {
      i_log_passed("%s\n", "llist_remove_absent_node_is_noop");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "llist_remove_absent_node_is_noop";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:363 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:385 START
  if (!filter || strstr("checksum_known_crc32c_vector", filter))
  {
    extern void test_checksum_known_crc32c_vector(void);
    i_log_info("========================= TEST CASE: %s\n", "checksum_known_crc32c_vector");
    int prev = test_ret;
    test_ret = 0;
    test_checksum_known_crc32c_vector();
    if (!test_ret)
    {
      i_log_passed("%s\n", "checksum_known_crc32c_vector");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "checksum_known_crc32c_vector";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:385 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:394 START
  if (!filter || strstr("checksum_distinct_bytes_differ", filter))
  {
    extern void test_checksum_distinct_bytes_differ(void);
    i_log_info("========================= TEST CASE: %s\n", "checksum_distinct_bytes_differ");
    int prev = test_ret;
    test_ret = 0;
    test_checksum_distinct_bytes_differ();
    if (!test_ret)
    {
      i_log_passed("%s\n", "checksum_distinct_bytes_differ");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "checksum_distinct_bytes_differ";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:394 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:407 START
  if (!filter || strstr("serializer_write_at_capacity_then_overflow", filter))
  {
    extern void test_serializer_write_at_capacity_then_overflow(void);
    i_log_info("========================= TEST CASE: %s\n", "serializer_write_at_capacity_then_overflow");
    int prev = test_ret;
    test_ret = 0;
    test_serializer_write_at_capacity_then_overflow();
    if (!test_ret)
    {
      i_log_passed("%s\n", "serializer_write_at_capacity_then_overflow");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "serializer_write_at_capacity_then_overflow";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:407 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:426 START
  if (!filter || strstr("serializer_incremental_write_overflow", filter))
  {
    extern void test_serializer_incremental_write_overflow(void);
    i_log_info("========================= TEST CASE: %s\n", "serializer_incremental_write_overflow");
    int prev = test_ret;
    test_ret = 0;
    test_serializer_incremental_write_overflow();
    if (!test_ret)
    {
      i_log_passed("%s\n", "serializer_incremental_write_overflow");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "serializer_incremental_write_overflow";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:426 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:445 START
  if (!filter || strstr("stride_constructors_resolve_correctly", filter))
  {
    extern void test_stride_constructors_resolve_correctly(void);
    i_log_info("========================= TEST CASE: %s\n", "stride_constructors_resolve_correctly");
    int prev = test_ret;
    test_ret = 0;
    test_stride_constructors_resolve_correctly();
    if (!test_ret)
    {
      i_log_passed("%s\n", "stride_constructors_resolve_correctly");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "stride_constructors_resolve_correctly";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:445 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:520 START
  if (!filter || strstr("string_ordering_operators", filter))
  {
    extern void test_string_ordering_operators(void);
    i_log_info("========================= TEST CASE: %s\n", "string_ordering_operators");
    int prev = test_ret;
    test_ret = 0;
    test_string_ordering_operators();
    if (!test_ret)
    {
      i_log_passed("%s\n", "string_ordering_operators");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "string_ordering_operators";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:520 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:561 START
  if (!filter || strstr("line_length_newline_found", filter))
  {
    extern void test_line_length_newline_found(void);
    i_log_info("========================= TEST CASE: %s\n", "line_length_newline_found");
    int prev = test_ret;
    test_ret = 0;
    test_line_length_newline_found();
    if (!test_ret)
    {
      i_log_passed("%s\n", "line_length_newline_found");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "line_length_newline_found";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:561 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:618 START
  if (!filter || strstr("string_equal_cases", filter))
  {
    extern void test_string_equal_cases(void);
    i_log_info("========================= TEST CASE: %s\n", "string_equal_cases");
    int prev = test_ret;
    test_ret = 0;
    test_string_equal_cases();
    if (!test_ret)
    {
      i_log_passed("%s\n", "string_equal_cases");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "string_equal_cases";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:618 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:656 START
  if (!filter || strstr("strings_are_disjoint_cases", filter))
  {
    extern void test_strings_are_disjoint_cases(void);
    i_log_info("========================= TEST CASE: %s\n", "strings_are_disjoint_cases");
    int prev = test_ret;
    test_ret = 0;
    test_strings_are_disjoint_cases();
    if (!test_ret)
    {
      i_log_passed("%s\n", "strings_are_disjoint_cases");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "strings_are_disjoint_cases";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:656 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:725 START
  if (!filter || strstr("string_plus_concatenates", filter))
  {
    extern void test_string_plus_concatenates(void);
    i_log_info("========================= TEST CASE: %s\n", "string_plus_concatenates");
    int prev = test_ret;
    test_ret = 0;
    test_string_plus_concatenates();
    if (!test_ret)
    {
      i_log_passed("%s\n", "string_plus_concatenates");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "string_plus_concatenates";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:725 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:776 START
  if (!filter || strstr("cbuffer_discard_all_resets_state", filter))
  {
    extern void test_cbuffer_discard_all_resets_state(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_discard_all_resets_state");
    int prev = test_ret;
    test_ret = 0;
    test_cbuffer_discard_all_resets_state();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_discard_all_resets_state");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_discard_all_resets_state";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:776 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:813 START
  if (!filter || strstr("cbuffer_read_write_wraparound", filter))
  {
    extern void test_cbuffer_read_write_wraparound(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_read_write_wraparound");
    int prev = test_ret;
    test_ret = 0;
    test_cbuffer_read_write_wraparound();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_read_write_wraparound");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_read_write_wraparound";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:813 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:872 START
  if (!filter || strstr("cbuffer_cbuffer_move_transfers_bytes", filter))
  {
    extern void test_cbuffer_cbuffer_move_transfers_bytes(void);
    i_log_info("========================= TEST CASE: %s\n", "cbuffer_cbuffer_move_transfers_bytes");
    int prev = test_ret;
    test_ret = 0;
    test_cbuffer_cbuffer_move_transfers_bytes();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cbuffer_cbuffer_move_transfers_bytes");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cbuffer_cbuffer_move_transfers_bytes";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\core_extra_tests.c:872 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\dbl_buffer.c:115 START
  if (!filter || strstr("dblb_create_basic", filter))
  {
    extern void test_dblb_create_basic(void);
    i_log_info("========================= TEST CASE: %s\n", "dblb_create_basic");
    int prev = test_ret;
    test_ret = 0;
    test_dblb_create_basic();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dblb_create_basic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_create_basic";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\dbl_buffer.c:115 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\dbl_buffer.c:131 START
  if (!filter || strstr("dblb_append_single", filter))
  {
    extern void test_dblb_append_single(void);
    i_log_info("========================= TEST CASE: %s\n", "dblb_append_single");
    int prev = test_ret;
    test_ret = 0;
    test_dblb_append_single();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dblb_append_single");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_append_single";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\dbl_buffer.c:131 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\dbl_buffer.c:152 START
  if (!filter || strstr("dblb_append_multiple", filter))
  {
    extern void test_dblb_append_multiple(void);
    i_log_info("========================= TEST CASE: %s\n", "dblb_append_multiple");
    int prev = test_ret;
    test_ret = 0;
    test_dblb_append_multiple();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dblb_append_multiple");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_append_multiple";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\dbl_buffer.c:152 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\dbl_buffer.c:174 START
  if (!filter || strstr("dblb_append_triggers_realloc", filter))
  {
    extern void test_dblb_append_triggers_realloc(void);
    i_log_info("========================= TEST CASE: %s\n", "dblb_append_triggers_realloc");
    int prev = test_ret;
    test_ret = 0;
    test_dblb_append_triggers_realloc();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dblb_append_triggers_realloc");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_append_triggers_realloc";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\dbl_buffer.c:174 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\dbl_buffer.c:211 START
  if (!filter || strstr("dblb_append_alloc_basic", filter))
  {
    extern void test_dblb_append_alloc_basic(void);
    i_log_info("========================= TEST CASE: %s\n", "dblb_append_alloc_basic");
    int prev = test_ret;
    test_ret = 0;
    test_dblb_append_alloc_basic();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dblb_append_alloc_basic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_append_alloc_basic";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\dbl_buffer.c:211 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\dbl_buffer.c:236 START
  if (!filter || strstr("dblb_append_alloc_sequential", filter))
  {
    extern void test_dblb_append_alloc_sequential(void);
    i_log_info("========================= TEST CASE: %s\n", "dblb_append_alloc_sequential");
    int prev = test_ret;
    test_ret = 0;
    test_dblb_append_alloc_sequential();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dblb_append_alloc_sequential");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_append_alloc_sequential";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\dbl_buffer.c:236 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\dbl_buffer.c:267 START
  if (!filter || strstr("dblb_append_alloc_triggers_realloc", filter))
  {
    extern void test_dblb_append_alloc_triggers_realloc(void);
    i_log_info("========================= TEST CASE: %s\n", "dblb_append_alloc_triggers_realloc");
    int prev = test_ret;
    test_ret = 0;
    test_dblb_append_alloc_triggers_realloc();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dblb_append_alloc_triggers_realloc");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_append_alloc_triggers_realloc";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\dbl_buffer.c:267 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\dbl_buffer.c:287 START
  if (!filter || strstr("dblb_different_element_sizes", filter))
  {
    extern void test_dblb_different_element_sizes(void);
    i_log_info("========================= TEST CASE: %s\n", "dblb_different_element_sizes");
    int prev = test_ret;
    test_ret = 0;
    test_dblb_different_element_sizes();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dblb_different_element_sizes");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_different_element_sizes";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\dbl_buffer.c:287 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\dbl_buffer.c:307 START
  if (!filter || strstr("dblb_struct_elements", filter))
  {
    extern void test_dblb_struct_elements(void);
    i_log_info("========================= TEST CASE: %s\n", "dblb_struct_elements");
    int prev = test_ret;
    test_ret = 0;
    test_dblb_struct_elements();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dblb_struct_elements");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_struct_elements";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\dbl_buffer.c:307 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\dbl_buffer.c:335 START
  if (!filter || strstr("dblb_free_resets", filter))
  {
    extern void test_dblb_free_resets(void);
    i_log_info("========================= TEST CASE: %s\n", "dblb_free_resets");
    int prev = test_ret;
    test_ret = 0;
    test_dblb_free_resets();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dblb_free_resets");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_free_resets";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\dbl_buffer.c:335 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\dbl_buffer.c:351 START
  if (!filter || strstr("dblb_large_append", filter))
  {
    extern void test_dblb_large_append(void);
    i_log_info("========================= TEST CASE: %s\n", "dblb_large_append");
    int prev = test_ret;
    test_ret = 0;
    test_dblb_large_append();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dblb_large_append");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dblb_large_append";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\dbl_buffer.c:351 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\dirty_page_table_tests.c:18 START
  if (!filter || strstr("dpgt_open", filter))
  {
    extern void test_dpgt_open(void);
    i_log_info("========================= TEST CASE: %s\n", "dpgt_open");
    int prev = test_ret;
    test_ret = 0;
    test_dpgt_open();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dpgt_open");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dpgt_open";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\dirty_page_table_tests.c:18 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\dirty_page_table_tests.c:38 START
  if (!filter || strstr("dpgt_merge_into", filter))
  {
    extern void test_dpgt_merge_into(void);
    i_log_info("========================= TEST CASE: %s\n", "dpgt_merge_into");
    int prev = test_ret;
    test_ret = 0;
    test_dpgt_merge_into();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dpgt_merge_into");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dpgt_merge_into";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\dirty_page_table_tests.c:38 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\dirty_page_table_tests.c:103 START
  if (!filter || strstr("dpgt_min_rec_lsn", filter))
  {
    extern void test_dpgt_min_rec_lsn(void);
    i_log_info("========================= TEST CASE: %s\n", "dpgt_min_rec_lsn");
    int prev = test_ret;
    test_ret = 0;
    test_dpgt_min_rec_lsn();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dpgt_min_rec_lsn");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dpgt_min_rec_lsn";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\dirty_page_table_tests.c:103 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\dirty_page_table_tests.c:133 START
  if (!filter || strstr("dpgt_exists", filter))
  {
    extern void test_dpgt_exists(void);
    i_log_info("========================= TEST CASE: %s\n", "dpgt_exists");
    int prev = test_ret;
    test_ret = 0;
    test_dpgt_exists();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dpgt_exists");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dpgt_exists";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\dirty_page_table_tests.c:133 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\dirty_page_table_tests.c:156 START
  if (!filter || strstr("dpgt_add", filter))
  {
    extern void test_dpgt_add(void);
    i_log_info("========================= TEST CASE: %s\n", "dpgt_add");
    int prev = test_ret;
    test_ret = 0;
    test_dpgt_add();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dpgt_add");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dpgt_add";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\dirty_page_table_tests.c:156 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\dirty_page_table_tests.c:193 START
  if (!filter || strstr("dpgt_get", filter))
  {
    extern void test_dpgt_get(void);
    i_log_info("========================= TEST CASE: %s\n", "dpgt_get");
    int prev = test_ret;
    test_ret = 0;
    test_dpgt_get();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dpgt_get");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dpgt_get";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\dirty_page_table_tests.c:193 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\dirty_page_table_tests.c:259 START
  if (!filter || strstr("dpgt_remove", filter))
  {
    extern void test_dpgt_remove(void);
    i_log_info("========================= TEST CASE: %s\n", "dpgt_remove");
    int prev = test_ret;
    test_ret = 0;
    test_dpgt_remove();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dpgt_remove");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dpgt_remove";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\dirty_page_table_tests.c:259 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\dirty_page_table_tests.c:323 START
  if (!filter || strstr("dpgt_serialize", filter))
  {
    extern void test_dpgt_serialize(void);
    i_log_info("========================= TEST CASE: %s\n", "dpgt_serialize");
    int prev = test_ret;
    test_ret = 0;
    test_dpgt_serialize();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dpgt_serialize");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dpgt_serialize";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\dirty_page_table_tests.c:323 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\dirty_page_table_tests.c:387 START
  if (!filter || strstr("dpgt_equal", filter))
  {
    extern void test_dpgt_equal(void);
    i_log_info("========================= TEST CASE: %s\n", "dpgt_equal");
    int prev = test_ret;
    test_ret = 0;
    test_dpgt_equal();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dpgt_equal");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dpgt_equal";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\dirty_page_table_tests.c:387 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\dpgt_concurrency_tests.c:99 START
  if (!filter || strstr("dpgt_concurrent", filter))
  {
    extern void test_dpgt_concurrent(void);
    i_log_info("========================= TEST CASE: %s\n", "dpgt_concurrent");
    int prev = test_ret;
    test_ret = 0;
    test_dpgt_concurrent();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dpgt_concurrent");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dpgt_concurrent";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\dpgt_concurrency_tests.c:99 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\ext_array.c:268 START
  if (!filter || strstr("ext_array_insert_read", filter))
  {
    extern void test_ext_array_insert_read(void);
    i_log_info("========================= TEST CASE: %s\n", "ext_array_insert_read");
    int prev = test_ret;
    test_ret = 0;
    test_ext_array_insert_read();
    if (!test_ret)
    {
      i_log_passed("%s\n", "ext_array_insert_read");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ext_array_insert_read";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\ext_array.c:268 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\ext_array.c:407 START
  if (!filter || strstr("ext_array_write", filter))
  {
    extern void test_ext_array_write(void);
    i_log_info("========================= TEST CASE: %s\n", "ext_array_write");
    int prev = test_ret;
    test_ret = 0;
    test_ext_array_write();
    if (!test_ret)
    {
      i_log_passed("%s\n", "ext_array_write");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ext_array_write";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\ext_array.c:407 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\ext_array.c:513 START
  if (!filter || strstr("ext_array_remove", filter))
  {
    extern void test_ext_array_remove(void);
    i_log_info("========================= TEST CASE: %s\n", "ext_array_remove");
    int prev = test_ret;
    test_ret = 0;
    test_ext_array_remove();
    if (!test_ret)
    {
      i_log_passed("%s\n", "ext_array_remove");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ext_array_remove";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\ext_array.c:513 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\ext_array.c:680 START
  if (!filter || strstr("ext_array_random", filter))
  {
    extern void test_ext_array_random(void);
    i_log_info("========================= TEST CASE: %s\n", "ext_array_random");
    int prev = test_ret;
    test_ret = 0;
    test_ext_array_random();
    if (!test_ret)
    {
      i_log_passed("%s\n", "ext_array_random");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ext_array_random";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\ext_array.c:680 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\file_pager.c:122 START
  if (!filter || strstr("fpgr_open", filter))
  {
    extern void test_fpgr_open(void);
    i_log_info("========================= TEST CASE: %s\n", "fpgr_open");
    int prev = test_ret;
    test_ret = 0;
    test_fpgr_open();
    if (!test_ret)
    {
      i_log_passed("%s\n", "fpgr_open");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "fpgr_open";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\file_pager.c:122 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\file_pager.c:231 START
  if (!filter || strstr("fpgr_new", filter))
  {
    extern void test_fpgr_new(void);
    i_log_info("========================= TEST CASE: %s\n", "fpgr_new");
    int prev = test_ret;
    test_ret = 0;
    test_fpgr_new();
    if (!test_ret)
    {
      i_log_passed("%s\n", "fpgr_new");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "fpgr_new";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\file_pager.c:231 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\file_pager.c:410 START
  if (!filter || strstr("fpgr_read_write", filter))
  {
    extern void test_fpgr_read_write(void);
    i_log_info("========================= TEST CASE: %s\n", "fpgr_read_write");
    int prev = test_ret;
    test_ret = 0;
    test_fpgr_read_write();
    if (!test_ret)
    {
      i_log_passed("%s\n", "fpgr_read_write");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "fpgr_read_write";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\file_pager.c:410 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\filenames.c:28 START
  if (!filter || strstr("file_basename", filter))
  {
    extern void test_file_basename(void);
    i_log_info("========================= TEST CASE: %s\n", "file_basename");
    int prev = test_ret;
    test_ret = 0;
    test_file_basename();
    if (!test_ret)
    {
      i_log_passed("%s\n", "file_basename");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "file_basename";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\filenames.c:28 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\gr_lock.c:417 START
  if (!filter || strstr("gr_lock_basic_sanity", filter))
  {
    extern void test_gr_lock_basic_sanity(void);
    i_log_info("========================= TEST CASE: %s\n", "gr_lock_basic_sanity");
    int prev = test_ret;
    test_ret = 0;
    test_gr_lock_basic_sanity();
    if (!test_ret)
    {
      i_log_passed("%s\n", "gr_lock_basic_sanity");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "gr_lock_basic_sanity";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\gr_lock.c:417 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\gr_lock.c:434 START
  if (!filter || strstr("gr_lock_is_is_compatible", filter))
  {
    extern void test_gr_lock_is_is_compatible(void);
    i_log_info("========================= TEST CASE: %s\n", "gr_lock_is_is_compatible");
    int prev = test_ret;
    test_ret = 0;
    test_gr_lock_is_is_compatible();
    if (!test_ret)
    {
      i_log_passed("%s\n", "gr_lock_is_is_compatible");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "gr_lock_is_is_compatible";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\gr_lock.c:434 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\gr_lock.c:490 START
  if (!filter || strstr("gr_lock_high_pressure_random", filter))
  {
    extern void test_gr_lock_high_pressure_random(void);
    i_log_info("========================= TEST CASE: %s\n", "gr_lock_high_pressure_random");
    int prev = test_ret;
    test_ret = 0;
    test_gr_lock_high_pressure_random();
    if (!test_ret)
    {
      i_log_passed("%s\n", "gr_lock_high_pressure_random");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "gr_lock_high_pressure_random";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\gr_lock.c:490 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\hash_table.c:194 START
  if (!filter || strstr("htable", filter))
  {
    extern void test_htable(void);
    i_log_info("========================= TEST CASE: %s\n", "htable");
    int prev = test_ret;
    test_ret = 0;
    test_htable();
    if (!test_ret)
    {
      i_log_passed("%s\n", "htable");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "htable";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\hash_table.c:194 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\hashing.c:31 START
  if (!filter || strstr("fnv1a_hash_empty", filter))
  {
    extern void test_fnv1a_hash_empty(void);
    i_log_info("========================= TEST CASE: %s\n", "fnv1a_hash_empty");
    int prev = test_ret;
    test_ret = 0;
    test_fnv1a_hash_empty();
    if (!test_ret)
    {
      i_log_passed("%s\n", "fnv1a_hash_empty");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "fnv1a_hash_empty";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\hashing.c:31 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\hashing.c:39 START
  if (!filter || strstr("fnv1a_hash_single_char", filter))
  {
    extern void test_fnv1a_hash_single_char(void);
    i_log_info("========================= TEST CASE: %s\n", "fnv1a_hash_single_char");
    int prev = test_ret;
    test_ret = 0;
    test_fnv1a_hash_single_char();
    if (!test_ret)
    {
      i_log_passed("%s\n", "fnv1a_hash_single_char");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "fnv1a_hash_single_char";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\hashing.c:39 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\hashing.c:48 START
  if (!filter || strstr("fnv1a_hash_known_value", filter))
  {
    extern void test_fnv1a_hash_known_value(void);
    i_log_info("========================= TEST CASE: %s\n", "fnv1a_hash_known_value");
    int prev = test_ret;
    test_ret = 0;
    test_fnv1a_hash_known_value();
    if (!test_ret)
    {
      i_log_passed("%s\n", "fnv1a_hash_known_value");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "fnv1a_hash_known_value";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\hashing.c:48 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\hashing.c:56 START
  if (!filter || strstr("fnv1a_hash_deterministic", filter))
  {
    extern void test_fnv1a_hash_deterministic(void);
    i_log_info("========================= TEST CASE: %s\n", "fnv1a_hash_deterministic");
    int prev = test_ret;
    test_ret = 0;
    test_fnv1a_hash_deterministic();
    if (!test_ret)
    {
      i_log_passed("%s\n", "fnv1a_hash_deterministic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "fnv1a_hash_deterministic";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\hashing.c:56 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\kvt_list_builder.c:200 START
  if (!filter || strstr("kvt_list_builder", filter))
  {
    extern void test_kvt_list_builder(void);
    i_log_info("========================= TEST CASE: %s\n", "kvt_list_builder");
    int prev = test_ret;
    test_ret = 0;
    test_kvt_list_builder();
    if (!test_ret)
    {
      i_log_passed("%s\n", "kvt_list_builder");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "kvt_list_builder";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\kvt_list_builder.c:200 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\lalloc.c:126 START
  if (!filter || strstr("lalloc_edge_cases", filter))
  {
    extern void test_lalloc_edge_cases(void);
    i_log_info("========================= TEST CASE: %s\n", "lalloc_edge_cases");
    int prev = test_ret;
    test_ret = 0;
    test_lalloc_edge_cases();
    if (!test_ret)
    {
      i_log_passed("%s\n", "lalloc_edge_cases");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lalloc_edge_cases";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\lalloc.c:126 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\latch.c:40 START
  if (!filter || strstr("latch", filter))
  {
    extern void test_latch(void);
    i_log_info("========================= TEST CASE: %s\n", "latch");
    int prev = test_ret;
    test_ret = 0;
    test_latch();
    if (!test_ret)
    {
      i_log_passed("%s\n", "latch");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "latch";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\latch.c:40 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\lexer.c:561 START
  if (!filter || strstr("lexer_two_char_tokens", filter))
  {
    extern void test_lexer_two_char_tokens(void);
    i_log_info("========================= TEST CASE: %s\n", "lexer_two_char_tokens");
    int prev = test_ret;
    test_ret = 0;
    test_lexer_two_char_tokens();
    if (!test_ret)
    {
      i_log_passed("%s\n", "lexer_two_char_tokens");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_two_char_tokens";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\lexer.c:561 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\lexer.c:587 START
  if (!filter || strstr("lexer_single_char_operators", filter))
  {
    extern void test_lexer_single_char_operators(void);
    i_log_info("========================= TEST CASE: %s\n", "lexer_single_char_operators");
    int prev = test_ret;
    test_ret = 0;
    test_lexer_single_char_operators();
    if (!test_ret)
    {
      i_log_passed("%s\n", "lexer_single_char_operators");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_single_char_operators";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\lexer.c:587 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\lexer.c:615 START
  if (!filter || strstr("lexer_strings", filter))
  {
    extern void test_lexer_strings(void);
    i_log_info("========================= TEST CASE: %s\n", "lexer_strings");
    int prev = test_ret;
    test_ret = 0;
    test_lexer_strings();
    if (!test_ret)
    {
      i_log_passed("%s\n", "lexer_strings");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_strings";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\lexer.c:615 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\lexer.c:628 START
  if (!filter || strstr("lexer_identifiers", filter))
  {
    extern void test_lexer_identifiers(void);
    i_log_info("========================= TEST CASE: %s\n", "lexer_identifiers");
    int prev = test_ret;
    test_ret = 0;
    test_lexer_identifiers();
    if (!test_ret)
    {
      i_log_passed("%s\n", "lexer_identifiers");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_identifiers";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\lexer.c:628 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\lexer.c:642 START
  if (!filter || strstr("lexer_numbers", filter))
  {
    extern void test_lexer_numbers(void);
    i_log_info("========================= TEST CASE: %s\n", "lexer_numbers");
    int prev = test_ret;
    test_ret = 0;
    test_lexer_numbers();
    if (!test_ret)
    {
      i_log_passed("%s\n", "lexer_numbers");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_numbers";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\lexer.c:642 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\lexer.c:658 START
  if (!filter || strstr("lexer_keywords", filter))
  {
    extern void test_lexer_keywords(void);
    i_log_info("========================= TEST CASE: %s\n", "lexer_keywords");
    int prev = test_ret;
    test_ret = 0;
    test_lexer_keywords();
    if (!test_ret)
    {
      i_log_passed("%s\n", "lexer_keywords");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_keywords";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\lexer.c:658 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\lexer.c:677 START
  if (!filter || strstr("lexer_primitives", filter))
  {
    extern void test_lexer_primitives(void);
    i_log_info("========================= TEST CASE: %s\n", "lexer_primitives");
    int prev = test_ret;
    test_ret = 0;
    test_lexer_primitives();
    if (!test_ret)
    {
      i_log_passed("%s\n", "lexer_primitives");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_primitives";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\lexer.c:677 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\lexer.c:697 START
  if (!filter || strstr("lexer_whitespace_handling", filter))
  {
    extern void test_lexer_whitespace_handling(void);
    i_log_info("========================= TEST CASE: %s\n", "lexer_whitespace_handling");
    int prev = test_ret;
    test_ret = 0;
    test_lexer_whitespace_handling();
    if (!test_ret)
    {
      i_log_passed("%s\n", "lexer_whitespace_handling");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_whitespace_handling";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\lexer.c:697 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\lexer.c:715 START
  if (!filter || strstr("lexer_complex_expression", filter))
  {
    extern void test_lexer_complex_expression(void);
    i_log_info("========================= TEST CASE: %s\n", "lexer_complex_expression");
    int prev = test_ret;
    test_ret = 0;
    test_lexer_complex_expression();
    if (!test_ret)
    {
      i_log_passed("%s\n", "lexer_complex_expression");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_complex_expression";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\lexer.c:715 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\lexer.c:737 START
  if (!filter || strstr("lexer_keyword_prefix", filter))
  {
    extern void test_lexer_keyword_prefix(void);
    i_log_info("========================= TEST CASE: %s\n", "lexer_keyword_prefix");
    int prev = test_ret;
    test_ret = 0;
    test_lexer_keyword_prefix();
    if (!test_ret)
    {
      i_log_passed("%s\n", "lexer_keyword_prefix");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_keyword_prefix";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\lexer.c:737 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\lexer.c:753 START
  if (!filter || strstr("lexer_errors", filter))
  {
    extern void test_lexer_errors(void);
    i_log_info("========================= TEST CASE: %s\n", "lexer_errors");
    int prev = test_ret;
    test_ret = 0;
    test_lexer_errors();
    if (!test_ret)
    {
      i_log_passed("%s\n", "lexer_errors");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_errors";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\lexer.c:753 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\lexer.c:765 START
  if (!filter || strstr("lexer_empty_string", filter))
  {
    extern void test_lexer_empty_string(void);
    i_log_info("========================= TEST CASE: %s\n", "lexer_empty_string");
    int prev = test_ret;
    test_ret = 0;
    test_lexer_empty_string();
    if (!test_ret)
    {
      i_log_passed("%s\n", "lexer_empty_string");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_empty_string";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\lexer.c:765 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\lexer.c:776 START
  if (!filter || strstr("lexer_numbers_in_sequence", filter))
  {
    extern void test_lexer_numbers_in_sequence(void);
    i_log_info("========================= TEST CASE: %s\n", "lexer_numbers_in_sequence");
    int prev = test_ret;
    test_ret = 0;
    test_lexer_numbers_in_sequence();
    if (!test_ret)
    {
      i_log_passed("%s\n", "lexer_numbers_in_sequence");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "lexer_numbers_in_sequence";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\lexer.c:776 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\llist.c:24 START
  if (!filter || strstr("llist", filter))
  {
    extern void test_llist(void);
    i_log_info("========================= TEST CASE: %s\n", "llist");
    int prev = test_ret;
    test_ret = 0;
    test_llist();
    if (!test_ret)
    {
      i_log_passed("%s\n", "llist");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "llist";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\llist.c:24 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\mem_vhmap.c:255 START
  if (!filter || strstr("mem_vhmap", filter))
  {
    extern void test_mem_vhmap(void);
    i_log_info("========================= TEST CASE: %s\n", "mem_vhmap");
    int prev = test_ret;
    test_ret = 0;
    test_mem_vhmap();
    if (!test_ret)
    {
      i_log_passed("%s\n", "mem_vhmap");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "mem_vhmap";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\mem_vhmap.c:255 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\memory.c:68 START
  if (!filter || strstr("i_malloc_injection", filter))
  {
    extern void test_i_malloc_injection(void);
    i_log_info("========================= TEST CASE: %s\n", "i_malloc_injection");
    int prev = test_ret;
    test_ret = 0;
    test_i_malloc_injection();
    if (!test_ret)
    {
      i_log_passed("%s\n", "i_malloc_injection");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_malloc_injection";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\memory.c:68 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\memory.c:162 START
  if (!filter || strstr("i_realloc_basic", filter))
  {
    extern void test_i_realloc_basic(void);
    i_log_info("========================= TEST CASE: %s\n", "i_realloc_basic");
    int prev = test_ret;
    test_ret = 0;
    test_i_realloc_basic();
    if (!test_ret)
    {
      i_log_passed("%s\n", "i_realloc_basic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_realloc_basic";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\memory.c:162 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\memory.c:206 START
  if (!filter || strstr("i_realloc_right", filter))
  {
    extern void test_i_realloc_right(void);
    i_log_info("========================= TEST CASE: %s\n", "i_realloc_right");
    int prev = test_ret;
    test_ret = 0;
    test_i_realloc_right();
    if (!test_ret)
    {
      i_log_passed("%s\n", "i_realloc_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_realloc_right";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\memory.c:206 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\memory.c:305 START
  if (!filter || strstr("i_realloc_left", filter))
  {
    extern void test_i_realloc_left(void);
    i_log_info("========================= TEST CASE: %s\n", "i_realloc_left");
    int prev = test_ret;
    test_ret = 0;
    test_i_realloc_left();
    if (!test_ret)
    {
      i_log_passed("%s\n", "i_realloc_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_realloc_left";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\memory.c:305 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\memory.c:382 START
  if (!filter || strstr("i_crealloc_right", filter))
  {
    extern void test_i_crealloc_right(void);
    i_log_info("========================= TEST CASE: %s\n", "i_crealloc_right");
    int prev = test_ret;
    test_ret = 0;
    test_i_crealloc_right();
    if (!test_ret)
    {
      i_log_passed("%s\n", "i_crealloc_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_crealloc_right";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\memory.c:382 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\memory.c:488 START
  if (!filter || strstr("i_crealloc_left", filter))
  {
    extern void test_i_crealloc_left(void);
    i_log_info("========================= TEST CASE: %s\n", "i_crealloc_left");
    int prev = test_ret;
    test_ret = 0;
    test_i_crealloc_left();
    if (!test_ret)
    {
      i_log_passed("%s\n", "i_crealloc_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_crealloc_left";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\memory.c:488 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\node_updates.c:225 START
  if (!filter || strstr("nupd_init", filter))
  {
    extern void test_nupd_init(void);
    i_log_info("========================= TEST CASE: %s\n", "nupd_init");
    int prev = test_ret;
    test_ret = 0;
    test_nupd_init();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nupd_init");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_init";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\node_updates.c:225 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\node_updates.c:298 START
  if (!filter || strstr("nupd_append_right", filter))
  {
    extern void test_nupd_append_right(void);
    i_log_info("========================= TEST CASE: %s\n", "nupd_append_right");
    int prev = test_ret;
    test_ret = 0;
    test_nupd_append_right();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nupd_append_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_append_right";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\node_updates.c:298 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\node_updates.c:414 START
  if (!filter || strstr("nupd_append_left", filter))
  {
    extern void test_nupd_append_left(void);
    i_log_info("========================= TEST CASE: %s\n", "nupd_append_left");
    int prev = test_ret;
    test_ret = 0;
    test_nupd_append_left();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nupd_append_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_append_left";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\node_updates.c:414 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\node_updates.c:664 START
  if (!filter || strstr("nupd_append_tip_right", filter))
  {
    extern void test_nupd_append_tip_right(void);
    i_log_info("========================= TEST CASE: %s\n", "nupd_append_tip_right");
    int prev = test_ret;
    test_ret = 0;
    test_nupd_append_tip_right();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nupd_append_tip_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_append_tip_right";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\node_updates.c:664 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\node_updates.c:864 START
  if (!filter || strstr("nupd_append_tip_left", filter))
  {
    extern void test_nupd_append_tip_left(void);
    i_log_info("========================= TEST CASE: %s\n", "nupd_append_tip_left");
    int prev = test_ret;
    test_ret = 0;
    test_nupd_append_tip_left();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nupd_append_tip_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_append_tip_left";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\node_updates.c:864 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\node_updates.c:1211 START
  if (!filter || strstr("nupd_consume_right", filter))
  {
    extern void test_nupd_consume_right(void);
    i_log_info("========================= TEST CASE: %s\n", "nupd_consume_right");
    int prev = test_ret;
    test_ret = 0;
    test_nupd_consume_right();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nupd_consume_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_consume_right";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\node_updates.c:1211 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\node_updates.c:1285 START
  if (!filter || strstr("nupd_consume_left", filter))
  {
    extern void test_nupd_consume_left(void);
    i_log_info("========================= TEST CASE: %s\n", "nupd_consume_left");
    int prev = test_ret;
    test_ret = 0;
    test_nupd_consume_left();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nupd_consume_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_consume_left";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\node_updates.c:1285 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\node_updates.c:1355 START
  if (!filter || strstr("nupd_done_observing_left", filter))
  {
    extern void test_nupd_done_observing_left(void);
    i_log_info("========================= TEST CASE: %s\n", "nupd_done_observing_left");
    int prev = test_ret;
    test_ret = 0;
    test_nupd_done_observing_left();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nupd_done_observing_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_done_observing_left";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\node_updates.c:1355 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\node_updates.c:1410 START
  if (!filter || strstr("nupd_done_observing_right", filter))
  {
    extern void test_nupd_done_observing_right(void);
    i_log_info("========================= TEST CASE: %s\n", "nupd_done_observing_right");
    int prev = test_ret;
    test_ret = 0;
    test_nupd_done_observing_right();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nupd_done_observing_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_done_observing_right";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\node_updates.c:1410 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\node_updates.c:1453 START
  if (!filter || strstr("nupd_done_consuming_left", filter))
  {
    extern void test_nupd_done_consuming_left(void);
    i_log_info("========================= TEST CASE: %s\n", "nupd_done_consuming_left");
    int prev = test_ret;
    test_ret = 0;
    test_nupd_done_consuming_left();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nupd_done_consuming_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_done_consuming_left";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\node_updates.c:1453 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\node_updates.c:1497 START
  if (!filter || strstr("nupd_done_consuming_right", filter))
  {
    extern void test_nupd_done_consuming_right(void);
    i_log_info("========================= TEST CASE: %s\n", "nupd_done_consuming_right");
    int prev = test_ret;
    test_ret = 0;
    test_nupd_done_consuming_right();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nupd_done_consuming_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_done_consuming_right";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\node_updates.c:1497 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\node_updates.c:1541 START
  if (!filter || strstr("nupd_done_left", filter))
  {
    extern void test_nupd_done_left(void);
    i_log_info("========================= TEST CASE: %s\n", "nupd_done_left");
    int prev = test_ret;
    test_ret = 0;
    test_nupd_done_left();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nupd_done_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_done_left";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\node_updates.c:1541 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\node_updates.c:1595 START
  if (!filter || strstr("nupd_done_right", filter))
  {
    extern void test_nupd_done_right(void);
    i_log_info("========================= TEST CASE: %s\n", "nupd_done_right");
    int prev = test_ret;
    test_ret = 0;
    test_nupd_done_right();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nupd_done_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nupd_done_right";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\node_updates.c:1595 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\numbers.c:125 START
  if (!filter || strstr("parse_i32_expect", filter))
  {
    extern void test_parse_i32_expect(void);
    i_log_info("========================= TEST CASE: %s\n", "parse_i32_expect");
    int prev = test_ret;
    test_ret = 0;
    test_parse_i32_expect();
    if (!test_ret)
    {
      i_log_passed("%s\n", "parse_i32_expect");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "parse_i32_expect";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\numbers.c:125 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\numbers.c:276 START
  if (!filter || strstr("parse_f32_expect", filter))
  {
    extern void test_parse_f32_expect(void);
    i_log_info("========================= TEST CASE: %s\n", "parse_f32_expect");
    int prev = test_ret;
    test_ret = 0;
    test_parse_f32_expect();
    if (!test_ret)
    {
      i_log_passed("%s\n", "parse_f32_expect");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "parse_f32_expect";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\numbers.c:276 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\numbers.c:316 START
  if (!filter || strstr("py_mod_f32", filter))
  {
    extern void test_py_mod_f32(void);
    i_log_info("========================= TEST CASE: %s\n", "py_mod_f32");
    int prev = test_ret;
    test_ret = 0;
    test_py_mod_f32();
    if (!test_ret)
    {
      i_log_passed("%s\n", "py_mod_f32");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "py_mod_f32";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\numbers.c:316 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\numbers.c:353 START
  if (!filter || strstr("py_mod_i32", filter))
  {
    extern void test_py_mod_i32(void);
    i_log_info("========================= TEST CASE: %s\n", "py_mod_i32");
    int prev = test_ret;
    test_ret = 0;
    test_py_mod_i32();
    if (!test_ret)
    {
      i_log_passed("%s\n", "py_mod_i32");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "py_mod_i32";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\numbers.c:353 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\numstore_regression_tests.c:24 START
  if (!filter || strstr("cgd_test_create_delete_rollback_delete", filter))
  {
    extern void test_cgd_test_create_delete_rollback_delete(void);
    i_log_info("========================= TEST CASE: %s\n", "cgd_test_create_delete_rollback_delete");
    int prev = test_ret;
    test_ret = 0;
    test_cgd_test_create_delete_rollback_delete();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cgd_test_create_delete_rollback_delete");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cgd_test_create_delete_rollback_delete";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\numstore_regression_tests.c:24 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\numstore_regression_tests.c:70 START
  if (!filter || strstr("cgd_test_create_crash_close_delete", filter))
  {
    extern void test_cgd_test_create_crash_close_delete(void);
    i_log_info("========================= TEST CASE: %s\n", "cgd_test_create_crash_close_delete");
    int prev = test_ret;
    test_ret = 0;
    test_cgd_test_create_crash_close_delete();
    if (!test_ret)
    {
      i_log_passed("%s\n", "cgd_test_create_crash_close_delete");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "cgd_test_create_crash_close_delete";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\numstore_regression_tests.c:70 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\numstore_regression_tests.c:100 START
  if (!filter || strstr("irwr_rollback_invalid_wal_header", filter))
  {
    extern void test_irwr_rollback_invalid_wal_header(void);
    i_log_info("========================= TEST CASE: %s\n", "irwr_rollback_invalid_wal_header");
    int prev = test_ret;
    test_ret = 0;
    test_irwr_rollback_invalid_wal_header();
    if (!test_ret)
    {
      i_log_passed("%s\n", "irwr_rollback_invalid_wal_header");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "irwr_rollback_invalid_wal_header";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\numstore_regression_tests.c:100 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\numstore_tests.c:26 START
  if (!filter || strstr("nsdb_create_txn", filter))
  {
    extern void test_nsdb_create_txn(void);
    i_log_info("========================= TEST CASE: %s\n", "nsdb_create_txn");
    int prev = test_ret;
    test_ret = 0;
    test_nsdb_create_txn();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nsdb_create_txn");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nsdb_create_txn";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\numstore_tests.c:26 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\numstore_tests.c:181 START
  if (!filter || strstr("nsdb_delete_txn", filter))
  {
    extern void test_nsdb_delete_txn(void);
    i_log_info("========================= TEST CASE: %s\n", "nsdb_delete_txn");
    int prev = test_ret;
    test_ret = 0;
    test_nsdb_delete_txn();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nsdb_delete_txn");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nsdb_delete_txn";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\numstore_tests.c:181 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\numstore_tests.c:289 START
  if (!filter || strstr("nsdb_insert_txn", filter))
  {
    extern void test_nsdb_insert_txn(void);
    i_log_info("========================= TEST CASE: %s\n", "nsdb_insert_txn");
    int prev = test_ret;
    test_ret = 0;
    test_nsdb_insert_txn();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nsdb_insert_txn");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nsdb_insert_txn";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\numstore_tests.c:289 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\numstore_tests.c:505 START
  if (!filter || strstr("nsdb_write_txn", filter))
  {
    extern void test_nsdb_write_txn(void);
    i_log_info("========================= TEST CASE: %s\n", "nsdb_write_txn");
    int prev = test_ret;
    test_ret = 0;
    test_nsdb_write_txn();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nsdb_write_txn");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nsdb_write_txn";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\numstore_tests.c:505 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\numstore_tests.c:764 START
  if (!filter || strstr("nsdb_get_if_exists", filter))
  {
    extern void test_nsdb_get_if_exists(void);
    i_log_info("========================= TEST CASE: %s\n", "nsdb_get_if_exists");
    int prev = test_ret;
    test_ret = 0;
    test_nsdb_get_if_exists();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nsdb_get_if_exists");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nsdb_get_if_exists";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\numstore_tests.c:764 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pager\page_fixture.c:298 START
  if (!filter || strstr("build_page_tree", filter))
  {
    extern void test_build_page_tree(void);
    i_log_info("========================= TEST CASE: %s\n", "build_page_tree");
    int prev = test_ret;
    test_ret = 0;
    test_build_page_tree();
    if (!test_ret)
    {
      i_log_passed("%s\n", "build_page_tree");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "build_page_tree";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pager\page_fixture.c:298 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pager\pager.c:182 START
  if (!filter || strstr("pager_fill_ht", filter))
  {
    extern void test_pager_fill_ht(void);
    i_log_info("========================= TEST CASE: %s\n", "pager_fill_ht");
    int prev = test_ret;
    test_ret = 0;
    test_pager_fill_ht();
    if (!test_ret)
    {
      i_log_passed("%s\n", "pager_fill_ht");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "pager_fill_ht";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pager\pager.c:182 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pager\pager.c:236 START
  if (!filter || strstr("wal_int", filter))
  {
    extern void test_wal_int(void);
    i_log_info("========================= TEST CASE: %s\n", "wal_int");
    int prev = test_ret;
    test_ret = 0;
    test_wal_int();
    if (!test_ret)
    {
      i_log_passed("%s\n", "wal_int");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "wal_int";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pager\pager.c:236 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pager\pgr_close.c:79 START
  if (!filter || strstr("pgr_close_success", filter))
  {
    extern void test_pgr_close_success(void);
    i_log_info("========================= TEST CASE: %s\n", "pgr_close_success");
    int prev = test_ret;
    test_ret = 0;
    test_pgr_close_success();
    if (!test_ret)
    {
      i_log_passed("%s\n", "pgr_close_success");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "pgr_close_success";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pager\pgr_close.c:79 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pager\pgr_delete_and_release.c:85 START
  if (!filter || strstr("pgr_delete", filter))
  {
    extern void test_pgr_delete(void);
    i_log_info("========================= TEST CASE: %s\n", "pgr_delete");
    int prev = test_ret;
    test_ret = 0;
    test_pgr_delete();
    if (!test_ret)
    {
      i_log_passed("%s\n", "pgr_delete");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "pgr_delete";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pager\pgr_delete_and_release.c:85 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pager\pgr_get.c:114 START
  if (!filter || strstr("pgr_get_invalid_checksum", filter))
  {
    extern void test_pgr_get_invalid_checksum(void);
    i_log_info("========================= TEST CASE: %s\n", "pgr_get_invalid_checksum");
    int prev = test_ret;
    test_ret = 0;
    test_pgr_get_invalid_checksum();
    if (!test_ret)
    {
      i_log_passed("%s\n", "pgr_get_invalid_checksum");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "pgr_get_invalid_checksum";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pager\pgr_get.c:114 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pager\pgr_new.c:220 START
  if (!filter || strstr("pgr_new_get_save", filter))
  {
    extern void test_pgr_new_get_save(void);
    i_log_info("========================= TEST CASE: %s\n", "pgr_new_get_save");
    int prev = test_ret;
    test_ret = 0;
    test_pgr_new_get_save();
    if (!test_ret)
    {
      i_log_passed("%s\n", "pgr_new_get_save");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "pgr_new_get_save";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pager\pgr_new.c:220 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pager\pgr_open.c:282 START
  if (!filter || strstr("pager_open", filter))
  {
    extern void test_pager_open(void);
    i_log_info("========================= TEST CASE: %s\n", "pager_open");
    int prev = test_ret;
    test_ret = 0;
    test_pager_open();
    if (!test_ret)
    {
      i_log_passed("%s\n", "pager_open");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "pager_open";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pager\pgr_open.c:282 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pager\pgr_open.c:324 START
  if (!filter || strstr("pgr_open_basic", filter))
  {
    extern void test_pgr_open_basic(void);
    i_log_info("========================= TEST CASE: %s\n", "pgr_open_basic");
    int prev = test_ret;
    test_ret = 0;
    test_pgr_open_basic();
    if (!test_ret)
    {
      i_log_passed("%s\n", "pgr_open_basic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "pgr_open_basic";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pager\pgr_open.c:324 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pager\pgr_reserve_and_ctrl_lock.c:96 START
  if (!filter || strstr("pgr_reserve_and_ctrl_lock_st", filter))
  {
    extern void test_pgr_reserve_and_ctrl_lock_st(void);
    i_log_info("========================= TEST CASE: %s\n", "pgr_reserve_and_ctrl_lock_st");
    int prev = test_ret;
    test_ret = 0;
    test_pgr_reserve_and_ctrl_lock_st();
    if (!test_ret)
    {
      i_log_passed("%s\n", "pgr_reserve_and_ctrl_lock_st");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "pgr_reserve_and_ctrl_lock_st";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pager\pgr_reserve_and_ctrl_lock.c:96 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pager\pgr_rollback.c:161 START
  if (!filter || strstr("aries_rollback_basic", filter))
  {
    extern void test_aries_rollback_basic(void);
    i_log_info("========================= TEST CASE: %s\n", "aries_rollback_basic");
    int prev = test_ret;
    test_ret = 0;
    test_aries_rollback_basic();
    if (!test_ret)
    {
      i_log_passed("%s\n", "aries_rollback_basic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "aries_rollback_basic";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pager\pgr_rollback.c:161 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pager\pgr_rollback.c:234 START
  if (!filter || strstr("aries_rollback_multiple_updates", filter))
  {
    extern void test_aries_rollback_multiple_updates(void);
    i_log_info("========================= TEST CASE: %s\n", "aries_rollback_multiple_updates");
    int prev = test_ret;
    test_ret = 0;
    test_aries_rollback_multiple_updates();
    if (!test_ret)
    {
      i_log_passed("%s\n", "aries_rollback_multiple_updates");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "aries_rollback_multiple_updates";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pager\pgr_rollback.c:234 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pager\pgr_rollback.c:313 START
  if (!filter || strstr("aries_rollback_with_crash_recovery", filter))
  {
    extern void test_aries_rollback_with_crash_recovery(void);
    i_log_info("========================= TEST CASE: %s\n", "aries_rollback_with_crash_recovery");
    int prev = test_ret;
    test_ret = 0;
    test_aries_rollback_with_crash_recovery();
    if (!test_ret)
    {
      i_log_passed("%s\n", "aries_rollback_with_crash_recovery");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "aries_rollback_with_crash_recovery";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pager\pgr_rollback.c:313 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pager\pgr_rollback.c:374 START
  if (!filter || strstr("aries_rollback_clr_not_undone", filter))
  {
    extern void test_aries_rollback_clr_not_undone(void);
    i_log_info("========================= TEST CASE: %s\n", "aries_rollback_clr_not_undone");
    int prev = test_ret;
    test_ret = 0;
    test_aries_rollback_clr_not_undone();
    if (!test_ret)
    {
      i_log_passed("%s\n", "aries_rollback_clr_not_undone");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "aries_rollback_clr_not_undone";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pager\pgr_rollback.c:374 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\data_list.c:87 START
  if (!filter || strstr("dl_validate", filter))
  {
    extern void test_dl_validate(void);
    i_log_info("========================= TEST CASE: %s\n", "dl_validate");
    int prev = test_ret;
    test_ret = 0;
    test_dl_validate();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dl_validate");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_validate";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\data_list.c:87 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\data_list.c:169 START
  if (!filter || strstr("dl_set_get", filter))
  {
    extern void test_dl_set_get(void);
    i_log_info("========================= TEST CASE: %s\n", "dl_set_get");
    int prev = test_ret;
    test_ret = 0;
    test_dl_set_get();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dl_set_get");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_set_get";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\data_list.c:169 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\data_list.c:232 START
  if (!filter || strstr("dl_read", filter))
  {
    extern void test_dl_read(void);
    i_log_info("========================= TEST CASE: %s\n", "dl_read");
    int prev = test_ret;
    test_ret = 0;
    test_dl_read();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dl_read");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_read";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\data_list.c:232 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\data_list.c:411 START
  if (!filter || strstr("dl_read_out_from", filter))
  {
    extern void test_dl_read_out_from(void);
    i_log_info("========================= TEST CASE: %s\n", "dl_read_out_from");
    int prev = test_ret;
    test_ret = 0;
    test_dl_read_out_from();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dl_read_out_from");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_read_out_from";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\data_list.c:411 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\data_list.c:614 START
  if (!filter || strstr("dl_append", filter))
  {
    extern void test_dl_append(void);
    i_log_info("========================= TEST CASE: %s\n", "dl_append");
    int prev = test_ret;
    test_ret = 0;
    test_dl_append();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dl_append");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_append";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\data_list.c:614 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\data_list.c:712 START
  if (!filter || strstr("dl_write", filter))
  {
    extern void test_dl_write(void);
    i_log_info("========================= TEST CASE: %s\n", "dl_write");
    int prev = test_ret;
    test_ret = 0;
    test_dl_write();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dl_write");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_write";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\data_list.c:712 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\data_list.c:842 START
  if (!filter || strstr("dl_memset", filter))
  {
    extern void test_dl_memset(void);
    i_log_info("========================= TEST CASE: %s\n", "dl_memset");
    int prev = test_ret;
    test_ret = 0;
    test_dl_memset();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dl_memset");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_memset";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\data_list.c:842 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\data_list.c:919 START
  if (!filter || strstr("dl_move_left", filter))
  {
    extern void test_dl_move_left(void);
    i_log_info("========================= TEST CASE: %s\n", "dl_move_left");
    int prev = test_ret;
    test_ret = 0;
    test_dl_move_left();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dl_move_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_move_left";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\data_list.c:919 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\data_list.c:999 START
  if (!filter || strstr("dl_shift_right", filter))
  {
    extern void test_dl_shift_right(void);
    i_log_info("========================= TEST CASE: %s\n", "dl_shift_right");
    int prev = test_ret;
    test_ret = 0;
    test_dl_shift_right();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dl_shift_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_shift_right";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\data_list.c:999 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\data_list.c:1089 START
  if (!filter || strstr("dl_move_right", filter))
  {
    extern void test_dl_move_right(void);
    i_log_info("========================= TEST CASE: %s\n", "dl_move_right");
    int prev = test_ret;
    test_ret = 0;
    test_dl_move_right();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dl_move_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dl_move_right";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\data_list.c:1089 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:126 START
  if (!filter || strstr("in_validate_for_db", filter))
  {
    extern void test_in_validate_for_db(void);
    i_log_info("========================= TEST CASE: %s\n", "in_validate_for_db");
    int prev = test_ret;
    test_ret = 0;
    test_in_validate_for_db();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_validate_for_db");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_validate_for_db";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:126 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:187 START
  if (!filter || strstr("in_set_get_simple", filter))
  {
    extern void test_in_set_get_simple(void);
    i_log_info("========================= TEST CASE: %s\n", "in_set_get_simple");
    int prev = test_ret;
    test_ret = 0;
    test_in_set_get_simple();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_set_get_simple");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_set_get_simple";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:187 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:245 START
  if (!filter || strstr("in_push_end", filter))
  {
    extern void test_in_push_end(void);
    i_log_info("========================= TEST CASE: %s\n", "in_push_end");
    int prev = test_ret;
    test_ret = 0;
    test_in_push_end();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_push_end");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_push_end";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:245 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:310 START
  if (!filter || strstr("in_memcpy", filter))
  {
    extern void test_in_memcpy(void);
    i_log_info("========================= TEST CASE: %s\n", "in_memcpy");
    int prev = test_ret;
    test_ret = 0;
    test_in_memcpy();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_memcpy");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_memcpy";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:310 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:472 START
  if (!filter || strstr("in_move_left", filter))
  {
    extern void test_in_move_left(void);
    i_log_info("========================= TEST CASE: %s\n", "in_move_left");
    int prev = test_ret;
    test_ret = 0;
    test_in_move_left();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_move_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_move_left";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:472 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:509 START
  if (!filter || strstr("in_move_left_two_keys", filter))
  {
    extern void test_in_move_left_two_keys(void);
    i_log_info("========================= TEST CASE: %s\n", "in_move_left_two_keys");
    int prev = test_ret;
    test_ret = 0;
    test_in_move_left_two_keys();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_move_left_two_keys");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_move_left_two_keys";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:509 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:537 START
  if (!filter || strstr("in_move_left_all_keys", filter))
  {
    extern void test_in_move_left_all_keys(void);
    i_log_info("========================= TEST CASE: %s\n", "in_move_left_all_keys");
    int prev = test_ret;
    test_ret = 0;
    test_in_move_left_all_keys();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_move_left_all_keys");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_move_left_all_keys";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:537 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:560 START
  if (!filter || strstr("in_move_left_into_empty", filter))
  {
    extern void test_in_move_left_into_empty(void);
    i_log_info("========================= TEST CASE: %s\n", "in_move_left_into_empty");
    int prev = test_ret;
    test_ret = 0;
    test_in_move_left_into_empty();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_move_left_into_empty");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_move_left_into_empty";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:560 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:639 START
  if (!filter || strstr("in_push_left", filter))
  {
    extern void test_in_push_left(void);
    i_log_info("========================= TEST CASE: %s\n", "in_push_left");
    int prev = test_ret;
    test_ret = 0;
    test_in_push_left();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_push_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_push_left";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:639 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:670 START
  if (!filter || strstr("in_push_left_into_empty", filter))
  {
    extern void test_in_push_left_into_empty(void);
    i_log_info("========================= TEST CASE: %s\n", "in_push_left_into_empty");
    int prev = test_ret;
    test_ret = 0;
    test_in_push_left_into_empty();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_push_left_into_empty");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_push_left_into_empty";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:670 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:683 START
  if (!filter || strstr("in_push_left_to_full", filter))
  {
    extern void test_in_push_left_to_full(void);
    i_log_info("========================= TEST CASE: %s\n", "in_push_left_to_full");
    int prev = test_ret;
    test_ret = 0;
    test_in_push_left_to_full();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_push_left_to_full");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_push_left_to_full";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:683 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:743 START
  if (!filter || strstr("in_move_right", filter))
  {
    extern void test_in_move_right(void);
    i_log_info("========================= TEST CASE: %s\n", "in_move_right");
    int prev = test_ret;
    test_ret = 0;
    test_in_move_right();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_move_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_move_right";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:743 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:780 START
  if (!filter || strstr("in_move_right_two_keys", filter))
  {
    extern void test_in_move_right_two_keys(void);
    i_log_info("========================= TEST CASE: %s\n", "in_move_right_two_keys");
    int prev = test_ret;
    test_ret = 0;
    test_in_move_right_two_keys();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_move_right_two_keys");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_move_right_two_keys";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:780 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:808 START
  if (!filter || strstr("in_move_right_all_keys", filter))
  {
    extern void test_in_move_right_all_keys(void);
    i_log_info("========================= TEST CASE: %s\n", "in_move_right_all_keys");
    int prev = test_ret;
    test_ret = 0;
    test_in_move_right_all_keys();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_move_right_all_keys");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_move_right_all_keys";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:808 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:831 START
  if (!filter || strstr("in_move_right_into_empty_right", filter))
  {
    extern void test_in_move_right_into_empty_right(void);
    i_log_info("========================= TEST CASE: %s\n", "in_move_right_into_empty_right");
    int prev = test_ret;
    test_ret = 0;
    test_in_move_right_into_empty_right();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_move_right_into_empty_right");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_move_right_into_empty_right";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:831 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:888 START
  if (!filter || strstr("in_choose_lidx", filter))
  {
    extern void test_in_choose_lidx(void);
    i_log_info("========================= TEST CASE: %s\n", "in_choose_lidx");
    int prev = test_ret;
    test_ret = 0;
    test_in_choose_lidx();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_choose_lidx");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_choose_lidx";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:888 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:986 START
  if (!filter || strstr("in_cut_left", filter))
  {
    extern void test_in_cut_left(void);
    i_log_info("========================= TEST CASE: %s\n", "in_cut_left");
    int prev = test_ret;
    test_ret = 0;
    test_in_cut_left();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_cut_left");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_cut_left";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:986 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:1024 START
  if (!filter || strstr("in_cut_left_all_at_once", filter))
  {
    extern void test_in_cut_left_all_at_once(void);
    i_log_info("========================= TEST CASE: %s\n", "in_cut_left_all_at_once");
    int prev = test_ret;
    test_ret = 0;
    test_in_cut_left_all_at_once();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_cut_left_all_at_once");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_cut_left_all_at_once";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:1024 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:1042 START
  if (!filter || strstr("in_cut_left_from_empty", filter))
  {
    extern void test_in_cut_left_from_empty(void);
    i_log_info("========================= TEST CASE: %s\n", "in_cut_left_from_empty");
    int prev = test_ret;
    test_ret = 0;
    test_in_cut_left_from_empty();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_cut_left_from_empty");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_cut_left_from_empty";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:1042 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:1055 START
  if (!filter || strstr("in_cut_left_to_one", filter))
  {
    extern void test_in_cut_left_to_one(void);
    i_log_info("========================= TEST CASE: %s\n", "in_cut_left_to_one");
    int prev = test_ret;
    test_ret = 0;
    test_in_cut_left_to_one();
    if (!test_ret)
    {
      i_log_passed("%s\n", "in_cut_left_to_one");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "in_cut_left_to_one";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\inner_node.c:1055 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\page.c:154 START
  if (!filter || strstr("page_set_get_simple", filter))
  {
    extern void test_page_set_get_simple(void);
    i_log_info("========================= TEST CASE: %s\n", "page_set_get_simple");
    int prev = test_ret;
    test_ret = 0;
    test_page_set_get_simple();
    if (!test_ret)
    {
      i_log_passed("%s\n", "page_set_get_simple");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "page_set_get_simple";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\page.c:154 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\var_page.c:41 START
  if (!filter || strstr("vp_init_empty", filter))
  {
    extern void test_vp_init_empty(void);
    i_log_info("========================= TEST CASE: %s\n", "vp_init_empty");
    int prev = test_ret;
    test_ret = 0;
    test_vp_init_empty();
    if (!test_ret)
    {
      i_log_passed("%s\n", "vp_init_empty");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "vp_init_empty";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\var_page.c:41 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\var_page.c:199 START
  if (!filter || strstr("vp_validate", filter))
  {
    extern void test_vp_validate(void);
    i_log_info("========================= TEST CASE: %s\n", "vp_validate");
    int prev = test_ret;
    test_ret = 0;
    test_vp_validate();
    if (!test_ret)
    {
      i_log_passed("%s\n", "vp_validate");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "vp_validate";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\var_page.c:199 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\var_tail.c:27 START
  if (!filter || strstr("vt_init_empty", filter))
  {
    extern void test_vt_init_empty(void);
    i_log_info("========================= TEST CASE: %s\n", "vt_init_empty");
    int prev = test_ret;
    test_ret = 0;
    test_vt_init_empty();
    if (!test_ret)
    {
      i_log_passed("%s\n", "vt_init_empty");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "vt_init_empty";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\var_tail.c:27 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\var_tail.c:52 START
  if (!filter || strstr("vt_validate", filter))
  {
    extern void test_vt_validate(void);
    i_log_info("========================= TEST CASE: %s\n", "vt_validate");
    int prev = test_ret;
    test_ret = 0;
    test_vt_validate();
    if (!test_ret)
    {
      i_log_passed("%s\n", "vt_validate");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "vt_validate";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\pages\var_tail.c:52 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\prim.c:49 START
  if (!filter || strstr("prim_t_validate", filter))
  {
    extern void test_prim_t_validate(void);
    i_log_info("========================= TEST CASE: %s\n", "prim_t_validate");
    int prev = test_ret;
    test_ret = 0;
    test_prim_t_validate();
    if (!test_ret)
    {
      i_log_passed("%s\n", "prim_t_validate");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "prim_t_validate";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\prim.c:49 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\prim.c:131 START
  if (!filter || strstr("prim_t_snprintf", filter))
  {
    extern void test_prim_t_snprintf(void);
    i_log_info("========================= TEST CASE: %s\n", "prim_t_snprintf");
    int prev = test_ret;
    test_ret = 0;
    test_prim_t_snprintf();
    if (!test_ret)
    {
      i_log_passed("%s\n", "prim_t_snprintf");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "prim_t_snprintf";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\prim.c:131 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\prim.c:198 START
  if (!filter || strstr("prim_t_byte_size", filter))
  {
    extern void test_prim_t_byte_size(void);
    i_log_info("========================= TEST CASE: %s\n", "prim_t_byte_size");
    int prev = test_ret;
    test_ret = 0;
    test_prim_t_byte_size();
    if (!test_ret)
    {
      i_log_passed("%s\n", "prim_t_byte_size");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "prim_t_byte_size";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\prim.c:198 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\prim.c:210 START
  if (!filter || strstr("prim_t_serialize", filter))
  {
    extern void test_prim_t_serialize(void);
    i_log_info("========================= TEST CASE: %s\n", "prim_t_serialize");
    int prev = test_ret;
    test_ret = 0;
    test_prim_t_serialize();
    if (!test_ret)
    {
      i_log_passed("%s\n", "prim_t_serialize");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "prim_t_serialize";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\prim.c:210 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\prim.c:248 START
  if (!filter || strstr("prim_t_deserialize", filter))
  {
    extern void test_prim_t_deserialize(void);
    i_log_info("========================= TEST CASE: %s\n", "prim_t_deserialize");
    int prev = test_ret;
    test_ret = 0;
    test_prim_t_deserialize();
    if (!test_ret)
    {
      i_log_passed("%s\n", "prim_t_deserialize");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "prim_t_deserialize";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\prim.c:248 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\prim.c:274 START
  if (!filter || strstr("prim_t_random", filter))
  {
    extern void test_prim_t_random(void);
    i_log_info("========================= TEST CASE: %s\n", "prim_t_random");
    int prev = test_ret;
    test_ret = 0;
    test_prim_t_random();
    if (!test_ret)
    {
      i_log_passed("%s\n", "prim_t_random");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "prim_t_random";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\prim.c:274 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\random.c:125 START
  if (!filter || strstr("randu32", filter))
  {
    extern void test_randu32(void);
    i_log_info("========================= TEST CASE: %s\n", "randu32");
    int prev = test_ret;
    test_ret = 0;
    test_randu32();
    if (!test_ret)
    {
      i_log_passed("%s\n", "randu32");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "randu32";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\random.c:125 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\random.c:156 START
  if (!filter || strstr("randu32r", filter))
  {
    extern void test_randu32r(void);
    i_log_info("========================= TEST CASE: %s\n", "randu32r");
    int prev = test_ret;
    test_ret = 0;
    test_randu32r();
    if (!test_ret)
    {
      i_log_passed("%s\n", "randu32r");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "randu32r";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\random.c:156 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\random.c:234 START
  if (!filter || strstr("randi32r", filter))
  {
    extern void test_randi32r(void);
    i_log_info("========================= TEST CASE: %s\n", "randi32r");
    int prev = test_ret;
    test_ret = 0;
    test_randi32r();
    if (!test_ret)
    {
      i_log_passed("%s\n", "randi32r");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "randi32r";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\random.c:234 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\random.c:373 START
  if (!filter || strstr("randu64r", filter))
  {
    extern void test_randu64r(void);
    i_log_info("========================= TEST CASE: %s\n", "randu64r");
    int prev = test_ret;
    test_ret = 0;
    test_randu64r();
    if (!test_ret)
    {
      i_log_passed("%s\n", "randu64r");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "randu64r";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\random.c:373 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\random.c:423 START
  if (!filter || strstr("randu64e", filter))
  {
    extern void test_randu64e(void);
    i_log_info("========================= TEST CASE: %s\n", "randu64e");
    int prev = test_ret;
    test_ret = 0;
    test_randu64e();
    if (!test_ret)
    {
      i_log_passed("%s\n", "randu64e");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "randu64e";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\random.c:423 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\random.c:487 START
  if (!filter || strstr("randi64r", filter))
  {
    extern void test_randi64r(void);
    i_log_info("========================= TEST CASE: %s\n", "randi64r");
    int prev = test_ret;
    test_ret = 0;
    test_randi64r();
    if (!test_ret)
    {
      i_log_passed("%s\n", "randi64r");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "randi64r";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\random.c:487 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\random.c:510 START
  if (!filter || strstr("randi64e", filter))
  {
    extern void test_randi64e(void);
    i_log_info("========================= TEST CASE: %s\n", "randi64e");
    int prev = test_ret;
    test_ret = 0;
    test_randi64e();
    if (!test_ret)
    {
      i_log_passed("%s\n", "randi64e");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "randi64e";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\random.c:510 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\robin_hood_ht_tests.c:24 START
  if (!filter || strstr("ht_insert_idx_regression_trigger_swap", filter))
  {
    extern void test_ht_insert_idx_regression_trigger_swap(void);
    i_log_info("========================= TEST CASE: %s\n", "ht_insert_idx_regression_trigger_swap");
    int prev = test_ret;
    test_ret = 0;
    test_ht_insert_idx_regression_trigger_swap();
    if (!test_ret)
    {
      i_log_passed("%s\n", "ht_insert_idx_regression_trigger_swap");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ht_insert_idx_regression_trigger_swap";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\robin_hood_ht_tests.c:24 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\robin_hood_ht_tests.c:102 START
  if (!filter || strstr("robin_hood_ht", filter))
  {
    extern void test_robin_hood_ht(void);
    i_log_info("========================= TEST CASE: %s\n", "robin_hood_ht");
    int prev = test_ret;
    test_ret = 0;
    test_robin_hood_ht();
    if (!test_ret)
    {
      i_log_passed("%s\n", "robin_hood_ht");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "robin_hood_ht";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\robin_hood_ht_tests.c:102 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\rope\ns_balance_and_release.c:70 START
  if (!filter || strstr("dlgt_balance_with_prev", filter))
  {
    extern void test_dlgt_balance_with_prev(void);
    i_log_info("========================= TEST CASE: %s\n", "dlgt_balance_with_prev");
    int prev = test_ret;
    test_ret = 0;
    test_dlgt_balance_with_prev();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dlgt_balance_with_prev");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dlgt_balance_with_prev";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\rope\ns_balance_and_release.c:70 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\rope\ns_balance_and_release.c:250 START
  if (!filter || strstr("dlgt_balance_with_next", filter))
  {
    extern void test_dlgt_balance_with_next(void);
    i_log_info("========================= TEST CASE: %s\n", "dlgt_balance_with_next");
    int prev = test_ret;
    test_ret = 0;
    test_dlgt_balance_with_next();
    if (!test_ret)
    {
      i_log_passed("%s\n", "dlgt_balance_with_next");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "dlgt_balance_with_next";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\rope\ns_balance_and_release.c:250 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\sarray.c:116 START
  if (!filter || strstr("sarray_t_snprintf", filter))
  {
    extern void test_sarray_t_snprintf(void);
    i_log_info("========================= TEST CASE: %s\n", "sarray_t_snprintf");
    int prev = test_ret;
    test_ret = 0;
    test_sarray_t_snprintf();
    if (!test_ret)
    {
      i_log_passed("%s\n", "sarray_t_snprintf");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "sarray_t_snprintf";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\sarray.c:116 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\sarray.c:154 START
  if (!filter || strstr("sarray_t_byte_size", filter))
  {
    extern void test_sarray_t_byte_size(void);
    i_log_info("========================= TEST CASE: %s\n", "sarray_t_byte_size");
    int prev = test_ret;
    test_ret = 0;
    test_sarray_t_byte_size();
    if (!test_ret)
    {
      i_log_passed("%s\n", "sarray_t_byte_size");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "sarray_t_byte_size";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\sarray.c:154 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\sarray.c:183 START
  if (!filter || strstr("sarray_t_get_serial_size", filter))
  {
    extern void test_sarray_t_get_serial_size(void);
    i_log_info("========================= TEST CASE: %s\n", "sarray_t_get_serial_size");
    int prev = test_ret;
    test_ret = 0;
    test_sarray_t_get_serial_size();
    if (!test_ret)
    {
      i_log_passed("%s\n", "sarray_t_get_serial_size");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "sarray_t_get_serial_size";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\sarray.c:183 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\sarray.c:219 START
  if (!filter || strstr("sarray_t_serialize", filter))
  {
    extern void test_sarray_t_serialize(void);
    i_log_info("========================= TEST CASE: %s\n", "sarray_t_serialize");
    int prev = test_ret;
    test_ret = 0;
    test_sarray_t_serialize();
    if (!test_ret)
    {
      i_log_passed("%s\n", "sarray_t_serialize");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "sarray_t_serialize";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\sarray.c:219 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\sarray.c:312 START
  if (!filter || strstr("sarray_t_deserialize_green_path", filter))
  {
    extern void test_sarray_t_deserialize_green_path(void);
    i_log_info("========================= TEST CASE: %s\n", "sarray_t_deserialize_green_path");
    int prev = test_ret;
    test_ret = 0;
    test_sarray_t_deserialize_green_path();
    if (!test_ret)
    {
      i_log_passed("%s\n", "sarray_t_deserialize_green_path");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "sarray_t_deserialize_green_path";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\sarray.c:312 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\sarray.c:347 START
  if (!filter || strstr("sarray_t_deserialize_red_path", filter))
  {
    extern void test_sarray_t_deserialize_red_path(void);
    i_log_info("========================= TEST CASE: %s\n", "sarray_t_deserialize_red_path");
    int prev = test_ret;
    test_ret = 0;
    test_sarray_t_deserialize_red_path();
    if (!test_ret)
    {
      i_log_passed("%s\n", "sarray_t_deserialize_red_path");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "sarray_t_deserialize_red_path";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\sarray.c:347 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\sarray_builder.c:138 START
  if (!filter || strstr("sarray_builder", filter))
  {
    extern void test_sarray_builder(void);
    i_log_info("========================= TEST CASE: %s\n", "sarray_builder");
    int prev = test_ret;
    test_ret = 0;
    test_sarray_builder();
    if (!test_ret)
    {
      i_log_passed("%s\n", "sarray_builder");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "sarray_builder";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\sarray_builder.c:138 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\slab_alloc.c:267 START
  if (!filter || strstr("slab_alloc_simple", filter))
  {
    extern void test_slab_alloc_simple(void);
    i_log_info("========================= TEST CASE: %s\n", "slab_alloc_simple");
    int prev = test_ret;
    test_ret = 0;
    test_slab_alloc_simple();
    if (!test_ret)
    {
      i_log_passed("%s\n", "slab_alloc_simple");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "slab_alloc_simple";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\slab_alloc.c:267 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\slab_alloc.c:383 START
  if (!filter || strstr("slab_alloc_cap_one", filter))
  {
    extern void test_slab_alloc_cap_one(void);
    i_log_info("========================= TEST CASE: %s\n", "slab_alloc_cap_one");
    int prev = test_ret;
    test_ret = 0;
    test_slab_alloc_cap_one();
    if (!test_ret)
    {
      i_log_passed("%s\n", "slab_alloc_cap_one");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "slab_alloc_cap_one";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\slab_alloc.c:383 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\slab_alloc.c:419 START
  if (!filter || strstr("slab_alloc_no_duplicates", filter))
  {
    extern void test_slab_alloc_no_duplicates(void);
    i_log_info("========================= TEST CASE: %s\n", "slab_alloc_no_duplicates");
    int prev = test_ret;
    test_ret = 0;
    test_slab_alloc_no_duplicates();
    if (!test_ret)
    {
      i_log_passed("%s\n", "slab_alloc_no_duplicates");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "slab_alloc_no_duplicates";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\slab_alloc.c:419 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\slab_alloc.c:448 START
  if (!filter || strstr("slab_alloc_free_all_realloc", filter))
  {
    extern void test_slab_alloc_free_all_realloc(void);
    i_log_info("========================= TEST CASE: %s\n", "slab_alloc_free_all_realloc");
    int prev = test_ret;
    test_ret = 0;
    test_slab_alloc_free_all_realloc();
    if (!test_ret)
    {
      i_log_passed("%s\n", "slab_alloc_free_all_realloc");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "slab_alloc_free_all_realloc";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\slab_alloc.c:448 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\slab_alloc.c:491 START
  if (!filter || strstr("slab_alloc_interleaved_patterns", filter))
  {
    extern void test_slab_alloc_interleaved_patterns(void);
    i_log_info("========================= TEST CASE: %s\n", "slab_alloc_interleaved_patterns");
    int prev = test_ret;
    test_ret = 0;
    test_slab_alloc_interleaved_patterns();
    if (!test_ret)
    {
      i_log_passed("%s\n", "slab_alloc_interleaved_patterns");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "slab_alloc_interleaved_patterns";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\slab_alloc.c:491 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\slab_alloc.c:543 START
  if (!filter || strstr("slab_alloc_free_head_slab", filter))
  {
    extern void test_slab_alloc_free_head_slab(void);
    i_log_info("========================= TEST CASE: %s\n", "slab_alloc_free_head_slab");
    int prev = test_ret;
    test_ret = 0;
    test_slab_alloc_free_head_slab();
    if (!test_ret)
    {
      i_log_passed("%s\n", "slab_alloc_free_head_slab");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "slab_alloc_free_head_slab";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\slab_alloc.c:543 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\slab_alloc.c:587 START
  if (!filter || strstr("slab_alloc_free_middle_slab", filter))
  {
    extern void test_slab_alloc_free_middle_slab(void);
    i_log_info("========================= TEST CASE: %s\n", "slab_alloc_free_middle_slab");
    int prev = test_ret;
    test_ret = 0;
    test_slab_alloc_free_middle_slab();
    if (!test_ret)
    {
      i_log_passed("%s\n", "slab_alloc_free_middle_slab");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "slab_alloc_free_middle_slab";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\slab_alloc.c:587 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\slab_alloc.c:633 START
  if (!filter || strstr("slab_alloc_minimum_size", filter))
  {
    extern void test_slab_alloc_minimum_size(void);
    i_log_info("========================= TEST CASE: %s\n", "slab_alloc_minimum_size");
    int prev = test_ret;
    test_ret = 0;
    test_slab_alloc_minimum_size();
    if (!test_ret)
    {
      i_log_passed("%s\n", "slab_alloc_minimum_size");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "slab_alloc_minimum_size";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\slab_alloc.c:633 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\slab_alloc.c:662 START
  if (!filter || strstr("slab_alloc_stress_random", filter))
  {
    extern void test_slab_alloc_stress_random(void);
    i_log_info("========================= TEST CASE: %s\n", "slab_alloc_stress_random");
    int prev = test_ret;
    test_ret = 0;
    test_slab_alloc_stress_random();
    if (!test_ret)
    {
      i_log_passed("%s\n", "slab_alloc_stress_random");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "slab_alloc_stress_random";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\slab_alloc.c:662 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\stride.c:157 START
  if (!filter || strstr("stride_resolve", filter))
  {
    extern void test_stride_resolve(void);
    i_log_info("========================= TEST CASE: %s\n", "stride_resolve");
    int prev = test_ret;
    test_ret = 0;
    test_stride_resolve();
    if (!test_ret)
    {
      i_log_passed("%s\n", "stride_resolve");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "stride_resolve";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\stride.c:157 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\string.c:77 START
  if (!filter || strstr("strings_all_unique", filter))
  {
    extern void test_strings_all_unique(void);
    i_log_info("========================= TEST CASE: %s\n", "strings_all_unique");
    int prev = test_ret;
    test_ret = 0;
    test_strings_all_unique();
    if (!test_ret)
    {
      i_log_passed("%s\n", "strings_all_unique");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "strings_all_unique";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\string.c:77 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\string.c:229 START
  if (!filter || strstr("string_contains", filter))
  {
    extern void test_string_contains(void);
    i_log_info("========================= TEST CASE: %s\n", "string_contains");
    int prev = test_ret;
    test_ret = 0;
    test_string_contains();
    if (!test_ret)
    {
      i_log_passed("%s\n", "string_contains");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "string_contains";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\string.c:229 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\struct.c:244 START
  if (!filter || strstr("struct_t_snprintf", filter))
  {
    extern void test_struct_t_snprintf(void);
    i_log_info("========================= TEST CASE: %s\n", "struct_t_snprintf");
    int prev = test_ret;
    test_ret = 0;
    test_struct_t_snprintf();
    if (!test_ret)
    {
      i_log_passed("%s\n", "struct_t_snprintf");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "struct_t_snprintf";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\struct.c:244 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\struct.c:312 START
  if (!filter || strstr("struct_t_byte_size", filter))
  {
    extern void test_struct_t_byte_size(void);
    i_log_info("========================= TEST CASE: %s\n", "struct_t_byte_size");
    int prev = test_ret;
    test_ret = 0;
    test_struct_t_byte_size();
    if (!test_ret)
    {
      i_log_passed("%s\n", "struct_t_byte_size");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "struct_t_byte_size";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\struct.c:312 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\struct.c:380 START
  if (!filter || strstr("struct_t_get_serial_size", filter))
  {
    extern void test_struct_t_get_serial_size(void);
    i_log_info("========================= TEST CASE: %s\n", "struct_t_get_serial_size");
    int prev = test_ret;
    test_ret = 0;
    test_struct_t_get_serial_size();
    if (!test_ret)
    {
      i_log_passed("%s\n", "struct_t_get_serial_size");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "struct_t_get_serial_size";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\struct.c:380 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\struct.c:455 START
  if (!filter || strstr("struct_t_serialize", filter))
  {
    extern void test_struct_t_serialize(void);
    i_log_info("========================= TEST CASE: %s\n", "struct_t_serialize");
    int prev = test_ret;
    test_ret = 0;
    test_struct_t_serialize();
    if (!test_ret)
    {
      i_log_passed("%s\n", "struct_t_serialize");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "struct_t_serialize";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\struct.c:455 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\struct.c:605 START
  if (!filter || strstr("struct_t_deserialize_green_path", filter))
  {
    extern void test_struct_t_deserialize_green_path(void);
    i_log_info("========================= TEST CASE: %s\n", "struct_t_deserialize_green_path");
    int prev = test_ret;
    test_ret = 0;
    test_struct_t_deserialize_green_path();
    if (!test_ret)
    {
      i_log_passed("%s\n", "struct_t_deserialize_green_path");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "struct_t_deserialize_green_path";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\struct.c:605 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\struct.c:661 START
  if (!filter || strstr("struct_t_deserialize_red_path", filter))
  {
    extern void test_struct_t_deserialize_red_path(void);
    i_log_info("========================= TEST CASE: %s\n", "struct_t_deserialize_red_path");
    int prev = test_ret;
    test_ret = 0;
    test_struct_t_deserialize_red_path();
    if (!test_ret)
    {
      i_log_passed("%s\n", "struct_t_deserialize_red_path");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "struct_t_deserialize_red_path";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\struct.c:661 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\testing\smfile_data_writer.c:127 START
  if (!filter || strstr("smfile_data_writer", filter))
  {
    extern void test_smfile_data_writer(void);
    i_log_info("========================= TEST CASE: %s\n", "smfile_data_writer");
    int prev = test_ret;
    test_ret = 0;
    test_smfile_data_writer();
    if (!test_ret)
    {
      i_log_passed("%s\n", "smfile_data_writer");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "smfile_data_writer";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\testing\smfile_data_writer.c:127 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\testing.c:38 START
  if (!filter || strstr("test_mark_works", filter))
  {
    extern void test_test_mark_works(void);
    i_log_info("========================= TEST CASE: %s\n", "test_mark_works");
    int prev = test_ret;
    test_ret = 0;
    test_test_mark_works();
    if (!test_ret)
    {
      i_log_passed("%s\n", "test_mark_works");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "test_mark_works";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\testing.c:38 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\testing.c:49 START
  if (!filter || strstr("test_mark_match", filter))
  {
    extern void test_test_mark_match(void);
    i_log_info("========================= TEST CASE: %s\n", "test_mark_match");
    int prev = test_ret;
    test_ret = 0;
    test_test_mark_match();
    if (!test_ret)
    {
      i_log_passed("%s\n", "test_mark_match");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "test_mark_match";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\testing.c:49 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\txn.c:320 START
  if (!filter || strstr("txn_basic", filter))
  {
    extern void test_txn_basic(void);
    i_log_info("========================= TEST CASE: %s\n", "txn_basic");
    int prev = test_ret;
    test_ret = 0;
    test_txn_basic();
    if (!test_ret)
    {
      i_log_passed("%s\n", "txn_basic");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txn_basic";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\txn.c:320 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\txn_table.c:71 START
  if (!filter || strstr("txnt_open", filter))
  {
    extern void test_txnt_open(void);
    i_log_info("========================= TEST CASE: %s\n", "txnt_open");
    int prev = test_ret;
    test_ret = 0;
    test_txnt_open();
    if (!test_ret)
    {
      i_log_passed("%s\n", "txnt_open");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_open";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\txn_table.c:71 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\txn_table.c:217 START
  if (!filter || strstr("txnt_merge_into", filter))
  {
    extern void test_txnt_merge_into(void);
    i_log_info("========================= TEST CASE: %s\n", "txnt_merge_into");
    int prev = test_ret;
    test_ret = 0;
    test_txnt_merge_into();
    if (!test_ret)
    {
      i_log_passed("%s\n", "txnt_merge_into");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_merge_into";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\txn_table.c:217 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\txn_table.c:358 START
  if (!filter || strstr("txnt_max_u_undo_lsn", filter))
  {
    extern void test_txnt_max_u_undo_lsn(void);
    i_log_info("========================= TEST CASE: %s\n", "txnt_max_u_undo_lsn");
    int prev = test_ret;
    test_ret = 0;
    test_txnt_max_u_undo_lsn();
    if (!test_ret)
    {
      i_log_passed("%s\n", "txnt_max_u_undo_lsn");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_max_u_undo_lsn";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\txn_table.c:358 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\txn_table.c:486 START
  if (!filter || strstr("txnt_min_lsn", filter))
  {
    extern void test_txnt_min_lsn(void);
    i_log_info("========================= TEST CASE: %s\n", "txnt_min_lsn");
    int prev = test_ret;
    test_ret = 0;
    test_txnt_min_lsn();
    if (!test_ret)
    {
      i_log_passed("%s\n", "txnt_min_lsn");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_min_lsn";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\txn_table.c:486 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\txn_table.c:594 START
  if (!filter || strstr("txnt_exists", filter))
  {
    extern void test_txnt_exists(void);
    i_log_info("========================= TEST CASE: %s\n", "txnt_exists");
    int prev = test_ret;
    test_ret = 0;
    test_txnt_exists();
    if (!test_ret)
    {
      i_log_passed("%s\n", "txnt_exists");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_exists";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\txn_table.c:594 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\txn_table.c:651 START
  if (!filter || strstr("txnt_insert", filter))
  {
    extern void test_txnt_insert(void);
    i_log_info("========================= TEST CASE: %s\n", "txnt_insert");
    int prev = test_ret;
    test_ret = 0;
    test_txnt_insert();
    if (!test_ret)
    {
      i_log_passed("%s\n", "txnt_insert");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_insert";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\txn_table.c:651 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\txn_table.c:804 START
  if (!filter || strstr("txnt_get", filter))
  {
    extern void test_txnt_get(void);
    i_log_info("========================= TEST CASE: %s\n", "txnt_get");
    int prev = test_ret;
    test_ret = 0;
    test_txnt_get();
    if (!test_ret)
    {
      i_log_passed("%s\n", "txnt_get");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_get";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\txn_table.c:804 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\txn_table.c:988 START
  if (!filter || strstr("txnt_remove", filter))
  {
    extern void test_txnt_remove(void);
    i_log_info("========================= TEST CASE: %s\n", "txnt_remove");
    int prev = test_ret;
    test_ret = 0;
    test_txnt_remove();
    if (!test_ret)
    {
      i_log_passed("%s\n", "txnt_remove");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_remove";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\txn_table.c:988 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\txn_table.c:1208 START
  if (!filter || strstr("txnt_serialize", filter))
  {
    extern void test_txnt_serialize(void);
    i_log_info("========================= TEST CASE: %s\n", "txnt_serialize");
    int prev = test_ret;
    test_ret = 0;
    test_txnt_serialize();
    if (!test_ret)
    {
      i_log_passed("%s\n", "txnt_serialize");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_serialize";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\txn_table.c:1208 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\txn_table.c:1342 START
  if (!filter || strstr("txnt_equal_ignore_state", filter))
  {
    extern void test_txnt_equal_ignore_state(void);
    i_log_info("========================= TEST CASE: %s\n", "txnt_equal_ignore_state");
    int prev = test_ret;
    test_ret = 0;
    test_txnt_equal_ignore_state();
    if (!test_ret)
    {
      i_log_passed("%s\n", "txnt_equal_ignore_state");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_equal_ignore_state";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\txn_table.c:1342 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\txnt_concurrency_tests.c:121 START
  if (!filter || strstr("txnt_concurrent", filter))
  {
    extern void test_txnt_concurrent(void);
    i_log_info("========================= TEST CASE: %s\n", "txnt_concurrent");
    int prev = test_ret;
    test_ret = 0;
    test_txnt_concurrent();
    if (!test_ret)
    {
      i_log_passed("%s\n", "txnt_concurrent");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "txnt_concurrent";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\txnt_concurrency_tests.c:121 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\type_accessor.c:354 START
  if (!filter || strstr("ta_subtype", filter))
  {
    extern void test_ta_subtype(void);
    i_log_info("========================= TEST CASE: %s\n", "ta_subtype");
    int prev = test_ret;
    test_ret = 0;
    test_ta_subtype();
    if (!test_ret)
    {
      i_log_passed("%s\n", "ta_subtype");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ta_subtype";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\type_accessor.c:354 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\type_accessor_builder.c:278 START
  if (!filter || strstr("type_accessor_builder", filter))
  {
    extern void test_type_accessor_builder(void);
    i_log_info("========================= TEST CASE: %s\n", "type_accessor_builder");
    int prev = test_ret;
    test_ret = 0;
    test_type_accessor_builder();
    if (!test_ret)
    {
      i_log_passed("%s\n", "type_accessor_builder");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "type_accessor_builder";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\type_accessor_builder.c:278 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\types.c:284 START
  if (!filter || strstr("type_generate_string", filter))
  {
    extern void test_type_generate_string(void);
    i_log_info("========================= TEST CASE: %s\n", "type_generate_string");
    int prev = test_ret;
    test_ret = 0;
    test_type_generate_string();
    if (!test_ret)
    {
      i_log_passed("%s\n", "type_generate_string");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "type_generate_string";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\types.c:284 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\types.c:967 START
  if (!filter || strstr("type_malloc_copy", filter))
  {
    extern void test_type_malloc_copy(void);
    i_log_info("========================= TEST CASE: %s\n", "type_malloc_copy");
    int prev = test_ret;
    test_ret = 0;
    test_type_malloc_copy();
    if (!test_ret)
    {
      i_log_passed("%s\n", "type_malloc_copy");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "type_malloc_copy";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\types.c:967 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\union.c:244 START
  if (!filter || strstr("union_t_snprintf", filter))
  {
    extern void test_union_t_snprintf(void);
    i_log_info("========================= TEST CASE: %s\n", "union_t_snprintf");
    int prev = test_ret;
    test_ret = 0;
    test_union_t_snprintf();
    if (!test_ret)
    {
      i_log_passed("%s\n", "union_t_snprintf");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "union_t_snprintf";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\union.c:244 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\union.c:315 START
  if (!filter || strstr("union_t_byte_size", filter))
  {
    extern void test_union_t_byte_size(void);
    i_log_info("========================= TEST CASE: %s\n", "union_t_byte_size");
    int prev = test_ret;
    test_ret = 0;
    test_union_t_byte_size();
    if (!test_ret)
    {
      i_log_passed("%s\n", "union_t_byte_size");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "union_t_byte_size";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\union.c:315 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\union.c:383 START
  if (!filter || strstr("union_t_get_serial_size", filter))
  {
    extern void test_union_t_get_serial_size(void);
    i_log_info("========================= TEST CASE: %s\n", "union_t_get_serial_size");
    int prev = test_ret;
    test_ret = 0;
    test_union_t_get_serial_size();
    if (!test_ret)
    {
      i_log_passed("%s\n", "union_t_get_serial_size");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "union_t_get_serial_size";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\union.c:383 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\union.c:458 START
  if (!filter || strstr("union_t_serialize", filter))
  {
    extern void test_union_t_serialize(void);
    i_log_info("========================= TEST CASE: %s\n", "union_t_serialize");
    int prev = test_ret;
    test_ret = 0;
    test_union_t_serialize();
    if (!test_ret)
    {
      i_log_passed("%s\n", "union_t_serialize");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "union_t_serialize";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\union.c:458 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\union.c:604 START
  if (!filter || strstr("union_t_deserialize_green_path", filter))
  {
    extern void test_union_t_deserialize_green_path(void);
    i_log_info("========================= TEST CASE: %s\n", "union_t_deserialize_green_path");
    int prev = test_ret;
    test_ret = 0;
    test_union_t_deserialize_green_path();
    if (!test_ret)
    {
      i_log_passed("%s\n", "union_t_deserialize_green_path");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "union_t_deserialize_green_path";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\union.c:604 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\union.c:660 START
  if (!filter || strstr("union_t_deserialize_red_path", filter))
  {
    extern void test_union_t_deserialize_red_path(void);
    i_log_info("========================= TEST CASE: %s\n", "union_t_deserialize_red_path");
    int prev = test_ret;
    test_ret = 0;
    test_union_t_deserialize_red_path();
    if (!test_ret)
    {
      i_log_passed("%s\n", "union_t_deserialize_red_path");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "union_t_deserialize_red_path";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\union.c:660 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\var\ns_find_var_page.c:351 START
  if (!filter || strstr("ns_find_var_page", filter))
  {
    extern void test_ns_find_var_page(void);
    i_log_info("========================= TEST CASE: %s\n", "ns_find_var_page");
    int prev = test_ret;
    test_ret = 0;
    test_ns_find_var_page();
    if (!test_ret)
    {
      i_log_passed("%s\n", "ns_find_var_page");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ns_find_var_page";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\var\ns_find_var_page.c:351 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\var\ns_var_get_or_create.c:90 START
  if (!filter || strstr("ns_var_get_or_create", filter))
  {
    extern void test_ns_var_get_or_create(void);
    i_log_info("========================= TEST CASE: %s\n", "ns_var_get_or_create");
    int prev = test_ret;
    test_ret = 0;
    test_ns_var_get_or_create();
    if (!test_ret)
    {
      i_log_passed("%s\n", "ns_var_get_or_create");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ns_var_get_or_create";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\var\ns_var_get_or_create.c:90 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\variables.c:275 START
  if (!filter || strstr("rand_varname_same_hash", filter))
  {
    extern void test_rand_varname_same_hash(void);
    i_log_info("========================= TEST CASE: %s\n", "rand_varname_same_hash");
    int prev = test_ret;
    test_ret = 0;
    test_rand_varname_same_hash();
    if (!test_ret)
    {
      i_log_passed("%s\n", "rand_varname_same_hash");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "rand_varname_same_hash";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\variables.c:275 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\variables.c:292 START
  if (!filter || strstr("rand_varname_different_hash", filter))
  {
    extern void test_rand_varname_different_hash(void);
    i_log_info("========================= TEST CASE: %s\n", "rand_varname_different_hash");
    int prev = test_ret;
    test_ret = 0;
    test_rand_varname_different_hash();
    if (!test_ret)
    {
      i_log_passed("%s\n", "rand_varname_different_hash");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "rand_varname_different_hash";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\variables.c:292 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\wal_tests.c:288 START
  if (!filter || strstr("wal", filter))
  {
    extern void test_wal(void);
    i_log_info("========================= TEST CASE: %s\n", "wal");
    int prev = test_ret;
    test_ret = 0;
    test_wal();
    if (!test_ret)
    {
      i_log_passed("%s\n", "wal");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "wal";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\wal_tests.c:288 DONE

  //////////////////// C:\Users\tlincke\dev\numstore\src\wal_tests.c:410 START
  if (!filter || strstr("wal_single_entry", filter))
  {
    extern void test_wal_single_entry(void);
    i_log_info("========================= TEST CASE: %s\n", "wal_single_entry");
    int prev = test_ret;
    test_ret = 0;
    test_wal_single_entry();
    if (!test_ret)
    {
      i_log_passed("%s\n", "wal_single_entry");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "wal_single_entry";
    }
  }
  //////////////////// C:\Users\tlincke\dev\numstore\src\wal_tests.c:410 DONE


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
    i_log_passed ("ALL TESTS PASSED\n");
  }

  return test_ret;
}
