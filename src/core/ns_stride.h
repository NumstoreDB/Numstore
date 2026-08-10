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

#ifndef NS_STRIDE_H
#define NS_STRIDE_H

#include <stdbool.h>

#include "core/ns_alloc.h"
#include "core/ns_error.h"
#include "core/ns_linked_list.h"
#include "core/ns_platform.h"
#include "core/ns_stdtypes.h"

struct builder;

/******************************************************************************
 * SECTION: Stride
 * ----------------------------------------------------------------------------
 *
 * @brief Stride is a pattern for iterating through an array
 ******************************************************************************/

/**
 * @struct stride
 * @brief A more tight stride than user_stride that restricts to the domain of
 * an array
 *
 * Stride is the main point of entry for any strided operation. User stride is
 * just the user facing version - user strides are "resolved" into strides
 *
 * @var stride::start
 * @brief The start index
 *
 * @var stride::stride
 * @brief The step between each element
 *
 * @var stride::nelems
 * @brief The number of elements to touch
 */
struct stride
{
  u64 start;
  u64 stride;
  u64 nelems;
};

enum
{
  START_PRESENT = (1 << 0),
  STEP_PRESENT  = (1 << 1),
  STOP_PRESENT  = (1 << 2),
  COLON_PRESENT = (1 << 3),
};

/**
 * @struct user_stride
 * @brief The user stride is an easy way for the user to define a stride based
 * on the variable
 *
 * In normal stride operations, queries like [0:-1] or [0::] or [:-1] etc are
 * all valid queries. User stride encodes this property and there's a single
 * function to convert a user stride into a stride for more rigorous stride
 * operations
 *
 * @var user_stride::start
 * @brief The start element - meaningless if present & START_PRESENT is 0
 * Negative means from the end
 *
 * @var user_stride::step
 * @brief The step - equivalent to stride::stride
 * meaningless if present & STEP_PRESENT is 0
 *
 * @var user_stride::end
 * @brief The end element - meaningless if present & STOP_PRESENT is 0
 *
 * @var user_stride::present
 * @brief A set of flags on which value is present.
 */

struct user_stride
{
  i64 start;
  i64 step;
  i64 stop;
  u32 present;
};

struct multi_user_stride
{
  struct user_stride *strides;
  u32                 len;
};

/**
 * @brief all elements from 0 to end
 */
#define USER_STRIDE_ALL                        \
  ((struct user_stride){                       \
      .start   = 0,                            \
      .step    = 1,                            \
      .stop    = 0,                            \
      .present = STEP_PRESENT | START_PRESENT, \
  })

bool  ustride_equal (struct user_stride left, struct user_stride right);
bool  user_stride_equal (const struct user_stride *left, const struct user_stride *right);
void  stride_resolve_expect (struct stride *dest, struct user_stride src, u64 arrlen);
err_t stride_resolve (struct stride *dest, struct user_stride src, u64 arrlen, error *e);

/*-----------------------------------------------------------------------------
 * SUBSECTION: Small Constructors
 * @brief Short Constructors for building user strides
 *----------------------------------------------------------------------------*/

#define make_ustride(start_, stop_, step_, present_) \
  ((struct user_stride){                             \
      .start   = (start_),                           \
      .stop    = (stop_),                            \
      .step    = (step_),                            \
      .present = (present_),                         \
  })

// [:stop]
HEADER_FUNC struct user_stride
ustride1 (i64 stop)
{
  return make_ustride (0, stop, 0, STOP_PRESENT | COLON_PRESENT);
}

// [::step]
HEADER_FUNC struct user_stride
ustride2 (i64 step)
{
  return make_ustride (0, 0, step, STEP_PRESENT | COLON_PRESENT);
}

// [:stop:step]
HEADER_FUNC struct user_stride
ustride12 (i64 stop, i64 step)
{
  return make_ustride (0, stop, step, STOP_PRESENT | STEP_PRESENT | COLON_PRESENT);
}

// [start:]
HEADER_FUNC struct user_stride
ustride0 (i64 start)
{
  return make_ustride (start, 0, 0, START_PRESENT | COLON_PRESENT);
}

// [start:stop]
HEADER_FUNC struct user_stride
ustride01 (i64 start, i64 stop)
{
  return make_ustride (start, stop, 0, STOP_PRESENT | START_PRESENT | COLON_PRESENT);
}

// [start::step]
HEADER_FUNC struct user_stride
ustride02 (i64 start, i64 step)
{
  return make_ustride (start, 0, step, STEP_PRESENT | START_PRESENT | COLON_PRESENT);
}

// [start:stop:step]
HEADER_FUNC struct user_stride
ustride012 (i64 start, i64 stop, i64 step)
{
  return make_ustride (
      start,
      stop,
      step,
      STOP_PRESENT | STEP_PRESENT | START_PRESENT | COLON_PRESENT
  );
}

// [start]  - bare index, no colon
HEADER_FUNC struct user_stride
ustride_single (i64 start)
{
  return make_ustride (start, 0, 0, START_PRESENT);
}

// [:]  - colon only
HEADER_FUNC struct user_stride
ustride (void)
{
  return make_ustride (0, 0, 0, COLON_PRESENT);
}

HEADER_FUNC struct user_stride
usfrms (const struct stride str)
{
  return ustride012 (str.start, str.start + str.stride * str.nelems, str.stride);
}

/*-----------------------------------------------------------------------------
 * SUBSECTION: Multi User Stride Builder
 *----------------------------------------------------------------------------*/

struct mus_llnode
{
  struct user_stride stride;
  struct llnode      link;
};

struct mus_builder
{
  struct llnode  *head;
  struct builder *b;
};

struct mus_builder musb_create (struct builder *b);
err_t              musb_accept_key (struct mus_builder *eb, struct user_stride stride, error *e);
err_t              musb_build (struct multi_user_stride *m, struct mus_builder *eb, error *e);

#endif // NS_STRIDE_H
