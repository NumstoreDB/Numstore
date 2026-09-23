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

#ifndef NS_QUERY_H
#define NS_QUERY_H

#include "core/ns_stdtypes.h"
#include "core/ns_stride.h"
#include "core/ns_string.h"
#include "nscore/types/ns_types.h"

#include <stdbool.h>

struct query
{
  enum query_type
  {
    // Array Operations
    QT_READ,
    QT_WRITE,
    QT_INSERT,
    QT_REMOVE,

    // Variable Operations
    QT_CREATE,
    QT_DELETE,
    QT_GET,

    // Meta Operations
    QT_EXIT,
    QT_HELP,
  } type;

  union {
    // Array Operations
    struct read_query
    {
      struct string      name;
      struct user_stride ustr;
    } read;

    struct write_query
    {
      struct string      name;
      struct user_stride ustr;
    } write;

    struct insert_query
    {
      struct string name;
      sb_size       ofst;
      b_size        len;
    } insert;

    struct remove_query
    {
      struct string      name;
      struct user_stride ustr;
    } remove;

    // Variable Operations
    struct create_query
    {
      struct string name;
      struct type   type;
    } create;

    struct delete_query
    {
      struct string name;
      bool          if_exists;
    } delete;

    struct get_query
    {
      struct string name;
      bool          if_exists;
    } get;

    // Meta Operations
    struct help_query
    {
      bool            has_command;
      enum query_type command;
    } help;
  };
};

bool query_equal (const struct query *left, const struct query *right);

void i_log_query (int LOG_LEVEL, struct query *q);

err_t query_vname_of_interest (struct string *dest, const struct query *q, error *e);
err_t query_ustr_of_interest (struct user_stride *dest, const struct query *q, error *e);

sb_size ns_query_fcompile (
    struct arena_alloc *alloc, // Where to allocate query
    const char         *fmt,   // Format string
    va_list             ap,    // Var args
    struct query       *q,     // destination
    error              *e
);

#endif
