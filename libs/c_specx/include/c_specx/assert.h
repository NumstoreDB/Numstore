#ifndef C_SPECX_ASSERT_H
#define C_SPECX_ASSERT_H

#include <c_specx/logging.h>

#define crash()             \
  do                        \
  {                         \
    *(volatile int *)0 = 1; \
    UNREACHABLE_HINT ();    \
  }                         \
  while (0)

#define UNIMPLEMENTED() UNREACHABLE ()

#define UNREACHABLE() \
  do                  \
  {                   \
    crash ();         \
  }                   \
  while (0)

#ifndef NDEBUG

////////////////////////////////////////////////////////////
/// PANIC

#  define panic(msg)                    \
    do                                  \
    {                                   \
      i_log_error ("PANIC! %s\n", msg); \
      i_log_flush ();                   \
      crash ();                         \
    }                                   \
    while (0)

////////////////////////////////////////////////////////////
/// ASSERT

#  define ASSERT(expr)                   \
    do                                   \
    {                                    \
      if (!(expr))                       \
      {                                  \
        i_log_assert (                   \
            "%s failed at %s:%d (%s)\n", \
            #expr,                       \
            __FILE__,                    \
            __LINE__,                    \
            __func__                     \
        );                               \
        i_log_flush ();                  \
        crash ();                        \
      }                                  \
    }                                    \
    while (0)

HEADER_FUNC void *
non_null (void *ptr)
{
  ASSERT (ptr != NULL);
  return ptr;
}

HEADER_FUNC int
gte0 (int val)
{
  ASSERT (val >= 0);
  return val;
}

#  define ASSERT_NN(ptr) non_null (ptr)
#  define ASSERT_GTE0(v) gte0 (v)

#  define DEFINE_DBG_ASSERT(type, name, var, body) \
    HEADER_FUNC void name##_assert__ (const type *var) body

#  define DBG_ASSERT(name, expr) name##_assert__ (expr)

#else

// Throw a compile time error to discourage various debug macros
#  define NOT_FOR_PRODUCTION() // typedef char
                               // PANIC_in_release_mode_is_not_allowed[-1]

// Release doesn't allow these two
#  define panic(msg) NOT_FOR_PRODUCTION ()
#  define ASSERT(expr)

#  define DEFINE_DBG_ASSERT(type, name, var, body)   \
    HEADER_FUNC void name##_assert (const type *var) \
    {                                                \
      (void)var;                                     \
    }

#  define ASSERT_NN(ptr) ptr
#  define ASSERT_GTE0(v) v

#  define DBG_ASSERT(name, expr) ((void)0)
#endif

#endif // C_SPECX_ASSERT_H
