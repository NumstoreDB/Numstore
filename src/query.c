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
      i_log (log_level, "READ [%.*s]\n", strfmt (&q->read.vname));
      return;
    }
    case QT_CREATE:
    {
      i_log (log_level, "CREATE [%.*s]\n", strfmt (&q->create.vname));
      return;
    }
    case QT_DELETE:
    {
      i_log (log_level, "DELETE [%.*s]\n", strfmt (&q->delete.vname));
      return;
    }
    case QT_GET:
    {
      i_log (log_level, "GET [%.*s]\n", strfmt (&q->get.vname));
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
      return string_equal (left->read.vname, right->read.vname)
             && user_stride_equal (&left->read.ustr, &right->read.ustr);
    }
    case QT_CREATE:
    {
      return string_equal (left->create.vname, right->create.vname)
             && type_equal (left->create.type, right->create.type);
    }
    case QT_DELETE:
    {
      return string_equal (left->delete.vname, right->delete.vname);
    }
    case QT_GET:
    {
      return string_equal (left->get.vname, right->get.vname);
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
