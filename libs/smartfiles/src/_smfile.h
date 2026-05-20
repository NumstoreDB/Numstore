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

#include "c_specx.h"
#include "nscore/pager.h"
#include "nscore/variables.h"

#define DEFAULT_VARIABLE "."

int _smfile_crash (smfile_t *ns);

HEADER_FUNC struct string vname_or_default (const char *name) {
  if (name != NULL) {
    return strfcstr (name);
  } else {
    return strfcstr (DEFAULT_VARIABLE);
  }
}
