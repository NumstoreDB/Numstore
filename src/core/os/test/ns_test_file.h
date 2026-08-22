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

#ifndef NS_TEST_FILE_H
#define NS_TEST_FILE_H

#include "core/ns_numerics.h"
#include "core/os/ns_file.h"

struct test_file_data
{
  float close_fail_prob;
  float eof_fail_prob;
  float fsync_fail_prob;
  float file_size_fail_prob;

  float read_once_fail_prob;
  float read_once_some_prob;

  float pread_once_fail_prob;
  float pread_once_some_prob;

  float write_once_fail_prob;
  float write_once_some_prob;

  float pwrite_once_fail_prob;
  float pwrite_once_some_prob;

  float writev_all_fail_prob;
  float truncate_fail_prob;
  float fallocate_fail_prob;
  float seek_fail_prob;
};

void register_test_file_data (struct test_file_data *data);

static inline err_t
i_file_fault_check (const i_file *f, float prob, error *e)
{
  if (f->table->test_data && randf () < prob) {
    return error_causef (e, ERR_IO, "Simulated fault");
  }
  return 0;
}

static inline err_t
i_file_some_check (u64 amount, float prob)
{
  if (randf () < prob) {
    return randu32r (1, amount);
  }
  return amount;
}

#define I_FILE_FAULT(f, field, e)                        \
  do {                                                   \
    struct test_file_data *data = (f)->table->test_data; \
    float                  prob;                         \
    if (data) {                                          \
      prob = data->field;                                \
    } else {                                             \
      prob = 0.0f;                                       \
    }                                                    \
    err_t _fe = i_file_fault_check ((f), prob, (e));     \
    if (_fe) return _fe;                                 \
  }                                                      \
  while (0)

#define I_FILE_CONDITION_AMOUNT(fp, field, amount)      \
  do {                                                  \
    struct test_file_data *data = fp->table->test_data; \
    float                  prob;                        \
    if (data) {                                         \
      prob = data->field;                               \
    } else {                                            \
      prob = 0.0f;                                      \
    }                                                   \
    amount = i_file_some_check (amount, prob);          \
  }                                                     \
  while (0)

#endif
