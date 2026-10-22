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

err_t numstore_init_pager (struct pager *p, error *e);

// Insert into a variable based on it's variable name
// If you pass a non null variable to this function,
// The variable will be captured
sb_size numstore_insert_from_name (
    struct pager *NONNULL       p,
    struct txn *NONNULL         tx,
    struct string               vname,  // Name of the variable
    b_size                      ofst,   // Offset (in elements)
    b_size                      len,    // Length of the data
    struct arena_alloc *NONNULL valloc, // Where to allocate the variable
    struct variable *NULLABLE   var,    // If not null - save the variable
    struct stream *NONNULL      src,    // Input stream
    error *NONNULL              e
);

// Insert into a variable based on already knowing the variable
// Removes a variable lookup operation
sb_size numstore_insert (
    struct pager *NONNULL    p,
    struct txn *NONNULL      tx,
    struct variable *NONNULL var,
    b_size                   ofst,
    b_size                   len,
    struct stream *NONNULL   src,
    error *NONNULL           e
);

////// Read
sb_size numstore_read_from_name (
    struct pager *NONNULL       p,
    struct txn *NONNULL         tx,
    struct string               name,   // Name of the variable
    struct user_stride          ustr,   // Stride to read
    struct arena_alloc *NONNULL valloc, // Allocator for variable in get
    struct variable *NULLABLE   var,    // If not null - save the variable
    struct stream *NONNULL      dest,   // Output stream
    error *NONNULL              e
);

sb_size numstore_read (
    struct pager *NONNULL    p,
    struct txn *NONNULL      tx,
    struct variable *NONNULL var,
    struct user_stride       ustr,
    struct stream *NONNULL   dest,
    error *NONNULL           e
);

////// Read malloc

err_t numstore_read_malloc_from_name (
    struct pager *NONNULL       p,
    struct txn *NONNULL         tx,
    struct string               name,   // Name of the variable
    struct user_stride          ustr,   // Stride to read
    struct arena_alloc *NONNULL valloc, // Allocator for variable in get
    struct variable *NULLABLE   var,    // If not null - save the variable
    void *NONNULL *NULLABLE     dest,   // Dest buffer
    b_size *NONNULL             dlen,   // If not null - save output len
    struct i_mem                mem,    // Where to allocate on
    error *NONNULL              e
);

err_t numstore_read_malloc (
    struct pager *NONNULL    p,
    struct txn *NONNULL      tx,
    struct variable *NONNULL var,
    struct user_stride       ustr,
    void *NONNULL *NULLABLE  dest,
    b_size *NONNULL          dlen,
    struct i_mem             mem,
    error *NONNULL           e
);

////// Write

sb_size numstore_write_from_name (
    struct pager *NONNULL       p,
    struct txn *NONNULL         tx,
    struct string               name,  // Name of the variable
    struct user_stride          ustr,  // Stride to write
    struct arena_alloc *NONNULL alloc, // Allocator for variable in get
    struct variable *NULLABLE   var,   // If not null - save the variable
    struct stream *NONNULL      src,   // Input stream
    error *NONNULL              e
);

sb_size numstore_write (
    struct pager *NONNULL    p,
    struct txn *NONNULL      tx,
    struct variable *NONNULL var,
    struct user_stride       ustr,
    struct stream *NONNULL   src,
    error *NONNULL           e
);

////// Remove

sb_size numstore_remove_from_name (
    struct pager *NONNULL       p,
    struct txn *NONNULL         tx,
    struct string               name,  // Name of the variable
    struct user_stride          ustr,  // Stride to remove
    struct arena_alloc *NONNULL alloc, // Allocator for variable in get
    struct variable *NULLABLE   var,   // If not null - save the variable
    struct stream *NULLABLE     dest,  // Output stream (can be null)
    error *NONNULL              e
);

sb_size numstore_remove (
    struct pager *NONNULL    p,
    struct txn *NONNULL      tx,
    struct variable *NONNULL var,
    struct user_stride       ustr,
    struct stream *NULLABLE  dest,
    error *NONNULL           e
);

////// Remove Malloc

err_t numstore_remove_malloc_from_name (
    struct pager *NONNULL       p,
    struct txn *NONNULL         tx,
    struct string               name,  // Name of the variable
    struct user_stride          ustr,  // Stride to remove
    struct arena_alloc *NONNULL alloc, // Allocator for variable in get
    struct variable *NULLABLE   var,   // If not null - save the variable
    void *NONNULL *NULLABLE     dest,  // destination buffer
    b_size *NONNULL             dlen,  // Output stream (can be null)
    struct i_mem                mem,   // Where to allocate on
    error *NONNULL              e
);

err_t numstore_remove_malloc (
    struct pager *NONNULL    p,
    struct txn *NONNULL      tx,
    struct variable *NONNULL var,
    struct user_stride       ustr,
    void *NONNULL *NULLABLE  dest,
    b_size *NONNULL          dlen,
    struct i_mem             mem,
    error *NONNULL           e
);

////// Get

err_t numstore_get (
    struct pager *NONNULL       p,
    struct txn *NONNULL         tx,
    bool                        if_exists,
    struct string               name,  // Name of the variable
    struct arena_alloc *NONNULL alloc, // Allocator for variable in get
    struct variable *NONNULL    var,   // If not null - save the variable
    error *NONNULL              e
);

////// Delete

err_t numstore_delete (
    struct pager *NONNULL p,
    struct txn *NONNULL   tx,
    struct string         name, // Name of the variable
    bool                  if_exists,
    error *NONNULL        e
);

////// Create

err_t numstore_create (
    struct pager *NONNULL       p,
    struct txn *NONNULL         tx,
    struct string               name,   // Name of new variable
    struct type                 type,   // Type for new variable
    struct arena_alloc *NONNULL valloc, // Allocator for variable
    struct variable *NULLABLE   var,    // If not null - save the variable
    error *NONNULL              e
);

#endif
