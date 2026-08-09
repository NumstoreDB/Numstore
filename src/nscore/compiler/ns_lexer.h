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

#ifndef NS_LEXER_H
#define NS_LEXER_H

#include "core/ns_alloc.h"
#include "core/ns_dbl_buffer.h"
#include "core/ns_stdtypes.h" // u32 ...etc

struct lexer
{
  const char *src;
  u32         src_len;
  u32         start;
  u32         current;

  struct token *tokens;

  u32               ntokens;
  struct dbl_buffer _tokens;
  struct allocator *alloc;
};

err_t lex_tokens (
    const char       *src,
    struct allocator *alloc,
    u32               src_len,
    struct lexer     *lex,
    error            *e
);

#endif
