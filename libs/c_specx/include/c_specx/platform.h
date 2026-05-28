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

#ifndef C_SPECX_PLATFORM_H
#define C_SPECX_PLATFORM_H

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
#if defined(__ANDROID__)
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
#  include "TargetConditionals.h"

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
// Utils
#define PLATFORM_APPLE   (PLATFORM_MAC || PLATFORM_IOS)
#define PLATFORM_MOBILE  (PLATFORM_ANDROID || PLATFORM_IOS)
#define PLATFORM_DESKTOP (PLATFORM_WINDOWS || PLATFORM_LINUX || PLATFORM_MAC || PLATFORM_BSD)

////////////////////////////////////////////////////////////
// Branch Prediction
#if PLATFORM_WINDOWS
#  define likely(x)   (x)
#  define unlikely(x) (x)
#elif PLATFORM_POSIX
#  define likely(x)   __builtin_expect (!!(x), 1)
#  define unlikely(x) __builtin_expect (!!(x), 0)
#endif

////////////////////////////////////////////////////////////
// UNREACHABLE
#if PLATFORM_WINDOWS
#  define UNREACHABLE_HINT() __assume (0)
#elif PLATFORM_POSIX
#  define UNREACHABLE_HINT() __builtin_unreachable ()
#endif

////////////////////////////////////////////////////////////
// NORETURN
#if PLATFORM_WINDOWS
#  define NORETURN __declspec (noreturn)
#elif PLATFORM_POSIX
#  define NORETURN __attribute__ ((noreturn))
#endif

////////////////////////////////////////////////////////////
// PRINTF_ATTR
#if PLATFORM_WINDOWS
#  define PRINTF_ATTR(fmt_pos, va_pos)
#else
#  define PRINTF_ATTR(fmt_pos, va_pos) __attribute__ ((format (printf, fmt_pos, va_pos)))
#endif

////////////////////////////////////////////////////////////
// MAYBE_UNUSED
#if PLATFORM_WINDOWS
#  define MAYBE_UNUSED
#else
#  define MAYBE_UNUSED __attribute__ ((unused))
#endif

////////////////////////////////////////////////////////////
// ANSI_COLORS
#if defined(_WIN32)
#  define ANSI_COLORS 0
#else
#  define ANSI_COLORS 1
#endif

////////////////////////////////////////////////////////////
// HAS_BUILTIN_OVERFLOW
#if PLATFORM_WINDOWS
#  define HAS_BUILTIN_OVERFLOW 0
#else
#  define HAS_BUILTIN_OVERFLOW 1
#endif

#define HEADER_FUNC static inline MAYBE_UNUSED

HEADER_FUNC const char *
platformstr (void)
{
  if (PLATFORM_WINDOWS) { return "Windows"; }
  else if (PLATFORM_LINUX) { return "Linux"; }
  else if (PLATFORM_ANDROID) { return "Android"; }
  else if (PLATFORM_MAC) { return "macOS"; }
  else if (PLATFORM_IOS) { return "iOS"; }
  else if (PLATFORM_BSD) { return "BSD"; }
  else if (PLATFORM_EMSCRIPTEN) { return "Emscripten/WebAssembly"; }

  return 0;
}

////////////////////////////////////////////////////////////
// SYSTEM INCLUDES

#include <complex.h>
#include <inttypes.h>
#include <math.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if PLATFORM_WINDOWS
#  define WIN32_LEAN_AND_MEAN
#  include "windows.h"
#elif PLATFORM_POSIX
#  include <pthread.h>
#  include <semaphore.h>
#  include <time.h>
#endif

#endif // C_SPECX_PLATFORM_H
