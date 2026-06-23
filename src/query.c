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

#include "query.h"

#include "csx_assert.h"
#include "serial.h"
#include "types.h"

void
i_log_query (int log_level, struct query *q)
{
  switch (q->type)
  {
    case QT_READ:
    {
      i_log (log_level, "READ [%.*s]\n", strfmt (&q->read.name));
      return;
    }
    case QT_WRITE:
    {
      i_log (log_level, "WRITE [%.*s]\n", strfmt (&q->write.name));
      return;
    }
    case QT_REMOVE:
    {
      i_log (log_level, "REMOVE [%.*s]\n", strfmt (&q->remove.name));
      return;
    }
    case QT_INSERT:
    {
      i_log (log_level, "INSERT [%.*s]\n", strfmt (&q->insert.name));
      return;
    }

    case QT_CREATE:
    {
      i_log (log_level, "CREATE [%.*s]\n", strfmt (&q->create.name));
      return;
    }
    case QT_DELETE:
    {
      i_log (log_level, "DELETE [%.*s]\n", strfmt (&q->delete.name));
      return;
    }
    case QT_GET:
    {
      i_log (log_level, "GET [%.*s]\n", strfmt (&q->get.name));
      return;
    }

    case QT_EXIT:
    {
      i_log (log_level, "EXIT\n");
      return;
    }
    case QT_HELP:
    {
      i_log (log_level, "HELP\n");
      return;
    }
  }
}

bool
query_equal (const struct query *left, const struct query *right)
{
  if (left->type != right->type)
  {
    return false;
  }

  switch (left->type)
  {
    case QT_READ:
    {
      return string_equal (left->read.name, right->read.name)
             && user_stride_equal (&left->read.ustr, &right->read.ustr);
    }
    case QT_WRITE:
    {
      return string_equal (left->write.name, right->write.name)
             && user_stride_equal (&left->write.ustr, &right->write.ustr);
    }
    case QT_REMOVE:
    {
      return string_equal (left->remove.name, right->remove.name)
             && user_stride_equal (&left->remove.ustr, &right->remove.ustr);
    }
    case QT_INSERT:
    {
      return string_equal (left->insert.name, right->insert.name)
             && left->insert.ofst == right->insert.ofst
             && left->insert.len == right->insert.len;
    }

    case QT_CREATE:
    {
      return string_equal (left->create.name, right->create.name)
             && type_equal (&left->create.type, &right->create.type);
    }
    case QT_DELETE:
    {
      return string_equal (left->delete.name, right->delete.name);
    }
    case QT_GET:
    {
      return string_equal (left->get.name, right->get.name)
             && left->get.if_exists == right->get.if_exists;
    }

    case QT_EXIT:
    {
      return true;
    }
    case QT_HELP:
    {
      if (left->help.has_command != right->help.has_command)
      {
        return false;
      }
      if (left->help.has_command)
      {
        return left->help.command == right->help.command;
      }
      return true;
    }
  }

  UNREACHABLE ();
}
