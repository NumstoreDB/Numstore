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

#include "fpool.h"

#include "htable.h"
#include "os.h"

struct fpool *
fpool_create (u32 nframes, fname_to_fid_map map, error *e)
{
  u32 size = sizeof (struct fpool) + nframes * sizeof (struct fpool_frame);
  struct fpool *ret = i_calloc (1, size, e);
  if (ret == NULL)
  {
    return NULL;
  }

  ret->nframes = nframes;

  return ret;
}
