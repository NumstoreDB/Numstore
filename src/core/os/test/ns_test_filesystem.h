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

#ifndef NS_TEST_FILESYSTEM_H
#define NS_TEST_FILESYSTEM_H

#include "core/os/ns_filesystem.h"

struct test_filesystem_data
{
  // Probability of just failure
  float                open_rw_fail_prob;
  float                open_r_fail_prob;
  float                open_w_fail_prob;
  float                remove_quiet_fail_prob;
  float                unlink_fail_prob;
  float                file_exists_fail_prob;

  // The fallback file system in green path
  struct i_file_system delegate;
};

struct i_file_system test_filesystem (struct test_filesystem_data *data);

#endif
