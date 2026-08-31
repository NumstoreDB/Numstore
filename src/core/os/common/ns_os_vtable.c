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

#include "core/os/ns_os_vtable.h"

#include "core/os/ns_filesystem.h"
#include "core/os/ns_memory.h"
#include "core/os/ns_threading.h"

const struct os_vtable default_os_vtable = {
    .close           = impl_close,
    .fsync           = impl_fsync,
    .file_size       = impl_file_size,
    .read_all        = impl_read_all,
    .pread_all       = impl_pread_all,
    .write_all       = impl_write_all,
    .pwrite_all      = impl_pwrite_all,
    .writev_all      = impl_writev_all,
    .truncate        = impl_truncate,
    .fallocate       = impl_fallocate,
    .seek            = impl_seek,

    .open_rw         = impl_open_rw,
    .open_r          = impl_open_r,
    .open_w          = impl_open_w,
    .remove_quiet    = impl_remove_quiet,
    .unlink          = impl_unlink,
    .file_exists     = impl_file_exists,

    .malloc          = impl_malloc,
    .calloc          = impl_calloc,
    .realloc         = impl_realloc,
    .free            = impl_free,

    .thread_create   = impl_thread_create,
    .thread_join     = impl_thread_join,
    .mutex_create    = impl_mutex_create,
    .mutex_free      = impl_mutex_free,
    .mutex_lock      = impl_mutex_lock,
    .mutex_unlock    = impl_mutex_unlock,
    .cond_create     = impl_cond_create,
    .cond_free       = impl_cond_free,
    .cond_wait       = impl_cond_wait,
    .cond_timed_wait = impl_cond_timed_wait,
    .cond_signal     = impl_cond_signal,
    .cond_broadcast  = impl_cond_broadcast,
};

struct i_mem
default_mem (void)
{
  return (struct i_mem){
      .table = &default_os_vtable,
      .data  = NULL,
  };
}

struct i_file_system
default_filesystem (void)
{
  return (struct i_file_system){
      .table = &default_os_vtable,
      .data  = NULL,
  };
}

struct i_threading
default_threading (void)
{
  return (struct i_threading){
      .table = &default_os_vtable,
      .data  = NULL,
  };
}
