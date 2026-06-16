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

#ifndef AST_H
#define AST_H

#include "collections.h"
#include "types.h"

/******************************************************************************
 * SECTION: Query
 * ----------------------------------------------------------------------------
 *
 * @brief A Literal is a user entered value
 ******************************************************************************/

struct query
{
  enum query_type
  {
    QT_READ,
    QT_CREATE,
    QT_DELETE,
    QT_GET,
    QT_EXIT,
    QT_HELP,
  } type;

  union {
    struct
    {
      struct string      vname;
      struct user_stride ustr;
    } read;

    struct
    {
      struct string vname;
      struct type  *type;
    } create;

    struct
    {
      struct string vname;
    } delete;

    struct
    {
      struct string vname;
    } get;

    struct
    {
      bool            has_command;
      enum query_type command;
    } help;
  };
};

void i_log_query (int log_level, struct query *q);

bool query_equal (const struct query *left, const struct query *right);

#endif
