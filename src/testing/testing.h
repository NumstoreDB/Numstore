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

#ifndef TESTING_H
#define TESTING_H

#include "csx_assert.h" // ASSERT
#include "logging.h"    // i_log_info
#include "platform.h"   // HEADER_FUNC
#include "utils.h"      // FPREFIX_STR

extern int test_ret;

// Limits
enum
{
  test_faults_max = 256,
  test_marks_max  = 256,
  test_mark_len   = 128,
};

/******************************************************************************
 * SECTION: TEST Macro
 * ----------------------------------------------------------------------------
 * @brief Macro for defining new tests
 ******************************************************************************/

#define TEST(name)            \
  void __test__##name (void); \
  void __test__##name (void)

#define TEST_CASE(fmt, ...)                                               \
  for (int _tc_once = (i_log_test_case (fmt "\n", ##__VA_ARGS__), 1),     \
           _tc_prev = test_ret;                                           \
       _tc_once;                                                          \
       _tc_once = 0,                                                      \
           (test_ret == _tc_prev                                          \
                ? (i_log_passed ("------ : " fmt "\n", ##__VA_ARGS__), 0) \
                : (i_log_failure ("------ : " fmt "\n", ##__VA_ARGS__), 0)))

/******************************************************************************
 * SECTION: Fault Injection
 * ----------------------------------------------------------------------------
 * @brief injecting faults
 ******************************************************************************/

bool fault_is_set (const char *name);
void fault_set (const char *name);
void fault_reset_all (void);

#ifndef FAULT
#  define FAULT(expr, name)                                                \
    (fault_is_set (name)                                                   \
         ? (error_causef ((e), ERR_INVALID_ARGUMENT, "Fault: %s", (name))) \
         : (expr))

#  define FAULT_NULL(expr, name)                                           \
    (fault_is_set (name)                                                   \
         ? (error_causef ((e), ERR_INVALID_ARGUMENT, "FAULT: %s", (name)), \
            (void *)0)                                                     \
         : (expr))
#endif

/******************************************************************************
 * SECTION: TEST Marker
 * ----------------------------------------------------------------------------
 * @brief A way to check if a line of code was run
 ******************************************************************************/

extern char test_marks[test_marks_max][test_mark_len];
extern int  test_marks_count;

// Match a test marker string
HEADER_FUNC bool
test_mark_match (const char *pat, const char *str)
{
  while (*pat)
  {
    if (*pat == '*')
    {
      pat++;
      if (!*pat)
      {
        return true;
      }

      while (*str)
      {
        if (test_mark_match (pat, str))
        {
          return true;
        }
        str++;
      }
      return false;
    }
    if (*str != *pat)
    {
      return false;
    }
    pat++;
    str++;
  }
  return *str == '\0';
}

HEADER_FUNC void
__test_mark (const char *_src)
{
  ASSERT (test_marks_count < test_marks_max);
  char *_dst = test_marks[test_marks_count++];
  int   _i   = 0;

  while (_i < test_mark_len - 1 && _src[_i])
  {
    _dst[_i] = _src[_i];
    _i++;
  }
  _dst[_i] = '\0';
}

#define TEST_MARK(label)          \
  do                              \
  {                               \
    if (!__test_mark_hit (label)) \
    {                             \
      __test_mark (label);        \
    }                             \
  }                               \
  while (0)

#define test_reset_marks() \
  do                       \
  {                        \
    test_marks_count = 0;  \
  }                        \
  while (0)

/******************************************************************************
 * SECTION: TEST runtime checks
 * ----------------------------------------------------------------------------
 * @brief methods to use during tests
 ******************************************************************************/

#define fail_test(fmt, ...)                                     \
  do                                                            \
  {                                                             \
    i_log_failure (FPREFIX_STR fmt, FPREFIX_ARGS, __VA_ARGS__); \
    test_ret = -1;                                              \
    return;                                                     \
  }                                                             \
  while (0)

#define test_assert_equal(left, right)         \
  do                                           \
  {                                            \
    if ((left) != (right))                     \
    {                                          \
      fail_test ("%s != %s\n", #left, #right); \
    }                                          \
  }                                            \
  while (0)

#define test_assert_int_equal(left, right) \
  test_assert_type_equal (left, right, i32, PRId32)

#define test_assert_type_equal(left, right, type, fmt) \
  do                                                   \
  {                                                    \
    type _left  = left;                                \
    type _right = right;                               \
    if ((_left) != (_right))                           \
    {                                                  \
      fail_test (                                      \
          "Expression: %s != %s values: "              \
          "(%" fmt ") != (%" fmt ")\n",                \
          #left,                                       \
          #right,                                      \
          _left,                                       \
          _right                                       \
      );                                               \
    }                                                  \
  }                                                    \
  while (0)

#define test_assert_ptr_equal(left, right) \
  test_assert_equal ((void *)left, (void *)right)

#define test_assert(expr)                  \
  do                                       \
  {                                        \
    if (!(expr))                           \
    {                                      \
      fail_test ("Expected: %s\n", #expr); \
    }                                      \
  }                                        \
  while (0)

#define test_fail_if(expr)                   \
  do                                         \
  {                                          \
    if (expr)                                \
    {                                        \
      fail_test ("Unexpected: %s\n", #expr); \
    }                                        \
  }                                          \
  while (0)

#define test_err_t_check(expr, exp, ename) \
  do                                       \
  {                                        \
    err_t __ret = (err_t)expr;             \
    test_assert_int_equal (__ret, exp);    \
    (ename)->cause_code = SUCCESS;         \
  }                                        \
  while (0)

#define test_assert_memequal(a, b, size) \
  test_assert_int_equal (memcmp (a, b, size), 0)

HEADER_FUNC bool
__test_mark_hit (const char *_pat)
{
  bool _hit = false;
  for (int _i = 0; _i < test_marks_count; _i++)
  {
    if (test_mark_match (_pat, test_marks[_i]))
    {
      _hit = true;
      break;
    }
  }
  return _hit;
}

#define test_assert_mark_hit(pattern)                       \
  do                                                        \
  {                                                         \
    if (!__test_mark_hit (pattern))                         \
    {                                                       \
      fail_test ("No mark matched pattern: %s\n", pattern); \
    }                                                       \
  }                                                         \
  while (0)

#define test_assert_mark_not_hit(pattern)                \
  do                                                     \
  {                                                      \
    if (__test_mark_hit (pattern))                       \
    {                                                    \
      fail_test ("Mark matched pattern: %s\n", pattern); \
    }                                                    \
  }                                                      \
  while (0)

#endif // TESTING_H
