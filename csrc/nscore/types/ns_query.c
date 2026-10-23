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

#include "nscore/types/ns_query.h"

#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_string.h"
#include "nscore/compiler/ns_compiler.h"
#include "nscore/types/ns_types.h"

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
query_vname_of_interest (struct string *dest, const struct query *q, error *e)
{
  ASSERT (dest);

  switch (q->type) {
    case QT_READ: {
      *dest = q->read.name;
      return SUCCESS;
    }
    case QT_WRITE: {
      *dest = q->write.name;
      return SUCCESS;
    }
    case QT_INSERT: {
      *dest = q->insert.name;
      return SUCCESS;
    }
    case QT_REMOVE: {
      *dest = q->remove.name;
      return SUCCESS;
    }

    // Variable Operations
    case QT_CREATE: {
      *dest = q->create.name;
      return SUCCESS;
    }
    case QT_DELETE: {
      *dest = q->delete.name;
      return SUCCESS;
    }
    case QT_GET: {
      *dest = q->get.name;
      return SUCCESS;
    }

    default: {
      return error_causef (e, ERR_INVALID_ARGUMENT, "Unsupported query type - no variable exists");
    }
  }
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

  // Allocate buffer for the query
  char *buf = arena_malloc (alloc, (size_t)qlen + 1, 1, e);
  if (!buf) {
    va_end (ap2);
    return error_trace (e);
  }

  // Actually write the formatted query into buf.
  qlen = vsnprintf (buf, (size_t)qlen + 1, fmt, ap2);
  ASSERT (qlen >= 0);
  va_end (ap2);

  // Compile the query
  if (compile_query (q, buf, alloc, e)) {
    return error_trace (e);
  }

  return SUCCESS;
}
