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
#include "core/ns_stream.h"
#include "nscore/algorithms/numstore/ns_numstore_algorithms.h"
#include "nscore/nsdb/ns_nsdb.h"
#include "nscore/nsdb/ns_nsdb_execute.h"
#include "nscore/types/ns_query.h"
#include "nscore/types/ns_types.h"
#include "nscore/variables/ns_variables.h"

static inline sb_size
numstore_execute_qt_read_on_buffer (
    struct nsdb        *ns,
    struct ns_txn      *txn,
    struct query       *q,
    struct variable    *var,
    void               *data,
    b_size              dlen,
    struct arena_alloc *alc,
    error              *e
)
{
  // Destination pointer is required
  if (data == NULL) {
    error_causef (e, ERR_INVALID_ARGUMENT, "data is required for a read operation");
    return error_trace (e);
  }

  struct stream          stream;
  struct stream_obuf_ctx octx;
  stream_obuf_init (&stream, &octx, data, dlen);

  // Execute read
  sb_size ret = numstore_read (ns->p, txn, q->read.name, q->read.ustr, alc, var, &stream, e);

  if (ret < 0) {
    return error_trace (e);
  }

  return ret;
}

sb_size
numstore_execute_qt_write_on_buffer (
    struct nsdb        *ns,
    struct ns_txn      *txn,
    struct query       *q,
    struct variable    *var,
    void               *data,
    b_size              dlen,
    struct arena_alloc *alc,
    error              *e
)
{
  // Source pointer is required
  if (data == NULL) {
    error_causef (e, ERR_INVALID_ARGUMENT, "data is required for a write operation");
    return error_trace (e);
  }

  struct stream          stream;
  struct stream_ibuf_ctx ictx;
  stream_ibuf_init (&stream, &ictx, data, dlen);

  sb_size ret = numstore_write (ns->p, txn, q->write.name, q->write.ustr, alc, var, &stream, e);

  if (ret < 0) {
    return error_trace (e);
  }

  return ret;
}

sb_size
numstore_execute_qt_remove_on_buffer (
    struct nsdb        *ns,
    struct ns_txn      *txn,
    struct query       *q,
    struct variable    *var,
    void               *data,
    b_size              dlen,
    struct arena_alloc *alc,
    error              *e
)
{
  struct stream          _stream;
  struct stream_obuf_ctx octx;
  struct stream         *stream;

  if (data) {
    stream_obuf_init (&_stream, &octx, data, dlen);
    stream = &_stream;
  } else {
    stream = NULL;
  }

  sb_size ret = numstore_remove (ns->p, txn, q->remove.name, q->remove.ustr, alc, var, stream, e);

  if (ret < 0) {
    return error_trace (e);
  }

  return ret;
}

static inline sb_size
numstore_execute_qt_insert_on_buffer (
    struct nsdb        *ns,
    struct ns_txn      *txn,
    struct query       *q,
    struct variable    *var,
    void               *data,
    b_size              dlen,
    struct arena_alloc *alc,
    error              *e
)
{
  // Source pointer is required
  if (data == NULL) {
    error_causef (e, ERR_INVALID_ARGUMENT, "data is required for a insert operation");
    return error_trace (e);
  }

  struct stream          stream;
  struct stream_ibuf_ctx ictx;
  stream_ibuf_init (&stream, &ictx, data, dlen);

  sb_size ret = numstore_insert (
      ns->p,
      txn,
      q->insert.name,
      q->insert.len,
      q->insert.ofst,
      alc,
      var,
      &stream,
      e
  );

  if (ret < 0) {
    return error_trace (e);
  }

  return ret;
}

sb_size
nsdb_execute_on_buffer (
    struct nsdb        *ns,
    struct ns_txn      *txn,
    struct query       *q,
    struct variable    *var,
    void               *data,
    b_size              dlen,
    struct arena_alloc *alc,
    error              *e
)
{
  switch (q->type) {
    case QT_READ: {
      return numstore_execute_qt_read_on_buffer (ns, txn, q, var, data, dlen, alc, e);
    }
    case QT_WRITE: {
      return numstore_execute_qt_write_on_buffer (ns, txn, q, var, data, dlen, alc, e);
    }
    case QT_REMOVE: {
      return numstore_execute_qt_remove_on_buffer (ns, txn, q, var, data, dlen, alc, e);
    }
    case QT_INSERT: {
      return numstore_execute_qt_insert_on_buffer (ns, txn, q, var, data, dlen, alc, e);
    }
    case QT_CREATE: {
      return numstore_create (ns->p, txn, q->create.name, q->create.type, alc, var, e);
    }
    case QT_DELETE: {
      return numstore_delete (ns->p, txn, q->delete.name, q->delete.if_exists, e);
    }
    case QT_GET: {
      return numstore_get (ns->p, txn, q->get.if_exists, q->get.name, alc, var, e);
    }
    case QT_EXIT: {
      return SUCCESS;
    }
    case QT_HELP: {
      return SUCCESS;
    }
  }

  return SUCCESS;
}
