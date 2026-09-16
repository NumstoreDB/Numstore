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

#include "core/ns_arena_alloc.h"
#include "core/ns_error.h"
#include "nscore/algorithms/numstore/ns_numstore_algorithms.h"
#include "nscore/nsdb/ns_nsdb.h"
#include "nscore/nsdb/ns_nsdb_execute.h"
#include "nscore/types/ns_query.h"
#include "nscore/types/ns_types.h"
#include "numstore/numstore.h"

static inline sb_size
console_qt_read (struct nsdb *ns, struct query *q, struct arena_alloc *alc, error *e)
{
  sb_size ret = nsdb_read_and_print (ns, &q->read, alc, e);
  if (ret < 0) {
    return error_trace (e);
  }

  return ret;
}

static inline sb_size
console_qt_write (void)
{
  return SUCCESS;
}

static inline sb_size
console_qt_remove (void)
{
  return SUCCESS;
}

static inline sb_size
console_qt_insert (void)
{
  return SUCCESS;
}

static inline sb_size
console_qt_create (struct nsdb *ns, struct query *q, struct arena_alloc *alc, error *e)
{
  struct ns_txn *tx = nsdb_begin (ns, e);
  if (tx == NULL) {
    return error_trace (e);
  }

  if (numstore_create (ns->p, tx, q->create.name, q->create.type, alc, NULL, e) < 0) {
    return error_trace (e);
  }

  if (nsdb_commit (ns, tx, e) < 0) {
    return error_trace (e);
  }

  printf ("{ \"Status\" : \"Ok\" }\n");

  return SUCCESS;
}

static inline sb_size
console_qt_delete (void)
{
  return SUCCESS;
}

static inline sb_size
console_qt_get (struct nsdb *ns, struct query *q, struct arena_alloc *alc, error *e)
{
  sb_size ret = nsdb_get_and_print (ns, &q->get, alc, e);
  if (ret < 0) {
    return error_trace (e);
  }

  return ret;
}

static inline sb_size
console_qt_exit (void)
{
  return SUCCESS;
}

static inline sb_size
console_qt_help (void)
{
  return SUCCESS;
}

err_t
nsdb_execute_in_console (struct nsdb *ns, struct query *q, struct arena_alloc *alc, error *e)
{
  switch (q->type) {
    case QT_READ: {
      return console_qt_read (ns, q, alc, e);
    }
    case QT_WRITE: {
      return console_qt_write ();
    }
    case QT_REMOVE: {
      return console_qt_remove ();
    }
    case QT_INSERT: {
      return console_qt_insert ();
    }
    case QT_CREATE: {
      return console_qt_create (ns, q, alc, e);
    }
    case QT_DELETE: {
      return console_qt_delete ();
    }
    case QT_GET: {
      return console_qt_get (ns, q, alc, e);
    }
    case QT_EXIT: {
      return console_qt_exit ();
    }
    case QT_HELP: {
      return console_qt_help ();
    }
  }

  return SUCCESS;
}
