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

#include "compiler.h"

#include "alloc.h"
#include "collections.h"
#include "error.h"
#include "numerics.h" // parse_i32_expect
#include "query.h"
#include "serial.h"          // string_equal
#include "testing/testing.h" // TEST
#include "types.h"
#include "utils.h" // case_ENUM_RETURN_STRING

/******************************************************************************
 * SECTION: Tokens
 ******************************************************************************/

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
    case_ENUM_RETURN_STRING (TT_GET);
    case_ENUM_RETURN_STRING (TT_EXIT);
    case_ENUM_RETURN_STRING (TT_HELP);
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

/******************************************************************************
 * SECTION: Lexer
 ******************************************************************************/

static bool
is_at_end (struct lexer *lex)
{
  return lex->current >= lex->src_len;
}

static char
peek (struct lexer *lex)
{
  if (is_at_end (lex))
  {
    return '\0';
  }
  return lex->src[lex->current];
}

static char
peek_next (struct lexer *lex)
{
  if (lex->current + 1 >= lex->src_len)
  {
    return '\0';
  }
  return lex->src[lex->current + 1];
}

static char
advance (struct lexer *lex)
{
  ASSERT (!is_at_end (lex));
  return lex->src[lex->current++];
}

static bool
match (struct lexer *lex, char expected)
{
  if (is_at_end (lex))
  {
    return false;
  }
  if (lex->src[lex->current] != expected)
  {
    return false;
  }

  lex->current++;
  return true;
}

static err_t
add_token (struct lexer *lex, enum token_t type, error *e)
{
  struct token next = {
      .type       = type,
      .text_start = &lex->src[lex->start],
      .text_len   = lex->current - lex->start
  };

  return dblb_append (&lex->_tokens, &next, 1, e);
}

static err_t
add_token_int (struct lexer *lex, i32 value, error *e)
{
  struct token next = {
      .type       = TT_INTEGER,
      .integer    = value,
      .text_start = &lex->src[lex->start],
      .text_len   = lex->current - lex->start
  };

  return dblb_append (&lex->_tokens, &next, 1, e);
}

static err_t
add_token_float (struct lexer *lex, f32 value, error *e)
{
  struct token next = (struct token){.type       = TT_FLOAT,
                                     .floating   = value,
                                     .text_start = &lex->src[lex->start],
                                     .text_len   = lex->current - lex->start};

  return dblb_append (&lex->_tokens, &next, 1, e);
}

static err_t
add_token_str (
    struct lexer *lex,
    enum token_t  type,
    const char   *data,
    u32           len,
    error        *e
)
{
  struct token next = (struct token){.type = type,
                                     .str =
                                         {
                                             .data = data,
                                             .len  = len,
                                         },
                                     .text_start = &lex->src[lex->start],
                                     .text_len   = lex->current - lex->start};

  return dblb_append (&lex->_tokens, &next, 1, e);
}

static err_t
add_token_prim (struct lexer *lex, enum prim_t prim, error *e)
{
  struct token next = (struct token){.type       = TT_PRIM,
                                     .prim       = prim,
                                     .text_start = &lex->src[lex->start],
                                     .text_len   = lex->current - lex->start};

  return dblb_append (&lex->_tokens, &next, 1, e);
}

static enum token_t
check_keyword (const char *text, u32 len)
{
  if (len == sizeof ("create") - 1 && strncmp (text, "create", len) == 0)
  {
    return TT_CREATE;
  }
  if (len == sizeof ("delete") - 1 && strncmp (text, "delete", len) == 0)
  {
    return TT_DELETE;
  }
  if (len == sizeof ("get") - 1 && strncmp (text, "get", len) == 0)
  {
    return TT_GET;
  }
  if (len == sizeof ("exit") - 1 && strncmp (text, "exit", len) == 0)
  {
    return TT_EXIT;
  }
  if (len == sizeof ("help") - 1 && strncmp (text, "help", len) == 0)
  {
    return TT_HELP;
  }
  if (len == sizeof ("insert") - 1 && strncmp (text, "insert", len) == 0)
  {
    return TT_INSERT;
  }
  if (len == sizeof ("append") - 1 && strncmp (text, "append", len) == 0)
  {
    return TT_APPEND;
  }
  if (len == sizeof ("read") - 1 && strncmp (text, "read", len) == 0)
  {
    return TT_READ;
  }
  if (len == sizeof ("write") - 1 && strncmp (text, "write", len) == 0)
  {
    return TT_WRITE;
  }
  if (len == sizeof ("remove") - 1 && strncmp (text, "remove", len) == 0)
  {
    return TT_REMOVE;
  }
  if (len == sizeof ("take") - 1 && strncmp (text, "take", len) == 0)
  {
    return TT_TAKE;
  }

  if (len == sizeof ("file") - 1 && strncmp (text, "file", len) == 0)
  {
    return TT_FILE;
  }
  if (len == sizeof ("query") - 1 && strncmp (text, "query", len) == 0)
  {
    return TT_QUERY;
  }
  if (len == sizeof ("ofst") - 1 && strncmp (text, "ofst", len) == 0)
  {
    return TT_OFST;
  }
  if (len == sizeof ("len") - 1 && strncmp (text, "len", len) == 0)
  {
    return TT_LEN;
  }

  if (len == sizeof ("struct") - 1 && strncmp (text, "struct", len) == 0)
  {
    return TT_STRUCT;
  }
  if (len == sizeof ("union") - 1 && strncmp (text, "union", len) == 0)
  {
    return TT_UNION;
  }

  if (len == sizeof ("true") - 1 && strncmp (text, "true", len) == 0)
  {
    return TT_TRUE;
  }
  if (len == sizeof ("false") - 1 && strncmp (text, "false", len) == 0)
  {
    return TT_FALSE;
  }

  if (len == sizeof ("as") - 1 && strncmp (text, "as", len) == 0)
  {
    return TT_AS;
  }

  return TT_IDENTIFIER;
}

static err_t
scan_string (struct lexer *lex, error *e)
{
  while (!is_at_end (lex) && peek (lex) != '"')
  {
    advance (lex);
  }

  if (is_at_end (lex))
  {
    return error_causef (
        e,
        ERR_SYNTAX,
        "Unterminated string at position %u",
        lex->start
    );
  }

  advance (lex); // Closing quote

  return add_token_str (
      lex,
      TT_STRING,
      &lex->src[lex->start + 1],
      (lex->current - lex->start) - 2,
      e
  );
}

static err_t
scan_number (struct lexer *lex, error *e)
{
  while (!is_at_end (lex) && is_num (peek (lex)))
  {
    advance (lex);
  }

  bool is_float = false;
  if (peek (lex) == '.' && is_num (peek_next (lex)))
  {
    is_float = true;
    advance (lex);

    while (!is_at_end (lex) && is_num (peek (lex)))
    {
      advance (lex);
    }
  }

  const char *text = &lex->src[lex->start];
  u32         len  = lex->current - lex->start;

  if (is_float)
  {
    f32 value;
    WRAP (parse_f32_expect (&value, text, len, e));
    return add_token_float (lex, value, e);
  }
  else
  {
    i32 value;
    WRAP (parse_i32_expect (&value, text, len, e));
    return add_token_int (lex, value, e);
  }

  return SUCCESS;
}

static err_t
scan_identifier (struct lexer *lex, error *e)
{
  while (!is_at_end (lex) && is_alpha_num (peek (lex)))
  {
    advance (lex);
  }

  const char *text = &lex->src[lex->start];
  u32         len  = lex->current - lex->start;

  // Check for primitive types first
  enum prim_t prim = strtoprim (text, len);
  if (prim != (enum prim_t) - 1)
  {
    return add_token_prim (lex, prim, e);
  }

  // Check for keywords
  enum token_t type = check_keyword (text, len);

  if (type == TT_IDENTIFIER)
  {
    return add_token_str (lex, TT_IDENTIFIER, text, len, e);
  }
  else
  {
    return add_token (lex, type, e);
  }
}

static err_t
scan_token (struct lexer *lex, error *e)
{
  char c = advance (lex);

  switch (c)
  {
    case ' ':
    case '\r':
    case '\t':
    case '\n':
    {
      return SUCCESS;
    }
    case '+':
    {
      return add_token (lex, TT_PLUS, e);
    }
    case '-':
    {
      return add_token (lex, TT_MINUS, e);
    }
    case '/':
    {
      return add_token (lex, TT_SLASH, e);
    }
    case '*':
    {
      return add_token (lex, TT_STAR, e);
    }
    case '~':
    {
      return add_token (lex, TT_NOT, e);
    }
    case '^':
    {
      return add_token (lex, TT_CARET, e);
    }
    case '%':
    {
      return add_token (lex, TT_PERCENT, e);
    }
    case ';':
    {
      return add_token (lex, TT_SEMICOLON, e);
    }
    case ':':
    {
      return add_token (lex, TT_COLON, e);
    }
    case '[':
    {
      return add_token (lex, TT_LEFT_BRACKET, e);
    }
    case ']':
    {
      return add_token (lex, TT_RIGHT_BRACKET, e);
    }
    case '{':
    {
      return add_token (lex, TT_LEFT_BRACE, e);
    }
    case '}':
    {
      return add_token (lex, TT_RIGHT_BRACE, e);
    }
    case '(':
    {
      return add_token (lex, TT_LEFT_PAREN, e);
    }
    case ')':
    {
      return add_token (lex, TT_RIGHT_PAREN, e);
    }
    case ',':
    {
      return add_token (lex, TT_COMMA, e);
    }
    case '.':
    {
      return add_token (lex, TT_DOT, e);
    }
    case '!':
    {
      add_token (lex, match (lex, '=') ? TT_BANG_EQUAL : TT_BANG, e);
      return SUCCESS;
    }
    case '=':
    {
      if (!match (lex, '='))
      {
        return error_causef (
            e,
            ERR_SYNTAX,
            "Unexpected '=' at "
            "position %u (use '==' "
            "for equality)",
            lex->start
        );
      }
      return add_token (lex, TT_EQUAL_EQUAL, e);
    }
    case '>':
    {
      return add_token (
          lex,
          match (lex, '=') ? TT_GREATER_EQUAL : TT_GREATER,
          e
      );
    }
    case '<':
    {
      return add_token (lex, match (lex, '=') ? TT_LESS_EQUAL : TT_LESS, e);
    }
    case '|':
    {
      return add_token (lex, match (lex, '|') ? TT_PIPE_PIPE : TT_PIPE, e);
    }
    case '&':
    {
      return add_token (
          lex,
          match (lex, '&') ? TT_AMPERSAND_AMPERSAND : TT_AMPERSAND,
          e
      );
    }
    case '"':
    {
      return scan_string (lex, e);
    }

    default:
    {
      if (is_num (c))
      {
        return scan_number (lex, e);
      }

      if (is_alpha (c))
      {
        return scan_identifier (lex, e);
      }

      return error_causef (
          e,
          ERR_SYNTAX,
          "Unexpected character '%c' at position %u",
          c,
          lex->start
      );
    }
  }
}

err_t
lex_tokens (const char *src, u32 src_len, struct lexer *lex, error *e)
{
  memset (lex, 0, sizeof (*lex));
  lex->src     = src;
  lex->src_len = src_len;
  lex->start   = 0;
  lex->current = 0;
  WRAP (dblb_create (&lex->_tokens, sizeof (struct token), 256, e));

  while (!is_at_end (lex))
  {
    lex->start = lex->current;
    if (scan_token (lex, e))
    {
      goto failed;
    }
  }

  if (add_token (lex, TT_EOF, e))
  {
    goto failed;
  }

  lex->ntokens = lex->_tokens.nelem;
  lex->tokens  = lex->_tokens.data;

  return SUCCESS;

failed:
  dblb_free (&lex->_tokens);
  return error_trace (e);
}

void
lex_free (struct lexer *lex)
{
  dblb_free (&lex->_tokens);
}

#ifndef NTEST

static void
test_lexer_case (const char *input, const struct token *expected, u32 nexpected)
{
  struct lexer lex;
  error        e = error_create ();

  err_t result = lex_tokens (input, strlen (input), &lex, &e);

  // Check for expected errors
  if (nexpected == 0)
  {
    test_assert (result != SUCCESS);
    return;
  }

  // Should succeed
  test_assert_int_equal (result, SUCCESS);

  // Check token count (including EOF)
  test_assert_int_equal (lex.ntokens, nexpected + 1);

  // Compare each token
  for (u32 i = 0; i < nexpected; i++)
  {
    struct token       *left  = &lex.tokens[i];
    const struct token *right = &expected[i];

    if (!token_equal (left, right))
    {
      i_log_failure ("Input: %s\n", input);
      i_log_failure (
          "Token %u: got %s, expected %s\n",
          i,
          tt_tostr (left->type),
          tt_tostr (right->type)
      );
    }
    test_assert (token_equal (left, right));
  }

  lex_free (&lex);
}

TEST (lexer_two_char_tokens)
{
  const char *src = "! ! != != == ! < <= > >= || && , ! , !=";

  struct token expected[] = {
      quick_tok (TT_BANG),
      quick_tok (TT_BANG),
      quick_tok (TT_BANG_EQUAL),
      quick_tok (TT_BANG_EQUAL),
      quick_tok (TT_EQUAL_EQUAL),
      quick_tok (TT_BANG),
      quick_tok (TT_LESS),
      quick_tok (TT_LESS_EQUAL),
      quick_tok (TT_GREATER),
      quick_tok (TT_GREATER_EQUAL),
      quick_tok (TT_PIPE_PIPE),
      quick_tok (TT_AMPERSAND_AMPERSAND),
      quick_tok (TT_COMMA),
      quick_tok (TT_BANG),
      quick_tok (TT_COMMA),
      quick_tok (TT_BANG_EQUAL),
  };

  test_lexer_case (src, expected, arrlen (expected));
}

TEST (lexer_single_char_operators)
{
  const char *src = "+ - / * ~ ^ % | & ; : [ ] { } ( ) ,";

  struct token expected[] = {
      quick_tok (TT_PLUS),
      quick_tok (TT_MINUS),
      quick_tok (TT_SLASH),
      quick_tok (TT_STAR),
      quick_tok (TT_NOT),
      quick_tok (TT_CARET),
      quick_tok (TT_PERCENT),
      quick_tok (TT_PIPE),
      quick_tok (TT_AMPERSAND),
      quick_tok (TT_SEMICOLON),
      quick_tok (TT_COLON),
      quick_tok (TT_LEFT_BRACKET),
      quick_tok (TT_RIGHT_BRACKET),
      quick_tok (TT_LEFT_BRACE),
      quick_tok (TT_RIGHT_BRACE),
      quick_tok (TT_LEFT_PAREN),
      quick_tok (TT_RIGHT_PAREN),
      quick_tok (TT_COMMA),
  };

  test_lexer_case (src, expected, arrlen (expected));
}

TEST (lexer_strings)
{
  const char *src = "\"hello\" \"world\" \"foo bar\"";

  struct token expected[] = {
      tt_string ("hello", sizeof ("hello") - 1),
      tt_string ("world", sizeof ("world") - 1),
      tt_string ("foo bar", sizeof ("foo bar") - 1),
  };

  test_lexer_case (src, expected, arrlen (expected));
}

TEST (lexer_identifiers)
{
  const char *src = "foo bar baz_qux hello123";

  struct token expected[] = {
      tt_ident ("foo", sizeof ("foo") - 1),
      tt_ident ("bar", sizeof ("bar") - 1),
      tt_ident ("baz_qux", sizeof ("baz_qux") - 1),
      tt_ident ("hello123", sizeof ("hello123") - 1),
  };

  test_lexer_case (src, expected, arrlen (expected));
}

TEST (lexer_numbers)
{
  const char *src = "0 123 456 1.0 3.14 0.5";

  struct token expected[] = {
      tt_integer (0),
      tt_integer (123),
      tt_integer (456),
      tt_float (1.0f),
      tt_float (3.14f),
      tt_float (0.5f),
  };

  test_lexer_case (src, expected, arrlen (expected));
}

TEST (lexer_keywords)
{
  const char *src =
      "create delete get exit help insert file query struct union true false";

  struct token expected[] = {
      quick_tok (TT_CREATE),
      quick_tok (TT_DELETE),
      quick_tok (TT_GET),
      quick_tok (TT_EXIT),
      quick_tok (TT_HELP),
      quick_tok (TT_INSERT),
      quick_tok (TT_FILE),
      quick_tok (TT_QUERY),
      quick_tok (TT_STRUCT),
      quick_tok (TT_UNION),
      quick_tok (TT_TRUE),
      quick_tok (TT_FALSE),
  };

  test_lexer_case (src, expected, arrlen (expected));
}

TEST (lexer_primitives)
{
  const char *src = "u8 u16 u32 u64 i8 i16 i32 i64 f32 f64";

  struct token expected[] = {
      tt_prim (U8),
      tt_prim (U16),
      tt_prim (U32),
      tt_prim (U64),
      tt_prim (I8),
      tt_prim (I16),
      tt_prim (I32),
      tt_prim (I64),
      tt_prim (F32),
      tt_prim (F64),
  };

  test_lexer_case (src, expected, arrlen (expected));
}

TEST (lexer_whitespace_handling)
{
  // Test that whitespace is properly ignored
  const char *src1 = "a+b";
  const char *src2 = "a + b";
  const char *src3 = "  a  +  b  ";

  struct token expected[] = {
      tt_ident ("a", sizeof ("a") - 1),
      quick_tok (TT_PLUS),
      tt_ident ("b", sizeof ("b") - 1),
  };

  test_lexer_case (src1, expected, arrlen (expected));
  test_lexer_case (src2, expected, arrlen (expected));
  test_lexer_case (src3, expected, arrlen (expected));
}

TEST (lexer_complex_expression)
{
  const char *src = "create foo { x: u32, y: f32 };";

  struct token expected[] = {
      quick_tok (TT_CREATE),
      tt_ident ("foo", sizeof ("foo") - 1),
      quick_tok (TT_LEFT_BRACE),
      tt_ident ("x", sizeof ("x") - 1),
      quick_tok (TT_COLON),
      tt_prim (U32),
      quick_tok (TT_COMMA),
      tt_ident ("y", sizeof ("y") - 1),
      quick_tok (TT_COLON),
      tt_prim (F32),
      quick_tok (TT_RIGHT_BRACE),
      quick_tok (TT_SEMICOLON),
  };

  test_lexer_case (src, expected, arrlen (expected));
}

TEST (lexer_keyword_prefix)
{
  // Keywords shouldn't match if they're prefixes
  const char *src = "create crate createx truex falsey";

  struct token expected[] = {
      quick_tok (TT_CREATE),
      tt_ident ("crate", sizeof ("crate") - 1),
      tt_ident ("createx", sizeof ("createx") - 1),
      tt_ident ("truex", sizeof ("truex") - 1),
      tt_ident ("falsey", sizeof ("falsey") - 1),
  };

  test_lexer_case (src, expected, arrlen (expected));
}

TEST (lexer_errors)
{
  // Unterminated string
  test_lexer_case ("\"unterminated", NULL, 0);

  // Invalid character
  test_lexer_case ("@", NULL, 0);

  // Bare equals (should be ==)
  test_lexer_case ("x = y", NULL, 0);
}

TEST (lexer_empty_string)
{
  const char *src = "\"\"";

  struct token expected[] = {
      tt_string ("", 0),
  };

  test_lexer_case (src, expected, arrlen (expected));
}

TEST (lexer_numbers_in_sequence)
{
  const char *src = "123 456.78 9";

  struct token expected[] = {
      tt_integer (123),
      tt_float (456.78f),
      tt_integer (9),
  };

  test_lexer_case (src, expected, arrlen (expected));
}

#endif // NTEST

/******************************************************************************
 * SECTION: Compiler
 ******************************************************************************/

err_t
compile_type (
    struct type        *dest,
    const char         *text,
    struct chunk_alloc *dalloc,
    error              *e
)
{
  struct lexer lex;
  WRAP (lex_tokens (text, strlen (text), &lex, e));

  struct parser parser = parser_init (lex.tokens, lex.ntokens);

  err_t ret = parse_type (&parser, dest, dalloc, e);
  lex_free (&lex);
  return ret;
}

err_t
compile_subtype (
    struct subtype     *dest,
    const char         *text,
    struct chunk_alloc *dalloc,
    error              *e
)
{
  struct lexer lex;
  WRAP (lex_tokens (text, strlen (text), &lex, e));

  struct parser parser = parser_init (lex.tokens, lex.ntokens);

  err_t ret = parse_subtype (&parser, dest, dalloc, e);
  lex_free (&lex);
  return ret;
}

err_t
compile_multi_user_stride (
    struct multi_user_stride *dest,
    const char               *text,
    struct chunk_alloc       *dalloc,
    error                    *e
)
{
  struct lexer lex;
  WRAP (lex_tokens (text, strlen (text), &lex, e));

  struct parser parser = parser_init (lex.tokens, lex.ntokens);

  err_t ret = parse_multi_user_stride (&parser, dest, dalloc, e);
  lex_free (&lex);
  return ret;
}

#ifndef NTEST
TEST (compile_multi_user_stride)
{
  error                    e      = error_create ();
  struct multi_user_stride stride = {0};
  struct chunk_alloc       alloc;
  chunk_alloc_create_default (&alloc);

  // -------------------------------------------------------------------------
  // Empty array
  // -------------------------------------------------------------------------
  TEST_CASE ("[ ]")
  {
    compile_multi_user_stride (&stride, "[]", &alloc, &e);
    test_assert_int_equal (stride.len, 0);
    test_assert_equal (stride.strides, NULL);
    compile_multi_user_stride (&stride, "[ ]", &alloc, &e);
    test_assert_int_equal (stride.len, 0);
    test_assert_equal (stride.strides, NULL);
    compile_multi_user_stride (&stride, " [ ]", &alloc, &e);
    test_assert_int_equal (stride.len, 0);
    test_assert_equal (stride.strides, NULL);
    compile_multi_user_stride (&stride, " [] ", &alloc, &e);
    test_assert_int_equal (stride.len, 0);
    test_assert_equal (stride.strides, NULL);
  }

  // -------------------------------------------------------------------------
  // Single bare index
  // -------------------------------------------------------------------------
  TEST_CASE ("[ 0 ]")
  {
    compile_multi_user_stride (&stride, "[0]", &alloc, &e);
    test_assert_int_equal (stride.len, 1);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (stride.strides[0].present, START_PRESENT);
    test_assert_int_equal (stride.strides[0].start, 0);
  }

  // -------------------------------------------------------------------------
  // Two bare indices
  // -------------------------------------------------------------------------
  TEST_CASE ("[ 0, 0 ]")
  {
    compile_multi_user_stride (&stride, "[0, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    // first
    test_assert_int_equal (stride.strides[0].present, START_PRESENT);
    test_assert_int_equal (stride.strides[0].start, 0);
    // second
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // -------------------------------------------------------------------------
  // start + colon, no stop, no step  →  "0:"
  // -------------------------------------------------------------------------
  TEST_CASE ("[ 0: ]")
  {
    compile_multi_user_stride (&stride, "[0:]", &alloc, &e);
    test_assert_int_equal (stride.len, 1);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (
        stride.strides[0].present,
        START_PRESENT | COLON_PRESENT
    );
    test_assert_int_equal (stride.strides[0].start, 0);
  }

  // -------------------------------------------------------------------------
  // "0:" followed by a bare index
  // -------------------------------------------------------------------------
  TEST_CASE ("[ 0:, 0 ]")
  {
    compile_multi_user_stride (&stride, "[0:, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (
        stride.strides[0].present,
        START_PRESENT | COLON_PRESENT
    );
    test_assert_int_equal (stride.strides[0].start, 0);
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // -------------------------------------------------------------------------
  // Colon only  →  ":"
  // -------------------------------------------------------------------------
  TEST_CASE ("[ :, 0 ]")
  {
    compile_multi_user_stride (&stride, "[:, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (stride.strides[0].present, COLON_PRESENT);
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // -------------------------------------------------------------------------
  // Colon + stop  →  ":0"
  // -------------------------------------------------------------------------
  TEST_CASE ("[ :0, 0 ]")
  {
    compile_multi_user_stride (&stride, "[:0, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (
        stride.strides[0].present,
        COLON_PRESENT | STOP_PRESENT
    );
    test_assert_int_equal (stride.strides[0].stop, 0);
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // -------------------------------------------------------------------------
  // Double colon, no numbers  →  "::"
  // parse_entry sets COLON_PRESENT, parse_step also matches ':' and sets it
  // again (no-op), no integers → only COLON_PRESENT in present
  // -------------------------------------------------------------------------
  TEST_CASE ("[ ::, 0 ]")
  {
    compile_multi_user_stride (&stride, "[::, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (stride.strides[0].present, COLON_PRESENT);
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // -------------------------------------------------------------------------
  // Double colon + step  →  "::0"
  // COLON_PRESENT (from first ':') | STEP_PRESENT (from "::0")
  // Note: no START_PRESENT, no STOP_PRESENT
  // -------------------------------------------------------------------------
  TEST_CASE ("[ ::0, 0 ]")
  {
    compile_multi_user_stride (&stride, "[::0, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (
        stride.strides[0].present,
        COLON_PRESENT | STEP_PRESENT
    );
    test_assert_int_equal (stride.strides[0].step, 0);
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // -------------------------------------------------------------------------
  // Colon + stop + colon, no step  →  ":0:"
  // -------------------------------------------------------------------------
  TEST_CASE ("[ :0:, 0 ]")
  {
    compile_multi_user_stride (&stride, "[:0:, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (
        stride.strides[0].present,
        COLON_PRESENT | STOP_PRESENT
    );
    test_assert_int_equal (stride.strides[0].stop, 0);
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // -------------------------------------------------------------------------
  // Colon + stop + colon + step  →  ":0:0"
  // -------------------------------------------------------------------------
  TEST_CASE ("[ :0:0, 0 ]")
  {
    compile_multi_user_stride (&stride, "[:0:0, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (
        stride.strides[0].present,
        COLON_PRESENT | STOP_PRESENT | STEP_PRESENT
    );
    test_assert_int_equal (stride.strides[0].stop, 0);
    test_assert_int_equal (stride.strides[0].step, 0);
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // -------------------------------------------------------------------------
  // Start + double colon, no stop/step  →  "0::"
  // -------------------------------------------------------------------------
  TEST_CASE ("[ 0::, 0 ]")
  {
    compile_multi_user_stride (&stride, "[0::, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (
        stride.strides[0].present,
        START_PRESENT | COLON_PRESENT
    );
    test_assert_int_equal (stride.strides[0].start, 0);
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // -------------------------------------------------------------------------
  // Start + double colon + step  →  "0::0"
  // -------------------------------------------------------------------------
  TEST_CASE ("[ 0::0, 0 ]")
  {
    compile_multi_user_stride (&stride, "[0::0, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (
        stride.strides[0].present,
        START_PRESENT | COLON_PRESENT | STEP_PRESENT
    );
    test_assert_int_equal (stride.strides[0].start, 0);
    test_assert_int_equal (stride.strides[0].step, 0);
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // -------------------------------------------------------------------------
  // Full slice  →  "0:0:0"
  // -------------------------------------------------------------------------
  TEST_CASE ("[ 0:0:0, 0 ]")
  {
    compile_multi_user_stride (&stride, "[0:0:0, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (
        stride.strides[0].present,
        START_PRESENT | COLON_PRESENT | STOP_PRESENT | STEP_PRESENT
    );
    test_assert_int_equal (stride.strides[0].start, 0);
    test_assert_int_equal (stride.strides[0].stop, 0);
    test_assert_int_equal (stride.strides[0].step, 0);
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // =========================================================================
  // Error conditions
  // =========================================================================

  // Missing opening bracket
  TEST_CASE ("error: no leading '['")
  {
    err_t err = compile_multi_user_stride (&stride, "0, 1]", &alloc, &e);
    test_assert (err < SUCCESS);
    test_assert_int_equal (e.cause_code, ERR_SYNTAX);
    e.cause_code = 0;
    e.cmlen      = 0;
  }

  // Missing closing bracket
  TEST_CASE ("error: no trailing ']'")
  {
    err_t err = compile_multi_user_stride (&stride, "[0, 1", &alloc, &e);
    test_assert (err < SUCCESS);
    test_assert_int_equal (e.cause_code, ERR_SYNTAX);
    e.cause_code = 0;
    e.cmlen      = 0;
  }

  // Empty input
  TEST_CASE ("error: empty string")
  {
    err_t err = compile_multi_user_stride (&stride, "", &alloc, &e);
    test_assert (err < SUCCESS);
    test_assert_int_equal (e.cause_code, ERR_SYNTAX);
    e.cause_code = 0;
    e.cmlen      = 0;
  }

  // Trailing comma with no entry after it  →  parse_entry gets ']', not a
  // number or ':', so it returns ERR_SYNTAX
  TEST_CASE ("error: trailing comma '[0,]'")
  {
    err_t err = compile_multi_user_stride (&stride, "[0,]", &alloc, &e);
    test_assert (err < SUCCESS);
    test_assert_int_equal (e.cause_code, ERR_SYNTAX);
    e.cause_code = 0;
    e.cmlen      = 0;
  }

  // Leading comma — parse_entry gets ',' which is neither number nor ':'
  TEST_CASE ("error: leading comma '[,0]'")
  {
    err_t err = compile_multi_user_stride (&stride, "[,0]", &alloc, &e);
    test_assert (err < SUCCESS);
    test_assert_int_equal (e.cause_code, ERR_SYNTAX);
    e.cause_code = 0;
    e.cmlen      = 0;
  }

  // Bare comma between two commas
  TEST_CASE ("error: double comma '[0,,1]'")
  {
    err_t err = compile_multi_user_stride (&stride, "[0,,1]", &alloc, &e);
    test_assert (err < SUCCESS);
    test_assert_int_equal (e.cause_code, ERR_SYNTAX);
    e.cause_code = 0;
    e.cmlen      = 0;
  }

  // Garbage token (not number, colon, comma, or bracket)
  TEST_CASE ("error: garbage token '[abc]'")
  {
    err_t err = compile_multi_user_stride (&stride, "[abc]", &alloc, &e);
    test_assert (err < SUCCESS);
    test_assert_int_equal (e.cause_code, ERR_SYNTAX);
    e.cause_code = 0;
    e.cmlen      = 0;
  }

  // Completely wrong structure
  TEST_CASE ("error: only a number, no brackets")
  {
    err_t err = compile_multi_user_stride (&stride, "42", &alloc, &e);
    test_assert (err < SUCCESS);
    test_assert_int_equal (e.cause_code, ERR_SYNTAX);
    e.cause_code = 0;
    e.cmlen      = 0;
  }

  chunk_alloc_free_all (&alloc);
}
#endif

err_t
compile_user_stride (struct user_stride *dest, const char *text, error *e)
{
  struct lexer lex;
  WRAP (lex_tokens (text, strlen (text), &lex, e));

  struct parser parser = parser_init (lex.tokens, lex.ntokens);

  err_t ret = parse_user_stride (&parser, dest, e);
  lex_free (&lex);
  return ret;
}

#ifndef NTEST
static void
test_compile_user_stride_green_path (
    const char        *query,
    struct user_stride expected
)
{
  struct user_stride actual;
  error              e = error_create ();

  TEST_CASE ("SHOULD PASS: %s", query)
  {
    compile_user_stride (&actual, query, &e);
    test_assert (user_stride_equal (&actual, &expected));
  }
}

static void
test_compile_user_stride_red_path (const char *query, err_t code)
{
  struct user_stride actual;
  error              e = error_create ();

  TEST_CASE ("SHOULD FAIL: %s", query)
  {
    test_err_t_check (compile_user_stride (&actual, query, &e), code, &e);
  }
}

TEST (compile_user_stride)
{
  test_compile_user_stride_red_path ("[]", ERR_SYNTAX);
  test_compile_user_stride_red_path ("[ ]", ERR_SYNTAX);
  test_compile_user_stride_red_path (" [ ]", ERR_SYNTAX);

  test_compile_user_stride_green_path ("[5]", ustride_single (5));

  test_compile_user_stride_green_path ("[:]", ustride ());
  test_compile_user_stride_green_path ("[:6]", ustride1 (6));
  test_compile_user_stride_green_path ("[5:]", ustride0 (5));
  test_compile_user_stride_green_path ("[5:6]", ustride01 (5, 6));

  test_compile_user_stride_green_path ("[::]", ustride ());
  test_compile_user_stride_green_path ("[::7]", ustride2 (7));
  test_compile_user_stride_green_path ("[:6:]", ustride1 (6));
  test_compile_user_stride_green_path ("[:6:7]", ustride12 (6, 7));

  test_compile_user_stride_green_path ("[5::]", ustride0 (5));
  test_compile_user_stride_green_path ("[5::7]", ustride02 (5, 7));
  test_compile_user_stride_green_path ("[5:6:]", ustride01 (5, 6));
  test_compile_user_stride_green_path ("[5:6:7]", ustride012 (5, 6, 7));
}
#endif

err_t
compile_type_ref (
    struct type_ref    *dest,
    const char         *text,
    struct chunk_alloc *dalloc,
    error              *e
)
{
  struct lexer lex;
  WRAP (lex_tokens (text, strlen (text), &lex, e));

  struct parser parser = parser_init (lex.tokens, lex.ntokens);

  err_t ret = parse_type_ref (&parser, dest, dalloc, e);

  lex_free (&lex);

  return ret;
}

#ifndef NTEST

static void
test_compile_type_ref_green_path (const char *query, struct type_ref expected)
{
  struct chunk_alloc alloc;
  struct type_ref    actual;
  chunk_alloc_create_default (&alloc);
  error e = error_create ();

  TEST_CASE ("SHOULD PASS: %s", query)
  {
    compile_type_ref (&actual, query, &alloc, &e);
    test_assert (type_ref_equal (expected, actual));
  }

  chunk_alloc_free_all (&alloc);
}

static void
test_compile_type_ref_red_path (const char *query, err_t code)
{
  struct chunk_alloc alloc;
  struct type_ref    actual;
  chunk_alloc_create_default (&alloc);
  error e = error_create ();

  TEST_CASE ("SHOULD FAIL: %s", query)
  {
    test_err_t_check (compile_type_ref (&actual, query, &alloc, &e), code, &e);
  }

  chunk_alloc_free_all (&alloc);
}

TEST (compile_type_ref)
{
  test_compile_type_ref_green_path (
      "myvar",
      tr_take (strfcstr ("myvar"), ta_take ())
  );

  test_compile_type_ref_green_path (
      "myvar[9]",
      tr_take (
          strfcstr ("myvar"),
          ta_range ((struct user_stride[]){ustride_single (9)}, 1, &ta_take ())
      )
  );

  test_compile_type_ref_green_path (
      "myvar.field",
      tr_take (strfcstr ("myvar"), ta_select (strfcstr ("field"), &ta_take ()))
  );

  struct type_accessor subrange =
      ta_range ((struct user_stride[]){ustride_single (0)}, 1, &ta_take ());
  test_compile_type_ref_green_path (
      "myvar.a[0]",
      tr_take (strfcstr ("myvar"), ta_select (strfcstr ("a"), &subrange))
  );

  test_compile_type_ref_green_path (
      "struct { a myvar }",
      tr_struct (
          1,
          (struct string[]){strfcstr ("a")},
          (struct type_ref[]){tr_take (strfcstr ("myvar"), ta_take ())}
      )
  );

  test_compile_type_ref_green_path (
      "struct { a x, b y }",
      tr_struct (
          2,
          (struct string[]){strfcstr ("a"), strfcstr ("b")},
          (struct type_ref[]){
              tr_take (strfcstr ("x"), ta_take ()),
              tr_take (strfcstr ("y"), ta_take ()),
          }
      )
  );

  test_compile_type_ref_green_path (
      "struct { a x, b y, c z }",
      tr_struct (
          3,
          (struct string[]){strfcstr ("a"), strfcstr ("b"), strfcstr ("c")},
          (struct type_ref[]){
              tr_take (strfcstr ("x"), ta_take ()),
              tr_take (strfcstr ("y"), ta_take ()),
              tr_take (strfcstr ("z"), ta_take ()),
          }
      )
  );

  test_compile_type_ref_green_path (
      "struct { a struct { b x } }",
      tr_struct (
          1,
          (struct string[]){strfcstr ("a")},
          (struct type_ref[]){
              tr_struct (
                  1,
                  (struct string[]){strfcstr ("b")},
                  (struct type_ref[]){tr_take (strfcstr ("x"), ta_take ())}
              ),
          }
      )
  );

  test_compile_type_ref_green_path (
      "struct { a myvar[0] }",
      tr_struct (
          1,
          (struct string[]){strfcstr ("a")},
          (struct type_ref[]){tr_take (
              strfcstr ("myvar"),
              ta_range (
                  (struct user_stride[]){ustride_single (0)},
                  1,
                  &ta_take ()
              )
          )}
      )
  );

  test_compile_type_ref_red_path ("", ERR_SYNTAX);
  test_compile_type_ref_red_path ("42", ERR_SYNTAX);
  test_compile_type_ref_red_path ("[0]", ERR_SYNTAX);
  test_compile_type_ref_red_path ("struct a x }", ERR_SYNTAX);
  test_compile_type_ref_red_path ("struct {}", ERR_SYNTAX);
  test_compile_type_ref_red_path ("struct { a }", ERR_SYNTAX);
  test_compile_type_ref_red_path ("struct { a x", ERR_SYNTAX);
  test_compile_type_ref_red_path ("struct { a x, }", ERR_SYNTAX);
  test_compile_type_ref_red_path ("struct { a x,, b y }", ERR_SYNTAX);
  test_compile_type_ref_red_path ("struct { a x, b }", ERR_SYNTAX);
  test_compile_type_ref_red_path ("struct { a x, struct y }", ERR_SYNTAX);
}

#endif

#ifndef NTEST

static void
test_compile_type_green_path (const char *query, struct type expected)
{
  struct chunk_alloc alloc;
  struct type        actual;
  chunk_alloc_create_default (&alloc);
  error e = error_create ();

  TEST_CASE ("SHOULD PASS: %s", query)
  {
    compile_type (&actual, query, &alloc, &e);
    test_assert (type_equal (&expected, &actual));
  }

  chunk_alloc_free_all (&alloc);
}

static void
test_compile_type_red_path (const char *query, err_t code)
{
  struct chunk_alloc alloc;
  struct type        actual;
  chunk_alloc_create_default (&alloc);
  error e = error_create ();

  TEST_CASE ("SHOULD FAIL: %s", query)
  {
    test_err_t_check (compile_type (&actual, query, &alloc, &e), code, &e);
  }

  chunk_alloc_free_all (&alloc);
}

TEST (compile_type)
{
  test_compile_type_green_path ("i8", TI8);
  test_compile_type_green_path ("i16", TI16);
  test_compile_type_green_path ("i32", TI32);
  test_compile_type_green_path ("i64", TI64);

  test_compile_type_green_path ("u8", TU8);
  test_compile_type_green_path ("u16", TU16);
  test_compile_type_green_path ("u32", TU32);
  test_compile_type_green_path ("u64", TU64);

  test_compile_type_green_path ("f16", TF16);
  test_compile_type_green_path ("f32", TF32);
  test_compile_type_green_path ("f64", TF64);
  test_compile_type_green_path ("f128", TF128);

  test_compile_type_green_path ("cf32", TCF32);
  test_compile_type_green_path ("cf64", TCF64);
  test_compile_type_green_path ("cf128", TCF128);
  test_compile_type_green_path ("cf256", TCF256);

  test_compile_type_red_path ("i2", ERR_SYNTAX);

  // SARRAY
  test_compile_type_green_path ("[10]i32", mk_sarray (1, (u32[]){10}, &TI32));
  test_compile_type_green_path (
      "[5][10]f64",
      mk_sarray (2, (u32[]){5, 10}, &TF64)
  );
  test_compile_type_green_path (
      "[2][3][4]u8",
      mk_sarray (3, (u32[]){2, 3, 4}, &TU8)
  );

  // STRUCT
  test_compile_type_green_path (
      "struct { x i32, y f64 }",
      mk_struct (
          2,
          (struct string[]){
              strfcstr ("x"),
              strfcstr ("y"),
          },
          (struct type *[]){
              &TI32,
              &TF64,
          }
      )
  );

  struct type inner = mk_struct (
      1,
      (struct string[]){strfcstr ("b")},
      (struct type *[]){&TI32}
  );
  test_compile_type_green_path (
      "struct { a struct { b i32 } }",
      mk_struct (
          1,
          (struct string[]){strfcstr ("a")},
          (struct type *[]){&inner}
      )
  );

  // UNION
  test_compile_type_green_path (
      "union { x i32, y f64 }",
      mk_union (
          2,
          (struct string[]){
              strfcstr ("x"),
              strfcstr ("y"),
          },
          (struct type *[]){
              &TI32,
              &TF64,
          }
      )
  );

  // COMPLICATED
  struct type inner_sarray = mk_sarray (1, (u32[]){5}, &TF64);
  struct type inner_struct = mk_struct (
      1,
      (struct string[]){strfcstr ("x"), strfcstr ("y")},
      (struct type *[]){&TI32, &inner_sarray}
  );
  test_compile_type_green_path (
      "[10]struct { x i32, y [5]f64 }",
      mk_sarray (1, (u32[]){10}, &inner_struct)
  );
}

#endif

err_t
compile_query (
    struct query       *dest,
    const char         *text,
    struct chunk_alloc *dalloc,
    error              *e
)
{
  struct lexer lex;
  WRAP (lex_tokens (text, strlen (text), &lex, e));

  struct parser parser = parser_init (lex.tokens, lex.ntokens);

  err_t ret = parse_query (&parser, dest, dalloc, e);
  lex_free (&lex);
  return ret;
}

#ifndef NTEST

static void
test_query_green_path (const char *query, struct query expected)
{
  struct chunk_alloc alloc;
  chunk_alloc_create_default (&alloc);
  struct query actual;
  error        e = error_create ();

  TEST_CASE ("SHOULD PASS: %s", query)
  {
    compile_query (&actual, query, &alloc, &e);
    test_assert (query_equal (&actual, &expected));
  }

  chunk_alloc_free_all (&alloc);
}

static void
test_query_red_path (const char *query, err_t code)
{
  struct chunk_alloc alloc;
  chunk_alloc_create_default (&alloc);
  struct query actual;
  error        e = error_create ();

  TEST_CASE ("SHOULD FAIL: %s", query)
  {
    test_err_t_check (compile_query (&actual, query, &alloc, &e), code, &e);
  }

  chunk_alloc_free_all (&alloc);
}

TEST (compile_query)
{
  // READ
  {
    test_query_red_path ("read", ERR_SYNTAX);
    test_query_red_path ("read;", ERR_SYNTAX);
    test_query_green_path (
        "read foo;",
        (struct query){.type = QT_READ,
                       .read = {.vname = strfcstr ("foo"), .ustr = ustride ()}}
    );
    test_query_green_path (
        "read foo[0:10:20];",
        (struct query){
            .type = QT_READ,
            .read = {.vname = strfcstr ("foo"), .ustr = ustride012 (0, 10, 20)},
        }
    );
  }

  // CREATE
  {
    test_query_red_path ("create", ERR_SYNTAX);
    test_query_red_path ("create 1;", ERR_SYNTAX);
    test_query_red_path ("create foo;", ERR_SYNTAX);
    test_query_red_path ("create foo 1;", ERR_SYNTAX);
    test_query_red_path ("create a i32", ERR_SYNTAX);

    // Sarray
    u32         dims[2] = {10, 20};
    struct type t0      = {
             .type = T_SARRAY,
             .sa   = {.rank = 2, .dims = dims, .t = &TF32}
    };

    // Union
    struct type  *utypes[2] = {&TI32, &t0};
    struct string ukeys[2]  = {strfcstr ("c"), strfcstr ("d")};
    struct type   t1        = {
                 .type = T_UNION,
                 .un =
            {
                         .len   = 2,
                         .types = utypes,
                         .keys  = ukeys,
            },
    };

    // Struct
    struct type  *stypes[2] = {&TI32, &t1};
    struct string skeys[2]  = {strfcstr ("a"), strfcstr ("b")};
    struct type   t2        = {
                 .type = T_STRUCT,
                 .st =
            {
                         .len   = 2,
                         .types = stypes,
                         .keys  = skeys,
            },
    };
    test_query_green_path (
        "create foo struct { a i32, b union { c i32, d [10][20]f32 } };",
        (struct query){
            .type = QT_CREATE,
            .create =
                {
                    .vname = strfcstr ("foo"),
                    .type  = &t2,
                },
        }
    );
  }

  // DELETE
  {
    test_query_red_path ("delete", ERR_SYNTAX);
    test_query_red_path ("delete 1;", ERR_SYNTAX);
    test_query_red_path ("delete foo", ERR_SYNTAX);
    test_query_green_path (
        "delete foo;",
        (struct query){
            .type = QT_DELETE,
            .delete =
                {
                    .vname = strfcstr ("foo"),
                },
        }
    );
  }

  // GET
  {
    test_query_red_path ("get", ERR_SYNTAX);
    test_query_red_path ("get 1;", ERR_SYNTAX);
    test_query_red_path ("get foo", ERR_SYNTAX);
    test_query_green_path (
        "get foo;",
        (struct query){
            .type = QT_GET,
            .get =
                {
                    .vname = strfcstr ("foo"),
                },
        }
    );
  }

  {
    test_query_red_path ("exit", ERR_SYNTAX);
    test_query_red_path ("exit 1;", ERR_SYNTAX);
    test_query_red_path ("exit foo", ERR_SYNTAX);
    test_query_green_path (
        "exit;",
        (struct query){
            .type = QT_EXIT,
        }
    );
  }

  {
    test_query_red_path ("help", ERR_SYNTAX);
    test_query_red_path ("help 1;", ERR_SYNTAX);
    test_query_red_path ("help foo", ERR_SYNTAX);
    test_query_red_path ("help read", ERR_SYNTAX);
    test_query_red_path ("help foo;", ERR_SYNTAX);
    test_query_green_path (
        "help;",
        (struct query){
            .type = QT_HELP,
            .help = {.has_command = false},
        }
    );
    test_query_green_path (
        "help read;",
        (struct query){
            .type = QT_HELP,
            .help = {.has_command = true, .command = QT_READ},
        }
    );
    test_query_green_path (
        "help create;",
        (struct query){
            .type = QT_HELP,
            .help = {.has_command = true, .command = QT_CREATE},
        }
    );
    test_query_green_path (
        "help delete;",
        (struct query){
            .type = QT_HELP,
            .help = {.has_command = true, .command = QT_DELETE},
        }
    );
    test_query_green_path (
        "help get;",
        (struct query){
            .type = QT_HELP,
            .help = {.has_command = true, .command = QT_GET},
        }
    );
    test_query_green_path (
        "help exit;",
        (struct query){
            .type = QT_HELP,
            .help = {.has_command = true, .command = QT_EXIT},
        }
    );
    test_query_green_path (
        "help help;",
        (struct query){
            .type = QT_HELP,
            .help = {.has_command = true, .command = QT_HELP},
        }
    );
  }
}
#endif
