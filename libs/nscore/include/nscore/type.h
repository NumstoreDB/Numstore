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

#include "nscore/parser.h"

#include <c_specx.h>

// type            ::= struct_type
// | union_type
// | sarray_type
// | primitive_type
// struct_type     ::= 'struct' '{' field (',' field)* '}'
// union_type      ::= 'union' '{' field (',' field)* '}'
// sarray_type     ::= '[' INTEGER ']'+ type
// primitive_type  ::= PRIM
// field           ::= IDENTIFIER type

err_t parse_type (
    struct parser      *p,
    struct type        *dest,
    struct chunk_alloc *dalloc,
    error              *e
);
