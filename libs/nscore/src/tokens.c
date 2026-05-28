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

#include "nscore/tokens.h"

bool
token_equal (const struct token *left, const struct token *right)
{
  if (left->type != right->type)
  {
    return false;
  }

  switch (left->type)
  {
      // Other
    case TT_STRING:
    case TT_IDENTIFIER:
    {
      return string_equal (
          (struct string){left->str.len, (char *)left->str.data},
          (struct string){left->str.len, (char *)right->str.data}
      );
    }

      // Tokens that start with a number or +/-
    case TT_INTEGER: return left->integer == right->integer;
    case TT_FLOAT: return left->floating == right->floating;

    default:
    {
      return true;
    }
  }
}

const char *
tt_tostr (enum token_t t)
{
  switch (t)
  {
    // Arithmetic Operators
    case_ENUM_RETURN_STRING (TT_PLUS);
    case_ENUM_RETURN_STRING (TT_MINUS);
    case_ENUM_RETURN_STRING (TT_SLASH);
    case_ENUM_RETURN_STRING (TT_STAR);

    // Logical Operators
    case_ENUM_RETURN_STRING (TT_BANG);
    case_ENUM_RETURN_STRING (TT_BANG_EQUAL);
    case_ENUM_RETURN_STRING (TT_EQUAL_EQUAL);
    case_ENUM_RETURN_STRING (TT_GREATER);
    case_ENUM_RETURN_STRING (TT_GREATER_EQUAL);
    case_ENUM_RETURN_STRING (TT_LESS);
    case_ENUM_RETURN_STRING (TT_LESS_EQUAL);

    // Fancy Operators
    case_ENUM_RETURN_STRING (TT_NOT);
    case_ENUM_RETURN_STRING (TT_CARET);
    case_ENUM_RETURN_STRING (TT_PERCENT);
    case_ENUM_RETURN_STRING (TT_PIPE);
    case_ENUM_RETURN_STRING (TT_PIPE_PIPE);
    case_ENUM_RETURN_STRING (TT_AMPERSAND);
    case_ENUM_RETURN_STRING (TT_AMPERSAND_AMPERSAND);

    // Other One char tokens
    case_ENUM_RETURN_STRING (TT_SEMICOLON);
    case_ENUM_RETURN_STRING (TT_COLON);
    case_ENUM_RETURN_STRING (TT_LEFT_BRACKET);
    case_ENUM_RETURN_STRING (TT_RIGHT_BRACKET);
    case_ENUM_RETURN_STRING (TT_LEFT_BRACE);
    case_ENUM_RETURN_STRING (TT_RIGHT_BRACE);
    case_ENUM_RETURN_STRING (TT_LEFT_PAREN);
    case_ENUM_RETURN_STRING (TT_RIGHT_PAREN);
    case_ENUM_RETURN_STRING (TT_COMMA);
    case_ENUM_RETURN_STRING (TT_DOT);

    // Other
    case_ENUM_RETURN_STRING (TT_STRING);
    case_ENUM_RETURN_STRING (TT_IDENTIFIER);

    // Tokens that start with a number or +/-
    case_ENUM_RETURN_STRING (TT_INTEGER);
    case_ENUM_RETURN_STRING (TT_FLOAT);

    // Literal Operations
    case_ENUM_RETURN_STRING (TT_CREATE);
    case_ENUM_RETURN_STRING (TT_DELETE);
    case_ENUM_RETURN_STRING (TT_INSERT);
    case_ENUM_RETURN_STRING (TT_APPEND);
    case_ENUM_RETURN_STRING (TT_READ);
    case_ENUM_RETURN_STRING (TT_WRITE);
    case_ENUM_RETURN_STRING (TT_REMOVE);
    case_ENUM_RETURN_STRING (TT_TAKE);

    // Type literals
    case_ENUM_RETURN_STRING (TT_STRUCT);
    case_ENUM_RETURN_STRING (TT_UNION);
    case_ENUM_RETURN_STRING (TT_PRIM);

    // other literals
    case_ENUM_RETURN_STRING (TT_FILE);
    case_ENUM_RETURN_STRING (TT_QUERY);
    case_ENUM_RETURN_STRING (TT_OFST);
    case_ENUM_RETURN_STRING (TT_LEN);

    // Bools
    case_ENUM_RETURN_STRING (TT_TRUE);
    case_ENUM_RETURN_STRING (TT_FALSE);

    case_ENUM_RETURN_STRING (TT_AS);

    case_ENUM_RETURN_STRING (TT_EOF);
  }

  UNREACHABLE ();
  return NULL;
}
