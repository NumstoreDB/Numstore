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

#include "core/os/test/ns_test_filesystem.h"

/******************************************************************************
 * SECTION: File System
 ******************************************************************************/

#define FAIL_WITH_PROB(data, label, expr)                         \
  do {                                                            \
    if (rand () < data->label) {                                  \
      return error_causef (e, ERR_IO, #expr "Simulated failure"); \
    } else {                                                      \
      return expr;                                                \
    }                                                             \
  }                                                               \
  while (0)

static err_t
test_open_rw (void *vfs, i_file *dest, const char *fname, error *e)
{
  struct test_filesystem_data *data = vfs;
  FAIL_WITH_PROB (data, open_rw_fail_prob, i_open_rw (data->delegate, dest, fname, e));
}

static err_t
test_open_r (void *vfs, i_file *dest, const char *fname, error *e)
{
  struct test_filesystem_data *data = vfs;
  FAIL_WITH_PROB (data, open_r_fail_prob, i_open_r (data->delegate, dest, fname, e));
}

static err_t
test_open_w (void *vfs, i_file *dest, const char *fname, error *e)
{
  struct test_filesystem_data *data = vfs;
  FAIL_WITH_PROB (data, open_w_fail_prob, i_open_w (data->delegate, dest, fname, e));
}

static err_t
test_remove_quiet (void *vfs, const char *fname, error *e)
{
  struct test_filesystem_data *data = vfs;
  FAIL_WITH_PROB (data, remove_quiet_fail_prob, i_remove_quiet (data->delegate, fname, e));
}

static err_t
test_unlink (void *vfs, const char *name, error *e)
{
  struct test_filesystem_data *data = vfs;
  FAIL_WITH_PROB (data, unlink_fail_prob, i_unlink (data->delegate, name, e));
}

static err_t
test_file_exists (void *vfs, const char *fname, bool *dest, error *e)
{
  struct test_filesystem_data *data = vfs;
  FAIL_WITH_PROB (data, file_exists_fail_prob, i_file_exists (data->delegate, fname, dest, e));
}

////////////////////////////////////////////////////////////
// Default file system vtable

static const struct i_file_system_vtable test_fsvtable = {
    .open_rw      = test_open_rw,
    .open_r       = test_open_r,
    .open_w       = test_open_w,
    .remove_quiet = test_remove_quiet,
    .unlink       = test_unlink,
    .file_exists  = test_file_exists,
};

struct i_file_system
test_filesystem (struct test_filesystem_data *data)
{
  return (struct i_file_system){
      .table = &test_fsvtable,
      .data  = data,
  };
}
