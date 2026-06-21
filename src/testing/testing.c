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

#include "testing/testing.h"

#ifndef NTEST

int  test_ret = 0;
char test_marks[test_marks_max][test_mark_len];
int  test_marks_count = 0;

static const char *g_faults[test_faults_max];
static size_t      g_fault_count = 0;

bool
fault_is_set (const char *name)
{
  for (size_t i = 0; i < g_fault_count; i++)
  {
    if (strcmp (g_faults[i], name) == 0)
    {
      return true;
    }
  }
  return false;
}

void
fault_set (const char *name)
{
  if (fault_is_set (name))
  {
    return;
  }
  ASSERT (g_fault_count < test_faults_max);

  g_faults[g_fault_count++] = name;
}

void
fault_reset_all (void)
{
  g_fault_count = 0;
}

static int
example_func (void)
{
  if (1)
  {
    TEST_MARK ("imhit:here");
    TEST_MARK ("imhit:there");
  }
  else
  {
    TEST_MARK ("imnothit:here");
  }
  return 0;
}

TEST (test_mark_works)
{
  test_reset_marks ();
  example_func ();
  test_assert_mark_hit ("imhit:*");
  test_assert_mark_hit ("imhit:here");
  test_assert_mark_hit ("imhit:there");
  test_assert_mark_not_hit ("imnothit:*");
  test_assert_mark_not_hit ("imnothit:here");
}

TEST (test_mark_match)
{
  TEST_CASE ("basics")
  {
    // exact match
    test_assert (test_mark_match ("foo", "foo"));
    test_fail_if (test_mark_match ("foo", "fo"));
    test_fail_if (test_mark_match ("foo", "fooo"));
    test_fail_if (test_mark_match ("foo", "bar"));

    // empty
    test_assert (test_mark_match ("", ""));
    test_fail_if (test_mark_match ("", "x"));
    test_fail_if (test_mark_match ("x", ""));
  }

  TEST_CASE ("wildcard")
  {
    // bare '*' matches anything including empty
    test_assert (test_mark_match ("*", ""));
    test_assert (test_mark_match ("*", "anything"));
    test_assert (test_mark_match ("*", "foo:bar:biz"));

    // trailing '*' — prefix match
    test_assert (test_mark_match ("foo:*", "foo:"));
    test_assert (test_mark_match ("foo:*", "foo:bar"));
    test_assert (test_mark_match ("foo:bar:*", "foo:bar:biz"));
    test_fail_if (test_mark_match ("foo:*", "fo"));
    test_fail_if (test_mark_match ("foo:*", "bar:foo"));

    // leading '*' — suffix match
    test_assert (test_mark_match ("*:biz", "foo:bar:biz"));
    test_assert (test_mark_match ("*biz", "biz"));
    test_fail_if (test_mark_match ("*:biz", "biz"));

    // middle '*'
    test_assert (test_mark_match ("foo:*:biz", "foo:bar:biz"));
    test_assert (test_mark_match ("foo:*:biz", "foo::biz"));
    test_assert (test_mark_match ("foo:*:biz", "foo:a:b:c:biz"));
    test_fail_if (test_mark_match ("foo:*:biz", "foo:bar:baz"));

    // multiple '*'
    test_assert (test_mark_match ("*:*:*", "a:b:c"));
    test_assert (test_mark_match ("**", "anything"));
    test_assert (test_mark_match ("a*b*c", "axxbyyc"));
    test_fail_if (test_mark_match ("a*b*c", "axxbyy"));
  }

  TEST_CASE ("via_assert")
  {
    test_reset_marks ();
    TEST_MARK ("parser:enter");
    TEST_MARK ("parser:utf8:decoded");
    TEST_MARK ("parser:exit");

    test_assert_mark_hit ("parser:enter");
    test_assert_mark_hit ("parser:*");
    test_assert_mark_hit ("*:utf8:*");
    test_assert_mark_hit ("*:exit");
  }
}
#else
typedef int make_compiler_happy_empty_translation;
#endif
