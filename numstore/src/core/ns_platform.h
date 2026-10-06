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

#ifndef PLATFORM_H
#define PLATFORM_H

//   target            PLATFORM_*  set to 1
//   ----------------  ------------------------------------------------
//   Windows           WINDOWS
//   Linux             LINUX
//   Android           ANDROID                 (LINUX explicitly forced 0)
//   macOS             MAC, APPLE
//   iOS               IOS, APPLE
//   FreeBSD/NetBSD/
//   OpenBSD/DragonFly/
//   BSDi               BSD
//   other Unix         (UNIX/POSIX only - no more specific macro set)
//   Emscripten/Wasm    EMSCRIPTEN              (POSIX set, UNIX is NOT)
//
// Derived/composite macros:
//   PLATFORM_APPLE  = (MAC || IOS),
//   PLATFORM_MOBILE = (ANDROID || IOS)
//   PLATFORM_DESKTOP (WINDOWS || LINUX || MAC || BSD).

////////////////////////////////////////////////////////////
// DEV / SYSTEM

#define PLATFORM_WINDOWS    0
#define PLATFORM_LINUX      0
#define PLATFORM_ANDROID    0
#define PLATFORM_MAC        0
#define PLATFORM_IOS        0
#define PLATFORM_BSD        0
#define PLATFORM_EMSCRIPTEN 0
#define PLATFORM_UNIX       0
#define PLATFORM_POSIX      0

////////////////////////////////////////////////////////////
// Emscripten
#ifdef __EMSCRIPTEN__
#  undef PLATFORM_EMSCRIPTEN
#  define PLATFORM_EMSCRIPTEN 1
#  undef PLATFORM_POSIX
#  define PLATFORM_POSIX 1
#endif

////////////////////////////////////////////////////////////
// Windows
#if defined(_WIN32) || defined(_WIN64) || defined(__WINDOWS__)
#  undef PLATFORM_WINDOWS
#  define PLATFORM_WINDOWS 1
#endif

////////////////////////////////////////////////////////////
// Android
#ifdef __ANDROID__
#  undef PLATFORM_ANDROID
#  define PLATFORM_ANDROID 1
#  undef PLATFORM_LINUX
#  define PLATFORM_LINUX 0
#  undef PLATFORM_UNIX
#  define PLATFORM_UNIX 1
#  undef PLATFORM_POSIX
#  define PLATFORM_POSIX 1
#endif

////////////////////////////////////////////////////////////
// Linux
#if defined(__linux__) && !defined(__ANDROID__)
#  undef PLATFORM_LINUX
#  define PLATFORM_LINUX 1
#  undef PLATFORM_UNIX
#  define PLATFORM_UNIX 1
#  undef PLATFORM_POSIX
#  define PLATFORM_POSIX 1
#endif

////////////////////////////////////////////////////////////
// Apple
#if defined(__APPLE__) && defined(__MACH__)
#  include <TargetConditionals.h>

////////////////////////////////////////////////////////////
// IOS
#  if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
#    undef PLATFORM_IOS
#    define PLATFORM_IOS 1

////////////////////////////////////////////////////////////
// MacOS
#  elif TARGET_OS_MAC
#    undef PLATFORM_MAC
#    define PLATFORM_MAC 1
#  endif
#  undef PLATFORM_UNIX
#  undef PLATFORM_POSIX
#  define PLATFORM_UNIX  1
#  define PLATFORM_POSIX 1
#endif

////////////////////////////////////////////////////////////
// BSD
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__) \
    || defined(__bsdi__)
#  undef PLATFORM_BSD
#  undef PLATFORM_UNIX
#  undef PLATFORM_POSIX
#  define PLATFORM_BSD   1
#  define PLATFORM_UNIX  1
#  define PLATFORM_POSIX 1
#endif

////////////////////////////////////////////////////////////
// UNIX
#if !PLATFORM_UNIX && (defined(__unix__) || defined(__unix))
#  undef PLATFORM_UNIX
#  undef PLATFORM_POSIX
#  define PLATFORM_UNIX  1
#  define PLATFORM_POSIX 1
#endif

////////////////////////////////////////////////////////////
// Verify
#if (                                                                                  \
    PLATFORM_WINDOWS + PLATFORM_LINUX + PLATFORM_ANDROID + PLATFORM_MAC + PLATFORM_IOS \
    + PLATFORM_BSD + PLATFORM_EMSCRIPTEN                                               \
) > 1
#  warning "Multiple platforms detected - check your build configuration"
#endif

////////////////////////////////////////////////////////////
// COMPILER
//
// Platform and compiler are independent axes and must not be conflated: mingw
// is GCC targeting Windows, so PLATFORM_WINDOWS is true there even though
// every GCC extension below is available. Gating extensions on PLATFORM_*
// silently degraded them to no-ops under mingw - which is how the Windows
// cross build ended up with a flood of -Wreturn-type and
// -Wimplicit-fallthrough warnings on paths UNREACHABLE() should have proven
// terminal. Anything that is a property of the *compiler* belongs here.

#define COMPILER_GNUC 0
#define COMPILER_MSVC 0

#if defined(__GNUC__) || defined(__clang__)
#  undef COMPILER_GNUC
#  define COMPILER_GNUC 1
#endif

#if defined(_MSC_VER) && !COMPILER_GNUC
#  undef COMPILER_MSVC
#  define COMPILER_MSVC 1
#endif

////////////////////////////////////////////////////////////
// Utils
#define PLATFORM_APPLE   (PLATFORM_MAC || PLATFORM_IOS)
#define PLATFORM_MOBILE  (PLATFORM_ANDROID || PLATFORM_IOS)
#define PLATFORM_DESKTOP (PLATFORM_WINDOWS || PLATFORM_LINUX || PLATFORM_MAC || PLATFORM_BSD)

////////////////////////////////////////////////////////////
// Branch Prediction
#if COMPILER_GNUC
#  define likely(x)   __builtin_expect (!!(x), 1)
#  define unlikely(x) __builtin_expect (!!(x), 0)
#else
#  define likely(x)   (x)
#  define unlikely(x) (x)
#endif

////////////////////////////////////////////////////////////
// UNREACHABLE
#if COMPILER_GNUC
#  define UNREACHABLE_HINT() __builtin_unreachable ()
#elif COMPILER_MSVC
#  define UNREACHABLE_HINT() __assume (0)
#else
#  define UNREACHABLE_HINT()
#endif

////////////////////////////////////////////////////////////
// UNREACHABLE_WARN_PUSH / UNREACHABLE_WARN_POP
//
// Wraps whatever follows UNREACHABLE_HINT() (e.g. a trailing return/abort
// added to satisfy -Wreturn-type on paths the compiler can't otherwise
// prove terminate) so mingw's gcc doesn't flag it under -Wunreachable-code
// -Werror. This branches on compiler identity, not PLATFORM_*, since
// mingw is GCC targeting Windows - PLATFORM_WINDOWS would be true there
// even though the pragma is GCC/Clang-specific syntax MSVC can't parse.
#if defined(__GNUC__) || defined(__clang__)
#  define UNREACHABLE_WARN_PUSH() \
    _Pragma ("GCC diagnostic push") _Pragma ("GCC diagnostic ignored \"-Wunreachable-code\"")
#  define UNREACHABLE_WARN_POP() _Pragma ("GCC diagnostic pop")
#else
#  define UNREACHABLE_WARN_PUSH()
#  define UNREACHABLE_WARN_POP()
#endif

////////////////////////////////////////////////////////////
// NORETURN
#if COMPILER_GNUC
#  define NORETURN __attribute__ ((noreturn))
#elif COMPILER_MSVC
#  define NORETURN __declspec (noreturn)
#else
#  define NORETURN
#endif

////////////////////////////////////////////////////////////
// PRINTF_ATTR
#if COMPILER_GNUC && PLATFORM_WINDOWS
#  define PRINTF_ATTR(fmt_pos, va_pos) __attribute__ ((format (gnu_printf, fmt_pos, va_pos)))
#elif COMPILER_GNUC
#  define PRINTF_ATTR(fmt_pos, va_pos) __attribute__ ((format (printf, fmt_pos, va_pos)))
#else
#  define PRINTF_ATTR(fmt_pos, va_pos)
#endif

////////////////////////////////////////////////////////////
// MAYBE_UNUSED
#if COMPILER_GNUC
#  define MAYBE_UNUSED __attribute__ ((unused))
#else
#  define MAYBE_UNUSED
#endif

////////////////////////////////////////////////////////////
// ANSI_COLORS
#if PLATFORM_WINDOWS
#  define ANSI_COLORS 0
#else
#  define ANSI_COLORS 1
#endif

////////////////////////////////////////////////////////////
// HAS_BUILTIN_OVERFLOW
#if COMPILER_GNUC
#  define HAS_BUILTIN_OVERFLOW 1
#else
#  define HAS_BUILTIN_OVERFLOW 0
#endif

#define HEADER_FUNC static inline MAYBE_UNUSED

HEADER_FUNC const char *
platformstr (void)
{
  if (PLATFORM_WINDOWS) {
    return "Windows";
  }
  if (PLATFORM_LINUX) {
    return "Linux";
  } else if (PLATFORM_ANDROID) {
    return "Android";
  } else if (PLATFORM_MAC) {
    return "macOS";
  } else if (PLATFORM_IOS) {
    return "iOS";
  } else if (PLATFORM_BSD) {
    return "BSD";
  } else if (PLATFORM_EMSCRIPTEN) {
    return "Emscripten/WebAssembly";
  }

  return 0;
}

////////////////////////////////////////////////////////////
// SYSTEM INCLUDES

#include <complex.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if PLATFORM_WINDOWS
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#elif PLATFORM_POSIX
#  include <pthread.h>
#  include <semaphore.h>
#  include <time.h>
#endif

////////////////////////////////////////////////////////////
// NS_PATH_MAX
//
// Longest path a stack path buffer needs to hold. POSIX spells this PATH_MAX
// in <limits.h> (4096 on Linux, 1024 on macOS); MSVC does not define it at
// all and the Win32 narrow file APIs cap out at MAX_PATH, so defer to
// whichever limit the platform states and keep a floor for anything exotic.
#if defined(PATH_MAX)
#  define NS_PATH_MAX PATH_MAX
#elif defined(MAX_PATH)
#  define NS_PATH_MAX MAX_PATH
#else
#  define NS_PATH_MAX 4096
#endif

#endif // PLATFORM_H
