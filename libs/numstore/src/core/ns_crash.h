#pragma once
#include "core/ns_error.h"

#include <stdlib.h>

#ifndef NDEBUG
/**
 * Only use longjmp in debug builds.
 * Release builds just crash normally
 */
#  include <setjmp.h>

struct crash_capture
{
  jmp_buf               env;
  err_t                *ret;
  error                *e;
  struct crash_capture *prev;
};

void crash_capture_begin (struct crash_capture *cc, err_t *ret, error *e);
void crash_capture_end (struct crash_capture *cc);
_Noreturn void intentionally_crash_impl (const char *file, int line, const char *func);

/**
 * err_t ret = SUCCESS;
 * struct error e = error_create ();
 * capture_intentional_crash (&ret, &e)
 * {
 *   code_that_panics ();
 * }
 * test_assert_int_equal (ret, ERR_CRASH);
 */
#  define capture_intentional_crash(ret_ptr, e_ptr)                                                \
    for (struct crash_capture cc_, *run_ = (crash_capture_begin (&cc_, (ret_ptr), (e_ptr)), &cc_); \
         run_;                                                                                     \
         run_ = (crash_capture_end (&cc_), NULL))                                                  \
      if (setjmp (cc_.env) == 0)

#  define intentionally_crash() \
    intentionally_crash_impl (file_basename (__FILE__), __LINE__, __func__)

#else
#  define intentionally_crash() abort ()
#endif
