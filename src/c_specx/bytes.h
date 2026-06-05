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

#ifndef C_SPECX_BYTES_H
#define C_SPECX_BYTES_H

#include <c_specx/platform.h>
#include <c_specx/stdtypes.h>

////////////////////////////////////////////////////////////
// BYTES

struct bytes
{
  u8 *head;
  u32 len;
};

struct cbytes
{
  const u8 *head;
  u32       len;
};

#define bytes_from(buffer)                 \
  (struct bytes)                           \
  {                                        \
    .head = buffer, .len = sizeof (buffer) \
  }

#define bytes_null() (struct bytes){.head = NULL, .len = 0}

#endif // C_SPECX_BYTES_H
