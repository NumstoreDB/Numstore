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

#include "numstore.h"

#include "alloc.h"
#include "compiler.h"
#include "csx_assert.h"
#include "error.h"
#include "nsdb.h"
#include "os.h"
#include "query.h"

/******************************************************************************
 * SECTION: Library exposed nsdb_execute
 ******************************************************************************/

sb_size
nsdb_execute (nsdb_t *nh, const char *query, void *data, ...)
{
  struct chunk_alloc alloc;          // Memory allocation context
  sb_size            ret;            // return variable
  char               stackbuf[2048]; // Stack buffer if the query fits
  char              *buf = stackbuf; // Pointer to the buffer
  va_list            ap, ap2;        // Argument list
  i32                qlen;           // Length of the query
  struct query       q;              // The AST

  // Reset errors before proceeding
  nh->e.cause_code = 0;
  nh->e.cmlen      = 0;

  chunk_alloc_create_default (&alloc);

  // A small stack buffer - if the query doesn't fit into
  // this buffer - we'll need to malloc

  va_start (ap, data);
  va_copy (ap2, ap);

  qlen = vsnprintf (stackbuf, sizeof stackbuf, query, ap);
  va_end (ap);

  if (qlen < 0)
  {
    va_end (ap2);
    ret =
        error_causef (&nh->e, ERR_INVALID_ARGUMENT, "Invalid printf argument");
    goto theend;
  }

  if ((size_t)qlen >= sizeof stackbuf)
  {
    buf = chunk_malloc (&alloc, qlen + 1, 1, &nh->e);
    if (!buf)
    {
      va_end (ap2);
      ret = error_trace (&nh->e);
      goto theend;
    }
    qlen = vsnprintf (buf, qlen + 1, query, ap2);
    ASSERT (qlen >= 0);
  }
  va_end (ap2);

  // Compile the query
  if (compile_query (&q, buf, &alloc, &nh->e))
  {
    ret = error_trace (&nh->e);
    goto theend;
  }

  ret = nsdb_execute_on_buffer (nh, &q, data, &alloc);

  if (buf != stackbuf)
  {
    i_free (buf);
  }

theend:
  chunk_alloc_free_all (&alloc);
  return ret;
}
