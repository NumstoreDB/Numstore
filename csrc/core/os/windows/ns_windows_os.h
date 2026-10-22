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

#ifndef NS_WINDOWS_OS_H
#define NS_WINDOWS_OS_H

#include "core/ns_platform.h"

#if PLATFORM_WINDOWS

#  include "core/ns_error.h"
#  include "core/ns_slab_alloc.h"
#  include "core/ns_stdtypes.h"
#  include "core/os/ns_malloc.h"
#  include "core/os/ns_os.h"

/// File handle slots. Win32 file handles are multiples of 4 and never NULL,
/// so NULL and 1 are free to use as sentinels. Any other value is occupied
/// and IS the HANDLE.
#  define WIN_SLOT_FREE    ((PVOID)0)
#  define WIN_SLOT_PENDING ((PVOID)1)

/// One thread slot. The start routine and argument live here (not on the
/// heap) - the frame outlives the thread because it's only freed on join.
struct windows_thread_frame
{
  HANDLE handle;
  DWORD  id;
  void *(*func) (void *);
  void         *arg;
  volatile LONG occupied; // 0 free, 1 occupied
};

struct windows_timer
{
  LARGE_INTEGER start;
  LARGE_INTEGER frequency;
};

struct windows_os
{
  PVOID volatile handles[MAX_OPEN_FILES];
  struct windows_thread_frame threads[MAX_THREADS];

  volatile LONG               handle_clock;
  volatile LONG               thread_clock;

  struct slab_alloc           mutex_alloc; // CRITICAL_SECTION
  struct slab_alloc           cond_alloc;  // CONDITION_VARIABLE
  struct slab_alloc           timer_alloc; // struct windows_timer

  struct i_mem                mem;
};

struct windows_os *windows_create (struct i_mem mem, error *e);
void windows_free (struct windows_os *w);

// File handles
PVOID volatile *windows_handle_reserve (struct windows_os *os, error *e);
void windows_handle_free (struct windows_os *os, PVOID volatile *slot);
PVOID windows_handle_load (PVOID volatile *slot);
void windows_handle_store (PVOID volatile *slot, PVOID value);

// Threads
struct windows_thread_frame *windows_thread_alloc (struct windows_os *os, error *e);
void windows_thread_free (struct windows_os *os, struct windows_thread_frame *t);

// Slab backed objects
CRITICAL_SECTION *windows_mutex_alloc (struct windows_os *os, error *e);
void windows_mutex_free (struct windows_os *os, CRITICAL_SECTION *m);
CONDITION_VARIABLE *windows_cond_alloc (struct windows_os *os, error *e);
void windows_cond_free (struct windows_os *os, CONDITION_VARIABLE *c);
struct windows_timer *windows_timer_alloc (struct windows_os *os, error *e);
void windows_timer_free (struct windows_os *os, struct windows_timer *t);

#endif // PLATFORM_WINDOWS

#endif // NS_WINDOWS_OS_H
