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

#include "nscore/lexer.h"

#include "nscore/errors.h"
#include "nscore/tokens.h"
#include "nscore/types.h"

static bool
is_at_end (struct lexer *lex)
{ return lex->current >= lex->src_len; }

static char
peek (struct lexer *lex)
{
  if (is_at_end (lex)) { return '\0'; }
  return lex->src[lex->current];
}

static char
peek_next (struct lexer *lex)
{
  if (lex->current + 1 >= lex->src_len) { return '\0'; }
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
  if (is_at_end (lex)) { return false; }
  if (lex->src[lex->current] != expected) { return false; }

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
  struct token next = (struct token){
      .type       = TT_FLOAT,
      .floating   = value,
      .text_start = &lex->src[lex->start],
      .text_len   = lex->current - lex->start
  };

  return dblb_append (&lex->_tokens, &next, 1, e);
}

static err_t
add_token_str (struct lexer *lex, enum token_t type, const char *data, u32 len, error *e)
{
  struct token next = (struct token){
      .type = type,
      .str =
          {
              .data = data,
              .len  = len,
          },
      .text_start = &lex->src[lex->start],
      .text_len   = lex->current - lex->start
  };

  return dblb_append (&lex->_tokens, &next, 1, e);
}

static err_t
add_token_prim (struct lexer *lex, enum prim_t prim, error *e)
{
  struct token next = (struct token){
      .type       = TT_PRIM,
      .prim       = prim,
      .text_start = &lex->src[lex->start],
      .text_len   = lex->current - lex->start
  };

  return dblb_append (&lex->_tokens, &next, 1, e);
}

static enum token_t
check_keyword (const char *text, u32 len)
{
  if (len == sizeof ("create") - 1 && strncmp (text, "create", len) == 0) { return TT_CREATE; }
  if (len == sizeof ("delete") - 1 && strncmp (text, "delete", len) == 0) { return TT_DELETE; }
  if (len == sizeof ("insert") - 1 && strncmp (text, "insert", len) == 0) { return TT_INSERT; }
  if (len == sizeof ("append") - 1 && strncmp (text, "append", len) == 0) { return TT_APPEND; }
  if (len == sizeof ("read") - 1 && strncmp (text, "read", len) == 0) { return TT_READ; }
  if (len == sizeof ("write") - 1 && strncmp (text, "write", len) == 0) { return TT_WRITE; }
  if (len == sizeof ("remove") - 1 && strncmp (text, "remove", len) == 0) { return TT_REMOVE; }
  if (len == sizeof ("take") - 1 && strncmp (text, "take", len) == 0) { return TT_TAKE; }

  if (len == sizeof ("file") - 1 && strncmp (text, "file", len) == 0) { return TT_FILE; }
  if (len == sizeof ("query") - 1 && strncmp (text, "query", len) == 0) { return TT_QUERY; }
  if (len == sizeof ("ofst") - 1 && strncmp (text, "ofst", len) == 0) { return TT_OFST; }
  if (len == sizeof ("len") - 1 && strncmp (text, "len", len) == 0) { return TT_LEN; }

  if (len == sizeof ("struct") - 1 && strncmp (text, "struct", len) == 0) { return TT_STRUCT; }
  if (len == sizeof ("union") - 1 && strncmp (text, "union", len) == 0) { return TT_UNION; }

  if (len == sizeof ("true") - 1 && strncmp (text, "true", len) == 0) { return TT_TRUE; }
  if (len == sizeof ("false") - 1 && strncmp (text, "false", len) == 0) { return TT_FALSE; }

  if (len == sizeof ("as") - 1 && strncmp (text, "as", len) == 0) { return TT_AS; }

  return TT_IDENTIFIER;
}

static err_t
scan_string (struct lexer *lex, error *e)
{
  while (!is_at_end (lex) && peek (lex) != '"') { advance (lex); }

  if (is_at_end (lex))
  {
    return error_causef (e, ERR_SYNTAX, "Unterminated string at position %u", lex->start);
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
  while (!is_at_end (lex) && is_num (peek (lex))) { advance (lex); }

  bool is_float = false;
  if (peek (lex) == '.' && is_num (peek_next (lex)))
  {
    is_float = true;
    advance (lex);

    while (!is_at_end (lex) && is_num (peek (lex))) { advance (lex); }
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
  while (!is_at_end (lex) && is_alpha_num (peek (lex))) { advance (lex); }

  const char *text = &lex->src[lex->start];
  u32         len  = lex->current - lex->start;

  // Check for primitive types first
  enum prim_t prim = strtoprim (text, len);
  if (prim != (enum prim_t) - 1) { return add_token_prim (lex, prim, e); }

  // Check for keywords
  enum token_t type = check_keyword (text, len);

  if (type == TT_IDENTIFIER) { return add_token_str (lex, TT_IDENTIFIER, text, len, e); }
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
      return add_token (lex, match (lex, '=') ? TT_GREATER_EQUAL : TT_GREATER, e);
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
      return add_token (lex, match (lex, '&') ? TT_AMPERSAND_AMPERSAND : TT_AMPERSAND, e);
    }
    case '"':
    {
      return scan_string (lex, e);
    }

    default:
    {
      if (is_num (c)) { return scan_number (lex, e); }

      if (is_alpha (c)) { return scan_identifier (lex, e); }

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
    if (scan_token (lex, e)) { goto failed; }
  }

  if (add_token (lex, TT_EOF, e)) { goto failed; }

  lex->ntokens = lex->_tokens.nelem;
  lex->tokens  = lex->_tokens.data;

  return SUCCESS;

failed:
  dblb_free (&lex->_tokens);
  return error_trace (e);
}

void
lex_free (struct lexer *lex)
{ dblb_free (&lex->_tokens); }

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
  const char *src = "create delete insert file query struct union true false";

  struct token expected[] = {
      quick_tok (TT_CREATE),
      quick_tok (TT_DELETE),
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
