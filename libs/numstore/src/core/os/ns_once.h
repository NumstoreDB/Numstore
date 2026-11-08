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

#ifndef NS_ONCE_H
#define NS_ONCE_H

#include "core/ns_platform.h"

#if PLATFORM_WINDOWS
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef _WIN32_WINNT
#    define _WIN32_WINNT 0x0601 // Windows 7+: INIT_ONCE, SetFileInformationByHandle
#  endif
#  include <windows.h>
#else
#  include <pthread.h>
#  include <time.h>
#endif

#if PLATFORM_WINDOWS
typedef INIT_ONCE i_once;
#  define I_ONCE_INIT INIT_ONCE_STATIC_INIT
#else
typedef pthread_once_t i_once;
#  define I_ONCE_INIT PTHREAD_ONCE_INIT
#endif

void i_once_run (i_once *once, void (*fn) (void));

#endif
