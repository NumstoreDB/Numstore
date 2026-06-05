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

#include "nscore/parsers/user_stride.h"

#include "nscore/parsers/parser.h"

// Parse optional ':' NUMBER (step)
static err_t
parse_step (struct parser *base, struct user_stride *s, error *e)
{
  if (!parser_match (base, TT_COLON))
  {
    return SUCCESS;
  }

  s->present |= COLON_PRESENT;
  parser_advance (base);

  if (parser_match (base, TT_INTEGER))
  {
    struct token *tok = parser_advance (base);
    s->step           = (sb_size)tok->integer;
    s->present |= STEP_PRESENT;
  }

  return SUCCESS;
}

// Parse optional NUMBER ':' NUMBER (stop + step)
static err_t
parse_stop (struct parser *base, struct user_stride *s, error *e)
{
  if (parser_match (base, TT_INTEGER))
  {
    struct token *tok = parser_advance (base);
    s->stop           = (sb_size)tok->integer;
    s->present |= STOP_PRESENT;
  }

  return parse_step (base, s, e);
}

// entry ::= [ NUMBER | NUMBER? ':' NUMBER? | NUMBER? ':' NUMBER? ':' NUMBER? ]
err_t
parse_user_stride (struct parser *parser, struct user_stride *dest, error *e)
{
  struct user_stride s = {0};

  WRAP (parser_expect (parser, TT_LEFT_BRACKET, e));

  if (parser_match (parser, TT_INTEGER))
  {
    struct token *tok = parser_advance (parser);
    s.start           = (sb_size)tok->integer;
    s.present |= START_PRESENT;

    if (!parser_match (parser, TT_COLON))
    {
      // Bare integer — single index
      *dest = s;
      return SUCCESS;
    }

    s.present |= COLON_PRESENT;
    parser_advance (parser);
    WRAP (parse_stop (parser, &s, e));
    *dest = s;
    return SUCCESS;
  }

  // No leading integer — must start with ':'
  if (parser_match (parser, TT_COLON))
  {
    s.present |= COLON_PRESENT;
    parser_advance (parser);
    WRAP (parser_expect (parser, TT_RIGHT_BRACKET, e));
    WRAP (parse_stop (parser, &s, e));
    *dest = s;
    return SUCCESS;
  }

  return error_causef (
      e,
      ERR_SYNTAX,
      "Expected number or ':' at position %u",
      parser->pos
  );
}
