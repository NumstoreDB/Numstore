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

#include "ns_arena_alloc.h"
#include "ns_dbl_buffer.h"
#include "ns_error.h"
#include "ns_stdtypes.h" // u32 ...etc
#include "ns_string.h"

struct arena_alloc;

/// The longest source text lex_tokens accepts. Its length is a u32, so
/// callers measure text with strnlen bounded by this.
#define MAX_COMPILE_TEXT_LEN UINT32_MAX

struct lexer
{
  const char         *src;
  u32                 src_len;
  u32                 start;
  u32                 current;

  struct token       *tokens;

  u32                 ntokens;
  struct dbl_buffer   _tokens;
  struct arena_alloc *alloc;
};

err_t lex_tokens (
    const char         *src,
    struct arena_alloc *alloc,
    u32                 src_len,
    struct lexer       *lex,
    error              *e
);

/// True if [text, text + len) lexes as a primitive type or a keyword rather
/// than an identifier, so it cannot be used as a variable name.
bool lex_is_keyword (const char *text, u32 len);

/// Fills [buffer] with a random identifier of [length] bytes, the last one
/// the null terminator. It is never a keyword or a primitive type name.
void var_random_name (char *buffer, u32 length);

/// A random identifier of minlen to maxlen bytes, allocated from [alloc]
err_t rand_varname (
    struct string      *dest,
    struct arena_alloc *alloc,
    u32                 minlen,
    u32                 maxlen,
    error              *e
);

#endif
