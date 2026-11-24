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

#include "types/ns_query.h"

#include "ns_compiler.h"
#include "ns_csx_assert.h"
#include "ns_error.h"
#include "ns_string.h"
#include "types/ns_types.h"

#ifndef NDEBUG
#  include "ns_testing.h" // TEST
#endif

bool
query_equal (const struct query *left, const struct query *right)
{
  if (left->type != right->type) {
    return false;
  }

  switch (left->type) {
    case QT_READ: {
      return (string_equal (left->read.name, right->read.name)
              && user_stride_equal (&left->read.ustr, &right->read.ustr))
             != 0;
    }
    case QT_WRITE: {
      return (string_equal (left->write.name, right->write.name)
              && user_stride_equal (&left->write.ustr, &right->write.ustr))
             != 0;
    }
    case QT_REMOVE: {
      return (string_equal (left->remove.name, right->remove.name)
              && user_stride_equal (&left->remove.ustr, &right->remove.ustr))
             != 0;
    }
    case QT_INSERT: {
      return (string_equal (left->insert.name, right->insert.name)
              && left->insert.ofst == right->insert.ofst && left->insert.len == right->insert.len)
             != 0;
    }

    case QT_CREATE: {
      return (string_equal (left->create.name, right->create.name)
              && type_equal (&left->create.type, &right->create.type))
             != 0;
    }
    case QT_DELETE: {
      return string_equal (left->delete.name, right->delete.name);
    }
    case QT_GET: {
      return (string_equal (left->get.name, right->get.name)
              && left->get.if_exists == right->get.if_exists)
             != 0;
    }

    case QT_EXIT: {
      return true;
    }
    case QT_HELP: {
      if (left->help.has_command != right->help.has_command) {
        return false;
      }
      if (left->help.has_command) {
        return left->help.command == right->help.command;
      }
      return true;
    }
  }

  UNREACHABLE (); // LCOV_EXCL_LINE
}

err_t
query_to_get (struct query *dest, const struct query *q, error *e)
{
  ASSERT (dest);

  struct string name;
  bool          if_exists = false;

  switch (q->type) {
    // These expect the variable - a miss is an error
    case QT_READ: {
      name = q->read.name;
      break;
    }
    case QT_WRITE: {
      name = q->write.name;
      break;
    }
    case QT_INSERT: {
      name = q->insert.name;
      break;
    }
    case QT_REMOVE: {
      name = q->remove.name;
      break;
    }
    case QT_CREATE: {
      name = q->create.name;
      break;
    }

    // These carry their own flag
    case QT_DELETE: {
      name      = q->delete.name;
      if_exists = q->delete.if_exists;
      break;
    }
    case QT_GET: {
      name      = q->get.name;
      if_exists = q->get.if_exists;
      break;
    }

    default: {
      return error_causef (e, ERR_INVALID_ARGUMENT, "Unsupported query type - no variable exists");
    }
  }

  *dest = (struct query){
      .type = QT_GET,
      .get  = {.name = name, .if_exists = if_exists},
  };

  return SUCCESS;
}

sb_size
ns_query_fcompile (
    struct arena_alloc *alloc, // Where to allocate query
    const char         *fmt,   // Format string
    va_list             ap,    // Var args
    struct query       *q,     // destination
    error              *e
)
{
  va_list ap2;
  va_copy (ap2, ap);

  // Compute the length the formatted query needs, without writing anything.
  i32 qlen = vsnprintf (NULL, 0, fmt, ap);
  if (qlen < 0) {
    va_end (ap2);
    return error_causef (e, ERR_INVALID_ARGUMENT, "Invalid printf argument");
  }

  char *buf = arena_malloc (alloc, (u32)qlen + 1, 1, e);
  if (buf == NULL) {
    va_end (ap2);
    return error_trace (e);
  }

  // Actually write the formatted query into buf.
  qlen = vsnprintf (buf, (size_t)qlen + 1, fmt, ap2);
  ASSERT (qlen >= 0);
  va_end (ap2);

  if (compile_and_validate_query (q, buf, alloc, e) < 0) {
    return error_trace (e);
  }

  return SUCCESS;
}

#ifndef NDEBUG

/******************************************************************************
 * SECTION: Tests
 ******************************************************************************/

/// ns_query_fcompile takes an already-started va_list, so a test needs a
/// variadic shim to start one.
static sb_size
test_fcompile (struct arena_alloc *alloc, struct query *q, error *e, const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  const sb_size ret = ns_query_fcompile (alloc, fmt, ap, q, e);
  va_end (ap);
  return ret;
}

/// Queries are built by compiling real text rather than by hand, so every
/// state under test is one the parser can actually produce.
static struct query
test_compile (struct arena_alloc *alloc, const char *text, error *e)
{
  struct query q;
  const err_t  ret = compile_query (&q, text, alloc, e);
  ASSERT (ret == SUCCESS);
  return q;
}

/// query_equal checks the parser against itself, so it has to be a real
/// equivalence: reflexive, and false the moment any field the type carries
/// differs. Comparing the payload without first comparing the tag would make
/// two queries equal whenever their union bytes happen to line up.
TEST (query_equal)
{
  ALLOC_INIT (alloc);
  error       e     = error_create ();

  // One query per type, so every switch arm is exercised against itself.
  const char *all[] = {
      "read foo[0:10:20]",
      "write foo[0:10:20]",
      "insert foo 0 1",
      "remove foo[0:10:20]",
      "create foo i32",
      "delete foo",
      "get if exists foo",
      "exit",
      "help read",
  };
  const u32 n = (u32)(sizeof (all) / sizeof (*all));

  TEST_CASE ("equal to itself, unequal to every other type")
  {
    for (u32 i = 0; i < n; ++i) {
      struct query a = test_compile (&alloc, all[i], &e);
      struct query b = test_compile (&alloc, all[i], &e);
      test_assert (query_equal (&a, &b));

      for (u32 j = 0; j < n; ++j) {
        if (i == j) {
          continue;
        }
        struct query other = test_compile (&alloc, all[j], &e);
        test_fail_if (query_equal (&a, &other));
      }
    }
  }

  // Each row is a pair that shares a type but differs in exactly one field.
#  define QUERY_DIFFERS(left, right)                       \
    do {                                                   \
      TEST_CASE ("'%s' != '%s'", left, right)              \
      {                                                    \
        struct query a = test_compile (&alloc, left, &e);  \
        struct query b = test_compile (&alloc, right, &e); \
        test_fail_if (query_equal (&a, &b));               \
        test_fail_if (query_equal (&b, &a));               \
      }                                                    \
    }                                                      \
    while (0)

  QUERY_DIFFERS ("read foo", "read bar");                   // name
  QUERY_DIFFERS ("read foo[0:10:20]", "read foo[0:10:21]"); // stride
  QUERY_DIFFERS ("write foo", "write bar");
  QUERY_DIFFERS ("write foo[0:10:20]", "write foo[1:10:20]");
  QUERY_DIFFERS ("remove foo", "remove bar");
  QUERY_DIFFERS ("remove foo[0:10:20]", "remove foo[0:11:20]");
  QUERY_DIFFERS ("insert foo 0 1", "insert bar 0 1");
  QUERY_DIFFERS ("insert foo 0 1", "insert foo 1 1"); // ofst
  QUERY_DIFFERS ("insert foo 0 1", "insert foo 0 2"); // len
  QUERY_DIFFERS ("create foo i32", "create bar i32");
  QUERY_DIFFERS ("create foo i32", "create foo f64"); // type
  QUERY_DIFFERS ("delete foo", "delete bar");
  QUERY_DIFFERS ("get foo", "get bar");
  QUERY_DIFFERS ("get foo", "get if exists foo"); // if_exists
  QUERY_DIFFERS ("help", "help read");            // has_command
  QUERY_DIFFERS ("help read", "help write");      // command

#  undef QUERY_DIFFERS

  TEST_CASE ("exit carries nothing, so all exits are equal")
  {
    struct query a = test_compile (&alloc, "exit", &e);
    struct query b = test_compile (&alloc, "exit", &e);
    test_assert (query_equal (&a, &b));
  }

  ALLOC_CLOSE (alloc);
}

/// The variable a query names is what the lock table keys on, so picking the
/// wrong union arm would lock the wrong variable. Meta queries name no
/// variable at all and must say so rather than returning a stale string.
TEST (query_to_get)
{
  ALLOC_INIT (alloc);
  error e = error_create ();

#  define TO_GET_CASE(text, expected)                                  \
    do {                                                               \
      TEST_CASE ("'%s' -> '%s'", text, expected)                       \
      {                                                                \
        struct query q    = test_compile (&alloc, text, &e);           \
        struct query want = test_compile (&alloc, expected, &e);       \
        struct query dest;                                             \
        test_assert_int_equal (query_to_get (&dest, &q, &e), SUCCESS); \
        test_assert (query_equal (&dest, &want));                      \
      }                                                                \
    }                                                                  \
    while (0)

  // These expect the variable, so the get they turn into does too
  TO_GET_CASE ("read a[0:10:20]", "get a");
  TO_GET_CASE ("write b[0:10:20]", "get b");
  TO_GET_CASE ("insert c 0 1", "get c");
  TO_GET_CASE ("remove d[0:10:20]", "get d");
  TO_GET_CASE ("create e i32", "get e");
  TO_GET_CASE ("delete f", "get f");
  TO_GET_CASE ("get g", "get g");

  // The flag carries over from the two queries that have one
  TO_GET_CASE ("delete if exists h", "get if exists h");
  TO_GET_CASE ("get if exists i", "get if exists i");

#  undef TO_GET_CASE

  // Each row is a query with no variable: the call has to fail and leave dest
  // alone, so a caller cannot act on an uninitialized query.
#  define NO_GET_CASE(text)                                                         \
    do {                                                                            \
      TEST_CASE ("'%s' has no variable", text)                                      \
      {                                                                             \
        struct query q    = test_compile (&alloc, text, &e);                        \
        struct query dest = q;                                                      \
        test_assert_int_equal (query_to_get (&dest, &q, &e), ERR_INVALID_ARGUMENT); \
        test_assert (query_equal (&dest, &q));                                      \
        error_reset (&e);                                                           \
      }                                                                             \
    }                                                                               \
    while (0)

  NO_GET_CASE ("exit");
  NO_GET_CASE ("help");
  NO_GET_CASE ("help read");

#  undef NO_GET_CASE

  ALLOC_CLOSE (alloc);
}

/// fcompile is printf plus compile_query, and it sizes the buffer with a first
/// vsnprintf pass. The second pass reuses the arg list, so a missing va_copy
/// reads garbage on the write pass: the formatted result is checked, not just
/// the return code.
TEST (ns_query_fcompile)
{
  TEST_CASE ("substitutes args and compiles the result")
  {
    ALLOC_INIT (alloc);
    error        e        = error_create ();

    struct query got      = {0};
    struct query expected = test_compile (&alloc, "insert myvar 7 42", &e);

    test_assert_type_equal (
        test_fcompile (&alloc, &got, &e, "insert %s %d %d", "myvar", 7, 42),
        SUCCESS,
        sb_size,
        PRsb_size
    );
    test_assert (query_equal (&got, &expected));

    ALLOC_CLOSE (alloc);
  }

  TEST_CASE ("a long format string is sized correctly, not truncated")
  {
    ALLOC_INIT (alloc);
    error e = error_create ();

    // 200 chars of name: longer than any fixed stack buffer would be.
    char  name[201];
    memset (name, 'x', sizeof (name) - 1);
    name[sizeof (name) - 1] = '\0';

    struct query got        = {0};
    test_assert_type_equal (
        test_fcompile (&alloc, &got, &e, "delete %s", name),
        SUCCESS,
        sb_size,
        PRsb_size
    );
    test_assert_int_equal (got.type, QT_DELETE);
    test_assert (string_equal (got.delete.name, strfcstr (name)));

    ALLOC_CLOSE (alloc);
  }

  TEST_CASE ("a syntax error in the formatted text is reported")
  {
    ALLOC_INIT (alloc);
    error        e   = error_create ();

    struct query got = {0};
    test_assert (test_fcompile (&alloc, &got, &e, "%s nonsense", "definitely") < 0);
    test_assert (e.cause_code < 0);
    error_reset (&e);

    ALLOC_CLOSE (alloc);
  }

  TEST_CASE ("the arena refusing the query buffer is surfaced")
  {
    // The buffer allocation is the first thing fcompile does after sizing, so
    // failing the arena's very first chunk fails it and nothing else.
    struct arena_alloc a;
    arena_alloc_create_default_with (&a, ctx->faulty_mem);

    faulty_mem_reset (ctx->faulty_mem);
    faulty_mem_fail_after (ctx->faulty_mem, 0);

    struct query got = {0};
    test_assert (test_fcompile (&a, &got, &ctx->e, "delete %s", "foo") < 0);
    test_assert (ctx->e.cause_code < 0);
    error_reset (&ctx->e);

    arena_alloc_free_all (&a);
    faulty_mem_reset (ctx->faulty_mem);
  }
}

#endif // NDEBUG
