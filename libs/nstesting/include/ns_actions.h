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

#pragma once

enum action_type
{
  AT_CREATE           = 0,
  AT_DELETE           = 1,
  AT_GET              = 2,
  AT_INSERT           = 3,
  AT_READ             = 4,
  AT_WRITE            = 5,
  AT_REMOVE           = 6,
  AT_BEGIN            = 7,
  AT_COMMIT_TXN       = 8,
  AT_ROLLBACK_TXN     = 9,
  AT_CLOSE_AND_REOPEN = 10,
  AT_CRASH_AND_REOPEN = 11,

  AT_LEN,
};
