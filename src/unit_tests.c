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

#ifdef TESTING 

// AUTO GENERATED - DO NOT MODIFY

#include <stdio.h>
#include <string.h>

#include "error.h"
#include "logging.h"
#include "os.h"
#include "testing.h"
#include "unit_tests.h"

int
run_unit_tests (int seed, const char* filter)
{
  srand(seed);
  int ntests = 0;

  error   e = error_create ();
  i_timer timer;
  if (i_timer_create (&timer, &e) != SUCCESS)
  {
    return -1;
  }

  int         failed = 0;
  const char *failed_names[342];

    //////////////////// smfile_test_fixture.c:123 START
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
  //////////////////// smfile_test_fixture.c:123 DONE
  //////////////////// serial.c:82 START
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
  //////////////////// serial.c:82 DONE
  //////////////////// serial.c:204 START
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
  //////////////////// serial.c:204 DONE
  //////////////////// serial.c:397 START
  if (!filter || strstr("stream_init", filter))
  {
    extern void __test__stream_init(void);
    i_log_info("========================= TEST CASE: %s\n", "stream_init");
    int prev = test_ret;
    test_ret = 0;
    __test__stream_init();
    if (!test_ret)
    {
      i_log_passed("%s\n", "stream_init");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "stream_init";
    }
    ntests++;
  }
  //////////////////// serial.c:397 DONE
  //////////////////// serial.c:422 START
  if (!filter || strstr("stream_finish", filter))
  {
    extern void __test__stream_finish(void);
    i_log_info("========================= TEST CASE: %s\n", "stream_finish");
    int prev = test_ret;
    test_ret = 0;
    __test__stream_finish();
    if (!test_ret)
    {
      i_log_passed("%s\n", "stream_finish");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "stream_finish";
    }
    ntests++;
  }
  //////////////////// serial.c:422 DONE
  //////////////////// serial.c:449 START
  if (!filter || strstr("stream_isdone", filter))
  {
    extern void __test__stream_isdone(void);
    i_log_info("========================= TEST CASE: %s\n", "stream_isdone");
    int prev = test_ret;
    test_ret = 0;
    __test__stream_isdone();
    if (!test_ret)
    {
      i_log_passed("%s\n", "stream_isdone");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "stream_isdone";
    }
    ntests++;
  }
  //////////////////// serial.c:449 DONE
  //////////////////// serial.c:475 START
  if (!filter || strstr("stream_close", filter))
  {
    extern void __test__stream_close(void);
    i_log_info("========================= TEST CASE: %s\n", "stream_close");
    int prev = test_ret;
    test_ret = 0;
    __test__stream_close();
    if (!test_ret)
    {
      i_log_passed("%s\n", "stream_close");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "stream_close";
    }
    ntests++;
  }
  //////////////////// serial.c:475 DONE
  //////////////////// serial.c:565 START
  if (!filter || strstr("stream_bread", filter))
  {
    extern void __test__stream_bread(void);
    i_log_info("========================= TEST CASE: %s\n", "stream_bread");
    int prev = test_ret;
    test_ret = 0;
    __test__stream_bread();
    if (!test_ret)
    {
      i_log_passed("%s\n", "stream_bread");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "stream_bread";
    }
    ntests++;
  }
  //////////////////// serial.c:565 DONE
  //////////////////// serial.c:663 START
  if (!filter || strstr("stream_bwrite", filter))
  {
    extern void __test__stream_bwrite(void);
    i_log_info("========================= TEST CASE: %s\n", "stream_bwrite");
    int prev = test_ret;
    test_ret = 0;
    __test__stream_bwrite();
    if (!test_ret)
    {
      i_log_passed("%s\n", "stream_bwrite");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "stream_bwrite";
    }
    ntests++;
  }
  //////////////////// serial.c:663 DONE
  //////////////////// serial.c:778 START
  if (!filter || strstr("stream_read", filter))
  {
    extern void __test__stream_read(void);
    i_log_info("========================= TEST CASE: %s\n", "stream_read");
    int prev = test_ret;
    test_ret = 0;
    __test__stream_read();
    if (!test_ret)
    {
      i_log_passed("%s\n", "stream_read");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "stream_read";
    }
    ntests++;
  }
  //////////////////// serial.c:778 DONE
  //////////////////// serial.c:996 START
  if (!filter || strstr("stream_ibuf", filter))
  {
    extern void __test__stream_ibuf(void);
    i_log_info("========================= TEST CASE: %s\n", "stream_ibuf");
    int prev = test_ret;
    test_ret = 0;
    __test__stream_ibuf();
    if (!test_ret)
    {
      i_log_passed("%s\n", "stream_ibuf");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "stream_ibuf";
    }
    ntests++;
  }
  //////////////////// serial.c:996 DONE
  //////////////////// serial.c:1100 START
  if (!filter || strstr("stream_obuf", filter))
  {
    extern void __test__stream_obuf(void);
    i_log_info("========================= TEST CASE: %s\n", "stream_obuf");
    int prev = test_ret;
    test_ret = 0;
    __test__stream_obuf();
    if (!test_ret)
    {
      i_log_passed("%s\n", "stream_obuf");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "stream_obuf";
    }
    ntests++;
  }
  //////////////////// serial.c:1100 DONE
  //////////////////// serial.c:1184 START
  if (!filter || strstr("stream_read_ibuf_to_obuf", filter))
  {
    extern void __test__stream_read_ibuf_to_obuf(void);
    i_log_info("========================= TEST CASE: %s\n", "stream_read_ibuf_to_obuf");
    int prev = test_ret;
    test_ret = 0;
    __test__stream_read_ibuf_to_obuf();
    if (!test_ret)
    {
      i_log_passed("%s\n", "stream_read_ibuf_to_obuf");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "stream_read_ibuf_to_obuf";
    }
    ntests++;
  }
  //////////////////// serial.c:1184 DONE
  //////////////////// serial.c:1272 START
  if (!filter || strstr("stream_sink", filter))
  {
    extern void __test__stream_sink(void);
    i_log_info("========================= TEST CASE: %s\n", "stream_sink");
    int prev = test_ret;
    test_ret = 0;
    __test__stream_sink();
    if (!test_ret)
    {
      i_log_passed("%s\n", "stream_sink");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "stream_sink";
    }
    ntests++;
  }
  //////////////////// serial.c:1272 DONE
  //////////////////// serial.c:1396 START
  if (!filter || strstr("stream_opsink", filter))
  {
    extern void __test__stream_opsink(void);
    i_log_info("========================= TEST CASE: %s\n", "stream_opsink");
    int prev = test_ret;
    test_ret = 0;
    __test__stream_opsink();
    if (!test_ret)
    {
      i_log_passed("%s\n", "stream_opsink");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "stream_opsink";
    }
    ntests++;
  }
  //////////////////// serial.c:1396 DONE
  //////////////////// serial.c:1586 START
  if (!filter || strstr("stream_limit_pull", filter))
  {
    extern void __test__stream_limit_pull(void);
    i_log_info("========================= TEST CASE: %s\n", "stream_limit_pull");
    int prev = test_ret;
    test_ret = 0;
    __test__stream_limit_pull();
    if (!test_ret)
    {
      i_log_passed("%s\n", "stream_limit_pull");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "stream_limit_pull";
    }
    ntests++;
  }
  //////////////////// serial.c:1586 DONE
  //////////////////// serial.c:1721 START
  if (!filter || strstr("stream_limit_push", filter))
  {
    extern void __test__stream_limit_push(void);
    i_log_info("========================= TEST CASE: %s\n", "stream_limit_push");
    int prev = test_ret;
    test_ret = 0;
    __test__stream_limit_push();
    if (!test_ret)
    {
      i_log_passed("%s\n", "stream_limit_push");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "stream_limit_push";
    }
    ntests++;
  }
  //////////////////// serial.c:1721 DONE
  //////////////////// serial.c:1800 START
  if (!filter || strstr("stream_limit_init", filter))
  {
    extern void __test__stream_limit_init(void);
    i_log_info("========================= TEST CASE: %s\n", "stream_limit_init");
    int prev = test_ret;
    test_ret = 0;
    __test__stream_limit_init();
    if (!test_ret)
    {
      i_log_passed("%s\n", "stream_limit_init");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "stream_limit_init";
    }
    ntests++;
  }
  //////////////////// serial.c:1800 DONE
  //////////////////// htable.c:195 START
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
  //////////////////// htable.c:195 DONE
  //////////////////// htable.c:270 START
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
  //////////////////// htable.c:270 DONE
  //////////////////// htable.c:278 START
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
  //////////////////// htable.c:278 DONE
  //////////////////// htable.c:287 START
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
  //////////////////// htable.c:287 DONE
  //////////////////// htable.c:295 START
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
  //////////////////// htable.c:295 DONE
  //////////////////// numstore.c:107 START
  if (!filter || strstr("nsdb_fexecute", filter))
  {
    extern void __test__nsdb_fexecute(void);
    i_log_info("========================= TEST CASE: %s\n", "nsdb_fexecute");
    int prev = test_ret;
    test_ret = 0;
    __test__nsdb_fexecute();
    if (!test_ret)
    {
      i_log_passed("%s\n", "nsdb_fexecute");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "nsdb_fexecute";
    }
    ntests++;
  }
  //////////////////// numstore.c:107 DONE
  //////////////////// numstore.c:136 START
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
  //////////////////// numstore.c:136 DONE
  //////////////////// numstore.c:182 START
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
  //////////////////// numstore.c:182 DONE
  //////////////////// numstore.c:215 START
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
  //////////////////// numstore.c:215 DONE
  //////////////////// numstore.c:431 START
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
  //////////////////// numstore.c:431 DONE
  //////////////////// numstore.c:614 START
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
  //////////////////// numstore.c:614 DONE
  //////////////////// numstore.c:729 START
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
  //////////////////// numstore.c:729 DONE
  //////////////////// numstore.c:991 START
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
  //////////////////// numstore.c:991 DONE
  //////////////////// variables.c:68 START
  if (!filter || strstr("i_print_variable", filter))
  {
    extern void __test__i_print_variable(void);
    i_log_info("========================= TEST CASE: %s\n", "i_print_variable");
    int prev = test_ret;
    test_ret = 0;
    __test__i_print_variable();
    if (!test_ret)
    {
      i_log_passed("%s\n", "i_print_variable");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_print_variable";
    }
    ntests++;
  }
  //////////////////// variables.c:68 DONE
  //////////////////// variables.c:117 START
  if (!filter || strstr("variable_equal", filter))
  {
    extern void __test__variable_equal(void);
    i_log_info("========================= TEST CASE: %s\n", "variable_equal");
    int prev = test_ret;
    test_ret = 0;
    __test__variable_equal();
    if (!test_ret)
    {
      i_log_passed("%s\n", "variable_equal");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "variable_equal";
    }
    ntests++;
  }
  //////////////////// variables.c:117 DONE
  //////////////////// variables.c:213 START
  if (!filter || strstr("validate_vname", filter))
  {
    extern void __test__validate_vname(void);
    i_log_info("========================= TEST CASE: %s\n", "validate_vname");
    int prev = test_ret;
    test_ret = 0;
    __test__validate_vname();
    if (!test_ret)
    {
      i_log_passed("%s\n", "validate_vname");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "validate_vname";
    }
    ntests++;
  }
  //////////////////// variables.c:213 DONE
  //////////////////// variables.c:291 START
  if (!filter || strstr("var_random_name", filter))
  {
    extern void __test__var_random_name(void);
    i_log_info("========================= TEST CASE: %s\n", "var_random_name");
    int prev = test_ret;
    test_ret = 0;
    __test__var_random_name();
    if (!test_ret)
    {
      i_log_passed("%s\n", "var_random_name");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "var_random_name";
    }
    ntests++;
  }
  //////////////////// variables.c:291 DONE
  //////////////////// variables.c:358 START
  if (!filter || strstr("rand_varname", filter))
  {
    extern void __test__rand_varname(void);
    i_log_info("========================= TEST CASE: %s\n", "rand_varname");
    int prev = test_ret;
    test_ret = 0;
    __test__rand_varname();
    if (!test_ret)
    {
      i_log_passed("%s\n", "rand_varname");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "rand_varname";
    }
    ntests++;
  }
  //////////////////// variables.c:358 DONE
  //////////////////// variables.c:496 START
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
  //////////////////// variables.c:496 DONE
  //////////////////// variables.c:513 START
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
  //////////////////// variables.c:513 DONE
  //////////////////// variables.c:532 START
  if (!filter || strstr("var_resolve_index", filter))
  {
    extern void __test__var_resolve_index(void);
    i_log_info("========================= TEST CASE: %s\n", "var_resolve_index");
    int prev = test_ret;
    test_ret = 0;
    __test__var_resolve_index();
    if (!test_ret)
    {
      i_log_passed("%s\n", "var_resolve_index");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "var_resolve_index";
    }
    ntests++;
  }
  //////////////////// variables.c:532 DONE
  //////////////////// variables.c:560 START
  if (!filter || strstr("var_resolve_nelem", filter))
  {
    extern void __test__var_resolve_nelem(void);
    i_log_info("========================= TEST CASE: %s\n", "var_resolve_nelem");
    int prev = test_ret;
    test_ret = 0;
    __test__var_resolve_nelem();
    if (!test_ret)
    {
      i_log_passed("%s\n", "var_resolve_nelem");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "var_resolve_nelem";
    }
    ntests++;
  }
  //////////////////// variables.c:560 DONE
  //////////////////// file_pager.c:123 START
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
  //////////////////// file_pager.c:123 DONE
  //////////////////// file_pager.c:232 START
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
  //////////////////// file_pager.c:232 DONE
  //////////////////// file_pager.c:411 START
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
  //////////////////// file_pager.c:411 DONE
  //////////////////// parsers.c:163 START
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
  //////////////////// parsers.c:163 DONE
  //////////////////// parsers.c:372 START
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
  //////////////////// parsers.c:372 DONE
  //////////////////// parsers.c:1003 START
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
  //////////////////// parsers.c:1003 DONE
  //////////////////// parsers.c:1675 START
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
  //////////////////// parsers.c:1675 DONE
  //////////////////// parsers.c:2259 START
  if (!filter || strstr("compile_subtype", filter))
  {
    extern void __test__compile_subtype(void);
    i_log_info("========================= TEST CASE: %s\n", "compile_subtype");
    int prev = test_ret;
    test_ret = 0;
    __test__compile_subtype();
    if (!test_ret)
    {
      i_log_passed("%s\n", "compile_subtype");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "compile_subtype";
    }
    ntests++;
  }
  //////////////////// parsers.c:2259 DONE
  //////////////////// parsers.c:2526 START
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
  //////////////////// parsers.c:2526 DONE
  //////////////////// collections.c:32 START
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
  //////////////////// collections.c:32 DONE
  //////////////////// collections.c:84 START
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
  //////////////////// collections.c:84 DONE
  //////////////////// collections.c:94 START
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
  //////////////////// collections.c:94 DONE
  //////////////////// collections.c:104 START
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
  //////////////////// collections.c:104 DONE
  //////////////////// collections.c:154 START
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
  //////////////////// collections.c:154 DONE
  //////////////////// collections.c:227 START
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
  //////////////////// collections.c:227 DONE
  //////////////////// collections.c:294 START
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
  //////////////////// collections.c:294 DONE
  //////////////////// collections.c:381 START
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
  //////////////////// collections.c:381 DONE
  //////////////////// collections.c:502 START
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
  //////////////////// collections.c:502 DONE
  //////////////////// collections.c:574 START
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
  //////////////////// collections.c:574 DONE
  //////////////////// collections.c:663 START
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
  //////////////////// collections.c:663 DONE
  //////////////////// collections.c:757 START
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
  //////////////////// collections.c:757 DONE
  //////////////////// collections.c:866 START
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
  //////////////////// collections.c:866 DONE
  //////////////////// collections.c:1027 START
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
  //////////////////// collections.c:1027 DONE
  //////////////////// collections.c:1129 START
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
  //////////////////// collections.c:1129 DONE
  //////////////////// collections.c:1160 START
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
  //////////////////// collections.c:1160 DONE
  //////////////////// collections.c:1216 START
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
  //////////////////// collections.c:1216 DONE
  //////////////////// collections.c:1290 START
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
  //////////////////// collections.c:1290 DONE
  //////////////////// collections.c:1351 START
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
  //////////////////// collections.c:1351 DONE
  //////////////////// collections.c:1422 START
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
  //////////////////// collections.c:1422 DONE
  //////////////////// collections.c:1493 START
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
  //////////////////// collections.c:1493 DONE
  //////////////////// collections.c:1630 START
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
  //////////////////// collections.c:1630 DONE
  //////////////////// collections.c:1648 START
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
  //////////////////// collections.c:1648 DONE
  //////////////////// collections.c:1671 START
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
  //////////////////// collections.c:1671 DONE
  //////////////////// collections.c:1695 START
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
  //////////////////// collections.c:1695 DONE
  //////////////////// collections.c:1734 START
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
  //////////////////// collections.c:1734 DONE
  //////////////////// collections.c:1761 START
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
  //////////////////// collections.c:1761 DONE
  //////////////////// collections.c:1794 START
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
  //////////////////// collections.c:1794 DONE
  //////////////////// collections.c:1816 START
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
  //////////////////// collections.c:1816 DONE
  //////////////////// collections.c:1837 START
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
  //////////////////// collections.c:1837 DONE
  //////////////////// collections.c:1867 START
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
  //////////////////// collections.c:1867 DONE
  //////////////////// collections.c:1885 START
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
  //////////////////// collections.c:1885 DONE
  //////////////////// collections.c:2167 START
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
  //////////////////// collections.c:2167 DONE
  //////////////////// collections.c:2306 START
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
  //////////////////// collections.c:2306 DONE
  //////////////////// collections.c:2412 START
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
  //////////////////// collections.c:2412 DONE
  //////////////////// collections.c:2579 START
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
  //////////////////// collections.c:2579 DONE
  //////////////////// collections.c:3308 START
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
  //////////////////// collections.c:3308 DONE
  //////////////////// collections.c:3566 START
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
  //////////////////// collections.c:3566 DONE
  //////////////////// collections.c:3711 START
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
  //////////////////// collections.c:3711 DONE
  //////////////////// collections.c:3874 START
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
  //////////////////// collections.c:3874 DONE
  //////////////////// collections.c:3963 START
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
  //////////////////// collections.c:3963 DONE
  //////////////////// collections.c:4293 START
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
  //////////////////// collections.c:4293 DONE
  //////////////////// wal.c:70 START
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
  //////////////////// wal.c:70 DONE
  //////////////////// wal.c:323 START
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
  //////////////////// wal.c:323 DONE
  //////////////////// wal.c:1460 START
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
  //////////////////// wal.c:1460 DONE
  //////////////////// wal.c:2995 START
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
  //////////////////// wal.c:2995 DONE
  //////////////////// wal.c:3208 START
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
  //////////////////// wal.c:3208 DONE
  //////////////////// wal.c:3326 START
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
  //////////////////// wal.c:3326 DONE
  //////////////////// dirty_page_table.c:409 START
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
  //////////////////// dirty_page_table.c:409 DONE
  //////////////////// dirty_page_table.c:429 START
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
  //////////////////// dirty_page_table.c:429 DONE
  //////////////////// dirty_page_table.c:494 START
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
  //////////////////// dirty_page_table.c:494 DONE
  //////////////////// dirty_page_table.c:524 START
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
  //////////////////// dirty_page_table.c:524 DONE
  //////////////////// dirty_page_table.c:547 START
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
  //////////////////// dirty_page_table.c:547 DONE
  //////////////////// dirty_page_table.c:584 START
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
  //////////////////// dirty_page_table.c:584 DONE
  //////////////////// dirty_page_table.c:650 START
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
  //////////////////// dirty_page_table.c:650 DONE
  //////////////////// dirty_page_table.c:714 START
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
  //////////////////// dirty_page_table.c:714 DONE
  //////////////////// dirty_page_table.c:871 START
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
  //////////////////// dirty_page_table.c:871 DONE
  //////////////////// alloc.c:373 START
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
  //////////////////// alloc.c:373 DONE
  //////////////////// alloc.c:489 START
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
  //////////////////// alloc.c:489 DONE
  //////////////////// alloc.c:525 START
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
  //////////////////// alloc.c:525 DONE
  //////////////////// alloc.c:554 START
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
  //////////////////// alloc.c:554 DONE
  //////////////////// alloc.c:597 START
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
  //////////////////// alloc.c:597 DONE
  //////////////////// alloc.c:649 START
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
  //////////////////// alloc.c:649 DONE
  //////////////////// alloc.c:693 START
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
  //////////////////// alloc.c:693 DONE
  //////////////////// alloc.c:739 START
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
  //////////////////// alloc.c:739 DONE
  //////////////////// alloc.c:768 START
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
  //////////////////// alloc.c:768 DONE
  //////////////////// numerics.c:114 START
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
  //////////////////// numerics.c:114 DONE
  //////////////////// numerics.c:125 START
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
  //////////////////// numerics.c:125 DONE
  //////////////////// numerics.c:137 START
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
  //////////////////// numerics.c:137 DONE
  //////////////////// numerics.c:169 START
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
  //////////////////// numerics.c:169 DONE
  //////////////////// numerics.c:193 START
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
  //////////////////// numerics.c:193 DONE
  //////////////////// numerics.c:255 START
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
  //////////////////// numerics.c:255 DONE
  //////////////////// numerics.c:387 START
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
  //////////////////// numerics.c:387 DONE
  //////////////////// numerics.c:436 START
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
  //////////////////// numerics.c:436 DONE
  //////////////////// numerics.c:498 START
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
  //////////////////// numerics.c:498 DONE
  //////////////////// numerics.c:520 START
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
  //////////////////// numerics.c:520 DONE
  //////////////////// numerics.c:559 START
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
  //////////////////// numerics.c:559 DONE
  //////////////////// numerics.c:689 START
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
  //////////////////// numerics.c:689 DONE
  //////////////////// numerics.c:840 START
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
  //////////////////// numerics.c:840 DONE
  //////////////////// numerics.c:880 START
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
  //////////////////// numerics.c:880 DONE
  //////////////////// numerics.c:917 START
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
  //////////////////// numerics.c:917 DONE
  //////////////////// pager.c:222 START
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
  //////////////////// pager.c:222 DONE
  //////////////////// pager.c:276 START
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
  //////////////////// pager.c:276 DONE
  //////////////////// pager.c:331 START
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
  //////////////////// pager.c:331 DONE
  //////////////////// pager.c:1321 START
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
  //////////////////// pager.c:1321 DONE
  //////////////////// pager.c:1363 START
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
  //////////////////// pager.c:1363 DONE
  //////////////////// pager.c:1457 START
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
  //////////////////// pager.c:1457 DONE
  //////////////////// pager.c:1564 START
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
  //////////////////// pager.c:1564 DONE
  //////////////////// pager.c:1964 START
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
  //////////////////// pager.c:1964 DONE
  //////////////////// pager.c:2103 START
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
  //////////////////// pager.c:2103 DONE
  //////////////////// pager.c:2676 START
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
  //////////////////// pager.c:2676 DONE
  //////////////////// pager.c:2757 START
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
  //////////////////// pager.c:2757 DONE
  //////////////////// pager.c:3034 START
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
  //////////////////// pager.c:3034 DONE
  //////////////////// pager.c:3107 START
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
  //////////////////// pager.c:3107 DONE
  //////////////////// pager.c:3186 START
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
  //////////////////// pager.c:3186 DONE
  //////////////////// pager.c:3247 START
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
  //////////////////// pager.c:3247 DONE
  //////////////////// pager.c:3325 START
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
  //////////////////// pager.c:3325 DONE
  //////////////////// robin_hood_ht.c:27 START
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
  //////////////////// robin_hood_ht.c:27 DONE
  //////////////////// robin_hood_ht.c:105 START
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
  //////////////////// robin_hood_ht.c:105 DONE
  //////////////////// os_common.c:74 START
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
  //////////////////// os_common.c:74 DONE
  //////////////////// os_common.c:168 START
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
  //////////////////// os_common.c:168 DONE
  //////////////////// os_common.c:209 START
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
  //////////////////// os_common.c:209 DONE
  //////////////////// os_common.c:308 START
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
  //////////////////// os_common.c:308 DONE
  //////////////////// os_common.c:385 START
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
  //////////////////// os_common.c:385 DONE
  //////////////////// os_common.c:491 START
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
  //////////////////// os_common.c:491 DONE
  //////////////////// var_algorithms.c:77 START
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
  //////////////////// var_algorithms.c:77 DONE
  //////////////////// var_algorithms.c:468 START
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
  //////////////////// var_algorithms.c:468 DONE
  //////////////////// var_algorithms.c:1636 START
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
  //////////////////// var_algorithms.c:1636 DONE
  //////////////////// page.c:148 START
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
  //////////////////// page.c:148 DONE
  //////////////////// page.c:228 START
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
  //////////////////// page.c:228 DONE
  //////////////////// page.c:300 START
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
  //////////////////// page.c:300 DONE
  //////////////////// page.c:377 START
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
  //////////////////// page.c:377 DONE
  //////////////////// page.c:459 START
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
  //////////////////// page.c:459 DONE
  //////////////////// page.c:522 START
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
  //////////////////// page.c:522 DONE
  //////////////////// page.c:619 START
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
  //////////////////// page.c:619 DONE
  //////////////////// page.c:805 START
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
  //////////////////// page.c:805 DONE
  //////////////////// page.c:903 START
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
  //////////////////// page.c:903 DONE
  //////////////////// page.c:983 START
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
  //////////////////// page.c:983 DONE
  //////////////////// page.c:1060 START
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
  //////////////////// page.c:1060 DONE
  //////////////////// page.c:1140 START
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
  //////////////////// page.c:1140 DONE
  //////////////////// page.c:1230 START
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
  //////////////////// page.c:1230 DONE
  //////////////////// page.c:1340 START
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
  //////////////////// page.c:1340 DONE
  //////////////////// page.c:1373 START
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
  //////////////////// page.c:1373 DONE
  //////////////////// page.c:1566 START
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
  //////////////////// page.c:1566 DONE
  //////////////////// page.c:1627 START
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
  //////////////////// page.c:1627 DONE
  //////////////////// page.c:1685 START
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
  //////////////////// page.c:1685 DONE
  //////////////////// page.c:1750 START
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
  //////////////////// page.c:1750 DONE
  //////////////////// page.c:1881 START
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
  //////////////////// page.c:1881 DONE
  //////////////////// page.c:1918 START
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
  //////////////////// page.c:1918 DONE
  //////////////////// page.c:1946 START
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
  //////////////////// page.c:1946 DONE
  //////////////////// page.c:1969 START
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
  //////////////////// page.c:1969 DONE
  //////////////////// page.c:2028 START
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
  //////////////////// page.c:2028 DONE
  //////////////////// page.c:2059 START
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
  //////////////////// page.c:2059 DONE
  //////////////////// page.c:2072 START
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
  //////////////////// page.c:2072 DONE
  //////////////////// page.c:2132 START
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
  //////////////////// page.c:2132 DONE
  //////////////////// page.c:2169 START
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
  //////////////////// page.c:2169 DONE
  //////////////////// page.c:2197 START
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
  //////////////////// page.c:2197 DONE
  //////////////////// page.c:2220 START
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
  //////////////////// page.c:2220 DONE
  //////////////////// page.c:2277 START
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
  //////////////////// page.c:2277 DONE
  //////////////////// page.c:2375 START
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
  //////////////////// page.c:2375 DONE
  //////////////////// page.c:2413 START
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
  //////////////////// page.c:2413 DONE
  //////////////////// page.c:2431 START
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
  //////////////////// page.c:2431 DONE
  //////////////////// page.c:2444 START
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
  //////////////////// page.c:2444 DONE
  //////////////////// page.c:2516 START
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
  //////////////////// page.c:2516 DONE
  //////////////////// page.c:2590 START
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
  //////////////////// page.c:2590 DONE
  //////////////////// page.c:2629 START
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
  //////////////////// page.c:2629 DONE
  //////////////////// page.c:2787 START
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
  //////////////////// page.c:2787 DONE
  //////////////////// page.c:2885 START
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
  //////////////////// page.c:2885 DONE
  //////////////////// page.c:2918 START
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
  //////////////////// page.c:2918 DONE
  //////////////////// page.c:2943 START
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
  //////////////////// page.c:2943 DONE
  //////////////////// page.c:2978 START
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
  //////////////////// page.c:2978 DONE
  //////////////////// utils.c:30 START
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
  //////////////////// utils.c:30 DONE
  //////////////////// testing.c:72 START
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
  //////////////////// testing.c:72 DONE
  //////////////////// testing.c:83 START
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
  //////////////////// testing.c:83 DONE
  //////////////////// error.c:113 START
  if (!filter || strstr("error_log_consume", filter))
  {
    extern void __test__error_log_consume(void);
    i_log_info("========================= TEST CASE: %s\n", "error_log_consume");
    int prev = test_ret;
    test_ret = 0;
    __test__error_log_consume();
    if (!test_ret)
    {
      i_log_passed("%s\n", "error_log_consume");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "error_log_consume";
    }
    ntests++;
  }
  //////////////////// error.c:113 DONE
  //////////////////// smartfiles.c:34 START
  if (!filter || strstr("smfile_perror", filter))
  {
    extern void __test__smfile_perror(void);
    i_log_info("========================= TEST CASE: %s\n", "smfile_perror");
    int prev = test_ret;
    test_ret = 0;
    __test__smfile_perror();
    if (!test_ret)
    {
      i_log_passed("%s\n", "smfile_perror");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "smfile_perror";
    }
    ntests++;
  }
  //////////////////// smartfiles.c:34 DONE
  //////////////////// smartfiles.c:56 START
  if (!filter || strstr("smfile_strerror", filter))
  {
    extern void __test__smfile_strerror(void);
    i_log_info("========================= TEST CASE: %s\n", "smfile_strerror");
    int prev = test_ret;
    test_ret = 0;
    __test__smfile_strerror();
    if (!test_ret)
    {
      i_log_passed("%s\n", "smfile_strerror");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "smfile_strerror";
    }
    ntests++;
  }
  //////////////////// smartfiles.c:56 DONE
  //////////////////// smartfiles.c:80 START
  if (!filter || strstr("smfile_cleanup", filter))
  {
    extern void __test__smfile_cleanup(void);
    i_log_info("========================= TEST CASE: %s\n", "smfile_cleanup");
    int prev = test_ret;
    test_ret = 0;
    __test__smfile_cleanup();
    if (!test_ret)
    {
      i_log_passed("%s\n", "smfile_cleanup");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "smfile_cleanup";
    }
    ntests++;
  }
  //////////////////// smartfiles.c:80 DONE
  //////////////////// smartfiles.c:152 START
  if (!filter || strstr("smfile_size", filter))
  {
    extern void __test__smfile_size(void);
    i_log_info("========================= TEST CASE: %s\n", "smfile_size");
    int prev = test_ret;
    test_ret = 0;
    __test__smfile_size();
    if (!test_ret)
    {
      i_log_passed("%s\n", "smfile_size");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "smfile_size";
    }
    ntests++;
  }
  //////////////////// smartfiles.c:152 DONE
  //////////////////// smartfiles.c:180 START
  if (!filter || strstr("smfile_close", filter))
  {
    extern void __test__smfile_close(void);
    i_log_info("========================= TEST CASE: %s\n", "smfile_close");
    int prev = test_ret;
    test_ret = 0;
    __test__smfile_close();
    if (!test_ret)
    {
      i_log_passed("%s\n", "smfile_close");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "smfile_close";
    }
    ntests++;
  }
  //////////////////// smartfiles.c:180 DONE
  //////////////////// smartfiles.c:203 START
  if (!filter || strstr("smfile_crash", filter))
  {
    extern void __test__smfile_crash(void);
    i_log_info("========================= TEST CASE: %s\n", "smfile_crash");
    int prev = test_ret;
    test_ret = 0;
    __test__smfile_crash();
    if (!test_ret)
    {
      i_log_passed("%s\n", "smfile_crash");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "smfile_crash";
    }
    ntests++;
  }
  //////////////////// smartfiles.c:203 DONE
  //////////////////// smartfiles.c:238 START
  if (!filter || strstr("smfile_txns", filter))
  {
    extern void __test__smfile_txns(void);
    i_log_info("========================= TEST CASE: %s\n", "smfile_txns");
    int prev = test_ret;
    test_ret = 0;
    __test__smfile_txns();
    if (!test_ret)
    {
      i_log_passed("%s\n", "smfile_txns");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "smfile_txns";
    }
    ntests++;
  }
  //////////////////// smartfiles.c:238 DONE
  //////////////////// smartfiles.c:313 START
  if (!filter || strstr("smfile_open", filter))
  {
    extern void __test__smfile_open(void);
    i_log_info("========================= TEST CASE: %s\n", "smfile_open");
    int prev = test_ret;
    test_ret = 0;
    __test__smfile_open();
    if (!test_ret)
    {
      i_log_passed("%s\n", "smfile_open");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "smfile_open";
    }
    ntests++;
  }
  //////////////////// smartfiles.c:313 DONE
  //////////////////// smartfiles.c:429 START
  if (!filter || strstr("smfile_insert", filter))
  {
    extern void __test__smfile_insert(void);
    i_log_info("========================= TEST CASE: %s\n", "smfile_insert");
    int prev = test_ret;
    test_ret = 0;
    __test__smfile_insert();
    if (!test_ret)
    {
      i_log_passed("%s\n", "smfile_insert");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "smfile_insert";
    }
    ntests++;
  }
  //////////////////// smartfiles.c:429 DONE
  //////////////////// smartfiles.c:563 START
  if (!filter || strstr("smfile_read", filter))
  {
    extern void __test__smfile_read(void);
    i_log_info("========================= TEST CASE: %s\n", "smfile_read");
    int prev = test_ret;
    test_ret = 0;
    __test__smfile_read();
    if (!test_ret)
    {
      i_log_passed("%s\n", "smfile_read");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "smfile_read";
    }
    ntests++;
  }
  //////////////////// smartfiles.c:563 DONE
  //////////////////// smartfiles.c:727 START
  if (!filter || strstr("smfile_remove", filter))
  {
    extern void __test__smfile_remove(void);
    i_log_info("========================= TEST CASE: %s\n", "smfile_remove");
    int prev = test_ret;
    test_ret = 0;
    __test__smfile_remove();
    if (!test_ret)
    {
      i_log_passed("%s\n", "smfile_remove");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "smfile_remove";
    }
    ntests++;
  }
  //////////////////// smartfiles.c:727 DONE
  //////////////////// smartfiles.c:922 START
  if (!filter || strstr("smfile_pwrite", filter))
  {
    extern void __test__smfile_pwrite(void);
    i_log_info("========================= TEST CASE: %s\n", "smfile_pwrite");
    int prev = test_ret;
    test_ret = 0;
    __test__smfile_pwrite();
    if (!test_ret)
    {
      i_log_passed("%s\n", "smfile_pwrite");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "smfile_pwrite";
    }
    ntests++;
  }
  //////////////////// smartfiles.c:922 DONE
  //////////////////// tests.c:53 START
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
  //////////////////// tests.c:53 DONE
  //////////////////// tests.c:72 START
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
  //////////////////// tests.c:72 DONE
  //////////////////// tests.c:79 START
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
  //////////////////// tests.c:79 DONE
  //////////////////// tests.c:111 START
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
  //////////////////// tests.c:111 DONE
  //////////////////// tests.c:165 START
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
  //////////////////// tests.c:165 DONE
  //////////////////// tests.c:200 START
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
  //////////////////// tests.c:200 DONE
  //////////////////// tests.c:231 START
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
  //////////////////// tests.c:231 DONE
  //////////////////// tests.c:270 START
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
  //////////////////// tests.c:270 DONE
  //////////////////// tests.c:294 START
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
  //////////////////// tests.c:294 DONE
  //////////////////// tests.c:328 START
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
  //////////////////// tests.c:328 DONE
  //////////////////// tests.c:365 START
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
  //////////////////// tests.c:365 DONE
  //////////////////// tests.c:387 START
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
  //////////////////// tests.c:387 DONE
  //////////////////// tests.c:396 START
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
  //////////////////// tests.c:396 DONE
  //////////////////// tests.c:409 START
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
  //////////////////// tests.c:409 DONE
  //////////////////// tests.c:428 START
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
  //////////////////// tests.c:428 DONE
  //////////////////// tests.c:447 START
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
  //////////////////// tests.c:447 DONE
  //////////////////// tests.c:522 START
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
  //////////////////// tests.c:522 DONE
  //////////////////// tests.c:563 START
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
  //////////////////// tests.c:563 DONE
  //////////////////// tests.c:620 START
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
  //////////////////// tests.c:620 DONE
  //////////////////// tests.c:658 START
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
  //////////////////// tests.c:658 DONE
  //////////////////// tests.c:727 START
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
  //////////////////// tests.c:727 DONE
  //////////////////// tests.c:764 START
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
  //////////////////// tests.c:764 DONE
  //////////////////// tests.c:823 START
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
  //////////////////// tests.c:823 DONE
  //////////////////// concurrency.c:55 START
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
  //////////////////// concurrency.c:55 DONE
  //////////////////// concurrency.c:100 START
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
  //////////////////// concurrency.c:100 DONE
  //////////////////// concurrency.c:146 START
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
  //////////////////// concurrency.c:146 DONE
  //////////////////// concurrency.c:356 START
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
  //////////////////// concurrency.c:356 DONE
  //////////////////// concurrency.c:454 START
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
  //////////////////// concurrency.c:454 DONE
  //////////////////// concurrency.c:613 START
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
  //////////////////// concurrency.c:613 DONE
  //////////////////// concurrency.c:630 START
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
  //////////////////// concurrency.c:630 DONE
  //////////////////// concurrency.c:686 START
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
  //////////////////// concurrency.c:686 DONE
  //////////////////// concurrency.c:876 START
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
  //////////////////// concurrency.c:876 DONE
  //////////////////// types.c:69 START
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
  //////////////////// types.c:69 DONE
  //////////////////// types.c:135 START
  if (!filter || strstr("struct_t_validate_shallow", filter))
  {
    extern void __test__struct_t_validate_shallow(void);
    i_log_info("========================= TEST CASE: %s\n", "struct_t_validate_shallow");
    int prev = test_ret;
    test_ret = 0;
    __test__struct_t_validate_shallow();
    if (!test_ret)
    {
      i_log_passed("%s\n", "struct_t_validate_shallow");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "struct_t_validate_shallow";
    }
    ntests++;
  }
  //////////////////// types.c:135 DONE
  //////////////////// types.c:272 START
  if (!filter || strstr("union_t_validate_shallow", filter))
  {
    extern void __test__union_t_validate_shallow(void);
    i_log_info("========================= TEST CASE: %s\n", "union_t_validate_shallow");
    int prev = test_ret;
    test_ret = 0;
    __test__union_t_validate_shallow();
    if (!test_ret)
    {
      i_log_passed("%s\n", "union_t_validate_shallow");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "union_t_validate_shallow";
    }
    ntests++;
  }
  //////////////////// types.c:272 DONE
  //////////////////// types.c:400 START
  if (!filter || strstr("sarray_t_validate_shallow", filter))
  {
    extern void __test__sarray_t_validate_shallow(void);
    i_log_info("========================= TEST CASE: %s\n", "sarray_t_validate_shallow");
    int prev = test_ret;
    test_ret = 0;
    __test__sarray_t_validate_shallow();
    if (!test_ret)
    {
      i_log_passed("%s\n", "sarray_t_validate_shallow");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "sarray_t_validate_shallow";
    }
    ntests++;
  }
  //////////////////// types.c:400 DONE
  //////////////////// types.c:545 START
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
  //////////////////// types.c:545 DONE
  //////////////////// types.c:699 START
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
  //////////////////// types.c:699 DONE
  //////////////////// types.c:859 START
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
  //////////////////// types.c:859 DONE
  //////////////////// types.c:957 START
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
  //////////////////// types.c:957 DONE
  //////////////////// types.c:1083 START
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
  //////////////////// types.c:1083 DONE
  //////////////////// types.c:1112 START
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
  //////////////////// types.c:1112 DONE
  //////////////////// types.c:1183 START
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
  //////////////////// types.c:1183 DONE
  //////////////////// types.c:1251 START
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
  //////////////////// types.c:1251 DONE
  //////////////////// types.c:1442 START
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
  //////////////////// types.c:1442 DONE
  //////////////////// types.c:1602 START
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
  //////////////////// types.c:1602 DONE
  //////////////////// types.c:1670 START
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
  //////////////////// types.c:1670 DONE
  //////////////////// types.c:1733 START
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
  //////////////////// types.c:1733 DONE
  //////////////////// types.c:1794 START
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
  //////////////////// types.c:1794 DONE
  //////////////////// types.c:1834 START
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
  //////////////////// types.c:1834 DONE
  //////////////////// types.c:1927 START
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
  //////////////////// types.c:1927 DONE
  //////////////////// types.c:2015 START
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
  //////////////////// types.c:2015 DONE
  //////////////////// types.c:2114 START
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
  //////////////////// types.c:2114 DONE
  //////////////////// types.c:2214 START
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
  //////////////////// types.c:2214 DONE
  //////////////////// types.c:2269 START
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
  //////////////////// types.c:2269 DONE
  //////////////////// types.c:2410 START
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
  //////////////////// types.c:2410 DONE
  //////////////////// types.c:2465 START
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
  //////////////////// types.c:2465 DONE
  //////////////////// types.c:2559 START
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
  //////////////////// types.c:2559 DONE
  //////////////////// types.c:2593 START
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
  //////////////////// types.c:2593 DONE
  //////////////////// types.c:2688 START
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
  //////////////////// types.c:2688 DONE
  //////////////////// types.c:3299 START
  if (!filter || strstr("struct_t_resolve_key", filter))
  {
    extern void __test__struct_t_resolve_key(void);
    i_log_info("========================= TEST CASE: %s\n", "struct_t_resolve_key");
    int prev = test_ret;
    test_ret = 0;
    __test__struct_t_resolve_key();
    if (!test_ret)
    {
      i_log_passed("%s\n", "struct_t_resolve_key");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "struct_t_resolve_key";
    }
    ntests++;
  }
  //////////////////// types.c:3299 DONE
  //////////////////// types.c:3430 START
  if (!filter || strstr("union_t_resolve_key", filter))
  {
    extern void __test__union_t_resolve_key(void);
    i_log_info("========================= TEST CASE: %s\n", "union_t_resolve_key");
    int prev = test_ret;
    test_ret = 0;
    __test__union_t_resolve_key();
    if (!test_ret)
    {
      i_log_passed("%s\n", "union_t_resolve_key");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "union_t_resolve_key";
    }
    ntests++;
  }
  //////////////////// types.c:3430 DONE
  //////////////////// types.c:3644 START
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
  //////////////////// types.c:3644 DONE
  //////////////////// types.c:3897 START
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
  //////////////////// types.c:3897 DONE
  //////////////////// types.c:4482 START
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
  //////////////////// types.c:4482 DONE
  //////////////////// types.c:4855 START
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
  //////////////////// types.c:4855 DONE
  //////////////////// types.c:4970 START
  if (!filter || strstr("type_ref_equal", filter))
  {
    extern void __test__type_ref_equal(void);
    i_log_info("========================= TEST CASE: %s\n", "type_ref_equal");
    int prev = test_ret;
    test_ret = 0;
    __test__type_ref_equal();
    if (!test_ret)
    {
      i_log_passed("%s\n", "type_ref_equal");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "type_ref_equal";
    }
    ntests++;
  }
  //////////////////// types.c:4970 DONE
  //////////////////// types.c:5115 START
  if (!filter || strstr("tr_construct", filter))
  {
    extern void __test__tr_construct(void);
    i_log_info("========================= TEST CASE: %s\n", "tr_construct");
    int prev = test_ret;
    test_ret = 0;
    __test__tr_construct();
    if (!test_ret)
    {
      i_log_passed("%s\n", "tr_construct");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "tr_construct";
    }
    ntests++;
  }
  //////////////////// types.c:5115 DONE
  //////////////////// types.c:5195 START
  if (!filter || strstr("subtype_equal", filter))
  {
    extern void __test__subtype_equal(void);
    i_log_info("========================= TEST CASE: %s\n", "subtype_equal");
    int prev = test_ret;
    test_ret = 0;
    __test__subtype_equal();
    if (!test_ret)
    {
      i_log_passed("%s\n", "subtype_equal");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "subtype_equal";
    }
    ntests++;
  }
  //////////////////// types.c:5195 DONE
  //////////////////// types.c:5252 START
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
  //////////////////// types.c:5252 DONE
  //////////////////// types.c:5478 START
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
  //////////////////// types.c:5478 DONE
  //////////////////// types.c:5621 START
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
  //////////////////// types.c:5621 DONE
  //////////////////// types.c:5707 START
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
  //////////////////// types.c:5707 DONE
  //////////////////// types.c:5813 START
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
  //////////////////// types.c:5813 DONE
  //////////////////// types.c:5837 START
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
  //////////////////// types.c:5837 DONE
  //////////////////// types.c:5983 START
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
  //////////////////// types.c:5983 DONE
  //////////////////// types.c:6009 START
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
  //////////////////// types.c:6009 DONE
  //////////////////// stride.c:160 START
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
  //////////////////// stride.c:160 DONE
  //////////////////// compiler.c:74 START
  if (!filter || strstr("tt_tostr", filter))
  {
    extern void __test__tt_tostr(void);
    i_log_info("========================= TEST CASE: %s\n", "tt_tostr");
    int prev = test_ret;
    test_ret = 0;
    __test__tt_tostr();
    if (!test_ret)
    {
      i_log_passed("%s\n", "tt_tostr");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "tt_tostr";
    }
    ntests++;
  }
  //////////////////// compiler.c:74 DONE
  //////////////////// compiler.c:611 START
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
  //////////////////// compiler.c:611 DONE
  //////////////////// compiler.c:637 START
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
  //////////////////// compiler.c:637 DONE
  //////////////////// compiler.c:665 START
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
  //////////////////// compiler.c:665 DONE
  //////////////////// compiler.c:678 START
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
  //////////////////// compiler.c:678 DONE
  //////////////////// compiler.c:692 START
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
  //////////////////// compiler.c:692 DONE
  //////////////////// compiler.c:708 START
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
  //////////////////// compiler.c:708 DONE
  //////////////////// compiler.c:735 START
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
  //////////////////// compiler.c:735 DONE
  //////////////////// compiler.c:755 START
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
  //////////////////// compiler.c:755 DONE
  //////////////////// compiler.c:773 START
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
  //////////////////// compiler.c:773 DONE
  //////////////////// compiler.c:795 START
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
  //////////////////// compiler.c:795 DONE
  //////////////////// compiler.c:811 START
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
  //////////////////// compiler.c:811 DONE
  //////////////////// compiler.c:823 START
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
  //////////////////// compiler.c:823 DONE
  //////////////////// compiler.c:834 START
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
  //////////////////// compiler.c:834 DONE
  //////////////////// mem_vhmap.c:255 START
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
  //////////////////// mem_vhmap.c:255 DONE
  //////////////////// txn_table.c:269 START
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
  //////////////////// txn_table.c:269 DONE
  //////////////////// txn_table.c:376 START
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
  //////////////////// txn_table.c:376 DONE
  //////////////////// txn_table.c:522 START
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
  //////////////////// txn_table.c:522 DONE
  //////////////////// txn_table.c:663 START
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
  //////////////////// txn_table.c:663 DONE
  //////////////////// txn_table.c:791 START
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
  //////////////////// txn_table.c:791 DONE
  //////////////////// txn_table.c:899 START
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
  //////////////////// txn_table.c:899 DONE
  //////////////////// txn_table.c:956 START
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
  //////////////////// txn_table.c:956 DONE
  //////////////////// txn_table.c:1109 START
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
  //////////////////// txn_table.c:1109 DONE
  //////////////////// txn_table.c:1293 START
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
  //////////////////// txn_table.c:1293 DONE
  //////////////////// txn_table.c:1469 START
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
  //////////////////// txn_table.c:1469 DONE
  //////////////////// txn_table.c:1659 START
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
  //////////////////// txn_table.c:1659 DONE
  //////////////////// page_fixture.c:387 START
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
  //////////////////// page_fixture.c:387 DONE
  //////////////////// rope_algorithms.c:90 START
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
  //////////////////// rope_algorithms.c:90 DONE
  //////////////////// rope_algorithms.c:270 START
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
  //////////////////// rope_algorithms.c:270 DONE
  //////////////////// rope_algorithms.c:901 START
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
  //////////////////// rope_algorithms.c:901 DONE
  //////////////////// rope_algorithms.c:1074 START
  if (!filter || strstr("ns_insert_from_empty", filter))
  {
    extern void __test__ns_insert_from_empty(void);
    i_log_info("========================= TEST CASE: %s\n", "ns_insert_from_empty");
    int prev = test_ret;
    test_ret = 0;
    __test__ns_insert_from_empty();
    if (!test_ret)
    {
      i_log_passed("%s\n", "ns_insert_from_empty");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ns_insert_from_empty";
    }
    ntests++;
  }
  //////////////////// rope_algorithms.c:1074 DONE
  //////////////////// rope_algorithms.c:2361 START
  if (!filter || strstr("possible_to_get_long_left_tail_on_nupd", filter))
  {
    extern void __test__possible_to_get_long_left_tail_on_nupd(void);
    i_log_info("========================= TEST CASE: %s\n", "possible_to_get_long_left_tail_on_nupd");
    int prev = test_ret;
    test_ret = 0;
    __test__possible_to_get_long_left_tail_on_nupd();
    if (!test_ret)
    {
      i_log_passed("%s\n", "possible_to_get_long_left_tail_on_nupd");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "possible_to_get_long_left_tail_on_nupd";
    }
    ntests++;
  }
  //////////////////// rope_algorithms.c:2361 DONE
  //////////////////// rope_algorithms.c:2488 START
  if (!filter || strstr("ns_rebalance_apply_to_pivot_splits_2_layer_tree", filter))
  {
    extern void __test__ns_rebalance_apply_to_pivot_splits_2_layer_tree(void);
    i_log_info("========================= TEST CASE: %s\n", "ns_rebalance_apply_to_pivot_splits_2_layer_tree");
    int prev = test_ret;
    test_ret = 0;
    __test__ns_rebalance_apply_to_pivot_splits_2_layer_tree();
    if (!test_ret)
    {
      i_log_passed("%s\n", "ns_rebalance_apply_to_pivot_splits_2_layer_tree");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "ns_rebalance_apply_to_pivot_splits_2_layer_tree";
    }
    ntests++;
  }
  //////////////////// rope_algorithms.c:2488 DONE
  //////////////////// node_updates.c:304 START
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
  //////////////////// node_updates.c:304 DONE
  //////////////////// node_updates.c:377 START
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
  //////////////////// node_updates.c:377 DONE
  //////////////////// node_updates.c:493 START
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
  //////////////////// node_updates.c:493 DONE
  //////////////////// node_updates.c:743 START
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
  //////////////////// node_updates.c:743 DONE
  //////////////////// node_updates.c:943 START
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
  //////////////////// node_updates.c:943 DONE
  //////////////////// node_updates.c:1288 START
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
  //////////////////// node_updates.c:1288 DONE
  //////////////////// node_updates.c:1362 START
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
  //////////////////// node_updates.c:1362 DONE
  //////////////////// node_updates.c:1432 START
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
  //////////////////// node_updates.c:1432 DONE
  //////////////////// node_updates.c:1487 START
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
  //////////////////// node_updates.c:1487 DONE
  //////////////////// node_updates.c:1530 START
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
  //////////////////// node_updates.c:1530 DONE
  //////////////////// node_updates.c:1574 START
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
  //////////////////// node_updates.c:1574 DONE
  //////////////////// node_updates.c:1618 START
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
  //////////////////// node_updates.c:1618 DONE
  //////////////////// node_updates.c:1672 START
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
  //////////////////// node_updates.c:1672 DONE
  //////////////////// logging.c:48 START
  if (!filter || strstr("i_log", filter))
  {
    extern void __test__i_log(void);
    i_log_info("========================= TEST CASE: %s\n", "i_log");
    int prev = test_ret;
    test_ret = 0;
    __test__i_log();
    if (!test_ret)
    {
      i_log_passed("%s\n", "i_log");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_log";
    }
    ntests++;
  }
  //////////////////// logging.c:48 DONE
  //////////////////// logging.c:57 START
  if (!filter || strstr("i_printf", filter))
  {
    extern void __test__i_printf(void);
    i_log_info("========================= TEST CASE: %s\n", "i_printf");
    int prev = test_ret;
    test_ret = 0;
    __test__i_printf();
    if (!test_ret)
    {
      i_log_passed("%s\n", "i_printf");
      test_ret = prev;
    }
    else
    {
      failed_names[failed++] = "i_printf";
    }
    ntests++;
  }
  //////////////////// logging.c:57 DONE


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

#else

typedef int make_compiler_happy;

#endif

// clang-format on
