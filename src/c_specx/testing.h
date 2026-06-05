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

#ifndef C_SPECX_TESTING_H
#define C_SPECX_TESTING_H

#include <c_specx/error.h>
#include <c_specx/logging.h>
#include <c_specx/platform.h>
#include <c_specx/stdtypes.h>

#ifndef NTEST

extern int test_ret;

#  define TEST(name)            \
    void __test__##name (void); \
    void __test__##name (void)

////////////////////////////////////////////////////////////
/// Test Marker

enum
{
  test_marks_max = 256,
  test_mark_len  = 128,
};

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

#  define TEST_MARK(label)                               \
    do                                                   \
    {                                                    \
      ASSERT (test_marks_count < test_marks_max);        \
      const char *_src = (label);                        \
      char       *_dst = test_marks[test_marks_count++]; \
      int         _i   = 0;                              \
      while (_i < test_mark_len - 1 && _src[_i])         \
      {                                                  \
        _dst[_i] = _src[_i];                             \
        _i++;                                            \
      }                                                  \
      _dst[_i] = '\0';                                   \
    }                                                    \
    while (0)

#  define test_reset_marks() \
    do                       \
    {                        \
      test_marks_count = 0;  \
    }                        \
    while (0)

////////////////////////////////////////////////////////////
/// Test Wrappers

#  define TEST_CASE(fmt, ...)                                               \
    for (int _tc_once =                                                     \
                 (i_log_info ("------ CASE: " fmt "\n", ##__VA_ARGS__), 1), \
             _tc_prev = test_ret;                                           \
         _tc_once;                                                          \
         _tc_once = 0,                                                      \
             (test_ret == _tc_prev                                          \
                  ? (i_log_passed ("------ : " fmt "\n", ##__VA_ARGS__), 0) \
                  : (i_log_failure ("------ : " fmt "\n", ##__VA_ARGS__), 0)))

////////////////////////////////////////////////////////////
/// Test Runtime Methods

#  define fail_test(fmt, ...)                                     \
    do                                                            \
    {                                                             \
      i_log_failure (FPREFIX_STR fmt, FPREFIX_ARGS, __VA_ARGS__); \
      test_ret = -1;                                              \
      return;                                                     \
    }                                                             \
    while (0)

#  define test_assert_equal(left, right)         \
    do                                           \
    {                                            \
      if ((left) != (right))                     \
      {                                          \
        fail_test ("%s != %s\n", #left, #right); \
      }                                          \
    }                                            \
    while (0)

#  define test_assert_int_equal(left, right) \
    test_assert_type_equal (left, right, i32, PRId32)

#  define test_assert_type_equal(left, right, type, fmt) \
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

#  define test_assert_ptr_equal(left, right) \
    test_assert_equal ((void *)left, (void *)right)

#  define test_assert(expr)                  \
    do                                       \
    {                                        \
      if (!(expr))                           \
      {                                      \
        fail_test ("Expected: %s\n", #expr); \
      }                                      \
    }                                        \
    while (0)

#  define test_fail_if(expr)                   \
    do                                         \
    {                                          \
      if (expr)                                \
      {                                        \
        fail_test ("Unexpected: %s\n", #expr); \
      }                                        \
    }                                          \
    while (0)

#  define test_err_t_check(expr, exp, ename) \
    do                                       \
    {                                        \
      err_t __ret = (err_t)expr;             \
      test_assert_int_equal (__ret, exp);    \
      (ename)->cause_code = SUCCESS;         \
    }                                        \
    while (0)

#  define test_assert_memequal(a, b, size) \
    test_assert_int_equal (memcmp (a, b, size), 0)

#  define test_assert_mark_hit(pattern)                    \
    do                                                     \
    {                                                      \
      const char *_pat = (pattern);                        \
      bool        _hit = false;                            \
      for (int _i = 0; _i < test_marks_count; _i++)        \
      {                                                    \
        if (test_mark_match (_pat, test_marks[_i]))        \
        {                                                  \
          _hit = true;                                     \
          break;                                           \
        }                                                  \
      }                                                    \
      if (!_hit)                                           \
      {                                                    \
        fail_test ("No mark matched pattern: %s\n", _pat); \
      }                                                    \
    }                                                      \
    while (0)

#  define test_assert_mark_not_hit(pattern)             \
    do                                                  \
    {                                                   \
      const char *_pat = (pattern);                     \
      bool        _hit = false;                         \
      for (int _i = 0; _i < test_marks_count; _i++)     \
      {                                                 \
        if (test_mark_match (_pat, test_marks[_i]))     \
        {                                               \
          _hit = true;                                  \
          break;                                        \
        }                                               \
      }                                                 \
      if (_hit)                                         \
      {                                                 \
        fail_test ("Mark matched pattern: %s\n", _pat); \
      }                                                 \
    }                                                   \
    while (0)

#else // NTEST

#  define TEST_SUITE(name, max) ((void)0)
#  define REGISTER(suite, name) ((void)0)
#  define TEST(type, name)                              \
    static inline void test_##name (void) MAYBE_UNUSED; \
    static inline void test_##name (void)
#  define TEST_CASE(fmt, ...)                         if (0)
#  define TEST_MARK(label)                            ((void)0)
#  define test_assert_mark_hit(pattern)               ((void)0)
#  define test_reset_marks()                          ((void)0)
#  define test_assert(expr)                           ((void)0)
#  define test_assert_equal(left, right)              ((void)0)
#  define test_assert_int_equal(left, right)          ((void)0)
#  define test_assert_type_equal(left, right, t, fmt) ((void)0)
#  define test_assert_ptr_equal(left, right)          ((void)0)
#  define test_assert_memequal(a, b, size)            ((void)0)
#  define test_fail_if(expr)                          ((void)0)
#  define test_err_t_wrap(expr, ename)                ((void)(expr))
#  define test_err_t_check(expr, exp, ename)          ((void)(expr))
#  define fail_test(fmt, ...)                         ((void)0)

#endif // NTEST

#endif // C_SPECX_TESTING_H
