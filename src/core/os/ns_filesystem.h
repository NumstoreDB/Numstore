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

#ifndef NS_FILESYSTEM_H
#define NS_FILESYSTEM_H

#include "core/os/ns_os_vtable.h"

struct i_file_system
{
  const struct os_vtable *table;
  void                   *data;
};

struct i_file_system default_filesystem (void);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Default Methods
 *----------------------------------------------------------------------------*/

#define i_open_rw(vfs, dest, fname, e)     (vfs).table->open_rw ((vfs).data, dest, fname, e)
#define i_open_r(vfs, dest, fname, e)      (vfs).table->open_r ((vfs).data, dest, fname, e)
#define i_open_w(vfs, dest, fname, e)      (vfs).table->open_w ((vfs).data, dest, fname, e)
#define i_remove_quiet(vfs, fname, e)      (vfs).table->remove_quiet ((vfs).data, fname, e)
#define i_unlink(vfs, name, e)             (vfs).table->unlink ((vfs).data, name, e)
#define i_file_exists(vfs, fname, dest, e) (vfs).table->file_exists ((vfs).data, fname, dest, e)

#endif
