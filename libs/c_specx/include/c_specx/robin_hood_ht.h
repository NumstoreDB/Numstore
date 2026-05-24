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

// NOTE: This header is a re-includable template. It must be included after
// defining KTYPE (for the first template variant) or HASH_FUNC (for the second).
// It does NOT have a traditional include guard - it is designed to be included
// multiple times with different macro definitions.

#include <c_specx/platform.h>
#include <c_specx/stdtypes.h>
#include <c_specx/logging.h>
#include <c_specx/latch.h>
#include <c_specx/ht_models.h>

////////////////////////////////////////////////////////////
// Robin Hood Hash Table templates
// Define KTYPE/VTYPE/SUFFIX before including for the key-value template,
// or define HASH_FUNC/CMP_FUNC/VTYPE/SUFFIX for the value-only template.

// The actual template code lives in c_specx.h and is activated by:
//   #define KTYPE ...
//   #include <c_specx/robin_hood_ht.h>
// or:
//   #define HASH_FUNC ...
//   #include <c_specx/robin_hood_ht.h>
//
// For direct inclusion, use the umbrella header c_specx.h which still contains
// the template code sections guarded by #ifdef KTYPE and #ifdef HASH_FUNC.
// Alternatively, include c_specx.h directly for backward compatibility.
