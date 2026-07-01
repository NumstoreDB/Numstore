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

#include "error.h"

#include "csx_assert.h"

#ifdef TESTING
#  include "testing/testing.h"
#endif

#ifdef ERR_T_FAIL_FAST
#endif

DEFINE_DBG_ASSERT (error, error, e, {
  ASSERT (e);
  if (e->cause_code != SUCCESS)
  {
    ASSERT (e->cmlen > 0);
  }
})

error
error_create (void)
{
  const error ret = {
      .cause_code = SUCCESS,
      .cmlen      = 0,
  };

  DBG_ASSERT (error, &ret);

  return ret;
}

void
error_silence (error *e)
{
  e->disable_log = true;
}

void
error_unsilence (error *e)
{
  e->disable_log = false;
}

err_t
error_causef (error *e, const err_t c, const char *fmt, ...)
{
  if (e == NULL)
  {
    return c;
  }

  ASSERT (fmt);

  va_list ap;
  va_start (ap, fmt);

  char    tmpbuf[256];
  va_list ap_copy;
  va_copy (ap_copy, ap);
  u32 cmlen = vsnprintf (tmpbuf, sizeof (tmpbuf), fmt, ap_copy);
  cmlen     = MIN (cmlen, 255);
  va_end (ap_copy);

  if (e->cause_code != SUCCESS)
  {
    if (!e->disable_log)
    {
      i_log_error ("TRACE: %s\n", tmpbuf);
    }
  }

  if (e->cause_code == SUCCESS)
  {
    memcpy (e->cause_msg, tmpbuf, cmlen);
    e->cause_code       = c;
    e->cause_msg[cmlen] = '\0';
    e->cmlen            = cmlen;

    if (!e->disable_log)
    {
      i_log_error ("%.*s\n", e->cmlen, e->cause_msg);
    }
  }

  va_end (ap);

  return c;
}

void
error_log_consume (error *e)
{
  DBG_ASSERT (error, e);
  ASSERT (e->cause_code != SUCCESS);
  i_log_error ("%.*s\n", e->cmlen, e->cause_msg);
  e->cmlen      = 0;
  e->cause_code = SUCCESS;
}

#ifdef TESTING
TEST (error_log_consume)
{
  TEST_CASE ("error consumes")
  {
    error e = error_create ();
    error_causef (&e, ERR_CORRUPT, "Test");
    test_assert (e.cause_code == SUCCESS);
    error_log_consume (&e);
    test_assert (e.cause_code == SUCCESS);
    test_assert (e.cmlen == 0);
  }
}
#endif
