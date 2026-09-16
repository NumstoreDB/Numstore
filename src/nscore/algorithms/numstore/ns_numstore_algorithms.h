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

/**
 * @file
 * @brief Internals of numstore user exposed type
 */

#ifndef NS_NUMSTORE_ALGORITHMS_H
#define NS_NUMSTORE_ALGORITHMS_H

#include "core/ns_error.h"
#include "nscore/pager/ns_pager.h"
#include "nscore/variables/ns_variables.h"

sb_size numstore_insert (
    struct pager       *p,
    struct ns_txn      *tx,
    struct string       vname,  // Name of the variable
    b_size              ofst,   // Offset (in elements)
    b_size              len,    // Length of the data
    struct arena_alloc *valloc, // Where to allocate the variable
    struct variable    *var,    // If not null - save the variable
    struct stream      *src,    // Input stream
    error              *e
);

sb_size numstore_read (
    struct pager       *p,
    struct ns_txn      *tx,
    struct string       name,   // Name of the variable
    struct user_stride  ustr,   // Stride to read
    struct arena_alloc *valloc, // Allocator for variable in get
    struct variable    *var,    // If not null - save the variable
    struct stream      *dest,   // Output stream
    error              *e
);

void *numstore_read_malloc (
    struct pager       *p,
    struct ns_txn      *tx,
    struct string       name,   // Name of the variable
    struct user_stride  ustr,   // Stride to read
    struct arena_alloc *valloc, // Allocator for variable in get
    struct variable    *var,    // If not null - save the variable
    b_size             *dlen,   // If not null - save output len
    struct i_mem        mem,    // Where to allocate on
    error              *e
);

sb_size numstore_write (
    struct pager       *p,
    struct ns_txn      *tx,
    struct string       name,  // Name of the variable
    struct user_stride  ustr,  // Stride to write
    struct arena_alloc *alloc, // Allocator for variable in get
    struct variable    *var,   // If not null - save the variable
    struct stream      *src,   // Input stream
    error              *e
);

sb_size numstore_remove (
    struct pager       *p,
    struct ns_txn      *tx,
    struct string       name,  // Name of the variable
    struct user_stride  ustr,  // Stride to remove
    struct arena_alloc *alloc, // Allocator for variable in get
    struct variable    *var,   // If not null - save the variable
    struct stream      *dest,  // Output stream (can be null)
    error              *e
);

void *numstore_remove_malloc (
    struct pager       *p,
    struct ns_txn      *tx,
    struct string       name,  // Name of the variable
    struct user_stride  ustr,  // Stride to remove
    struct arena_alloc *alloc, // Allocator for variable in get
    struct variable    *var,   // If not null - save the variable
    b_size             *dlen,  // Output stream (can be null)
    struct i_mem        mem,   // Where to allocate on
    error              *e
);

err_t numstore_get (
    struct pager       *p,
    struct ns_txn      *tx,
    bool                if_exists,
    struct string       name,  // Name of the variable
    struct arena_alloc *alloc, // Allocator for variable in get
    struct variable    *var,   // If not null - save the variable
    error              *e
);

err_t numstore_delete (
    struct pager  *p,
    struct ns_txn *tx,
    struct string  name, // Name of the variable
    bool           if_exists,
    error         *e
);

err_t numstore_create (
    struct pager       *p,
    struct ns_txn      *tx,
    struct string       name,   // Name of new variable
    struct type         type,   // Type for new variable
    struct arena_alloc *valloc, // Allocator for variable
    struct variable    *var,    // If not null - save the variable
    error              *e
);

#endif
