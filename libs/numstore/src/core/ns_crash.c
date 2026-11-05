#include "core/ns_crash.h"

#include "core/ns_error.h"

#ifndef NDEBUG

static _Thread_local struct crash_capture *top = NULL;

void
crash_capture_begin (struct crash_capture *cc, err_t *ret, error *e)
{
  cc->ret  = ret;
  cc->e    = e;
  cc->prev = top;
  top      = cc; // push
}

void
crash_capture_end (struct crash_capture *cc)
{
  top = cc->prev; // pop
}

_Noreturn void
intentionally_crash_impl (const char *file, int line, const char *func)
{
  if (top == NULL) {
    abort (); // no capture installed: crash for real
  }

  struct crash_capture *cc = top;

  *cc->ret                 = error_causef (
      cc->e,
      ERR_CRASHED,
      "Crashed the program at " FPREFIX_STR,
      file,
      line,
      func
  );

  longjmp (cc->env, 1);
}

#endif
